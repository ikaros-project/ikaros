//
//    MPIFaceDetector.cc	This file is a part of the IKAROS project
//                          Wrapper for MPISearch and MPIEyeFinder from the Machine Perception Toolbox
//
//    Copyright (C) 2009-2012 Christian Balkenius
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
//	Created: 2009-01-27
//


#include "MPIFaceDetector.h"

#include "mpisearchFaceDetector.h"
#include "eyefinderBinary.h"

using namespace ikaros;

static void
draw_rectangle(float ** image, int x0, int y0, int x1, int y1, int size_x, int size_y)
{
    x0 = max(x0, 0);
    y0 = max(y0, 0);
    x1 = min(x1, size_x-1);
    y1 = min(y1, size_y-1);
    
    for(int x=x0; x<x1; x++)
    {
        image[y0][x] = 0;
        image[y1][x] = 0;
    }

    for(int y=y0; y<y1; y++)
    {
        image[y][x0] = 0;
        image[y][x1] = 0;
    }
}



void
MPIFaceDetector::Init()
{
    input = GetInputMatrix("INPUT");
    output = GetOutputMatrix("OUTPUT");

    face_table = GetOutputMatrix("FACES");
    face = GetOutputMatrix("FACE");
    eye_left = GetOutputMatrix("EYE_LEFT");
    eye_right = GetOutputMatrix("EYE_RIGHT");
    
    face_position = GetOutputArray("FACE_POSITION");
    eye_left_position = GetOutputArray("EYE_LEFT_POSITION");
    eye_right_position = GetOutputArray("EYE_RIGHT_POSITION");
    
    size_x = GetInputSizeX("INPUT");
    size_y = GetInputSizeY("INPUT");    
}



void
MPIFaceDetector::Tick()
{
    MPISearchFaceDetector mpi;
    MPEyeFinder * eyefinder = new MPEyeFinderBinary();
    
    RImage<float> pixels(*input, size_x, size_y);
    eyefinder->initStream(pixels.width, pixels.height);
    
    VisualObject faces;
    eyefinder->findEyes(pixels, faces, 1.25, wt_avg);    
    
    copy_matrix(output, input, size_x, size_y);
    
    list<VisualObject *>::iterator cur_face = faces.begin();

    // copy data from first face
    
    FaceObject *face1 = static_cast<FaceObject*>(*cur_face);
    
    if(!faces.empty())
    {
        face_position[0] = (face1->x + 0.5*face1->xSize)/size_x;
        face_position[1] = (face1->y + 0.5*face1->ySize)/size_y;
        
        reset_matrix(face, 256, 256);
        int fox = face1->x+0.5*face1->xSize-128;
        int foy = face1->y+0.5*face1->ySize-128;
        for(int y=0; y<256; y++)
            for(int x=0; x<256; x++)
            {
                int yy = foy+y;
                int xx = fox+x;
                int xd = x;
                int yd = y;
                if(0<=xx && xx<size_x && 0<=yy && yy<size_y)
                    face[yd][xd] = input[yy][xx];
            }
        
        eye_left_position[0] = face1->eyes.xLeft/size_x;
        eye_left_position[1] = face1->eyes.yLeft/size_y;
        
        int eyeSize = min(25, static_cast<int>(face1->xSize * 0.15)); // min to avoid overflow
        reset_matrix(eye_left, 50, 50);
        for(int y=0; y<min(50, 2*eyeSize); y++)
            for(int x=0; x<min(50, 2*eyeSize); x++)
            {
                int yy = face1->eyes.yLeft-eyeSize+y;
                int xx = face1->eyes.xLeft-eyeSize+x;
                if(xx >= 0 && yy >= 0)
                    eye_left[y+25-eyeSize][x+25-eyeSize] = input[yy][xx];
            }

        eye_right_position[0] = face1->eyes.xRight/size_x;
        eye_right_position[1] = face1->eyes.yRight/size_y;
        
        reset_matrix(eye_right, 50, 50);
        for(int y=0; y<min(50, 2*eyeSize); y++)
            for(int x=0; x<min(50, 2*eyeSize); x++)
            {
                int yy = face1->eyes.yRight-eyeSize+y;
                int xx = face1->eyes.xRight-eyeSize+x;
                if(xx >= 0 && yy >= 0)
                    eye_right[y+25-eyeSize][x+25-eyeSize] = input[yy][xx];
            }
    }

    // mark all faces in output image and store in faces output
    
    for(int i=0; i<10; i++)
    {
        face_table[i][0] = -1;
        face_table[i][1] = -1;
        face_table[i][2] = 0;
        face_table[i][3] = 0;
    }
    
    int i=0;
	for( ; cur_face != faces.end(); ++cur_face)
    {
        FaceObject *fo = static_cast<FaceObject*>(*cur_face);
        draw_rectangle(output, fo->x, fo->y, fo->x+fo->xSize, fo->y+fo->ySize, size_x, size_y);
        
        if(i<10)
        {
            face_table[i][0] = (fo->x+0.5*fo->xSize)/size_x;
            face_table[i][1] = (fo->y+0.5*fo->ySize)/size_y;
            face_table[i][2] = fo->xSize/size_x;
            face_table[i][3] = fo->ySize/size_y;
            i++;
        }
        int eyeSize = static_cast<int>(fo->xSize * 0.1);
        
        //lefteye
        draw_rectangle(output,  max(fo->eyes.xLeft-eyeSize, 0.0f), 
                                max(fo->eyes.yLeft-eyeSize, 0.0f),
                                min(fo->eyes.xLeft+eyeSize, (float)pixels.width),
                                min(fo->eyes.yLeft+eyeSize, (float)pixels.height),
                                size_x, size_y);
        //righteye
        draw_rectangle(output,  max(fo->eyes.xRight-eyeSize, 0.0f), 
                                max(fo->eyes.yRight-eyeSize, 0.0f),
                                min(fo->eyes.xRight+eyeSize, (float)pixels.width),
                                min(fo->eyes.yRight+eyeSize, (float)pixels.height),
                                size_x, size_y);
    }
   
    delete eyefinder;
}



static InitClass init("MPIFaceDetector", &MPIFaceDetector::Create, "Source/Modules/VisionModules/MPIFaceDetector/");

