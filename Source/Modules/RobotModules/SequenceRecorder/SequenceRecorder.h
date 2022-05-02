//
//	SequenceRecorder.h		This file is a part of the IKAROS project
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

// JSON library from https://github.com/nlohmann/json


#ifndef SequenceRecorder_
#define SequenceRecorder_


#include "IKAROS.h"
#include "json.hpp"

using json = nlohmann::json; 


class SequenceRecorder: public Module
{
public:
    static Module * Create(Parameter * p) { return new SequenceRecorder(p); }

    SequenceRecorder(Parameter * p) : Module(p) {}
    virtual ~SequenceRecorder();

    void 		Init();
    void 		Tick();

    // Commands

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
    void        GoToTime(float time);   // set up current target and left and right interpolation sources
    void        Trig(int id);

    void        ExtendTime();
    void        ReduceTime();
    void        LockChannel(int c);

    void        LinkKeypoints();
    void        AddKeypoint(float time); // add keypoint at time t
    void        PushKeypoint(); // push keypoint at the end of sequence
    void        ClearSequence();    // clear currently selected sequence
    void        DeleteKeypoints(); // Delete all points within the selection time window for channels in record mode   
    void        DeleteKeypointsInRange(float t0, float t1);
    void        SetInitial();
    void        LoadJSON(std::string filename);
    void        StoreJSON(std::string filename);  

    void        Command(std::string s, float x, float y, std::string value);

    std::string GetJSONData(const std::string & name, const std::string & tab);

    void        SetTargetForTime(float t);
    void        SetOutputForChannel(int c); 
    void            StartRecord();



    // Current state

    int             channels;
    int             max_sequences;

    float *         trig;
    float *         playing;
    float *         completed;

    float *         positions;

    float           smoothing_time;
    float *         smoothing_start;

    float *         target;
    float *         input;
    float *         default_output; // value for initial from ikg3 file if set
    float *         internal_control;

    float *         initial;
    float *         output;
    float *         active;
    float *         ready;
    bool            start_record;

    int             current_sequence;
    std::string     sequence_names;
    json        sequence_data;

    // Control  variables

    const int states = 8;

    float *     state; // state of the head controller buttons
    float **    channel_mode;

    float *     left_time;
    float *     right_time;
    float *     left_position;
    float *     right_position;
    int *       left_index;
    int *       right_index;

    float           last_time;
    float           last_index;

    Timer       timer;
    float       position;
    float       last_record_position;
    float       last_position; // to see if the value has been changed by WebUI
    float       mark_start;
    float       mark_end;

    std::string filename;

    std::string time_string;
    std::string end_time_string;

    void        SetOutputForTime(float t); // time in ms

/*
    void        ToggleMode(int x, int y);

    float *     trig;
    float *     trig_last;
    int         trig_size;
    float *     trig_out;
    

    float *     completed;

    float *     input;
    float *     output;

    float *     positions;

    int         smoothing_time; // for torque and position
    float *     stop_position;
    float *     start_position;
    float *     start_torque;
    float *     enable;

    const char * file_name;
    const char * json_file_name;
    const char * directory;

    bool        record_on_trig;
    bool        auto_save;
*/

};

#endif

