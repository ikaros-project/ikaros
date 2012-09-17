//
//	PlanarArm.h		This file is a part of the IKAROS project
// 					Implements a simple arm and a target object
//
//    Copyright (C) 2007 Christian Balkenius
///
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

#ifndef PlanarArm_
#define PlanarArm_

#include "IKAROS.h"

class PlanarArm: public Module
{
public:
	// Arm variables
	
	float	*	desired_angles;		// input: desired angles
	float	*	angles;			// output: current angles of the arm
	float	*	movement;		// internal: actual movement (< speed)
		
	float	*	segment_length;	// parameter: arm lengths
	float	**	joints;			// output: position of the joints
	float	*	hand_position;		// position of the last
	float	*	hit;				// output: 0/1 contact with the target
	
	float		speed;			// maximum angular velocity per tick

	// Target variables
	
	float	*	target;			// output: location of the target
	int		target_behavior;	// random, circular, linear, etc
	float		target_speed;
	float		target_range;		// distance from center
	float		target_noise;		// stdev of gaussian noise
	float		target_size;		// radius of the target
	int		time;

	// Distance
	
	float *	distance;			// output: distance from hand to target
	float *	contact;			// output: 0/1
	float		grasp_limit;		// max distance from hand to target for a grasp (which moves the target to a random location)

	PlanarArm(Parameter * p) : Module(p) {}
	virtual ~PlanarArm() {}

	static Module * Create(Parameter * p) { return new PlanarArm(p); }

	void		MoveArm();
	void		UpdateArm();

	void 		Init();
	void 		Tick();

};

#endif

