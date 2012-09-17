//
//	OutputPNG.cc		This file is a part of the IKAROS project
//				A module for writing PNG images to files
//
//    Copyright (C) 2007  Jan Moren
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

#include "OutputPNG.h"
#include <string.h>

#ifdef USE_LIBPNG


extern "C" {
#include "png.h"
}

#include <math.h>
#include <setjmp.h>


    Module *
OutputPNG::Create(Parameter * p)
{
    return new OutputPNG(p);
}



OutputPNG::OutputPNG(Parameter * p):
    Module(p)
{
    file_name		= GetValue("filename");
    supress		= GetIntValue("supress", 1);
    offset	 	= GetIntValue("offset", 0);


    if(file_name == NULL)
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



OutputPNG::~OutputPNG()
{
}



void
OutputPNG::Init()
{
    if(InputConnected("WRITE"))
        writesig = GetInputArray("WRITE");

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
OutputPNG::WriteGrayPNG(FILE * fileprt, float ** image)
{
    // Calculate max and min of image

    float maximum = image[0][0];
    float minimum = image[0][0];

    for(int j=0; j<size_y; j++)
	for(int i=0; i<size_x; i++)
	    if(image[j][i] > maximum)
		maximum = image[j][i];
	    else if(image[j][i] < minimum)
		minimum = image[j][i];

    if(minimum < 0 || maximum > 1)
    {
        Notify(msg_warning, "WARNING: OutputPNG - Gray levels not within range [0..1]. Will be scaled.");
    }
    else
    {
        minimum = 0.0;
        maximum = 1.0;
    }

    // Beginning of the PNG-specific parts. Note how all other parts of this
    // module are almost identical to the corresponding JPEG writer. This is by
    // design.

    png_structp png_ptr = png_create_write_struct
	(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr){
        Notify (msg_fatal_error, "Could not allocate PNG write struct\n");
	return;
    }

    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr)
    {
        png_destroy_write_struct(&png_ptr,
		(png_infopp)NULL);
	Notify (msg_fatal_error, "Could not allocate PNG info struct\n");
	return;
    }

    // Default error handling

    if (setjmp(png_jmpbuf(png_ptr)))
    {
        png_destroy_write_struct(&png_ptr, &info_ptr);
        fclose(fileprt);
        Notify (msg_fatal_error, "Abnormal abort on PNG write\n");
        return;
    }


    png_init_io(png_ptr, fileprt);

    // Here we wstart setting the actual image parameters. Up until now
    // it's been identical code for all PNG writing variations.

    png_set_IHDR(png_ptr, info_ptr, 
	    size_x, size_y,				// Size of image, in pixels
	    8,					// Bit depth of one channel 
	    PNG_COLOR_TYPE_GRAY,			// color type 
	    PNG_INTERLACE_NONE,
	    PNG_COMPRESSION_TYPE_DEFAULT,
	    PNG_FILTER_TYPE_DEFAULT);

    // There's _lots_ of additional info and parameters that could be set,
    // like gamma, colorspace and rendering intent, ICC device profile,
    // comments and so on. We don't need any of it, but be aware that such
    // things are possible if needed.


    // As an example, write a default gamma value
    // png_set_gAMA(png_ptr, info_ptr, 1.8);		// Wild guess


    // This writes all data coming before the image itself. It is possible
    // to add more data after the image if needed.

    png_write_info(png_ptr, info_ptr);

    png_byte *image_buffer = new png_byte [size_x*size_y];
    png_byte **row_pointer = new png_byte * [size_y];   // pointer to rows

    int row_stride = size_x * 1;	// Make it easier to cut and paste

    int rpos = 0;
    for (int j=0; j<size_y; j++) {

	rpos = j*row_stride;
	row_pointer[j] = &image_buffer[rpos];

	if(maximum-minimum != 0) {
	    for(int i=0; i<size_x; i++) {

		image_buffer[rpos+i] = int(255.0*(image[j][i]-minimum)/(maximum-minimum));    
	    }
	} else {
	    for(int i=0; i<size_x; i++) {

		image_buffer[rpos+i] = 0;
	    }
	}
    }

    png_write_image(png_ptr, row_pointer);
    png_write_end(png_ptr, info_ptr);
    png_destroy_write_struct(&png_ptr, &info_ptr);

    delete image_buffer;
    delete row_pointer;
}



void
OutputPNG::WriteRGBPNG(FILE * fileprt, float ** r, float ** g, float ** b)
{

    bool clipping = false;
 
    // This is almost verbatim cut and pasted from above. At some point it may
    // make sense to refactor this.

    png_structp png_ptr = png_create_write_struct
	(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr){
	Notify (msg_fatal_error, "Could not allocate PNG write struct\n");
	return;
    }

    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr)
    {
	png_destroy_write_struct(&png_ptr,
		(png_infopp)NULL);
	Notify (msg_fatal_error, "Could not allocate PNG info struct\n");
	return;
    }

    // Default error handling

    if (setjmp(png_jmpbuf(png_ptr)))
    {
	png_destroy_write_struct(&png_ptr, &info_ptr);
	fclose(fileprt);
	Notify (msg_fatal_error, "Abnormal abort on PNG write\n");
	return;
    }


    png_init_io(png_ptr, fileprt);

    // Here we wstart setting the actual image parameters. Up until now
    // it's been identical code for all PNG writing variations.

    png_set_IHDR(png_ptr, info_ptr, 
	    size_x, size_y,				// Size of image, in pixels
	    8,						// Bit depth of one channel 
	    PNG_COLOR_TYPE_RGB,				// color type 
	    PNG_INTERLACE_NONE,
	    PNG_COMPRESSION_TYPE_DEFAULT,
	    PNG_FILTER_TYPE_DEFAULT);

    // There's _lots_ of additional info and parameters that could be set,
    // like gamma, colorspace and rendering intent, ICC device profile,
    // comments and so on. We don't need any of it, but be aware that such
    // things are possible if needed.


    // As an example, write a default gamma value
    // png_set_gAMA(png_ptr, info_ptr, 1.8);		// Wild guess


    // This writes all data coming before the image itself. It is possible
    // to add more data after the image if needed.

    png_write_info(png_ptr, info_ptr);

    png_byte *image_buffer = new png_byte [size_x*size_y*3];
    png_byte **row_pointer = new png_byte * [size_y];	// pointer to rows

    int row_stride = size_x * 3;	// Make it easier to cut and paste

    int rpos = 0;
    int ppos = 0;
    for (int j=0; j<size_y; j++) {

	rpos = j*row_stride;
	row_pointer[j] = &image_buffer[rpos];

	for(int i=0; i<size_x; i++) {

	    ppos = rpos + i*3;
	    float rr = (r[j][i] < 0.0 ? 0.0 : (r[j][i] > 1.0 ? 1.0 : r[j][i]));
	    float gg = (g[j][i] < 0.0 ? 0.0 : (g[j][i] > 1.0 ? 1.0 : g[j][i]));
	    float bb = (b[j][i] < 0.0 ? 0.0 : (b[j][i] > 1.0 ? 1.0 : b[j][i]));
	
	    clipping |= (rr != r[j][i]) || (gg != g[j][i]) || (bb != b[j][i]);

	    image_buffer[ppos++] = int(255.0*rr);    
	    image_buffer[ppos++] = int(255.0*gg);    
	    image_buffer[ppos] = int(255.0*bb);    
	}
    }

    png_write_image(png_ptr, row_pointer);
    png_write_end(png_ptr, info_ptr);
    png_destroy_write_struct(&png_ptr, &info_ptr);

    delete image_buffer;
    delete row_pointer;

    if(clipping)
	Notify(msg_warning, "OutputPNG - some color values were not in the required range 0..1. These values werre clipped.");
}



void
OutputPNG::Tick()
{
    char fn[256];
    sprintf(fn, file_name, offset + cur_image);

    // If we are using a gating signal from outside, we're looking for it here
    if ((writesig != NULL) && (!writesig[0] > 0.0)) {

	Notify(msg_verbose, " Write signal suppression: \"%s\" (%dx%d)\n", fn, size_x, size_y);
	cur_image++;
	return;
    }
	    
	
    if(supress > cur_image)
    {
	Notify(msg_verbose, "Supressing write of \"%s\" (%dx%d)\n", fn, size_x, size_y);
	cur_image++;
	return;
    }

    file = fopen(fn, "wb");

    Notify(msg_verbose, "Writing \"%s\" (%dx%d)\n", fn, size_x, size_y);

    if(file == NULL)
    {
	Notify(msg_fatal_error, "Could not open image file \"%s\" \n", fn);
	return;
    }

    if(input_intensity != NULL)
	WriteGrayPNG(file, input_intensity);
    else if(input_red != NULL)
	WriteRGBPNG(file, input_red, input_green, input_blue);

    fclose(file);

    cur_image++;
}

#endif

