//
//	SequenceRecorder.cc		This file is a part of the IKAROS project
//
//    Copyright (C) 2015-2025 Christian Balkenius
//
#include "ikaros.h"

#include <iostream>
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;

using namespace ikaros;


/*
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
*/

 
static std::string 
make_timestamp(float t)
{
    // Validate input
    if (t < 0) {
        return "00:00:000";
    }

    // Calculate time components with bounds checking
    const int t_ms = std::max(0, std::min(999, int(t) % 1000));
    const int t_s = std::max(0, std::min(59, (int(t)/1000) % 60));
    const int t_min = std::max(0, std::min(99, int(t/60000)));

    // Use safe string formatting with sufficient buffer
    constexpr size_t BUFF_SIZE = 12; // "MM:SS:mmm\0" needs 11 chars
    char buff[BUFF_SIZE];
    
    // Format with bounds checking
    const int written = snprintf(buff, BUFF_SIZE, "%02d:%02d:%03d", t_min, t_s, t_ms);
    
    // Verify formatting succeeded
    if (written < 0 || written >= BUFF_SIZE) {
        return "ERROR";
    }

    return std::string(buff);
}


static inline float
quantize(double time, long q)
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
find_index_for_time(list keypoints, float t)
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


class SequenceRecorder: public Module
{
public:
    //void 		Init();
    //void 		Tick();

    // Commands
    /*
    void        Stop();
    void        Play();
    void        Record();
    void        Pause();
    void        SkipStart();
    void        SkipEnd();
    void        SetStartMark();
    void        SetEndMark();
    void        GoToPreviousKeypoint();
    void        GoToNextKeypoint();
    void        GoToTime(double time);   // set up current target and left and right interpolation sources
    void        Trig(int id);

    void        ExtendTime();
    void        ReduceTime();
    void        LockChannel(int c);

    void        LinkKeypoints();
    void        DeleteEmptyKeypoints();

    void        StoreChannelMode();
    void        LoadChannelMode();

    void        AddKeypoint(double time); // add keypoint at time t
    void        DeleteKeypoint(double time); // delete a single keypoint at time t (or close to it)


    void        Crop(); // Remove points outside the selected area, and let the sequence start at t=0

    void        ClearSequence();    // clear currently selected sequence
    void        ClearKeypointAtIndex(int i, bool all=false); // nortmally only clears point in record mode, set all to true to clear all
    void        DeleteKeypoints(); // Delete all points within the selection time window for channels in record mode   
    void        DeleteKeypointsInRange(float t0, float t1);

    void        New(); // Create new sequence
    bool        Open(const std::string & name); // Open sequence file
    void        Save(const std::string & name); // Sabe sequence file under supplied name

    void        Command(std::string s, float x, float y, std::string value);

    std::string GetJSONData(const std::string & name, const std::string & tab);

    void        SetTargetForTime(float t);
    void        SetOutputForChannel(int c); 
    void        StartRecord();

    void        UpdateSequenceNames();
    void        Rename(const std::string & new_name); // Renames the selected sequence
    */


    void
    StartRecord()
    {
        last_record_position = position;
        start_record = true;
    }


    void
    Stop()
    {
        bool was_recoding = state[2] > 0;
        set_one(state, 0, states);
        //timer.Stop();
        timer.Restart();    // FIXME: Is this scorrect?
        if(was_recoding)
            LinkKeypoints(); // at end of recording
    }


    void
    Play()
    {
        //if(sequence_data["sequences"][current_sequence.as_int()]["end_mark_time"] == 0)
        set_one(state, 1, states); 
        timer.Continue();
    }



    void
    Record()
    {
        StartRecord();
        set_one(state, 2, states); 
    }



    void
    Pause()
    {
        bool was_recoding = state[2] > 0;
        set_one(state, 3, states); 
        timer.Pause(); // FIXME: Possible error
        if(was_recoding)
            LinkKeypoints(); // at end of recording
    }



    void
    SkipStart()
    {
        timer.Pause(); // FIXME: Possible error
        set_one(state, 3, states);// Pause
        if((1000*timer.GetTime()) <= sequence_data["sequences"][current_sequence.as_int()]["start_mark_time"])
            timer.SetTime(sequence_data["sequences"][current_sequence.as_int()]["start_time"]/1000.0);
        else if((1000*timer.GetTime()) <= sequence_data["sequences"][current_sequence.as_int()]["end_mark_time"])
            timer.SetTime(sequence_data["sequences"][current_sequence.as_int()]["start_mark_time"]/1000.0);
        else
            timer.SetTime(sequence_data["sequences"][current_sequence.as_int()]["end_mark_time"]/1000.0);
    }



