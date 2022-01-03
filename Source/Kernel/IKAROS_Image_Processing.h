////
//	IKAROS_Inage_Processing.h		Various image processing functions for IKAROS
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

#ifndef IKAROS_IMAGE_PROCESSING
#define IKAROS_IMAGE_PROCESSING

namespace ikaros
{
    float **    im2row(float ** result, float ** source, int result_size_x, int result_size_y, int source_size_x, int source_size_y, int kernel_size_x, int kernel_size_y, int stride_x=1, int stride_y=1);
float ** 	    spanned_im2row(float ** result, float ** source, int result_size_x, int result_size_y, int source_size_x, int source_size_y, int kernel_size_x, int kernel_size_y, int stride_x, int stride_y, int block_x, int block_y, int span_x, int span_y);
	float ** 	spanned_row2im(float **out, float **in, int out_x, int out_y,int in_x, int in_y,int rf_x, int rf_y,int inc_x, int inc_y,int blk_x, int blk_y,int spn_x, int spn_y);
    float **	convolve(float ** result, float ** source, float ** kernel, int rsizex, int rsizey, int ksizex, int ksizey, float bias = 0.0); // ksizex and ksizey must be odd for BLAS calls to work // TODO: Check that it works otherwise as well
	float **	box_filter(float ** r, float ** a, int sizex, int sizey, int boxsize, bool scale = false, float ** t = 0);
    float **    integral_image(float ** r, float ** a, int sizex, int sizey);
    
	/*
	 float **	erode(float ** result, int masksize, int sizex, int sizey);
	 float **	dilate(float ** result, int masksize, int sizex, int sizey);
	 float **	open(float ** result, int masksize, int sizex, int sizey);
	 float **	close(float ** result, int masksize, int sizex, int sizey);
     float **	skeletonize(...);
	 */

	// image file formats
    
    char *      create_jpeg(long int & size, float * array, int sizex, int sizey, float minimum=0, float maximum=1, int quality=100);
    char *      create_jpeg(long int & size, float ** matrix, int sizex, int sizey, float minimum=0, float maximum=1, int quality=100);
    char *      create_jpeg(long int & size, float * matrix, int sizex, int sizey, int color_table[256][3], int quality=100);
    char *      create_jpeg(long int & size, float ** matrix, int sizex, int sizey, int color_table[256][3], int quality=100);
    char *      create_jpeg(long int & size, float * red_array, float * green_array, float * blue_array, int sizex, int sizey, int quality=100);
    char *      create_jpeg(long int & size, float ** red_matrix, float ** green_matrix, float ** blue_matrix, int sizex, int sizey, int quality=100);

    void        destroy_jpeg(char * jpeg);

    void        decode_jpeg(float ** matrix, int sizex, int sizey, char * data, long int size);
    void        decode_jpeg(float ** red_matrix, float ** green_matrix, float ** blue_matrix, int sizex, int sizey, char * data, long int size);

    bool        jpeg_get_info(int & sizex, int & sizey, int & planes, char * data, long int size);
    void        jpeg_decode(float ** red_matrix, float ** green_matrix, float ** blue_matrix, float ** intensity_matrix, int sizex, int sizey, char * data, long int size);

    char *      create_bmp(long int & size, float * red_array, float * green_array, float * blue_array, int sizex, int sizey);
    char *      create_bmp(long int & size, float ** red_matrix, float ** green_matrix, float ** blue_matrix, int sizex, int sizey);
    void        destroy_bmp(char * bmp);

    // drawing routines for draing in matrices (not currently used anywhere in Ikaros)

    void        draw_line(float ** image, int sizex, int sizey, int x0, int y0, int x1, int x2, float color);
    void        draw_line(float ** red, float ** green, float ** blue, int sizex, int sizey, int x0, int y0, int x1, int x2, float r, float g, float b);
    void        draw_circle(float ** image, int sizex, int sizey, int x, int y, int radius, float color);
    void        draw_circle(float ** red, float ** green, float ** blue, int sizex, int sizey, int x, int y, int radius, float r, float g, float b);

    void        draw_rectangle(float ** image, int size_x, int size_y, int x0, int y0, int x1, int y1, float color);
    void        draw_rectangle(float ** red_image, float ** green_image, float ** blue_image, int size_x, int size_y, int x0, int y0, int x1, int y1, float red, float green, float blue);

}

#endif
