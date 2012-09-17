//
//	Epuck.cc		This file is a part of the IKAROS project
//                  Module that controls an e-puck robot
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
//    Foundation, Inc., 59 rcvmsgle Place, Suite 330, Boston, MA  02111-1307  USA
//
//	Created: 2006-11-17
//

#include "Epuck.h"
//#define DEBUG_EPUCK
#include <iostream>
#include <sstream>
#include <string>
#include <stdio.h>

using namespace ikaros;


Module * Epuck::Create(Parameter * p)
{
    return new Epuck(p);
}


Epuck::Epuck(Parameter * p): Module(p)
{

    rcvmsg = new char[BUFSIZE];

    // get the parameters to the module
    CheckParameters();

    // Create a serial port
    s = NULL;
    s = new Serial(GetValue("port"), GetIntValue("BaudRate", 115200));
    
    // add input channels
    AddInput("VELOCITY"); // 2
    AddInput("LED"); // 8
    AddInput("LIGHT"); // 1
    AddInput("BODY"); // 1
    AddInput("SOUND"); // 1

    // add output channels
    AddOutput("PROXIMITY", 8);
    AddOutput("ACCELERATION_PLAIN", 3);
    AddOutput("ACCELERATION", 1);
    AddOutput("ORIENTATION", 2);
    AddOutput("INCLINATION", 2);
    AddOutput("ENCODER", 2);
    AddOutput("MICROPHONE_VOLUME", 3);
    AddOutput("MICROPHONE_BUFFER", 3, 100);
    AddOutput("MICROPHONE_SCAN_ID", 1);

    // the "camera" parameter decides if the camera should be used in
    // gray scale (1), color (2), or not at all (0)
    if (camera == 1)
    {
        AddOutput("IMAGE", width, height);
    }
    else if (camera == 2)
    {
        AddOutput("RED", width, height);
        AddOutput("GREEN", width, height);
        AddOutput("BLUE", width, height);
    }
}


Epuck::~Epuck()
{
    Reset();
    s->Close();
    delete timer;
    delete rcvmsg;
}


void
Epuck::Init()
{
    int i;

    // Get Inputs
    velocity = GetInputArray("VELOCITY", false);
    led = GetInputArray("LED", false);
    light = GetInputArray("LIGHT", false);
    body = GetInputArray("BODY", false);
    sound = GetInputArray("SOUND", false);

    // Get Outputs
    proximity = GetOutputArray("PROXIMITY");
    acceleration_plain = GetOutputArray("ACCELERATION_PLAIN");
    acceleration = GetOutputArray("ACCELERATION");
    orientation = GetOutputArray("ORIENTATION");
    inclination = GetOutputArray("INCLINATION");
    encoder =  GetOutputArray("ENCODER");
    microphone_volume =  GetOutputArray("MICROPHONE_VOLUME");
    microphone_buffer = GetOutputMatrix("MICROPHONE_BUFFER");
    microphone_scan_id =  GetOutputArray("MICROPHONE_SCAN_ID");

    // proximity init is used to try to record baseline noise from the IR proximity sensors
    for (i = 0; i < 8; i++)
        proximity_init[i] = 0;

    // see which channels are connected. if a channel is connected the corresponding
    // function to get that data will be run. functions for output channels, however,
    // are also run if its indicated by the channel_on parameters in the config file.
    // this is done because even though an output channel is not connected it might still
    // be observed by the webui module
    CheckChannels();

    // send the reset command to the epuck
//    if(!dummy)
//        Reset();

    // this variable is used to indicate if we did not manage to read a complete
    // reply from a command. if that is the case, the next time we want to send
    // a command, we first try to read the remainnig bytes from the previous command.
    bytes_left_to_read = 0;

    // calibrate sensors and record baseline noise from IR sensors
    if(proximity_on)
        CalibrateSensors();

    // did be not manage to connect to the epuck via the serial port?
    if (s == NULL)
    {
        Notify(msg_fatal_error, "Epuck - Could not connect to the e-puck.\n");
        return;
    }

    // set camera settings depending on if gray scale or color is used.
    if (camera == 1)
    {
        SetCameraSettings();
        image = GetOutputMatrix("IMAGE");
    }
    else if (camera == 2)
    {
        SetCameraSettings();
        red = GetOutputMatrix("RED");
        green = GetOutputMatrix("GREEN");
        blue = GetOutputMatrix("BLUE");
    }

    // flush on occasion
    s->Flush();

    timer = new Timer();
}


/** send reset command to the epuck.
**/
void
Epuck::Reset()
{
    int counter;

    // before csending the actual command, we first send some unknown
    // data and read the reply. this is done because there always seems
    // to exist some crap in the buffers from start.

    // flush buffers
    s->Flush();
    // send unknown crap
    s->SendString("z\n");
    // there could be some crap received here
    s->ReceiveUntil(rcvmsg, '\n');

    // send reset command
    s->SendString("r\n");
    //s->flush_in();

    // read the reply that confirms that the reset worked.
    // if it did not work, send fatal error signal.
    counter = 0;
    Notify(msg_print, "Epuck - resetting ");
    while (strncmp (rcvmsg,"the EPFL education robot type \"H\" for help\r\n",44) != 0)
    {
        counter++;
        printf(".");
        fflush(stdout);
        memset(rcvmsg, '0', BUFSIZE);
        timer->Sleep(10); // milliseconds
        // should eventually receive: the EPFL education robot type \"H\" for help\r\n"
        s->ReceiveUntil(rcvmsg, '\n');
        // This value is made up by shooting from the hip...
        if (counter > 100)
        {
            printf("aborting!\n");
            Notify(msg_fatal_error,"Epuck - Failed resetting e-puck!\n");
            return;
        }
    }
    printf("ok!\n");
}


