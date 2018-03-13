//
//	  InputVideoStream.cc		This file is a part of the IKAROS project
//
//    Copyright (C) 2018 Birger Johansson
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
//
// Code inspired from:
// https://www.ffmpeg.org/doxygen/2.8/examples.html
// http://dranger.com/ffmpeg/
// https://sourceforge.net/u/leixiaohua1020/profile/
// https://blogs.gentoo.org/lu_zero/2016/03/29/new-avcodec-api/
// And enormous amount of googleing...

#include "InputVideoStream.h"

#include <unistd.h>
#include <thread>

using namespace std;
using namespace ikaros;

InputVideoStream::InputVideoStream(Parameter * p):
Module(p)
{
	url = GetValue("url");
	if (url == NULL)
	{
		Notify(msg_fatal_error, "No input file parameter supplied.\n");
		return;
	}
	
	size_x = GetIntValue("size_x");
	size_y = GetIntValue("size_y");
	
	printInfo = GetBoolValue("info");
	
	uv4l = GetBoolValue("uv4l"); // Tune code fore uv4l server
	
	syncGrab = GetBoolValue("syncronized_framegrabber");
	syncTick = GetBoolValue("syncronized_tick");
	
	// Create a int to float table to speed up int to float cast.
	for (int i = 0;i <= 255; i++)
		convertIntToFloat[i] = 1.0/255.0 * i;
	
	// Create a grabber
	framegrabber = new FFMpegGrab();
	
	// Set paramters.
	framegrabber->uv4l = uv4l;
	framegrabber->url = url; // copy string?
	framegrabber->printInfo = printInfo;
	
	framegrabber->outputSizeX = size_x;
	framegrabber->outputSizeY = size_y;
	framegrabber->syncronized = syncGrab;
	
	
	// Check sizes
	//	if(size_x == 0 || size_y == 0)
	//	{
	//		size_x = framegrabber->native_size_x;
	//		size_y = framegrabber->native_size_y;
	//	}
	//size_x = framegrabber->inputSizeX;
	//size_y = framegrabber->inputSizeY;
	
	// Create memory
	newFrame = new uint8_t[size_x*size_y*3*sizeof(uint8_t)];
	
	// Init grabber
	if (!framegrabber->Init())
		Notify(msg_fatal_error, "Can not start frame grabber.");
	
	AddOutput("INTENSITY", false, size_x, size_y);
	AddOutput("RED", false, size_x, size_y);
	AddOutput("GREEN", false, size_x, size_y);
	AddOutput("BLUE", false, size_x, size_y);
}
void
InputVideoStream::Init()
{
	intensity	= GetOutputArray("INTENSITY");
	red			= GetOutputArray("RED");
	green		= GetOutputArray("GREEN");
	blue		= GetOutputArray("BLUE");
}

