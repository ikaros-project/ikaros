//
//	ContinuousWorld.cc		This file is a part of the IKAROS project
//					        Implements a world with obstacles and rewards
//
//    Copyright (C) 2022 Christian Balkenius
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

#include "ContinuousWorld.h"



bool 
is_left(float * a, float * b, float * p)
{
     return ((b[0] - a[0])*(p[1] - a[1]) - (b[1] - a[1])*(p[0] - a[0])) > 0;
}




float
angle_to(float * a, float * b, float * p)
{
 //   printf("%f %f - %f %f; %f %f\t\t", a[0], a[1], b[0],b[1], p[0], p[1]);
    float l0 = hypot((b[0]-a[0]), (b[1]-a[1]));
    float l1 = hypot((p[0]-a[0]), (p[1]-a[1]));
    float ca = ((b[0]-a[0])*(p[0]-a[0]) + (b[1]-a[1])*(p[1]-a[1]))/(l0*l1);

    if(ca > 1)
        ca = 1;
    if(ca < -1)
        ca = -1;

    if(is_left(a, b, p))
        return -acos(ca);
    else
        return acos(ca);
    
}





using namespace ikaros;


void
ContinuousWorld::Init()
{
    Bind(place_obstacles_at, "place_obstacles_at");
    
    io(attend, "ATTEND");
    io(visible, "VISIBLE");
    io(approach, "APPROACH");
    io(object, "OBJECT");
    io(reward, "REWARD");
    io(obstacle, "OBSTACLE");
    io(obstacles_pos, "OBSTACLES_POS");


    io(agent, "AGENT");
    io(goal, "GOAL");
    io(obstacles, "OBSTACLES");

agent_angle = 0;
agent_length = 1;

    agent = {{5, 1, 5, 2}};

    goal = {{3, 5}, {7, 5}};

    obstacles =  {
                    {-1, -1,-1, -1},
                    {-1, -1,-1, -1},
                    {-1, -1,-1, -1},
                    {-1, -1,-1, -1}
                };

    obstacles_pos =  {
                    {2, 4, 4,4},
                    {4, 4, 4, 6},
                    {6, 4, 8, 4},
                    {6, 4, 6, 6}
                };

    obstacle_present[0] = false;
    obstacle_present[1] = false;   
}



void
ContinuousWorld::Tick()
{
    if(GetTick() == place_obstacles_at)
    {
        obstacles = obstacles_pos;
    obstacle_present[0] = true;
    obstacle_present[1] = true;   
    }



    visible[0] = 1;
    reward[0] = 0.2;

    object[0] = 0;
    object[1] = 0;

    float avoidance_factor = 0;
    float avoidance_angle = 0;
    float angle_to_obstacle = 0;

    // Avoidance

    if(obstacle_present[0] && obstacle_present[0])
    {


    vector ag = {agent[0][0], agent[0][1]};

    float dd = 1000;
    vector p;

    for(int i=0; i<4; i++)
    {
        vector o0 = {obstacles[i][0], obstacles[i][1]};
        vector o1 = {obstacles[i][2], obstacles[i][3]};
        vector cp = closest_point_on_line_segment(o0, o1, ag);
        float dist = hypot(cp[0]-ag[0], cp[1]-ag[1]);
        if(dist < dd)
        {
            dd = dist;
            p = cp;
        }
    }

    obstacle[0] = p[0];
    obstacle[1] = p[1];

    angle_to_obstacle = angle_to(agent[0], &agent[0][2], &p[0]);
    float distance_to_obstacle = hypot(p[0]-agent[0][0], p[1]-agent[0][1]);

    if(abs(angle_to_obstacle) < pi/2) // approaching obstacle
    {
        avoidance_factor = 1/(2+distance_to_obstacle*distance_to_obstacle);
        if(avoidance_factor > 1)
            avoidance_factor = 1;
    }

    if(distance_to_obstacle<0.1)
        avoidance_factor = 1;
    }

    // Approach

    float approach_speed = 0.1; 
    float turn_rate = 0.25;
    float turn = 0;

    if(attend[0] > 0)
            turn = turn_rate * angle_to(agent[0], &agent[0][2], goal[0]);
    if(attend[1] > 0)
            turn = turn_rate * angle_to(agent[0], &agent[0][2], goal[1]);

    agent_angle += (1-avoidance_factor) * turn - avoidance_factor * angle_to_obstacle;


    if(*approach)
    {
        agent[0][0] += approach_speed*cos(agent_angle);
        agent[0][1] -= approach_speed*sin(agent_angle); 
    }

    agent[0][2] = agent[0][0] + agent_length*cos(agent_angle);
    agent[0][3] = agent[0][1] - agent_length*sin(agent_angle); 

    // Rewards

    float goalDist0 = hypot(agent[0][0]-goal[0][0], agent[0][1]-goal[0][1]);
    float goalDist1 = hypot(agent[0][0]-goal[01][0], agent[0][1]-goal[1][1]);
*reward = 0;

    if(goalDist0 < 0.2)
    {
        *reward = 1.0;
    agent = {{5, 1, 5, 2}};
    }

    if(goalDist1 < 0.2)
    {
        *reward = 0.1;
        agent = {{5, 1, 5, 2}};
    }   

    // Bottom-up attention

    if(obstacle_present[0] && obstacle_present[0])
    {
        visible[0] = 0;
        visible[1] = 0;
    }
    else
    {
        visible[0] = 1;
        visible[1] = 1;
    }

    if(agent[0][0] < 4 && agent[0][1] > 4)
    {
        visible[0] = 1;
    }

    if(agent[0][0] > 6 && agent[0][1] > 4)
    {
        visible[1] = 1;
    }

    //Object identification

    if(attend[0]>0 && visible[0])
{
    object[0] = 1;
    object[1] = 0;
}


    if(attend[1]>0 && visible[1])
{
    object[0] = 0;
    object[1] = 1;
}

}




static InitClass init("ContinuousWorld", &ContinuousWorld::Create, "Source/Modules/EnvironmentModules/ContinuousWorld/");

