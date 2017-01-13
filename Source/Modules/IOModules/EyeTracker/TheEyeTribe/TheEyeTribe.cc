//
//	TheEyeTribe.cc		This file is a part of the IKAROS project
//
//    Copyright (C) 2016 Birger Johansson
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
//    See http://www.ikaros-project.org/ f§or more information.
//

#include "TheEyeTribe.h"

using namespace ikaros;

void
TheEyeTribe::Init()
{
	// Create a Gaze API
	
	// Try to connect to server
	if( m_api.connect() )
		Notify(msg_verbose,"Connected");
	else
		Notify(msg_fatal_error, "Could not connect to TheEyeTribe server. Is it running?");
	
	// Get server state
	gtl::ServerState SS;
	SS = m_api.get_server_state();
	
	Notify(msg_verbose,"Eye Tribe server state");
	Notify(msg_verbose,"Version: %i",SS.version);
	Notify(msg_verbose,"Trackerstate: %i",SS.trackerstate);
	Notify(msg_verbose,"framerate: %i",SS.framerate);
	Notify(msg_verbose,"isCalibrated: %i",SS.iscalibrated);
	Notify(msg_verbose,"isCalibrating: %i",SS.iscalibrating);
	
	gtl::GazeData gaze_data;
	m_api.get_frame(gaze_data);
	
	StartTime = gaze_data.time;
	timeStamp = GetOutputArray("TIME_STAMP");
	raw = GetOutputArray("RAW");
	avg = GetOutputArray("AVG");
	fix = GetOutputArray("FIX");
	
	leftRaw = GetOutputArray("LEFT_EYE_RAW");
	leftAvg = GetOutputArray("LEFT_AVG_RAW");
	leftPupilSize = GetOutputArray("LEFT_PUPIL_SIZE");
	leftPupilCenter = GetOutputArray("LEFT_PUPIL_CENTER");

	rightRaw = GetOutputArray("RIGHT_EYE_RAW");
	rightAvg = GetOutputArray("RIGHT_AVG_RAW");
	rightPupilSize = GetOutputArray("RIGHT_PUPIL_SIZE");
	rightPupilCenter = GetOutputArray("RIGHT_PUPIL_CENTER");
}

TheEyeTribe::~TheEyeTribe()
{
	m_api.disconnect();
}

void
TheEyeTribe::Tick()
{
	// Get server state
	gtl::ServerState SS;
	SS = m_api.get_server_state();
	switch (SS.trackerstate) {
		case 1:
			Notify(msg_warning,"Tracker device is not detected");
			break;
		case 2:
			Notify(msg_warning,"Tracker device is detected but not working due to wrong/unsupported firmware");
			break;
		case 3:
			Notify(msg_warning,"Tracker device is detected but not working due to unsupported USB host");
			break;
		case 4:
			Notify(msg_warning,"Tracker device is detected but not working due to no stream could be received");
			break;
		default:
			break;
	}

	// Asking for Eye Tracking data
	gtl::GazeData gaze_data;
	m_api.get_frame(gaze_data);
	
	// Fill data to ikaros outputs
	timeStamp[0] = (float)(gaze_data.time - StartTime);
	raw[0] = gaze_data.raw.x;
	raw[1] = gaze_data.raw.y;
	avg[0] = gaze_data.avg.x;
	avg[1] = gaze_data.avg.y;
	fix[0] = gaze_data.fix;
	
	leftRaw[0] = gaze_data.lefteye.raw.x;
	leftRaw[1] = gaze_data.lefteye.raw.y;
	leftAvg[0] = gaze_data.lefteye.avg.x;
	leftAvg[1] = gaze_data.lefteye.avg.y;
	leftPupilSize[0] = gaze_data.lefteye.psize;
	leftPupilCenter[0] = gaze_data.lefteye.pcenter.x;
	leftPupilCenter[1] = gaze_data.lefteye.pcenter.y;

	rightRaw[0] = gaze_data.righteye.raw.x;
	rightRaw[1] = gaze_data.righteye.raw.y;
	rightAvg[0] = gaze_data.righteye.avg.x;
	rightAvg[1] = gaze_data.righteye.avg.y;
	rightPupilSize[0] = gaze_data.righteye.psize;
	rightPupilCenter[0] = gaze_data.righteye.pcenter.x;
	rightPupilCenter[1] = gaze_data.righteye.pcenter.y;
}

static InitClass init("TheEyeTribe", &TheEyeTribe::Create, "Source/Modules/IOModules/EyeTracker/TheEyeTribe/");
