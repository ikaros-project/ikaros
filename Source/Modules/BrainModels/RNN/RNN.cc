//
//      RNN.cc		This file is a part of the IKAROS project
//				
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


#include "RNN.h"

using namespace ikaros;

void
RNN::Init()
{
    input       =	GetInputArray("INPUT");
    t_input     =	GetInputArray("T-INPUT");
    
    output      =	GetOutputArray("OUTPUT");
    state_out	=	GetOutputArray("STATE-OUT");
    t_output	=	GetInputArray("T-OUTPUT");
    top_down	=	GetInputArray("TOP-DOWN");
    
    aux         =	GetInputArray("AUX");
    t_aux       =	GetInputArray("T-AUX");
    
    size        =   GetInputSize("INPUT");
    aux_size    =   GetInputSize("AUX");
    
    if(t_input && size != GetInputSize("T-INPUT"))
        Notify(msg_fatal_error, "T-INPUT has incorrect size");

    if(t_output && size != GetInputSize("T-OUTPUT"))
        Notify(msg_fatal_error, "T-OUTPUT has incorrect size");

    if(top_down && size != GetInputSize("TOP-DOWN"))
        Notify(msg_fatal_error, "TOP-DOWN has incorrect size");

    if(aux && t_aux && size != GetInputSize("T-AUX"))
        Notify(msg_fatal_error, "T-AUX has incorrect size");
    
    // Allocate memory for weights
    
    w = create_matrix(size, size);
    
    if(aux)
        u = create_matrix(aux_size, size);
}



RNN::~RNN()
{
    destroy_matrix(w);
    destroy_matrix(u);
}



void
RNN::Tick()
{
}



static InitClass init("RNN", &RNN::Create, "Source/Modules/ANN/RNN/");

