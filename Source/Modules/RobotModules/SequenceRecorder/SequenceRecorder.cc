//
//	SequenceRecorder.cc		This file is a part of the IKAROS project
//
//    Copyright (C) 2015-2022 Christian Balkenius
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

#include <iostream>
#include <fstream>

using namespace ikaros;


std::string
make_timestamp(float t)
{
    char buff[100];
    int t_ms = int(t) %1000;
    int t_s = (int(t)/1000) % 60;
    int t_min = int(t/60000);
    snprintf(buff, 100, "%02d:%02d:%03d", t_min, t_s, t_ms);
    return buff;     
}



float
interpolate(float t, float t1, float t2, float p1, float p2) // linear interpolation
{
    float alpha = (t-t1) / (t2-t1);
    return p1 + alpha*(p2-p1);
}





void
SequenceRecorder::SetTargetForTime(float t)
{
    int n = sequence_data["sequences"][current_sequence]["keypoints"].size();

    // Handle empty sequence data
    if(n < 1)
        return; // FIXME: How should the output be set in this case - to input or to last output?

    // Find intermediate points and linearly interpolate
    // Calculate the position for each channel independently

    for(int c=0; c<channels; c++)
    {
        float p_left = 0;
        float p_right = 0;
        if(initial)
        {
            p_left = initial[c];
            p_right = initial[c];
        }
        float time_left = 0;
        float time_right = sequence_data["sequences"][current_sequence]["end_time"];

        // Start with the value in the first (non-null) keypoint 

        for(int ix=0; ix<n; ix++)
            if(!sequence_data["sequences"][current_sequence]["keypoints"][ix]["point"][c].is_null())
            {
                p_left = sequence_data["sequences"][current_sequence]["keypoints"][ix]["point"][c];
                break;
            }


        // Interate to keypoint just before the current time to find left keypoint

        int ix = 0;
        for( ; ix<n; ix++)
        {
            if(sequence_data["sequences"][current_sequence]["keypoints"][ix]["time"] > t)
                break; // we have passed the current time and possibly found the left value
            if(!sequence_data["sequences"][current_sequence]["keypoints"][ix]["point"][c].is_null())
            {
                // Store candidate left point and time
                p_left = sequence_data["sequences"][current_sequence]["keypoints"][ix]["point"][c]; 
                time_left = sequence_data["sequences"][current_sequence]["keypoints"][ix]["time"];
            }
        }
    
        p_right = p_left; // in case there are no more valid keypoints

        // time_right is the en of the equence already

        // Search for right keypoint

        for( ; ix<n; ix++)
        {
          if(!sequence_data["sequences"][current_sequence]["keypoints"][ix]["point"][c].is_null())
            {
                p_right = sequence_data["sequences"][current_sequence]["keypoints"][ix]["point"][c];
                time_right = sequence_data["sequences"][current_sequence]["keypoints"][ix]["time"];
                break;
            }
        }
        target[c] = interpolate(t, time_left, time_right, p_left, p_right);
    }
}



 void
 SequenceRecorder::SetOutputForChannel(int c)
 {
    if(channel_mode[c][0] == 1) //locked
    {
        // Do not change output
        active[c] = 1;
    }

    else if(channel_mode[c][1] == 1) //play
     {
        output[c] = target[c]; // SMOOTH HERE AS WELL
        active[c] = 1;
     }   

    else if(channel_mode[c][2] == 1) //record
     {
        output[c] = input[c];
        active[c] = 0;
     }   

    else if(channel_mode[c][3] == 1) //copy
     {
         output[c] = input[c];
         active[c] = 0;
     }
 }



void
SequenceRecorder::StartRecord()
{
//    sequence_data["sequences"][current_sequence]["keypoints"] = json::array();
}



static json create_sequence(int index)
{
    json sq;

    sq["name"] = "Sequence "+std::string(1,65+index);
    sq["start_time"] = 0;
    sq["start_mark_time"] = 0;
    sq["end_mark_time"] = 1000;
    sq["end_time"] = 1000;
    sq["keypoints"] = json::array();

    return sq;
}