    void
    SkipEnd()
    {
        timer.Pause();
        set_one(state, 3, states);
        if((1000*timer.GetTime()) >= sequence_data["sequences"][current_sequence.as_int()]["end_mark_time"])
            timer.SetTime(sequence_data["sequences"][current_sequence.as_int()]["end_time"]/1000.0);
        else if((1000*timer.GetTime()) >= sequence_data["sequences"][current_sequence.as_int()]["start_mark_time"])
            timer.SetTime(sequence_data["sequences"][current_sequence.as_int()]["end_mark_time"]/1000.0);
        else
            timer.SetTime(sequence_data["sequences"][current_sequence.as_int()]["start_mark_time"]/1000.0);
    }



    void
    SetStartMark()
    {
        sequence_data["sequences"][current_sequence.as_int()]["start_mark_time"] = (1000*timer.GetTime());
    }



    void
    SetEndMark()
    {
        sequence_data["sequences"][current_sequence.as_int()]["end_mark_time"] = (1000*timer.GetTime());

    }



    void
    GoToPreviousKeypoint()
    {
        float t = (1000*timer.GetTime());
        auto & keypoints = sequence_data["sequences"][current_sequence.as_int()]["keypoints"];
        int i = find_index_for_time(keypoints, t);
        if(i > 0)
        {
            float kpt = sequence_data["sequences"][current_sequence.as_int()]["keypoints"][i-1]["time"];
            if(kpt == t && i>1)
                kpt = sequence_data["sequences"][current_sequence.as_int()]["keypoints"][i-2]["time"];
            timer.SetTime(kpt);
            float end_time = sequence_data["sequences"][current_sequence.as_int()]["end_time"];
            position = end_time? kpt/end_time : 0; //Fix me: use set time function
        }
    }



    void
    GoToNextKeypoint()
    {
        float t = (1000*timer.GetTime());
        auto & keypoints = sequence_data["sequences"][current_sequence.as_int()]["keypoints"];
        int n = keypoints.size();
        int i = find_index_for_time(keypoints, t);
        if(i < n)
        {
            float kpt = sequence_data["sequences"][current_sequence.as_int()]["keypoints"][i]["time"];
            timer.SetTime(kpt/1000.0);
            float end_time = sequence_data["sequences"][current_sequence.as_int()]["end_time"];
            position = end_time? kpt/end_time : 0; //Fix me: use set time function
        }
    }



    void
    GoToTime(double time)
    {
        SetTargetForTime(time);

    }



    void
    ExtendTime() // add one second to the end of the sequence
    {
        float end_time = sequence_data["sequences"][current_sequence.as_int()]["end_time"];
        sequence_data["sequences"][current_sequence.as_int()]["end_time"] = 1000.0f+1000*int(0.001*end_time);
    }



    void
    ReduceTime()
    {
        float end_time = sequence_data["sequences"][current_sequence.as_int()]["end_time"];
        end_time = -1000.0f+1000*int(0.001*end_time+0.99999);
        sequence_data["sequences"][current_sequence.as_int()]["end_time"] = end_time > 0 ? end_time: 0;
    }



    void
    LockChannel(int c)
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
    LinkKeypoints()
    {
        auto & keypoints = sequence_data["sequences"][current_sequence.as_int()]["keypoints"];
        int n = keypoints.size();

        std::vector<int> left_link(channels.as_int(), -1);
        std::vector<int> right_link(channels.as_int(), -1);

        // left to right sweep

        for(int i=0; i<n; i++)
        {
            for(int c=0; c<channels.as_int(); c++)
                if(!keypoints[i]["point"][c].is_null()) // channel has data from this keypoint
                {
                    left_link[c] = i;
                    right_output[c] = keypoints[i]["point"][c].as_float(); // candidate rightmost output
                }
            for(int c=0; c<channels.as_int(); c++)
                keypoints[i]["link_left"][c] = left_link[c];
        }

        // right to left sweep

        for(int i=n-1; i>0; i--)
        {
            for(int c=0; c<channels.as_int(); c++)
                if(!keypoints[i]["point"][c].is_null()) // channel has data from this keypoint
                {
                    right_link[c] = i;
                    left_output[c] = keypoints[i]["point"][c].as_float(); //candidate leftmost output
                }
            for(int c=0; c<channels.as_int(); c++)
                keypoints[i]["link_right"][c] = right_link[c];
        }
    }


