// image_file_format.h
// Copyright (C) 2023-2025  Christian Balkenius

#pragma once

#include <filesystem>

namespace ikaros
{    
    //char *      create_jpeg(long int & size, matrix & data, float minimum=0, float maximum=1, int quality=100);

    unsigned char * create_color_jpeg(long int & size, matrix & image, int quality=100);
    unsigned char * create_gray_jpeg(long int & size, matrix & image, float minimum=0, float maximum=1, int quality=100);
    unsigned char * create_pseudocolor_jpeg(long int & size, matrix & image, float minimum=0, float maximum=1, const std::string & table="fire",  int quality=100);
    void destroy_jpeg(unsigned char * jpeg);

    //void        decode_jpeg(float ** matrix, int sizex, int sizey, char * data, long int size);
    //void        decode_jpeg(float ** red_matrix, float ** green_matrix, float ** blue_matrix, int sizex, int sizey, char * data, long int size);

    //bool        jpeg_get_info(int & sizex, int & sizey, int & planes, char * data, long int size);
    //void        jpeg_decode(float ** red_matrix, float ** green_matrix, float ** blue_matrix, float ** intensity_matrix, int sizex, int sizey, char * data, long int size);


        void    png_get_size(int & sizex, int & sizey, std::filesystem::path filename);
        int     png_get_channels(std::filesystem::path filename);
         void   png_get_image(matrix & red, matrix & green, matrix & blue, std::filesystem::path filename);
};


