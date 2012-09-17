//    M02_Amygdala.cc
//
//    Copyright (C) 2001-2002 Jan Moren
//    
//    This program is free software; you can redistribute it and/or
//    modify
//    it under the terms of the GNU General Public License as published
//    by
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
//    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
//    USA
//
//
// 2001-12-06 Initial version
// 2002-12-20 Moved to IKAROS 0.7
// 2009-06-20 Moved to Ikaros 1.2 (CB)
//
// Implements the basic Amygdala model. Also needed is - ideally - an
// Orbitofrontal model and perhaps a Hippocampus and/or Thalamus.

#include "M02_Amygdala.h"

using namespace ikaros;

void M02_Amygdala::Init()
{
    Bind(alpha, "alpha");
    
    size	= GetInputSize("INPUT");

    input       = GetInputArray("INPUT");
    input_last  = create_array(size);

    Rew		= GetInputArray("Rew");
    EO		= GetInputArray("EO", false); // This input is not required; do not produce error if unconnected
    
    V		= GetOutputArray("V");
    A       = GetOutputArray("A");
    E		= GetOutputArray("E");    

    set_array(V, 0.0, size); // initial weights

    SumA    = 0;
    OldSumA = 0;
    EOld    = 0;
}


M02_Amygdala::~M02_Amygdala()
{
    destroy_array(input_last);
}


void M02_Amygdala::Tick()
{
    // Update weights
    // We use Old SumA to allow Rew to succeed the CS by one timestep
    
    float ARew = max(0.0, (Rew[0] - OldSumA));
    for(int i=0; i<size;i++)
    {
        V[i] += alpha * (input_last[i] * ARew);
        V[i] = min(1.0, V[i]);
    }
    
    // Calculate new activity in nodes A

    SumA = 0;
    for(int i=0; i<size;i++)
        SumA += A[i] = input[i] * V[i];
    
    // Calcaulte emotional output

    E[0] = max(0.0, min(1.0, EOld));    
    if(EO)
        EOld = max(0.0, (OldSumA - EO[0]));
    else
        EOld = max(0.0, OldSumA);

    OldSumA = SumA;

    // Store input
    
    copy_array(input_last, input, size);
}

