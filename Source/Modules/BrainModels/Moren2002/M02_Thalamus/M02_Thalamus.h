//
//	M02_Thalamus.h		This file is a part of the IKAROS project
//							A module implemeting a simplistic model of the M02_Thalamus
//
//    Copyright (C) 2002  Jan Morén
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
//	Created: 2002-01-05
//
//	2002-12-25 Converted to new simulator
//
//	A fake thalamus module. All it does is add a new output of the maximum over
//	all inputs. Do not take this as a serious model of thalamic function!

/********************************************************************

Simulation definition file:

<module class = "M02_Thalamus" name = "THA">
</module>

In:

	CS		- Vector of stimuli

Out:

	CSout	- Vector of stimuli - identical to CS

	TH		- Scalar maximum ovar all CS
	
*******************************************************************/

#include "IKAROS.h"

class M02_Thalamus: public Module 
{
public:
    M02_Thalamus(Parameter * p) : Module(p) {}
    virtual ~M02_Thalamus() {}
    
    static Module * Create(Parameter * p) {return new M02_Thalamus(p);}

    void Init();
    void Tick();

    int     size;

    float * input;		// The main input vector ("CS")
    float * Th;         // Thalamic output (scalar) ("TH")
    float * output;     // Output to Amygdala ("CSout" , "CSout" parameter)
};


