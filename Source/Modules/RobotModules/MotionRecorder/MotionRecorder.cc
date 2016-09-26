//
//	MotionRecorder.cc		This file is a part of the IKAROS project
//
//    Copyright (C) 2015-2016 Christian Balkenius
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


void copy_no_zeros(float * d, float * s1, float * s2, int size)
{
    for(int i=0; i<size; i++)
        if(s1[i] != 0)
            d[i] = s1[i];
        else
            d[i] = s2[i];
}



void
MotionRecorder::Init()
{
    Bind(stop, "stop");
    Bind(off, "off");
    Bind(record, "record");
    Bind(play, "play");
    Bind(clear, "clear");
    Bind(save, "save");
    Bind(load, "load");
    Bind(sqplay, "sqplay");
    Bind(record_on_trig, "record_on_trig");
    Bind(smoothing_time, "smoothing_time");

    Bind(current_behavior, "current_behavior");

    stop_debounce = false;
    off_debounce = false;
    record_debounce = false;
    play_debounce = false;
    clear_debounce = false;
    save_debounce = false;

    max_behaviors = GetIntValue("max_behaviors");
    current_behavior = 0;

    playing = GetOutputArray("PLAYING");
    
    input = GetInputArray("INPUT");
    size = GetInputSize("INPUT");
    
    stop_position = create_array(size);
    start_position = create_array(size);
    start_torque = create_array(size);

    position_data_max = GetIntValue("position_data_max");
    position_data_count = new int[max_behaviors];
    position_data  = create_matrix(size, position_data_max, max_behaviors);

    trig = GetInputArray("TRIG");
    trig_size = GetInputSize("TRIG");
    
    if(trig_size > max_behaviors)
    {
        Notify(msg_warning, "TRIG input larger than max behaviors");
        trig_size = max_behaviors;
    }
    
    trig_last = create_array(max_behaviors);

    completed = GetOutputArray("COMPLETED");

    output = GetOutputArray("OUTPUT");
    torque = GetOutputArray("TORQUE");
    enable = GetOutputArray("ENABLE");

    mode = GetInputArray("MODE");
    if(!mode)
    {
        mode = create_array(size);
        set_array(mode, 0, size);   // Set record mode for all channels as default
    }

    state = GetOutputArray("STATE");
    *state = state_stop;


    torque_data  = set_matrix(create_matrix(size, position_data_max, max_behaviors), 0.5, size, position_data_max, max_behaviors);

    time = GetOutputArray("TIME");
    timebase = GetTickLength();
    if(timebase == 0)
        timebase = 1;

    play_torque = GetArray("torque", size);
    mask        = GetArray("mask", size);

    file_name = GetValue("filename");

    auto_save = GetBoolValue("auto_save"); // Cannot be called in destructor
    if(GetBoolValue("auto_load"))
    {
        for(current_behavior=0; current_behavior<max_behaviors; current_behavior++)
            Load();
        current_behavior=0;
    }
}



MotionRecorder::~MotionRecorder()
{
    if(auto_save)
    {
        for(current_behavior=0; current_behavior<max_behaviors; current_behavior++)
            Save();
    }

    destroy_array(start_torque);
    destroy_array(start_position);
    destroy_array(stop_position);
    destroy_matrix(position_data);
    destroy_array(trig_last);
    delete [] position_data_count;
}



