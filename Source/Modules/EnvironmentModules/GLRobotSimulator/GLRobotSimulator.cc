//
//	GLRobotSimulator.cc		This file is a part of the IKAROS project
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


// TODO: Do not use persistent indices, will not work if world is changed
// TODO: Clean up the code
// TODO: Use superclass for the messy stuff

#include "GLRobotSimulator.h"

using namespace ikaros;

// World data structure

const int object_type = 0;
const int object_id = 1;
//const int marker_id = 2;
//const int marker_size = 3;
const int cf = 4;
const int obj_coord = 5;
const int cam_coord = obj_coord+16;
const int world_coord = cam_coord+16;
const int world_coord_x = world_coord+3;
const int world_coord_y = world_coord+7;
const int world_coord_z = world_coord+11;
const int columns = world_coord+16;

// Object types

//const int ot_camera = 0;
const int ot_robot = 1;
const int ot_world = 2;
//const int ot_marker = 3;
const int ot_cube = 4;

// Action types

const int ACTION_NONE = 0;
const int ACTION_MOVE = 1;
const int ACTION_PICK = 2;
const int ACTION_PLACE = 3;
const int ACTION_CHARGE = 4;


static float
short_angle(float a1, float a2)
{
    return atan2(sin(a2-a1), cos(a2-a1)); 
}



static float
integer(float x)
{
    return float(int(x));
}



static float object_color(float id)
{
    if(id > 10) id-= 10;
    return float(1+int((id-1)/10));
}



void
GLRobotSimulator::Init()
{
    Bind(locomotion_speed, "locomotion_speed");
    Bind(rotation_speed, "rotation_speed");
    Bind(arm_length, "arm_length");
    Bind(view_radius, "view_radius");
    Bind(battery_decay, "battery_decay");
    Bind(auto_stack, "auto_stack");

    objects_in = GetInputMatrix("OBJECTS_IN");
    objects_out = GetOutputMatrix("OBJECTS_OUT");
    object_count = GetInputSizeY("OBJECTS_IN");
    
    move_block = GetInputArray("MOVE_BLOCK", false); // not required
    
    blocks = GetOutputMatrix("BLOCKS");
    landmarks = GetOutputMatrix("LANDMARKS");
    
    view_field = GetOutputArray("VIEW_FIELD");
    target = GetOutputArray("TARGET");

    range_map = GetOutputArray("RANGE_MAP");
    range_color = GetOutputArray("RANGE_COLOR");
    range_map_size = GetOutputSize("RANGE_MAP");
    blocks_in_view = GetOutputMatrix("BLOCKS_IN_VIEW");

    robot_location = GetOutputArray("ROBOT_LOCATION");
    speed_out = GetOutputArray("SPEED_OUT");
    battery_level = GetOutputArray("BATTERY_LEVEL");
    charging_station = GetOutputArray("CHARGING_STATION");
    
    copy_array(charging_station, GetArray("charging_station", 2), 2);

    locomotion_phase = GetOutputArray("LOCOMOTION_PHASE");
    pick_phase = GetOutputArray("PICK_PHASE");
    place_phase = GetOutputArray("PLACE_PHASE");
    charge_phase = GetOutputArray("CHARGE_PHASE");

    locomotion_error = GetOutputArray("LOCOMOTION_ERROR");
    pick_error = GetOutputArray("PICK_ERROR");
    place_error = GetOutputArray("PLACE_ERROR");
    charge_error = GetOutputArray("CHARGE_ERROR");

    direct_control = GetInputArray("DIRECT_CONTROL", false);
    goal_location = GetInputArray("GOAL_LOCATION");
    speed = GetInputArray("SPEED");
    locomotion_trigger = GetInputArray("LOCOMOTION_TRIGGER");
    pick_object_id = GetInputArray("PICK_OBJECT_ID");
    pick_object_spatial_attention = GetInputArray("PICK_OBJECT_SPATIAL_ATTENTION", false); 
    pick_trigger = GetInputArray("PICK_OBJECT_TRIGGER");
    place_object_location = GetInputArray("PLACE_OBJECT_LOCATION");
    place_trigger = GetInputArray("PLACE_OBJECT_TRIGGER");
    charge_batteries = GetInputArray("CHARGE_BATTERIES");

    current_action = ACTION_NONE;
    current_phase = 0;
    current_step = 0;
    
    *battery_level = 1;
    locomotion_phase[4] = 1;
    
    carried_object = -1;
    pick = -1;
    
    place[0] = -1;
    place[1] = -1;
    place[2] = -1;
    place[3] = -1;

    for(int i=0; i<50; i++) // FIXME: Should be parameter
    {
        blocks[i][0] = -99999;
        blocks[i][1] = -99999;
        landmarks[i][0] = -99999;
        landmarks[i][1] = -99999;
    }

    io_flag = false;
    
    reset_array(last_goal_location, 4);
    set_array(target, -99999, 4);
}


