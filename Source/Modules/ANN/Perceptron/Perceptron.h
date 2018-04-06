//
//	Perceptron.h		This file is a part of the IKAROS project
// 						Single layer of perceptrons
//
//    Copyright (C) 2002 Alexander Kolodziej
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

#ifndef PERCEPTRON_H
#define PERCEPTRON_H

#include "IKAROS.h"

#define DEFAULT 0

// define values for learning rules
#define ROSENBLATT 1
#define ROSENBLATT_MARGIN 2
#define MAY 3
#define ALPHA_LMS 4
#define MU_LMS 5
#define DELTA 6

// define values for activation types
#define STEP 1
#define SIGN 2
#define SIGMOID 3
#define TANH 4

// define values for learning types
#define INSTANT 1
#define BATCH 2
#define MOMENTUM 3

// define values for learning_rate modifications
#define SQRT 1
#define LOG 2

// define values for target normalization
#define YES 1
#define NO 2


class Perceptron: public Module
{
public:

    Perceptron(Parameter * p);
    virtual ~Perceptron();

    static Module * Create(Parameter * p);

    void		SetSizes();
    void 		Init();
    void 		Tick();

    int         input_size; // size of input array
    int         target_size; //determines how many perceptrons there will be instantiated in the layer/net

                // input and output arrays
    float       *input;
    float       *t_input;
    float       *t_target;
    float       *train;
    float       *output;
    float       *error;
    float       *correct;   // array with one item (float) that gives the number of correctly classified examples (not all examples.
                            // this is the average of the last correct_average_size examples) in percent during the modules lifetime.

    float       **perceptron_layer; // array of arrays, where each array is the weights for a perceptron
    float       *input_with_bias; // same as input, but with the bias appended
    float       dot_product; // calculated by Activate() and used by Learn()
    float       learning_rate; // defaults to 0.1
    int         learning_rate_mod; // how to modify learning_rate over time. sqrt/e_something
    float       bias; // defaults to 1. this is that extra input that each perceptron has.
    float       momentum_ratio; // percent (as a float) that decides how much of the learning will be based on the previous learning.
                                // used with learning_type momentum. default is 0.42
    float       step_threshold; // threshold for the activation_type step. default is 0.0
    float       margin; // for ROSENBLATT_MARGIN and MAY learning rules. default is 0.2
    float       alpha; // for ALPHA-LMS learning rule. defult is 0.1
    float       mu; // for MU-LMS learning rule. defult is 0.1
    float       beta; // for delta-rule
    float       **batch_memory; // this is where the last wannabe-corrections are summed up (how many? batch_size many...). used with learning_type batch
    float       **momentum_memory; // this is where the previous corrections are stored. used with learning_type momentum
    float       **current_memory; // this is where the current corrections are stored

    int         activation_type; // step, sign or sigmoid. default is sign.
    int         learning_rule; // what kind of updating of the weights. default is rosenblatt.
    int         learning_type; // instant, batch or momentum. default is instant.
    int         batch_size; // how many examples are seen before learning occurs. default is 42.
    int         correct_average_size; // on how many examples to base the 'correct' output on
    bool        normalize_target; // if we should manipulate the target data so that it fits the activation function

    void        (Perceptron::*Activate)(void); // pointer to the function that will be used to activate (perceptron) unit i
    void        ActivateStep(void); // 1 if dotproduct > 0, else 0
    void        ActivateSign(void); // 1 if dotproduct > 0, else -1
    void        ActivateSigmoid(void); // 1 / (1 - e^(-dot_product)
    void        ActivateTanh(void); //

    void        Learn(void); // activates the appropriate learning_type (instant/batch/momentum) and appropriate learning_rule
    void        (Perceptron::*LearnUpdate)(void); // function pointer
                // the references below denote formulas in the book "Funtamentals of ARTIFICIAL NEURAL NETWORKS
                // by Mohamad H. Hassoun
    void        LearnRosenblatt(void); // (3.1.2)
    void        LearnRosenblattMargin(void); // (3.1.16)
    void        LearnMay(void); // (3.1.29)
    void        LearnAlpha_LMS(void); // (3.1.30)
    void        LearnMu_LMS(void); // (3.1.35)
    void        LearnDelta(void); // (3.1.50)

                // this learning_type collects adjustments (only from misclassified examples) to a temp memory
                // and only adds them to the perceptrons weight when that memory is full
    void        DoBatch(void);
                // this learning_type adjusts to some part based on this ticks adjustments and to some part
                // (see momentum_ratio) on the previous adjustments
    void        DoMomentum(void);

                // modifies the learning rate. makes it smaller over time.
    void        ModifyLearningRate(void);

                // changes the target array to values that suit the activation functions.
                // e.g. if the targets are 0/1 and the chosen activation function counts
                // on them to be -1/1 it changes all 0's to -1
    void        NormalizeTarget(void);

                // calculates error and correct outputs
    void        CalculateErrorAndCorrect(void);

                // checks (all) the parameters, bias, learning_type etc...
    void        CheckParameters(float rw_min, float rw_max);

private:
                // here we have some variables that could be considered more "internal" to the module.
    float       *perceptron_errors; // array of squared errors (sqr(target-output)) of each perceptron. the error is the sqrt of the sum of these
    float       *tmp_memory; // used for steps in calculations...
    float       m_ratio_1; // see DoMomentum() in .cc for explanation
    float       m_ratio_2; // see DoMomentum() in .cc for explanation
    int         batch_errors_seen; // how many misclassified examples DoBatch() has collected so far (in this batch)
    int         train_tick; // which train tick we are in (not incremented when not learning)
    float       correct_now; // used to see if this ticks example is correctly classified
    float       *correct_all; // the correct_now values of the correct_average_size last examples
    float       learning_rate_now; // the modified learning_rate for this tick
    //float       tanh(float); //
};

#endif
