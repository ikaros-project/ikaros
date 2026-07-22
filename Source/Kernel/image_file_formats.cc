// image_file_format.cc
// Copyright (C) 2023-2025  Christian Balkenius

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <new>
#include <stdexcept>
#include <string>

#include "matrix.h"
#include "image_file_formats.h"
#include "color_tables.h"

#if defined(__APPLE__) && defined(IKAROS_MATRIX_ACCELERATE) && IKAROS_MATRIX_ACCELERATE
// Avoid Accelerate.h, which pulls deprecated compatibility headers into C++ builds.
#include <Accelerate/../Frameworks/vImage.framework/Headers/Conversion.h>
#define IKAROS_IMAGE_ACCELERATE 1
#else
#define IKAROS_IMAGE_ACCELERATE 0
#endif

extern "C"
{
#include "jpeglib.h"
#include <setjmp.h>
#include <png.h>
}
namespace ikaros
{


    struct jpeg_encoder_error_mgr
    {
        jpeg_error_mgr pub;
        jmp_buf setjmp_buffer;
        char message[JMSG_LENGTH_MAX];
    };


    static void
    jpeg_encoder_error_exit(j_common_ptr cinfo)
    {
        auto * error = reinterpret_cast<jpeg_encoder_error_mgr *>(cinfo->err);
        (*cinfo->err->format_message)(cinfo, error->message);
        longjmp(error->setjmp_buffer, 1);
    }


    static void
    validate_float_to_byte_range(float minimum, float maximum)
    {
        if(!std::isfinite(minimum) || !std::isfinite(maximum))
            throw std::invalid_argument("JPEG conversion range must be finite.");
        if(maximum < minimum)
            throw std::invalid_argument("JPEG conversion maximum must not be less than its minimum.");
        if(maximum > minimum && !std::isfinite(maximum - minimum))
            throw std::invalid_argument("JPEG conversion range is too wide.");
    }


    static inline unsigned char
    normalized_float_to_byte(float value, float minimum, float maximum, float scale) noexcept
    {
        if(!(value > minimum))
            return 0;
        if(value >= maximum)
            return 255;
        return static_cast<unsigned char>((value - minimum) * scale + 0.5f);
    }


    static void
    float_to_byte(unsigned char * result, const float * source,
                  float minimum, float maximum, long size)
    {
        if(maximum == minimum)
        {
            std::fill_n(result, static_cast<std::size_t>(size), static_cast<unsigned char>(0));
            return;
        }

#if IKAROS_IMAGE_ACCELERATE
        vImage_Buffer source_buffer =
        {
            const_cast<float *>(source),
            1,
            static_cast<vImagePixelCount>(size),
            static_cast<std::size_t>(size) * sizeof(float),
        };
        vImage_Buffer result_buffer =
        {
            result,
            1,
            static_cast<vImagePixelCount>(size),
            static_cast<std::size_t>(size) * sizeof(unsigned char),
        };
        if(vImageConvert_PlanarFtoPlanar8(&source_buffer, &result_buffer,
                                          maximum, minimum, kvImageDoNotTile) == kvImageNoError)
            return;
#endif

        const float scale = 255.0f / (maximum - minimum);
        for(long i = 0; i < size; ++i)
            result[i] = normalized_float_to_byte(source[i], minimum, maximum, scale);
    }


