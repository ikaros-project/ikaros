//
//	InputRawImage.cc	This file is a part of the IKAROS project
//					A module for reading from files with raw image data
//
//    Copyright (C) 2001-2002  Christian Balkenius
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

#include "InputRawImage.h"

#include <string.h>



InputRawImage::InputRawImage(Parameter * p):
        Module(p)
{
    // Count no of images/filenames

    file_name = GetValue("filename");

    if (file_name == NULL)
    {
        Notify(msg_fatal_error, "ERROR: No filename(s) supplied.\n");
        return;
    }

    iteration	= 1;
    iterations  = GetIntValue("iterations", 0);

    repeat	= 1;
    repeats	= GetIntValue("repeats", 1);

    cur_image = 0;
    max_images = GetIntValue("filecount", 1);

    size_x  = GetIntValue("size_x", 0);
    size_y  = GetIntValue("size_y", 0);

    if (size_x*size_y < 1)
    {
        Notify(msg_fatal_error, "Image size not supplied.\n");
        return;
    }

    AddOutput("OUTPUT", size_x, size_y);
}



InputRawImage::~InputRawImage()
{
}



Module *
InputRawImage::Create(Parameter * p)
{
    return new InputRawImage(p);
}



void
InputRawImage::Init()
{
    image = GetOutputArray("OUTPUT");
}



void
InputRawImage::Tick()
{
    float scale = 1.0/255.0;
    char fn[256];
    sprintf(fn, file_name, cur_image);
    file = fopen(fn, "rb");

    Notify(msg_verbose, "Reading  \"%s\"\n", fn);

    if (file == NULL)
    {
        Notify(msg_fatal_error, "Could not open image file \"%s\" \n", fn);
        return;
    }

    for (int i=0; i<size_x*size_y; i++)
    {
        unsigned char t;
        fscanf(file, "%c", &t);
        image[i] = scale*float(t);
    }

    fclose(file);

    if (repeat < repeats)
    {
        repeat++;
        return;
    }

    repeat = 1;

    cur_image++;
    if (cur_image >= max_images)
    {
        cur_image = 0 ;
        iteration++;
        Notify(msg_verbose, "InputRawImage: Iteration (%d/%d)\n", iteration-1, iterations);  fflush(NULL);
    }

    if (iterations != 0 && iteration > iterations)
        Notify(msg_terminate);
}



