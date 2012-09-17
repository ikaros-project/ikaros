//
//	Perceptron.cc	This file is a part of the IKAROS project
// 					Single layer of perceptrons.
//
//    Copyright (C) 2007 Alexander Kolodziej
//
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program; if not, write to the Free Software
//    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//    See http://www.ikaros-project.org/ for more information.
//
//	Created: June 2007
//
//	Single layer of perceptrons.


#include "Perceptron.h"
#include <math.h>

using namespace ikaros;


Module *
Perceptron::Create(Parameter * p)
{
	return new Perceptron(p);
}



Perceptron::Perceptron(Parameter * p): Module(p)
{
    AddInput("INPUT");
    AddInput("T_INPUT");
    AddInput("T_TARGET");
    AddOutput("OUTPUT", unknown_size);

    AddOutput("ERROR", 1);
    AddOutput("CORRECT", 1);

    AddInput("TRAIN");
}



void
Perceptron::SetSizes()
{
    int tmp;

    input_size = GetInputSize("INPUT");
    if (input_size == unknown_size)
        return;

    tmp = GetInputSize("T_INPUT");
    if (tmp == unknown_size)
        return;
    if (tmp != input_size)
        Notify(msg_fatal_error, "Module Perceptron - sizes of inputs INPUT (size %i) and T_INPUT (size %i) do not match!\n", input_size, tmp);

    target_size = GetInputSize("T_TARGET");
    if (target_size == unknown_size)
        return;
    if (target_size < 1)
        Notify(msg_fatal_error, "Module Perceptron - size of input TARGET is less than 1 (it is %i)!\n", target_size);

    tmp = GetInputSize("TRAIN");
    if (tmp == unknown_size)
        return;
    if (tmp != 1)
        Notify(msg_fatal_error, "Module Perceptron - size of input TRAIN is not 1 (it is %i)!\n", tmp);

    SetOutputSize("OUTPUT", target_size);
}



void
Perceptron::Init()
{
    int i;
    float rand_weights_min, rand_weights_max;

    Notify(msg_verbose, "Module Perceptron - perceptrons: %i\tweights: %i (incl bias)\n", target_size, input_size + 1);

    train_tick = 0;

    // amount of weights is the same as amount of inputs (dont forget the bias, thats why its +1)
    // amount of perceptrons is the same as amount of targets. each perceptron has one output...
    perceptron_layer = create_matrix(input_size + 1, target_size);

    // allocate the array to which the inputs (with the bias appended) will be copied
    input_with_bias = create_array(input_size + 1);

    // allocate array that keeps the errors of each perceptron at any given tick
    perceptron_errors = create_array(target_size);

    // fetch all the parameters
    rand_weights_min = GetFloatValue("rand_weights_min", -0.5);
    rand_weights_max = GetFloatValue("rand_weights_max", 0.5);
    learning_rate = GetFloatValue("learning_rate", 0.1);
    learning_rate_mod = GetIntValueFromList("learning_rate_mod");
    bias = GetFloatValue("bias", 1);
    momentum_ratio = GetFloatValue("momentum_ratio", 0.42);
    activation_type = GetIntValueFromList("activation_type", "default/step/sign/sigmoid/tanh");
    learning_rule = GetIntValueFromList("learning_rule", "default/rosenblatt/rosenblatt_margin/may/alpha_lms/mu_lms/delta");
    learning_type = GetIntValueFromList("learning_type", "default/instant/batch/momentum");
    batch_size = GetIntValue("batch_size", 42);
    step_threshold = GetFloatValue("step_threshold", 0);
    margin = GetFloatValue("margin", 0.2);
    alpha = GetFloatValue("alpha", 0.1);
    mu = GetFloatValue("mu", 0.1);
    beta = GetFloatValue("beta", 1.0);
    correct_average_size = GetIntValue("correct_average_size", 42);
    correct_all = create_array(correct_average_size);
    normalize_target = GetBoolValue("normalize_target", false);

    // also checks the other parameters, but they are members of the object so no need to pass them
    CheckParameters(rand_weights_min, rand_weights_max);

    // current memory is where the calculated changes to from this tick are stored.
    // how they are added to the network itself depends on the learning_type,
    // instant, batch or momentum...
    current_memory = create_matrix(input_size + 1, target_size);

    // with the delta rule the weight should be set to 0 instead of being randomized
    // in the start. correct???
    if (learning_rule == DELTA)
        for (i = 0; i < target_size; i++)
            reset_array(perceptron_layer[i], input_size + 1);
    else
        for (i = 0; i < target_size; i++)
            random(perceptron_layer[i], rand_weights_min, rand_weights_max, input_size + 1);

    // set the bias at the last position in the extended input array. it is not changed here after.
    input_with_bias[input_size] = bias;

    input = GetInputArray("INPUT");
    t_input = GetInputArray("T_INPUT");
    t_target = GetInputArray("T_TARGET");
    train = GetInputArray("TRAIN");

    output = GetOutputArray("OUTPUT");
    error = GetOutputArray("ERROR");
    correct = GetOutputArray("CORRECT");
    correct[0] = 0;

    // if lerning_type is momentum...
    // wee need some adjustments that can be considered being from the last tick even for the
    // first tick, thats why we do one learning here. note that this doesnt alter the perceptrons
    // weights, it only stores adjustment vectors to current_memory, which is merged with
    // the perceptrons weights (perceptron_layer[i]) later.
    if (learning_type == MOMENTUM)
        (this->*LearnUpdate)();
}


