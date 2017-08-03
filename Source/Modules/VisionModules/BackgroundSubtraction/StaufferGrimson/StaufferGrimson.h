//
//    StaufferGrimson.h		This file is a part of the IKAROS project
// 							Implementation of the Stauffer-Grimson forground/background segmentation for grayscale images
//
//    Copyright (C) 2009 Christian Balkenius
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
//	Created: 2009-05-19
//
//	<Additional description of the module>

#ifndef _StaufferGrimson_
#define _StaufferGrimson_

#include "IKAROS.h"

class StaufferGrimson: public Module
    {
    public:
        StaufferGrimson(Parameter * p) : Module(p) {}
        virtual ~StaufferGrimson() {}
        
        static Module * Create(Parameter * p) { return new StaufferGrimson(p); }
        
        void 		Init();
        void 		Tick();
        
        float       alpha;
        float       backGroundPortion;
        int         gaussPerPixel;
        float       initialStandardDeviation;
        float       deviationMultiplier;

        float **    input;
        float **    output;
        
        int   ***   weightOrder;
        float ***   weight;
        float ***   average;
        float ***   stdDev;
        
        float *     norm_data;

        int         sizex;
        int         sizey;
    };

#endif
