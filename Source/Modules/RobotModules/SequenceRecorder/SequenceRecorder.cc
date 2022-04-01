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
    int n = sequence_data["sequences"][0]["keypoints"].size();

    // Handle empty sequence data
    if(n < 1)
        return; // FIXME: How should the output be set in this case - to input or to last output?

    // Check start of sequence

    float t0 = sequence_data["sequences"][0]["keypoints"][0]["time"];
    if(t <= t0)
    {
        for(int i=0; i<channels; i++)
        {
            if(!sequence_data["sequences"][0]["keypoints"][0]["point"][i].is_null())
                target[i] = sequence_data["sequences"][0]["keypoints"][0]["point"][i];
            else
                target[i] = initial[i];
        }
        return;
    }

    // Check end of sequence

    float tn_1 = sequence_data["sequences"][0]["keypoints"][n-1]["time"];
    if(t >= tn_1)
    {
        for(int i=0; i<channels; i++)
            if(!sequence_data["sequences"][0]["keypoints"][n-1]["point"][i].is_null())
                target[i] = sequence_data["sequences"][0]["keypoints"][n-1]["point"][i];
            // FIXME: else search for last viable non-null value or use initial - only necessary if we jumped directly to the end
        return;
    }

    // Find intermediate points and linearly interpolate - basic version - could add s-curves here later

    for(int i=0; i<n-1; i++)
    {
        float t1 = sequence_data["sequences"][0]["keypoints"][i]["time"];
        float t2 = sequence_data["sequences"][0]["keypoints"][i+1]["time"];
        if(t1 < t && t < t2)
        {

            for(int c=0; c<channels; c++)
            if(!sequence_data["sequences"][0]["keypoints"][i]["point"][c].is_null() && !sequence_data["sequences"][0]["keypoints"][i+1]["point"][c].is_null())
            {

                float p1 = sequence_data["sequences"][0]["keypoints"][i]["point"][c];
                float p2 = sequence_data["sequences"][0]["keypoints"][i+1]["point"][c];
                target[c] = interpolate(t, t1, t2, p1, p2);
            }
            // else don't change output
            return;
        }
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
//    sequence_data["sequences"][0]["keypoints"] = json::array();
}



void
SequenceRecorder::LoadJSON(std::string filename)
{
    if(!file_exists(filename.c_str()))
    {
        json data ;
        json sq_a;

        sq_a["name"] = "Sequence A";
        sq_a["start_time"] = 0;
        sq_a["start_mark_time"] = 0;
        sq_a["end_mark_time"] = 1000;
        sq_a["end_time"] = 1000;
        sq_a["keypoints"] = json::array();

        data["channels"] = channels;
        // data["ranges"] // FIXME: yes do!
          data["sequences"] = json::array();
          data["sequences"].push_back(sq_a);  

        std::ofstream file(filename);
        file << data;               
    }




    std::ifstream i(filename);
    i >> sequence_data;
    std::string s = sequence_data["sequences"][0]["name"];
    int sz = sequence_data["sequences"][0]["keypoints"].size();

}



void        
SequenceRecorder::StoreJSON(std::string filename)
{

}




void
SequenceRecorder::Init()
{
    Bind(channels, "channels");
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

     filename = GetValue("filename"); // FIXME: check that file exists - or create it

    io(target, "TARGET");
    io(input, "INPUT");
    io(output, "OUTPUT");
    io(active, "ACTIVE");
    io(smoothing_start,"SMOOTHING_START");
    io(ready, "READY");
    initial = NULL;

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
    if(sequence_data["sequences"][0]["end_mark_time"] == 0)
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
    if(timer.GetTime() <= sequence_data["sequences"][0]["start_mark_time"])
        timer.SetTime(sequence_data["sequences"][0]["start_time"]);
    else if(timer.GetTime() <= sequence_data["sequences"][0]["end_mark_time"])
        timer.SetTime(sequence_data["sequences"][0]["start_mark_time"]);
    else
        timer.SetTime(sequence_data["sequences"][0]["end_mark_time"]);
}



void
SequenceRecorder::SkipEnd()
{
    timer.Stop();
    set_one(state, 3, states);
    if(timer.GetTime() >= sequence_data["sequences"][0]["end_mark_time"])
        timer.SetTime(sequence_data["sequences"][0]["end_time"]);
    else if(timer.GetTime() >= sequence_data["sequences"][0]["start_mark_time"])
        timer.SetTime(sequence_data["sequences"][0]["end_mark_time"]);
    else
        timer.SetTime(sequence_data["sequences"][0]["start_mark_time"]);
}



void
SequenceRecorder::SetStartMark()
{
    sequence_data["sequences"][0]["start_mark_time"] = timer.GetTime();
}



void
SequenceRecorder::SetEndMark()
{
    sequence_data["sequences"][0]["end_mark_time"] = timer.GetTime();

}



void
SequenceRecorder::ExtendTime() // add one second to the end of the sequence
{
    float end_time = sequence_data["sequences"][0]["end_time"];
    sequence_data["sequences"][0]["end_time"] = 1000.0f+1000*int(0.001*end_time);
}



void
SequenceRecorder::ReduceTime()
{
    float end_time = sequence_data["sequences"][0]["end_time"];
    end_time = -1000.0f+1000*int(0.001*end_time+0.99999);
     sequence_data["sequences"][0]["end_time"] = end_time > 0 ? end_time: 0;
}