/** if we fail to read the whole expected reply to a command sent to the epuck,
    there will probably be bytes in the buffer that we need to get rid of before
    trying to get another reply to another command. when we expect there to be X
    more bytes to read, we set the bytes_left_to_read to X, and (almost) always
    run this function before sending a command if bytes_left_to_read != 0.
    also the need_to_read_binary (bool) needs to be set. this will decide if we are
    trying to read X bytes, or simply read until we find a '\n' char.

    after 12 reads we give up even if we failed to read all we wanted.
    cant stay here forever after all, just hope that the next call of this function
    works things out for us.

    NOTE! when it was a reply to an ascii command that failed, we just need to
    set bytes_left_to_read to ANYTHING but 0, because we will try to read until
    the next '\n' char, and we do not know how many bytes that is.

    NOTE 2! every 5th call this function tries to read as much as possible regardless
    of how much we think there might still be in the buffers.
**/
void
Epuck::ReadLeftOverBytes()
{
    static int readings = 0;
    int r, i = 0;

    if (need_to_read_binary)
        while (bytes_left_to_read)
        {
            r = s->ReceiveBytes(rcvmsg, bytes_left_to_read);
            i++;
            if (i % 3 == 0){
                Notify(msg_warning, "Epuck - tried to read left over bytes %i times so far.\n", i);
                timer->Sleep(5); // milliseconds
            }
            if (i == 12)
                break;
            bytes_left_to_read -= r;
        }
    else
        while (bytes_left_to_read > 0){
            r = s->ReceiveUntil(rcvmsg, '\n');
            i++;
            if (i % 3 == 0){
                Notify(msg_warning, "Epuck - tried to read left over bytes %i times so far.\n", i);
                timer->Sleep(5); // milliseconds
            }
            if (i == 12)
                 break;
            if (r > 0 && rcvmsg[r-1] == '\n')
                bytes_left_to_read = 0;
        }

    // every 5th call of this function, see if there is a little something extra
    // to read, and read as much of it as possible if there is.
    if (++readings % 5 == 0)
        while(true)
            // break if we read 0 (nothing) or <0 (error while reading)
            if (s->ReceiveBytes(rcvmsg, BUFSIZE) < 1)
                break;

    memset(rcvmsg, '0', BUFSIZE);
    bytes_left_to_read = 0;
}


/** send calibration command to the epuck. if that fails, send the fatal error
    to the ikaros kernel.
    if it works, call the function that tries to record baseline noise from the
    IR sensors.
**/
void Epuck::CalibrateSensors()
{
    int counter;

    s->SendString("K\n");

    counter = 0;
    Notify(msg_print, "Epuck - calibrating ");
    while (strncmp (rcvmsg,"k, Calibration finished\r\n",25) != 0)
    {
        counter++;
        printf(".");
        fflush(stdout);
        memset(rcvmsg, '0', BUFSIZE);
        s->ReceiveUntil(rcvmsg, '\n');
        // This value is made up by shooting from the hip...
        if (counter > 24)
        {
            printf("aborting!\n");
            Notify(msg_fatal_error,"Epuck - failed to calibrate sensors!");
            return;
        }
    }
    printf("ok!\n");
    memset(rcvmsg, '0', BUFSIZE);

    // Home made Calibration
    // samples average noise readings from the IR-sensors.
    // no objects should be nearby the epuck.
    CalibrateProximity();
}


/** read the proximity IR sensors 100 times (here we are assuming nothing is close
    to the bot), add all that, then divide by 100 and consider that the baseline
    noise that we henceforth subtract from the sensor readings before copying to
    the channel outputs.
**/
void
Epuck::CalibrateProximity()
{
    int p[8];
    long p_sum[8] = {0,0,0,0,0,0,0,0};
    int number_of_samples = 100;
    std::stringstream oss;
    char neg_char = -'N';
    int t, i, m;

    oss << neg_char << (char)0;
    std::string message(oss.str());
    for (m = 0; m < number_of_samples; m++)
    {
        s->SendBytes(message.c_str(), 2);
        for (i=0; i<8; i++)
        {
            if (bytes_left_to_read)
                ReadLeftOverBytes();

            p[i] = 0;
            memset(rcvmsg, '0', BUFSIZE);
            t = s->ReceiveBytes(rcvmsg, 2);
            if (t != 2){
                Notify(msg_warning, "Epuck - (calibration) could not receive proximity info (%i)\n", t);
                bytes_left_to_read = 2 - t;
                need_to_read_binary = true;
                continue;
            }
            p[i] = (char)rcvmsg[1]&0xff;
            p[i] <<= 8;
            p[i] |= (char)rcvmsg[0]&0xff;
            p_sum[i] = p_sum[i]+p[i];
        }
    }

    for (i=0; i<8; i++)
        proximity_init[i] = (int)p_sum[i]/number_of_samples;
}


