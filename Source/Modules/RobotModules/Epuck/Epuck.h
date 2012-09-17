//
//	Epuck.h		This file is a part of the IKAROS project
// 				Module that controls an e-puck robot
//
//    Copyright (C) 2006 Christian Balkenius
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
//	Created: 2006-11-17
//

#ifndef EPUCK_
#define EPUCK_

#include "IKAROS.h"


//#define TIMER
#define BUFSIZE 4096
#define SOUND_LENGHT 1500

// the max camsize is set to the same size as the buffer from which the e-puck sends its replies
// but subtract 3 because 3 chars of that buffer are not part of the image anyway.
#define MAX_CAMSIZE 4139-3


class Epuck: public Module
{
public:
            // inputs
	float   *led;
	float   *light;
	float   *body;
	float   *velocity;
	float   *sound;

            // outputs
	float   *proximity;
	float   *acceleration_plain;
	float   *acceleration;
    float   *orientation;
    float   *inclination;
    float   *encoder;
    float   *microphone_volume;
    float   **microphone_buffer;
    float   *microphone_scan_id;
    float   **image;
	float   **red;
	float   **green;
	float   **blue;

	bool    proximity_on;
	bool    acceleration_plain_on;
	bool    acceleration_on;
    bool    orientation_on;
    bool    inclination_on;
    bool    encoder_on;
    bool    microphone_volume_on;
    bool    microphone_buffer_on;
//    bool    dummy;

            // internal
	char    *device;
	bool    calibrate;

	int     camera;		// 0 = none; 1 = grayscale; 2 = color
	int     height;
	int     width;
	int     zoom;

	float   alpha;

            // variables for which INPUTS should be used
	bool    velocity_is_connected;
	bool    led_is_connected;
	bool    light_is_connected;
	bool    body_is_connected;
	bool    sound_is_connected;

	bool    proximity_is_connected;
	bool    acceleration_plain_is_connected;
	bool    acceleration_is_connected;
    bool    orientation_is_connected;
    bool    inclination_is_connected;
    bool    encoder_is_connected;
    bool    microphone_volume_is_connected;
    bool    microphone_buffer_is_connected;

    int proximity_init[8];

    Serial  *s;
    bool    need_to_read_more;
    bool    need_to_read_binary;
    int     bytes_left_to_read;

    char    *rcvmsg;
    Timer   *timer;

	void    Reset();
	void    SetSpeed();
    void    SetLight();
    void    SetLeds();
    void    SetBodyLight();
    void    SetSound();

    void    SetCameraSettings();

	void    CalibrateSensors();
    void    GetProximity();
    void    CalibrateProximity();
    void    GetAccelerationPlain();
    void    GetAcceleration();
    void    GetImage();
    void    GetEncoder();
    //void        SetEncoder(); // to be continued...
    void    GetMicrophoneVolume();
    void    GetMicrophoneBuffer();

    void    ReadLeftOverBytes(void);

    void    CheckParameters(void);
    void    CheckChannels(void);

            Epuck(Parameter * p);
    virtual ~Epuck();

	static  Module *Create(Parameter * p);

	void    Init();
	void    Tick();
};

#endif
