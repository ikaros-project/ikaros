//
//	MarkerTracker		This file is a part of the IKAROS project
//                      Wrapper module for ARToolKitPlus available at:
//                      https://launchpad.net/artoolkitplus
//
//    Copyright (C) 2011-2012 Christian Balkenius
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
//    Created: 2011-12-09

#include "MarkerTracker.h"

#include <ARToolKitPlus/Tracker.h>

using namespace ikaros;
using namespace ARToolKitPlus;

const bool useBCH = true;
typedef struct { ARFloat para[16];} ARFloatArray16;


static float **
center_sort_rows(float ** m, int size_x, int size_y)
{
    float * t = create_array(size_x);
    int n = size_y;
    do {
        int nn = 0;
        for(int i = 1; i < n; i++)
        {
            if(hypot(m[i-1][0]-0.5, m[i-1][1]-0.5) < hypot(m[i][0]-0.5, m[i][1]-0.5))
            {
                // swap rows i-1 and i
                
                copy_array(t, m[i-1], size_x);
                copy_array(m[i-1], m[i], size_x);
                copy_array(m[i], t, size_x);

                nn = i;
            }
        }
        n = nn;
    } while(n > 0);

    destroy_array(t);
    return m;
}



// World data structure

const int marker_pos_x = 0;
const int marker_pos_y = 1;
const int marker_id = 2;
const int marker_confidence = 3;
const int marker_corners = 4;
const int trans_matrix_cam_coord = 12;



// This class is only necessary because the variables we want to set are protected;
// Maybe we should make the class a friend instead?

class MTCamera: public Camera
{
public:
    MTCamera() : Camera() {};
    ~MTCamera() {};

    bool setCameraCalibration(float * calibration);
};



bool
MTCamera::setCameraCalibration(float * calibration)
{
    xsize = int(calibration[0]);
    ysize = int(calibration[1]);
    cc[0] = calibration[2];
    cc[1] = calibration[3];
    fc[0] = calibration[4];
    fc[1] = calibration[5];
    kc[0] = calibration[6];
    kc[1] = calibration[7];
    kc[2] = calibration[8];
    kc[3] = calibration[9];
    kc[4] = calibration[10];
    kc[5] = calibration[11];
    undist_iterations = int(calibration[12]);

    undist_iterations = min(undist_iterations, 20); // CAMERA_ADV_MAX_UNDIST_ITERATIONS

    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 4; j++)
            mat[i][j] = 0.;

    mat[0][0] = fc[0]; // fc_x
    mat[1][1] = fc[1]; // fc_y
    mat[0][2] = cc[0]; // cc_x
    mat[1][2] = cc[1]; // cc_y
    mat[2][2] = 1.0;

    return true;
}



class MFTracker: public Tracker
{
public:
    MFTracker(int imWidth, int imHeight, int maxImagePatterns = 8, int pattWidth = 6, int pattHeight = 6, int pattSamples = 24, int maxLoadPatterns = 0, int threshold=127);
    virtual bool init(float * calibration, ARFloat nNearClip, ARFloat nFarClip);
    virtual void calc(const uint8_t* nImage, float marker_size, float ** marker_sizes, int marker_size_ranges, bool use_history);
    virtual void setPatternWidth(ARFloat nWidth) { patt_width = nWidth; }

    void GetCorner(int marker, int edge, float & x, float & y);
    void GetEdge(int marker, int edge, float &x0, float & y0, float & x1, float & y1);
   
    ARFloat confidence;
    ARFloat patt_width;
    ARFloat patt_center[2];
    ARFloat patt_trans[3][4];

    ARMarkerInfo *marker_info;
    ARFloatArray16 * marker_gl;
    float ***   marker_matrix;
    int marker_num;
};



MFTracker::MFTracker(int imWidth, int imHeight, int maxImagePatterns, int pattWidth, int pattHeight,
        int pattSamples, int maxLoadPatterns, int threshold) :
    Tracker(imWidth, imHeight, maxImagePatterns, pattWidth, pattHeight, pattSamples, maxLoadPatterns)
{
    thresh = threshold;
    patt_width = 2.0;
    patt_center[0] = patt_center[1] = 0.0;
    
}