/** functions that correspond to input channels are run if those channels
    are connected.
    functions that correspond to output channels are run if those channels
    are either connected, or their on/off-status variables are set to true
    in the ikc file.
**/
void
Epuck::Tick()
{
    #ifdef TIMER
        float now;
        float tick_start = timer->GetTime();
    #endif

    if (velocity_is_connected){
        #ifdef TIMER
            now = timer->GetTime();
        #endif
        SetSpeed();
        #ifdef TIMER
            Notify(msg_print, "Epuck - SetSpeed() took %.0f ms\n", timer->GetTime() - now);
        #endif
    }

    if (light_is_connected){
        #ifdef TIMER
            now = timer->GetTime();
        #endif
        SetLight();
        #ifdef TIMER
            Notify(msg_print, "Epuck - SetLight() took %.0f ms\n", timer->GetTime() - now);
        #endif
    }

    if (led_is_connected){
        #ifdef TIMER
            now = timer->GetTime();
        #endif
        SetLeds();
        #ifdef TIMER
            Notify(msg_print, "Epuck - SetLeds() took %.0f ms\n", timer->GetTime() - now);
        #endif
    }

    if (body_is_connected){
        #ifdef TIMER
            now = timer->GetTime();
        #endif
        SetBodyLight();
        #ifdef TIMER
            Notify(msg_print, "Epuck - SetBodyLight() took %.0f ms\n", timer->GetTime() - now);
        #endif
    }

    if (sound_is_connected){
        #ifdef TIMER
            now = timer->GetTime();
        #endif
        SetSound();
        #ifdef TIMER
            Notify(msg_print, "Epuck - SetSound() took %.0f ms\n", timer->GetTime() - now);
        #endif
    }

    if (proximity_on || proximity_is_connected){
        #ifdef TIMER
            now = timer->GetTime();
        #endif
        GetProximity();
        #ifdef TIMER
            Notify(msg_print, "Epuck - GetProximity() took %.0f ms\n", timer->GetTime() - now);
        #endif
    }

    if (acceleration_plain_on || acceleration_plain_is_connected){
        #ifdef TIMER
            now = timer->GetTime();
        #endif
        GetAccelerationPlain();
        #ifdef TIMER
            Notify(msg_print, "Epuck - GetAccelerationPlain() took %.0f ms\n", timer->GetTime() - now);
        #endif
    }

    // note that GetAcceleration function sends the binary -'A' command to
    // the epuck from which we get info for all acc/ori/incl. so if one of
    // them is connected, the others will be calculated and available as output_list
    // also anyway.
    if (acceleration_on || acceleration_is_connected ||
            inclination_on || inclination_is_connected ||
            orientation_on || orientation_is_connected){
        #ifdef TIMER
            now = timer->GetTime();
        #endif
        GetAcceleration();
        #ifdef TIMER
            Notify(msg_print, "Epuck - GetAcceleration() took %.0f ms\n", timer->GetTime() - now);
        #endif
    }

    if (encoder_on || encoder_is_connected){
        #ifdef TIMER
            now = timer->GetTime();
        #endif
        GetEncoder();
        #ifdef TIMER
            Notify(msg_print, "Epuck - GetEncoder() took %.0f ms\n", timer->GetTime() - now);
        #endif
    }

    if (microphone_volume_on || microphone_volume_is_connected){
        #ifdef TIMER
            now = timer->GetTime();
        #endif
        GetMicrophoneVolume();
        #ifdef TIMER
            Notify(msg_print, "Epuck - GetMicrophoneVolume() took %.0f ms\n", timer->GetTime() - now);
        #endif
    }

    if (microphone_buffer_on || microphone_buffer_is_connected){
        #ifdef TIMER
            now = timer->GetTime();
        #endif
        GetMicrophoneBuffer();
        #ifdef TIMER
            Notify(msg_print, "Epuck - GetMicrophoneBuffer() took %.0f ms\n", timer->GetTime() - now);
        #endif
    }

    if (camera){
        #ifdef TIMER
            now = timer->GetTime();
        #endif
        GetImage();
        #ifdef TIMER
            Notify(msg_print, "Epuck - GetImage() took %.0f ms\n", timer->GetTime() - now);
        #endif
    }

    #ifdef TIMER
        Notify(msg_print, "Epuck - Whole tick took %.0f ms\n", timer->GetTime() - tick_start);
        Notify(msg_print, "====================================\n");
    #endif
}


bool flag = true;

/** sets the speed based on the VELOCITY input channel.
**/
void
Epuck::SetSpeed()
{
    static int previous_left = 0;
    static int previous_right = 0;
    std::stringstream oss;

    // max speed in m/s of the epuck
    #define MAX_SPEED 0.129

    if (velocity[0] < -MAX_SPEED || velocity[1] < -MAX_SPEED || velocity[0] > MAX_SPEED || velocity[1] > MAX_SPEED)
        print_array("Warning! E-puck changing velocity",velocity,2);
    // check limits
    velocity[0] = velocity[0] < -MAX_SPEED ? -MAX_SPEED : velocity[0];
    velocity[1] = velocity[1] < -MAX_SPEED ? -MAX_SPEED : velocity[1];
    velocity[0] = velocity[0] > MAX_SPEED ? MAX_SPEED : velocity[0];
    velocity[1] = velocity[1] > MAX_SPEED ? MAX_SPEED : velocity[1];

    #ifdef DEBUG_EPUCK
        print_array("velocity",velocity,2);
    #endif
    float Constant = 0.9;
    // unreliable at speeds below ca 50 (internal unit).
    // probably due to uneven surfaces in the ground.
    int L = int(velocity[0]/MAX_SPEED * 1000*Constant);
    int R = int(velocity[1]/MAX_SPEED * 1000*Constant);

    // if the speed hasnt changed, no need to send the command to the epuck
    // again since it will simply continue with the same speed(s) anyway.
    if (L == previous_left && R == previous_right)
        return;

    // Smoothen ride Add this in Grid world as well!;
    // This should be a little more Sofisticated to impress!
    //printf("EPUCK:alpha %f",alpha);
    // Control
    L = previous_left + int(float ((L-previous_left))*alpha);
    R = previous_right + int(float ((R-previous_right))*alpha);

//    L = (L+previous_left)/2.0f;
//    R = (R+previous_right)/2.0f;
//    printf("L %i (after\n",L);

    previous_left = L;
    previous_right = R;

    #ifdef DEBUG_EPUCK
        Notify(msg_print, "Epuck - changing velocity to (%i, %i)\n", L, R);
    #endif

    // BINARY MODE (does not work anymore)
/*
    fflush(stdout);
    // Binary   ex. -D800800
    char neg_char = -'D';
    oss << neg_char << (char)(L&0xff) << (char)(L>>8) << (char)(R&0xff) << (char)(R>>8) << (char)0;
    std::string message(oss.str());
    s->SendBytes(message.c_str(), 6);

    // Always flush.. twice a day
    s->Flush();
*/    
  
    // ASCII MODE

    char buf[256];
    sprintf(buf, "D,%d,%d\n", L, R);
    s->SendString(buf);
    
    // If we flush the message is remove before it has ben sent

}


