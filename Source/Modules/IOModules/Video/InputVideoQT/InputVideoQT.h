//
//	InputVideoQT.h		This file is a part of the IKAROS project
//                      A module to grab image data from a QuickTime compatible camera
//
//    Copyright (C) 2002-2012  Christian Balkenius
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

#ifndef INPUTVIDEOQT
#define INPUTVIDEOQT


#include "IKAROS.h"

#ifdef USE_QUICKTIME

class InputVideoQTData;

class InputVideoQT: public Module {
public:
    void *          grabber;

    int				size_x;
    int				size_y;
    bool            flip;
    bool            list_devices;
    int             mode;
    char *          device_id;

    float *			intensity;
    float *			red;
    float *			green;
    float *			blue;

    static Module * Create(Parameter * p) { return new InputVideoQT(p); }
    
    InputVideoQT(Parameter * p);
    virtual ~InputVideoQT();

    void Init();
    void Tick();
private:
    FILE	*	file;
};

#endif

#ifndef USE_QUICKTIME

class InputVideoQT {
public:
    static Module * Create(Parameter * p)
    {
        printf("This version was not compiled with QuickTime\n.");
        return NULL;
    }
};

#endif

#endif