bool
MFTracker::init(float * calibration, ARFloat nNearClip, ARFloat nFarClip)
{
    if (!this->checkPixelFormat())
        return false;

    if (this->marker_infoTWO == NULL)
        this->marker_infoTWO = new ARMarkerInfo2[MAX_IMAGE_PATTERNS];

    this->marker_gl = new ARFloatArray16[MAX_IMAGE_PATTERNS];
    
    marker_matrix = create_matrix(4, 4, MAX_IMAGE_PATTERNS);
    
    MTCamera * cam = new MTCamera();

    cam->setCameraCalibration(calibration);

    if (arCamera)
        delete arCamera;

    arCamera = NULL;

    setCamera(cam, nNearClip, nFarClip);

    return true;
}



void
MFTracker::calc(const uint8_t* nImage, float marker_size, float ** marker_sizes, int marker_size_ranges, bool use_history = false)
{
    if (nImage == NULL)
        return;

    // Rest confidences
    
    for (int i = 0; i < marker_num; i++)
        marker_info[i].cf = 0;
    
    // detect the markers in the video frame
    
    if(use_history)
    {
        if(arDetectMarker(const_cast<unsigned char*> (nImage), this->thresh, &marker_info, &marker_num) < 0) // Uses tracking history
            return;
    }
    else  // Do not use tracking history
    {
        if(arDetectMarkerLite(const_cast<unsigned char*> (nImage), this->thresh, &marker_info, &marker_num) < 0)
            return;
    }
    
    
    // calculate all matrices
    
    for (int i = 0; i < marker_num; i++)
        if (marker_info[i].id != -1)
        {
            // Lookup size
            
            if(marker_sizes != NULL)
                for(int j=0; j<marker_size_ranges; j++)
                    if(int(marker_sizes[j][0]) <= marker_info[i].id && marker_info[i].id <= int(marker_sizes[j][1]))
                    {
                        marker_size = marker_sizes[j][2];
                        break;
                    }

            // Estimate position
            
            float err = executeSingleMarkerPoseEstimator(&marker_info[i], patt_center, marker_size, patt_trans); 
            if(err > 3) // > 3 == err, not visible
                marker_info[i].cf = 0;

            convertTransformationMatrixToOpenGLStyle(patt_trans, marker_gl[i].para);
            
            float s = 0;
            for(int x=0; x<4; x++)
                for(int y=0; y<4; y++)
                    s += marker_matrix[i][y][x] = marker_gl[i].para[4*x+y];
        }
}



void
MFTracker::GetCorner(int marker, int edge, float & x, float & y)
{
    int dir = marker_info[marker].dir;
    x = marker_info[marker].vertex[(4+edge-dir)%4][0];
    y = marker_info[marker].vertex[(4+edge-dir)%4][1];
}



void
MFTracker::GetEdge(int marker, int edge, float & x0, float & y0, float & x1, float & y1)
{
    int dir = marker_info[marker].dir;
    x0 = marker_info[marker].vertex[(4+edge-dir)%4][0];
    y0 = marker_info[marker].vertex[(4+edge-dir)%4][1];
    x1 = marker_info[marker].vertex[(4+edge-dir+1)%4][0];
    y1 = marker_info[marker].vertex[(4+edge-dir+1)%4][1];
}



static float **
create_matrix_view(float * data, int sizex, int sizey)
{
    float ** b = (float **)malloc(sizey*sizeof(float *));
    for (int j=0; j<sizey; j++)
        b[j] = &data[j*sizex];
    return b;
}



static void
destroy_matrix_view(float ** m)
{
    free(m);
}



