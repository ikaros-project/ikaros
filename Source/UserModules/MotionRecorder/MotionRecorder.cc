//
//	MotionRecorder.cc		This file is a part of the IKAROS project
//
//    Copyright (C) 2015 Christian Balkenius
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

#include "MotionRecorder.h"

using namespace ikaros;

enum { state_stop, state_off, state_record, state_play, state_train, state_save, state_load, state_sq_play };



void
MotionRecorder::Init()
{
    Bind(stop, "stop");
    Bind(off, "off");
    Bind(record, "record");
    Bind(play, "play");
    Bind(train, "train");
    Bind(save, "save");
    Bind(load, "load");
    Bind(sqplay, "sqplay");

    stop_debounce = false;
    off_debounce = false;
    record_debounce = false;
    play_debounce = false;
    train_debounce = false;
    save_debounce = false;

    output = GetOutputArray("OUTPUT");
    torque = GetOutputArray("TORQUE");
    enable = GetOutputArray("ENABLE");

    input = GetInputArray("INPUT");
    size = GetInputSize("INPUT");

    mode = GetInputArray("MODE");
    if(!mode)
    {
        mode = create_array(size);
        set_array(mode, 0, size);   // Set record mode for all channels as default
    }

    state = GetOutputArray("STATE");
    *state = state_stop;


    position_data_max = GetIntValue("position_data_max");
    position_data_count = 0;
    position_data  = create_matrix(size, position_data_max);

    torque_data  = set_matrix(create_matrix(size, position_data_max), 0.5, size, position_data_max);

    time = GetOutputArray("TIME");
    timebase = GetTickLength();
    if(timebase == 0)
        timebase = 1;

    play_torque = GetArray("torque", size);
    mask        = GetArray("mask", size);

    file_count = 0;
    file_name = GetValue("filename");
}



MotionRecorder::~MotionRecorder()
{
    destroy_matrix(position_data);
}



void
MotionRecorder::Save()
{
    char fname[1024];

    snprintf(fname, 1023, file_name, file_count);
    fname[1023] = 0;

    FILE * f = fopen(fname, "w");

    if(!f)
    {
        printf("ERROR could not open motion file \"%s\"\n", fname);
        return;
    }

    fprintf(f, "POSITION/%d TORQUE/%d MASK/%d\n", size, size, size);

    for(int j=0; j<position_data_count; j++)
    {
        for(int i=0; i<size; i++)
        {
            if(i!=0) fprintf(f, "\t");
            fprintf(f, "%.4f", position_data[j][i]);
        }

        for(int i=0; i<size; i++)
        {
            fprintf(f, "\t");
            fprintf(f, "%.4f", torque[i]);
        }
        for(int i=0; i<size; i++)
        {
            fprintf(f, "\t");
            fprintf(f, "%.4f", mask[i]);
        }

        fprintf(f, "\n");
    }

    fclose(f);
    file_count++;
}



void
MotionRecorder::Load() // SHOULD READ WIDTH FROM FILE AND CHECK THAT IT IS CORRECT
{
    char fname[1024];

    snprintf(fname, 1023, file_name, file_count);
    fname[1023] = 0;

    FILE * f = fopen(fname, "r");

    if(f == NULL)
    {
        file_count = 0;
        snprintf(fname, 1023, file_name, file_count);
        fname[1023] = 0;
        f = fopen(fname, "r");
    }

    if(!f)
    {
        printf("ERROR could not open motion file \"%s\"\n", fname);
        return;
    }

    printf("LOADING: %d\n", file_count);


//    fprintf(f, "POSITION/%d TORQUE/%d MASK/%d\n", size, size, size);

    position_data_count = 0;

    char buff [1024];
    fscanf(f, "%s", buff);
    fscanf(f, "%s", buff);
    fscanf(f, "%s", buff);

    while(!feof(f))
    {
        for(int i=0; i<size; i++)
        {
            if(i!=0) fprintf(f, "\t");
            fscanf(f, "%f", &position_data[position_data_count][i]);
        }

        for(int i=0; i<size; i++)
        {
            fscanf(f, "%f", &torque[i]);
        }
        for(int i=0; i<size; i++)
        {
            fscanf(f, "%f", &mask[i]);
        }

        position_data_count++;
    }

    fclose(f);
    file_count++;
}