void
SequenceRecorder::LoadJSON(std::string filename)
{
    if(!check_file_exists(filename.c_str()))
    {
        json data ;
        data["channels"] = channels;
        // data["ranges"] // FIXME: yes do!
          data["sequences"] = json::array();
          for(int i=0; i<9; i++)
            data["sequences"].push_back(create_sequence(i));  


        std::ofstream file(filename);
        file << data;               
    }

    std::ifstream i(filename);
    i >> sequence_data;
    std::string s = sequence_data["sequences"][current_sequence]["name"];
    int sz = sequence_data["sequences"][current_sequence]["keypoints"].size();


    sequence_names = "";
    std::string sep = "";

    for(auto s : sequence_data["sequences"])
    {
        std::string name = s["name"];
        sequence_names += sep + name;
        sep = ",";
    }
}


void        
SequenceRecorder::StoreJSON(std::string filename)
{
    std::ofstream file(filename);
    file << sequence_data;
}




void
SequenceRecorder::Init()
{
    Bind(channels, "channels");

    SetOutputSize("INPUT", channels); // Make sure that there is an input for every channel even if they are not connected

    Bind(positions, channels, "positions", true); // parameter size will be set by the value channels

    Bind(smoothing_time, "smoothing_time");

    Bind(state, 10, "state", true);
    int modes = 4;

    Bind(channel_mode, modes, channels, "channel_mode", true);
    Bind(time_string, "time");
    Bind(end_time_string, "end_time");
    Bind(position, "position");
    Bind(mark_start, "mark_start");
    Bind(mark_end, "mark_end");

    Bind(sequence_names, "sequence_names");
    Bind(current_sequence, "current_sequence");
    Bind(internal_control, channels, "internal_control", true);

    int do_size = 0;
    Bind(default_output, &do_size, "default_output");
    if(do_size == channels)
        initial = default_output;
    else if(do_size != 0)
        Notify(msg_warning,"Incorrect size for initial_data; does not match number of channels");

     filename = GetValue("filename"); // FIXME: check that file exists - or create it

    io(target, "TARGET");
    io(input, "INPUT");
    io(output, "OUTPUT");
    io(active, "ACTIVE");
    io(smoothing_start,"SMOOTHING_START");
    io(ready, "READY");

    for(int c=0; c<channels; c++)
        set_one_row(channel_mode, 3, c, 4, channels);
 
    LoadJSON(filename);

    Stop();
}



SequenceRecorder::~SequenceRecorder()
{
// auto save
}



void
SequenceRecorder::Stop()
{
    set_one(state, 0, states);
    timer.Stop();
    timer.Reset();
}



void
SequenceRecorder::Play()
{
    if(sequence_data["sequences"][current_sequence]["end_mark_time"] == 0)
    /*
    {
        Pause(); // pause immediately to prevent time from running 
        return;
    }
*/
    set_one(state, 1, states); 
      timer.Start();
}



void
SequenceRecorder::Record()
{
    StartRecord();
    set_one(state, 2, states); 
    timer.Start();
}



void
SequenceRecorder::Pause()
{
    set_one(state, 3, states); 
    timer.Stop();
}



void
SequenceRecorder::SkipStart()
{
    timer.Stop();
    set_one(state, 3, states);// Pause
    if(timer.GetTime() <= sequence_data["sequences"][current_sequence]["start_mark_time"])
        timer.SetTime(sequence_data["sequences"][current_sequence]["start_time"]);
    else if(timer.GetTime() <= sequence_data["sequences"][current_sequence]["end_mark_time"])
        timer.SetTime(sequence_data["sequences"][current_sequence]["start_mark_time"]);
    else
        timer.SetTime(sequence_data["sequences"][current_sequence]["end_mark_time"]);
}



void
SequenceRecorder::SkipEnd()
{
    timer.Stop();
    set_one(state, 3, states);
    if(timer.GetTime() >= sequence_data["sequences"][current_sequence]["end_mark_time"])
        timer.SetTime(sequence_data["sequences"][current_sequence]["end_time"]);
    else if(timer.GetTime() >= sequence_data["sequences"][current_sequence]["start_mark_time"])
        timer.SetTime(sequence_data["sequences"][current_sequence]["end_mark_time"]);
    else
        timer.SetTime(sequence_data["sequences"][current_sequence]["start_mark_time"]);
}



