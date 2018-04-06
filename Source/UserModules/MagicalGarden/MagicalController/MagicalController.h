//
//		MagicalController.h		This file is a part of the IKAROS project
//
//
//    Copyright (C) 2015 Christian Balkenius
///
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

#ifndef MagicalController_
#define MagicalController_

#include "IKAROS.h"

#include <unordered_map>
#include <string>


class MagicalController: public Module
{
public:
    std::unordered_map<std::string, int> behavior;
    std::unordered_map<std::string, int> sound;
    std::unordered_map<std::string, float> sound_len;


    ServerSocket *  socket;
    float *         animation_trig;
    float *         sound_trig;
    float *         target;
    float *         sound_active;
    
    int             no_of_animations;
    int             no_of_sounds;
    
    float           sound_duration;
    Timer           sound_timer;

	static Module * Create(Parameter * p) { return new MagicalController(p); }
	
				MagicalController(Parameter * p) : Module(p) {};
    virtual		~MagicalController() {};

    void		Init();
    void		Tick();
};

#endif

