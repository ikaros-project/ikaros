//
//	Phidgets.cc		This file is a part of the IKAROS project
//                      <Short description of the module>
//
//    Copyright (C) 2009 <Author Name>
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
//  This example is intended as a starting point for writing new Ikaros modules
//  The example includes most of the calls that you may want to use in a module.
//  If you prefer to start with a clean example, use he module MinimalModule instead.
//
//  Phidgets is a new version of MyModule that uses the IKC file rather than 
//  function calls during initialization.

#include "Phidgets.h"
#import <Phidget21/phidget21.h>


using namespace ikaros;

Phidgets::Phidgets(Parameter * p):Module(p)
{
    showInfo = GetBoolValue("info");
    boardSerial = GetIntValue("serial");
    SensitivityTriggerValue = GetIntValue("sensitivity");
    ratiometricMode = GetBoolValue("ratiometric");

    // Init the interface
    interfacekit_start();
}
 
void
Phidgets::Init()
{

    
    // Ikaros IO
    digitalInputArray = GetInputArray("DIGITAL_INPUTS", false);
    digitalOutputArray = GetOutputArray("DIGITAL_OUTPUTS", false);
    analogOutputArray = GetOutputArray("ANALOG_OUTPUTS",false);
    
    deviceAttached = GetOutputArray("ATTACHED", false);
}


void
Phidgets::SetSizes()
{
    // Check input
    if (InputConnected("DIGITAL_INPUT"))
    {   
        sizeOfDigitalInput = GetInputSize("DIGITAL_INPUT");
        if (sizeOfDigitalInput < nrDigitalInputs) // if we have not connected all the possible inputs from the board. Warning.
            Notify(msg_warning, "The number of inputs of the board (%d) do not match size of ikaros input (%d) Check connections!",nrDigitalInputs, sizeOfDigitalInput);
        else if (sizeOfDigitalInput > nrDigitalInputs) // if we have too many inputs for this io board. Fatal error.
                Notify(msg_fatal_error, "The number of inputs of the board (%d) do not match size of ikaros input (%d) Check connections!",nrDigitalInputs, sizeOfDigitalInput);
    }

    //printf("Phidgets board %d/%d/%d",nrAnalogOutputs,nrDigitalInputs,nrDigitalOutputs);
    SetOutputSize("ANALOG_OUTPUTS", nrAnalogOutputs);
    SetOutputSize("DIGITAL_OUTPUTS", nrDigitalOutputs);
    SetOutputSize("ATTACHED", 1);


}

Phidgets::~Phidgets()
{
    interfacekit_stop();
    delete analogOutputBuffer;
    delete digitalInputBuffer;
    delete digitalOutputBuffer;
    delete deviceAttachedBuffer;
}



void
Phidgets::Tick()
{
    // transfer buffer to ikaros outputs. Output size is known 
    for (int i = 0; i < nrAnalogOutputs; i++) {
        analogOutputArray[i] = analogOutputBuffer[i];
    }
    
    for (int i = 0; i < nrDigitalOutputs; i++) {
        digitalOutputArray[i] = digitalOutputBuffer[i];
    }
    
    // transfer input buffer. 
    if (sizeOfDigitalInput > 0 and sizeOfDigitalInput < nrDigitalInputs)
    {
        for (int i = 0; i < sizeOfDigitalInput; i++) {
            digitalInputArray[i] = digitalInputBuffer[i];
        }
    }
    deviceAttached[0] = deviceAttachedBuffer[0];

}


// From Phidgets sample coede
int AttachHandler(CPhidgetHandle IFK, void *userptr)
{
	int serialNo;
	const char *name;
    
    // Setting attachedbuffer to 1
    int * value;
    value = (int *) userptr;
    *value = 1;

    CPhidget_getDeviceName(IFK, &name);
	CPhidget_getSerialNumber(IFK, &serialNo);
    
	//printf("%s %10d attached!\n", name, serialNo);
	return 0;
}

int DetachHandler(CPhidgetHandle IFK, void *userptr)
{
	int serialNo;
	const char *name;
    
    // Setting attachedbuffer to 0
    int * value;
    value = (int *) userptr;
    *value = 0;

	CPhidget_getDeviceName (IFK, &name);
	CPhidget_getSerialNumber(IFK, &serialNo);
    
	//printf("%s %10d detached!\n", name, serialNo);
	return 0;
}

int ErrorHandler(CPhidgetHandle IFK, void *userptr, int ErrorCode, const char *unknown)
{
	printf("Phidget Error handled. %d - %s", ErrorCode, unknown);
	return 0;
}

//callback that will run if an input changes.
//Index - Index of the input that generated the event, State - boolean (0 or 1) representing the input state (on or off)
int InputChangeHandler(CPhidgetInterfaceKitHandle IFK, void *usrptr, int Index, int State)
{
    int * p = (int *) usrptr + Index;
    *p = State;
    
	//printf("Digital Input: %d > State: %d\n", Index, State);
	return 0;
}

//callback that will run if an output changes.
//Index - Index of the output that generated the event, State - boolean (0 or 1) representing the output state (on or off)
int OutputChangeHandler(CPhidgetInterfaceKitHandle IFK, void *usrptr, int Index, int State)
{
    int * p = (int *) usrptr + Index;
    *p = State;
    
	//printf("Digital Output: %d > State: %d\n", Index, State);
    return 0;
}

//callback that will run if the sensor value changes by more than the OnSensorChange trigger.
//Index - Index of the sensor that generated the event, Value - the sensor read value
int SensorChangeHandler(CPhidgetInterfaceKitHandle IFK, void *usrptr, int Index, int Value)
{
    int * p = (int *) usrptr + Index;
    *p = Value;
    
    //printf("(Ikaros)Sensor: %d > Value: %d\n", Index, Value);
    return 0;
}