void
SequenceRecorder::SetStartMark()
{
    sequence_data["sequences"][current_sequence]["start_mark_time"] = timer.GetTime();
}



void
SequenceRecorder::SetEndMark()
{
    sequence_data["sequences"][current_sequence]["end_mark_time"] = timer.GetTime();

}



void
SequenceRecorder::GoToPreviousKeypoint()
{
    float t = timer.GetTime();
    int n = sequence_data["sequences"][current_sequence]["keypoints"].size();
    float end_time = sequence_data["sequences"][current_sequence]["end_time"];
    float kpt_last = 0;
    for(int i=0; i<n; i++)
    {
        float kpt = sequence_data["sequences"][current_sequence]["keypoints"][i]["time"];
        if(kpt >= t)
        {
            timer.SetTime(kpt_last);
            position = end_time? kpt_last/end_time : 0; //Fix me: use set time function
            return;
        }
        else
            kpt_last = kpt;
    }
    timer.SetTime(kpt_last);
    position = end_time? kpt_last/end_time : 0;
}



void
SequenceRecorder::GoToNextKeypoint()
{
    float t = timer.GetTime();
    int n = sequence_data["sequences"][current_sequence]["keypoints"].size();
    float end_time = sequence_data["sequences"][current_sequence]["end_time"];
    float kpt = 0;
    for(int i=0; i<n; i++)
    {
        kpt = sequence_data["sequences"][current_sequence]["keypoints"][i]["time"];
        if(kpt > t)
        {
            timer.SetTime(kpt);
            position = end_time? kpt/end_time : 0;
            return;
        }
    }
    timer.SetTime(kpt);
    position = end_time? kpt/end_time : 0;
}


void
SequenceRecorder::ExtendTime() // add one second to the end of the sequence
{
    float end_time = sequence_data["sequences"][current_sequence]["end_time"];
    sequence_data["sequences"][current_sequence]["end_time"] = 1000.0f+1000*int(0.001*end_time);
}



void
SequenceRecorder::ReduceTime()
{
    float end_time = sequence_data["sequences"][current_sequence]["end_time"];
    end_time = -1000.0f+1000*int(0.001*end_time+0.99999);
     sequence_data["sequences"][current_sequence]["end_time"] = end_time > 0 ? end_time: 0;
}



void
SequenceRecorder::LockChannel(int c)
{
    if(c < 0)
        return;
    if(c >= channels)
        return;
    if(internal_control[c])
        output[c] = positions[c];
    else
        output[c] = input[c]; // Make sure output is at the present servo position
}



