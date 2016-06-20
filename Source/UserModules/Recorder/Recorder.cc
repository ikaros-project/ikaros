//
//		Recorder.cc		This file is a part of the IKAROS project
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


#include "Recorder.h"

using namespace ikaros;


void
Recorder::SetSizes()
{
    length = GetIntValue("length");
    count  = GetIntValue("count");
    if (length != unknown_size && count != unknown_size)
    {
        SetOutputSize("OUTPUT", 2*count, length);
        SetOutputSize("RECORDING", count, length);
    }
}



void
Recorder::Init()
{
    start = GetIntValue("start");

    Bind(minimum, "min");
    Bind(maximum, "max");
    

    input       =	GetInputArray("INPUT");
    output      =	GetOutputMatrix("OUTPUT");
    recording   =	GetOutputMatrix("RECORDING");
    
    row = 0;
    col = 0;
    rcol = 0;

    float mrgn = 0.1;
    
    scale = (1.0-mrgn)/((maximum-minimum)*float(count));
    offset = 1.0/float(count);
    margin = 0.5*mrgn/float(count);
    increment = 1.0/float(length);

    for(int i=0; i<count; i++)
        for(int j=0; j<length; j++)
        {
            output[j][2*i] = float(j) * increment;
            output[j][2*i+1] = float(i+1) * offset - margin;
        }
}



Recorder::~Recorder()
{
    const char * filename = GetValue("filename");

    if(!filename)
        return;

    FILE * f = fopen(filename, "w");
    
    char s[2] = "A";
        fprintf(f, "t");
    for(int i=0; i<count; i++)
    {
        fprintf(f, "\t%s", s);
        s[0]++;
    }
    fprintf(f, "\n");

    for(int j=0; j<length; j++)
    {
        fprintf(f, "%d", j);
        for(int i=0; i<count; i++)
            fprintf(f, "\t%f", recording[j][i]);
        fprintf(f, "\n");
    }
    
    fclose(f);
}



void
Recorder::Tick()
{
    if(GetTick()<start)
        return;
    
    if(col >= count)
        return;

//    output[row][col+1] = float(col+2)*offset + scale*(*input);

    output[row][2*col+1] = (float(col+1) * offset - margin)  -  scale*(*input-minimum);
    recording[row][rcol] = *input;
    
    row++;
    
    if(row >= length)
    {
        row = 0;
        col += 1;
        rcol++;
    }
}



static InitClass init("Recorder", &Recorder::Create, "Source/UserModules/Recorder/");

