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

#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;

using namespace ikaros;

static std::string
make_timestamp(float t)
{
    char buff[100];
    int t_ms = int(t) %1000;
    int t_s = (int(t)/1000) % 60;
    int t_min = int(t/60000);
    snprintf(buff, 100, "%02d:%02d:%03d", t_min, t_s, t_ms);
    return buff;     
}


static inline float
quantize(float time, long q)
{
    return q*(int(time+q/2)/q);
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
            target[c] = left_output[c];
        else
            target[c] = keypoints[0]["point"][c];

        return;
    }

    // Check if we are at or after the last keypoint: use last keypoint

    if(i > n-1)
    {
        for(int c=0; c<channels; c++)
        if(keypoints[n-1]["point"][c].is_null())
            target[c] = right_output[c];
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
        float point_left = left_output[c];
    

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
        float point_right = right_output[c];
    

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

        if(interpolation[c]==0)
            target[c] = point_left;
        else // 1 = linear interpolation
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
        positions[c] = target[c];
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
}



void
SequenceRecorder::Rename(const std::string & new_name)
{
    sequence_data["sequences"][current_sequence]["name"] = new_name;
    UpdateSequenceNames();
}



void
SequenceRecorder::UpdateSequenceNames()
{
    sequence_names = "";
    std::string sep = "";

    for(auto s : sequence_data["sequences"])
    {
        std::string name = s["name"];
        sequence_names += sep + name;
        sep = ",";
    }
}



static json create_sequence(int index)
{
    json sq;

    sq["name"] = "Sequence "+std::string(1,65+index / 8)+std::string(1,49+index % 8);
    sq["start_time"] = 0;
    sq["start_mark_time"] = 0;
    sq["end_mark_time"] = 1000;
    sq["end_time"] = 1000;
    sq["keypoints"] = json::array();

    return sq;
}





void
SequenceRecorder::New()
{
    filename = "untitled"+std::to_string(untitled_count++)+".json";
    sequence_data = json(); // in case it is not empty
    sequence_data["type"] = "Ikaros Sequence Data";
    sequence_data["channels"] = channels;
    sequence_data["ranges"] = json::array();
    for(int c=0; c<channels; c++)
    {
        json range = json::array();
        range.push_back(range_min[c]);
        range.push_back(range_max[c]);
        sequence_data["ranges"].push_back(range);
    }
    sequence_data["sequences"] = json::array();
    for(int i=0; i<max_sequences; i++)
    {
            sequence_data["sequences"].push_back(create_sequence(i)); 
    }

    UpdateSequenceNames();
}



bool
SequenceRecorder::Open(const std::string & name)
{
     if(filename.empty())
        return false;

    filename = name;
    auto path = directory+"/"+filename;

    if(!check_file_exists(path.c_str()))
    {
        Notify(msg_warning, "File \"%s\" does not exist.", path.c_str());
        return false;
    }

    try
    {
        std::ifstream ifs(path);
        json data = json::parse(ifs);

        // Validate

        if(data["type"].is_null() || data["type"] != "Ikaros Sequence Data")
            return Notify(msg_warning, "File has wrong format. Cannot be opended.");

        if(data["channels"].is_null() || data["channels"] != channels)
            return Notify(msg_warning, "Sequence file has wrong number of channels. Cannot be opended.");

       // Data is ok

        sequence_data = data;
        UpdateSequenceNames();
        LoadChannelMode();
        LinkKeypoints(); // Just in case...
        current_sequence = 0;
    }

    catch(const std::exception& e)
    {
        return Notify(msg_warning, "Sequence file could not be loaded.");
    }
    return true;
}



void        
SequenceRecorder::Save(const std::string &  name)
{   
    if(ends_with(name, ".json"))
        filename = name;
    else    
        filename = name + ".json";
    auto path = directory+"/"+filename;

    LinkKeypoints(); // FIXME: maybe not necessary here
    StoreChannelMode();

    std::ofstream file(path);
    file << std::setw(4) << sequence_data << std::endl;  

    if(file_names.find(filename) == std::string::npos)
        file_names += ","+filename;
}



