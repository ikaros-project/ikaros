// M02_Amygdala.h
//
//    Copyright (C) 2001-2002 Jan Moren
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
//    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307
//    USA
//
//
// 2001-12-06 Initial version 
// 2002-12-20 Moved to IKAROS 0.7 (CB)
// 
// Implements the basic M02_Amygdala model. Also needed is  - ideally - an
// orbitofrontal model and perhaps a Hippocampus and/or Thalamus.


#include "IKAROS.h"

class M02_Amygdala: public Module 
{

public:
    M02_Amygdala(Parameter * p) : Module(p) {};
    virtual ~M02_Amygdala();
    
    static Module * Create(Parameter * p) {return new M02_Amygdala(p);}

    void Init();
    void Tick();

    int     size;
    float * input;          // The main input vector ("CS")
    float * input_last;     // The previous input vector ("OldCS")
    
    float * Rew;	// Reinforcing signal (scalar) ("Rew")
    float * EO;		// Inhibitory signal from OFC (scalar) ("EO")
    float * E;		// Output (scalar) ("E")
    float * A;      // Activity; Output vector to OFC ("O" , "Osize" parameter)
    float * V;

    float   SumA;
    float   OldSumA;
    float   EOld;
    
    float   alpha;
};

