//
//    ActionCompetition.h			This file is a part of the IKAROS project
//
//    Copyright (C) 2018  Christian Balkenius
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

#ifndef ActionCompetition_
#define ActionCompetition_

#include "IKAROS.h"


class ActionCompetition: public Module
{
public:

    ActionCompetition(Parameter * p) : Module(p) {}
    virtual ~ActionCompetition() {}

    static Module * Create(Parameter * p) { return new ActionCompetition(p); }

    void        SelectNewState();

    void        Init();
    void        Tick();

    int *       duration; // generates completion internally after this time; 0 equals no internal completion
    int *       timeout;

    float *     input;
    float *     complete;
    float *     output;
    float *     trigger;

    float *     rest;
    float *     min_;
    float *     max_;
    float *     passive;
    float *     bias;
    float *     completion_bias;

    int         input_size;
    int         output_size;
    int         complete_size;

    int         counter;
    int         current_winner;
};


#endif