Perceptron::~Perceptron()
{
    destroy_matrix(perceptron_layer);
    destroy_array(input_with_bias);
    destroy_array(perceptron_errors);
    destroy_array(correct_all);
    destroy_matrix(current_memory);
    if (learning_type == BATCH)
        destroy_matrix(batch_memory);
    if (learning_type == MOMENTUM)
        destroy_matrix(momentum_memory);
    if (learning_rule == ROSENBLATT_MARGIN || learning_rule == MAY ||
        learning_rule == ALPHA_LMS || learning_rule == MU_LMS || learning_rule == DELTA)
        destroy_array(tmp_memory);
}



void
Perceptron::Tick()
{
    if (normalize_target)
        NormalizeTarget();

    // the ->Activate() function sets the output of the module.
    // so if we want to also train, first activate and learn with
    // the t_input/t_target arrays, and then reactivate with the
    // input array.
    if (train[0] == 1){
        train_tick++;
        copy_array(input_with_bias, t_input, input_size);
        (this->*Activate)();
        Learn();
        CalculateErrorAndCorrect();
    }

    // (re)activate with the input array
    copy_array(input_with_bias, input, input_size);
    (this->*Activate)();

}



inline void
Perceptron::Learn()
{
    if (learning_rate_mod != DEFAULT)
        ModifyLearningRate();

    // note that LearnUpdate() is what calculates this ticks updates to the array current_memory.
    // LearnUpdate() is a pointer to one of the learning rules. it does NOT alter the perceptron(s)
    // directly. thats done after LearnUpdate() has returned, either with the quick add() when
    // learning_type is INSTANT, or by one of the DoBatch() or DoMomentum() functions.
    // thats why in the case of momentum we need to store away the array current_memory to some other
    // memory first, since those are the values from the previous tick, and we need those.
    switch (learning_type){
        case INSTANT:
            (this->*LearnUpdate)();
            add(perceptron_layer, current_memory, input_size + 1, target_size);
            break;
        case BATCH:
            // only misclassified examples are handled
            if (correct_now)
                return;
            (this->*LearnUpdate)();
            DoBatch();
            break;
        case MOMENTUM:
            copy_matrix(momentum_memory, current_memory, input_size + 1, target_size);
            (this->*LearnUpdate)();
            DoMomentum();
            break;
    }
}

inline void
Perceptron::DoBatch()
{
    batch_errors_seen++;
    add(batch_memory, current_memory, input_size + 1, target_size);

    // if the batch is full, update and reset some stuff.
    if (batch_errors_seen == batch_size){
        add(perceptron_layer, batch_memory, input_size + 1, target_size);
        reset_matrix(batch_memory, input_size + 1, target_size);
        batch_errors_seen = 0;
    }
}

