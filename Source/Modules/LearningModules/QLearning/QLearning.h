//
//	  QLearning.h		This file is a part of the IKAROS project
//                      Simple implementation of Q-learning
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

#ifndef QLEARNING
#define QLEARNING

#include "IKAROS.h"

class QLearning: public Module 
{
public:
	float **    state;
	float *     action;
	float *     reinforcement;
	float ** 	Q[4];
    float **    value;
    
	int         size_x;
	int         size_y;
	
	int         last_action;
	int         last_x;
	int         last_y;
	
    int         horizon;    // 0 = infinite, 1 = finite
	float		alpha;
	float		gamma;
	float		epsilon;
	float		initial;
	
    QLearning(Parameter * p) : Module(p) {}
    virtual ~QLearning();
        
    static Module * Create(Parameter * p) { return new QLearning(p); }

    void 		Init();

	void		RunQ(float R);

    void 		Tick();
		
};

#endif