void
SequenceRecorder::Init()
{
    Bind(channels, "channels");

    SetInputSize("INPUT", channels); // Make sure that there is an input for every channel even if they are not connected

    Bind(positions, channels, "positions", true); // parameter size will be set by the value channels

    int ranges;
    Bind(range_min, &ranges, "range_min");
    if(ranges != channels)
        Notify(msg_fatal_error, "Min range not set for correct number of channels.");


    Bind(range_max, &ranges, "range_max");
    if(ranges != channels)
        Notify(msg_fatal_error, "Max range not set for correct number of channels.");

    int ints;
    Bind(interpolation, &ints, "interpolation");
    if(ints != channels)
        Notify(msg_fatal_error, "Interpolation not set for correct number of channels.");


    Bind(smoothing_time, "smoothing_time");


    Bind(state, states, "state", true);
    Bind(loop, "loop");
    Bind(shuffle,  "shuffle");

    Bind(channel_mode, modes, channels, "channel_mode", true);
    Bind(time_string, "time");
    Bind(end_time_string, "end_time");
    Bind(position, "position");
    Bind(mark_start, "mark_start");
    Bind(mark_end, "mark_end");

    Bind(max_sequences, "max_sequences");
    Bind(sequence_names, "sequence_names");
    Bind(file_names, "file_names");
    Bind(filename, "filename");

    file_names = "";

    Bind(current_sequence, "current_sequence");
    Bind(internal_control, channels, "internal_control", true);

    int do_size = 0;
    Bind(default_output, &do_size, "default_output");
    if(do_size !=channels)
        Notify(msg_fatal_error,"Incorrect size for default_output; does not match number of channels.");
    left_output = copy_array(create_array(channels), default_output, channels);
    right_output = copy_array(create_array(channels), default_output, channels);
    for(int c=0; c<channels; c++)
        if(internal_control[c])
            positions[c] = default_output[c];

    

     directory = GetValue("directory");
     filename = GetValue("filename");
     untitled_count = 1;

    fs::create_directory(directory); // Only works if not a path // FIXME: make recursive later

    io(trig, trig_size, "TRIG");
    trig_last = create_array(trig_size);

    io(playing, "PLAYING");
    io(completed, "COMPLETED");

    io(target, "TARGET");
    io(input, "INPUT");
    io(output, "OUTPUT");
    io(active, "ACTIVE");
    io(smoothing_start,"SMOOTHING_START");

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

    if(!filename.empty() && check_file_exists((directory+"/"+filename).c_str()))
    {
         if(!Open(filename))
            New();
    }
    else
        New();

    // Get files in directory

        std::string fsep = "";
        for(auto& p: fs::directory_iterator(directory)) // was recursive_directory_iterator
    {   auto pp = p.path();
        if(pp.extension() == ".json")
        {
            file_names += fsep + std::string(pp.filename());
            fsep = ",";
        }
    }

    Stop();
}



SequenceRecorder::~SequenceRecorder()
{
// auto save
}



void
SequenceRecorder::Stop()
{
    bool was_recoding = state[2] > 0;
    set_one(state, 0, states);
    timer.Stop();
    timer.Reset();
    if(was_recoding)
        LinkKeypoints(); // at end of recording
}



void
SequenceRecorder::Play()
{
    //if(sequence_data["sequences"][current_sequence]["end_mark_time"] == 0)
    set_one(state, 1, states); 
     timer.Start();
}



void
SequenceRecorder::Record()
{
    StartRecord();
    set_one(state, 2, states); 
}



void
SequenceRecorder::Pause()
{
    bool was_recoding = state[2] > 0;
    set_one(state, 3, states); 
    timer.Stop();
    if(was_recoding)
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
    auto & keypoints = sequence_data["sequences"][current_sequence]["keypoints"];
    int i = find_index_for_time(keypoints, t);
    if(i > 0)
    {
        float kpt = sequence_data["sequences"][current_sequence]["keypoints"][i-1]["time"];
        if(kpt == t && i>1)
            kpt = sequence_data["sequences"][current_sequence]["keypoints"][i-2]["time"];
        timer.SetTime(kpt);
        float end_time = sequence_data["sequences"][current_sequence]["end_time"];
        position = end_time? kpt/end_time : 0; //Fix me: use set time function
    }
}



void
SequenceRecorder::GoToNextKeypoint()
{
    float t = timer.GetTime();
    auto & keypoints = sequence_data["sequences"][current_sequence]["keypoints"];
    int n = keypoints.size();
    int i = find_index_for_time(keypoints, t);
    if(i < n)
    {
        float kpt = sequence_data["sequences"][current_sequence]["keypoints"][i]["time"];
        timer.SetTime(kpt);
        float end_time = sequence_data["sequences"][current_sequence]["end_time"];
        position = end_time? kpt/end_time : 0; //Fix me: use set time function
    }
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
            {
                left_link[c] = i;
                right_output[c] = keypoints[i]["point"][c]; // candidate rightmost output
            }
        for(int c=0; c<channels; c++)
            keypoints[i]["link_left"][c] = left_link[c];
    }

    // right to left sweep

    for(int i=n-1; i>0; i--)
    {
        for(int c=0; c<channels; c++)
            if(!keypoints[i]["point"][c].is_null()) // channel has data from this keypoint
            {
                right_link[c] = i;
                left_output[c] = keypoints[i]["point"][c]; //candidate leftmost output
            }
        for(int c=0; c<channels; c++)
            keypoints[i]["link_right"][c] = right_link[c];
    }
}



