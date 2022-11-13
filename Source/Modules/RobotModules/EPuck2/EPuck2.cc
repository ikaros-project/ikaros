//
//	EPuck2.cc		This file is a part of the IKAROS project
//
//    Copyright (C) 2012 <Author Name>
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
//  This example is intended as a starting point for writing new Ikaros modules
//  The example includes most of the calls that you may want to use in a module.
//  If you prefer to start with a clean example, use he module MinimalModule instead.
//

#include "EPuck2.h"
#include <iostream>
// use the ikaros namespace to access the math library
// this is preferred to using math.h

using namespace ikaros;
//using namespace cv;

void
EPuck2::Init()
{
    Bind(parameter, "parameter1");
	Bind(debugmode, "debug");    

    // io(torqueEnable, torqueEnableSize, "TORQUE_ENABLE");
    io(input_motors, motor_size, "MOTORS");
    io(camera_grey, camera_x, camera_y, "CAMERA_GREY");
    internal_array = create_array(10);

    device_index = 0;
    camera = new cv::VideoCapture(device_index);
    if(!camera->isOpened()) {
		std::cout << "EPuck2: Cannot open the video device" << std::endl;
		//exit(1);
	}
    image = new cv::Mat();

}



EPuck2::~EPuck2()
{
    // Destroy data structures that you allocated in Init.
    destroy_array(internal_array);
    // delete camera;
    // delete image;
}



void
EPuck2::Tick()
{
    camera->read(*image);
    if(image->empty()) {
        std::cerr << "frame not grabbed properly" << std::endl;
        //return -1;
    }
    else {
        cv::Mat E = (*image)(cv::Range(1, 3), cv::Range(1,3));
        std::cout << "E = " << std::endl << " " << E << "\n" << "\n";
        std::vector<uchar> array;
        if (image->isContinuous()) {
            // array.assign(mat.datastart, mat.dataend); // <- has problems for sub-matrix like mat = big_mat.row(i)
            array.assign(image->data, image->data + image->total()*image->channels());
        }

    }
	if(debugmode)
	{
		// print out debug info
	}
}



// Install the module. This code is executed during start-up.

static InitClass init("EPuck2", &EPuck2::Create, "Source/Modules/RobotModules/EPuck2/");


