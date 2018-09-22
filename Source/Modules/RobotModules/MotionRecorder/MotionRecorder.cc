//
//	MotionRecorder.cc		This file is a part of the IKAROS project
//
//    Copyright (C) 2015-2018 Christian Balkenius
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
    Bind(record_on_trig, "record_on_trig");
    Bind(smoothing_time, "smoothing_time");

    Bind(current_motion, "current_motion");
    Bind(mode_string, "mode");
    
    max_motions = GetIntValue("max_motions");
    current_motion = 0;

    input = GetInputArray("INPUT");
    size = GetInputSize("INPUT");
    
    stop_position = create_array(size);
    start_position = create_array(size);
    start_torque = create_array(size);

    position_data_max = GetIntValue("position_data_max");
    position_data_count = new int[max_motions];
    position_data  = create_matrix(size, position_data_max, max_motions);
    time_data = create_matrix(size, position_data_max);

    trig = GetInputArray("TRIG");
    trig_size = GetInputSize("TRIG");
    
    if(trig_size > max_motions)
    {
        Notify(msg_warning, "TRIG input larger than max behaviors");
        trig_size = max_motions;
    }
    
    trig_last = create_array(max_motions);

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


    torque_data  = set_matrix(create_matrix(size, position_data_max, max_motions), 0.5, size, position_data_max, max_motions);

    time = GetOutputArray("TIME");
    timebase = GetTickLength();
    if(timebase == 0)
        timebase = 1;

    play_torque = GetArray("torque", size, true);

    file_name = GetValue("filename");

    auto_save = GetBoolValue("auto_save"); // Cannot be called in destructor
    if(GetBoolValue("auto_load"))
    {
        for(current_motion=0; current_motion<max_motions; current_motion++)
            Load();
        current_motion=0;
    }
}