void
SequenceRecorder::AddKeypoint()
{
    float time = timer.GetTime();

    // Create the point data array

    json points = json::array();
    for(int c=0; c<channels; c++)
    {
        if(channel_mode[c][0] == 1) //locked
        {
            points.push_back(nullptr); // Do not record locked channel
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

    int n = sequence_data["sequences"][0]["keypoints"].size();
    json kp;
    float min_dist = 9999999;
    for(int i=0; i<n-1; i++)
    {
        float t = sequence_data["sequences"][0]["keypoints"][i]["time"];
        if(abs(t-time) < min_dist)
        {
            min_dist = abs(t-time);
            kp = sequence_data["sequences"][0]["keypoints"][i];
        }
    }
    // Insert or merge keypoint

    if(kp.is_null()) // no data - just insert new keypoint at the end of list
    {
        json keypoint;
        keypoint["time"] = time;
        keypoint["point"] = points;
        sequence_data["sequences"][0]["keypoints"].push_back(keypoint);
            printf("%s\n", sequence_data.dump().c_str());
        return;
    }

    // See if keypoints can be merged

    if(min_dist < 10) // 10 ms window // FIXME: should use a constant
    {
        for(int c=0; c<channels; c++)
        {
            if(!points[c].is_null())
                    kp[c] = points[c]; // replace value in closest keypoint
        }
        printf("%s\n", sequence_data.dump().c_str());
        return;
    }

    // Insert new keypoint at the correct place in the list

    json keypoint;
    keypoint["time"] = time;
    keypoint["point"] = points;

    // FIND INSERT POINT LOOP

    for(int i=0; i<n-1; i++)
    {
        float t = sequence_data["sequences"][0]["keypoints"][i]["time"];
        if(time < t) // insert before this
        {
            //INSERT BEFORE THIS
            sequence_data["sequences"][0]["keypoints"].insert(sequence_data["sequences"][0]["keypoints"].begin() + i, keypoint);
            printf("%s\n", sequence_data.dump().c_str());
            return;
        }
    }
    // Insert last

    json keypoint_l;
    keypoint_l["time"] = time;
    keypoint_l["point"] = points;
    sequence_data["sequences"][0]["keypoints"].push_back(keypoint_l);
    printf("%s\n", sequence_data.dump().c_str());
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

    sequence_data["sequences"][0]["keypoints"].push_back(keypoint);
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
    else if (s == "extend_time")
        ExtendTime();
    else if (s == "reduce_time")
        ReduceTime();
    else if (s == "add_keypoint")
        AddKeypoint();
    else if(s == "set_initial")
        SetInitial();

}



std::string
SequenceRecorder::GetJSONData(const std::string & name, const std::string & tab)
{
    if(name=="RANGES")
        return sequence_data["ranges"].dump();

    else if(name=="SEQUENCE")
    {
        std::string sq = sequence_data["sequences"][0].dump();
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
    // Set initial position if not set already - this is used as output when no data is available

    if(!initial && norm1(input, channels) != 0)
    {
        initial = copy_array(create_array(channels), input, channels);
        copy_array(target, input, channels);
        copy_array(output, input, channels);
        //FIXME: Set smoothing start also
    }

    *ready = initial ? 1 : 0; 
    if(!initial)
        return; // Do nothing until initial is set

// Check if position has been changed from WebUI - should use command in the future

    if(position != last_position) 
    {
            Pause();
            float end_time = sequence_data["sequences"][0]["end_time"];
            timer.SetTime(position*end_time);
            last_position = position;
    }

    // Set position

    float t = timer.GetTime();
    float end_time = sequence_data["sequences"][0]["end_time"];
    position = end_time? t/end_time : 0;
    last_position = position;
    
    if(state[1]) // handle play mode
    {
        if(state[8] && t >= float(sequence_data["sequences"][0]["end_mark_time"])) // loop
        {
            timer.SetTime(float(sequence_data["sequences"][0]["start_mark_time"]));
        }
        else if(position >= 1 || end_time == 0)
        {   
            timer.SetTime(sequence_data["sequences"][0]["end_time"]);
            Pause();
    }   }

    else if(state[2]) // handle record mode
    {
        if(position >= 1 || end_time == 0) // extend recoding if at end
            sequence_data["sequences"][0]["end_time"] = t;

    }

   // Set position again

    t = timer.GetTime();
    end_time = sequence_data["sequences"][0]["end_time"];
    position = end_time? t/end_time : 0;
    last_position = position;

// Set parameters and outputs

    time_string = make_timestamp(timer.GetTime());
    end_time_string  = make_timestamp(sequence_data["sequences"][0]["end_time"]);

    if(float(end_time = sequence_data["sequences"][0]["end_time"]) != 0)
    {
        mark_start = float(sequence_data["sequences"][0]["start_mark_time"])/float(end_time = sequence_data["sequences"][0]["end_time"]);
        mark_end = float(sequence_data["sequences"][0]["end_mark_time"])/float(end_time = sequence_data["sequences"][0]["end_time"]);}

    // Set the outputs

    SetTargetForTime(t);

    // FIXME: Add smoothing here

    for(int c=0; c<channels; c++)
        SetOutputForChannel(c);

    // AddPoints if in recording mode

    if(state[2] == 1) // record mode
        AddKeypoint();
}


static InitClass init("SequenceRecorder", &SequenceRecorder::Create, "Source/Modules/RobotModules/SequenceRecorder/");