inline void
Perceptron::DoMomentum()
{
    // the weight updates in current_memory are multiplied with m_ratio_1
    // which is equal to 1 - momentum_ratio, since this is what we want to keep
    // from this ticks weight updates.
    multiply(current_memory, m_ratio_1, input_size + 1, target_size);

    // however, the weight updates in momentum_memory were just recently
    // copied from current_memory, where they were multiplied with
    // m_ratio_1 one tick ago, so we cant just multiply them with
    // momentum_ratio. they need to first be divided by m_ratio_1
    // and then multiplied with momentum_ratio, which is the same as
    // multiplying the original weight updates with momentum_ratio.
    // ( i.e. m_ratio_2 = momentum_ratio / m_ratio_1; )
    multiply(momentum_memory, m_ratio_2, input_size + 1, target_size);

    add(perceptron_layer, current_memory, input_size + 1, target_size);
    add(perceptron_layer, momentum_memory, input_size + 1, target_size);
}


void // (3.1.2)
Perceptron::LearnRosenblatt()
{
    int pi; // perceptron index
    for (pi = 0; pi < target_size; pi++)
        // variable matching between the code and the formula in Hassoun's book
        // input_with_bias = x ; learning_rate_now = p ; t_target[pi] = d ; output[pi] = y
        multiply(current_memory[pi], input_with_bias, learning_rate_now * (t_target[pi] - output[pi]) , input_size + 1);
}

void // (3.1.16)
Perceptron::LearnRosenblattMargin()
{
    int pi; // perceptron index
    float tmp;
    for (pi = 0; pi < target_size; pi++){
        // variable matching between the code and the formula in Hassoun's book
        // tmp_memory = z ; input_with_bias = x; t_target[pi] = d    (SEE 3.1.4)
        // NOTE: if t_target contains anything else than -1.0 and 1.0 this
        // learning rule does NOT work!!!
        multiply(tmp_memory, input_with_bias, t_target[pi], input_size + 1);
        // calculate z*weights
        tmp = dot(tmp_memory, perceptron_layer[pi], input_size + 1);
        if (tmp <= margin)
            // store p*z in current_memory
            multiply(current_memory[pi], tmp_memory, learning_rate_now, input_size + 1);
    }
}

void // (3.1.29)
Perceptron::LearnMay()
{
    int pi; // perceptron index
    float tmp, tmp2;
    for (pi = 0; pi < target_size; pi++){
        // variable matching between the code and the formula in Hassoun's book
        // tmp_memory = z ; input_with_bias = x; t_target[pi] = d    also see 3.1.4
        // NOTE: if t_target contains anything else than -1.0 and 1.0 this
        // learning rule does NOT work!!!
        multiply(tmp_memory, input_with_bias, t_target[pi], input_size + 1);
        // calculate z*weights
        tmp = dot(tmp_memory, perceptron_layer[pi], input_size + 1);
        if (tmp <= margin){
            // calculate p * "the division"
            tmp2 = learning_rate_now * ((margin - tmp) / dot(tmp_memory, tmp_memory, input_size + 1));
            // multiply "p * the division" with z and store it in current_memory
            multiply(current_memory[pi], tmp_memory, tmp2, input_size + 1);
        }
    }
}

void // (3.1.30)
Perceptron::LearnAlpha_LMS()
{
    int pi; // perceptron index
    float tmp;
    for (pi = 0; pi < target_size; pi++){
        // store alpha * (d - y) / "the divisor in the division (||x||^2)"
        tmp = (alpha * (t_target[pi] - output[pi])) / dot(input_with_bias, input_with_bias, input_size + 1);
        // multiply tmp with the divident (x)in the division and store in current_memory
        multiply(current_memory[pi], input_with_bias, tmp, input_size + 1);
    }
}

void // (3.1.35)
Perceptron::LearnMu_LMS()
{
    int pi; // perceptron index
    float tmp;
    for (pi = 0; pi < target_size; pi++){
        tmp = t_target[pi] - output[pi];
        multiply(current_memory[pi], input_with_bias, mu * tmp, input_size + 1);
    }
}

