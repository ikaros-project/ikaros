//    Arbiter.cc		This file is a part of the IKAROS project  Copyright (C) 2006-2025 Christian Balkenius

#include "ikaros.h"

using namespace ikaros;

class Arbiter: public Module
{
public:
    char **     input_name;
    char **     value_name;

    matrix          input; // float ***
    matrix          value_in;

    matrix          output;
    matrix          value_out;

    matrix          amplitudes;
    matrix          rbitration_state;
    matrix          smoothed;
    matrix          normalized;
    
    int         no_of_inputs;
    int         size_x;
    int         size_y;
    
    parameter       switch_time;
    parameter       alpha;

    parameter       metric;
    parameter       arbitration_method;
    parameter        by_row;
    parameter       softmax_exponent;
    parameter       hysteresis_threshold;
    
    int         winner;
    bool        debug;



    Arbiter(Parameter * p):
        Module(p)
    {
        Bind(by_row, "by_row");

        no_of_inputs = GetIntValue("no_of_inputs");

        input_name  = new char * [no_of_inputs];
        value_name  = new char * [no_of_inputs];

        input      = new float ** [no_of_inputs];
        value_in   = new float * [no_of_inputs];

        for (int i=0; i<no_of_inputs; i++)
        {
            AddInput(input_name[i] = create_formatted_string("INPUT_%d", i+1));
            AddInput(value_name[i] = create_formatted_string("VALUE_%d", i+1));
        }

        AddOutput("OUTPUT");
        AddOutput("VALUE");

        AddOutput("AMPLITUDES");
        AddOutput("ARBITRATION");
        AddOutput("SMOOTHED");
        AddOutput("NORMALIZED");
    }



    ~Arbiter()
    {
        for (int i=0; i<no_of_inputs; i++)
        {
            destroy_string(input_name[i]);
            destroy_string(value_name[i]);
        }
    }



    void
    SetSizes()
    {
        int sx = 0;
        int sy = 0;

        for(int i=0; i<no_of_inputs; i++)
        {
            int sxi = GetInputSizeX(input_name[i]);
            int syi = GetInputSizeY(input_name[i]);

            if(sxi == unknown_size)
                continue; // Not ready yet

            if(syi == unknown_size)
                continue; // Not ready yet

            if(sx != 0 && sxi != 0 && sx != sxi)
                Notify(msg_fatal_error, "Inputs have different sizes x: %s, %i vs %i, ", input_name[i], sxi, sx);

            if(sy != 0 && syi != 0 && sy != syi)
                Notify(msg_fatal_error, "Inputs have different sizes y: %s, %i vs %i", input_name[i], syi, sy);
            
            sx = sxi;
            sy = syi;
        }

        if(sx == unknown_size || sy == unknown_size)
            return;  // Not ready yet

        if(by_row)
        {
            SetOutputSize("OUTPUT", sx, sy);
            SetOutputSize("VALUE", sy);

            SetOutputSize("AMPLITUDES", no_of_inputs, sy);
            SetOutputSize("ARBITRATION", no_of_inputs, sy);
            SetOutputSize("SMOOTHED", no_of_inputs, sy);
            SetOutputSize("NORMALIZED", no_of_inputs, sy);
        }
        else
        {
            SetOutputSize("OUTPUT", sx, sy);
            SetOutputSize("VALUE", 1);

            SetOutputSize("AMPLITUDES", no_of_inputs, 1);
            SetOutputSize("ARBITRATION", no_of_inputs, 1);
            SetOutputSize("SMOOTHED", no_of_inputs, 1);
            SetOutputSize("NORMALIZED", no_of_inputs, 1);
        }
    }



    void
    Init()
    {
        Bind(metric, "metric");
        Bind(arbitration_method, "arbitration");
        Bind(softmax_exponent, "softmax_exponent");
        Bind(hysteresis_threshold, "hysteresis_threshold");
        Bind(switch_time, "switch_time");
        Bind(alpha, "alpha");
        Bind(debug, "debug");

        int vcnt = 0;
        for(int i=0; i<no_of_inputs; i++)
        {
            input[i] = GetInputMatrix(input_name[i]);
            value_in[i] = GetInputArray(value_name[i], false);
            if(value_in[i])
                vcnt++;
        }

        if(vcnt!=0 && vcnt != no_of_inputs)
            Notify(msg_fatal_error, "All VALUE inputs must have connections - or none");

        amplitudes = GetOutputMatrix("AMPLITUDES");
        arbitration_state = GetOutputMatrix("ARBITRATION");
        smoothed = GetOutputMatrix("SMOOTHED");
        normalized = GetOutputMatrix("NORMALIZED");

        output = GetOutputMatrix("OUTPUT");
        value_out = GetOutputMatrix("VALUE");

        if(by_row)
        {
            size_x = GetOutputSizeX("OUTPUT");
            size_y = GetOutputSizeY("OUTPUT");
        }
        else
        {
            size_x = GetOutputSizeX("OUTPUT") * GetOutputSizeY("OUTPUT"); // overflow to next line
            size_y = 1;

        }
    }



