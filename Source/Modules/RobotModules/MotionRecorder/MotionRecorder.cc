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

#include <sys/types.h>
#include <dirent.h>

using namespace ikaros;

enum { state_stop, state_off, state_record, state_play, state_train, state_save, state_load, state_sq_play };
enum { mode_off=0, mode_stop, mode_play, mode_record };

void
MotionRecorder::Init()
{
    Bind(record_on_trig, "record_on_trig");
    Bind(smoothing_time, "smoothing_time");

    Bind(current_motion, "current_motion");
    Bind(mode_string, "mode_string");
    
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
    timestamp_data = create_matrix(size, position_data_max);

    file_name = GetValue("filename");
    json_file_name = GetValue("json_filename");
    directory = GetValue("directory");

    motion_name = new std::string [max_motions];

    trig = GetInputArray("TRIG");
    trig_size = GetInputSize("TRIG");
    
    trig_out = GetOutputArray("TRIG_OUT");

    if(trig_size > max_motions)
    {
        Notify(msg_warning, "TRIG input larger than max behaviors");
        trig_size = max_motions;
    }
    
    trig_last = create_array(max_motions);

    completed = GetOutputArray("COMPLETED");

    output = GetOutputArray("OUTPUT");
    enable = GetOutputArray("ENABLE");
    mode = GetOutputMatrix("MODE");
    for(int i=0; i<size; i++)
        mode[mode_off][i] = 1;

    state = GetOutputArray("STATE");
    *state = state_stop;

    time = GetOutputArray("TIME");
    timebase = GetTickLength();
    if(timebase == 0)
        timebase = 1;

    auto_save = GetBoolValue("auto_save"); // Cannot be called in destructor
    if(GetBoolValue("auto_load"))
    {
        for(current_motion=0; current_motion<max_motions; current_motion++)
            Load();
        current_motion=0;
    }
    
    keypoints = GetOutputMatrix("KEYPOINTS");
    timestamps = GetOutputArray("TIMESTAMPS");
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
MotionRecorder::Command(std::string s, float x, float y, std::string value)
{
//    printf("##%s\n", s.c_str());
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
MotionRecorder::ToggleMode(int x, int y)
{
    for(int i=0; i<4; i++) // reset row
        mode[i][x] = 0;
    mode[y][x] = 1;
    
    // Check if we should store servo position if stop is selected
    
    if(y==1) // STOP
    {
        start_position[x] = input[x];
        stop_position[x] = input[x];
        enable[x] = 1;
    }
}



void
MotionRecorder::Off()
{
    mode_string = "Off";
    *state = state_off;
    copy_array(output, input, size); // Immediate no torque response even before the button is released
    for(int i=0; i<size; i++)
        if(mode[mode_stop][i] == 0)
            enable[i] = 0;
}



void
MotionRecorder::Stop()
{
    mode_string = "Stop";
    *state = state_stop;
    
    for(int i=0; i<size; i++)
        if(mode[mode_stop][i] == 1)
            stop_position[i] = start_position[i]; // Never set new position for stoppped channel
        else
            stop_position[i] = input[i];
        
//    copy_array(stop_position, input, size);

    print_array("stop_position", stop_position, size);

    // If we record - copy position for all recorded channels to the rest of the track

    if(*state == state_record)
    {
        for(int i=0; i<size; i++)
            if(mode[mode_record][i])
                for(int p=int(*time/float(timebase)); p<position_data_max; p++)
                    position_data[current_motion][p][i] = input[i];
    }

    *state = state_stop;
    copy_array(output, stop_position, size); // Immediate freeze response even before the button is released

    set_array(enable, 1, size);
    if(mode)
    {
        for(int i=0; i<size; i++)
            if(mode[mode_off][i])
                enable[i] = 0;
     }

    printf("stop\n");
}


void
MotionRecorder::Record()
{
    mode_string = "Rec";
    *state = state_record;
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
MotionRecorder::Play()
{
    mode_string = "Play";
    *state = state_play;
    *trig_out = 1;

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
MotionRecorder::SaveAsJSON()
{
    char fname[1024];
    snprintf(fname, 1023, json_file_name, current_motion);
    fname[1023] = 0;

    FILE * f = fopen(fname, "w");
    if(!f)
    {
        printf("ERROR could save to motion JSON file \"%s\"\n", fname);
        return;
    }

    fprintf(f, "[\n\t{\n");

    fprintf(f, "\t\t\"channels\": %d\n", size);
    fprintf(f, "\t\t\"timebase\": %ld\n", GetTickLength());
    fprintf(f, "\t\t\"interpolation\": \"linear\"\n");
    fprintf(f, "\t\t\"units\": \"ms\"\n");
    fprintf(f, "\t\t\"loop\": \"no\"\n");
    fprintf(f, "\t\t\"start\": 0\n");
    fprintf(f, "\t\t\"stop\": %ld\n", position_data_count[current_motion]*GetTickLength());
    fprintf(f, "\t\t\"motion\":\n\t\t[\n");

    char c0[] = "\t\t\t";
    char c1[] = ",\n\t\t\t";
    char * start_chars = c0;

    for(int j=0; j<position_data_count[current_motion]; j++)
    {
        fprintf(f, "%s", start_chars);
        long t = j*GetTickLength();
        fprintf(f, "{\"t\" : %ld\t\"p\" : [", t);

        for(int i=0; i<size; i++)
        {
            if(i!=0) fprintf(f, ", ");
            fprintf(f, "%.4f", position_data[current_motion][j][i]);
        }

        fprintf(f, "]}");
        start_chars = c1;
    }

    fprintf(f, "\n\t\t]\n");
    fprintf(f, "\t}\n]\n");
    fclose(f);

    printf("Saved: %d\n", current_motion);
}



void
MotionRecorder::Save()
{
    *state = state_stop;
    mode_string = "Stop";

    if(file_name)
    {
        char fname[1024];
        snprintf(fname, 1023, file_name, current_motion);
        fname[1023] = 0;

        FILE * f = fopen(fname, "w");

        if(!f)
        {
            printf("ERROR could not save to motion file \"%s\"\n", fname);
            return;
        }

        fprintf(f, "TIME/1  POSITION/%d\n", size);

        for(int j=0; j<position_data_count[current_motion]-1; j++) // FIXME: should 1 be subtracted or not???
        {
            long t = j*GetTickLength();
            fprintf(f, "%ld\t", t);

            for(int i=0; i<size; i++)
            {
                if(i!=0) fprintf(f, "\t");
                fprintf(f, "%.4f", position_data[current_motion][j][i]);
            }

            fprintf(f, "\n");
        }

        fclose(f);

        printf("Saved: %d\n", current_motion);
    }

    // Also save in new JSON format

    if(json_file_name)
        SaveAsJSON();

    *time = 0;
}



void
MotionRecorder::Load() // SHOULD READ WIDTH FROM FILE AND CHECK THAT IT IS CORRECT;  // FIXME: causes output to change somehow!!!
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
    // TEST: Copy current data to webui output
    
    for(int c=0; c<2; c++)
        for(int i=0; i<1000; i++)
        {
            keypoints[i][c] = position_data[current_motion][i][c]; // position_data_count[current_motion]
            timestamps[i] = float(i)*20;    // ms
        }
    
    if(GetTick() < 20) // wait for valid data
    {
        copy_array(start_position, input, size);
        copy_array(stop_position, input, size);
        copy_array(output, input, size);
        reset_array(enable, size);
        return;
    }

    reset_array(completed, max_motions);

    if(trig) // FIXME: call rectord or play functions instead after setting current_motion
    {
        if(record_on_trig)
        {
            for(int i=0; i<trig_size; i++)
            {
                if(trig[i] == 1 && trig_last[i] == 0)
                {
                    current_motion = i;

                    *state = state_record;

                    set_array(enable, 0, size); // disable all
                    if(mode)
                    {
                        for(int i=0; i<size; i++)
                            if(mode[mode_stop][i] || mode[mode_play][i])
                            {
                                enable[i] = 1;
                            }
                            else
                            {
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

                    set_array(enable, 1, size);
                    if(mode)
                    {
                        for(int i=0; i<size; i++)
                            if(mode[mode_off][i]) // disable some channels
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

        set_array(enable, 1, size);
        if(mode)
        {
            for(int i=0; i<size; i++)
                if(mode[mode_off][i]) // disable some channels
                    enable[i] = 0;
         }

            for(int i=0; i<size; i++)
                if(mode[mode_play][i]) // disable some channels
                    output[i] = stop_position[i];
     }

    else if(*state == state_off)
    {
        *time = 0;
        for(int i=0; i<size; i++)
        {
            if(mode[mode_stop][i] == 0)
            {
                output[i] = input[i];
                enable[i] = 0; 
            }
        }
    }

    else if(*state == state_record)
    {
        for(int i=0; i<size; i++)
            if(mode[mode_stop][i] == 0)
                output[i] = input[i];

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
                if(mode[mode_play][i] == 1 || mode[mode_record][i] == 1)
                    if(position_data[current_motion][f] != 0)
                        output[i] = (1-a) * start_position[i] + a * position_data[current_motion][f][i];
            }
        }
        else
        {
            for(int i=0; i< size; i++)
                if(mode[mode_play][i] == 1 || mode[mode_record][i] == 1)
                    output[i] = position_data[current_motion][f][i];
        }
//           copy_array(output, position_data[current_motion][f], size);

        if(f < position_data_count[current_motion]-1)
            *time += timebase;
        else
        {
            completed[current_motion] = 1;
            *state = state_stop;
            mode_string = "Stop";
//            copy_array(stop_position, input, size);
            for(int i=0; i< size; i++)
            {
                if(mode[mode_play][i] == 1)
                {
                    stop_position[i] = input[i];
                    output[i] = stop_position[i];
                }
            }
            set_array(enable, 1, size);
            if(mode)
            {
                for(int i=0; i<size; i++)
                    if(mode[mode_off][i]) // disable some channels
                        enable[i] = 0;
             }
        }
    }

    if(*state != state_play || f > 1)
        *trig_out = 0;

    if(trig)
        copy_array(trig_last, trig, trig_size);
}



static InitClass init("MotionRecorder", &MotionRecorder::Create, "Source/Modules/RobotModules/MotionRecorder/");