static int
calc_index(float vx, float vy, float ox, float oy, int size)
{    
    float a = atan2(vy, vx) - atan2(oy, ox);
    int i = size-int(size*(1-(a/pi))/2);
    
    if(i<0) i += size;
    if(i>size-1) i-= size;
    return i;
}


bool
GLRobotSimulator::ObjectIsInView(float id)
{
    id = integer(id);
    for(int i=0; i<object_count; i++)
    {
        if(objects_out[i][object_type] == ot_cube && objects_out[i][object_id] == id)
        {
            float pos[2] = { objects_out[i][world_coord_x], objects_out[i][world_coord_y] };
            if(dist(view_field, pos, 2) < view_radius) 
                return true;
            else
                return false;
        }
    }
    return false;
}



float
GLRobotSimulator::DistanceToObject(float id)
{
    id = integer(id);
    for(int i=0; i<object_count; i++)
    {
        if(objects_out[i][object_type] == ot_cube && objects_out[i][object_id] == id) 
        {
            float loc[2] = { objects_out[i][world_coord_x], objects_out[i][world_coord_y] };
            return dist(robot_location, loc, 2);
        }
    }

    return 100.0*arm_length; // id not found, return distance that cannot be reached
}



float
GLRobotSimulator::GetClosestHighestObject(float * place, float max_distance) // Very slow way to do this
{
    float oi = -1;
    float d = max_distance;
    float last_height = -1;
    int ix = -1;
    
    for(int i=0; i<object_count; i++)
    {
        float loc[3] = { objects_out[i][world_coord_x], objects_out[i][world_coord_y], objects_out[i][world_coord_z] };
        if(objects_out[i][object_type] == ot_cube && dist(loc, place, 2) < d && loc[2] > last_height)
        {
            last_height = loc[2];
            oi = objects_out[i][object_id];
            ix = i;
        }
    }
    
    // adjust place
    
    if(ix != -1)
    {
        place[0] = objects_out[ix][world_coord_x];
        place[1] = objects_out[ix][world_coord_y];
        place[2] = objects_out[ix][world_coord_z] + 35.0; // TODO: Check height
        return oi;
    }

    return -1.0; // no object found
}



