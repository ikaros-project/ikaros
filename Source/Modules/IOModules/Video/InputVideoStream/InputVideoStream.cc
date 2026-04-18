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

#include "FFMpegGrab.h"
#include "ikaros.h"

#if defined(MAC_OS_X)
#include <vecLib/vDSP.h>
#endif

#include <algorithm>
#include <memory>
#include <vector>

using namespace ikaros;

class InputVideoStream : public Module
{
	parameter size_x;
	parameter size_y;
	parameter printInfo;
	parameter url;
	parameter uv4l;
	parameter synchronized_framegrabber;
	parameter synchronized_tick;

	matrix intensity;
	matrix red;
	matrix green;
	matrix blue;
	matrix output;

	std::unique_ptr<FFMpegGrab> framegrabber;
	std::vector<uint8_t> newFrame;

	bool
	InitializeFrameGrabber()
	{
		framegrabber = std::make_unique<FFMpegGrab>();
		framegrabber->SetUv4l(uv4l);
		framegrabber->SetUrl(url.c_str());
		framegrabber->SetPrintInfo(printInfo);
		framegrabber->SetOutputSize(size_x.as_int(), size_y.as_int());
		framegrabber->SetSynchronized(synchronized_framegrabber);
		return framegrabber->Init();
	}

	void
	ConvertFrameToOutputs(const uint8_t * data, int width, int height)
	{
		const int pixel_count = width * height;
		float * r = red.data();
		float * g = green.data();
		float * b = blue.data();
		float * intensity_data = intensity.data();

#if defined(MAC_OS_X)
		const vDSP_Length count = static_cast<vDSP_Length>(pixel_count);
		const float byte_to_float_scale = 1.0f / 255.0f;
		const float one_third = 1.0f / 3.0f;

		vDSP_vfltu8(data, 3, r, 1, count);
		vDSP_vfltu8(data + 1, 3, g, 1, count);
		vDSP_vfltu8(data + 2, 3, b, 1, count);

		vDSP_vsmul(r, 1, &byte_to_float_scale, r, 1, count);
		vDSP_vsmul(g, 1, &byte_to_float_scale, g, 1, count);
		vDSP_vsmul(b, 1, &byte_to_float_scale, b, 1, count);

		std::copy_n(r, pixel_count, intensity_data);
		vDSP_vadd(intensity_data, 1, g, 1, intensity_data, 1, count);
		vDSP_vadd(intensity_data, 1, b, 1, intensity_data, 1, count);
		vDSP_vsmul(intensity_data, 1, &one_third, intensity_data, 1, count);
#else
		const float byte_to_float_scale = 1.0f / 255.0f;
		const float one_third = 1.0f / 3.0f;
		for (int pixel = 0; pixel < pixel_count; ++pixel)
		{
			const float red_value = static_cast<float>(*data++) * byte_to_float_scale;
			const float green_value = static_cast<float>(*data++) * byte_to_float_scale;
			const float blue_value = static_cast<float>(*data++) * byte_to_float_scale;

			*r++ = red_value;
			*g++ = green_value;
			*b++ = blue_value;
			*intensity_data++ = (red_value + green_value + blue_value) * one_third;
		}
#endif
	}

	void
	Init()
	{
		Bind(size_x, "size_x");
		Bind(size_y, "size_y");
		Bind(url, "url");
		Bind(printInfo, "info");
		Bind(uv4l, "uv4l");
		Bind(synchronized_framegrabber, "synchronized_framegrabber");
		Bind(synchronized_tick, "synchronized_tick");

		if (size_x.as_int() <= 0 || size_y.as_int() <= 0)
		{
			Notify(msg_fatal_error, "InputVideoStream requires positive size_x and size_y.");
			return;
		}

		if (url.empty())
		{
			Notify(msg_fatal_error, "InputVideoStream requires a non-empty url.");
			return;
		}

		Bind(intensity, "INTENSITY");
		Bind(red, "RED");
		Bind(green, "GREEN");
		Bind(blue, "BLUE");
		Bind(output, "OUTPUT");

		try
		{
			if (!InitializeFrameGrabber())
			{
				Notify(msg_fatal_error, "Can not start frame grabber");
				return;
			}
		}
		catch (const std::exception &)
		{
			Notify(msg_fatal_error, "Can not create frame grabber");
			return;
		}

		newFrame.resize(static_cast<std::size_t>(size_x.as_int()) *
						static_cast<std::size_t>(size_y.as_int()) * 3);
	}

	void
	Tick()
	{
		if (!framegrabber)
			return;

		const bool wait_for_new_data = synchronized_tick;
		if (!framegrabber->ReadFrame(newFrame.data(), newFrame.size(), wait_for_new_data))
			return;

		ConvertFrameToOutputs(newFrame.data(), size_x.as_int(), size_y.as_int());
	}

	~InputVideoStream() = default;
};

INSTALL_CLASS(InputVideoStream)
