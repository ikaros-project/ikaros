//
//	Phidgets.h		This file is a part of the IKAROS project
// 					<Short description of the module>
//
//    Copyright (C) 2007 <Author Name>
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
//	Created: <date>
//
//	<Additional description of the module>

#ifndef Phidgets_
#define Phidgets_

#include "IKAROS.h"
#ifdef MAC_OS_X
#include <Phidget21/phidget21.h>
#else 
#include "phidget.21.h"
#endif
class Phidgets: public Module
{
public:

    Phidgets(Parameter * p);
    virtual ~Phidgets();

    static Module * Create(Parameter * p) { return new Phidgets(p); }
    //static Module * Create(Parameter * p);

    void 		Init();
    void 		Tick();
    void		SetSizes();

    // Phidget
    CPhidgetInterfaceKitHandle ifKit;

    CPhidgetInterfaceKitHandle interfacekit_start();
    int interfacekit_stop();

    // Board io
    int nrDigitalInputs;
    int nrDigitalOutputs;
    int nrAnalogOutputs;

    // Ikaors io
    int sizeOfDigitalInput;
    int sizeOfDigitalOoutput;
    int sizeOfAnalogOutput;
    
    // Used between the board and phidgets lib. Can be updated every 8ms according to manual.
    int *   analogOutputBuffer;
    int *   digitalOutputBuffer;
    int *   digitalInputBuffer;
    
    int *   deviceAttachedBuffer;

    // Ikaros output arrays
    float *     digitalInputArray;
    float *     digitalOutputArray;
    float *     analogOutputArray;
    
    float *     deviceAttached;
    
    int boardSerial; // is int enogh?
    bool showInfo; 
    int SensitivityTriggerValue;
    bool ratiometricMode;

};

#endif
