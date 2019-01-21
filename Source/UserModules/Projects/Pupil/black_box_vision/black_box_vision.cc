//
//	black_box_vision.cc		This file is a part of the IKAROS project
//
//    Copyright (C) 2016 Birger Johansson
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


#include "black_box_vision.h"

#include <stdio.h>
#include <stdlib.h>

// For reading image
#define INITIAL_BUFFER_SIZE 65536

// The knowledge table colums
#define TRIAL         0
#define FILENAME      1
#define TIME          2
#define EMOTION       3
#define OBJECT        4

// The object that the "vision" recognise
#define O_NONE        -1
#define O_CIRCLE      0
#define O_RECTANGLE   1
#define O_TRIANGLE    2
#define O_HAPPIER     3
#define O_HAPPY       4
#define O_NUTRAL      5
#define O_SAD         6
#define O_SADER       7
#define O_SUN         8
#define O_MOON        9

// How many objects does the system know about
//#define numberOfObjects        10

//#define DEGBUG

using namespace ikaros;

void
black_box_vision::Init()
{
    // Parameter
    numberOfObjects =   GetIntValue("number_of_objects");

    // The black box knows everything.
    knowedge = GetInputMatrix("TOTAL_KNOWLEDGE_TABLE");
    knowedge_size_x = GetInputSizeX("TOTAL_KNOWLEDGE_TABLE");
    knowedge_size_y = GetInputSizeY("TOTAL_KNOWLEDGE_TABLE");
    
    // Output image stimuli
    s_intensity = GetOutputMatrix("S_INTENSITY");
    s_intensity_size_x = GetOutputSizeX("S_INTENSITY");
    s_intensity_size_y = GetOutputSizeY("S_INTENSITY");
    
    // Output unrealistic model
    novelty = GetOutputArray("NOVELTY");
    emotionPos = GetOutputArray("EMOTION_POS");
    emotionNeg = GetOutputArray("EMOTION_NEG");
    objects = GetOutputArray("OBJECT");
    
    stimuliCounter = new int[numberOfObjects];
    
    currentImage = 0;
}



void
black_box_vision::Tick()
{
    if (currentImage >= knowedge_size_y)    // End of experiment
    {
        Notify(msg_terminate, "End of experiment");
        return;
    }
    
    
    if (imageTickCounter == 0)              // New stimuli
    {
#ifdef DEGBUG
        printf("\n***************\n");
        printf("Stimuli %i\n",currentImage);
        printf("ID %i\n",int (knowedge[currentImage][FILENAME]));
        
        //printf("knowedge columns: Filename/time/emotion/sun\n");
        //print_matrix("knowedge", knowedge, knowedge_size_x, knowedge_size_y);
        printf("***************\n");
#endif
        
        // Get filename
        std::string s = std::to_string(int(knowedge[currentImage][FILENAME]));
        s.append(".jpg");
        char * name = create_string(s.c_str());
        ReadJPEGFromFile(name, s_intensity);
        
        
        int object = int (knowedge[currentImage][OBJECT]);
      
        // Reset arrays
        reset_array(emotionPos, numberOfObjects);
        reset_array(emotionNeg, numberOfObjects);
        reset_array(novelty, numberOfObjects);
        reset_array(objects, numberOfObjects);

        if (object != O_NONE)
        {
        // Emotion
        if (knowedge[currentImage][EMOTION] > 0)
            emotionPos[object] = knowedge[currentImage][EMOTION];
        else
            emotionNeg[object] = -knowedge[currentImage][EMOTION];
        
        // Novelty
        stimuliCounter[object]++;
        if (stimuliCounter[object] == 1) // First time
            novelty[object] = 1;
        
        // Object
        objects[object] = 1;
        }
    }
    
    imageTickCounter++;
    
#ifdef DEGBUG
    printf("%i ",imageTickCounter);
#endif
    
    if (imageTickCounter == knowedge[currentImage][TIME])
    {
        currentImage++;
        imageTickCounter = 0;
    }
}

// Read a JPEG and fill the intensity matrix
bool black_box_vision::ReadJPEGFromFile(char * file_name, float ** intensity)
{
    long int        buffer_size;
    char *          buffer;
    
    buffer_size =   INITIAL_BUFFER_SIZE;
    buffer      =   (char *)malloc(INITIAL_BUFFER_SIZE*sizeof(char));
    
    FILE * infile;				/* source file */
    if ((infile = fopen(file_name, "rb")) == NULL)
    {
        Notify(msg_fatal_error, "Could not open image file \"%s\" \n", file_name);
        return false;
    }
    
    // Fill buffer
    fseek(infile, 0, SEEK_END);
    long fsize = ftell(infile);
    fseek(infile, 0, SEEK_SET);  //same as rewind(f);
    
    fread(buffer, fsize, 1, infile);
    fclose(infile);
    
    int sizex;
    int sizey;
    int planes;
    
    jpeg_get_info(sizex, sizey, planes, buffer, buffer_size);
    
    // If the images is in RGB format
    float ** red = create_matrix(sizex, sizey);
    float ** green = create_matrix(sizex, sizey);
    float ** blue = create_matrix(sizex, sizey);
    jpeg_decode(red, green, blue, intensity, sizex, sizey, buffer, buffer_size);
    
    free (buffer);
    destroy_matrix(red);
    destroy_matrix(green);
    destroy_matrix(blue);
    
    return true;
}


black_box_vision::~black_box_vision()
{
    delete stimuliCounter;
}

static InitClass init("black_box_vision", &black_box_vision::Create, "Source/UserModules/black_box_vision/");


