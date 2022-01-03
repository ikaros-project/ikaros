//
//	IKAROS_Image_Processing.cc		Various math functions for IKAROS
//
//    Copyright (C) 2022  Christian Balkenius
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
//    See http://www.ikaros-project.org/ for more information.
//

#include "IKAROS_System.h"
#include "IKAROS_Math.h"
#include "IKAROS_Utils.h"
#include "IKAROS_Image_Processing.h"


// includes for JPEG

#include <stdio.h>
extern "C"
{
#include "jpeglib.h"
#include <setjmp.h>
}

#ifdef MAC_OS_X
#include <Accelerate/Accelerate.h>
#endif

#ifdef LINUX
//#define	MAXFLOAT	3.402823466e+38f
#ifdef USE_BLAS
extern "C"
{
#include "gsl/gsl_blas.h"
}
#endif
#endif

namespace ikaros
{


// image processing
    // MARK: -
    // MARK: image processing
    
    float **
    im2row(float ** result, float ** source, int result_size_x, int result_size_y, int source_size_x, int source_size_y, int kernel_size_x, int kernel_size_y, int stride_x, int stride_y)  // mode = 'sliding'
    {
        float * r = *result;
        for(int j=0; j<result_size_y; j++)
            for(int i=0; i<result_size_x; i++)
            {
                int s[kernel_size_y];
                for(int k=0; k<kernel_size_y; k++)
                    s[k] = (j*stride_y+k)*source_size_x + i*stride_x;
                for(int k=0; k<kernel_size_y; k++)
                    for(int v=0; v<kernel_size_x; v++)
                        *r++ = (*source)[s[k]++];
            }

        return result;
    }


    float** spanned_im2row(
        float ** result, 
        float ** source, 
        int map_size_x, 
        int map_size_y, 
        int source_size_x, 
        int source_size_y, 
        int kernel_size_x, 
        int kernel_size_y, 
        int stride_x, 
        int stride_y,
        int block_x,
        int block_y,
        int span_x,
        int span_y)  // mode = 'sliding'
    {
        float * r = *result;
        int r_ix = 0;
        for(int j=0; j<map_size_y; j++)
            for(int i=0; i<map_size_x; i++)
            {
                int s[kernel_size_y];
                // for rows
                for(int k=0; k<kernel_size_y; k++){
                    int dv = k/block_y;
                    int offset = dv*span_y;
                    s[k] = (j*stride_y+k+offset)*source_size_x + i*stride_x;
                }
                for(int k=0; k<kernel_size_y; k++)
                    for(int v=0; v<kernel_size_x; v++){
                        int dv = v/block_x;
                        int offset = dv*span_x;
                        int s_ix = offset + s[k]++;
                        r[r_ix++] = (*source)[s_ix];
                    }
            }
        return result;
    }

    float ** spanned_row2im(float **out, float **in, 
        int out_x, int out_y,
        int map_x, int map_y,
        int rf_x, int rf_y,
        int inc_x, int inc_y,
        int blk_x, int blk_y,
        int spn_x, int spn_y)
    {
        int *s = (int*)calloc(rf_y, sizeof(int));
        float *o = *out;

        int r_ix = 0;
        for (int j = 0; j < map_y; ++j)
            for (int i = 0; i < map_x; ++i)
            {
                memset(s, 0, rf_y);
                for (int k = 0; k < rf_y; ++k)
                {
                    int dv = k / blk_y;
                    int offset = dv * spn_y;
                    s[k] = (j*inc_y + k + offset) * out_x + 
                        i * inc_x;
                }
                for (int k = 0; k < rf_y; ++k)
                    for (int v = 0; v < rf_x; ++v)
                    {
                        int dv = v / blk_x;
                        int offset = dv * spn_x;
                        int s_ix = offset + s[k];
                        o[s_ix] += (*in)[r_ix];
                        r_ix += 1;
                        s[k] += 1;
                    }
            }
        free (s);
        return out;
    }

