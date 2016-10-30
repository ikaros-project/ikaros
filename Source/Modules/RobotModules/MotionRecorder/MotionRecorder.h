//
//	MotionRecorder.h		This file is a part of the IKAROS project
//
//    Copyright (C) 2015-2016 Christian Balkenius
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

#ifndef MotionRecorder_
#define MotionRecorder_

#include "IKAROS.h"

class MotionRecorder: public Module
{
public:
    static Module * Create(Parameter * p) { return new MotionRecorder(p); }

    MotionRecorder(Parameter * p) : Module(p) {}
    virtual ~MotionRecorder();

    void 		Init();
    void 		Tick();

    void 		Save();
    void 		Load();

    
    float *     trig;
    float *     trig_last;
    int         trig_size;

    float *     completed;

    float *     output;
    float *     state;
    float *     torque;
    float *     mask;
    float *     input;
    float *     playing;
    
    int         smoothing_time; // for torque and position
    float *     stop_position;
    float *     start_position;
    float *     start_torque;

    float *     mode; // record, play, hold, free = (0, 1, 2, 3)

    int         size;

    long        timebase;

    float ***   position_data;
    float ***   torque_data;

    int         max_behaviors;
    int         current_behavior;

    int         position_data_max;
    int   *     position_data_count;

    float *     time; // in ms

    float *     play_torque;
    float *     enable;

    const char * file_name;
//    int          file_count;

    bool        stop;
    bool        off;
    bool        record;
    bool        play;
    bool        clear;
    bool        save;
    bool        load;
    bool        sqplay;

    bool        record_on_trig;
    bool        auto_save;

    bool        stop_debounce;
    bool        off_debounce;
    bool        record_debounce;
    bool        play_debounce;
    bool        clear_debounce;
    bool        save_debounce;
    bool        load_debounce;
    bool        sqplay_debounce;
};

#endif