MotionRecorder::~MotionRecorder()
{
    if(auto_save)
    {
        for(current_motion=0; current_motion<max_motions; current_motion++)
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
MotionRecorder::Command(std::string s)
{
    printf("Received command: %s\n", s.c_str());

    if(s == "off")
        Off();
    else if (s == "stop")
        Stop();
    else if (s == "record")
        Record();
    else if (s == "play")
        Play();
    else if (s == "load")
        Load();
    else if (s == "save")
        Save();
}


void
MotionRecorder::Off()
{
    printf("Off");
    mode_string = "Off";
    *state = state_off;
    *state = state_off;
    copy_array(output, input, size); // Immediate no torque response even before the button is released
    set_array(torque, 0, size);
    set_array(enable, 0, size);
}


void
MotionRecorder::Stop()
{
    printf("Stop");
    mode_string = "Stop";
    *state = state_stop;
    copy_array(stop_position, input, size);
    print_array("stop_position", stop_position, size);

    // If we record - copy position for all recorded channels to the rest of the track

    if(*state == state_record)
    {
        for(int i=0; i<size; i++)
            if(!mode || mode[i] == 0)
                for(int p=int(*time/float(timebase)); p<position_data_max; p++)
                    position_data[current_motion][p][i] = input[i];
    }

    // Do the rest

    *state = state_stop;
    copy_array(output, stop_position, size); // Immediate freeze response even before the button is released

    set_array(torque, 1, size); // Ok, but will be overridden if we just started
    set_array(enable, 1, size);
    if(mode)
    {
        for(int i=0; i<size; i++)
            if(mode && mode[i] == 3) // disable some channels
                enable[i] = 0;
     }

    printf("stop\n");
}


void
MotionRecorder::Record()
{
    printf("Record");
    mode_string = "Rec";
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

    position_data_count[current_motion] = 0;
    *time = 0;
}


void
MotionRecorder::Play()
{
    printf("Play");
    mode_string = "Play";
    *state = state_play;

    copy_array(start_position, input, size);
    copy_array(start_torque, torque, size);
//        copy_array(torque, play_torque, size);
    set_array(enable, 1, size);
    if(mode)
    {
        for(int i=0; i<size; i++)
            if(mode && mode[i] == 3) // disable some channels
                enable[i] = 0;
     }
    *time = 0;
}


void
MotionRecorder::Save()
{
    *state = state_stop;
    mode_string = "Stop";
    
   char fname[1024];

    snprintf(fname, 1023, file_name, current_motion);
    fname[1023] = 0;

    FILE * f = fopen(fname, "w");

    if(!f)
    {
        printf("ERROR could not open motion file \"%s\"\n", fname);
        return;
    }

    fprintf(f, "TIME/1  POSITION/%d\n", size);

    for(int j=0; j<position_data_count[current_motion]; j++)
    {
        long t = j*GetTickLength();
        fprintf(f, "%ld\n", t);
        
        for(int i=0; i<size; i++)
        {
            if(i!=0) fprintf(f, "\t");
            fprintf(f, "%.4f", position_data[current_motion][j][i]);
        }

        fprintf(f, "\n");
    }

    fclose(f);

    printf("Saved: %d\n", current_motion);
    *time = 0;
}



void
MotionRecorder::Load() // SHOULD READ WIDTH FROM FILE AND CHECK THAT IT IS CORRECT
{
    *state = state_stop;
    mode_string = "Stop";
    char fname[1024];

    snprintf(fname, 1023, file_name, current_motion);
    fname[1023] = 0;

    FILE * f = fopen(fname, "r");

    if(f == NULL)
    {
        snprintf(fname, 1023, file_name, current_motion);
        fname[1023] = 0;
        f = fopen(fname, "r");
    }

    if(!f)
    {
        printf("WARNING: could not open motion file \"%s\" (ignored)\n", fname);
        return;
    }


//    fprintf(f, "POSITION/%d TORQUE/%d MASK/%d\n", size, size, size);

    position_data_count[current_motion] = 0;

    char buff [1024];
    fscanf(f, "%s", buff);
    fscanf(f, "%s", buff);
    fscanf(f, "%s", buff);

    while(!feof(f))
    {
        long t;
        fscanf(f, "%ld", &t); // ignore for now

        for(int i=0; i<size; i++)
        {
            if(i!=0) fprintf(f, "\t");
            fscanf(f, "%f", &position_data[current_motion][position_data_count[current_motion]][i]);
        }

        position_data_count[current_motion]++;
    }

    // Fill the rest of the buffer with the last read positions - necessary with multirecording with mixed play/record
    
    for(int p=position_data_count[current_motion]; p<position_data_max; p++)
        for(int i=0; i<size; i++)
            position_data[current_motion][p][i] = position_data[current_motion][position_data_count[current_motion]][i];
    
    fclose(f);

    printf("Loaded: %d\n", current_motion);
    
    *time = 0;
}


void
MotionRecorder::Tick()
{
    if(GetTick() < 20) // wait for valid data
    {
        copy_array(stop_position, input, size);
        copy_array(output, stop_position, size);
        reset_array(enable, size);
        return;
    }

    reset_array(completed, max_motions);

    if(trig)
    {
        if(record_on_trig)
        {
            for(int i=0; i<trig_size; i++)
            {
                if(trig[i] == 1 && trig_last[i] == 0)
                {
                    current_motion = i;

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

                    position_data_count[current_motion] = 0;
                    *time = 0;
                    printf("record (trig)\n");
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
                    current_motion = i;
                    *state = state_play;

                    copy_array(start_position, input, size);
                    copy_array(start_torque, torque, size);

                    set_array(enable, 1, size);
                    if(mode)
                    {
                        for(int i=0; i<size; i++)
                            if(mode && mode[i] == 3) // disable some channels
                                enable[i] = 0;
                     }

                    *time = 0;

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
        if(mode)
        {
            for(int i=0; i<size; i++)
                if(mode && mode[i] == 3) // disable some channels
                    enable[i] = 0;
         }

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
        if(position_data_count[current_motion] < position_data_max && norm1(input, size) > 0)
        {
            // copy_array(position_data[position_data_count], input, size);
            
            for(int i=0; i<size; i++)
                switch(int(mode[i]))
                {
                    case 0: // record
                        position_data[current_motion][position_data_count[current_motion]][i] = input[i];
                        break;
                        
                    case 1: // play
                        if(position_data[current_motion][f][i] != 0)
                            output[i] = position_data[current_motion][f][i];
                        
                    case 2: // hold
                        break;
                        
                    case 3: // free
                        break;
                        
                    default:
                        break;
                }
                
            copy_array(torque_data[current_motion][position_data_count[current_motion]], play_torque, size);
            position_data_count[current_motion]++;
        }
        else
            Notify(msg_warning, "Recording buffer full.");
        
        *time = float(position_data_count[current_motion]*timebase);
    }

    else if(*state == state_play)
    {
        if(f < smoothing_time)
        {
            float a = float(f)/float(smoothing_time);
            for(int i=0; i< size; i++)
            {
                if(position_data[current_motion][f] != 0)
                    output[i] = (1-a) * start_position[i] + a * position_data[current_motion][f][i];
                torque[i] = (1-a) * start_torque[i] + a * play_torque[i];
            }
        }
        else
            copy_array(output, position_data[current_motion][f], size);

        if(f < position_data_count[current_motion]-1)
            *time += timebase;
        else
        {
            completed[current_motion] = 1;
            *state = state_stop;
            copy_array(stop_position, input, size);
            copy_array(output, stop_position, size); // Just in case this is not run later
            set_array(torque, 1, size);
            set_array(enable, 1, size);
            if(mode)
            {
                for(int i=0; i<size; i++)
                    if(mode && mode[i] == 3) // disable some channels
                        enable[i] = 0;
             }
//            *playing = 0;
        }
    }

    else if(*state == state_sq_play)
    {
        copy_array(output, position_data[current_motion][f], size);
        if(f < position_data_count[current_motion]-1)
            *time += timebase;
        else
        {
            // Load();
            current_motion ++;
            if(current_motion >= max_motions)
                current_motion = 0;
            *time = 0;
        }
    }


    if(trig)
        copy_array(trig_last, trig, trig_size);
}



static InitClass init("MotionRecorder", &MotionRecorder::Create, "Source/Modules/RobotModules/MotionRecorder/");