void
MotionRecorder::Tick()
{
    // Change state

    if(!stop && stop_debounce)
    {
        stop_debounce = false;
        *state = state_stop;

    }
    else if(!off && off_debounce)
    {
        off_debounce = false;
        *state = state_off;
        printf("off\n");
    }
    else if(!record && record_debounce)
    {
        record_debounce = false;
        *state = state_record;

        set_array(torque, 0, size);
        set_array(enable, 0, size); // disable all
        if(mode)
        {
            for(int i=0; i<size; i++)
                if(mode && (mode[i] == 1 || mode[i] == 2)) // enabled channels with play
                {
                    enable[i] = 1;
                    torque[i] = play_torque[i];
                }
                else
                {
                    torque[i] = 0;
                    enable[i] = 0;
                }
        }

        position_data_count = 0;
        *time = 0;
        printf("record\n");
    }
    else if(!play && play_debounce)
    {
        play_debounce = false;
        *state = state_play;

        copy_array(torque, play_torque, size);
        set_array(enable, 1, size);
        *time = 0;
        printf("play\n");
    }
    else if(!sqplay && sqplay_debounce)
    {
        sqplay_debounce = false;
        *state = state_sq_play;

        copy_array(torque, play_torque, size);
        set_array(enable, 1, size);
        *time = 0;
        printf("sqplay\n");
    }
    else if(!train && train_debounce)
    {
        train_debounce = false;
        *state = state_train;

        copy_array(torque, play_torque, size);
        set_array(enable, 1, size);
        *time = 0;
        printf("train\n");
    }
    else if(!save && save_debounce)
    {
        save_debounce = false;
        *state = state_stop;

        Save();

        *time = 0;
        printf("save %d\n", file_count);
    }
    else if(!load && load_debounce)
    {
        load_debounce = false;
        *state = state_stop;

        printf("load %d\n", file_count);
        Load();

        *time = 0;
    }
    else if(stop)
    {
        stop_debounce = false;
        *state = state_stop;
        copy_array(output, input, size); // Immediate freeze response even before the button is released
        set_array(torque, 1, size);
        set_array(enable, 1, size);
        printf("stop\n");
    }
    else if(off)
    {
        off_debounce = false;
        *state = state_off;
        copy_array(output, input, size); // Immediate no torque response even before the button is released
        set_array(torque, 0, size);
        set_array(enable, 0, size);
        printf("off\n");
    }

    // Initial key press

    else if(record)
    {
        record_debounce = true;
    }
    else if(play)
    {
        play_debounce = true;
    }
    else if(train)
    {
        train_debounce = true;
    }
    else if(save)
    {
        save_debounce = true;
    }
    else if(load)
    {
        load_debounce = true;
    }
    else if(sqplay)
    {
        sqplay_debounce = true;
    }

    // Handle the different states

    int f = int(*time/float(timebase));

    if(*state == state_stop)
    {
        *time = 0;
        set_array(torque, 1, size);
        set_array(enable, 1, size);
        copy_array(output, input, size);
     }

    else if(*state == state_off)
    {
        *time = 0;
        set_array(torque, 0, size);
        set_array(enable, 0, size);
        copy_array(output, input, size);
    }

    else if(*state == state_record)
    {
        copy_array(output, input, size);
        if(position_data_count < position_data_max && norm1(input, size) > 0)
        {
            
            // copy_array(position_data[position_data_count], input, size);
            
            for(int i=0; i<size; i++)
                switch(int(mode[i]))
                {
                    case 0: // record
                        position_data[position_data_count][i] = input[i];
                        break;
                        
                    case 1: // play
                        output[i] = position_data[f][i];
                        
                    case 2: // hold
                        break;
                        
                    case 3: // free
                        break;
                        
                    default:
                        break;
                }
                
            copy_array(torque_data[position_data_count], play_torque, size);
            position_data_count++;
        }
        *time = float(position_data_count*timebase);
    }

    else if(*state == state_play)
    {
        copy_array(output, position_data[f], size);
         if(f < position_data_count-1)
            *time += timebase;
    }

    else if(*state == state_sq_play)
    {
        copy_array(output, position_data[f], size);
         if(f < position_data_count-1)
            *time += timebase;
        else
        {
            Load();
            *time = 0;
        }
    }

    else if(*state == state_train)
    {
    }
}



static InitClass init("MotionRecorder", &MotionRecorder::Create, "Source/UserModules/MotionRecorder/");