/** set the front light based on the LIGHT input channel.
**/
void
Epuck::SetLight()
{
    static int previous_light = 0;
    int current_light, r;
    std::stringstream oss;

    if (light[0] == 0 || light[0] == 1 || light[0] ==2)
        current_light = (int)light[0];
    else if (light[0] < 0)
        current_light = 0;
    else
        current_light = 1;

    if (current_light == previous_light)
        return;
    previous_light = current_light;

    #ifdef DEBUG_EPUCK
        Notify(msg_print, "Epuck - changing front light to %i\n", current_light);
    #endif

    if (bytes_left_to_read)
        ReadLeftOverBytes();

    oss << "F," << current_light << "\n";
    std::string message(oss.str());
    s->SendString(message.c_str());

    // should receive "f\r\n"
    r = s->ReceiveUntil(rcvmsg, '\n');

    if (r > 0 && rcvmsg[0] != 'f')
        Notify(msg_warning, "Epuck - got some reply to front light command, but it seems erroneous.\n");

    if (r != 3){
        Notify(msg_warning, "Epuck - I think I failed to send front light command.\n");
        bytes_left_to_read = 3 - r;
        need_to_read_binary = false;
    }

    // Always flush.. twice a day
    s->Flush();
}


/** set the body light based on the BODY input channel.
**/
void Epuck::SetBodyLight()
{
    static int previous_body = 0;
    int current_body, r;
    std::stringstream oss;

    if (body[0] == 0 || body[0] == 1 || body[0] == 2)
        current_body = (int)body[0];
    else if (body[0] < 0)
        current_body = 0;
    else
        current_body = 1;

    // return if input has not changed from previous tick
    if (current_body == previous_body)
        return;

    previous_body = current_body;

    #ifdef DEBUG_EPUCK
        Notify(msg_print, "Epuck - changing body to %i\n", current_body);
    #endif

    if (bytes_left_to_read)
        ReadLeftOverBytes();

    oss << "B," << current_body << "\n";
    std::string message(oss.str());
    s->SendString(message.c_str());

    // should receive "b\r\n"
    r = s->ReceiveUntil(rcvmsg, '\n');

    if (r > 0 && rcvmsg[0] != 'b')
        Notify(msg_warning, "Epuck - got some reply to body light command, but it seems erroneous.\n");

    if (r != 3){
        Notify(msg_warning, "Epuck - i think i failed to send body light command.\n");
        bytes_left_to_read = 3 - r;
        need_to_read_binary = false;
    }

    // Always flush.. twice a day
    s->Flush();
}


/** sets the leds based on the LED input channel.
**/
void Epuck::SetLeds()
{
    static int previous_leds[8] = {0,0,0,0,0,0,0,0};
    int current_leds[8] = {-1, -1, -1, -1, -1, -1, -1, -1};
    std::stringstream oss;
    std::string message;

    for (int i=0; i<8; i++){

        if (led[i] == 0.0 || led[i] == 1.0 || led[i] == 2.0)
            current_leds[i] = (int)led[i];
        else
            if (led[i] < 0)
                current_leds[i] = 0;
            else
                current_leds[i] = 1;

        if (current_leds[i] == previous_leds[i])
            continue;
        previous_leds[i] = current_leds[i];

        #ifdef DEBUG_EPUCK
            Notify(msg_print, "Epuck - changing led[%i] to %i\n", i, current_leds[i]);
        #endif

        char neg_char = -'L';
        oss << neg_char << (char)i << (char)current_leds[i] << (char)0;
        message.append(oss.str());
        s->SendBytes(message.c_str(), 4);
        s->Flush();

        message.clear();
        oss.str("");
    }

    // Always flush.. twice a day
    s->Flush();
}


