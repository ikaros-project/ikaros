//
//	OutputJPEG.cc		This file is a part of the IKAROS project
//					A module for writing JPEG images to files
//
//    Copyright (C) 2005  Christian Balkenius
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

#include "OutputJPEG.h"
#include <string.h>

//#include <math.h>
#include <setjmp.h>

extern "C"
{
#include "jpeglib.h"
}



Module *
OutputJPEG::Create(Parameter * p)
{
    return new OutputJPEG(p);
}



OutputJPEG::OutputJPEG(Parameter * p):
        Module(p)
{
    scale	 	= GetFloatValue("scale", 1.0);
    file_name		= GetValue("filename");
    supress		= GetIntValue("supress", 1);
    offset	 	= GetIntValue("offset", 0);

    quality		= GetIntValue("quality", 100);

    if (file_name == NULL)
    {
        Notify(msg_fatal_error, "No filename(s) supplied.\n");
        return;
    }

    cur_image = 0;

    AddInput("WRITE");

    AddInput("INTENSITY");

    AddInput("RED");
    AddInput("GREEN");
    AddInput("BLUE");
}



OutputJPEG::~OutputJPEG()
{}



void
OutputJPEG::Init()
{
    if(InputConnected("INTENSITY"))
        writesig = GetInputArray("INTENSITY");

    if(InputConnected("INTENSITY"))
    {
        input_intensity = GetInputMatrix("INTENSITY");
        size_x = GetInputSizeX("INTENSITY");
        size_y = GetInputSizeY("INTENSITY");
    }

    else
    {
        input_red = GetInputMatrix("RED");
        input_green = GetInputMatrix("GREEN");
        input_blue = GetInputMatrix("BLUE");
        size_x = GetInputSizeX("RED");
        size_y = GetInputSizeY("RED");

        // Check connection consistency
    }
}



void
OutputJPEG::WriteGrayJPEG(FILE * fileref, float ** image)
{
    // Calculate max and min of image

    float maximum = image[0][0];
    float minimum = image[0][0];

    for (int j=0; j<size_y; j++)
        for (int i=0; i<size_x; i++)
            if (image[j][i] > maximum)
                maximum = image[j][i];
            else if (image[j][i] < minimum)
                minimum = image[j][i];

    if (minimum < 0 || maximum > 1)
    {
        Notify(msg_warning, "WARNING: OutputJPEG - Gray levels not within range [0..1]. Will be scaled.");
    }
    else
    {
        minimum = 0.0;
        maximum = 1.0;
    }

    JSAMPLE * image_buffer = new JSAMPLE [size_x];

    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;

    JSAMPROW row_pointer[1];	/* pointer to JSAMPLE row[s] */
    //int row_stride;				/* physical row width in image buffer */

    cinfo.err = jpeg_std_error(&jerr);

    jpeg_create_compress(&cinfo);	// Replace with ikaros error handler later

    jpeg_stdio_dest(&cinfo, fileref);

    // Set compression parameters

    cinfo.image_width = size_x; 	/* image width and height, in pixels */
    cinfo.image_height = size_y;
    cinfo.input_components = 1;		/* # of color components per pixel */
    cinfo.in_color_space = JCS_GRAYSCALE;

    jpeg_set_defaults(&cinfo);
    jpeg_set_quality(&cinfo, quality, true);

    // Do the compression

    jpeg_start_compress(&cinfo, true);

    //row_stride = size_x * 1;	/* JSAMPLEs per row in image_buffer */
    int j=0;

    while (cinfo.next_scanline < cinfo.image_height)
    {
        // Convert row to image buffer (assume max == 1 for now)
        if (maximum-minimum != 0)
            for (int i=0; i<size_x; i++)
                image_buffer[i] = int(255.0*(image[j][i]-minimum)/(maximum-minimum));
        else
            for (int i=0; i<size_x; i++)
                image_buffer[i] = 0;

        // Write to compressor
        row_pointer[0] = image_buffer; // & image_buffer[cinfo.next_scanline * row_stride];
        (void) jpeg_write_scanlines(&cinfo, row_pointer, 1);
        j++;
    }

    jpeg_finish_compress(&cinfo);
    jpeg_destroy_compress(&cinfo);

    delete [] image_buffer;
}



void
OutputJPEG::WriteRGBJPEG(FILE * fileref, float ** r, float ** g, float ** b)
{
    bool clipping = false;

    JSAMPLE * image_buffer = new JSAMPLE [3*size_x];

    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;

    JSAMPROW row_pointer[1];	/* pointer to JSAMPLE row[s] */
    //int row_stride;		/* physical row width in image buffer */

    cinfo.err = jpeg_std_error(&jerr);

    jpeg_create_compress(&cinfo);	// Replace with ikaros error ahdnler later

    jpeg_stdio_dest(&cinfo, fileref);

    // Set compression parameters

    cinfo.image_width = size_x; 	/* image width and height, in pixels */
    cinfo.image_height = size_y;
    cinfo.input_components = 3;		/* # of color components per pixel */
    cinfo.in_color_space = JCS_RGB; 	/* colorspace of input image */ // JCS_GRAYSCALE

    jpeg_set_defaults(&cinfo);
    jpeg_set_quality(&cinfo, quality, TRUE);

    // Do the compression

    jpeg_start_compress(&cinfo, TRUE);

    //row_stride = size_x * 3;		/* JSAMPLEs per row in image_buffer */
    int j=0;

    while (cinfo.next_scanline < cinfo.image_height)
    {
        int x = 0;
        for (int i=0; i<size_x; i++)
        {
            float rr = (r[j][i] < 0.0 ? 0.0 : (r[j][i] > 1.0 ? 1.0 : r[j][i]));
            float gg = (g[j][i] < 0.0 ? 0.0 : (g[j][i] > 1.0 ? 1.0 : g[j][i]));
            float bb = (b[j][i] < 0.0 ? 0.0 : (b[j][i] > 1.0 ? 1.0 : b[j][i]));

            clipping |= (rr != r[j][i]) || (gg != g[j][i]) || (bb != b[j][i]);

            image_buffer[x++] = int(255.0*rr);
            image_buffer[x++] = int(255.0*gg);
            image_buffer[x++] = int(255.0*bb);
        }

        // Write to compressor
        row_pointer[0] = image_buffer; // & image_buffer[cinfo.next_scanline * row_stride];
        (void) jpeg_write_scanlines(&cinfo, row_pointer, 1);
        j++;
    }
    jpeg_finish_compress(&cinfo);
    jpeg_destroy_compress(&cinfo);

    delete [] image_buffer;

    if (clipping)
        Notify(msg_warning, "OutputJPEG - some color values were not in the required range 0..1. These values werre clipped.");
}



void
OutputJPEG::Tick()
{
    char fn[256];
    sprintf(fn, file_name, offset + cur_image);

    // If we are using a gating signal from outside, we're looking for it here
    
    if ((writesig != NULL) && (!(writesig[0] > 0.0))) {

        Notify(msg_verbose, " Write signal suppression: \"%s\" (%dx%d)\n", fn, size_x, size_y);
        cur_image++;
        return;
    }
 

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

    if (input_intensity != NULL)
        WriteGrayJPEG(file, input_intensity);
    else if (input_red != NULL)
        WriteRGBJPEG(file, input_red, input_green, input_blue);

    fclose(file);

    cur_image++;
}



