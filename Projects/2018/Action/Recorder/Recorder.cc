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
        SetOutputSize("MEASUREMENTS", count);
    }
}



void
Recorder::Init()
{
    start = GetIntValue("start");

    measure_start   = GetIntValue("measure_start");
    measure_end     = GetIntValue("measure_end");
    operation       = GetIntValueFromList("operation"); // FromList should not be necessary ***

    Bind(minimum, "min");
    Bind(maximum, "max");
    
    input       =	GetInputArray("INPUT");
    output      =	GetOutputMatrix("OUTPUT");
    recording   =	GetOutputMatrix("RECORDING");
    measurements=   GetOutputArray("MEASUREMENTS");

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
    
    char * fn2 = create_formatted_string("measurements_%s" ,filename);
    FILE * f2 = fopen(fn2, "w");
    fprintf(f2, "measurement\tvalue\n");
    for(int i=0; i<count; i++)
        fprintf(f2, "%d\t%.4f\n", i, measurements[i]);
    fclose(f2);
}



void
Recorder::Tick()
{
    if(GetTick()<start)
        return;
    
    if(col >= count)
        return;

    output[row][2*col+1] = (float(col+1) * offset - margin)  -  scale*(*input-minimum);
    recording[row][rcol] = *input;
    
    row++;
    
    if(row >= length)
    {
        row = 0;
        col += 1;
        rcol++;
    }
    
    // Just for fun we make the measurements every tick to give a nice animation
    
    for(int i=0; i<count; i++)
    {
        float s = 0;
        for(int j=measure_start; j<measure_end; j++)    // ******** checl range here, included in measurements?
            s += recording[j][i];
        
        if(operation == 1 && measure_end!=measure_start) // average
            s /= float(measure_end-measure_start);
        
        measurements[i] = s;
    }
}



static InitClass init("Recorder", &Recorder::Create, "Source/UserModules/Projects/ActionProject/Recorder/");