void
MotionRecorder::Save()
{
    char fname[1024];

    snprintf(fname, 1023, file_name, current_behavior);
    fname[1023] = 0;

    FILE * f = fopen(fname, "w");

    if(!f)
    {
        printf("ERROR could not open motion file \"%s\"\n", fname);
        return;
    }

    fprintf(f, "POSITION/%d TORQUE/%d MASK/%d\n", size, size, size);

    for(int j=0; j<position_data_count[current_behavior]; j++)
    {
        for(int i=0; i<size; i++)
        {
            if(i!=0) fprintf(f, "\t");
            fprintf(f, "%.4f", position_data[current_behavior][j][i]);
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

    printf("Saved: %d\n", current_behavior);
}



void
MotionRecorder::Load() // SHOULD READ WIDTH FROM FILE AND CHECK THAT IT IS CORRECT
{
    char fname[1024];

    snprintf(fname, 1023, file_name, current_behavior);
    fname[1023] = 0;

    FILE * f = fopen(fname, "r");

    if(f == NULL)
    {
        snprintf(fname, 1023, file_name, current_behavior);
        fname[1023] = 0;
        f = fopen(fname, "r");
    }

    if(!f)
    {
        printf("WARNING: could not open motion file \"%s\" (ignored)\n", fname);
        return;
    }


//    fprintf(f, "POSITION/%d TORQUE/%d MASK/%d\n", size, size, size);

    position_data_count[current_behavior] = 0;

    char buff [1024];
    fscanf(f, "%s", buff);
    fscanf(f, "%s", buff);
    fscanf(f, "%s", buff);

    while(!feof(f))
    {
        for(int i=0; i<size; i++)
        {
            if(i!=0) fprintf(f, "\t");
            fscanf(f, "%f", &position_data[current_behavior][position_data_count[current_behavior]][i]);
//            printf("%f\n", position_data[current_behavior][position_data_count[current_behavior]][i]);
        }

        for(int i=0; i<size; i++)
        {
            fscanf(f, "%f", &torque[i]);
        }
        for(int i=0; i<size; i++)
        {
            fscanf(f, "%f", &mask[i]);
        }

        position_data_count[current_behavior]++;
    }

    position_data_count[current_behavior]--;
    
    // Fill the rest of the buffer with the last read positions - necessary with multirecording with mixed play/record
    
    for(int p=position_data_count[current_behavior]; p<position_data_max; p++)
        for(int i=0; i<size; i++)
            position_data[current_behavior][p][i] = position_data[current_behavior][position_data_count[current_behavior]][i];
    
    fclose(f);

    printf("Loaded: %d\n", current_behavior);
}



void
MotionRecorder::Tick()
{
    if(GetTick() == 1)
    {
        copy_array(stop_position, input, size);
    }
    reset_array(completed, max_behaviors);

    // Change state

    if(!stop && stop_debounce)
    {
        stop_debounce = false;
        *state = state_stop;
        copy_array(stop_position, input, size);
        print_array("stop_position", stop_position, size);
        *playing = 0;
    }
    else if(!off && off_debounce)
    {
        off_debounce = false;
        *state = state_off;
        printf("off\n");
        *playing = 0;
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

        position_data_count[current_behavior] = 0;
        *time = 0;
        printf("record\n");
    }
    else if(!clear && clear_debounce)
    {
        clear_debounce = false;
        if(*state == state_record)
        {
            for(int i=0; i<size; i++)
//              if(!mode || mode[i] == 0) // Ignore mode for now
                for(int p=0; p<position_data_max; p++)
                    position_data[current_behavior][p][i] = input[i];
        }
    }
    else if(!play && play_debounce)
    {
        *playing = 1;
        play_debounce = false;
        *state = state_play;

        copy_array(start_position, input, size);
        copy_array(start_torque, torque, size);
//        copy_array(torque, play_torque, size);
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
    else if(!save && save_debounce)
    {
        save_debounce = false;
        *state = state_stop;

        Save();

        *time = 0;
        printf("save %d\n", current_behavior);
    }
    else if(!load && load_debounce)
    {
        load_debounce = false;
        *state = state_stop;

        printf("load %d\n", current_behavior);
        Load();

        *time = 0;
    }
    else if(stop)
    {
        *playing = 0;
        copy_array(stop_position, input, size);
        print_array("stop_position", stop_position, size);
        
        // If we record - copy position for all recorded channels to the rest of the track
        
        if(*state == state_record)
        {
            for(int i=0; i<size; i++)
                if(!mode || mode[i] == 0)
                    for(int p=int(*time/float(timebase)); p<position_data_max; p++)
                        position_data[current_behavior][p][i] = input[i];
        }
        
        // Do the rest
        
        stop_debounce = false;
        *state = state_stop;
        copy_array(output, stop_position, size); // Immediate freeze response even before the button is released
        
        set_array(torque, 1, size); // Ok, but will be overridden if we just started
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
    else if(clear)
    {
        clear_debounce = true;
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

    // Handle trig input

    if(trig)
    {
        if(record_on_trig)
        {
            for(int i=0; i<trig_size; i++)
            {
                if(trig[i] == 1 && trig_last[i] == 0) // trig[i] > trig_last[i] DOES NOT WORK - WHY???
                {
                    current_behavior = i;

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

                    position_data_count[current_behavior] = 0;
                    *time = 0;
                    printf("record (trig)\n");
					*playing = 1;
                    break;
                }
            }
        }
        else // play on trig
        {
            for(int i=0; i<trig_size; i++)
            {
                if(trig[i] == 1 && trig_last[i] == 0)
                {
                    current_behavior = i;
                    play_debounce = false;
                    *state = state_play;

                    copy_array(start_position, input, size);
                    copy_array(start_torque, torque, size);

                    set_array(enable, 1, size);
                    *time = 0;
                    *playing = 1;

                    printf("play (trig)\n");
                    break;
                }

                if(trig[i] > 0)
                    break;
            }
        }
    }

    // Handle the different states

    int f = int(*time/float(timebase));

    if(*state == state_stop)
    {
        *time = 0;
        
        if(GetTick() < smoothing_time)
        {
            float a = float(GetTick())/float(smoothing_time);
            for(int i=0; i< size; i++)
            {
                torque[i] = (1-a) * start_torque[i] + a * play_torque[i];
            }
        }

        
        set_array(enable, 1, size);
        copy_array(output, stop_position, size);
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
        if(position_data_count[current_behavior] < position_data_max && norm1(input, size) > 0)
        {
            // copy_array(position_data[position_data_count], input, size);
            
            for(int i=0; i<size; i++)
                switch(int(mode[i]))
                {
                    case 0: // record
                        position_data[current_behavior][position_data_count[current_behavior]][i] = input[i];
                        break;
                        
                    case 1: // play
                        if(position_data[current_behavior][f][i] != 0)
                            output[i] = position_data[current_behavior][f][i];
                        
                    case 2: // hold
                        break;
                        
                    case 3: // free
                        break;
                        
                    default:
                        break;
                }
                
            copy_array(torque_data[current_behavior][position_data_count[current_behavior]], play_torque, size);
//            printf("Recordning (ch: %d: pos: %d, v: %f)\n", current_behavior, position_data_count[current_behavior], position_data[current_behavior][position_data_count[current_behavior]][0]);

            position_data_count[current_behavior]++;
        }
        else
            Notify(msg_warning, "Recording buffer full.");
        
        *time = float(position_data_count[current_behavior]*timebase);
    }

    else if(*state == state_play)
    {
        if(f < smoothing_time)
        {
            float a = float(f)/float(smoothing_time);
            for(int i=0; i< size; i++)
            {
                if(position_data[current_behavior][f] != 0)
                    output[i] = (1-a) * start_position[i] + a * position_data[current_behavior][f][i];
                torque[i] = (1-a) * start_torque[i] + a * play_torque[i];
                
            }
        }
        else
            copy_no_zeros(output, position_data[current_behavior][f], input, size);

        if(f < position_data_count[current_behavior]-1)
            *time += timebase;
        else
        {
            completed[current_behavior] = 1;
            *state = state_stop;
            copy_array(stop_position, input, size);
            copy_array(output, stop_position, size); // Just in case this is not run later
            set_array(torque, 1, size);
            set_array(enable, 1, size);
            *playing = 0;
        }
    }

    else if(*state == state_sq_play)
    {
        copy_no_zeros(output, position_data[current_behavior][f], input, size);
         if(f < position_data_count[current_behavior]-1)
            *time += timebase;
        else
        {
            Load();
            *time = 0;
        }
    }


    if(trig)
        copy_array(trig_last, trig, trig_size);
}



static InitClass init("MotionRecorder", &MotionRecorder::Create, "Source/Modules/RobotModules/MotionRecorder/");