void
GLRobotSimulator::Tick()
{
    float m;
    
    if(!io_flag)
    {
        copy_matrix(objects_out, objects_in, columns, object_count);
        io_flag = true;
    }
    
    // Move block i input exists and indiciates an id
    
    if(move_block && move_block[4] > 0)
    {
        for(int i=0; i<object_count; i++)
        {
            if(objects_out[i][object_type] == ot_cube &&  objects_out[i][object_id] == move_block[4])
            {
                objects_out[i][world_coord_x] = move_block[0];
                objects_out[i][world_coord_x] = move_block[1];
                objects_out[i][world_coord_x] = move_block[2];
                // rotation is ignored for now
            }
        }
    }
    
    // Find robot location and possibly object

    int robot_index = 0;
    int object_index = 0;
    for(int i=0; i<object_count; i++)
    {
        if(objects_out[i][object_type] == ot_robot) 
        {
            robot_index = i;
            robot_location[0] = objects_out[i][world_coord_x];
            robot_location[1] = objects_out[i][world_coord_y];
            robot_location[2] = 0;
            robot_location[3] = atan2(objects_out[i][world_coord+4], objects_out[i][world_coord]);
        }
        if(objects_out[i][object_type] == ot_cube && objects_out[i][object_id] == integer(*pick_object_id))
        {
            object_index = i;
        }
    }
    
    if(pick_object_spatial_attention) // Spatial attention active; find closest object
    {
        float m = maxfloat;
        for(int i=0; i<object_count; i++)
        {
            float tmp = 0;
             if(objects_out[i][object_type] == ot_cube && objects_out[i][cf] > 0.7 && (tmp = hypot(pick_object_spatial_attention[0]-objects_out[i][world_coord_x], pick_object_spatial_attention[1]-objects_out[i][world_coord_y])) < m)
            {
                m = tmp;
                object_index = i;
                *pick_object_id = objects_out[i][object_id]; // FIXME: should never change an input
            }
        }
    }

    // Check if robot must move
    
    float loc_trig = *locomotion_trigger;
    
    if(current_action != ACTION_MOVE && dist(goal_location, last_goal_location, 4) > 0)  // TODO: change to support rotation
        loc_trig = 1;

    copy_array(last_goal_location, goal_location, 4);

    if(loc_trig > 0)
    {
        // Start locomotion

        current_action = ACTION_MOVE;
        current_phase = 0;
        current_step = 0;
        
        move_type = 0;

        reset_array(charge_phase, 2);
        reset_array(locomotion_phase, 5);
        locomotion_phase[0] = 1.0;
        *locomotion_error = 0.0;

        float d = sqrt(sqr(robot_location[0]-goal_location[0]) + sqr(robot_location[1]-goal_location[1]) + 10 * sqr(robot_location[3]-goal_location[3])); // FIXME: Goal location has no z
        
        if(d < 200.0) // slide within 20 cm
            move_type = 1;

        if(d < 0.1) // we are at the goal
        {
            reset_array(locomotion_phase, 5);
            locomotion_phase[4] = 1.0;
            copy_array(goal, goal_location, 2);
        }
        else if(goal_location[0] < 0 || goal_location[1] < 0 || goal_location[0] > 1700 || goal_location[1] > 1200)
        {
            reset_array(locomotion_phase, 5);
            *locomotion_error = 1.0;
        }
    }

    // Check pick trigger

    if(*pick_trigger > 0)
    {
        if(norm(locomotion_phase, 4) > 0 && locomotion_phase[4] != 1.0) // we are moving, cannot pick
        {
            *pick_error = 1;
        }
        else if(pick_phase[4] == 1.0) // we are already holding an object, cannot pick
        {
            *pick_error = 1;
        }
        else if(!ObjectIsInView(*pick_object_id)) // we cannot see the object, cannot pick
        {
            *pick_error = 1;
        }
        else if(DistanceToObject(*pick_object_id) > arm_length) // we cannot reach the object, cannot pick
        {
            *pick_error = 1;
        }
        else // start pick action
        {
            current_action = ACTION_PICK;
            current_phase = 0;
            current_step = 0;
            reset_array(pick_phase, 5);
            reset_array(place_phase, 5); // forget that we may have placed an object before
            reset_array(charge_phase, 2);
            pick_phase[0] = 1.0;
            pick = object_index;
            *pick_error = 0;
            
            target[0] = objects_out[pick][world_coord_x]; // Set target output to location of object to pick
            target[1] = objects_out[pick][world_coord_y];
            target[2] = objects_out[pick][world_coord_z];
            target[3] = atan2(objects_out[pick][world_coord+4], objects_out[pick][world_coord]);
        }
    }

    // Check place trigger
    
    if(*place_trigger > 0)
    {
       if(locomotion_phase[4] != 1.0) // we are moving, cannot place
        {
            *place_error = 1;
        }
        else if(pick_phase[4] != 1.0) // we are not holding an object, cannot place
        {
            *place_error = 1;
        }
        else if(dist(place_object_location, robot_location, 3) > arm_length) // location is not in range
        {
            *place_error = 1;
        }
        else if(dist(place_object_location, view_field, 2) > view_radius) // location is not visible
        {
            *place_error = 1;
        }
        else if(carried_object == -1)
        {
            *place_error = 1;
        }
        else
        {
            current_action = ACTION_PLACE;
            current_phase = 0;
            current_step = 0;
            reset_array(place_phase, 5);
            reset_array(charge_phase, 2);
            place_phase[0] = 1.0;
            *place_error = 0;
            copy_array(place, place_object_location, 3); // rotation is ignored for now
            
            if(auto_stack)  // Check if there already is an object and adjust location to place it on top
                GetClosestHighestObject(place, 50.0);
            
            copy_array(target, place, 4); // Set target output to location of object to pick
        }
    }

    // Check charge trigger
    
    if(*charge_batteries > 0)
    {
        if(locomotion_phase[4] != 1.0) // we are moving, cannot charge
        {
            *charge_error = 1;
        }
        else if(dist(charging_station, robot_location, 2) > 50.0) // charging location is not in range (more than 5 cm)
        {
            *charge_error = 1;
        }
        else
        {
            current_action = ACTION_CHARGE;
            current_phase = 0;
            current_step = 0;
            reset_array(charge_phase, 2);
            charge_phase[0] = 1.0;
            *charge_error = 0;
        }
    }

    // State machine

    if(direct_control)
    {
        robot_location[0] = clip(robot_location[0] + direct_control[0], 100, 1600);
        robot_location[1] = clip(robot_location[1] + direct_control[1], 100, 1100);
        robot_location[3] = short_angle(0, robot_location[3] + direct_control[3]);
        
    }

    else
    switch(current_action)
    {
        case ACTION_MOVE: // locomotion
    
            if(move_type == 0)
            {
                switch(current_phase)
                {
                     case 0: // Park arm and initiate initial rotation
                        if(current_step < 50)
                            current_step++;
                        else
                        {
                            goal_direction = atan2(goal_location[1] - robot_location[1], goal_location[0] - robot_location[0]);
                            dr = short_angle(robot_location[3], goal_direction);
                            ir = (dr > 0 ? rotation_speed : -rotation_speed);
                            current_phase = 1;
                            current_step = 0;
                        }
                        break;
                        
                    case 1: // Initial rotation
                        if(abs(short_angle(goal_direction, robot_location[3])) > rotation_speed)
                        {
                            robot_location[3] += *speed * ir;
                        }
                        else
                        {
                            ix = goal_location[0] - robot_location[0];
                            iy = goal_location[1] - robot_location[1];
                            m = max(abs(ix), abs(iy));
                            if(m > 0)
                            {
                                ix /= m;
                                iy /= m;
                                ix *= locomotion_speed; // FIXME: reduce speed close to goal
                                iy *= locomotion_speed;
                            }
                            current_phase = 2;
                        }
                        break;
                        
                    case 2: // Linear movement
                        if(dist(goal, goal_location, 2) > 0.0001) // goal location changed; return to phase 1
                        {
                            robot_location[0] = clip(robot_location[0], 100, 1600);
                            robot_location[1] = clip(robot_location[1], 100, 1100);
                            copy_array(goal, goal_location, 2);
                            goal_direction = atan2(goal_location[1] - robot_location[1], goal_location[0] - robot_location[0]);
                            dr = short_angle(robot_location[3], goal_direction);
                            ir = (dr > 0 ? rotation_speed : -rotation_speed);
                            *locomotion_error = 0;
                            current_phase = 1;
                        }
                        else if(robot_location[0] < 100 || robot_location[0] > 1600 || robot_location[1] < 100 || robot_location[1] > 1100) // Outside area; Should not be hard coded
                        {
                             *locomotion_error = 1;
                            break;
                        }
                        else if(sqrt(sqr(robot_location[0]-goal_location[0]) + sqr(robot_location[1]-goal_location[1])) > 10)
                        {
                            if(sqr(robot_location[0]-goal_location[0]))
                                robot_location[0] += *speed * ix;
                            if(sqr(robot_location[1]-goal_location[1]))
                                robot_location[1] += *speed * iy;
                                
                            if(pick_phase[4] == 1.0 && object_index != 0) // we are holding an object, move it with the robot
                            {
                                objects_out[carried_object][world_coord_x] = robot_location[0];
                                objects_out[carried_object][world_coord_y] = robot_location[1];
                            }
                        }
                        else
                        {
                            dr = short_angle(robot_location[3], goal_location[3]); // FIXME: Goal location has no z
                            ir = (dr > 0 ? rotation_speed : -rotation_speed);
                            current_phase = 3;
                        }
                        break;
                        
                    case 3: // Final rotation
                        if(abs(short_angle(robot_location[3], goal_location[3])) > rotation_speed)
                        {
                            robot_location[3] += *speed * ir;
                        }
                        else
                        {
                            current_phase = 4; // movement complete
                            current_action = ACTION_NONE;
                        }
                        break;
                        
                    default:
                        break;
                }
            }
            
            else // move_type == 1
            {
                switch(current_phase)
                {
                    case 0: // Initiate
                            goal_direction = atan2(goal_location[1] - robot_location[1], goal_location[0] - robot_location[0]);
                            dr = short_angle(robot_location[3], goal_direction);
                            ir = (dr > 0 ? rotation_speed : -rotation_speed);
                            ix = goal_location[0] - robot_location[0];
                            iy = goal_location[1] - robot_location[1];
                            m = max(abs(ix), abs(iy));
                            if(m > 0)
                            {
                                ix /= m;
                                iy /= m;
                                ix *= locomotion_speed; // FIXME: reduce speed close to goal
                                iy *= locomotion_speed;
                            }
                            current_phase = 2;
                            current_step = 0;
                        break;
                        
                    case 1:
                            current_phase = 2;
                            current_step = 0;
                        break;
                        
                    case 2: // "Linear" movement
                        if(dist(goal, goal_location, 2) > 0.0001) // goal location changed; return to phase 1
                        {
                            robot_location[0] = clip(robot_location[0], 100, 1600);
                            robot_location[1] = clip(robot_location[1], 100, 1100);
                            copy_array(goal, goal_location, 2);
                            goal_direction = atan2(goal_location[1] - robot_location[1], goal_location[0] - robot_location[0]);
                            dr = short_angle(robot_location[3], goal_direction);
                            *locomotion_error = 0;
                            current_phase = 0;
                        }
                        else if(robot_location[0] < 100 || robot_location[0] > 1600 || robot_location[1] < 100 || robot_location[1] > 1100) // Outside area; Should not be hard coded
                        {
                             *locomotion_error = 1;
                            break;
                        }
                        else if(sqrt(sqr(robot_location[0]-goal_location[0]) + sqr(robot_location[1]-goal_location[1])) > 10)
                        {
                            // Move
                            if(sqr(robot_location[0]-goal_location[0]))
                                robot_location[0] += *speed * ix;
                            if(sqr(robot_location[1]-goal_location[1]))
                                robot_location[1] += *speed * iy;
                                
                            if(pick_phase[4] == 1.0 && object_index != 0) // we are holding an object, move it with the robot
                            {
                                objects_out[carried_object][world_coord_x] = robot_location[0];
                                objects_out[carried_object][world_coord_y] = robot_location[1];
                            }
                            
                            // Rotate
                            if(abs(short_angle(robot_location[3], goal_location[3])) > rotation_speed)
                            {
                                robot_location[3] += *speed * ir;
                            }
                        }
                        else if(abs(short_angle(robot_location[3], goal_location[3])) > rotation_speed) // Only rotate
                        {
                            robot_location[3] += *speed * ir;
                        }

                        else // we are finished
                        {
                            current_phase = 4; // movement complete
                            current_action = ACTION_NONE;
                        }
                        break;
                
                    default:
                        break;
                }
            }
            
            reset_array(locomotion_phase, 5);
            locomotion_phase[current_phase] = 1.0;
            *battery_level = max(0.0f, *battery_level-battery_decay);
            break;
            
        case ACTION_PICK: // pick
            switch(current_phase)
            {
                case 0:
                    if(current_step < 50)
                        current_step++;
                    else
                    {
                        current_phase = 1;
                        current_step = 0;
                    }
                    break;
                    
                case 1:
                    if(current_step < 50)
                        current_step++;
                    else
                    {
                        current_phase = 2;
                        current_step =0 ;
                    }
                    break;
                    
                case 2:
                    if(current_step < 50)
                        current_step++;
                    else
                    {
                        current_phase = 3;
                        current_step = 0;
                    }
                    break;
                    
                case 3:
                    if(current_step < 50)
                        current_step++;
                    else
                    {
                        current_phase = 4;
                        current_step = 0;
                    }
                    break;
                    
                case 4:
                    // Move object to robot (ignore the arm location for now)
                    if(object_index != 0)
                    {
                        objects_out[pick][world_coord_x] = robot_location[0];
                        objects_out[pick][world_coord_y] = robot_location[1];
                        objects_out[pick][world_coord_z] = 350;
                        carried_object = pick;
                        pick = -1;
                    }
                    current_action = ACTION_NONE;
                    break;
            }
            reset_array(pick_phase, 5);
            pick_phase[current_phase] = 1.0;
            *battery_level = max(0.0f, *battery_level-battery_decay);
            
            break;

        case ACTION_PLACE: // place
            switch(current_phase)
            {
                case 0:
                    if(current_step < 50)
                        current_step++;
                    else
                    {
                        current_phase = 1;
                        current_step = 0;
                    }
                    break;
                    
                case 1:
                    if(current_step < 50)
                        current_step++;
                    else
                    {
                        current_phase = 2;
                        current_step =0 ;
                    }
                    break;
                    
                case 2:
                    if(current_step < 50)
                        current_step++;
                    else
                    {
                        current_phase = 3;
                        current_step = 0;
                    }
                    break;
                    
                case 3:
                    if(current_step < 50)
                        current_step++;
                    else
                    {
                        current_phase = 4;
                        current_step = 0;
                    }
                    break;
                    
                case 4:
                    objects_out[carried_object][world_coord_x] = place[0];
                    objects_out[carried_object][world_coord_y] = place[1];
                    objects_out[carried_object][world_coord_z] = place[2];
                    
                    reset_array(pick_phase, 5); // we are no longer holding an object
                    current_action = ACTION_NONE;
                    carried_object = -1;
                    set_array(place, -1 ,4);
                    break;
            }
            reset_array(place_phase, 5);
            place_phase[current_phase] = 1.0;
            *battery_level = max(0.0f, *battery_level-battery_decay);
        
            break;

        case ACTION_CHARGE: // charge
            switch(current_phase)
            {
                case 0:
                    if(*battery_level < 1)
                    {
                        *battery_level += 0.005;
                        if(*battery_level > 1.0)
                            *battery_level = 1.0;
                    }
                    else
                    {
                        current_phase = 1;
                    }
                    break;
                    
                case 1:
                    current_action = ACTION_NONE;
            }
            reset_array(charge_phase, 2);
            charge_phase[current_phase] = 1.0;

            break;
            
        default:
            break;
    }

    // Update world data structure
    
    objects_out[robot_index][world_coord_x] = robot_location[0];
    objects_out[robot_index][world_coord_y] = robot_location[1];
    objects_out[robot_index][world_coord] = cos(robot_location[3]);      // Check order
    objects_out[robot_index][world_coord+4] = sin(robot_location[3]);

    //
    //
    // Generate all outputs
    //
    //
    
    // Generate object in view output

    view_field[0] = robot_location[0] + (150+view_radius)*cos(robot_location[3]);
    view_field[1] = robot_location[1] + (150+view_radius)*sin(robot_location[3]);

    reset_matrix(blocks_in_view, 6, 50);

    int k=0;
    for(int i=0; i<object_count; i++)
    {
        if(objects_out[i][object_type] == ot_cube && objects_out[i][cf] > 0.7)
        {
            float pos[2] = { objects_out[i][world_coord_x], objects_out[i][world_coord_y] };
            if(dist(view_field, pos, 2) < view_radius) 
            {
                blocks_in_view[k][0] = objects_out[i][world_coord_x];
                blocks_in_view[k][1] = objects_out[i][world_coord_y];
                blocks_in_view[k][2] = objects_out[i][world_coord_z];
                blocks_in_view[k][3] = atan2(objects_out[i][world_coord+4], objects_out[i][world_coord]);
                blocks_in_view[k][4] = objects_out[i][object_id];
                blocks_in_view[k][5] = object_color(objects_out[i][object_id]);            

                k++;
            }
        }
    }

    // Generate position output for world markers
    
    set_matrix(landmarks, -99999, 2, 50);
    k = 0;
    for(int i=0; i<object_count; i++)
    {
        if(objects_out[i][object_type] == ot_world && k<50)
        {
            landmarks[k][0] = objects_out[i][world_coord_x];
            landmarks[k][1] = objects_out[i][world_coord_y];
            landmarks[k][2] = 0;
            landmarks[k][3] = 0;
            landmarks[k][4] = 0; // no id yet
            landmarks[k][5] = 0; // no color
            k++;
        }
    }

    set_matrix(blocks, -99999, 2, 50);
    k = 0;
    for(int i=0; i<object_count; i++)
    {
        if(objects_out[i][cf] > 0.7 && objects_out[i][object_type] == ot_cube && k<50)
        {
            blocks[k][0] = objects_out[i][world_coord_x];
            blocks[k][1] = objects_out[i][world_coord_y];
            blocks[k][2] = objects_out[i][world_coord_z];
            blocks[k][3] = atan2(objects_out[i][world_coord+4], objects_out[i][world_coord]);
            blocks[k][4] = objects_out[i][object_id];
            blocks[k][5] = object_color(objects_out[i][object_id]);
            k++;
        }
    }
    
    *speed_out = *speed;
    
    // Generate range image from all objects
    // TODO: Move to separate module later to make it available to the real robot
    
    // Robot view vector

    float x0 = robot_location[0];
    float y0 = robot_location[1];

    float vx = view_field[0] - x0;
    float vy = view_field[1] - y0;
    
    float vd = hypot(vx, vy);
    if(vd > 0)
    {
        vx /= vd;
        vy /= vd;
    }
    
    // Set initial range to walls

    set_array(range_map, 1700, range_map_size);
    set_array(range_color, 0, range_map_size);
    
    for(int i=0; i<1700; i+=1)
    {
        float ox = float(i) - x0;
        float oy = 0 - y0;
        int index = calc_index(vx, vy, ox, oy, range_map_size);
        float d = hypot(ox, oy);
        if(d < range_map[index])
            range_map[index] = d;
    }

    for(int i=0; i<1700; i+=1)
    {
        float ox = float(i) - x0;
        float oy = 1200 - y0;
        int index = calc_index(vx, vy, ox, oy, range_map_size);
        float d = hypot(ox, oy);
        if(d < range_map[index])
            range_map[index] = d;
    }

    for(int i=0; i<1200; i++)
    {
        float ox = 1700 - x0;
        float oy = float(i) - y0;
        int index = calc_index(vx, vy, ox, oy, range_map_size);
        float d = hypot(ox, oy);
        if(d < range_map[index])
            range_map[index] = d;
    }

    for(int i=0; i<1200; i++)
    {
        float ox = 0 - x0;
        float oy = float(i) - y0;
        int index = calc_index(vx, vy, ox, oy, range_map_size);
        float d = hypot(ox, oy);
        if(d < range_map[index])
            range_map[index] = d;
    }

    // Sweep rays
    
    for(int i=0; i<object_count; i++)
    {
        if(objects_out[i][object_type] == ot_cube && objects_out[i][cf] > 0.7)
        {
            float ox = objects_out[i][world_coord_x] - x0;
            float oy = objects_out[i][world_coord_y] - y0;
            float od = hypot(ox, oy);

            if(od > 0)
            {
                ox /= od;
                oy /= od;
            }
            
            int index = calc_index(vx, vy, ox, oy, range_map_size);
            float d = hypot(objects_out[i][world_coord_x]-x0, objects_out[i][world_coord_y]-y0);
            
            if(d < range_map[index])
            {
                range_map[index] = d;
                range_color[index] = object_color(objects_out[i][object_id]);
            }
         }
    }
}

static InitClass init("GLRobotSimulator", &GLRobotSimulator::Create, "Source/Modules/EnvironmentModules/GLRobotSimulator/");