void
SequenceRecorder::DeleteEmptyKeypoints()
{
    auto & keypoints = sequence_data["sequences"][current_sequence]["keypoints"];
    auto it = keypoints.begin();
    while(it != keypoints.end()) 
    {
        int e=0;
        for(int c=0; c<channels; c++)
            if(!(*it)["point"][c].is_null())
                e++;

        if(e==0)
        {
            it = keypoints.erase(it);
        } 
        else
        {
            it++;
        }
    }
}



void
SequenceRecorder::StoreChannelMode()
{
    json cm =  json::array();
    for(int c=0; c<channels; c++)
    {
        json modes = json::array();
        for(int m=0; m<4; m++)
            modes.push_back(channel_mode[c][m]);
        cm.push_back(modes);
    }
    sequence_data["channel_mode"] = cm;
}



void
SequenceRecorder::LoadChannelMode()
{
    try
    {
        for(int c=0; c<channels; c++)
            for(int m=0; m<4; m++)
                channel_mode[c][m] = sequence_data["channel_mode"][c][m];
    }
    catch(const std::exception& e)
    {
        return; // ignore error
    }
}



void
SequenceRecorder::AddKeypoint(float time)
{   

    auto & keypoints = sequence_data["sequences"][current_sequence]["keypoints"];
    int n = keypoints.size();

    float qtime = quantize(time, GetTickLength());
    ///printf(">>> add: %f => %f\n", time, qtime);

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

    int i = find_index_for_time(keypoints, qtime);
    if(n>0 && i<n)
    {
        // Find position in list
        int i = find_index_for_time(keypoints, qtime);

        if(i==0) // insert first
        {
            json keypoint;
            keypoint["time"] = qtime;
            keypoint["point"] = points;
            keypoints.insert(keypoints.begin(), keypoint);
            ///printf("INSERT FIRST\n");
            return;
        }

        float t = keypoints[i-1]["time"];
        if(qtime == t) // merge
        {
            for(int c=0; c<channels; c++)
            if(!points[c].is_null())
                keypoints[i-1]["point"][c] = points[c];
            ///printf("MERGED AT %d\n", i-1);
            return;
        }
        else if(qtime < keypoints[i]["time"]) // insert before
        {
            json keypoint;
            keypoint["time"] = qtime;
            keypoint["point"] = points;
            keypoints.insert(keypoints.begin() + i, keypoint);
            ///printf("INSERT BEFORE %d\n", i);
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
    sequence_data["sequences"][current_sequence]["keypoints"] = json::array();
    sequence_data["sequences"][current_sequence]["start_time"] = 0;
    sequence_data["sequences"][current_sequence]["start_mark_time"] = 0;
    sequence_data["sequences"][current_sequence]["end_mark_time"] = 1000;
    sequence_data["sequences"][current_sequence]["end_time"] = 1000;
}



void
SequenceRecorder::Crop()
{
    auto & keypoints = sequence_data["sequences"][current_sequence]["keypoints"];
    int n = keypoints.size();

    if(n<1)
        return;
    float start_mark_time = sequence_data["sequences"][current_sequence]["start_mark_time"] ;
    float end_mark_time = sequence_data["sequences"][current_sequence]["end_mark_time"] ;


    for(int i=0; i<n; i++)
        if(float(keypoints[i]["time"]) < start_mark_time || float(keypoints[i]["time"])>end_mark_time)
            ClearKeypointAtIndex(i, true);

    DeleteEmptyKeypoints();

    // Retime keypoints

    float start_time = keypoints[0]["time"];
    n = keypoints.size();
    for(int i=0; i<n; i++)
        keypoints[i]["time"] = float(keypoints[i]["time"])-start_time;

    sequence_data["sequences"][current_sequence]["start_mark_time"] = 0;
    sequence_data["sequences"][current_sequence]["end_mark_time"] = float(sequence_data["sequences"][current_sequence]["end_mark_time"]) - start_time;

    LinkKeypoints();
}



void
SequenceRecorder::DeleteKeypoint(float time)
{
    auto & keypoints = sequence_data["sequences"][current_sequence]["keypoints"];
    int n = keypoints.size();
    int i = max(0, find_index_for_time(keypoints, time)-1);
    
    float t = keypoints[i]["time"];
    if(abs(t-time) < GetTickLength()/2)
        ClearKeypointAtIndex(i);
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
        }
    }

    // Clean up

    DeleteEmptyKeypoints();
    LinkKeypoints();
}



