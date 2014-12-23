//
//    CIFaceDetector.cc	This file is a part of the IKAROS project
//
//
//    Copyright (C) 2014 Christian Balkenius
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

#include "CIFaceDetector.h"

#import <Cocoa/Cocoa.h>
#import <QuartzCore/QuartzCore.h>


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



static CIDetector * detector;



void
CIFaceDetector::Init()
{
    Bind(max_faces, "max_faces");
    Bind(use_tracking, "use_tracking");

    min_size = clip(GetFloatValue("min_size"), 0.2, 1.0);
    
    input = GetInputMatrix("INPUT");
    output = GetOutputMatrix("OUTPUT");

    face_table = GetOutputMatrix("FACES");
    
    face = GetOutputMatrix("FACE");
    eye_left = GetOutputMatrix("EYE_LEFT");
    eye_right = GetOutputMatrix("EYE_RIGHT");
    
    face_position = GetOutputArray("FACE_POSITION");
    eye_left_position = GetOutputArray("EYE_LEFT_POSITION");
    eye_right_position = GetOutputArray("EYE_RIGHT_POSITION");
    
    edges = GetOutputMatrix("EDGES");
    features = GetOutputMatrix("FEATURES");
    
    face_count = GetOutputArray("FACE_COUNT");
    
    size_x = GetInputSizeX("INPUT");
    size_y = GetInputSizeY("INPUT");
    
    image_data = create_array(4*size_x*size_y);
    
    detector = [CIDetector detectorOfType:CIDetectorTypeFace context:nil
    options:@{  CIDetectorAccuracy : CIDetectorAccuracyHigh,
                CIDetectorTracking : (use_tracking ? @(YES) : @(NO)),
                CIDetectorMinFeatureSize : [NSNumber numberWithFloat: min_size] }];
}



void
CIFaceDetector::Tick()
{
    copy_matrix(output, input, size_x, size_y);

    int k = 0;
    for(int j=0; j<size_y; j++)
        for(int i=0; i<size_x; i++)
        {
            image_data[k++] = input[j][i];
            image_data[k++] = input[j][i];
            image_data[k++] = input[j][i];
            image_data[k++] = 1.0;
        }

    NSData * data   = [NSData dataWithBytes: image_data length: size_x*size_y*4*sizeof(float)];
    CIImage * image = [CIImage imageWithBitmapData: data
                                bytesPerRow: size_x*sizeof(float)*4
                                size: CGSizeMake(float(size_x), float(size_y))
                                format:kCIFormatRGBAf // 128 bits per pixel
                                colorSpace:CGColorSpaceCreateWithName(kCGColorSpaceGenericRGB)];

    NSArray * faces = [detector featuresInImage: image options: @{ CIDetectorSmile: @(YES) }];
    
    face_position[0] = -1;
    face_position[1] = -1;

    reset_matrix(edges, 8, max_faces);
    reset_matrix(features, 6, max_faces);

    int c = 0;
    
    for(CIFaceFeature * f in faces)
    {
        if(c >= max_faces)
            break;

        edges[c][0] = (f.bounds.origin.x) / float(size_x);
        edges[c][1] = 1 - (f.bounds.origin.y) / float(size_y);
        
        edges[c][2] = (f.bounds.origin.x + f.bounds.size.width) / float(size_x);
        edges[c][3] = 1 - (f.bounds.origin.y) / float(size_y);
        
        edges[c][4] = (f.bounds.origin.x + f.bounds.size.width) / float(size_x);
        edges[c][5] = 1 - (f.bounds.origin.y + f.bounds.size.height) / float(size_y);

        edges[c][6] = (f.bounds.origin.x) / float(size_x);
        edges[c][7] = 1 - (f.bounds.origin.y + f.bounds.size.height) / float(size_y);
        



        face_position[0] = (f.bounds.origin.x + 0.5*f.bounds.size.width)/size_x;
        face_position[1] = 1-(f.bounds.origin.y + 0.5*f.bounds.size.height)/size_y;

        

        if(f.hasLeftEyePosition)
        {
            features[c][0] = f.leftEyePosition.x / float(size_x);
            features[c][1] = 1 - f.leftEyePosition.y / float(size_y);
         }
 
        if(f.hasRightEyePosition)
        {
            features[c][2] = f.rightEyePosition.x / float(size_x);
            features[c][3] = 1 - f.rightEyePosition.y / float(size_y);
        }
        
        if(f.hasMouthPosition)
        {
            features[c][4] = f.mouthPosition.x / float(size_x);
            features[c][5] = 1 - f.mouthPosition.y / float(size_y);
        }
 
        printf("%3d %3d ", (f.hasTrackingID ? f.trackingID : 0), (f.hasTrackingFrameCount ? f.trackingFrameCount : 0));
    
        if(f.hasSmile)
        {
            printf("SMILE :-)\n");
        }
        else
            printf("\n");


        // Output face image for face closest to the center and normliaze size for PCA recognition etc
 /*
        reset_matrix(face, 256, 256);
        int fox = face_position[0];
        int foy = face_position[1];
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
*/

        c++;
    }
    
    *face_count = float(c);
 }



/*

void
CIFaceDetector::Tick()
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
*/


static InitClass init("CIFaceDetector", &CIFaceDetector::Create, "Source/Modules/VisionModules/CIFaceDetector/");

