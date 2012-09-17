//
//	InputJPEG.cc	This file is a part of the IKAROS project
//				A module for reading from JPEG files
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

#include "InputJPEG.h"

#ifdef USE_LIBJPEG

#include <string.h>

extern "C"
{
#include "jpeglib.h"
}

#include <setjmp.h>


struct my_error_mgr
{
    struct jpeg_error_mgr pub;	/* "public" fields */
    jmp_buf setjmp_buffer;		/* for return to caller */
};


typedef struct my_error_mgr * my_error_ptr;


static void
my_error_exit (j_common_ptr cinfo)
{
    my_error_ptr myerr = (my_error_ptr) cinfo->err;
    (*cinfo->err->output_message) (cinfo);
    longjmp(myerr->setjmp_buffer, 1);
}



InputJPEG::InputJPEG(Parameter * p):
        Module(p)
{
    // Count no of images/filenames

    file_name = GetValue("filename");

    if (file_name == NULL)
    {
        Notify(msg_fatal_error, "No filename(s) supplied.\n");
        return;
    }

    iteration	= 1;
    iterations  = GetIntValue("iterations", 0);
    cur_image = 0;
    max_images = GetIntValue("filecount", 1);

    read_once = GetBoolValue("read_once", true);
    if (strstr(file_name, "%") != NULL)
        read_once = false;
    first = true;

    size_x  = 0;
    size_y  = 0;

    if (!GetImageSize(size_x, size_y) || size_x*size_y < 1)
    {
        Notify(msg_fatal_error, "Image size could not be found in the file.\n");
        return;
    }

    AddOutput("INTENSITY", size_x, size_y);
    AddOutput("RED", size_x, size_y);
    AddOutput("GREEN", size_x, size_y);
    AddOutput("BLUE", size_x, size_y);
}



InputJPEG::~InputJPEG()
{}



Module *
InputJPEG::Create(Parameter * p)
{
    return new InputJPEG(p);
}



bool
InputJPEG::GetImageSize(int & x, int & y)
{
    struct jpeg_decompress_struct cinfo;
    struct my_error_mgr jerr;
    FILE * infile;
    char fn[256];
    sprintf(fn, file_name, cur_image);

    if ((infile = fopen(fn, "rb")) == NULL)
    {
        Notify(msg_fatal_error, "Could not open image file \"%s\" \n", fn);
        return false;
    }

    cinfo.err = jpeg_std_error(&jerr.pub);
    jerr.pub.error_exit = my_error_exit;

    if (setjmp(jerr.setjmp_buffer))
    {
        jpeg_destroy_decompress(&cinfo);
        fclose(infile);
        return false;
    }

    jpeg_create_decompress(&cinfo);
    jpeg_stdio_src(&cinfo, infile);
    (void) jpeg_read_header(&cinfo, TRUE);
    (void) jpeg_start_decompress(&cinfo);

    Notify(msg_verbose, "Checking \"%s\"  (width = %d height = %d channels = %d)\n", fn, cinfo.output_width, cinfo.output_height, cinfo.output_components);

    x  = cinfo.output_width;
    y  = cinfo.output_height;

    fclose(infile);

    return true;
}



void
InputJPEG::Init()
{
    intensity	= GetOutputArray("INTENSITY");
    red         = GetOutputArray("RED");
    green       = GetOutputArray("GREEN");
    blue		= GetOutputArray("BLUE");
}



void
InputJPEG::Tick()
{
    if (first || !read_once)
    {
        first = false;

        struct jpeg_decompress_struct cinfo;
        struct my_error_mgr jerr;
        FILE * infile;				/* source file */
        JSAMPARRAY buffer;			/* Output row buffer */
        int row_stride;				/* physical row width in output buffer */
        char fn[256];
        sprintf(fn, file_name, cur_image);

        if ((infile = fopen(fn, "rb")) == NULL)
        {
            Notify(msg_fatal_error, "Could not open image file \"%s\" \n", fn);
            return;
        }

        cinfo.err = jpeg_std_error(&jerr.pub);
        jerr.pub.error_exit = my_error_exit;

        if (setjmp(jerr.setjmp_buffer))
        {
            jpeg_destroy_decompress(&cinfo);
            fclose(infile);
            return;
        }

        jpeg_create_decompress(&cinfo);
        jpeg_stdio_src(&cinfo, infile);
        (void) jpeg_read_header(&cinfo, TRUE);
        (void) jpeg_start_decompress(&cinfo);
        row_stride = cinfo.output_width * cinfo.output_components;
        Notify(msg_verbose, "InputJPEG: width = %d height = %d components = %d\n", cinfo.output_width, cinfo.output_height, cinfo.output_components);

        if (cinfo.output_width != (unsigned int)(size_x) ||  cinfo.output_height != (unsigned int)(size_y))
        {
            Notify(msg_fatal_error, "Image \"%s\" has incorrect size\n", fn);
            fclose(infile);
            return;
        }

        buffer = (*cinfo.mem->alloc_sarray)
                 ((j_common_ptr) &cinfo, JPOOL_IMAGE, row_stride, 1);

        if (cinfo.output_components == 1) // Gray Scale Image
            while (cinfo.output_scanline < cinfo.output_height)
            {
                //	printf("scanline = %d\n", cinfo.output_scanline);
                (void) jpeg_read_scanlines(&cinfo, buffer, 1);
                for (int i=0; i<size_x; i++) // CHECK BOUNDS
                {
                    int ix = size_x*(cinfo.output_scanline-1) + i;
                    intensity[ix]	= float(buffer[0][i]) /255.0;
                    red[ix]		= intensity[ix];
                    green[ix]		= intensity[ix];
                    blue[ix]		= intensity[ix];
                }
            }

        else // RGB Color Image
        {
            float c255 = 1.0/255.0;
            float c3 = 1.0/3.0;
            while (cinfo.output_scanline < cinfo.output_height)
            {
                (void) jpeg_read_scanlines(&cinfo, buffer, 1);
                unsigned char * buf = buffer[0];
                int ix = size_x*(cinfo.output_scanline-1);
                float * r = &red[ix];
                float * g = &green[ix];
                float * b = &blue[ix];
                float * iy = &intensity[ix];
                for (int i=0; i<size_x; i++) // CHECK BOUNDS
                {
                    *r		= c255*float(*buf++);
                    *g		= c255*float(*buf++);
                    *b		= c255*float(*buf++);
                    *iy++	= c3*((*r++)+(*g++)+(*b++));	// Do this correctly later!!!
                }
            }

        }

        (void) jpeg_finish_decompress(&cinfo);
        jpeg_destroy_decompress(&cinfo);
        fclose(infile);

        /* At this point you may want to check to see whether any corrupt-data
         * warnings occurred (test whether jerr.pub.num_warnings is nonzero).
         */
    }

    cur_image++;
    if (cur_image >= max_images)
    {
        cur_image = 0 ;
        iteration++;
//		printf("InputJPEG: Repeating (%ld/%ld)\n", iteration, iterations);
    }

    if (iterations != 0 && iteration > iterations)
        Notify(msg_terminate);
}

#endif