	float **
	convolve(float ** result, float ** source, float ** kernel, int rsizex, int rsizey, int ksizex, int ksizey, float bias)
	{
#ifdef USE_VIMAGE
		if (ksizex % 2 == 1 && ksizey % 2 == 1)
		{
			struct vImage_Buffer src =
            {
                *source, static_cast<vImagePixelCount>(rsizey+(ksizey-1)), static_cast<vImagePixelCount>(rsizex+(ksizex-1)), sizeof(float)*(rsizex+(ksizex-1))
            };
			struct vImage_Buffer dest =
            {
                *result, static_cast<vImagePixelCount>(rsizey), static_cast<vImagePixelCount>(rsizex), sizeof(float)*rsizex
            };
			vImage_Error err;
			if (bias != 0)
				err = vImageConvolveWithBias_PlanarF(&src, &dest, NULL, (ksizex-1)/2, (ksizey-1)/2, *kernel, ksizex, ksizey, bias, 0, kvImageTruncateKernel);
			else
				err = vImageConvolve_PlanarF(&src, &dest, NULL, (ksizex-1)/2, (ksizey-1)/2, *kernel, ksizex, ksizey, 0, kvImageTruncateKernel);
			if (err < 0) printf("IKAROS_Math:convolve vImage_Error = %ld\n", err);
		}
		else
		{
			// If kernel sizes are not odd use scalar version instead
			for (int j=0; j<rsizey; j++)
				for (int i=0; i<rsizex; i++)
				{
					float s = 0.0;
					for (int l=0; l<ksizey; l++)
					{
						float * src = &source[j+l][i];
						float * krn = kernel[l];
						for (int k=0; k<ksizex; k++)
							s += krn[k] * (*src++);
					}
					result[j][i] = s + bias;
				}
		}
#else
		for (int j=0; j<rsizey; j++)
			for (int i=0; i<rsizex; i++)
			{
				float s = 0.0;
				for (int l=0; l<ksizey; l++)
				{
					float * src = &source[j+l][i];
					float * krn = kernel[l];
					for (int k=0; k<ksizex; k++)
						s += krn[k] * (*src++);
				}
				result[j][i] = s + bias;
			}
#endif
		return result;
	}
	


    float **
    integral_image(float ** r, float ** a, int sizex, int sizey)
    {
        r[0][0] = a[0][0];

        for(int i=1; i<sizex; i++)
            r[0][i] = a[0][i] + r[0][i-1];
        
        for(int j=1; j<sizey; j++)
            r[j][0] = a[j][0] + r[j-1][0];

        for(int i=1; i<sizex; i++)
            for(int j=1; j<sizey; j++)
                r[j][i] = a[j][i] + r[j-1][i] + r[j][i-1] - r[j-1][i-1];
                
        return r;
    }



	/*
	 Reasonably fast implementation of the box filter that sums all the
	 matrix elements within each box of size [boxsize] x [boxsize].
	 r - matrix with the sums; [sizex] x [sizey]
	 a - source matrix; [sizex + boxsize - 1] x [sizey + boxsize - 1]
	 t - temporary storage; size [sizex] x [sizey + boxsize - 1]; allocated and deallocated if t == NULL
	 */
    // TODO: Use intergal image instead

	float **
	box_filter(float ** r, float ** a, int sizex, int sizey, int boxsize, bool scale, float ** t)
	{
		bool owns_temp = false;
		if (t == NULL)
		{
			t = create_matrix(sizex, sizey+boxsize-1);	// sizex-boxsize+1
			owns_temp = true;
		}
		for (int j=0; j<sizey+boxsize-1; j++)
		{
			t[j][0] = 0;
			for (int i=0; i < boxsize; i++)
				t[j][0] += a[j][i];
		}
		for (int j=0; j<sizey+boxsize-1; j++)
		{
			for (int i=1; i < sizex; i++)
				t[j][i] = t[j][i-1] - a[j][i-1] + a[j][i+boxsize-1];
		}
		for (int i=0; i<sizex; i++)
		{
			r[0][i] = 0;
			for (int j=0; j < boxsize; j++)
				r[0][i] += t[j][i];
		}
		for (int j=1; j < sizey; j++)
		{
			float * sj0 = r[j];
			float * sj1 = r[j-1];
			float * tj1 = t[j-1];
			float * tjb = t[j+boxsize-1];
			for (int i=0; i<sizex; i++)
				*sj0++ = (*sj1++) - (*tj1++) + (*tjb++);
		}
		if (owns_temp)
			destroy_matrix(t);
		if (scale)
			multiply(r, 1.0/sqr(boxsize), sizex, sizey);
		return r;
	}
    


// image file formats
    // MARK: -
    // MARK: image file formats
    //
    // these functions will be moved to a separate file in the future
    //
    
