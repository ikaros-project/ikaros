//
//	EyeLidModel.h		This file is a part of the IKAROS project
// 						
//    Copyright (C) 2016 Christian Balkenius
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


#ifndef EyeLidModel_
#define EyeLidModel_

#include "IKAROS.h"

class EyeLidModel: public Module
{
public:
    static Module * Create(Parameter * p) { return new EyeLidModel(p); }

    EyeLidModel(Parameter * p) : Module(p) {}
    virtual ~EyeLidModel() {}

    void 		Init();
    void 		Tick();
    
    float       pupil_min;
    float       pupil_max;

    float       epsilon;
    float       m3;
    float       alpha1a;
    float       amplifier;
    
    float *     gaze;

    float *     pupil_sphincter;
    float *     pupil_dilator;

    float *     pupil_diameter;
    float *     output;
};

#endif