//Display the properties of the attached phidget to the screen.  We will be displaying the name, serial number and version of the attached device.
//Will also display the number of inputs, outputs, and analog inputs on the interface kit as well as the state of the ratiometric flag
//and the current analog sensor sensitivity.
int display_properties(CPhidgetInterfaceKitHandle phid)
{
	int serialNo, version, numInputs, numOutputs, numSensors, triggerVal, ratiometric, i;
	const char* ptr;
    
	CPhidget_getDeviceType((CPhidgetHandle)phid, &ptr);
	CPhidget_getSerialNumber((CPhidgetHandle)phid, &serialNo);
	CPhidget_getDeviceVersion((CPhidgetHandle)phid, &version);
    
	CPhidgetInterfaceKit_getInputCount(phid, &numInputs);
	CPhidgetInterfaceKit_getOutputCount(phid, &numOutputs);
	CPhidgetInterfaceKit_getSensorCount(phid, &numSensors);
	CPhidgetInterfaceKit_getRatiometric(phid, &ratiometric);
    
    
	printf("%s\n", ptr);
	printf("Serial Number: %10d\nVersion: %8d\n", serialNo, version);
	printf("# Digital Inputs: %d\n# Digital Outputs: %d\n", numInputs, numOutputs);
	printf("# Sensors: %d\n", numSensors);
	printf("Ratiometric: %d\n", ratiometric);
    
	for(i = 0; i < numSensors; i++)
	{
		CPhidgetInterfaceKit_getSensorChangeTrigger (phid, i, &triggerVal);
		printf("Sensor#: %d > Sensitivity Trigger: %d\n", i, triggerVal);
	}
    
	return 0;
}


CPhidgetInterfaceKitHandle Phidgets::interfacekit_start()
{
	int result, numSensors, i;
	const char *err;
    
    // Create a buffer for outputs. Max 100 outputs on a board.
    analogOutputBuffer = new int[100];
    digitalOutputBuffer  = new int[100];
    digitalInputBuffer  = new int[100];
    
    deviceAttachedBuffer = new int;
    
    //create the InterfaceKit object
	CPhidgetInterfaceKit_create(&ifKit);
    
	//Set the handlers to be run when the device is plugged in or opened from software, unplugged or closed from software, or generates an error.
	CPhidget_set_OnAttach_Handler((CPhidgetHandle)ifKit, AttachHandler, deviceAttachedBuffer);
	CPhidget_set_OnDetach_Handler((CPhidgetHandle)ifKit, DetachHandler, deviceAttachedBuffer);
	CPhidget_set_OnError_Handler((CPhidgetHandle)ifKit, ErrorHandler, NULL);
    
	//Registers a callback that will run if an input changes.
	//Requires the handle for the Phidget, the function that will be called, and an arbitrary pointer that will be supplied to the callback function (may be NULL).
	CPhidgetInterfaceKit_set_OnInputChange_Handler (ifKit, InputChangeHandler, digitalInputBuffer);
    
    
	//Registers a callback that will run if the sensor value changes by more than the OnSensorChange trig-ger.
	//Requires the handle for the IntefaceKit, the function that will be called, and an arbitrary pointer that will be supplied to the callback function (may be NULL).
	CPhidgetInterfaceKit_set_OnSensorChange_Handler (ifKit, SensorChangeHandler, analogOutputBuffer);
    
    
	//Registers a callback that will run if an output changes.
	//Requires the handle for the Phidget, the function that will be called, and an arbitrary pointer that will be supplied to the callback function (may be NULL).
	CPhidgetInterfaceKit_set_OnOutputChange_Handler (ifKit, OutputChangeHandler, digitalOutputBuffer);
    
	//open the interfacekit for device connections

    if (boardSerial > 0)
        CPhidget_open((CPhidgetHandle)ifKit, boardSerial);
    else
        CPhidget_open((CPhidgetHandle)ifKit, -1);
    
	//get the program to wait for an interface kit device to be attached
	printf("Phigets: Waiting for interface kit to be attached....");
	if((result = CPhidget_waitForAttachment((CPhidgetHandle)ifKit, 10000)))
	{
		CPhidget_getErrorDescription(result, &err);
		printf("Problem waiting for attachment: %s\n", err);
		return 0;
	}

	//Display the properties of the attached interface kit device
    if (showInfo)
        display_properties(ifKit);
    
	//printf("Modifying sensor sensitivity triggers....\n");
    
	//get the number of sensors available
	CPhidgetInterfaceKit_getSensorCount(ifKit, &numSensors);
    
    // Store number of io on Phidgets board (used to create ikaros io connections)
    CPhidgetInterfaceKit_getInputCount(ifKit, &nrDigitalInputs);
	CPhidgetInterfaceKit_getOutputCount(ifKit, &nrDigitalOutputs);
	CPhidgetInterfaceKit_getSensorCount(ifKit, &nrAnalogOutputs);
    
	//Change the sensitivity trigger of the sensors
	for(i = 0; i < numSensors; i++)
	{
		CPhidgetInterfaceKit_setSensorChangeTrigger(ifKit, i, SensitivityTriggerValue);  
	}
    

    //printf("Toggling Ratiometric....\n");
	CPhidgetInterfaceKit_setRatiometric(ifKit, ratiometricMode);
    
	//all done, exit
	return ifKit;
}

int Phidgets::interfacekit_stop()
{
    // Closing Phidgets
	// printf("Closing...\n");
	CPhidget_close((CPhidgetHandle)ifKit);
	CPhidget_delete((CPhidgetHandle)ifKit);
    return 0;
}

static InitClass init("Phidgets", &Phidgets::Create, "Source/Modules/RobotModules/Phidgets/");