    // create jpeg functions
    
    struct jpeg_destination { 
        struct jpeg_destination_mgr dest_mgr; 
        JOCTET * buffer; 
        size_t   size; 
        size_t   used;
    }; 
    
    
    static void 
    dst_init(j_compress_ptr cinfo) 
    { 
        struct jpeg_destination *dst = (jpeg_destination *)cinfo->dest; 
        dst->used = 0; 
        dst->size = 2048+cinfo->image_width * cinfo->image_height * cinfo->input_components * 2; // arbitrary initial size ; 2048 for header in case of very small images
        dst->buffer = (JOCTET *)malloc(dst->size * sizeof *dst->buffer);
        dst->dest_mgr.next_output_byte = dst->buffer; 
        dst->dest_mgr.free_in_buffer = dst->size; 
        return; 
    }
    
    
    // FIXME:   Something goes wrong when dst_empty is called more than once
    //          Fortunately it never happens since the size set by dst_init is large enough
    
    static boolean
    dst_empty(j_compress_ptr cinfo) 
    { 
        struct jpeg_destination *dst = (jpeg_destination *)cinfo->dest; 
        dst->used = dst->size - dst->dest_mgr.free_in_buffer;
        dst->size *= 2; 
        dst->buffer = (JOCTET *)realloc(dst->buffer, dst->size * sizeof *dst->buffer);
        dst->dest_mgr.next_output_byte = &dst->buffer[dst->used];
        dst->dest_mgr.free_in_buffer = dst->size - dst->used; 
        return true; 
    }
    
    
    static void
    dst_term(j_compress_ptr cinfo) 
    { 
        struct jpeg_destination *dst = (jpeg_destination *)cinfo->dest; 
        dst->used = dst->size - dst->dest_mgr.free_in_buffer;
        return; 
    } 
    
    
    static void
    jpeg_set_destination(j_compress_ptr cinfo, struct jpeg_destination *dst) 
    { 
        dst->dest_mgr.init_destination = dst_init; 
        dst->dest_mgr.empty_output_buffer = dst_empty; 
        dst->dest_mgr.term_destination = dst_term; 
        cinfo->dest = (jpeg_destination_mgr *)dst; 
        return; 
    } 
    
    
    
    char *
    create_jpeg(long int & size, float * matrix, int sizex, int sizey, float minimum, float maximum, int quality)
    {
        if (matrix == NULL)
        {
            size = 0;
            return NULL;
        }
        
        JSAMPLE *   image_buffer = new JSAMPLE [sizex];
        JSAMPROW    row_pointer[1];
        
        struct jpeg_compress_struct cinfo;
        struct jpeg_error_mgr       jerr;
        struct jpeg_destination     dst;
        
        cinfo.err = jpeg_std_error(&jerr);
        
        jpeg_create_compress(&cinfo);	// Replace with ikaros error handler later
        
        cinfo.image_width = sizex; 	//* image width and height, in pixels
        cinfo.image_height = sizey;
        cinfo.input_components = 1;	// # of color components per pixel
        cinfo.in_color_space = JCS_GRAYSCALE;
        
        jpeg_set_defaults(&cinfo);
        jpeg_set_quality(&cinfo, quality, true);
        jpeg_set_destination(&cinfo, &dst);
        
        // Do the compression
        
        jpeg_start_compress(&cinfo, true);
        
        while (cinfo.next_scanline < cinfo.image_height)
        {
            // Convert row to image buffer (assume max == 1 for now)
            
            JSAMPLE * ib = image_buffer;
            if (maximum != minimum)
                float_to_byte(image_buffer, matrix, minimum, maximum, sizex);
            else
                for (int i=0; i<sizex; i++)
                    *ib++ = 0;
            
            // Write to compressor
            row_pointer[0] = image_buffer;
            (void) jpeg_write_scanlines(&cinfo, row_pointer, 1);
            matrix += sizex;
        }
        
        jpeg_finish_compress(&cinfo);
        jpeg_destroy_compress(&cinfo);
        
        delete [] image_buffer;
        
        size = dst.used;
        return (char *)dst.buffer;
    }
    
    
    
