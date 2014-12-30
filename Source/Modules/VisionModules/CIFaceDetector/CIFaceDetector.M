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


void
CIFaceDetector::Init()
{
    Bind(max_faces, "max_faces");
    Bind(use_tracking, "use_tracking");
    Bind(mouth_correction, "mouth_correction");
    Bind(detect_smiles, "detect_smiles");
    Bind(detect_blinks, "detect_blinks");

    min_size = clip(GetFloatValue("min_size"), 0.2, 1.0);
    
    input = GetInputMatrix("INPUT");
    
    size_x = GetInputSizeX("INPUT");
    size_y = GetInputSizeY("INPUT");
    
    face_position = GetOutputMatrix("FACE_POSITION");
    face_size = GetOutputMatrix("FACE_SIZE");
    face_bounds = GetOutputMatrix("FACE_BOUNDS");
    eye_left_position = GetOutputMatrix("EYE_LEFT_POSITION");
    eye_right_position = GetOutputMatrix("EYE_RIGHT_POSITION");
    mouth_position = GetOutputMatrix("MOUTH_POSITION");
    rotation = GetOutputArray("ROTATION");
    blink_left = GetOutputArray("BLINK_LEFT");
    blink_right = GetOutputArray("BLINK_RIGHT");
    smile = GetOutputArray("SMILE");
    novelty = GetOutputArray("NOVELTY");
    life = GetOutputArray("LIFE");
    object_id = GetOutputArray("OBJECT_ID");

    
    face_count = GetOutputArray("FACE_COUNT");
    
    image_data = create_array(4*size_x*size_y);
    
    detector = [CIDetector detectorOfType:CIDetectorTypeFace context:nil
    options:@{  CIDetectorAccuracy : CIDetectorAccuracyHigh,
                CIDetectorTracking : (use_tracking ? @(YES) : @(NO)),
                CIDetectorMinFeatureSize : [NSNumber numberWithFloat: min_size] }];
}


CIFaceDetector::~CIFaceDetector()
{
    destroy_array(image_data);
    [(CIDetector *)detector release];
}



void
CIFaceDetector::Tick()
{
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

    NSArray * faces = [(CIDetector *)detector featuresInImage: image options: @{
        CIDetectorSmile: (detect_smiles ? @(YES) : @(NO)),
        CIDetectorEyeBlink: (detect_blinks ? @(YES) : @(NO)),
    }];
    
    reset_matrix(face_bounds, 8, max_faces);
    reset_matrix(face_position, 2, max_faces);
    reset_matrix(face_size, 2, max_faces);
    reset_matrix(eye_left_position, 2, max_faces);
    reset_matrix(eye_right_position, 2, max_faces);
    reset_matrix(mouth_position, 2, max_faces);
    reset_array(rotation, max_faces);

    reset_array(smile, max_faces);
    reset_array(novelty, max_faces);
    reset_array(object_id, max_faces);
    reset_array(life, max_faces);

    int c = 0;
    
    for(CIFaceFeature * f in faces)
    {
        if(c >= max_faces)
            break;

        face_bounds[c][0] = (f.bounds.origin.x) / float(size_x);
        face_bounds[c][1] = 1 - (f.bounds.origin.y) / float(size_y);
        
        face_bounds[c][2] = (f.bounds.origin.x + f.bounds.size.width) / float(size_x);
        face_bounds[c][3] = 1 - (f.bounds.origin.y) / float(size_y);
        
        face_bounds[c][4] = (f.bounds.origin.x + f.bounds.size.width) / float(size_x);
        face_bounds[c][5] = 1 - (f.bounds.origin.y + f.bounds.size.height) / float(size_y);

        face_bounds[c][6] = (f.bounds.origin.x) / float(size_x);
        face_bounds[c][7] = 1 - (f.bounds.origin.y + f.bounds.size.height) / float(size_y);
        

        face_position[c][0] = (f.bounds.origin.x + 0.5*f.bounds.size.width)/size_x;
        face_position[c][1] = 1-(f.bounds.origin.y + 0.5*f.bounds.size.height)/size_y;

        face_size[c][0] = f.bounds.size.width/size_x;
        face_size[c][1] = f.bounds.size.height/size_y;
    
        if(f.hasLeftEyePosition)
        {
            eye_left_position[c][0] = f.leftEyePosition.x / float(size_x);
            eye_left_position[c][1] = 1 - f.leftEyePosition.y / float(size_y);
         }
 
        if(f.hasRightEyePosition)
        {
            eye_right_position[c][0] = f.rightEyePosition.x / float(size_x);
            eye_right_position[c][1] = 1 - f.rightEyePosition.y / float(size_y);
        }
        
        if(f.hasMouthPosition)
        {
            if(!mouth_correction)
            {
                mouth_position[c][0] = f.mouthPosition.x / float(size_x);
                mouth_position[c][1] = 1 - f.mouthPosition.y / float(size_y);
            }
            
            else if(f.hasLeftEyePosition && f.hasRightEyePosition)
            {
                float p0x = (f.leftEyePosition.x + f.rightEyePosition.x)/2;
                float p0y = (f.leftEyePosition.y + f.rightEyePosition.y)/2;
                
                float dx = f.rightEyePosition.x - f.leftEyePosition.x;
                float dy = f.rightEyePosition.y - f.leftEyePosition.y;

                // calculate direction to mouth
                
                float mdx = + dy;
                float mdy = - dx;

                float mvx = f.mouthPosition.x - p0x;
                float mvy = f.mouthPosition.y - p0y;
                
                float d = (mdx*mvx+mdy*mvy)/(hypot(dx,dy)*hypot(mvx,mvy));
                
                float mpx =  p0x + d * mdx;
                float mpy =  p0y + d * mdy;
 
                mouth_position[c][0] = mpx/ float(size_x);
                mouth_position[c][1] = 1 - mpy / float(size_y);

            }
        }

        rotation[c] = (f.hasFaceAngle ? f.faceAngle : 0);
        blink_left[c] = (f.leftEyeClosed ? 1 : 0);
        blink_right[c] = (f.rightEyeClosed ? 1 : 0);
        smile[c] = (f.hasSmile ? 1 : 0);
        novelty[c] = (f.hasTrackingFrameCount && f.trackingFrameCount == 1 ? 1 : 0);
        life[c] = (f.hasTrackingFrameCount ? f.trackingFrameCount : 0);
        object_id[c] = (f.hasTrackingID ? f.trackingID : 0);
    
        c++;
    }
    
    *face_count = float(c);
 }



static InitClass init("CIFaceDetector", &CIFaceDetector::Create, "Source/Modules/VisionModules/CIFaceDetector/");

