// FIXME: Should use code in image_file_format later on

#include <string.h>
#include <setjmp.h>
extern "C"
{
#include <stdio.h>
#include "jpeglib.h"
}

#include "ikaros.h"

using namespace ikaros;



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



class InputJPEG: public Module
{
public:
    parameter	iterations;
    parameter	iteration;
    parameter   filecount;
    int	        cur_image;
    parameter	read_once;
    bool	    first;
    parameter	filename;


    parameter	size_x;
    parameter	size_y;

    matrix	    intensity;
    matrix	    red;
    matrix	    green;
    matrix	    blue;

    FILE *	    file;


    bool
    GetImageSize(int & x, int & y)
    {
        Bind(filename, "filename");

        struct jpeg_decompress_struct cinfo;
        struct my_error_mgr jerr;
        FILE * infile;
        char fn[256];
        snprintf(fn, 256, std::string(filename).c_str(), cur_image);

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

        Notify(msg_debug, u8"Checking \""+std::string(fn)+"\" (width = "+std::to_string(cinfo.output_width)+" height = "+std::to_string(cinfo.output_height)+" channels = "+std::to_string(cinfo.output_components)+")");

        x  = cinfo.output_width;
        y  = cinfo.output_height;

        fclose(infile);

        return true;
    }



    void
    SetParameters() // Getimage size from JPEG file
    {
        Bind(size_x, "size_x");
        Bind(size_y, "size_y");

        int sx, sy;
        if (!GetImageSize(sx, sy) || sx*sy < 1)
        {
            Notify(msg_fatal_error, "Image size could not be found in the file.\n");
            return;
        }
        size_x = sx;
        size_y = sy;
    }


 
    void
    Init()
    {
        // Count no of images/filenames

        Bind(filename, "filename");

        if (std::string(filename).empty())
        {
            Notify(msg_fatal_error, "No filename(s) supplied.\n");
            return;
        }

        iteration	= 1;
        cur_image = 0;

        Bind(iterations , "iterations");
        Bind(filecount, "filecount");
        Bind(read_once,"read_once");

        if (strstr(std::string(filename).c_str(), "%") != NULL)
            read_once = false;
        first = true;

        Bind(intensity, "INTENSITY");
        Bind(red, "RED");
        Bind(green, "GREEN");
        Bind(blue, "BLUE");
    }



    void
    Tick()
    {
        if (first || !read_once)
        {
            first = false;

            struct jpeg_decompress_struct cinfo;
            struct my_error_mgr jerr;
            FILE * infile;				// source file
            JSAMPARRAY buffer;			// Output row buffer 
            int row_stride;				// physical row width in output buffer 
            char fn[256];
            snprintf(fn, 256, filename.c_str(), cur_image);

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
            Notify(msg_debug, u8"InputJPEG: width = "+std::to_string(cinfo.output_width)+" height = "+std::to_string(cinfo.output_height)+" components = "+std::to_string(cinfo.output_components)+"\n");

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
                        intensity.data()[ix]	= float(buffer[0][i]) /255.0;
                        red.data()[ix]		    = intensity.data()[ix];
                        green.data()[ix]        = intensity.data()[ix];
                        blue.data()[ix]		    = intensity.data()[ix];
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
                    float * r = &red.data()[ix];
                    float * g = &green.data()[ix];
                    float * b = &blue.data()[ix];
                    float * iy = &intensity.data()[ix];
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

            // At this point you may want to check to see whether any corrupt-data
            // warnings occurred (test whether jerr.pub.num_warnings is nonzero).

        }

        cur_image++;
        if (cur_image >= filecount)
        {
            cur_image = 0 ;
            iteration = iteration + 1;
    //		printf("InputJPEG: Repeating (%ld/%ld)\n", iteration, iterations);
        }

        if (iterations != 0 && iteration > iterations)
            Notify(msg_terminate, "End of image sequence");

    }
};

INSTALL_CLASS(InputJPEG)