void
SequenceRecorder::ClearKeypointAtIndex(int i, bool all)
{
    //(">>> delete: %d\n", i);
    auto & keypoints = sequence_data["sequences"][current_sequence]["keypoints"];
    int n = keypoints.size();
    if(i <0 || i>=n)
        return;

    for(int c=0; c<channels; c++)
    {
        if(channel_mode[c][2] == 1 || all) // record mode or all-flag set
            keypoints[i]["point"][c] = nullptr;
    }
}



void
SequenceRecorder::DeleteKeypointsInRange(float t0, float t1) // FIXME: Needs further testing for different quantizations ***
{
    auto & keypoints = sequence_data["sequences"][current_sequence]["keypoints"];
    int n = keypoints.size();

    /*
    for(int i=0; i<n; i++)
    {
        float t = float(keypoints[i]["time"]);
        if(t0 < t && t<t1)
        {
            ClearKeypointAtIndex(i);
            printf("--- Deleting: %d", i);
        }
    }
    */

    int i0 = find_index_for_time(keypoints, t0);
    int i1 = find_index_for_time(keypoints, t1);
//    printf("%d - %d\n\n", i0, i1);
    for(int i = i0; i< i1; i++)
        ClearKeypointAtIndex(i);
        // printf("--- Deleting: %d", i);
}



void
SequenceRecorder::Trig(int id)
{
    int m = sequence_data["sequences"].size();
    if(id<0 || id>=m)
    {
        Notify(msg_warning, "Sequence %d does not exist", id);
        return;
    }
    Stop();
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
    {
        AddKeypoint(timer.GetTime());
        LinkKeypoints();
    }
    else if(s == "delete_keypoint")
    {
        DeleteKeypoint(timer.GetTime());
        DeleteEmptyKeypoints();
        LinkKeypoints();
        // Cleanup
    }
    else if(s == "crop")
            Crop();
    else if(s=="clear")
            ClearSequence();
    else if(s=="delete")
        DeleteKeypoints();
    else if(s=="lock")
        LockChannel(y);
    else if(s=="trig")
        Trig(8*y+x);
    else if(s=="rename")
        Rename(value);

    else if(s=="new")
        New();
    else if(s=="open")
        Open(value);
    else if(s =="save")
        Save(filename);
    else if(s=="saveas")
        Save(value);
}



std::string
SequenceRecorder::GetJSONData(const std::string & name, const std::string & tab)
{
    if(name=="RANGES")
        return sequence_data["ranges"].dump();

    else if(name=="SEQUENCE")
    {
        int n = sequence_data["sequences"][current_sequence]["keypoints"].size();
        if(n>400) // 400 = 10 s
        {
            return "{}"; //Minimal JSON
        }
        std::string sq = sequence_data["sequences"][current_sequence].dump();
        return sq;
    }

    else
        return "";
}



void
SequenceRecorder::Tick()
{
    long tl = GetTickLength();
    reset_array(playing, max_sequences);
    reset_array(completed, max_sequences);

    // Check trig input

    for(int s=0; s<trig_size; s++)
        if(trig[s] > 0 && trig_last[s] == 0) // Trig on rising edge
            Trig(s);
        copy_array(trig_last, trig, trig_size);

    float t = timer.GetTime();


    if(start_record) // timer start at tick to increase probability of overlapping keypoint when starting at a keypoint
    {                // FIXME: May want to jump to closest keypoint if dense recording is used
        timer.Start();
        start_record = false;
    }



    // Set initial position if not set already - this is used as output when no data is available

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

    // Set inputs from parameters for internal channels

    for(int c=0; c<channels; c++)
        if(internal_control[c])
            input[c] = positions[c];

    if(state[1]) // handle play mode
    {
        set_one(playing, current_sequence, max_sequences);
        if(loop && t >= float(sequence_data["sequences"][current_sequence]["end_mark_time"])) // loop
        {
            timer.SetTime(float(sequence_data["sequences"][current_sequence]["start_mark_time"]));
        }
        else if(position >= 1 || end_time == 0)
        {   
            timer.SetTime(sequence_data["sequences"][current_sequence]["end_time"]);
            set_one(completed, current_sequence, max_sequences);
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
        DeleteKeypointsInRange(quantize(last_record_position, tl), quantize(t, tl));
        last_record_position = t;
        AddKeypoint(t);
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

    // Set positions parameter for externally controlled channels

    for(int c=0; c<channels; c++)
            if(internal_control[c] == 0)
                positions[c] = output[c];
}


static InitClass init("SequenceRecorder", &SequenceRecorder::Create, "Source/Modules/RobotModules/SequenceRecorder/");

