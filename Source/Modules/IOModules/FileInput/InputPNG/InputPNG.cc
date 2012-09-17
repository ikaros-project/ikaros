//
//	InputPNG.cc	This file is a part of the IKAROS project
//              A module for reading PNG files
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

#include "InputPNG.h"

#ifdef USE_LIBPNG

#include <string.h>

#define CHECK_BYTES 4

extern "C" {
#include "png.h"
}

#include <setjmp.h>

InputPNG::InputPNG(Parameter * p):
	Module(p)
{
	// Count no of images/filenames
	
	file_name = GetValue("filename");

	if(file_name == NULL)
	{
		Notify(msg_fatal_error, "No filename(s) supplied.\n"); 
		return;
	}

	iteration   = 1;
	iterations  = GetIntValue("iterations", 0);
	cur_image   = 0;
	max_images  = GetIntValue("filecount", 1);

	size_x  = GetIntValue("size_x", 0);
	size_y  = GetIntValue("size_y", 0);

	if(size_x <0 || size_y < 0)
	{
		Notify(msg_fatal_error, "Image sizes must be larger than 0.\n");
		return;
	}

	AddOutput("INTENSITY");
	AddOutput("RED");
	AddOutput("GREEN");
	AddOutput("BLUE");
}



InputPNG::~InputPNG()
{
}



Module *
InputPNG::Create(Parameter * p)
{
	return new InputPNG(p);
}



void
InputPNG::SetSizes()
{

    if (size_x*size_y < 1 ) {
	// We read the first image header to find out sizes and such

	FILE *fp;			// source file
	char fn[256];
	char header[CHECK_BYTES];

	png_structp png_ptr;
	png_infop info_ptr;
	png_infop end_info;
//	png_bytep *row_p;	     

	sprintf(fn, file_name, cur_image);

	// Open and check if it actually is a PNG file

	fp = fopen(fn, "rb");
	if (!fp)
	{
	    Notify(msg_fatal_error, "Could not open image file \"%s\" \n", fn);
	    return;
	}
	fread(header, 1, CHECK_BYTES, fp);

	int is_png = !png_sig_cmp((png_byte *)header, 0, CHECK_BYTES);
	if (!is_png)
	{
	    Notify(msg_fatal_error, "Not a PNG file: \"%s\" \n", fn);
	    return;
	}

	// Read struct. No error handling of our own at this point, so no user
	// functions for it defined.

	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING,
		NULL, NULL, NULL);
	if (png_ptr == NULL)
	{
	    fclose(fp);
	    Notify(msg_fatal_error, "Unable to allocate read struct");
	    return;
	}

	// Image information struct

	info_ptr = png_create_info_struct(png_ptr);
	if (info_ptr == NULL)
	{
	    fclose(fp);
	    png_destroy_read_struct(&png_ptr, NULL, NULL);
	    Notify(msg_fatal_error, "Unable to allocate info struct");
	    return;
	}

	// Postprocess info struct

	end_info = png_create_info_struct(png_ptr);
	if (info_ptr == NULL)
	{
	    fclose(fp);
	    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
	    Notify(msg_fatal_error, "Unable to allocate PP info struct");
	    return;
	}


	// Do the longjmp callback setup in case of read fault

	if (setjmp(png_jmpbuf(png_ptr)))
	{
	    png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
	    fclose(fp);
	    Notify(msg_fatal_error, "Error reading file %s\n", fn);
	    return;
	}

	// Initialize reading
	png_init_io(png_ptr, fp);

	// ..but tell libpng that we've already read a few bytes:
	png_set_sig_bytes(png_ptr, CHECK_BYTES);


	// Read the whole image

	int png_trans = PNG_TRANSFORM_STRIP_16 |   // convert to 8 bit always
	    PNG_TRANSFORM_STRIP_ALPHA |	    // No alpha channel
	    PNG_TRANSFORM_PACKING |		    // Unpack small-bit grayscale
	    PNG_TRANSFORM_EXPAND;		    // make grayscale always full 8 bits, make
						    // indexed RGB to fullcolor (and add an 
						    // alpha channel for transparent pixels, 
						    // but I think STRIP_ALPHA prevents that)


	png_read_png(png_ptr, info_ptr, png_trans, NULL);

	fclose (fp);

	// First some information on the image
	png_uint_32 p_width, p_height; 
	int p_bdepth, p_color_type; // p_channels;
	png_get_IHDR(png_ptr, info_ptr,
		&p_width, &p_height,
		&p_bdepth, 
		&p_color_type,
		NULL, NULL, NULL);

	Notify (msg_verbose, "%s: Width:%d Height:%d Depth:%d Color:%d \n", 
		fn, p_width, p_height, p_bdepth, p_color_type);

	size_x = int(p_width);
	size_y = int(p_height);


	png_destroy_read_struct(&png_ptr, &info_ptr,&end_info);
    }

    SetOutputSize ("INTENSITY", size_x, size_y);
    SetOutputSize ("RED", size_x, size_y);
    SetOutputSize ("GREEN", size_x, size_y);
    SetOutputSize ("BLUE", size_x, size_y);
}



void
InputPNG::Init()
{
	intensity   = GetOutputArray("INTENSITY");
	red	    = GetOutputArray("RED");
	green	    = GetOutputArray("GREEN");
	blue	    = GetOutputArray("BLUE");
}



