/*
 *  EyeTracker.h
*/

// Initialize EyeTracker
//
// Typical call for 64x64 image: EyeTracker_Init(64, 64, 3, 0.5, 5, 40, 5, 15, 2);
//

void		EyeTracker_Init(int image_width, int image_height, int filter_radius, float filter_scale, float dist_min, float dist_max, float dist_min2, float dist_max2, int density);

// Find iris in the image; returns coordinates in x and y

void		EyeTracker_FindIris(int * image,  float * x, float * y);

