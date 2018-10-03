//
//	SequenceRecorder.cc		This file is a part of the IKAROS project
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

#include "SequenceRecorder.h"

#include <sys/types.h>
#include <dirent.h>

#include "json.hpp"

using namespace ikaros;

enum { state_stop, state_off, state_record, state_play };
enum { mode_off=0, mode_stop, mode_play, mode_record };

void
SequenceRecorder::Init()
{
    Bind(smoothing_time, "smoothing_time");
    Bind(nominal_torque, "torque");
    Bind(current_motion, "current_motion");
    Bind(mode_string, "mode_string");
    
    current_motion = 0;

    // Mode
    
    mode = GetOutputMatrix("MODE");
    for(int i=0; i<size; i++)
        mode[mode_off][i] = 1;

    // Trig and completion
    
    completed = GetOutputArray("COMPLETED");
    trig = GetInputArray("TRIG");
    trig_size = GetInputSize("TRIG");
    if(trig_size > max_motions)
    {
        Notify(msg_warning, "TRIG input larger than max behaviors");
        trig_size = max_motions;
    }

    // Input and outputs

    input = GetInputArray("INPUT");
    output = GetOutputArray("OUTPUT");
    torque = GetOutputArray("TORQUE");
    enable = GetOutputArray("ENABLE");
    size = GetInputSize("INPUT");
    
    // Load/Save

    file_name = GetValue("filename");

    // UI
    
    time = GetOutputArray("TIME");
    keypoints = GetOutputMatrix("KEYPOINTS");
    
    // Misc
    
    stop_position = create_array(size); // How are these used
    start_position = create_array(size);
    start_torque = create_array(size);
}



SequenceRecorder::~SequenceRecorder()
{
    if(auto_save)
    {
    }

}



void
SequenceRecorder::Command(std::string s, int x, int y, std::string value)
{
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
    else if (s == "toggle")
        ToggleMode(x, y);
}



void
SequenceRecorder::ToggleMode(int x, int y)
{
    for(int i=0; i<4; i++) // reset row
        mode[i][x] = 0;
    mode[y][x] = 1;
}



void
SequenceRecorder::Off()
{
    mode_string = "Off";
    copy_array(output, input, size);
    reset_array(torque, size);
    reset_array(enable, size);
}


void
SequenceRecorder::Stop()
{
    mode_string = "Stop";

    copy_array(stop_position, input, size); // Save stop position - why?
    copy_array(output, input, size); // Immediately freeze response
    set_array(enable, 1, size);

    for(int i=0; i<size; i++)
    {
        enable[i] = mode[mode_off][i] ? 0 : 1;
        torque[i] = mode[mode_off][i] ? 0 : nominal_torque;
    }
}


void
SequenceRecorder::Record()
{
    mode_string = "Rec";
    set_array(enable, 0, size); // disable all

    print_matrix("mode", mode, 2, 4);

    for(int i=0; i<size; i++)
        if(mode[mode_stop][i] || mode[mode_play][i]) // enabled channels with stop or play
        {
            enable[i] = 1;
        }
        else
        {
            enable[i] = 0;
        }

    position_data_count[current_motion] = 0;
    *time = 0;
}


void
SequenceRecorder::Play()
{
    mode_string = "Play";

    copy_array(start_position, input, size);
    set_array(enable, 1, size);
    if(mode)
    {
        for(int i=0; i<size; i++)
            if(mode[mode_off][i]) // disable some channels
                enable[i] = 0;
     }
    *time = 0;
}


void
SequenceRecorder::Save()
{
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
SequenceRecorder::Load() // SHOULD READ WIDTH FROM FILE AND CHECK THAT IT IS CORRECT
{
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
SequenceRecorder::Tick()
{
 /*
    // TEST: Copy current data to webui output
    
    for(int c=0; c<2; c++)
        for(int i=0; i<1000; i++)
        {
            keypoints[i][c] = position_data[current_motion][i][c]; // position_data_count[current_motion]
//            timestamps[i] = float(i)*20;    // ms
        }
    
    if(GetTick() < 20) // wait for valid data
    {
        copy_array(stop_position, input, size);
        copy_array(output, stop_position, size);
        reset_array(enable, size);
        return;
    }

    reset_array(completed, max_motions);

    // Handle the different states

    int f = int(*time/float(timebase));

    if(*state == state_stop)
    {
        *time = 0;

        set_array(enable, 1, size);
        if(mode)
        {
            for(int i=0; i<size; i++)
                if(mode[mode_off][i]) // disable some channels
                    enable[i] = 0;
         }

        copy_array(output, stop_position, size);
     }

    else if(*state == state_off)
    {
        *time = 0;
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
                    if(mode[mode_record][i])
                        position_data[current_motion][position_data_count[current_motion]][i] = input[i];
                        
                    else if(mode[mode_play][i])
                        if(position_data[current_motion][f][i] != 0)
                            output[i] = position_data[current_motion][f][i];

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
            set_array(enable, 1, size);
            if(mode)
            {
                for(int i=0; i<size; i++)
                    if(mode[mode_off][i]) // disable some channels
                        enable[i] = 0;
             }
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
    */
}



static InitClass init("SequenceRecorder", &SequenceRecorder::Create, "Source/Modules/RobotModules/SequenceRecorder/");