/** plays a sound based on the input from the SOUND channel.
**/
void Epuck::SetSound()
{
    static bool sound_is_playing = false;
    static float last_sound_time = -( SOUND_LENGHT + 1);
    int snd, r;
    std::stringstream oss;
    std::string message;

    if (bytes_left_to_read)
        ReadLeftOverBytes();

    // check if sound is playing. return if it is, dont wanna cut it off.
    // also assume sound playing takes SOUND_LENGTH milliseconds.
    if ((timer->GetTime() - last_sound_time) < SOUND_LENGHT)
        return;

    snd = (int) sound[0];
    if (snd < 0 || snd > 5) // 0=sound off   1-5=sounds
        snd = 0;

    // if sound was playing just now, make the bot quiet.
    // SOUND_LENGHT ms has passed and we wanna get rid of that after-sound-noise
    if (sound_is_playing){
        sound_is_playing = false;
        oss << "T,0\n";
        message.append(oss.str());
        //Notify(msg_print, "stopping sound\n");
        s->SendString(message.c_str());
        //s->flush_in();

        message.clear();
        oss.str("");
        r = s->ReceiveUntil(rcvmsg, '\n');

        if (r > 0 && rcvmsg[0] != 't')
            Notify(msg_warning, "Epuck - got some reply to the stop-sound command, but it seems erroneous.\n");

        if (r != 3){
            Notify(msg_warning, "Epuck - i think i failed to send the stop-sound command.\n");
            bytes_left_to_read = 3 - r;
            need_to_read_binary = false;
        }
    }

    // start playing a new sound if there is something in the input channel
    if (snd){
        if (bytes_left_to_read)
            ReadLeftOverBytes();

        sound_is_playing = true;
        oss << "T," << snd << "\n";
        message.append(oss.str());
        //Notify(msg_print, "starting sound\n");
        s->SendString(message.c_str());
        //s->flush();
        last_sound_time = timer->GetTime();
        message.clear();
        oss.str("");
        r = s->ReceiveUntil(rcvmsg, '\n');

        if (r > 0 && rcvmsg[0] != 't')
            Notify(msg_warning, "Epuck - got some reply to the sound command, but it seems erroneous.\n");

        if (r != 3){
            Notify(msg_warning, "Epuck - i think i failed to send the sound command.\n");
            bytes_left_to_read = 3 - r;
            need_to_read_binary = false;
        }
    }
}


/** upload the camera settings to the epuck. these settings are set
    through the ikc file. abort if that fails.
**/
void
Epuck::SetCameraSettings()
{
    int r;

    if (bytes_left_to_read)
        ReadLeftOverBytes();

    std::stringstream oss;
    oss << "J," <<  (camera-1) << "," << height << "," << width << "," << zoom << "\n";
    std::string message(oss.str());
    s->SendString(message.c_str());
    //s->flush();

    r = s->ReceiveUntil(rcvmsg, '\n');

    if (r > 0 && rcvmsg[0] != 'j')
        Notify(msg_warning, "Epuck - got some reply to the upload camera settings command, but it seems erroneous.\n");

    if (r != 3)
        Notify(msg_fatal_error, "Epuck - failed to upload camera settings.\n");
    else
        Notify(msg_verbose, "Epuck - uploaded camera settings.\n");
}


/** get the proximity data and update the PROXIMITY output channel.
**/
void Epuck::GetProximity()
{
    std::stringstream oss;
    char neg_char = -'N';
    int t, p;

    if ( bytes_left_to_read)
        ReadLeftOverBytes();

    oss << neg_char << (char)0;
    std::string message(oss.str());
    s->SendBytes(message.c_str(), 2);
    //s->flush();

    t = s->ReceiveBytes(rcvmsg, 16);
    if (t != 16){
        Notify(msg_warning, "Epuck - could not get proximity info (%i, %i)\n", t);
        bytes_left_to_read = 16 - t;
        need_to_read_binary = true;
        return;
    }

    int m = 0;
    for (int i = 0; i < 8; i++)
    {
        p = (char)rcvmsg[m+1]&0xff;
        p <<= 8;
        p |= (char)rcvmsg[m]&0xff;
        m++;
        m++;
        proximity[i] = (float) (p- proximity_init[i]);
        proximity[i] = proximity[i]/4000;
    }
}


/** get the acceleration data (from binary command) and update the ACCELERATION_PLAIN output channel.
**/
void
Epuck::GetAccelerationPlain()
{
    // NOTE binary mode and ASCII mode are different!
    // Binary gives acceleration,orientation,inclination.  Flat ASCII gives int acc_x,int acc_y,int acc_z.

    // here ascii
    std::stringstream oss;
    std::string message;
    int i, r;
    int ap[3];
    char c1, c2;

    if (bytes_left_to_read)
        ReadLeftOverBytes();

    // send request for plain ascii accelerator info
    oss << "A\n";
    message.append(oss.str());
    s->SendString(message.c_str());
    //s->flush();

    // get reply with plain ascii accelerator info
    s->ReceiveUntil(rcvmsg, '\n');

    // try to read reply
    r = sscanf(rcvmsg, "a,%d,%d,%d%c%c", ap+0, ap+1, ap+2, &c1, &c2);

    // warn if we could not read the whole reply
    if (r != 5 || c1 != '\r' || c2 != '\n'){
        Notify(msg_warning, "Epuck - could not get acceleration_plain info (got %i values from: %s)\n", r, rcvmsg);
        bytes_left_to_read = 1; // dunno how many, so just set to 1
        need_to_read_binary = false;
        return;
    }

    for (i = 0; i < 3; i++)
        acceleration_plain[i] = ((float) (ap[i])) / 4000.0;
}