void // (3.1.53)
Perceptron::LearnDelta()
{
    int pi; // perceptron index
    float tmp;

    for (pi = 0; pi < target_size; pi++){
        switch (activation_type){
            case TANH:
                // should the last output[pi] here be multiplied by itself???
                // när jag kollade igenom den här regeln nu så hade jag inte skrivit att
                // den skulle multipliceras med sig själv. vet inte varför, mest förmodligen
                // bara ett fel. nu är den ^2 som det står i boken, om jag inte läst fel.
                // provkörningar har inte gett några vettiga indikationer, ibland funkar output^2
                // i slutet bättre, ibland inte. men det ska nog vara som det står nu, eller?
                tmp = learning_rate * (t_target[pi] - output[pi]) * (beta * (1 - (output[pi]*output[pi])));
                multiply(current_memory[pi], input_with_bias, tmp, input_size + 1);
                break;
            case SIGMOID:
                tmp = learning_rate * (t_target[pi] - output[pi]) * (beta * output[pi] * (1 - output[pi]));;
                multiply(current_memory[pi], input_with_bias, tmp, input_size + 1);
                break;
        }
    }
}


void
Perceptron::ActivateStep()
{
    int pi; // perceptron index
    float tmp;
    correct_now = 1;
    for (pi = 0; pi < target_size; pi++){
        tmp = dot(input_with_bias, perceptron_layer[pi], input_size + 1);
        output[pi] = tmp > step_threshold ? 1: 0;
        if (t_target[pi] != output[pi])
            correct_now = 0;
    }
}

void
Perceptron::ActivateSign()
{
    int pi; // perceptron index
    float tmp;
    correct_now = 1;
    for (pi = 0; pi < target_size; pi++){
        tmp = dot(input_with_bias, perceptron_layer[pi], input_size + 1);
        output[pi] = tmp > 0 ? 1: -1;
        if (t_target[pi] != output[pi])
            correct_now = 0;
    }
}

void
Perceptron::ActivateSigmoid()
{
    int pi; // perceptron index
    float tmp;
    correct_now = 1;
    for (pi = 0; pi < target_size; pi++){
        tmp = dot(input_with_bias, perceptron_layer[pi], input_size + 1);
        output[pi] = 1 / (1 + ikaros::pow(2.71828183, -tmp));
        if (((tmp<=0.5) && (t_target[pi]==1))  ||  ((tmp>0.5) && (t_target[pi]==0)))
            correct_now = 0;
    }
}

void
Perceptron::ActivateTanh()
{
    int pi; // perceptron index
    float tmp;
    correct_now = 1;
    for (pi = 0; pi < target_size; pi++){
        tmp = dot(input_with_bias, perceptron_layer[pi], input_size + 1);
        output[pi] = tanhf(tmp);
        if (((tmp<=0) && (t_target[pi]==1))  ||  ((tmp>0) && (t_target[pi]==-1)))
            correct_now = 0;
    }
}


inline void
Perceptron::ModifyLearningRate()
{
    switch (learning_rate_mod){
        case SQRT:
            learning_rate_now = learning_rate / (0.10 * ikaros::sqrt(train_tick + 100));
            break;
        case LOG:
            learning_rate_now = learning_rate / (0.42 * ikaros::log(train_tick + 10));
            break;
    }
}


inline void
Perceptron::NormalizeTarget()
{
    int pi;

    switch (activation_type){
        case STEP:
        case SIGMOID:
            for (pi = 0; pi < target_size; pi++)
                if (t_target[pi] <= 0.0)
                    t_target[pi] = 0.0;
                else
                    t_target[pi] = 1.0;
            break;
        case SIGN:
        case TANH:
            for (pi = 0; pi < target_size; pi++)
                if (t_target[pi] <= 0.0)
                    t_target[pi] = -1.0;
                else
                    t_target[pi] = 1.0;
            break;
    }
}


void
Perceptron::CalculateErrorAndCorrect()
{
    int tmp;

    // first the ERROR-output
    perceptron_errors = ikaros::subtract(perceptron_errors, t_target, output, target_size);
    perceptron_errors = ikaros::sqr(perceptron_errors, target_size);
    error[0] = ikaros::add(perceptron_errors, target_size);
    error[0] = ikaros::sqrt(error[0]);

    // next the CORRECT-output
    tmp = (train_tick-1) % correct_average_size; // index in batch
    correct_all[tmp] = correct_now;
    // then tmp is the size of batch. in the beginning the batch grows until it
    // reaches its desired size. we want correct statistics from the start!
    tmp = train_tick < correct_average_size ? train_tick: correct_average_size;
    correct[0] = add(correct_all, tmp) / tmp;
}


