//
//	YARPPort.h		This file is a part of the IKAROS project
//                      Ikaros to Yarp network communication
//
//    Copyright (C) 20011 Birger Johansson
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
//	Created: 20111031
//
//  Module that can read and write to an yarp network.

#ifndef YARPPort_
#define YARPPort_

#include "IKAROS.h"

// YARP
#include "yarp/os/all.h"
#include "yarp/os/PortablePair.h"
#include "yarp/sig/Vector.h"

#include <iostream>


class YARPPort: public Module
{
public:

    YARPPort(Parameter * p) : Module(p) {}
    virtual ~YARPPort();

    static Module * Create(Parameter * p) { return new YARPPort(p); }

    void 		Init();
    void 		Tick();
    
    void        YarpWrite();
    void        YarpRead();
    
    const char *yarpName;
    int         type;

    // YARP
    yarp::os::BufferedPort<yarp::os::PortablePair<yarp::os::Bottle,yarp::sig::Vector> > port;
    void showBottle(yarp::os::Bottle&, int);

    int tick;
    
    float * input;
    float * output;
    
    float * trigger;
    float * previousData;
    bool SendYARP;
    bool ReceiveYARP;
    bool SenOnlyNewValues;
    
    int size_x;
    int size_y;
};


#endif