void
MarkerTracker::Init()
{
    calibration = GetArray("calibration", 13);
    max_markers = GetIntValue("max_markers");
    sort_markers = GetBoolValue("sort_makers");
    use_history = GetBoolValue("use_history");
    
    auto_threshold = equal_strings(GetValue("threshold"), "auto");
    if(auto_threshold)
        threshold = 127;
    else
        threshold = int(255.0*GetFloatValue("threshold"));

    markers = GetOutputMatrix("MARKERS");
    marker_count = GetOutputArray("MARKER_COUNT");

    input = GetInputMatrix("INPUT");

    max_positions = GetOutputSizeY("POS");
     
    size_x = GetInputSizeX("INPUT");
    size_y = GetInputSizeY("INPUT");
    
    // Check size of size data and get matrix
    
    int x;
    marker_sizes = create_matrix(GetValue("marker_size"), x, marker_size_ranges);
    
    // Get single value or default size
    
    if(marker_size_ranges <= 1)
    {
        marker_size = GetFloatValue("marker_size");
        marker_size_ranges = 0;
    }
    
    tracker = new MFTracker(size_x, size_y, max_markers, 6, 6, 6, 0, threshold);
    tracker->setPixelFormat(ARToolKitPlus::PIXEL_FORMAT_LUM);
//    tracker->setPoseEstimator(POSE_ESTIMATOR_RPP);
//    tracker->setImageProcessingMode(ARToolKitPlus::IMAGE_HALF_RES);

    if (!tracker->init(calibration, 1.0f, 1000.0f))
    {
        Notify(msg_fatal_error, "Could not find camera calibration data\n");
    }

    tracker->setBorderWidth(useBCH ? 0.125 : 0.25);
    tracker->activateAutoThreshold(auto_threshold);
    tracker->setUndistortionMode(ARToolKitPlus::UNDIST_LUT); // ARToolKitPlus::UNDIST_LUT, UNDIST_NONE
    tracker->setMarkerMode(useBCH ? ARToolKitPlus::MARKER_ID_BCH : ARToolKitPlus::MARKER_ID_SIMPLE);
    
    buffer = new unsigned char [size_x*size_y];
    
    coordinate_system =   GetIntValueFromList("coordinate_system");

}



MarkerTracker::~MarkerTracker()
{
    delete [] buffer;
}



void
MarkerTracker::Tick()
{		 
    reset_matrix(markers, max_markers, 28);
    for(int i=0; i<max_markers; i++)
    {
        markers[i][0] = -1;
        markers[i][1] = -1;
    }

    *marker_count = 0;
    
    if(norm(input, size_x, size_y) == 0) // No image, return
        return;
    
    float_to_byte(buffer, *input, 0, 1, size_x*size_y);
    
    tracker->calc(buffer, marker_size, marker_sizes, marker_size_ranges, use_history);

    int ix = 0;
    for (int j = 0; j<tracker->marker_num; j++)
    {
        if(ix < max_markers && tracker->marker_info[j].cf > 0.7)
        {
            float ** m = create_matrix_view(&markers[ix][trans_matrix_cam_coord], 4, 4);
            
            copy_matrix(m, tracker->marker_matrix[j], 4, 4); // In ARToolkit coordinate system
            
            if (coordinate_system == 1) // Convert to ikaros coordinate system z = x', -x = y', -y = z'.
            {
                printf("Changing to Ikaros base\n");
                h_matrix baseChange;
                h_matrix rotY;
                h_matrix rotZ;
                h_rotation_matrix(rotZ, Z, pi/2);
                h_rotation_matrix(rotY, Y, -pi/2);
                h_multiply(baseChange, rotZ, rotY); // Create a rotation matrix
                h_multiply(*m, *m, baseChange); // Changing coordinate system
            }
            
            destroy_matrix_view(m);
            
            markers[ix][marker_pos_x] = tracker->marker_info[j].pos[0]/float(size_x);
            markers[ix][marker_pos_y] = tracker->marker_info[j].pos[1]/float(size_y);
            
            markers[ix][marker_id] = float(tracker->marker_info[j].id);
            markers[ix][marker_confidence] = tracker->marker_info[j].cf;
            
            int p = marker_corners;
            for(int l=0; l<4; l++)
            {
                float x, y;
                tracker->GetCorner(j, l, x, y);

                markers[ix][p++] = x/float(size_x);
                markers[ix][p++] = y/float(size_y);
            }
            
            ix++;
        }
    }

    if(sort_markers)
        center_sort_rows(markers, 28, ix);
    
    *marker_count = float(ix);
}


static InitClass init("MarkerTracker", &MarkerTracker::Create, "Source/Modules/VisionModules/MarkerTracker/");

