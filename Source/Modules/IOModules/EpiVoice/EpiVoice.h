//
//	EpiVoice.h		This file is a part of the IKAROS project
// 						
//    Copyright (C) 2022 Christian Balkenius
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


#ifndef EpiVoice_
#define EpiVoice_

#include "IKAROS.h"

#include <vector>
#include <string>

class EmotionSound
{
    public:
        EmotionSound(const std::string & s) : sound_path(s) { timer = new Timer(); };

        void    Play(const char * command);
        bool    UpdateVolume(float * rms, float lag=0);  // returns true if still playing

        std::string sound_path;

        std::vector<float> time;
        std::vector<float> left;
        std::vector<float> right;

        int pid;

        int frame;
        Timer * timer;
};



class EpiVoice: public Module
{
public:
    float * trig;
    float * intensity;
    float * last_trig;
    int     size;
    float * inhibition;

    float * playing;
    float * completed;
    float * active;

    float * rms;
    float * volume;
    
    float   scale_volume;
    float   lag;
    int intensities;
    int variants;                                                       

    const char *  command;
    
    std::vector<EmotionSound> sound;
    int queued_sound;
    int current_sound;
    int last_sound;


    static Module * Create(Parameter * p) { return new EpiVoice(p); }

    EpiVoice(Parameter * p) : Module(p) {}
    virtual ~EpiVoice() {}

    EmotionSound       CreateSound(std::string sound_name);

    void 		Init();
    void 		Tick();
};

#endif

