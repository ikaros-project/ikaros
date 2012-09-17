// M02_OFC.h
//
//    Copyright (C) 2002, 2003 Jan Moren
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
//
// Jan Morén, 2001-12-26
//
// 2002-12-23 Moved to the new simulator
// 2009-06-20 Moved to Ikaros 1.2 (CB)
// 
// Implements the basic orbitofrontal model. Used together with the Amygdala
// model.

/*******************************************************************/

#include "IKAROS.h"

class M02_OFC: public Module 
{

public:
    M02_OFC(Parameter * p) : Module(p) {};
    virtual ~M02_OFC();

    static Module * Create(Parameter * p) {return new M02_OFC(p);};

    void SetSizes();
    void Init();
    void Tick();

    int     input_size;
	int		CON_size;
	int     A_size;
    int     T_size;
	    

    float * input;	// The main input vector ("CS")
    float * Rew;	// Reinforcing signal (scalar) ("Rew")
    float * EO;		// Inhibitory output from M02_OFC ("EO")
    float * A;		// Input vector to OFC ("A")
    float * CON;	// Context input vector ("CON")
    
    float * O;
    float * W;
    float * T;
    float * T_last;

    float beta;
};

