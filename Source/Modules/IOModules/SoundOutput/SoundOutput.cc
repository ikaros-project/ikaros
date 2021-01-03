//
//	SoundOutput.cc		This file is a part of the IKAROS project
//
//    Copyright (C) 2013-2021 Christian Balkenius
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


#include "SoundOutput.h"

#include <unistd.h>

using namespace ikaros;


void
Sound::Play(const char * command)
{
    timer->Restart();
    frame = 0;
    char * argv[5] = { (char *)command, (char *)sound_path.c_str(), NULL };
    if(!(pid = vfork()))
        execvp(command, argv);
}



bool
Sound::UpdateVolume(float * rms)
{
    float rt = 0.001*timer->GetTime();
    while(frame < time.size() && time[frame] < rt)
        frame++;

    if(frame == time.size())
    {
        rms[0] = -50; // dB with no signal
        rms[1] = -50;
        return false;
    }

    rms[0] = left[frame];
    rms[1] = right[frame];
    return true;
}



Sound
SoundOutput::CreateSound(std::string sound_path)
{
    Sound sound(sound_path);

    // Get amplitudes using ffprobe

    char * command_line = create_formatted_string("ffprobe -f lavfi -i amovie=%s,astats=metadata=1:reset=1 -show_entries frame=pkt_pts_time:frame_tags=lavfi.astats.Overall.RMS_level,lavfi.astats.1.RMS_level,lavfi.astats.2.RMS_level -of csv=p=0", sound_path.c_str());
    float t, l, r;
    FILE * fp = popen(command_line, "r"); 
    while(fscanf(fp, "%f,%f,%f\n", &t, &l, &r) == 3)
    {
        sound.time.push_back(t);
        sound.left.push_back(l);
        sound.right.push_back(r);
    }

    return sound;
}



void
SoundOutput::Init()
{
    input = GetInputArray("INPUT");
    size = GetInputSize("INPUT");
    last_input = create_array(size);
    current_sound = -1;
    rms = GetOutputArray("RMS");
    volume = GetOutputArray("VOLUME");
    command = GetValue("command");
    for(auto sound_name: split(GetValue("sounds"), ","))
        sound.push_back(CreateSound(trim(sound_name)));
}



void
SoundOutput::Tick()
{
    // Check for trig

    for(int i=0; i<size; i++)
        if(input[i] == 1 && last_input[i] == 0 && i < sound.size())
        {
            current_sound = i;
            sound[current_sound].Play(command);
        }

    // Update volume

    if(current_sound != -1)
        if(!sound[current_sound].UpdateVolume(rms))
            current_sound = -1;

    volume[0] = pow(10, 0.1*rms[0]); // convert to linear volume scale
    volume[1] = pow(10, 0.1*rms[1]);

    // Store last input

    copy_array(last_input, input, size);
}



static InitClass init("SoundOutput", &SoundOutput::Create, "Source/Modules/IOModules/SoundOutput/");


