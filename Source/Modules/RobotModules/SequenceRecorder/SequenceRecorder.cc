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



static int
find_index_for_time(json & keypoints, float t)
{
    int n = keypoints.size();

    // binary search for nearest keypoint
    int low = 0;
    int high = n-1;
    int i = 0;
    while (low <= high)
    {

        i = (low + high) / 2;
        if ((float)(keypoints[i]["time"]) > t)
            high = i - 1;
        else if ((float)(keypoints[i]["time"]) < t)
            low = i + 1;
        else
        {
            low = i+1;
            break;
        }
    }

    return low;
}



void
SequenceRecorder::SetTargetForTime(float t)
{
    //printf("%f\n", t);
    auto & keypoints = sequence_data["sequences"][current_sequence]["keypoints"];
    int n = keypoints.size();
    int i = find_index_for_time(keypoints, t);


    // Check if no keypoints: use default output as target

    if(n==0)
    {
        for(int c=0; c<channels; c++)
            target[c] = default_output[c];
        return;
    }

    // Check if index is zero OR there is only one keypoint: use first keypoint

    if(i == 0 || n==1)
    {
        for(int c=0; c<channels; c++)
        if(keypoints[0]["point"][c].is_null())
            target[c] = default_output[c];
        else
            target[c] = keypoints[0]["point"][c];

        return;
    }

    // Check if we are at or after the last keypoint: use last keypoint

    if(i > n-1)
    {
        for(int c=0; c<channels; c++)
        if(keypoints[n-1]["point"][c].is_null())
            target[c] = default_output[c];
        else
            target[c] = keypoints[n-1]["point"][c];

        return;
    }

    // Do normal interpolation

    for(int c=0; c<channels; c++)
    {
    
        // Process left point

        auto & kp_left = keypoints[i-1];
        float time_left = keypoints[i-1]["time"];
        float point_left = default_output[c];
    

        if(!kp_left["point"][c].is_null()) //keypoint has data
        {
            point_left = kp_left["point"][c];
        }
        else if(!kp_left["link_left"][c].is_null()) // use linked keypoint if it exists
        {
            int l = kp_left["link_left"][c];
            if(l!=-1 && !keypoints[l]["point"][c].is_null()) // check that linked keypoint has data - as should be the case
            {
                point_left = keypoints[l]["point"][c];
                time_left = keypoints[l]["time"];
            }
        }

        auto & kp_right = keypoints[i];
        float time_right = keypoints[i]["time"];
        float point_right = default_output[c];
    

        if(!kp_right["point"][c].is_null()) //keypoint has data
        {
            point_right = kp_right["point"][c];
        }
        else if(!kp_right["link_right"][c].is_null()) // use linked keypoint if it exists
        {
            int l = kp_right["link_right"][c];
            if(l!=-1 && !keypoints[l]["point"][c].is_null()) // check that linked keypoint has data - as should be the case
            {
                point_right = keypoints[l]["point"][c];
                time_right = keypoints[l]["time"];
            }
        }

        target[c] = interpolate(t, time_left, time_right, point_left, point_right);
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
         if(internal_control[c] == 1)
            active[c] = 1;
     }   

    else if(channel_mode[c][3] == 1) //copy
     {
         output[c] = input[c];
         active[c] = 0;
         if(internal_control[c] == 1)
            active[c] = 1;
     }
 }



void
SequenceRecorder::StartRecord()
{
    last_record_position = position;
    start_record = true;
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

    std::ifstream ifs(filename);
    sequence_data = json::parse(ifs);


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
    LinkKeypoints(); // TEST **********

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
 
    left_time = create_array(channels);
    right_time = create_array(channels);
    left_position = create_array(channels);
    right_position = create_array(channels);
    left_index = new int[channels];
    right_index = new int[channels];
    for(int c=0; c<channels; c++)
    {   
        left_index[c] = 0;
        right_index[c] = INT_MAX;
    }

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
    LinkKeypoints(); // at end of recording
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
    //timer.Start();
}



