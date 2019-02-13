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

enum { run_mode_stop, run_mode_off, run_mode_record, run_mode_play };
enum { channel_mode_off=0, channel_mode_stop, channel_mode_play, channel_mode_record };



void
SequenceRecorder::Init()
{
    Bind(record_on_trig, "record_on_trig");
    Bind(smoothing_time, "smoothing_time");

    Bind(current_motion, "current_motion");
    Bind(run_mode_string, "run_mode_string");
    
    max_motions = GetIntValue("max_motions");
    current_motion = 0;

    input = GetInputArray("INPUT");
    size = GetInputSize("INPUT");
    
    stop_position = create_array(size);
    start_position = create_array(size);
    lock_position = create_array(size);
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
    channel_mode = GetOutputMatrix("MODE");
    for(int i=0; i<size; i++)
        channel_mode[channel_mode_off][i] = 1;

    run_mode = GetOutputArray("STATE");
    *run_mode = run_mode_stop;

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



SequenceRecorder::~SequenceRecorder()
{
    if(auto_save)
    {
        for(current_motion=0; current_motion<max_motions; current_motion++)
            Save();
    }

    destroy_array(start_torque);
    destroy_array(start_position);
    destroy_array(stop_position);
    destroy_array(lock_position);
    destroy_matrix(position_data);
    destroy_array(trig_last);
    delete [] position_data_count;
}


void
SequenceRecorder::SetOutputs(float * position)
{
    for(int i=0; i<size; i++)
    {
        switch(arg_max_col(channel_mode, i, size, 4))
        {
            case channel_mode_off:
                output[i] = input[i];
                enable[i] = 0;
                break;
            
            case channel_mode_stop:
                output[i] = lock_position[i];
                enable[i] = 1;
                break;
            
            case channel_mode_play:

                output[i] = position[i];
                enable[i] = 1;
                break;
            
            case channel_mode_record:
                if(run_mode_record)
                {
                output[i] = input[i];
                enable[i] = 0;
                }
                //else if
                break;

            default: ;
        }
    }


        /*
        if(channel_mode[channel_mode_off][i])
        {
            output[i] = input[i];
            enable[i] = 0;
        }
        else if(channel_mode[channel_mode_stop][i])
        {
            output[i] = lock_position[i];
            enable[i] = 1;
        }
        else if(channel_mode[channel_mode_play][i])
        {
            output[i] = position[i];
            enable[i] = 1
        }
        else if(channel_mode[channel_mode_record][i] && run_mode == run_mode_record)
        {
            output[i] = input[i];
            enable[i] = 0;
        }
        else if(channel_mode[channel_mode_record][i] && run_mode == run_mode_play)
        {
            output[i] = input[i];
            enable[i] = 0;
        }
        */
}


void
SequenceRecorder::Command(std::string s, float x, float y, std::string value)
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
SequenceRecorder::ToggleMode(int channel, int y)
{
    set_one_col(channel_mode, channel, y, size, 4);
    if(y==channel_mode_stop) // Store servo position if stop is selected
        lock_position[channel] = input[channel];
}



void
SequenceRecorder::Off()
{
    run_mode_string = "Off";
    *run_mode = run_mode_off;
/*
    copy_array(output, input, size); // Immediate no torque response even before the button is released
    for(int i=0; i<size; i++)
        if(channel_mode[channel_mode_stop][i] == 0)
            enable[i] = 0;
*/
}



void
SequenceRecorder::Stop()
{
    run_mode_string = "Stop";
    *run_mode = run_mode_stop;
    
    copy_array(stop_position, input, size); // FIXME: where is this used???


    // If we record - copy position for all recorded channels to the rest of the track
    // This is used if we have already recorded something longer on another channel
    // Will not be needed when interpolation is used and we know the end of the recording on each channel
    // Interpolation after last position should use last position (as interpolation before)

    if(*run_mode == run_mode_record)
    {
        for(int i=0; i<size; i++)
            if(channel_mode[channel_mode_record][i])
                for(int p=int(*time/float(timebase)); p<position_data_max; p++)
                    position_data[current_motion][p][i] = input[i];
    }

}


void
SequenceRecorder::Record()
{
    run_mode_string = "Rec";
    *run_mode = run_mode_record;
    position_data_count[current_motion] = 0;
    *time = 0;
}


void
SequenceRecorder::Play()
{
    run_mode_string = "Play";
    *run_mode = run_mode_play;
    *trig_out = 1; // FIXME: Should be set by sequenceer on tick 1 instead
    *time = 0;
}


void
SequenceRecorder::SaveAsJSON()
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
SequenceRecorder::Save()
{
    *run_mode = run_mode_stop;
    run_mode_string = "Stop";

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
SequenceRecorder::Load() // SHOULD READ WIDTH FROM FILE AND CHECK THAT IT IS CORRECT;  // FIXME: causes output to change somehow!!!
{
    *run_mode = run_mode_stop;
    run_mode_string = "Stop";
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

                    *run_mode = run_mode_record;

                    set_array(enable, 0, size); // disable all
                    if(channel_mode)
                    {
                        for(int i=0; i<size; i++)
                            if(channel_mode[channel_mode_stop][i] || channel_mode[channel_mode_play][i])
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
                    *run_mode = run_mode_play;

                    copy_array(start_position, input, size);

                    set_array(enable, 1, size);
                    if(channel_mode)
                    {
                        for(int i=0; i<size; i++)
                            if(channel_mode[channel_mode_off][i]) // disable some channels
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

    // Handle the different run_modes

    int f = int(*time/float(timebase));

    if(*run_mode == run_mode_stop)
    {
        *time = 0;

        set_array(enable, 1, size);
        if(channel_mode)
        {
            for(int i=0; i<size; i++)
                if(channel_mode[channel_mode_off][i]) // disable some channels
                    enable[i] = 0;
         }

            for(int i=0; i<size; i++)
                if(channel_mode[channel_mode_play][i]) // disable some channels
                    output[i] = stop_position[i];
     }

    else if(*run_mode == run_mode_off)
    {
        *time = 0;
        for(int i=0; i<size; i++)
        {
            if(channel_mode[channel_mode_stop][i] == 0)
            {
                output[i] = input[i];
                enable[i] = 0;
            }
        }
    }

    else if(*run_mode == run_mode_record)
    {
        for(int i=0; i<size; i++)
            if(channel_mode[channel_mode_stop][i] == 0)
                output[i] = input[i];

        if(position_data_count[current_motion] < position_data_max && norm1(input, size) > 0)
        {
            // copy_array(position_data[position_data_count], input, size);
            
            for(int i=0; i<size; i++)
                    if(channel_mode[channel_mode_record][i])
                        position_data[current_motion][position_data_count[current_motion]][i] = input[i];
            
                    else if(channel_mode[channel_mode_play][i])
                        if(position_data[current_motion][f][i] != 0)
                            output[i] = position_data[current_motion][f][i];

            position_data_count[current_motion]++;
        }
        else
            Notify(msg_warning, "Recording buffer full.");
        
        *time = float(position_data_count[current_motion]*timebase);
    }

    else if(*run_mode == run_mode_play)
    {
        if(f < smoothing_time)
        {
            float a = float(f)/float(smoothing_time);
            for(int i=0; i< size; i++)
            {
                if(channel_mode[channel_mode_play][i] == 1 || channel_mode[channel_mode_record][i] == 1)
                    if(position_data[current_motion][f] != 0)
                        output[i] = (1-a) * start_position[i] + a * position_data[current_motion][f][i];
            }
        }
        else
        {
            for(int i=0; i< size; i++)
                if(channel_mode[channel_mode_play][i] == 1 || channel_mode[channel_mode_record][i] == 1)
                    output[i] = position_data[current_motion][f][i];
        }
//           copy_array(output, position_data[current_motion][f], size);

        if(f < position_data_count[current_motion]-1)
            *time += timebase;
        else
        {
            completed[current_motion] = 1;
            *run_mode = run_mode_stop;
            run_mode_string = "Stop";
//            copy_array(stop_position, input, size);
            for(int i=0; i< size; i++)
            {
                if(channel_mode[channel_mode_play][i] == 1)
                {
                    stop_position[i] = input[i];
                    output[i] = stop_position[i];
                }
            }
            set_array(enable, 1, size);
            if(channel_mode)
            {
                for(int i=0; i<size; i++)
                    if(channel_mode[channel_mode_off][i]) // disable some channels
                        enable[i] = 0;
             }
        }
    }

    if(*run_mode != run_mode_play || f > 1)
        *trig_out = 0;

    if(trig)
        copy_array(trig_last, trig, trig_size);
}



static InitClass init("SequenceRecorder", &SequenceRecorder::Create, "Source/Modules/RobotModules/SequenceRecorder/");