    void
    CalculateAmplitudes(int row)
    {
        if(value_in[0])
        {
            for(int i=0; i<no_of_inputs; i++)
                amplitudes[row][i] = value_in[i][row]; // FIXME: This needs to be checked
        }
        else if(metric == 0)
        {
            for(int i=0; i<no_of_inputs; i++)
                amplitudes[row][i] = norm1(input[i][row], size_x);
        }
        else if(metric == 1)
        {
            for(int i=0; i<no_of_inputs; i++)
                amplitudes[row][i] = norm(input[i][row], size_x);
        }
    }



    void
    Arbitrate(int row)
    {
        // Do the actual arbitration

        int a;
        switch(arbitration_method)
        {
            case 0: // WTA
                a = arg_max(amplitudes[row], no_of_inputs);
                reset_array(arbitration_state[row], no_of_inputs);
                arbitration_state[row][a] = amplitudes[row][a];
                break;

            case 1: // hysteresis
                a = arg_max(amplitudes[row], no_of_inputs);
                if(amplitudes[row][a] > amplitudes[row][winner] + hysteresis_threshold || amplitudes[row][winner] == 0)
                    winner = a;
                reset_array(arbitration_state[row], no_of_inputs);
                arbitration_state[row][winner] = amplitudes[row][winner];
                break;

            case 2: // softmax
                for(int i=0; i<no_of_inputs; i++)
                    arbitration_state[row][i] = pow(amplitudes[row][i], softmax_exponent);
                break;

            case 3: // hierarchy
                for(int i=no_of_inputs-1; i>=0; i--)
                    if(amplitudes[row][i] > 0 || i==0)
                    {
                        reset_array(arbitration_state[row], no_of_inputs);
                        arbitration_state[row][i] = amplitudes[row][i];
                        break;
                    }
                break;

            default: // no arbitration - should never happen
                copy_array(arbitration_state[row], amplitudes[row], no_of_inputs);
                break;
        }
        
        // Save last max for hysteresis or reset otherwise

        if(arbitration_method != 1)
            winner = 0;
    }



    void
    Smooth(int row)
    {
        float a = alpha;
        if(switch_time != 0)
            a = 1.0/switch_time;

        if(switch_time > 0 || alpha != 1)
            add(smoothed[row], 1-a, smoothed[row], a, arbitration_state[row], no_of_inputs);
        else
            copy_array(smoothed[row], arbitration_state[row], no_of_inputs);
    }



    void
    Tick()
    {
        for(int r=0; r < (by_row ? size_y : 1); r++)
        {
            CalculateAmplitudes(r);
            Arbitrate(r);
            Smooth(r);

            // Normalize

            copy_array(normalized[r], smoothed[r], no_of_inputs);
            normalize1(normalized[r], no_of_inputs);

            // Weigh inputs together

            reset_array(output[r], size_x);
            for(int i=0; i<no_of_inputs; i++)
                add(output[r], normalized[r][i], input[i][r], size_x);
        }
        if(debug)
        {
            printf("--Instance name: %s--\n", this->instance_name);
            printf("size_x= %i, size_y= %i\n", size_x, size_y);
            for (int i = 0; i < no_of_inputs; i++)
            {
                print_matrix(input_name[i], input[i], size_x, size_y);
                print_array(value_name[i], value_in[i], size_x);
            }
            print_matrix("output", output, size_x, size_y);
            print_matrix("value_out", value_out, size_x, size_y);
            print_matrix("amplitudes", amplitudes, size_x, size_y);
            print_matrix("arbitration_state", arbitration_state, size_x, size_y);
            print_matrix("smoothed", smoothed, size_x, size_y);
            print_matrix("normalized", normalized, size_x, size_y);
        }

    
};


INSTALL_CLASS(Arbiter)