void
SequenceRecorder::Pause()
{
    set_one(state, 3, states); 
    timer.Stop();
    LinkKeypoints(); // at end of recording
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
SequenceRecorder::GoToTime(float time)
{
    SetTargetForTime(time);

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
SequenceRecorder::LinkKeypoints()
{
    auto & keypoints = sequence_data["sequences"][current_sequence]["keypoints"];
    int n = keypoints.size();

    int left_link[channels];
    int right_link[channels];

    for(int c=0; c<channels; c++)
    {
        left_link[c] = -1;
        right_link[c] = -1;
    }

    // left to right sweep

    for(int i=0; i<n; i++)
    {
        for(int c=0; c<channels; c++)
            if(!keypoints[i]["point"][c].is_null()) // channel has data from this keypoint
                left_link[c] = i;
        for(int c=0; c<channels; c++)
            keypoints[i]["link_left"][c] = left_link[c];
    }

    // right to left sweep

    for(int i=n-1; i>0; i--)
    {
        for(int c=0; c<channels; c++)
            if(!keypoints[i]["point"][c].is_null()) // channel has data from this keypoint
                right_link[c] = i;
        for(int c=0; c<channels; c++)
            keypoints[i]["link_right"][c] = right_link[c];
    }

}



void
SequenceRecorder::AddKeypoint(float time)
{   
    auto & keypoints = sequence_data["sequences"][current_sequence]["keypoints"];
    int n = keypoints.size();

    long tl = GetTickLength();
    float qtime = tl*(int(time)/tl); // Quantized time

    // Create the point data array

    json points = json::array();
    for(int c=0; c<channels; c++)
        if(channel_mode[c][0] == 1) //locked
            points.push_back(nullptr); // Do not record locked channel???
        else if(channel_mode[c][1] == 1) //play - use null to indicate nodata
            points.push_back(nullptr); // was target[c]
        else if(channel_mode[c][2] == 1) //record - store current input (or sliders)
            points.push_back(input[c]);
        else if(channel_mode[c][3] == 1) //copy - do not record this channel but use null
            points.push_back(nullptr);
        else // default
            points.push_back(nullptr);

    int i = find_index_for_time(keypoints, time);
    if(n>0 && i<n)
    {
        // Find position in list
        int i = find_index_for_time(keypoints, time);

        //printf(">>>%d\n", i);
        if(i==0) // insert first
        {
            json keypoint;
            keypoint["time"] = qtime;
            keypoint["point"] = points;
            keypoints.insert(keypoints.begin(), keypoint);
            //printf("INSERT FIRST\n");
            return;
        }

        float t = keypoints[i-1]["time"];
        if(qtime == t) // merge
        {
            for(int c=0; c<channels; c++)
            if(!points[c].is_null())
                keypoints[i-1]["point"][c] = points[c];
            //printf("MERGED AT %d\n", i-1);
            return;
        }
        else if(qtime < keypoints[i]["time"]) // insert before
        {
            json keypoint;
            keypoint["time"] = qtime;
            keypoint["point"] = points;
            keypoints.insert(keypoints.begin() + i, keypoint);
            //printf("INSERT BEFORE %d\n", i);
            return;
        }
    }

    // Check if we are editing last - handle that separately

    if(n>0 && i==n && keypoints[n-1]["time"] == qtime)
    {
            for(int c=0; c<channels; c++)
            if(!points[c].is_null())
                keypoints[n-1]["point"][c] = points[c];
            //printf("MERGED AT LAST\n");
            return;
    }

    // insert last - also for first keypoint to be inserted

    json keypoint;
    keypoint["time"] = qtime;
    keypoint["point"] = points;
    keypoints.push_back(keypoint);
            //printf("INSERT LAST\n");
}




void
SequenceRecorder::ClearSequence()
{
        static int last_index = 0;
    static float last_time = 0;

    sequence_data["sequences"][current_sequence]["keypoints"] = json::array();
    sequence_data["sequences"][current_sequence]["start_time"] = 0;
    sequence_data["sequences"][current_sequence]["start_mark_time"] = 0;
    sequence_data["sequences"][current_sequence]["end_mark_time"] = 1000;
    sequence_data["sequences"][current_sequence]["end_time"] = 1000;
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
                if(channel_mode[c][2] == 1) // record mode
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
SequenceRecorder::DeleteKeypointsInRange(float t0, float t1)
{
    int n = sequence_data["sequences"][current_sequence]["keypoints"].size();
    for(int i=0; i<n; i++)
    {
        float t = float(sequence_data["sequences"][current_sequence]["keypoints"][i]["time"]);
        if(t0 < t && t<=t1)
        {
            int number_of_null_points = 0;
            for(int c=0; c<channels; c++)
            {
                if(channel_mode[c][2] == 1) // record mode
                {
                    sequence_data["sequences"][current_sequence]["keypoints"][i]["point"][c] = nullptr;
                }
                if(sequence_data["sequences"][current_sequence]["keypoints"][i]["point"][c].is_null())
                    number_of_null_points++;
            }
        
            if(number_of_null_points == channels)
            {
                // FIXME: Delete the complete keypoint (i) if possible during iterations
                //printf("Delete %f - %f, %f\n", t0, t1, t);
            }
        }
    }
}



void
SequenceRecorder::Trig(int id)
{
    Stop();
    printf("START >>>>> %d\n", id);
    current_sequence = id;
    Play();
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
        AddKeypoint(timer.GetTime());
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
    else if(s=="trig")
        Trig(x);
}


std::string
SequenceRecorder::GetJSONData(const std::string & name, const std::string & tab)
{
    if(name=="RANGES")
        return sequence_data["ranges"].dump();

    else if(name=="SEQUENCE")
    {
        int n = sequence_data["sequences"][current_sequence]["keypoints"].size();
        if(n>400) // state[2] ||  // 400 = 10 s
        {
            return "{}"; //Minimal JSON
        }
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
    if(start_record) // timer start at tick to increase probability of overlapping keypoint when starting at a keypoint
    {                // FIXME: May want to jump to closest keypoint if dense recording is used
        timer.Start();
        start_record = false;
    }

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

        GoToTime(t);

        // FIXME: Add smoothing here

        for(int c=0; c<channels; c++)
            SetOutputForChannel(c);

        // AddPoints if in recording mode

        if(state[2] == 1) // record mode
        {
            // DeleteKeypointsInRange(last_record_position+0.001, t); // *******************
            //printf(">>> %f - %f\n", last_record_position, t);
            last_record_position = t;
            AddKeypoint(t);
        }
    }

// Set position again

    end_time = sequence_data["sequences"][current_sequence]["end_time"];
    position = end_time? t/end_time : 0;
    last_position = position;

    time_string = make_timestamp(t); // timer.GetTime()
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

