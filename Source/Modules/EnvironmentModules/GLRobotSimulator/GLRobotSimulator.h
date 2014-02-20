//
//	GLRobotSimulator.h		This file is a part of the IKAROS project
//                      
//
//    Copyright (C) 2012-2014 Christian Balkenius
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

#ifndef GLRobotSimulator_
#define GLRobotSimulator_


#include "IKAROS.h"

class GLRobotSimulator: public Module
{
public:

    GLRobotSimulator(Parameter * p) : Module(p) {}
    virtual ~GLRobotSimulator() {}

    static Module * Create(Parameter * p) { return new GLRobotSimulator(p); }

    bool        ObjectIsInView(float id);
    float       DistanceToObject(float id);
    float       GetClosestHighestObject(float * place, float max_distance=50.0);

    void 		Init();
    void 		Tick();

    bool        io_flag;
    
    float **    objects_in;
    float **    objects_out;
    int         object_count;
        
    float *     move_block;

    int         current_action;
    int         current_phase;
    int         current_step;
    
    float       goal_direction;
    float       dr;
    float       ir;
    float       ix;
    float       iy;
    
    int         carried_object;
    int         pick;
    float       goal[2];
    float       place[4];
    
    float       last_goal_location[4];
    

    // Visualization output

    float **    landmarks;
    float **    blocks;
        
    float *     range_map;
    float *     range_color;
    
    int         range_map_size;

    float *     view_field;
    float *     target;
    
    // Control Inputs and Outputs
    
    // outputs
    
    float *     robot_location;
    float *     speed_out;
    float *     battery_level;
    float **    blocks_in_view;
    
    float *     locomotion_phase;
    float *     pick_phase;
    float *     place_phase;
    float *     charge_phase;

    float *     locomotion_error;
    float *     pick_error;
    float *     place_error;
    float *     charge_error;

    // inputs

    float *     goal_location;
    float *     speed;
    float *     locomotion_trigger;
    float *     pick_object_id;
    float *     pick_object_spatial_attention;
    float *     pick_trigger;
    float *     place_object_location;
    float *     place_trigger;
    float *     charge_batteries;
    
    // parameters
    
    float       locomotion_speed;
    float       rotation_speed;
    float       arm_length;
    float *     charging_station;
    float       view_radius;
    float       battery_decay;
    bool        auto_stack;
};

#endif