    char *
    create_jpeg(long int & size, float ** matrix, int sizex, int sizey, float minimum, float maximum, int quality)
    {
        if (matrix == NULL)
        {
            size = 0;
            return NULL;
        }
        
        JSAMPLE *   image_buffer = new JSAMPLE [sizex];
        JSAMPROW    row_pointer[1];
        
        struct jpeg_compress_struct cinfo;
        struct jpeg_error_mgr       jerr;
        struct jpeg_destination     dst;
        
        //int    row_stride;				// physical row width in image buffer
        
        cinfo.err = jpeg_std_error(&jerr);
        
        jpeg_create_compress(&cinfo);	// Replace with ikaros error handler later
        
        cinfo.image_width = sizex; 	//* image width and height, in pixels
        cinfo.image_height = sizey;
        cinfo.input_components = 1;	// # of color components per pixel
        cinfo.in_color_space = JCS_GRAYSCALE;
        
        jpeg_set_defaults(&cinfo);
        jpeg_set_quality(&cinfo, quality, true);
        jpeg_set_destination(&cinfo, &dst);
        
        // Do the compression
        
        jpeg_start_compress(&cinfo, true);
        
        //row_stride = sizex * 1;	// JSAMPLEs per row in image_buffer
        int j=0;
        
        while (cinfo.next_scanline < cinfo.image_height)
        {
            // Convert row to image buffer (assume max == 1 for now)
            
            JSAMPLE * ib = image_buffer;
            if (maximum != minimum)
                float_to_byte(image_buffer, matrix[j], minimum, maximum, sizex);
            else
                for (int i=0; i<sizex; i++)
                    *ib++ = 0;
            
            // Write to compressor
            row_pointer[0] = image_buffer;
            (void) jpeg_write_scanlines(&cinfo, row_pointer, 1);
            j++;
        }
        
        jpeg_finish_compress(&cinfo);
        jpeg_destroy_compress(&cinfo);
        
        delete [] image_buffer;
        
        size = dst.used;
        return (char *)dst.buffer;
    }
    
    
    
    char *
    create_jpeg(long int & size, float * matrix, int sizex, int sizey, int color_table[256][3], int quality)
    {
        if (matrix == NULL)
        {
            size = 0;
            return NULL;
        }
        
        JSAMPLE *   image_buffer = new JSAMPLE [3*sizex];
        JSAMPROW    row_pointer[1];
        
        struct jpeg_compress_struct cinfo;
        struct jpeg_error_mgr       jerr;
        struct jpeg_destination     dst;
        
        cinfo.err = jpeg_std_error(&jerr);
        
        jpeg_create_compress(&cinfo); // TODO: Replace with ikaros error handler later
        
        cinfo.image_width = sizex; 	//* image width and height, in pixels
        cinfo.image_height = sizey;
        cinfo.input_components = 3;	// # of color components per pixel
        cinfo.in_color_space = JCS_RGB;
        
        jpeg_set_defaults(&cinfo);
        jpeg_set_quality(&cinfo, quality, true);
        jpeg_set_destination(&cinfo, &dst);
        
        // Do the compression
        
        jpeg_start_compress(&cinfo, true);
        
        unsigned char * z = new unsigned char [sizex];
        while (cinfo.next_scanline < cinfo.image_height)
        {
            float_to_byte(z, matrix, 0.0, 1.0, sizex);
            int x = 0;
            unsigned char * zz = z;
            
            for (int i=0; i<sizex; i++)
            {
                image_buffer[x++] = color_table[*zz][0];
                image_buffer[x++] = color_table[*zz][1];
                image_buffer[x++] = color_table[*zz][2];
                zz++;
            }
            
            // Write to compressor
            row_pointer[0] = image_buffer;
            (void) jpeg_write_scanlines(&cinfo, row_pointer, 1);
            matrix += sizex;
        }
        
        jpeg_finish_compress(&cinfo);
        jpeg_destroy_compress(&cinfo);
        
        delete [] z;
        delete [] image_buffer;
        
        size = dst.used;
        return (char *)dst.buffer;
    }
    
    
    
    char *
    create_jpeg(long int & size, float ** matrix, int sizex, int sizey, int color_table[256][3], int quality)
    {
        return create_jpeg(size, *matrix, sizex, sizey, color_table, quality);
    }
    
    
    
