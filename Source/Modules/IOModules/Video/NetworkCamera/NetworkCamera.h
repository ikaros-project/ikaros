//
//	NetworkCamera.h     This file is a part of the IKAROS project
//                      A module to grab images from a network camera
//
//    Copyright (C) 2002-2008  Christian Balkenius, Birger Johansson
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

#ifndef NETWORKCAMERA
#define NETWORKCAMERA

#include "IKAROS.h"

#if defined USE_SOCKET && defined USE_LIBJPEG

#define INITIAL_BUFFER_SIZE 65536

class NetworkCamera: public Module {
public:
    int				size_x;	// Size of the image to grab
    int				size_y;

    float **		intensity;
    float **		red;
    float **		green;
    float **		blue;

    const char *	host_ip;
    int				host_port;
    Socket *		socket;
    const char *	image_request;

    int				fps;
    int				compression;

    long			timestamp;

    char			boundary[128];
    
    long int        buffer_size;
    char *          buffer;

    static Module * Create(Parameter * p) {return new NetworkCamera(p);};
    
    NetworkCamera(Parameter * p);
    virtual ~NetworkCamera() {};

    void    ReadLine(char * line);
    void	SkipBoundary(char  * bound);
    void	ReadHeader();
    bool    ReadBlock();

    void    Init();
    void    Tick();
};

#else

class NetworkCamera {
public:
    static Module * Create(char * name, Parameter * p) { return NULL; }
};


#endif
#endif

