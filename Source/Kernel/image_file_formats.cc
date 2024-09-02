// image_file_format.cc
// Copyright (C) 2023  Christian Balkenius

#include "matrix.h"
#include "image_file_formats.h"
#include "color_tables.h"

extern "C"
{
#include "jpeglib.h"
#include <setjmp.h>
}
namespace ikaros
{


    static void
	float_to_byte(unsigned char * r, float * a, float min, float max, long size)
	{
#ifdef USE_VIMAGE
		struct vImage_Buffer src =
        {
            a, 1, static_cast<vImagePixelCount>(size), size*sizeof(float)
        };
		struct vImage_Buffer dest =
        {
            r, 1, static_cast<vImagePixelCount>(size), size*sizeof(float)
        };
		vImage_Error err = vImageConvert_PlanarFtoPlanar8 (&src, &dest, max, min, 0);
		if (err < 0) 
            throw exception("image_file_formats:float_to_byte: vImage_Error");
#else
		for (long i=0; i<size; i++)
			if (a[i] < min)
				r[i] = 0;
			else if (a[i] > max)
				r[i] = 255;
			else
				r[i] = int(255.0*(a[i]-min)/(max-min));
#endif
	}
	

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
    
    
     /*
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
        
        cinfo.image_width = sizex; 	/// image width and height, in pixels
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
    */
    
    
    unsigned char *
    create_gray_jpeg(long int & size, matrix & image, float minimum, float maximum, int quality)
    {
        long sizex = image.size(1);
        long sizey = image.size(0);

        JSAMPLE *   image_buffer = new JSAMPLE [sizex];
        JSAMPROW    row_pointer[1];
        
        struct jpeg_compress_struct cinfo;
        struct jpeg_error_mgr       jerr;
        struct jpeg_destination     dst;
        
        //int    row_stride;				// physical row width in image buffer
        
        cinfo.err = jpeg_std_error(&jerr);
        
        jpeg_create_compress(&cinfo);	// Replace with ikaros error handler later
        
        cinfo.image_width = sizex; 	/// image width and height, in pixels
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
            if (maximum != minimum) // FIXME: test in conversion instead
                float_to_byte(image_buffer, image[j], minimum, maximum, sizex);
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
        return dst.buffer;
    }
    
    
    unsigned char * create_pseudocolor_jpeg(long int & size, matrix & image, float minimum, float maximum, const std::string & table,  int quality)
    {
        if(!color_table.count(table))
            return nullptr;

        long sizex = image.size(1);
        long sizey = image.size(0);

        JSAMPLE *   image_buffer = new JSAMPLE [3*sizex];
        JSAMPROW    row_pointer[1];
        
        struct jpeg_compress_struct cinfo;
        struct jpeg_error_mgr       jerr;
        struct jpeg_destination     dst;
        
        cinfo.err = jpeg_std_error(&jerr);
        
        jpeg_create_compress(&cinfo); // TODO: Replace with ikaros error handler later
        
        cinfo.image_width = sizex; 	//
        cinfo.image_height = sizey;
        cinfo.input_components = 3;	// # of color components per pixel
        cinfo.in_color_space = JCS_RGB;
        
        jpeg_set_defaults(&cinfo);
        jpeg_set_quality(&cinfo, quality, true);
        jpeg_set_destination(&cinfo, &dst);
        
        // Do the compression
        
        jpeg_start_compress(&cinfo, true);
        int j=0;
        
        unsigned char * z = new unsigned char [sizex];
        while (cinfo.next_scanline < cinfo.image_height)
        {
            float_to_byte(z, image[j], minimum, maximum, sizex);
            int x = 0;
            unsigned char * zz = z;
            
            for (int i=0; i<sizex; i++)
            {
                image_buffer[x++] = color_table[table][*zz][0];
                image_buffer[x++] = color_table[table][*zz][1];
                image_buffer[x++] = color_table[table][*zz][2];
                zz++;
            }
            
            // Write to compressor
            row_pointer[0] = image_buffer;
            (void) jpeg_write_scanlines(&cinfo, row_pointer, 1);
            j++;
        }
        
        jpeg_finish_compress(&cinfo);
        jpeg_destroy_compress(&cinfo);
        
        delete [] z;
        delete [] image_buffer;
        
        size = dst.used;
        return dst.buffer;
    }
    
    /*
    char *
    create_jpeg(long int & size, float ** matrix, int sizex, int sizey, int color_table[256][3], int quality)
    {
        return create_jpeg(size, *matrix, sizex, sizey, color_table, quality);
    }
    */
    
    /*
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
        
        cinfo.image_width = sizex; 	/// image width and height, in pixels
        cinfo.image_height = sizey;
        cinfo.input_components = 3;	// # of color components per pixel
        cinfo.in_color_space = JCS_RGB;
        
        jpeg_set_defaults(&cinfo);
        jpeg_set_quality(&cinfo, quality, true);
        jpeg_set_destination(&cinfo, &dst);
        
        // Do the compression
        
        jpeg_start_compress(&cinfo, true);
        
        //row_stride = sizex * 3;		// JSAMPLEs per row in image_buffer
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
    */
    
    /*
    char *
    create_jpeg(long int & size, float ** r, float ** g, float ** b, int sizex, int sizey, int quality)
    {
        return create_jpeg(size, *r, *g, *b, sizex, sizey, quality);
    }
    */
    
    
    void
    destroy_jpeg(unsigned char * jpeg)
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


    unsigned char *
    create_color_jpeg(long int & size, matrix & image, int quality)
    {
        size = 0;

        float * r = image[0][0];
        float * g = image[1][0];
        float * b = image[2][0];
        long sizex = image.shape()[2];  // FIXME: Change to size(2)
        long sizey = image.shape()[1];

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
        return dst.buffer;
    }
    
};