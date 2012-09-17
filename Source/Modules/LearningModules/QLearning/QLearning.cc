//
//	QLearning.cc		This file is a part of the IKAROS project
//                      Implementation of Q-learning
//
//    Copyright (C) 2003 Christian Balkenius
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
//	Created: 2003-08-02
//

#include "QLearning.h"


using namespace ikaros;

void
QLearning::Init()
{
    horizon     = GetIntValueFromList("horizon");
	alpha		= GetFloatValue("alpha");
	gamma		= GetFloatValue("gamma");
	epsilon		= GetFloatValue("epsilon");
    initial     = GetFloatValue("initial");

	state           = GetInputMatrix("STATE");
    size_x          = GetInputSizeX("STATE");
    size_y          = GetInputSizeY("STATE");

	reinforcement	= GetInputArray("REINFORCEMENT");
	action          = GetOutputArray("ACTION");	
    
	Q[0]             = set_matrix(create_matrix(size_x, size_y), initial, size_x, size_y);
	Q[1]             = set_matrix(create_matrix(size_x, size_y), initial, size_x, size_y);
	Q[2]             = set_matrix(create_matrix(size_x, size_y), initial, size_x, size_y);
	Q[3]             = set_matrix(create_matrix(size_x, size_y), initial, size_x, size_y);
    
    value            = GetOutputMatrix("VALUE");
    
    last_action     = -1;
    
    // Fill in initial values
    
    for(int j=0; j<size_y; j++)
        for(int i=0; i<size_x; i++)
        {
            value[j][i] = 0;
            for(int a = 0; a < 4; a++)
                value[j][i] = max(Q[a][j][i], value[j][i]);
        }
}



QLearning::~QLearning()
{
	destroy_matrix(Q[0]);
	destroy_matrix(Q[1]);
	destroy_matrix(Q[2]);
	destroy_matrix(Q[3]);
}



void
QLearning::Tick()
{
    int x = 0, y = 0;
    arg_max(x, y, state, size_x, size_y);
	
	// Evaluate last action

    if(last_action != -1)
    {
        float V = 0;
        for(int i = 0; i < 4; i++)
            V = max(Q[i][y][x], V);
        
        switch(horizon)
        {
            case 0: // infinite
                V += *reinforcement;

            case 1: // finite
                if(*reinforcement > 0)
                    V = *reinforcement;
        }
        
        Q[last_action][last_y][last_x] += alpha * (gamma * V - Q[last_action][last_y][last_x]);
	}
    
	// Produce new action

    int a = int(random(0.0, 4.0));
	if(random(0.0, 1.0) > epsilon)
    {
        float action_strength[4];
		for(int i = 0; i < 4; i++)
			action_strength[i] = Q[i][y][x];
		if(non_zero(action_strength, 4))
			a = arg_max(action_strength, 4);
	}

	reset_array(action, 4);
	action[a]  = 1;	

    // Update value output (assuming we start with 0 in Q-table)
    
    value[last_y][last_x] = 0;
    for(int i = 0; i < 4; i++)
        value[last_y][last_x] = max(Q[i][last_y][last_x], value[last_y][last_x]);
    
	// Remember action and state

	last_action = a;
    last_x = x;
    last_y = y;
}


