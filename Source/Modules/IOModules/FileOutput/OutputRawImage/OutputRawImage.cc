//
//	OutputRawImage.cc		This file is a part of the IKAROS project
//						A module for writing raw image data to files
//
//    Copyright (C) 2001-2011  Christian Balkenius
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

#include "OutputRawImage.h"


using namespace ikaros;


Module *
OutputRawImage::Create(Parameter * p)
{
    return new OutputRawImage(p);
}



OutputRawImage::OutputRawImage(Parameter * p):
        Module(p)
{
    scale	 	= GetFloatValue("scale");
    file_name	= GetValue("filename");
    supress 	= GetIntValue("supress");
    offset	 	= GetIntValue("offset");

    if (file_name == NULL)
    {
        Notify(msg_fatal_error, "No filename(s) supplied.\n");
        return;
    }

    cur_image = 0;

    AddInput("INPUT");
}



OutputRawImage::~OutputRawImage()
{
}



void
OutputRawImage::Init()
{
    image = GetInputArray("INPUT");
    size_x = GetInputSizeX("INPUT");
    size_y = GetInputSizeY("INPUT");
}



void
OutputRawImage::Tick()
{
    char fn[256];
    sprintf(fn, file_name, offset + cur_image);

    if (supress > cur_image)
    {
        Notify(msg_verbose, "Supressing write of \"%s\" (%dx%d)\n", fn, size_x, size_y);
        cur_image++;
        return;
    }

    file = fopen(fn, "wb");

    Notify(msg_verbose, "Writing \"%s\" (%dx%d)\n", fn, size_x, size_y);

    if (file == NULL)
    {
        Notify(msg_fatal_error, "Could not open image file \"%s\" \n", fn);
        return;
    }

    for (int i=0; i<size_x*size_y; i++)
        fputc(clip(scale*image[i], 0, 255), file);

    fclose(file);

    cur_image++;
}