void
Perceptron::CheckParameters(float rw_min, float rw_max)
{
    if (learning_rate < 0.001)
        Notify(msg_fatal_error, "Module Perceptron - faulty value '0' to parameter learning_rate.\n");
    Notify(msg_verbose, "Module Perceptron - learning_rate is '%.2f'\n", learning_rate);
    switch (learning_rate_mod){
        case DEFAULT:
            learning_rate_now = learning_rate;
            Notify(msg_verbose, "Module Perceptron - learning_rate_mod is \"none\". learning_rate unmodified.\n");
            break;
        case SQRT:
            Notify(msg_verbose, "Module Perceptron - learning_rate_mod is 'sqrt'\n");
            break;
        case LOG:
            Notify(msg_verbose, "Module Perceptron - learning_rate_mod is 'log'\n");
            break;
        default:
            Notify(msg_fatal_error, "Module Perceptron - Weirdo value for learning_rate_mod (%i)\n", learning_rate_mod);
    }

    if (bias == 0)
        Notify(msg_warning, "Module Perceptron - faulty value '0' for parameter bias?\n");
    Notify(msg_verbose, "Module Perceptron - bias is '%.2f'\n", bias);

    if (rw_min >= rw_max)
        Notify(msg_fatal_error, "Module Perceptron - 'rand_weights_min' is not smaller than 'rand_weights_max'.\n");
    if (rw_min < -1 || rw_max > 1)
        Notify(msg_warning, "Module Perceptron - Really have the weights randomized outside interval -1 to 1?\n");
    Notify(msg_verbose, "Module Perceptron - rand_weights_min=%.2f\trand_weights_max=%.2f\n", rw_min, rw_max);

    if (correct_average_size < 1)
        Notify(msg_fatal_error, "Module Perceptron - correct_average_size should be at least 1. (was %i).\n", correct_average_size);

    if (normalize_target == YES)
        Notify(msg_verbose, "Module Perceptron - will normalize target!\n");

    switch (learning_rule){
        case DEFAULT:
            Notify(msg_verbose, "Module Perceptron - learning_rule defaults to 'rosenblatt'\n");
            learning_rule = ROSENBLATT;
            LearnUpdate = &Perceptron::LearnRosenblatt;
            break;
        case ROSENBLATT:
            Notify(msg_verbose, "Module Perceptron - learning_rule is 'rosenblatt'\n");
            LearnUpdate = &Perceptron::LearnRosenblatt;
            break;
        case ROSENBLATT_MARGIN:
            Notify(msg_verbose, "Module Perceptron - learning_rule is 'rosenblatt_margin'\n");
            Notify(msg_verbose, "Module Perceptron - margin is '%.2f'\n", margin);
            tmp_memory = create_array(input_size + 1);
            LearnUpdate = &Perceptron::LearnRosenblattMargin;
            break;
        case MAY:
            Notify(msg_verbose, "Module Perceptron - learning_rule is 'may'\n");
            Notify(msg_verbose, "Module Perceptron - margin is '%.2f'\n", margin);
            tmp_memory = create_array(input_size + 1);
            LearnUpdate = &Perceptron::LearnMay;
            break;
        case ALPHA_LMS:
            Notify(msg_verbose, "Module Perceptron - learning_rule is 'alpha_lms'\n");
            if (alpha <= 0 || alpha >= 2)
                Notify(msg_fatal_error, "Module Perceptron - alpha has nonsens value '%.2f'\n", alpha);
            Notify(msg_verbose, "Module Perceptron - alpha is '%.2f'\n", alpha);
            tmp_memory = create_array(input_size + 1);
            LearnUpdate = &Perceptron::LearnAlpha_LMS;
            break;
        case MU_LMS:
            Notify(msg_verbose, "Module Perceptron - learning_rule is 'mu_lms'\n");
            if (mu <= 0)
                Notify(msg_fatal_error, "Module Perceptron - mu has nonsens value '%.2f'\n", mu);
            Notify(msg_verbose, "Module Perceptron - mu is '%.2f'\n", mu);
            tmp_memory = create_array(input_size + 1);
            LearnUpdate = &Perceptron::LearnMu_LMS;
            break;
        case DELTA:
            Notify(msg_verbose, "Module Perceptron - learning_rule is 'delta'\n");
            if (activation_type != SIGMOID && activation_type != TANH)
                Notify(msg_fatal_error, "Module Perceptron - activation type must be 'sigmoid' or 'tanh' with learning rule 'delta'!\n");
            if (beta <= 0) // is this sound???
                Notify(msg_fatal_error, "Module Perceptron - beta has faulty value <= 0\n", beta);
            tmp_memory = create_array(input_size + 1);
            LearnUpdate = &Perceptron::LearnDelta;
            break;
        default:
            Notify(msg_fatal_error, "Module Perceptron - faulty learning_rule (%i)\n", learning_rule);
            break;
    }
    switch (activation_type){
        case DEFAULT:
            Notify(msg_verbose, "Module Perceptron - activation_type defaults to 'step'\n");
            activation_type = STEP;
            Activate = &Perceptron::ActivateStep;
            break;
        case STEP:
            Notify(msg_verbose, "Module Perceptron - activation_type is 'step'\n");
            Notify(msg_verbose, "Module Perceptron - step_threshold is %.2f\n", step_threshold);
            if (learning_rule == ROSENBLATT_MARGIN || learning_rule == MAY)
                Notify(msg_fatal_error, "Module Perceptron - Activation types 'step' and 'sigmoid' do not work with learning rules 'rosenblatt_margin' and 'may'!\n");
            Activate = &Perceptron::ActivateStep;
            break;
        case SIGN:
            Notify(msg_verbose, "Module Perceptron - activation_type is 'sign'\n");
            Activate = &Perceptron::ActivateSign;
            break;
        case SIGMOID:
            Notify(msg_verbose, "Module Perceptron - activation_type is 'sigmoid'\n");
            if (learning_rule == ROSENBLATT_MARGIN || learning_rule == MAY)
                Notify(msg_fatal_error, "Module Perceptron - Activation types 'step' and 'sigmoid' do not work with learning rules 'rosenblatt_margin' and 'may'!\n");
            Activate = &Perceptron::ActivateSigmoid;
            break;
        case TANH:
            Notify(msg_verbose, "Module Perceptron - activation_type is 'tanh'\n");
            Activate = &Perceptron::ActivateTanh;
            break;
        default:
            Notify(msg_fatal_error, "Module Perceptron - faulty activation_type (%i)\n", activation_type);
            break;
    }
    switch (learning_type){
        case DEFAULT:
            Notify(msg_verbose, "Module Perceptron - learning_type defaults to 'instant'\n");
            learning_type = INSTANT;
            break;
        case INSTANT:
            Notify(msg_verbose, "Module Perceptron - learning_type is 'instant'\n");
            break;
        case BATCH:
            Notify(msg_verbose, "Module Perceptron - learning_type is 'batch'\n");
            if (batch_size < 1)
                Notify(msg_fatal_error, "Module Perceptron - batch_size is <0. This just doesnt work.\n");
            if (batch_size == 1)
                Notify(msg_warning, "Module Perceptron - batch_size is 1. This is the same as learning_type == 'instant'. But ok, lets go.\n");
            else
                Notify(msg_verbose, "Module Perceptron - batch_size is %i. Lets go!\n", batch_size);
            batch_memory = create_matrix(input_size + 1, target_size);
            batch_errors_seen = 0;
            break;
        case MOMENTUM:
            Notify(msg_verbose, "Module Perceptron - learning_type is 'momentum'\n");
            if (momentum_ratio <= 0 || momentum_ratio >= 1)
                Notify(msg_fatal_error, "Module Perceptron - momentum_ratio is outside reasonable range. Please set it between 0 an 1 (excluding).\n");
            Notify(msg_verbose, "Module Perceptron - momentum_ratio is '%.2f'\n", momentum_ratio);
            m_ratio_1 = 1 - momentum_ratio;         // see DoMomentum() for explanation
            m_ratio_2 = momentum_ratio / m_ratio_1; // see DoMomentum() for explanation
            momentum_memory = create_matrix(input_size + 1, target_size);
            break;
        default:
            Notify(msg_fatal_error, "Module Perceptron - faulty 'learning_type' (%i)\n", learning_type);
            break;
    }


}