void
SequenceRecorder::AddKeypoint()
{
    if(!initial)
        return;

    float time = timer.GetTime();

    // Create the point data array

    json points = json::array();
    for(int c=0; c<channels; c++)
    {
        if(channel_mode[c][0] == 1) //locked
        {
            points.push_back(nullptr); // Do not record locked channel???
        }

        else if(channel_mode[c][1] == 1) //play - use current target which contains interpolated value already //FIXME: CHECK THIS
        {
            points.push_back(target[c]);
        }   

        else if(channel_mode[c][2] == 1) //record - store current input - OR POSSIBLY SLIDERS - HOW?
        {
            points.push_back(input[c]);
        }   

        else if(channel_mode[c][3] == 1) //copy - do not record this channel but use null
        {
            points.push_back(nullptr);
        }
       else // default
        {
            points.push_back(nullptr);
        }
    }
    // Find closest keypoint

    int n = sequence_data["sequences"][current_sequence]["keypoints"].size();
    json kp;
    int kpi = 0;
    float min_dist = 9999999;
    for(int i=0; i<n; i++)
    {
        float t = sequence_data["sequences"][current_sequence]["keypoints"][i]["time"];
        if(abs(t-time) < min_dist)
        {
            min_dist = abs(t-time);
            kp = sequence_data["sequences"][current_sequence]["keypoints"][i];
            kpi = i;
        }
    }
    // Insert or merge keypoint

    if(kp.is_null()) // no data - just insert new keypoint at the end of list
    {
        json keypoint;
        keypoint["time"] = int(time);
        keypoint["point"] = points;
        sequence_data["sequences"][current_sequence]["keypoints"].push_back(keypoint);
    //        printf("%s\n", sequence_data.dump().c_str());
        return;
    }

    // See if keypoints can be merged

    if(min_dist < 0.5*float(GetTickLength())) // half the ticklength
    {
        for(int c=0; c<channels; c++)
        {
//            printf("%s\n", points[c].dump().c_str());
            if(!points[c].is_null())
                    //kp["points"][c] = points[c]; // replace value in closest keypoint
                    sequence_data["sequences"][current_sequence]["keypoints"][kpi]["point"][c] = points[c];
        }
//        printf("%s\n", sequence_data.dump().c_str());
        return;
    }

    // Insert new keypoint at the correct place in the list

    json keypoint;
    keypoint["time"] = int(time);
    keypoint["point"] = points;

    // FIND INSERT POINT LOOP

    for(int i=0; i<n; i++)
    {
        float t = sequence_data["sequences"][current_sequence]["keypoints"][i]["time"];
        if(time < t) // insert before this
        {
            //INSERT BEFORE THIS
            sequence_data["sequences"][current_sequence]["keypoints"].insert(sequence_data["sequences"][current_sequence]["keypoints"].begin() + i, keypoint);
    //        printf("INSERT: %s\n", sequence_data.dump().c_str());
            return;
        }
    }
    // Insert last

    json keypoint_l;
    keypoint_l["time"] = int(time);
    keypoint_l["point"] = points;
    sequence_data["sequences"][current_sequence]["keypoints"].push_back(keypoint_l);
//    printf("%s\n", sequence_data.dump().c_str());
}



void
SequenceRecorder::PushKeypoint() // Add keypoint at the end - only works during record - and for all channels
{
    float time = timer.GetTime();

    json points = json::array();
    for(int c=0; c<channels; c++)
        points.push_back(input[c]); // FIXME: compare 'positions'  

    json keypoint;
    keypoint["time"] = time;
    keypoint["point"] = points;

    sequence_data["sequences"][current_sequence]["keypoints"].push_back(keypoint);
}



void
SequenceRecorder::ClearSequence()
{
    sequence_data["sequences"][current_sequence]["keypoints"] = json::array();
}



void
SequenceRecorder::DeleteKeypoints()
{
    float start_mark_time = float(sequence_data["sequences"][current_sequence]["start_mark_time"]);
    float end_mark_time = float(sequence_data["sequences"][current_sequence]["end_mark_time"]);
    int n = sequence_data["sequences"][current_sequence]["keypoints"].size();
    for(int i=0; i<n; i++)
    {
        float t = sequence_data["sequences"][current_sequence]["keypoints"][i]["time"];
        if(start_mark_time <= t && t<=end_mark_time)
        {
            int number_of_deleted_points = 0;
            for(int c=0; c<channels; c++)
            {
                if(channel_mode[c][2] == 1)
                {
                    sequence_data["sequences"][current_sequence]["keypoints"][i]["point"][c] = nullptr;
                    number_of_deleted_points++;
                }
            }
            if(number_of_deleted_points == channels)
            {
                // FIXME: Delete the complete keypoint (i) if possible during iterations
            }
        }
    }
}



void
SequenceRecorder::Command(std::string s, float x, float y, std::string value)
{
    if(s == "stop")
        Stop();
    else if (s == "play")
        Play();
    else if (s == "record")
        Record();
    else if (s == "pause")
        Pause();
    else if (s == "skip_start")
        SkipStart();
    else if (s == "skip_end")
        SkipEnd();
    else if (s == "set_start_mark")
        SetStartMark();
    else if (s == "set_end_mark")
        SetEndMark();
    else if (s == "step_forward")
        GoToNextKeypoint();
    else if (s == "step_backward")
        GoToPreviousKeypoint();
    else if (s == "extend_time")
        ExtendTime();
    else if (s == "reduce_time")
        ReduceTime();
    else if (s == "add_keypoint")
        AddKeypoint();
    else if(s == "set_initial")
        SetInitial();
    else if(s =="save")
            StoreJSON(filename);
    else if(s =="load")
            LoadJSON(filename);
    else if(s=="clear")
            ClearSequence();
    else if(s=="delete")
        DeleteKeypoints();
    else if(s=="lock")
        LockChannel(y);

}