void
InputVideoStream::Tick()
{
	std::mutex mtx;           // mutex for critical section
	Timer timer;

	// Modes
	// Get all frames (waitForNewData + syncronized)
	// Get latest frame	(waitForNewData + !syncronized)
	// Get a frame every tick (could be the same as last tick) 	(!waitForNewData + !syncronized)
	
	bool waitForNewData = syncTick;
	bool gotNewData = false;

	mtx.lock();
	gotNewData = framegrabber->freshData;
	mtx.unlock();
	
	if (!gotNewData and waitForNewData) // No new data
		while (!gotNewData) // wait until new data
		{
			mtx.lock();
			gotNewData = framegrabber->freshData;
			mtx.unlock();
			usleep(1000);
		}
	
#ifdef DEBUGTIMER
	if (gotNewData)
	{
		printf("InputVideoStream:We got an new image Time %f\n", timer.GetTime());
		timer.Restart();
	}
#endif

	if (!gotNewData) // No need to int->float convert again. Use last output.
		return;
	
	mtx.lock();
	memcpy(newFrame, framegrabber->ikarosFrame, size_x*size_y*3*sizeof(uint8_t));
	framegrabber->freshData = 0;
	mtx.unlock();
#ifdef DEBUGTIMER
		printf("InputVideoStream: memcpy %f\n", timer.GetTime());
		timer.Restart();
#endif
	unsigned char * data = newFrame;
	
//	float * r = red;
//	float * g = green;
//	float * b = blue;
//	float * inte = intensity;
//	int t = 0;
	
	// Singel core
//	while(t++ < size_x*size_y)
//	{
//		*r++       = convertIntToFloat[*data++];
//		*g++       = convertIntToFloat[*data++];
//		*b++       = convertIntToFloat[*data++];
//		*inte++ = *r + *g + *b;

//		*r       = *data++;
//		*g       = *data++;
//		*b       = *data++;
//		*r++ = (*data >> 8) & 0xFF;
//		*g++ = (*data >> 8) & 0xFF;
//		*b++ = *data & 0xFF;
//		data++;
//		data++;
//		data++;
		//*inte++ = *r++ + *g++ + *b++;
//	}

//		const int nrOfCores = 3;//thread::hardware_concurrency();
//		// Create some threads pointers
//		thread tConv[nrOfCores];
//		for(int j=0; j<nrOfCores; j++)
//		{
//			tConv[j] = thread([&,j]()
//							  {
//								  int l = 0;
//								  unsigned char * d = data;
//								  float * p = NULL;
//								  switch (j) {
//									  case 0:
//										  p = r;
//										  break;
//									  case 1:
//										  p = g;
//										  d++;
//										  break;
//									  case 2:
//										  p = b;
//										  d++;
//										  d++;
//										  break;
//									  default:
//
//										  break;
//								  };
//
//								  while(l++ < size_x*size_y){
//									  *p++  = convertIntToFloat[*d];
//									  d++;
//									  d++;
//									  d++;
//								  }
//							  });
//		}
//
//		for(int j=0; j<nrOfCores; j++)
//			tConv[j].join();

	
		// Multi core 2
		const int nrOfCores = 4;
		// Create some threads pointers
		thread tConv[nrOfCores];
		//printf("Number of threads %i\n",thread::hardware_concurrency());
		int span = size_x*size_y/nrOfCores;

		for(int j=0; j<nrOfCores; j++)
		{
			tConv[j] = thread([&,j]()
							  {
								  float * r = red;
								  float * g = green;
								  float * b = blue;
								  float * inte = intensity;
								  int t = 0;

								  unsigned char * d = data;

								  for (int i = 0; i < j*span; i++)
								  {
									  d++;
									  d++;
									  d++;
									  r++;
									  g++;
									  b++;
									  inte++;
								  }
								  while(t++ < span)
								  {
//									  *r       = *convertIntToFloat*(*d++);
//									  *g       = *convertIntToFloat*(*d++);
//									  *b       = *convertIntToFloat*(*d++);
//									  *inte++ = *r++ + *g++ + *b++;
									  *r       = convertIntToFloat[*d++];
									  *g       = convertIntToFloat[*d++];
									  *b       = convertIntToFloat[*d++];
//									  *r       = convertIntToFloat[*d & 0xFF];
//									  *g       = convertIntToFloat[(*d >> 8) & 0xFF];
//									  *b       = convertIntToFloat[(*d >> 16) & 0xFF];
//									  d++;d++;d++;

									  *inte++ = *r++ + *g++ + *b++;
								  }
							  });

		}
		for(int j=0; j<nrOfCores; j++)
			tConv[j].join();
	
#ifdef DEBUGTIMER
	printf("InputVideoStream: float convert %f\n", timer.GetTime());
	timer.Restart();
#endif
	
}

InputVideoStream::~InputVideoStream()
{
	delete newFrame;
	delete framegrabber;
}


static InitClass init("InputVideoStream", &InputVideoStream::Create, "Source/Modules/IOModules/Video/InputVideoStream/");