/** get the acceleration data (from ascii command) and update the
    ACCELERATION/INCLINATION/ORIENTATION output channels.
**/
void
Epuck::GetAcceleration()
{
    // NOTE binary mode and ASCII mode are different!
    // Binary gives acceleration,orientation,inclination.  Flat ASCII gives int acc_x,int acc_y,int acc_z.

    // here binary
    float acc, ori, inc;
    unsigned char *fpoint1;
    unsigned char *fpoint2;
    unsigned char *fpoint3;
    int r, i;
    std::stringstream oss;
    char neg_char = -'A';

    if (bytes_left_to_read)
        ReadLeftOverBytes();

    oss << neg_char<< (char)0;
    std::string message(oss.str());
    s->SendBytes(message.c_str(), 2);

    r = s->ReceiveBytes(rcvmsg, 12);
    if (r != 12){
        Notify(msg_warning, "Epuck - could not receive acceleration info (%i)\n", r);
        bytes_left_to_read = 12 - r;
        need_to_read_binary = true;
        return;
    }

    fpoint1 = (unsigned char* )&acc;
    fpoint2 = (unsigned char* )&ori;
    fpoint3 = (unsigned char* )&inc;

    for (i = 0; i < (int)sizeof(float); i++)
    {
        fpoint1[i] = rcvmsg[i];
        fpoint2[i] = rcvmsg[i+sizeof(float)];
        fpoint3[i] = rcvmsg[i+sizeof(float)+sizeof(float)];
    }

    // normalize
    acceleration[0]= acc/4000.0;

    ori = ori*(2.0*pi/360.0);

    orientation[0] = ikaros::cos(ori);
    orientation[1] = ikaros::sin(ori);

    inc = inc*(2.0*pi/360.0);

    inclination[0] = ikaros::cos(inc);
    inclination[1] = ikaros::sin(inc);
}


/** get the image based on the previously uploaded camera settings and
    fill the IMAGE or RED/GREEN/BLUE output channels.
**/
void
Epuck::GetImage()
{
    int i, j, r, w, h, type;
    int size, msgidx;
    float tmp;
    std::stringstream oss;
    char neg_char = -'I';
    oss << neg_char<< (char)0;
    std::string message(oss.str());

    // read left over bytes, if any
    if (bytes_left_to_read)
        ReadLeftOverBytes();

    // send command to get camera data
    s->SendBytes(message.c_str(), 2);

    // get gray scale image?
    if (camera == 1){
        // get it!
        r = s->ReceiveBytes(rcvmsg, 3);

        // if we dont get the whole header, assume that there is an image
        // following that corresponds to the sizes gotten from the ikc file, and abort.
        if (r  != 3){
            Notify(msg_warning, "Epuck - could not get image info (%i)\n", r);
            size = width * height;
            bytes_left_to_read = size + 3 - r;
            need_to_read_binary = true;
            return;
        }

        type = (int)rcvmsg[0];
        w = (int)rcvmsg[1];
        h = (int)rcvmsg[2];
        Notify(msg_verbose, "Epuck - image type=%i, width=%i, height=%i\n", type, w, h);

        // does the header info match the sizes that we think the image should have?
        if (type == (camera-1) && w == width && h == height){
            size = width * height;
            r = s->ReceiveBytes(rcvmsg, size);

            // did we not read the whole image?
            if (r  != size){
                Notify(msg_warning, "Epuck - could not get image data (%i)\n", r);
                bytes_left_to_read = size - r;
                need_to_read_binary = true;
                return;
            }

            // ok, we read the whole image and all seems fine.
            // now copy it to the IMAGE output channel!
            msgidx = 0;
            for (i = 0; i < width; i++){
                for (j = height-1; j > -1; j--){
                    tmp = float(rcvmsg[msgidx++]);
                    if (tmp < 0)
                        tmp = (ikaros::abs(tmp + 128)) + 128;
                    image[j][i] = tmp / 255;
                }
            }
            return;
        }

        // ok, so we got the header, but the sizes do not match. lets assume that there
        // is an image following that matches the header sizes info, and abort.
        else{
            Notify(msg_warning, "Epuck - image info not correct\n");
            size = w*h;
            r = s->ReceiveBytes(rcvmsg, size);
            bytes_left_to_read = size - r;
            need_to_read_binary = true;
            s->Flush();
            return;
        }
    }

    // get color image?
    if (camera == 2){
        r = s->ReceiveBytes(rcvmsg, 3);

        // if we dont get the whole header, assume that there is an image
        // following that corresponds to the sizes gotten from the ikc file, and abort.
        // remember... color images are twice the size of gray scale.
        if (r != 3){
            Notify(msg_warning, "Epuck - could not get image info (%i)\n", r);
            size = 2 * width * height;
            bytes_left_to_read = size - r;
            need_to_read_binary = true;
            return;
        }

        type = (int)rcvmsg[0];
        w = (int)rcvmsg[1];
        h = (int)rcvmsg[2];
        Notify(msg_verbose, "Epuck - image type=%i, width=%i, height=%i\n", type, w, h);

        // does the header info match the sizes that we think the image should have?
        if (type == (camera-1) && w == width && h == height){
            size = 2 * width*height;
            r = s->ReceiveBytes(rcvmsg, size);

            // did we not read the whole image?
            if (r != size){
                Notify(msg_warning, "Epuck - could not get image data (%i)\n", r);
                bytes_left_to_read = size - r;
                need_to_read_binary = true;
                return;
            }

            // ok, we read the whole image and all seems fine.
            // now copy it to the RED/GREEN/BLUE output channels!
            msgidx = 0;
            for (i = 0; i < width; i++){
                for (j = height - 1; j > -1; j--){
                    int Pix_1 = rcvmsg[msgidx++];
                    int Pix_2 = rcvmsg[msgidx++];

                    int Red_pix=(int)((Pix_1&0xF8));
                    int Green_pix=(int)(((Pix_1 & 0x07)<<5)|((Pix_2 & 0xE0)>>3));
                    int Blue_pix=(int)((Pix_2 & 0x1F)<<3);

                    if (Red_pix > 255) Red_pix = 255;
                    if (Green_pix > 255) Green_pix = 255;
                    if (Blue_pix > 255) Blue_pix = 255;

                    red[j][i] = float(Red_pix)/255;
                    green[j][i] = float(Green_pix)/255;
                    blue[j][i] = float(Blue_pix)/255;
                }
            }
        }

        // ok, so we got the header, but the sizes do not match. lets assume that there
        // is an image following that matches the header sizes info, and abort.
        // remember... color images are twice the size of gray scale.
        else{
            Notify(msg_warning, "Epuck - image info not correct\n");
            size = 2 * w*h;
            r = s->ReceiveBytes(rcvmsg, size);
            bytes_left_to_read = size - r;
            need_to_read_binary = true;
            s->Flush();
            return;
        }
    }
}


