//
//	FFT.cc		This file is a part of the IKAROS project
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

#include "FFT.h"
#include "AudioFFT.h"
// use the ikaros namespace to access the math library
// this is preferred to using math.h

using namespace ikaros;


void
FFT::Init()
{
    Bind(debugmode, "debug");    

    io(input, input_size, "INPUT"); // must be power of 2

    io(real_output, output_size, "REAL_OUTPUT");
    io(im_output, output_size, "IM_OUTPUT");


    internal_array = create_array(10);
}



FFT::~FFT()
{
    // Destroy data structures that you allocated in Init.
    destroy_array(internal_array);
}



void
FFT::Tick()
{
    if (debugmode)
    {
        // print out debug info
        printf("Instance: %s\n", this->instance_name);
        printf("inputsize: %i, outputsize: %i\n", input_size, output_size);
        print_array("Input: ", input, input_size);
    }
    const size_t fftSize = input_size; // Needs to be power of 2!

    // std::vector<float> input(fftSize, 0.0f);
    // std::vector<float> re(audiofft::AudioFFT::ComplexSize(fftSize));
    // std::vector<float> im(audiofft::AudioFFT::ComplexSize(fftSize));
    // std::vector<float> output(fftSize);

    audiofft::AudioFFT fft;
    fft.init(fftSize);
    fft.fft(input, real_output, im_output);
    // fft.ifft(output.data(), re.data(), im.data());
    
}



// Install the module. This code is executed during start-up.

static InitClass init("FFT", &FFT::Create, "Source/Modules/UtilityModules/FFT/");


