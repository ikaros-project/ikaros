//
//	  Select.cc     This file is a part of the IKAROS project
//
//
//    Copyright (C) 2015  Christian Balkenius
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


#include "Select.h"

using namespace ikaros;


void
Select::SetSizes()
{
    selection_indices = GetMatrix("select", selection_x, selection_y);
    if(selection_y == 1)
        SetOutputSize("OUTPUT", selection_x);
    else
        SetOutputSize("OUTPUT", selection_y);
}



void
Select::Init()
{
    size_x	= GetInputSizeX("INPUT");
    size_y	= GetInputSizeY("INPUT");

    input   = GetInputMatrix("INPUT");
    
    output	= GetOutputArray("OUTPUT");
}



void
Select::Tick()
{
    if(selection_y == 1)
    {
        for(int i=0; i<selection_x; i++)
        {
            int x = int(selection_indices[0][i]);
            if(0 <= x && x < size_x)
                output[i] = input[0][x];
            else
                Notify(msg_warning, "Selection index out of range (%d).", x);            
        }
    }
    
    else
    {
        for(int i=0; i<selection_y; i++)
        {
            int x = int(selection_indices[i][0]);
            int y = int(selection_indices[i][1]);
            if(0 <= x && x < size_x && 0 <= y && y < size_y)
                output[i] = input[y][x];
            else
                Notify(msg_warning, "Selection indices out of range (%d, %d).", x, y);
        }
    }
}


static InitClass init("Select", &Select::Create, "Source/Modules/UtilityModules/Select/");