// A thought: Do we really need to read these "live"? Is there a use for it,
// or should we instead allocate and read the images during initialization? It
// would speed things up quite a bit.

void
InputPNG::Tick()
{
	FILE *fp;			// source file
	char fn[256];
	char header[CHECK_BYTES];
	
	png_structp png_ptr;
	png_infop info_ptr;
	png_infop end_info;
	png_bytep *row_p;	     
	
	sprintf(fn, file_name, cur_image);
	
	// Open and check if it actually is a PNG file
	
	fp = fopen(fn, "rb");
	if (!fp)
	{
	    Notify(msg_fatal_error, "Could not open image file \"%s\" \n", fn);
	    return;
	}
	fread(header, 1, CHECK_BYTES, fp);
	
	int is_png = !png_sig_cmp((png_byte *)header, 0, CHECK_BYTES);
	if (!is_png)
	{
	    Notify(msg_fatal_error, "Not a PNG file: \"%s\" \n", fn);
	    return;
	}

	// Read struct. No error handling of our own at this point, so no user
	// functions for it defined.

	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING,
		NULL, NULL, NULL);
	if (png_ptr == NULL)
	{
	    fclose(fp);
	    Notify(msg_fatal_error, "Unable to allocate read struct");
	    return;
	}

	// Image information struct
	
	info_ptr = png_create_info_struct(png_ptr);
	if (info_ptr == NULL)
	{
	    fclose(fp);
	    png_destroy_read_struct(&png_ptr, NULL, NULL);
	    Notify(msg_fatal_error, "Unable to allocate info struct");
	    return;
	}

	// Postprocess info struct
	
	end_info = png_create_info_struct(png_ptr);
	if (info_ptr == NULL)
	{
	    fclose(fp);
	    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
	    Notify(msg_fatal_error, "Unable to allocate PP info struct");
	    return;
	}

	
	// Do the longjmp callback setup in case of read fault

	if (setjmp(png_jmpbuf(png_ptr)))
	{
	    png_destroy_read_struct(&png_ptr, &info_ptr,
		    &end_info);
	    fclose(fp);
	    Notify(msg_fatal_error, "Error reading file %s\n", fn);
	    return;
	}

	// Initialize reading
	png_init_io(png_ptr, fp);
   
	// ..but tell libpng that we've already read a few bytes:
	png_set_sig_bytes(png_ptr, CHECK_BYTES);
	

	// Read the whole image

	int png_trans = PNG_TRANSFORM_STRIP_16 |	// convert to 8 bit always
		PNG_TRANSFORM_STRIP_ALPHA |		// No alpha channel
		PNG_TRANSFORM_PACKING |		// Unpack small-bit grayscale
		PNG_TRANSFORM_EXPAND;			// make grayscale always full 8 bits, make
							// indexed RGB to fullcolor (and add an 
							// alpha channel for transparent pixels, 
							// but I think STRIP_ALPHA prevents that)

	    
	png_read_png(png_ptr, info_ptr, png_trans, NULL);
		
	fclose (fp);
	
	// The image is read. Let's deal with it
	
   	row_p = png_get_rows(png_ptr, info_ptr);
   
	// First some information on the image
	png_uint_32 p_width, p_height; 
	int p_bdepth, p_color_type, p_channels;
	png_get_IHDR(png_ptr, info_ptr,
		&p_width, &p_height,
		&p_bdepth, 
		&p_color_type,
		NULL, NULL, NULL);
	p_channels = png_get_channels (png_ptr, info_ptr);

	Notify (msg_verbose, "%s: Width:%d Height:%d Depth:%d Color:%d Channels:%d\n", 
		fn, p_width, p_height, p_bdepth, p_color_type, p_channels);
	

	if (p_width != (png_uint_32)size_x || p_height != (png_uint_32)size_y) {

	    png_destroy_read_struct(&png_ptr, &info_ptr,&end_info);
	    Notify(msg_fatal_error, "File %s has wrong size!", fn);
	    return;
	}
	    
	// Grayscale image
	if (p_color_type == PNG_COLOR_TYPE_GRAY) {
	
	    int gidx = 0;   // gray and color indexes
	    for (int row=0; row<(int)p_height; row++) {
		
		for (int col=0; col<(int)p_width; col++) {

		    intensity[gidx]   = float(row_p[row][col]) /255.0;
		    red[gidx]	    = intensity[gidx];
		    green[gidx]	    = intensity[gidx];
		    blue[gidx]	    = intensity[gidx];

		    gidx++;
		}
	    }
	} else {    // Color image (we've stripped alpha channel already)
	    
	    int gidx = 0;   // gray and color indexes
	    for (int row=0; row<(int)p_height; row++) {
		
		for (int col=0; col<(int)(p_width*3); col+=3) {

		    red[gidx]	    = float(row_p[row][col]) /255.0;
		    green[gidx]	    = float(row_p[row][col+1]) /255.0;
		    blue[gidx]	    = float(row_p[row][col+2]) /255.0;
		    intensity[gidx] = red[gidx]*0.24 + green[gidx]*0.68 + blue[gidx]*0.08; 

		    gidx++;
		}
	    }
	    
	}

	cur_image++;

	if(cur_image >= max_images)
	{
		cur_image = 0 ;
		iteration++;
		printf("InputPNG: Repeating (%ld/%ld)\n", iteration, iterations);
	}
	
	if(iterations != 0 && iteration > iterations)
		Notify(msg_terminate);
	
	png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
}

#endif
