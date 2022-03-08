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
    void        ExtendTime();
    void        ReduceTime();

    void        Command(std::string s, float x, float y, std::string value);

    std::string GetJSONData(const std::string & name, const std::string & tab);

    // Current state

    int         channels;
float *         positions;

float *         output;
float *         active;

    // Data

    json        sequence_data;

    // Control  variables


    float *     state; // state vector for controls
    Timer       timer;
    float       position;
    float       last_position; // to see if the value has been changed by WebUI
    float       mark_start;
    float       mark_end;

    
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

