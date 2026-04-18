//
//	  FFMpegGrab.h		This file is a part of the IKAROS project
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

#ifndef FFMpegGrab_h
#define FFMpegGrab_h

#include <cstddef>
#include <cstdint>
#include <memory>

class FFMpegGrab
{
	public:
	
	FFMpegGrab();
	~FFMpegGrab();

	FFMpegGrab(const FFMpegGrab &) = delete;
	FFMpegGrab & operator=(const FFMpegGrab &) = delete;

	void SetOutputSize(int width, int height);
	void SetUrl(const char * stream_url);
	void SetPrintInfo(bool enabled);
	void SetUv4l(bool enabled);
	void SetSynchronized(bool enabled);
	bool Init();
	bool ReadFrame(uint8_t * destination, std::size_t bytes, bool wait_for_new_frame);

	private:
	void loop();
	void resetDecoderState();

	struct Impl;
	std::unique_ptr<Impl> impl_;
};

#endif