/** get how many steps the wheels have turned (1000 steps is a full circle).
**/
void
Epuck::GetEncoder()
{
    static int prev_encoder[2] = {0, 65536};
    int e[2];
    int i, t;

    // Binary   ex. -Q
    std::stringstream oss;
    char neg_char = -'Q';

    if (bytes_left_to_read)
        ReadLeftOverBytes();

    // send request for encoder info
    oss << neg_char << (char)0;
    std::string message(oss.str());
    s->SendBytes(message.c_str(), 2);

    // get reply with encoder info
    memset(rcvmsg, '0', BUFSIZE);
    t = s->ReceiveBytes(rcvmsg, 4);

    // if we failed to get all 4 chars that the reply consists of,
    // assume that the epuck still has the same velocity as in last tick.
    if (t != 4){
        Notify(msg_warning, "Epuck - could not get encoder info (read %i chars). Assuming previous velocity.\n",t);
        bytes_left_to_read = 4 - t;
        need_to_read_binary = true;

        // assuming the same velocity as in the last tick.
        // but first update prev_encoder (that stores the previous ticks velocity)
        // before the function returns.
        for (i = 0; i < 2; i++){
            prev_encoder[i] += (int)(encoder[i]) + 65536;
            prev_encoder[i] %= 65536;
        }

        return;
    }

    // read the reply. store as ints
    e[0] = (char)rcvmsg[1]&0xff;
    e[0] <<= 8;
    e[0] |= (char)rcvmsg[0]&0xff;

    e[1] = (char)rcvmsg[3]&0xff;
    e[1] <<= 8;
    e[1] |= (char)rcvmsg[2]&0xff;

    // the right engine spins backwards so to speak, thats why 65536 - e[1]  (1=right)
    e[1] = 65536 - e[1];

    for (i = 0; i < 2; i++){
        // check rollover. the encoder should not be able to make such a big jump
        // as >32768 steps in one tick. the epuck just isnt THAT fast, unfortunately...
        if (ikaros::abs(e[i] - prev_encoder[i]) > 32768)
        {
            if (e[i] < 32768)
                prev_encoder[i] -= 65536;
            else
                prev_encoder[i] += 65536;
        }
        // set the output and store this ticks encoder info in the prev_info array
        encoder[i] = (float) ( e[i] - prev_encoder[i] );
        prev_encoder[i] = e[i];
    }
}


/** fetch the mic volumes and update the MICROPHONE_VOLUME output channel.
**/
void
Epuck::GetMicrophoneVolume()
{
    std::stringstream oss;
    std::string message;
    int i, r;
    int mv[3];
    char c1, c2;

    if (bytes_left_to_read)
        ReadLeftOverBytes();

    // send request for mic vol info
    oss << "U\n";
    message.append(oss.str());
    s->SendString(message.c_str());
    //s->flush_in();

    // get reply with mic vol info
    s->ReceiveUntil(rcvmsg, '\n');

    // try to read reply
    r = sscanf(rcvmsg, "u,%d,%d,%d%c%c", mv, mv+1, mv+2, &c1, &c2);

    // warn if we could not read the whole reply
    if (r != 5 || c1 != '\r' || c2 != '\n'){
        Notify(msg_warning, "Epuck - could not get microphone_volume info (got %i values from: %s)\n", r, rcvmsg);
        bytes_left_to_read = 1; // dunno how many, so just set to 1
        need_to_read_binary = false;
        return;
    }

    for (i = 0; i < 3; i++)
        microphone_volume[i] = (float) (mv[i]);
}


