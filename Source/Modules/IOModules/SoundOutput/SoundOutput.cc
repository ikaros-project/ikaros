//
//	SoundOutput.cc		This file is a part of the IKAROS project
//
//    Copyright (C) 2013-2022 Christian Balkenius
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

#include <spawn.h>

extern char **environ;

using namespace ikaros;

void
Sound::Play(const char * command)
{
    timer->Restart();
    frame = 0;
    char * argv[5] = { (char *)command, (char *)sound_path.c_str(), NULL };
    pid_t pid;
    extern char **environ;
    int status = posix_spawn(&pid, (char *) command, NULL, NULL, argv, NULL);
}

bool
Sound::UpdateVolume(float * rms, float lag)
{
    float rt = 0.001*(timer->GetTime() - lag);
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

    int err = 0;
    char * command_line = create_formatted_string("ffprobe -f lavfi -i amovie=%s,astats=metadata=1:reset=1 -show_entries frame=pkt_pts_time:frame_tags=lavfi.astats.Overall.RMS_level,lavfi.astats.1.RMS_level,lavfi.astats.2.RMS_level -of csv=p=0 2>/dev/null", sound_path.c_str());
    float t, l, r;
    FILE * fp = popen(command_line, "r"); 
    if(fp != NULL)
    {
        while(fscanf(fp, "%f,%f,%f\n", &t, &l, &r) == 3)
        {
            sound.time.push_back(t);
            sound.left.push_back(l);
            sound.right.push_back(r);
        }
        err = pclose(fp);
    }
    if(err != 0) // Command failed - fake 1s output
    {
            sound.time.push_back(0);
            sound.left.push_back(-15);
            sound.right.push_back(-15);
            sound.time.push_back(1);
            sound.left.push_back(-15);
            sound.right.push_back(-15);
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
    Bind(scale_volume, "scale_volume");
    Bind(lag, "lag");

    for(auto sound_name: split(GetValue("sounds"), ","))
        sound.push_back(CreateSound(this->kernel->ikc_dir + trim(sound_name)));
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

    rms[0] = -50; // dB with no signal
    rms[1] = -50;

    if(current_sound != -1)
        if(!sound[current_sound].UpdateVolume(rms, lag))
            current_sound = -1;

    volume[0] = scale_volume * pow(10, 0.1*rms[0]); // convert to linear volume scale
    volume[1] = scale_volume * pow(10, 0.1*rms[1]);

    // Store last input

    copy_array(last_input, input, size);
}



static InitClass init("SoundOutput", &SoundOutput::Create, "Source/Modules/IOModules/SoundOutput/");