    void
    DeleteEmptyKeypoints()
    {
        list keypoints = sequence_data["sequences"][current_sequence.as_int()]["keypoints"];
        auto it = keypoints.begin();
        while(it != keypoints.end()) 
        {
            int e=0;
            for(int c=0; c<channels.as_int(); c++)
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
    StoreChannelMode()
    {
        list cm =  list();
        for(int c=0; c<channels.as_int(); c++)
        {
            list modes = list();
            for(int m=0; m<4; m++)
                modes.push_back(channel_mode(c,m));
            cm.push_back(modes);
        }
        sequence_data["channel_mode"] = cm;
    }


    void
    LoadChannelMode()
    {
        try
        {
            for(int c=0; c<channels.as_int(); c++)
                for(int m=0; m<4; m++)
                    channel_mode(c,m) = sequence_data["channel_mode"][c][m];
        }
        catch(const std::exception& e)
        {
            return; // ignore error
        }
    }


    void
    AddKeypoint(double time)
    {   
        list keypoints = sequence_data["sequences"][current_sequence.as_int()]["keypoints"];
        int n = keypoints.size();

        float qtime = quantize(time, 1000*GetTickDuration()); // Scaled to ms; FIXME: use seconds and nominal time if possible later
       // printf(">>> add: %f => %f\n", time, qtime);

        // Create the point data array

        list points = list();
        for(int c=0; c<channels.as_int(); c++)
            if(channel_mode[c](0) == 1) //locked
                points.push_back(null()); // Do not record locked channel???
            else if(channel_mode(c,1) == 1) //play - use null to indicate nodata
                points.push_back(null()); // was target[c]
            else if(channel_mode(c,2) == 1) //record - store current input (or sliders)
                points.push_back(input(c));
            else if(channel_mode(c,3) == 1) //copy - do not record this channel but use null
                points.push_back(null());
            else // default
                points.push_back(null());

        int i = find_index_for_time(keypoints, qtime);
        if(n>0 && i<n)
        {
            // Find position in list
            int i = find_index_for_time(keypoints, qtime);

            if(i==0) // insert first
            {
                dictionary keypoint;
                keypoint["time"] = qtime;
                keypoint["point"] = points;
                keypoints.insert(keypoints.begin(), keypoint);
                ///printf("INSERT FIRST\n");
                return;
            }

            float t = keypoints[i-1]["time"];
            if(qtime == t) // merge
            {
                for(int c=0; c<channels.as_int(); c++)
                if(!points[c].is_null())
                    keypoints[i-1]["point"][c] = points[c];
                ///printf("MERGED AT %d\n", i-1);
                return;
            }
            else if(qtime < keypoints[i]["time"]) // insert before
            {
                dictionary keypoint;
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
                for(int c=0; c<channels.as_int(); c++)
                if(!points[c].is_null())
                    keypoints[n-1]["point"][c] = points[c];
                //printf("MERGED AT LAST\n");
                return;
        }

        // insert last - also for first keypoint to be inserted

        dictionary keypoint;
        keypoint["time"] = qtime;
        keypoint["point"] = points;
        keypoints.push_back(keypoint);
                //printf("INSERT LAST\n");
    }


    void
    ClearSequence()
    {
        sequence_data["sequences"][current_sequence.as_int()]["keypoints"] = list();
        sequence_data["sequences"][current_sequence.as_int()]["start_time"] = 0;
        sequence_data["sequences"][current_sequence.as_int()]["start_mark_time"] = 0;
        sequence_data["sequences"][current_sequence.as_int()]["end_mark_time"] = 1000;
        sequence_data["sequences"][current_sequence.as_int()]["end_time"] = 1000;
    }


    void
    Crop()
    {
        auto & keypoints = sequence_data["sequences"][current_sequence.as_int()]["keypoints"];
        int n = keypoints.size();

        if(n<1)
            return;
        float start_mark_time = sequence_data["sequences"][current_sequence.as_int()]["start_mark_time"] ;
        float end_mark_time = sequence_data["sequences"][current_sequence.as_int()]["end_mark_time"] ;


        for(int i=0; i<n; i++)
            if(float(keypoints[i]["time"]) < start_mark_time || float(keypoints[i]["time"])>end_mark_time)
                ClearKeypointAtIndex(i, true);

        DeleteEmptyKeypoints();

        // Retime keypoints

        float start_time = keypoints[0]["time"];
        n = keypoints.size();
        for(int i=0; i<n; i++)
            keypoints[i]["time"] = float(keypoints[i]["time"])-start_time;

        sequence_data["sequences"][current_sequence.as_int()]["start_mark_time"] = 0;
        sequence_data["sequences"][current_sequence.as_int()]["end_mark_time"] = float(sequence_data["sequences"][current_sequence.as_int()]["end_mark_time"]) - start_time;

        LinkKeypoints();
    }


    void
    DeleteKeypoint(double time)
    {
        auto & keypoints = sequence_data["sequences"][current_sequence.as_int()]["keypoints"];
        int n = keypoints.size();
        int i = max(0, find_index_for_time(keypoints, time)-1);
        
        float t = keypoints[i]["time"];
        if(abs(t-time) < GetTickDuration()/2)
            ClearKeypointAtIndex(i);
    }


    void
    DeleteKeypoints()
    {
        float start_mark_time = float(sequence_data["sequences"][current_sequence.as_int()]["start_mark_time"]);
        float end_mark_time = float(sequence_data["sequences"][current_sequence.as_int()]["end_mark_time"]);
        int n = sequence_data["sequences"][current_sequence.as_int()]["keypoints"].size();
        for(int i=0; i<n; i++)
        {
            float t = sequence_data["sequences"][current_sequence.as_int()]["keypoints"][i]["time"];
            if(start_mark_time <= t && t<=end_mark_time)
            {
                int number_of_deleted_points = 0;
                for(int c=0; c<channels.as_int(); c++)
                {
                    if(channel_mode(c,2) == 1) // record mode
                    {
                        sequence_data["sequences"][current_sequence.as_int()]["keypoints"][i]["point"][c] = null();
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
    ClearKeypointAtIndex(int i, bool all=false)
    {
        //(">>> delete: %d\n", i);
        auto & keypoints = sequence_data["sequences"][current_sequence.as_int()]["keypoints"];
        int n = keypoints.size();
        if(i <0 || i>=n)
            return;

        for(int c=0; c<channels.as_int(); c++)
        {
            if(channel_mode(c,2) == 1 || all) // record mode or all-flag set
                keypoints[i]["point"][c] = null();
        }
    }


    void
    DeleteKeypointsInRange(float t0, float t1) // FIXME: Needs further testing for different quantizations ***
    {
        auto & keypoints = sequence_data["sequences"][current_sequence.as_int()]["keypoints"];
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
    Trig(int id)
    {
        int m = sequence_data["sequences"].size();
        if(id<0 || id>=m)
        {
            Notify(msg_warning, "Sequence %d does not exist"+std::to_string(id));
            return;
        }
        Stop();
        current_sequence = id;
        Play();
    }



    void
   // Command(std::string s, float x, float y, std::string value)
    Command(std::string command_name, dictionary & parameters)
    {
        std::string s = command_name;
        std::string value = parameters["value"];

        float x = 0;
        float y = 0;

        if(parameters.contains("x"))
            x = parameters["x"];

        if(parameters.contains("y"))
            y = parameters["y"];

        std::cout << "COMMAND: " << s << std::endl;
        
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
            AddKeypoint((1000*timer.GetTime()));
            LinkKeypoints();
        }
        else if(s == "delete_keypoint")
        {
            DeleteKeypoint((1000*timer.GetTime()));
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

    std::string json(const std::string & name)
    {
        if(name=="RANGES")
            return sequence_data["ranges"].json();

        else if(name=="SEQUENCE")
        {
            int n = sequence_data["sequences"][current_sequence.as_int()]["keypoints"].size();
            if(n>400) // 400 = 10 s
            {
                return "{}"; //Minimal JSON
            }
            std::string sq = sequence_data["sequences"][current_sequence.as_int()].json();
            return sq;
        }

        else
            return "";
    }


    void
    SetTargetForTime(float t)
    {
    list keypoints = sequence_data["sequences"][current_sequence.as_int()]["keypoints"];
    int n = keypoints.size();
    int i = find_index_for_time(keypoints, t);

    // Check if no keypoints: use default output as target

    if(n==0)
    {
        for(int c=0; c<channels.as_int(); c++)
            target[c] = default_output[c];
        return;
    }

    // Check if index is zero OR there is only one keypoint: use first keypoint

    if(i == 0 || n==1)
    {
        for(int c=0; c<channels.as_int(); c++)
        if(keypoints[0]["point"][c].is_null())
            target[c] = left_output[c];
        else
            target[c] = keypoints[0]["point"][c].as_float();

        return;
    }

    // Check if we are at or after the last keypoint: use last keypoint

    if(i > n-1)
    {
        for(int c=0; c<channels.as_int(); c++)
        if(keypoints[n-1]["point"][c].is_null())
            target[c] = right_output[c];
        else
            target[c] = keypoints[n-1]["point"][c].as_float();

        return;
    }

    // Do normal interpolation

    for(int c=0; c<channels.as_int(); c++)
    {

        // Process left point

        auto & kp_left = keypoints[i-1];
        double time_left = keypoints[i-1]["time"];
        double point_left = left_output[c];


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
        double time_right = keypoints[i]["time"];
        double point_right = right_output[c];


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
    SetOutputForChannel(int c)
    {
    if(channel_mode(c,0) == 1) //locked
    {
        // Do not change output
        active[c] = 1;
    }

    else if(channel_mode(c,1) == 1) //play
        {
        output[c] = target[c]; // SMOOTH HERE AS WELL
        positions[c] = target[c];
        active[c] = 1;
        }   

    else if(channel_mode(c,2) == 1) //record
        {
        output[c] = input[c];
        active[c] = 0;
            if(internal_control[c] == 1)
            active[c] = 1;
        }   

    else if(channel_mode(c,3) == 1) //copy
        {
            output[c] = input[c];
            active[c] = 0;
            if(internal_control[c] == 1)
            active[c] = 1;
        }
    }


    void
    Rename(const std::string & new_name)
    {
    sequence_data["sequences"][current_sequence.as_int()]["name"] = new_name;
    UpdateSequenceNames();
    }


    void
    UpdateSequenceNames()
    {
        sequence_names = "";
        std::string sep = "";

        for(auto s : sequence_data["sequences"])
        {
            std::string name = s["name"];
            sequence_names = std::string(sequence_names) + sep + name;
            sep = ",";
        }
    }


    static dictionary create_sequence(int index)
    {
    dictionary sq;

    sq["name"] = "Sequence "+std::string(1,65+index / 8)+std::string(1,49+index % 8);
    sq["start_time"] = 0;
    sq["start_mark_time"] = 0;
    sq["end_mark_time"] = 1000;
    sq["end_time"] = 1000;
    sq["keypoints"] = list();

    return sq;
    }


    void
    New()
    {
    filename = "untitled"+std::to_string(untitled_count++)+".json";
    sequence_data = dictionary(); // in case it is not empty
    sequence_data["type"] = "Ikaros Sequence Data";
    sequence_data["channels"] = channels.as_int();
    sequence_data["ranges"] = list();
    for(int c=0; c<channels.as_int(); c++)
    {
        list range = list();
        range.push_back(range_min(c));
        range.push_back(range_max(c));
        sequence_data["ranges"].push_back(range);
    }
    sequence_data["sequences"] = list();
    for(int i=0; i<max_sequences; i++)
    {
            sequence_data["sequences"].push_back(create_sequence(i)); 
    }

    UpdateSequenceNames();
    }


    bool
    Open(const std::string & name)
    {
        if(std::string(filename).empty())
        return false;

    filename = name;
    auto path = std::string(directory)+"/"+std::string(filename);

    if(!check_file_exists(path.c_str())) // Remove call t check_file_exists(path.c_str())
    {
        Notify(msg_warning, "File does not exist."); // FIXME: path.c_str()
        return false;
    }

    try
    {
        dictionary data;
        data.load_json(path);

        // Validate

        if(data["type"].is_null() || data["type"].as_string() != u8"Ikaros Sequence Data")
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
    Save(const std::string &  name)
    {   
        if(ends_with(name, ".json"))
            filename = name;
        else    
            filename = name + ".json";
        auto path = std::string(directory)+"/"+std::string(filename);

        LinkKeypoints(); // FIXME: maybe not necessary here
        StoreChannelMode();

        std::ofstream file(path);

        if (!file.is_open())
        {
            Notify(msg_warning, "Could not open file for writing: " + path);
            return;
        }

    file << sequence_data.json() << std::endl;  

    if(std::string(file_names).find(std::string(filename)) == std::string::npos) // ***************** CONTAINS
        file_names = std::string(file_names) + ","+std::string(filename);
    }


    void
    Init()
    {
    Bind(channels, "channels");

    // SetInputSize("INPUT", channels); // Make sure that there is an input for every channel even if they are not connected ********* FIX ME ************

    Bind(positions, "positions"); // parameter size will be set by the value channels


    Bind(range_min, "range_min");
    if(range_min.size_x() != channels)
        Notify(msg_fatal_error, "Min range not set for correct number of channels.");


    Bind(range_max, "range_max");
    if(range_max.size_x() != channels)
        Notify(msg_fatal_error, "Max range not set for correct number of channels.");

    Bind(interpolation, "interpolation");
    if(interpolation.size_x() != channels)
        Notify(msg_fatal_error, "Interpolation not set for correct number of channels.");


    Bind(smoothing_time, "smoothing_time");


    Bind(state, "state");
    Bind(loop, "loop");
    Bind(shuffle, "shuffle");

    Bind(channel_mode, "channel_mode");
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
    Bind(internal_control, "internal_control");

    Bind(default_output, "default_output");
    if(default_output.size_x() !=channels)
        Notify(msg_fatal_error,"Incorrect size for default_output; does not match number of channels.");

    left_output.copy(default_output);
    right_output.copy(default_output);

    for(int c=0; c<channels.as_int(); c++) // FIXME: Rremove as int where not necessary
        if(internal_control(c))
            positions(c) = default_output(c);

        Bind(directory, "directory");
        Bind(filename, "filename");
        untitled_count = 1;

    fs::create_directory(std::string(directory)); // Only works if not a path // FIXME: make recursive later

    Bind(trig, "TRIG");
    //trig_last = matrix(trig.size()); // Set in first assignment

    Bind(playing, "PLAYING");
    Bind(completed, "COMPLETED");

    Bind(target, "TARGET");
    Bind(input, "INPUT");
    Bind(output, "OUTPUT");
    Bind(active, "ACTIVE");
    Bind(smoothing_start,"SMOOTHING_START");


    /* TEMPORARY
    for(int c=0; c<channels.as_int(); c++)
    {
        //set_one_row(channel_mode, 3, c, 4, channels); //  ************ FIXME: default size for matrix parameter **************
        channel_mode[c].reset();
        channel_mode[c](3) = 1;
    }
    */
    /*
    left_time = matrix(channels.as_int());
    right_time = matrix(channels.as_int());
    left_position = matrix(channels.as_int());
    right_position = matrix(channels.as_int());
    */
    left_index = matrix(channels.as_int());
    right_index = matrix(channels.as_int());

    for(int c=0; c<channels.as_int(); c++)
    {   
        left_index[c] = 0;
        right_index[c] = INT_MAX;
    }

    if(!std::string(filename).empty() && check_file_exists((std::string(directory)+"/"+std::string(filename)).c_str())) // TODO: Remove call to check_file_exists
    {
            if(!Open(std::string(filename)))
            New();
    }
    else
        New();

    // Get files in directory

    std::string fsep = "";
    for(auto& p: fs::directory_iterator(std::string(directory))) // was recursive_directory_iterator
{   auto pp = p.path();
    if(pp.extension() == ".json")
        {
            file_names = std::string(file_names) + fsep + std::string(pp.filename());
            fsep = ",";
        }
    }

    Stop();
    }


    /*
    ~SequenceRecorder()
    {
    // auto save
    }
    */


    void
    Tick()
    {
        long tl = GetTickDuration();
        playing.reset();
        completed.reset();

        // Check trig input

        for(int s=0; s<trig.size(); s++)
            if(trig[s] > 0 && trig_last[s] == 0) // Trig on rising edge
                Trig(s);

        trig_last.copy(trig);
        float t = (1000*timer.GetTime());

        if(start_record) // timer start at tick to increase probability of overlapping keypoint when starting at a keypoint
        {                // FIXME: May want to jump to closest keypoint if dense recording is used
            timer.Continue();
            start_record = false;
        }

    //     // Set initial position if not set already - this is used as output when no data is available

    //     // Check if position has been changed from WebUI - should use command in the future

         if(position != last_position) 
         {
                 Pause();
                 float end_time = sequence_data["sequences"][current_sequence.as_int()]["end_time"];
                 timer.SetTime(position*end_time/1000.0);
                last_position = position;
        }

    //     // Set position

         float end_time = sequence_data["sequences"][current_sequence.as_int()]["end_time"];
         position = end_time? t/end_time : 0;
         last_position = position;

    //     // Set inputs from parameters for internal channels

         for(int c=0; c<channels.as_int(); c++)
             if(internal_control[c])
                 input[c] = positions[c];

         if(state[1]) // handle play mode
         {
             set_one(playing, current_sequence, max_sequences);
             if(loop && t >= float(sequence_data["sequences"][current_sequence.as_int()]["end_mark_time"])) // loop
             {
                 timer.SetTime(float(sequence_data["sequences"][current_sequence.as_int()]["start_mark_time"]));
             }
             else if(position >= 1 || end_time == 0)
             {   
                 timer.SetTime(sequence_data["sequences"][current_sequence.as_int()]["end_time"]/1000.0);
                 set_one(completed, current_sequence, max_sequences);
                 Pause();
         }   }

         else if(state[2]) // handle record mode
         {
             if(position >= 1 || end_time == 0) // extend recoding if at end
                 sequence_data["sequences"][current_sequence.as_int()]["end_time"] = t;
         }

         // Set outputs

         GoToTime(t);

    //     // FIXME: Add smoothing here

         for(int c=0; c<channels.as_int(); c++)
             SetOutputForChannel(c);

    //     // AddPoints if in recording mode

         if(state[2] == 1) // record mode
         {
             DeleteKeypointsInRange(quantize(last_record_position, tl), quantize(t, tl));
             last_record_position = t;
             AddKeypoint(t);
         }

    // Set position again

         end_time = sequence_data["sequences"][current_sequence.as_int()]["end_time"];
         position = end_time? t/end_time : 0;
         last_position = position;

         time_string = make_timestamp(t); // timer.GetTime()
         end_time_string  = make_timestamp(sequence_data["sequences"][current_sequence.as_int()]["end_time"]);

         if(float(end_time = sequence_data["sequences"][current_sequence.as_int()]["end_time"]) != 0)
         {
             mark_start = float(sequence_data["sequences"][current_sequence.as_int()]["start_mark_time"])/float(end_time = sequence_data["sequences"][current_sequence.as_int()]["end_time"]);
             mark_end = float(sequence_data["sequences"][current_sequence.as_int()]["end_mark_time"])/float(end_time = sequence_data["sequences"][current_sequence.as_int()]["end_time"]);}

    //     // Set positions parameter for externally controlled channels

         for(int c=0; c<channels.as_int(); c++)
                 if(internal_control[c] == 0)
                     positions[c] = output[c];
    }

    // Current state

    parameter       channels;
        parameter   max_sequences;

    matrix          range_min;
    matrix          range_max;
    matrix          interpolation;

    matrix          trig;
    matrix          trig_last;
    //int             trig_size;  // REMOVE ALL SIZE VARIABLES *********************

    matrix          playing;
    matrix          completed;

    matrix          positions;

    parameter       smoothing_time;
    matrix          smoothing_start;

    matrix       target;
    matrix       input;
    matrix       default_output; // value for initial from ikg file if set
    matrix       left_output;    // Value to use to the left of first keypoint
    matrix       right_output;   // value to use to the right of the last keypoint
    matrix       internal_control;

    matrix          output;
    matrix          active;

    bool            start_record;
    parameter       current_sequence;
    parameter       sequence_names;
    parameter       file_names;
    dictionary      sequence_data;

    // Control  variables

    int         states = 8;
    int         modes = 4;

    matrix      state; // state of the head controller buttons
    matrix      channel_mode;
    parameter   loop;
    parameter   shuffle;

    /*
    matrix      left_time;
    matrix      right_time;
    matrix      left_position;
    matrix      right_position;
    */
    matrix       left_index;
    matrix       right_index;

    float       last_time;
    float       last_index;

    Timer       timer;
    parameter   position;
    float       last_record_position;
    float       last_position; // to see if the value has been changed by WebUI
    parameter   mark_start;
    parameter   mark_end;

    parameter   directory;
    parameter   filename;

    int         untitled_count;
    parameter   time_string;
    parameter   end_time_string;

    void        SetOutputForTime(float t); // time in ms

   //  int smoothing_time; // for torque and position
};

INSTALL_CLASS(SequenceRecorder)