    char *
    create_jpeg(long int & size, float * r, float * g, float * b, int sizex, int sizey, int quality)
    {
        size = 0;
        if (r==NULL) return NULL;
        if (g==NULL) return NULL;
        if (b==NULL) return NULL;
        
        JSAMPLE *   image_buffer = new JSAMPLE [3*sizex];
        JSAMPROW    row_pointer[1];
        
        struct jpeg_compress_struct cinfo;
        struct jpeg_error_mgr       jerr;
        struct jpeg_destination     dst;
        
        //int    row_stride;				// physical row width in image buffer
        
        cinfo.err = jpeg_std_error(&jerr);
        
        jpeg_create_compress(&cinfo);	// Replace with ikaros error handler later
        
        cinfo.image_width = sizex; 	//* image width and height, in pixels
        cinfo.image_height = sizey;
        cinfo.input_components = 3;	// # of color components per pixel
        cinfo.in_color_space = JCS_RGB;
        
        jpeg_set_defaults(&cinfo);
        jpeg_set_quality(&cinfo, quality, true);
        jpeg_set_destination(&cinfo, &dst);
        
        // Do the compression
        
        jpeg_start_compress(&cinfo, true);
        
        //row_stride = sizex * 3;		/* JSAMPLEs per row in image_buffer */
        int j=0;
        
        float * rp = r;
        float * gp = g;
        float * bp = b;
        
        while (cinfo.next_scanline < cinfo.image_height)
        {
            int x = 0;
            for (int i=0; i<sizex; i++)
            {
                // IGNORE OVERFLOW
                // float rr = (*rp < 0.0 ? 0.0 : (*rp > 1.0 ? 1.0 : *rp));
                // float gg = (*gp < 0.0 ? 0.0 : (*gp > 1.0 ? 1.0 : *gp));
                // float bb = (*bp < 0.0 ? 0.0 : (*bp > 1.0 ? 1.0 : *bp));

                 // clipping |= (rr != r[j][i]) || (gg != g[j][i]) || (bb != b[j][i]);
                
                image_buffer[x++] = int(255.0*(*rp));
                image_buffer[x++] = int(255.0*(*gp));
                image_buffer[x++] = int(255.0*(*bp));
                
                rp++;
                gp++;
                bp++;
            }
            
            // Write to compressor
            row_pointer[0] = image_buffer;
            (void) jpeg_write_scanlines(&cinfo, row_pointer, 1);
            j++;
        }
        jpeg_finish_compress(&cinfo);
        jpeg_destroy_compress(&cinfo);
        
        delete [] image_buffer;
        
        size = dst.used;
        return (char *)dst.buffer;
    }
    
    
    
    char *
    create_jpeg(long int & size, float ** r, float ** g, float ** b, int sizex, int sizey, int quality)
    {
        return create_jpeg(size, *r, *g, *b, sizex, sizey, quality);
    }
    
    
    
    void
    destroy_jpeg(char * jpeg)
    {
        free(jpeg);
    }
    
    
    // decode jpeg functions
    
    typedef struct {
        struct jpeg_source_mgr pub;	// public fields
        JOCTET eoi_buffer[2];         // a place to put a dummy EOI
    } my_source_mgr;
    