/** not really sure what this data (the data gotten by sending binary -u to the epuck) means,
    but here below is an answer from Michael Bonani:

"The data you get is in fact the internal rotating buffer of the microphone
where it is an array of 100 int for each three microphone. The fist char is
the last sample position of the rotating buffer."

    dont know what he means by "the first char", since when looking at the code (eg firmware.c)
    it shows that the first 600bytes sent are those 3*100 ints, and then another char is sent
    that is the scan id. so i guess the scan id should point to the position in the arrays
    where the last reading was stored, but when looking at the output values while making
    noises at the mics, it doesnt look like the output simply is volume. actually it seems
    like the output data is broken and jumps around a bit too much. or maybe its just that
    the data isnt processed in any way... i hope the docs are released soon:)

the code for the reply in the epuck looks like this:

        case 'U': // get micro buffer
          address = (char *) e_mic_scan;
          e_send_uart1_char(address, 600); // send sound buffer
          n = e_last_mic_scan_id;          // send last scan
          buffer[i++] = n & 0xff;
          break;

**/
// this sends in binary
void Epuck::GetMicrophoneBuffer()
{
    std::stringstream oss;
    char neg_char = -'U';
    char mic_scan_id;
    int mic_buffer[3][100];
    //int mic_min[3];
    int r, i, j;

    if (bytes_left_to_read)
        ReadLeftOverBytes();

    oss << neg_char << (char)0;
    std::string message(oss.str());
    s->SendBytes(message.c_str(), 2);

    r = s->ReceiveBytes(rcvmsg, 600);
    if (r != 600){
        Notify(msg_warning, "Epuck - could not read all microphone_buffer info. got %i chars (out of 600)\n", r);
        bytes_left_to_read = 601 - r; // 600+1 because we are supposed to read one more byte after this receiving
        need_to_read_binary = true;
        return;
    }

    r = s->ReceiveBytes(&mic_scan_id, 1);
    if (r != 1){
        Notify(msg_warning, "Epuck - could not read microphone_buffer mic_scan_id (read %i)\n", r);
        bytes_left_to_read = 1;
        need_to_read_binary = true;
        return;
    }

    // get the 3*100 ints from the epuck. each int consists of 2 bytes.
    //mic_min[0] = mic_min[1] = mic_min[2] = 65536;
    for (i = 0; i < 100; i++)
        for (j = 0; j < 3; j++){
            mic_buffer[j][i] = 0;
            mic_buffer[j][i] = (char)(rcvmsg[i*2 + j*200 + 1] & 0xff);
            mic_buffer[j][i] <<= 8;
            mic_buffer[j][i] |= (char)(rcvmsg[i*2 + j*200] & 0xff);
            //mic_min[j] = mic_min[j] < mic_buffer[j][i] ? mic_min[j] : mic_buffer[j][i];
        }

    /** add enough to all values so that no value is less than 0. should this be
        done or not??? if so, then uncomment the 3 mic_min related lines above.
        however, this is not very correct, since part of the buffer might be the
        same in two different ticks, while the minimums in thos 2 ticks might be
        different, and that would cause the outputs to be _different_ for
        the _same_ sounds. bad!
    for (i = 0; i < 3; i++)
        if (mic_min[i] < 0)
            for (j = 0; j < 100; j++)
                mic_buffer[i][j] += -mic_min[i];
    **/

    // cast the ints to floats and store in the output channel.
    // in this code that value was previously divided by 1500, not sure why,
    // however, it seems that the output is around 1300 from the epuck when
    // there is no sound.
    for (i = 0; i < 3; i++)
        for (j = 0; j < 100; j++){
            microphone_buffer[i][j] = (float) (mic_buffer[i][j]); // div with 1500 ?
            microphone_buffer[i][j] = (float) (mic_buffer[i][j]); // div with 1500 ?
            microphone_buffer[i][j] = (float) (mic_buffer[i][j]); // div with 1500 ?
        }

    microphone_scan_id[0] = (float) mic_scan_id;
}


/** see which channels are connected.
**/
void
Epuck::CheckChannels(void)
{
	velocity_is_connected = InputConnected("VELOCITY");
	led_is_connected = InputConnected("LED");
	light_is_connected = InputConnected("LIGHT");
	body_is_connected = InputConnected("BODY");
	sound_is_connected = InputConnected("SOUND");

	proximity_is_connected = OutputConnected("PROXIMITY");
	acceleration_plain_is_connected = OutputConnected("ACCELERATION_PLAIN");
	acceleration_is_connected = OutputConnected("ACCELERATION");
    orientation_is_connected = OutputConnected("ORIENTATION");
    inclination_is_connected = OutputConnected("INCLINATION");
    encoder_is_connected = OutputConnected("ENCODER");
    microphone_volume_is_connected = OutputConnected("MICROPHONE_VOLUME");
    microphone_buffer_is_connected = OutputConnected("MICROPHONE_BUFFER");
}


/** see what the variables in the ikc file are set to.
**/
void
Epuck::CheckParameters(void)
{
    int camsize;

    alpha = GetFloatValue("alpha", 0.9); // 0 = no respect to previous value
    calibrate = GetBoolValue("calibrate", true);
    camera = GetIntValueFromList("camera", "none/gray/color");
    height = GetIntValue("height", 32);
    width = GetIntValue("width", 32);
    zoom = GetIntValue("zoom", 8);

    camsize = height * width * camera;
    if (camsize > MAX_CAMSIZE)
    {
        if (camera == 1)
            Notify(msg_fatal_error, "Epuck - too large values for camera.  %i x %i = %i (max %i)\n", width, height, width*height, MAX_CAMSIZE);
        else
            Notify(msg_fatal_error, "Epuck - too large values for camera.  %i x %i x 2 = %i (max %i)\n", width, height, width*height*2, MAX_CAMSIZE);
    }
    proximity_on = GetBoolValue("proximity_on", false);
    acceleration_on = GetBoolValue("acceleration_on", false);
    acceleration_plain_on = GetBoolValue("acceleration_plain_on", false);
    orientation_on = GetBoolValue("orientation_on", false);
    inclination_on = GetBoolValue("inclination_on", false);
    encoder_on = GetBoolValue("encoder_on", false);
    microphone_volume_on = GetBoolValue("microphone_volume_on", false);
    microphone_buffer_on = GetBoolValue("microphone_buffer_on", false);
}
