//
//	SequenceRecorder.h		This file is a part of the IKAROS project
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

#ifndef SequenceRecorder_
#define SequenceRecorder_

#include "IKAROS.h"

class MotionTrack
{
public:
    MotionTrack(int no_of_channels);
    ~MotionTrack();
    
    void        AppendKeypoint(float * positions, float * torque, float * enable, float time);  // Add keypoint to the end of the array
    void        GetPosition(float * positions, float * torque, float * enable, float time);     // Interpolate
    
    std::string WriteJSON();
    void        ReadJSON(std::string json);
};



class SequenceRecorder: public Module
{
public:
    static Module * Create(Parameter * p) { return new SequenceRecorder(p); }

    SequenceRecorder(Parameter * p) : Module(p) {}
    virtual ~SequenceRecorder();

    void 		Init();
    void 		Tick();

    // Commands

    void        Command(std::string s, int x, int y, std::string value);

    void        ToggleMode(int x, int y);
    void        Off();
    void        Stop();
    void        Record();
    void        Play();
    void 		Save();
    void 		Load();

    std::string  mode_string;
    float **    mode;

    float *     trig;
    int         trig_size;

    float *     completed;
    float *     input;
    float *     output;
    float *     torque;
    float *     enable;

    int         size; // of all of the above

    int         smoothing_time; // for torque and position
    float       nominal_torque; // torque to record for active tracks - placehoder for the future
    
    float *     stop_position;
    float *     start_position;
    float *     start_torque;

    

    std::string *   motion_name;
    float ***       position_data;
    float **        timestamp_data;
    
    float **        keypoints;

    int         max_motions;
    int         current_motion;

    int         position_data_max;
    int   *     position_data_count;

    float *     time; // in ms

    const char * file_name;
 
    bool        auto_load;
    bool        auto_save;
};

#endif