    typedef my_source_mgr * my_src_ptr;
    
    
    static void
    init_source (j_decompress_ptr cinfo)
    {
    }
    
    
    static boolean
    fill_input_buffer (j_decompress_ptr cinfo)
    {
        my_src_ptr src = (my_src_ptr) cinfo->src;
        
        //      WARNMS(cinfo, JWRN_JPEG_EOF);
        
        // Create a fake EOI marker 
        src->eoi_buffer[0] = (JOCTET) 0xFF;
        src->eoi_buffer[1] = (JOCTET) JPEG_EOI;
        src->pub.next_input_byte = src->eoi_buffer;
        src->pub.bytes_in_buffer = 2;
        
        return TRUE;
    }
    
    
    static void
    skip_input_data (j_decompress_ptr cinfo, long num_bytes)
    {
        my_src_ptr src = (my_src_ptr) cinfo->src;
        
        if (num_bytes > 0) {
            while (num_bytes > (long) src->pub.bytes_in_buffer) {
                num_bytes -= (long) src->pub.bytes_in_buffer;
                (void) fill_input_buffer(cinfo);
                // note we assume that fill_input_buffer will never return FALSE, so suspension need not be handled.
            }
            src->pub.next_input_byte += (size_t) num_bytes;
            src->pub.bytes_in_buffer -= (size_t) num_bytes;
        }
    }
    
    
    static void
    term_source (j_decompress_ptr cinfo)
    {
    }
    
    
    static void
    jpeg_memory_src (j_decompress_ptr cinfo, const JOCTET * buffer, size_t bufsize)
    {
        my_src_ptr src;
        
        if (cinfo->src == NULL) // first time for this JPEG object?
        {	
            cinfo->src = (struct jpeg_source_mgr *)
            (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_PERMANENT, sizeof(my_source_mgr));
        }
        
        src = (my_src_ptr) cinfo->src;
        src->pub.init_source = init_source;
        src->pub.fill_input_buffer = fill_input_buffer;
        src->pub.skip_input_data = skip_input_data;
        src->pub.resync_to_restart = jpeg_resync_to_restart; // use default method 
        src->pub.term_source = term_source;
        
        src->pub.next_input_byte = buffer;
        src->pub.bytes_in_buffer = bufsize;
    }
    
    
    struct my_error_mgr
    {
        struct jpeg_error_mgr pub;	// "public" fields 
        jmp_buf setjmp_buffer;		// for return to caller
    };
    
    
    typedef struct my_error_mgr * my_error_ptr;
    
    
    static void
    my_error_exit (j_common_ptr cinfo)
    {
        my_error_ptr myerr = (my_error_ptr) cinfo->err;
        (*cinfo->err->output_message) (cinfo);
        longjmp(myerr->setjmp_buffer, 1);
    }
    
    
    bool
    jpeg_get_info(int & sizex, int & sizey, int & planes, char * data, long int size)
    {
        struct jpeg_decompress_struct cinfo;
        struct my_error_mgr jerr;
        
        cinfo.err = jpeg_std_error(&jerr.pub);
        jerr.pub.error_exit = my_error_exit;
        
        if (setjmp(jerr.setjmp_buffer))
        {
            jpeg_destroy_decompress(&cinfo);
            sizex = 0;
            sizey = 0;
            planes = 0;
            return false;
        }
        
        jpeg_create_decompress(&cinfo);
        jpeg_memory_src(&cinfo, (JOCTET *)data, size);
        (void) jpeg_read_header(&cinfo, TRUE);
        (void) jpeg_start_decompress(&cinfo);
        
        sizex  = cinfo.output_width;
        sizey  = cinfo.output_height;
        planes = cinfo.output_components;
        return true;
    }
    
    
    void
    jpeg_decode(float ** red_matrix, float ** green_matrix, float ** blue_matrix, float ** intensity_matrix, int sizex, int sizey, char * data, long int size)
    {
        struct jpeg_decompress_struct cinfo;
        struct my_error_mgr jerr;
        FILE * infile = NULL;       /* source file */
        JSAMPARRAY buffer;			/* Output row buffer */
        int row_stride;				/* physical row width in output buffer */
        
        cinfo.err = jpeg_std_error(&jerr.pub);
        jerr.pub.error_exit = my_error_exit;
        
        if (setjmp(jerr.setjmp_buffer))
        {
            jpeg_destroy_decompress(&cinfo);
            fclose(infile);
            return;
        }
        
        jpeg_create_decompress(&cinfo);
        jpeg_memory_src(&cinfo, (JOCTET *)data, size);
        
        (void) jpeg_read_header(&cinfo, TRUE);
        (void) jpeg_start_decompress(&cinfo);
        row_stride = cinfo.output_width * cinfo.output_components;
        
        if(cinfo.output_components != 3)
        {
            printf("IKAROS_Math:jpeg_decode: ERROR Not a color image.\n");
            return;
        }
        
        if (cinfo.output_width != (unsigned int)(sizex) ||  cinfo.output_height != (unsigned int)(sizey))
        {
            printf("IKAROS_Math:jpeg_decode: ERROR Incorresct size for destination.\n");
            return;
        }
        
        buffer = (*cinfo.mem->alloc_sarray)((j_common_ptr) &cinfo, JPOOL_IMAGE, row_stride, 1);
        
        // RGB Color Image
        
        float c255 = 1.0/255.0;
        float c3 = 1.0/3.0;
        while (cinfo.output_scanline < cinfo.output_height)
        {
            (void) jpeg_read_scanlines(&cinfo, buffer, 1);
            unsigned char * buf = buffer[0];
            int ix = (cinfo.output_scanline-1);
            float * r = red_matrix[ix];
            float * g = green_matrix[ix];
            float * b = blue_matrix[ix];
            float * iy = intensity_matrix[ix];
            for (int i=0; i<sizex; i++) // FIXME: CHECK BOUNDS
            {
                *r		= c255*float(*buf++);
                *g		= c255*float(*buf++);
                *b		= c255*float(*buf++);
                *iy++	= c3*((*r++)+(*g++)+(*b++));	// FIXME: Do this correctly later!!!
            }
        }
        
        (void) jpeg_finish_decompress(&cinfo);
        jpeg_destroy_decompress(&cinfo);
    }
    
    
    
