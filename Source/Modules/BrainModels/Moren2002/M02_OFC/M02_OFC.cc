//    M02_OFC.cc
//
//    Copyright (C) 2001, 2003 Jan Moren
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
// Implements the basic orbitofrontal model.
// Used together with the amygdala model (M02_Amygdala).

#include "M02_OFC.h"

using namespace ikaros;

void M02_OFC::SetSizes()
{
	Module::SetSizes();

    int CONi    = GetInputSize("CON");
    int CSi	    = GetInputSize("INPUT");

    if(CONi != unknown_size && CSi != unknown_size)
    {
        if(CONi==0)
            SetOutputSize("W", CSi);
        else
            SetOutputSize("W", CSi * CONi);
    }
}


void M02_OFC::Init()
{
    Bind(beta, "beta");
    
    CON_size	= GetInputSize("CON");
    input_size  = GetInputSize("INPUT");
    A_size		= GetInputSize("A");

    input	= GetInputArray("INPUT");
    CON     = GetInputArray("CON", false);
    Rew     = GetInputArray("Rew");
    A       = GetInputArray("A");
    W      = GetOutputArray("W");
    EO      = GetOutputArray("EO");
    
    if(CON_size==0)
	{
		CON_size = 1;
		CON = create_array(1);
		CON[0] = 1;
	}
		
    T_size = input_size*CON_size;

    T       = create_array(T_size);
    T_last  = create_array(T_size);
    O       = create_array(T_size);
}


M02_OFC::~M02_OFC()
{
    destroy_array(T);
    destroy_array(T_last);
    destroy_array(O);
}


void M02_OFC::Tick()
{
    // We need to combine the CS and CON inputs. We do this by populating an
    // array of size CS*CON with, well, CS*CON. Actually the outer product of CS and CON
    // (Morén, 2002, page 123)
    
	for(int i=0; i<CON_size; i++)
	    for(int j=0; j<input_size; j++)
            T[j+input_size*i] = input[j] * CON[i];
    
    // Calulcate sums

    float A_sum = 0;
    for(int i=0;i<A_size; i++)
        A_sum += A[i];
    
	float O_sum = 0;
    for(int i=0;i<T_size; i++)
        O_sum += O[i];

    // Calculate OFC reinforcement (Morén, 2002, page 89 & page 124)
    
    float TRew;
    if(Rew[0] != 0.0)
        TRew = max(0.0, (A_sum-Rew[0])) - O_sum;
    else
        TRew = max(0.0, A_sum - O_sum);

    // Update weights (Morén, 2002, page 88 & page 123)
    
	for(int i=0; i<T_size;i++)
    {
	    W[i] += beta * (T_last[i] * TRew);
        W[i] = clip(W[i], 0, 1); 
	}

    // Calculate output (Morén, 2002, page 123)
    
    EO[0] = 0;
    for(int i=0; i<T_size;i++)
    {
	    O[i] = T[i] * W[i];
	    EO[0] += O[i];	
    }

    // Copy old CS values
    
    copy_array(T_last, T, T_size);
}