std::string
SequenceRecorder::GetJSONData(const std::string & name, const std::string & tab)
{
    if(name=="RANGES")
        return sequence_data["ranges"].dump();

    else if(name=="SEQUENCE")
    {
        std::string sq = sequence_data["sequences"][current_sequence].dump();
    //    printf(">>> %s\n", sq.c_str());
        return sq;
    }

    else
        return "";
}



void
SequenceRecorder::SetInitial() // Manual setting of initial position
{
    if(!initial)
        initial = copy_array(create_array(channels), input, channels);
    else
        copy_array(initial, input, channels);
    copy_array(target, input, channels);
    copy_array(output, input, channels);
    //FIXME: Set smoothing start also
}



void
SequenceRecorder::Tick()
{
    float t = timer.GetTime();

    // Set initial position if not set already - this is used as output when no data is available

    if(!initial && norm1(input, channels) != 0)
    {
        initial = copy_array(create_array(channels), input, channels);
        copy_array(target, input, channels);
        copy_array(output, input, channels);
        //FIXME: Set smoothing start also
    }

    *ready = initial ? 1 : 0;

    // Check if position has been changed from WebUI - should use command in the future

    if(position != last_position) 
    {
            Pause();
            float end_time = sequence_data["sequences"][current_sequence]["end_time"];
            timer.SetTime(position*end_time);
            last_position = position;
    }

    // Set position

    float end_time = sequence_data["sequences"][current_sequence]["end_time"];
    position = end_time? t/end_time : 0;
    last_position = position;
    
    if(initial) // Do nothing until initial is set
    {
        // Set inputs from parameters for internal channels

        for(int c=0; c<channels; c++)
            if(internal_control[c])
                input[c] = positions[c];

        if(state[1]) // handle play mode
        {
            if(state[8] && t >= float(sequence_data["sequences"][current_sequence]["end_mark_time"])) // loop
            {
                timer.SetTime(float(sequence_data["sequences"][current_sequence]["start_mark_time"]));
            }
            else if(position >= 1 || end_time == 0)
            {   
                timer.SetTime(sequence_data["sequences"][current_sequence]["end_time"]);
                Pause();
        }   }

        else if(state[2]) // handle record mode
        {
            if(position >= 1 || end_time == 0) // extend recoding if at end
                sequence_data["sequences"][current_sequence]["end_time"] = t;

        }

        // Set outputs

        SetTargetForTime(t);

        // FIXME: Add smoothing here

        for(int c=0; c<channels; c++)
            SetOutputForChannel(c);

        // AddPoints if in recording mode

        if(state[2] == 1) // record mode
            AddKeypoint();
    }

// Set position again

    end_time = sequence_data["sequences"][current_sequence]["end_time"];
    position = end_time? t/end_time : 0;
    last_position = position;

    time_string = make_timestamp(timer.GetTime());
    end_time_string  = make_timestamp(sequence_data["sequences"][current_sequence]["end_time"]);

    if(float(end_time = sequence_data["sequences"][current_sequence]["end_time"]) != 0)
    {
        mark_start = float(sequence_data["sequences"][current_sequence]["start_mark_time"])/float(end_time = sequence_data["sequences"][current_sequence]["end_time"]);
        mark_end = float(sequence_data["sequences"][current_sequence]["end_mark_time"])/float(end_time = sequence_data["sequences"][current_sequence]["end_time"]);}

    // Set positions parameter for exterbally controlled channels

    for(int c=0; c<channels; c++)
            if(internal_control[c] == 0)
                positions[c] = output[c];
}


static InitClass init("SequenceRecorder", &SequenceRecorder::Create, "Source/Modules/RobotModules/SequenceRecorder/");