    char *
    create_bmp(long int & size, float * r, float * g, float * b, int sizex, int sizey)
    {
        size = 54 + 4 * sizex * sizey;
        
        // character array is used rather than struct to
        // avoid padding of the data structure
        
        unsigned char * bmp = new unsigned char [size];
        
        for(int i=0; i<54; i++)
            bmp[i] =0;
        
        bmp[0] = 'B';
        bmp[1] = 'M';
        *(unsigned int *)(&bmp[2]) = 54 + 4 * sizex * sizey; // file size
        *(unsigned int *)(&bmp[10]) = 54; // offset
        *(unsigned int *)(&bmp[14]) = 40; // header size
        *(unsigned int *)(&bmp[18]) = sizex; // size_x
        *(unsigned int *)(&bmp[22]) = -sizey; // -size_y
        *(unsigned short *)(&bmp[26]) = 1; // planes
        *(unsigned short *)(&bmp[28]) = 4*8; // bits
        *(unsigned short *)(&bmp[38]) = 2835; // 72 dpi
        *(unsigned short *)(&bmp[42]) = 2835; // 72 dpi
        
        long int ix = 54;
        for(int j=0; j<sizey; j++)
            for(int i=0; i<sizex; i++)
            {
                bmp[ix++] = int(255.0 * (*(b++)));
                bmp[ix++] = int(255.0 * (*(g++)));
                bmp[ix++] = int(255.0 * (*(r++)));
                bmp[ix++] = 255;
            }
        
        return (char *)(bmp);
    }
    
    
    
    char *
    create_bmp(long int & size, float ** r, float ** g, float ** b, int sizex, int sizey) // FIXME: Use function above later
    {
        size = 54 + 4 * sizex * sizey;
        
        // character array is used rather than struct to
        // avoid padding of the data structure
        
        unsigned char * bmp = new unsigned char [size];
        
        for(int i=0; i<54; i++)
            bmp[i] =0;
        
        bmp[0] = 'B';
        bmp[1] = 'M';
        *(unsigned int *)(&bmp[2]) = 54 + 4 * sizex * sizey; // file size
        *(unsigned int *)(&bmp[10]) = 54; // offset
        *(unsigned int *)(&bmp[14]) = 40; // header size
        *(unsigned int *)(&bmp[18]) = sizex; // size_x
        *(unsigned int *)(&bmp[22]) = -sizey; // -size_y
        *(unsigned short *)(&bmp[26]) = 1; // planes
        *(unsigned short *)(&bmp[28]) = 4*8; // bits
        *(unsigned short *)(&bmp[38]) = 2835; // 72 dpi
        *(unsigned short *)(&bmp[42]) = 2835; // 72 dpi
        
        long int ix = 54;
        for(int j=0; j<sizey; j++)
            for(int i=0; i<sizex; i++)
            {
                bmp[ix++] = int(255.0*b[j][i]);
                bmp[ix++] = int(255.0*g[j][i]);
                bmp[ix++] = int(255.0*r[j][i]);
                bmp[ix++] = 255;
            }
        
        return (char *)(bmp);
    }
    
    
    void
    destroy_bmp(char * bmp)
    {
        delete [] bmp;
    }
    
    // drawing functions
    // MARK: -
    // MARK: drawing functions
    
    // Simple minded implemetation of the Bresenham algorithm
    
    static inline void
    swap(int & x, int & y)
    {
        int t = x;
        x = y;
        y = t;
    }
    
    
    static inline void
    plot(float ** image, int sizex, int sizey, int x, int y, float color)
    {
        if(x<0) return;
        if(y<0) return;
        if(x>=sizex) return;
        if(y>=sizey) return;
        image[y][x] = color;
    }
    
