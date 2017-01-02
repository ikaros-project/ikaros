//
//	TheEyeTribe.h		This file is a part of the IKAROS project
//
//    Copyright (C) 2016 Birger Johansson
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

#ifndef TheEyeTribe_
#define TheEyeTribe_

#include "IKAROS.h"
#include <gazeapi.h>


class TheEyeTribe: public Module
{
public:
    static Module * Create(Parameter * p) { return new TheEyeTribe(p); }

    TheEyeTribe(Parameter * p) : Module(p) {}
    virtual ~TheEyeTribe();

    void	Init();
    void 	Tick();
	
	float * timeStamp;
	float * raw;
	float * avg;
	float * fix;
	
	float * leftRaw;
	float * leftAvg;
	float * leftPupilSize;
	float * leftPupilCenter;
	
	float * rightRaw;
	float * rightAvg;
	float * rightPupilSize;
	float * rightPupilCenter;
	
	int		StartTime;
	gtl::GazeApi m_api;
};

#endif