    static void
    float_rgb_to_byte(unsigned char * result, unsigned char * planar_buffer,
                      const float * red, const float * green, const float * blue, long size)
    {
#if IKAROS_IMAGE_ACCELERATE
        const auto width = static_cast<vImagePixelCount>(size);
        const auto float_row_bytes = static_cast<std::size_t>(size) * sizeof(float);
        const auto byte_row_bytes = static_cast<std::size_t>(size) * sizeof(unsigned char);
        vImage_Buffer source_red{const_cast<float *>(red), 1, width, float_row_bytes};
        vImage_Buffer source_green{const_cast<float *>(green), 1, width, float_row_bytes};
        vImage_Buffer source_blue{const_cast<float *>(blue), 1, width, float_row_bytes};
        vImage_Buffer planar_red{planar_buffer, 1, width, byte_row_bytes};
        vImage_Buffer planar_green{planar_buffer + size, 1, width, byte_row_bytes};
        vImage_Buffer planar_blue{planar_buffer + 2 * size, 1, width, byte_row_bytes};
        vImage_Buffer destination{result, 1, width, 3 * byte_row_bytes};

        if(vImageConvert_PlanarFtoPlanar8(&source_red, &planar_red, 1, 0,
                                          kvImageDoNotTile) == kvImageNoError &&
           vImageConvert_PlanarFtoPlanar8(&source_green, &planar_green, 1, 0,
                                          kvImageDoNotTile) == kvImageNoError &&
           vImageConvert_PlanarFtoPlanar8(&source_blue, &planar_blue, 1, 0,
                                          kvImageDoNotTile) == kvImageNoError &&
           vImageConvert_Planar8toRGB888(&planar_red, &planar_green, &planar_blue,
                                         &destination, kvImageDoNotTile) == kvImageNoError)
            return;
#else
        static_cast<void>(planar_buffer);
#endif

        for(long i = 0; i < size; ++i)
        {
            *result++ = normalized_float_to_byte(red[i], 0, 1, 255);
            *result++ = normalized_float_to_byte(green[i], 0, 1, 255);
            *result++ = normalized_float_to_byte(blue[i], 0, 1, 255);
        }
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
        if(dst->buffer == nullptr)
            throw std::bad_alloc();
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
        JOCTET * resized = (JOCTET *)realloc(dst->buffer, dst->size * sizeof *dst->buffer);
        if(resized == nullptr)
            throw std::bad_alloc();
        dst->buffer = resized;
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
        size = 0;
        validate_float_to_byte_range(minimum, maximum);

        long sizex = image.size(1);
        long sizey = image.size(0);

        JSAMPLE *   image_buffer = new JSAMPLE [sizex];
        JSAMPROW    row_pointer[1];
        
        struct jpeg_compress_struct cinfo{};
        struct jpeg_encoder_error_mgr jerr{};
        struct jpeg_destination dst{};
        
        //int    row_stride;				// physical row width in image buffer
        
        cinfo.err = jpeg_std_error(&jerr.pub);
        jerr.pub.error_exit = jpeg_encoder_error_exit;

        if(setjmp(jerr.setjmp_buffer))
        {
            jpeg_destroy_compress(&cinfo);
            free(dst.buffer);
            delete [] image_buffer;
            size = 0;
            throw std::runtime_error("JPEG encoding failed: " + std::string(jerr.message));
        }
        
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
            float_to_byte(image_buffer, image.logical_block_data(j),
                          minimum, maximum, sizex);
            
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
        size = 0;
        if(!color_table.count(table))
            return nullptr;
        validate_float_to_byte_range(minimum, maximum);

        long sizex = image.size(1);
        long sizey = image.size(0);

        JSAMPLE *   image_buffer = new JSAMPLE [3*sizex];
        unsigned char * z = nullptr;
        try
        {
            z = new unsigned char [sizex];
        }
        catch(...)
        {
            delete [] image_buffer;
            throw;
        }
        JSAMPROW    row_pointer[1];
        
        struct jpeg_compress_struct cinfo{};
        struct jpeg_encoder_error_mgr jerr{};
        struct jpeg_destination dst{};
        
        cinfo.err = jpeg_std_error(&jerr.pub);
        jerr.pub.error_exit = jpeg_encoder_error_exit;

        if(setjmp(jerr.setjmp_buffer))
        {
            jpeg_destroy_compress(&cinfo);
            free(dst.buffer);
            delete [] z;
            delete [] image_buffer;
            size = 0;
            throw std::runtime_error("JPEG encoding failed: " + std::string(jerr.message));
        }
        
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
        
        while (cinfo.next_scanline < cinfo.image_height)
        {
            float_to_byte(z, image.logical_block_data(j), minimum, maximum, sizex);
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
        
        if (cinfo->src == nullptr) // first time for this JPEG object?
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

        if(image.rank() != 3 || image.size(0) != 3)
            return nullptr;

        long sizex = image.size(2);
        long sizey = image.shape()[1];
        
        const long image_storage_size = (IKAROS_IMAGE_ACCELERATE ? 6 : 3) * sizex;
        JSAMPLE * image_storage = new JSAMPLE [image_storage_size];
        JSAMPLE * image_buffer = image_storage;
        JSAMPLE * planar_buffer = IKAROS_IMAGE_ACCELERATE ? image_storage + 3 * sizex : nullptr;
        JSAMPROW    row_pointer[1];
        
        struct jpeg_compress_struct cinfo{};
        struct jpeg_encoder_error_mgr jerr{};
        struct jpeg_destination dst{};
        
        //int    row_stride;				// physical row width in image buffer
        
        cinfo.err = jpeg_std_error(&jerr.pub);
        jerr.pub.error_exit = jpeg_encoder_error_exit;

        if(setjmp(jerr.setjmp_buffer))
        {
            jpeg_destroy_compress(&cinfo);
            free(dst.buffer);
            delete [] image_storage;
            size = 0;
            throw std::runtime_error("JPEG encoding failed: " + std::string(jerr.message));
        }
        
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
        
        while (cinfo.next_scanline < cinfo.image_height)
        {
            const float * red = image.logical_block_data(j);
            const float * green = image.logical_block_data(sizey + j);
            const float * blue = image.logical_block_data(2 * sizey + j);
            float_rgb_to_byte(image_buffer, planar_buffer, red, green, blue, sizex);
            
            // Write to compressor
            row_pointer[0] = image_buffer;
            (void) jpeg_write_scanlines(&cinfo, row_pointer, 1);
            j++;
        }
        jpeg_finish_compress(&cinfo);
        jpeg_destroy_compress(&cinfo);
        
        delete [] image_storage;
        
        size = dst.used;
        return dst.buffer;
    }


  void 
    jpeg_get_size(int & sizex, int & sizey, std::filesystem::path filename)
    {
        FILE * infile;
        if ((infile = fopen(filename.c_str(), "rb")) == nullptr) {
            throw std::runtime_error("Can't open " + filename.string());
        }

        struct jpeg_decompress_struct cinfo;
        struct jpeg_error_mgr jerr;

        cinfo.err = jpeg_std_error(&jerr);
        jpeg_create_decompress(&cinfo);
        jpeg_stdio_src(&cinfo, infile);
        jpeg_read_header(&cinfo, TRUE);

        sizex = cinfo.image_width;
        sizey = cinfo.image_height;

        jpeg_destroy_decompress(&cinfo);
        fclose(infile);
    }

    int 
    jpeg_get_channels(std::filesystem::path filename)
    {
        FILE * infile;
        if ((infile = fopen(filename.c_str(), "rb")) == nullptr) {
            throw std::runtime_error("Can't open " + filename.string());
        }

        struct jpeg_decompress_struct cinfo;
        struct jpeg_error_mgr jerr;

        cinfo.err = jpeg_std_error(&jerr);
        jpeg_create_decompress(&cinfo);
        jpeg_stdio_src(&cinfo, infile);
        jpeg_read_header(&cinfo, TRUE);

        int channels = cinfo.num_components;

        jpeg_destroy_decompress(&cinfo);
        fclose(infile);

        return channels;
    }

    void
    jpeg_get_image(matrix & red, matrix & green, matrix & blue, std::filesystem::path filename)
    {
        FILE * infile;
        if ((infile = fopen(filename.c_str(), "rb")) == nullptr) {
            throw std::runtime_error("Can't open " + filename.string());
        }

        struct jpeg_decompress_struct cinfo;
        struct jpeg_error_mgr jerr;

        cinfo.err = jpeg_std_error(&jerr);
        jpeg_create_decompress(&cinfo);
        jpeg_stdio_src(&cinfo, infile);
        jpeg_read_header(&cinfo, TRUE);
        jpeg_start_decompress(&cinfo);

        int width = cinfo.output_width;
        int height = cinfo.output_height;
        int row_stride = width * cinfo.output_components;

        // Resize matrices
        red.resize(height, width);
        green.resize(height, width);
        blue.resize(height, width);

        JSAMPARRAY buffer = (*cinfo.mem->alloc_sarray)
            ((j_common_ptr) &cinfo, JPOOL_IMAGE, row_stride, 1);

        int row = 0;
        while (cinfo.output_scanline < cinfo.output_height) {
            jpeg_read_scanlines(&cinfo, buffer, 1);
            
            for (int x = 0; x < width; x++) {
                red[row][x] = buffer[0][x*3] / 255.0f;
                green[row][x] = buffer[0][x*3+1] / 255.0f;
                blue[row][x] = buffer[0][x*3+2] / 255.0f;
            }
            row++;
        }

        jpeg_finish_decompress(&cinfo);
        jpeg_destroy_decompress(&cinfo);
        fclose(infile);
    }

    //
    // PNG Images
    //

        void    
        png_get_size(int & sizex, int & sizey, std::filesystem::path filename)
        {
            FILE *fp = fopen(filename.c_str(), "rb");
            if (!fp) {
                throw std::runtime_error("Cannot open file: " + filename.string());
            }

            unsigned char header[8];
            fread(header, 1, 8, fp);
            if (png_sig_cmp(header, 0, 8)) {
                fclose(fp);
                throw std::runtime_error("Not a PNG file: " + filename.string());
            }

            png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
            if (!png) {
                fclose(fp);
                throw std::runtime_error("Failed to create PNG read struct");
            }

            png_infop info = png_create_info_struct(png);
            if (!info) {
                png_destroy_read_struct(&png, nullptr, nullptr);
                fclose(fp);
                throw std::runtime_error("Failed to create PNG info struct");
            }

            if (setjmp(png_jmpbuf(png))) {
                png_destroy_read_struct(&png, &info, nullptr);
                fclose(fp);
                throw std::runtime_error("Error during PNG read");
            }

            png_init_io(png, fp);
            png_set_sig_bytes(png, 8);
            png_read_info(png, info);

            sizex = png_get_image_width(png, info);
            sizey = png_get_image_height(png, info);

            png_destroy_read_struct(&png, &info, nullptr);
            fclose(fp);
        }


        int     
        png_get_channels(std::filesystem::path filename)
        {
            FILE *fp = fopen(filename.c_str(), "rb");
            if (!fp) {
                throw std::runtime_error("Cannot open file: " + filename.string());
            }

            unsigned char header[8];
            fread(header, 1, 8, fp);
            if (png_sig_cmp(header, 0, 8)) {
                fclose(fp);
                throw std::runtime_error("Not a PNG file: " + filename.string());
            }

            png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
            png_infop info = png_create_info_struct(png);

            if (setjmp(png_jmpbuf(png))) {
                png_destroy_read_struct(&png, &info, nullptr);
                fclose(fp);
                throw std::runtime_error("Error during PNG read");
            }

            png_init_io(png, fp);
            png_set_sig_bytes(png, 8);
            png_read_info(png, info);

            int channels = png_get_channels(png, info);

            png_destroy_read_struct(&png, &info, nullptr);
            fclose(fp);

            return channels;
        }



         void   
         png_get_image(matrix & red, matrix & green, matrix & blue, std::filesystem::path filename)
         {
 FILE *fp = fopen(filename.c_str(), "rb");
    if (!fp) {
        throw std::runtime_error("Cannot open file: " + filename.string());
    }

    unsigned char header[8];
    fread(header, 1, 8, fp);
    if (png_sig_cmp(header, 0, 8)) {
        fclose(fp);
        throw std::runtime_error("Not a PNG file: " + filename.string());
    }

    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    png_infop info = png_create_info_struct(png);

    if (setjmp(png_jmpbuf(png))) {
        png_destroy_read_struct(&png, &info, nullptr);
        fclose(fp);
        throw std::runtime_error("Error during PNG read");
    }

    png_init_io(png, fp);
    png_set_sig_bytes(png, 8);
    png_read_info(png, info);

    int width = png_get_image_width(png, info);
    int height = png_get_image_height(png, info);
    int color_type = png_get_color_type(png, info);
    int bit_depth = png_get_bit_depth(png, info);

    if (color_type == PNG_COLOR_TYPE_PALETTE)
        png_set_palette_to_rgb(png);
    if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
        png_set_expand_gray_1_2_4_to_8(png);
    if (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
        png_set_gray_to_rgb(png);
    if (png_get_valid(png, info, PNG_INFO_tRNS))
        png_set_tRNS_to_alpha(png);
    if (bit_depth == 16)
        png_set_strip_16(png);
    if (color_type & PNG_COLOR_MASK_ALPHA || png_get_valid(png, info, PNG_INFO_tRNS))
        png_set_strip_alpha(png);

    png_read_update_info(png, info);
    if(png_get_channels(png, info) != 3)
    {
        png_destroy_read_struct(&png, &info, nullptr);
        fclose(fp);
        throw std::runtime_error("Unsupported PNG color format: " + filename.string());
    }

    // Allocate memory for the row pointers
    png_bytep *row_pointers = (png_bytep*)malloc(sizeof(png_bytep) * height);
    if(row_pointers == nullptr)
    {
        png_destroy_read_struct(&png, &info, nullptr);
        fclose(fp);
        throw std::runtime_error("Could not allocate PNG row pointers");
    }
    for(int y = 0; y < height; y++) {
        row_pointers[y] = (png_byte*)malloc(png_get_rowbytes(png, info));
        if(row_pointers[y] == nullptr)
        {
            for(int i = 0; i < y; i++)
                free(row_pointers[i]);
            free(row_pointers);
            png_destroy_read_struct(&png, &info, nullptr);
            fclose(fp);
            throw std::runtime_error("Could not allocate PNG row");
        }
    }

    png_read_image(png, row_pointers);

    // Resize matrices if needed
    red.resize(height, width);
    green.resize(height, width);
    blue.resize(height, width);

    // Copy data to matrices
    for(int y = 0; y < height; y++) {
        png_bytep row = row_pointers[y];
        for(int x = 0; x < width; x++) {
            png_bytep px = &(row[x * 3]);
            red[y][x] = px[0] / 255.0f;
            green[y][x] = px[1] / 255.0f;
            blue[y][x] = px[2] / 255.0f;
        }
    }

    // Cleanup
    for(int y = 0; y < height; y++) {
        free(row_pointers[y]);
    }
    free(row_pointers);

    png_destroy_read_struct(&png, &info, nullptr);
    fclose(fp);
         }

};