    void
    draw_line(float ** image, int sizex, int sizey, int x0, int y0, int x1, int y1, float color)
    {
        bool steep = ::abs(y1 - y0) > ::abs(x1 - x0);
        if(steep)
        {
            swap(x0, y0);
            swap(x1, y1);
        }
        
        if(x0 > x1)
        {
            swap(x0, x1);
            swap(y0, y1);
        }
        
        int deltax = x1 - x0;
        int deltay = ::abs(y1 - y0);
        float error = 0;
        float deltaerr = float(deltay) / float(deltax);
        int ystep;
        int y = y0;
        
        if(y0 < y1)
            ystep = 1;
        else
            ystep = -1;
        
        for(int x=x0; x<=x1; x++)
        {
            if(steep)
                plot(image, sizex, sizey, x, y, color);
            else
                plot(image, sizex, sizey, y, x, color);
            
            error = error + deltaerr;
            
            if(error >= 0.5)
            {
                y = y + ystep;
                error = error - 1.0;
            }
        }
    }
    
    
    void
    draw_line(float ** red_image, float ** green_image, float ** blue_image, int sizex, int sizey, int x0, int y0, int x1, int y1, float red, float green, float blue)
    {
        draw_line(red_image, sizex, sizey, x0, y0, x1, y1, red);
        draw_line(green_image, sizex, sizey, x0, y0, x1, y1, green);
        draw_line(blue_image, sizex, sizey, x0, y0, x1, y1, blue);
    }
    
    
    void
    draw_circle(float ** image, int sizex, int sizey, int x0, int y0, int radius, float color)
    {
        int f = 1 - radius;
        int ddF_x = 1;
        int ddF_y = -2 * radius;
        int x = 0;
        int y = radius;
        
        plot(image, sizex, sizey, x0, y0 + radius, color);
        plot(image, sizex, sizey, x0, y0 - radius, color);
        plot(image, sizex, sizey, x0 + radius, y0, color);
        plot(image, sizex, sizey, x0 - radius, y0, color);
        
        while(x < y)
        {
            if(f >= 0) 
            {
                y--;
                ddF_y += 2;
                f += ddF_y;
            }
            x++;
            ddF_x += 2;
            f += ddF_x;    
            plot(image, sizex, sizey, x0 + x, y0 + y, color);
            plot(image, sizex, sizey, x0 - x, y0 + y, color);
            plot(image, sizex, sizey, x0 + x, y0 - y, color);
            plot(image, sizex, sizey, x0 - x, y0 - y, color);
            plot(image, sizex, sizey, x0 + y, y0 + x, color);
            plot(image, sizex, sizey, x0 - y, y0 + x, color);
            plot(image, sizex, sizey, x0 + y, y0 - x, color);
            plot(image, sizex, sizey, x0 - y, y0 - x, color);
        }
    }
    
    
    void
    draw_circle(float ** red, float ** green, float ** blue, int sizex, int sizey, int x, int y, int radius, float r, float g, float b)
    {
        draw_circle(red, sizex, sizey, x, y, radius, r);
        draw_circle(green, sizex, sizey, x, y, radius, g);
        draw_circle(blue, sizex, sizey, x, y, radius, b);
    }


    void
    draw_rectangle(float ** image, int size_x, int size_y, int x0, int y0, int x1, int y1, float color)
    {
        x0 = max(x0, 0);
        y0 = max(y0, 0);
        x1 = min(x1, size_x-1);
        y1 = min(y1, size_y-1);
        
        for(int x=x0; x<x1; x++)
        {
            image[y0][x] = color;
            image[y1][x] = color;
        }

        for(int y=y0; y<y1; y++)
        {
            image[y][x0] = color;
            image[y][x1] = color;
        }
    }
    
    
    void
    draw_rectangle(float ** red_image, float ** green_image, float ** blue_image, int size_x, int size_y, int x0, int y0, int x1, int y1, float red, float green, float blue)
    {
        draw_rectangle(red_image, size_x, size_y, x0, y0, x1, y1, red);
        draw_rectangle(green_image, size_x, size_y, x0, y0, x1, y1, green);
        draw_rectangle(blue_image, size_x, size_y, x0, y0, x1, y1, blue);
    }
}