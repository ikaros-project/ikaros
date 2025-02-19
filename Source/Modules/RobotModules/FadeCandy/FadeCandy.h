//
//	FadeCandy.h		This file is a part of the IKAROS project
//
//    Copyright (C) 2016 Christian Balkenius>
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

#ifndef FadeCandy_
#define FadeCandy_

#include "IKAROS.h"


class FadeCandy: public Module
{
public:
    static Module * Create(Parameter * p) { return new FadeCandy(p); }

    FadeCandy(Parameter * p);
    virtual ~FadeCandy();

    void 		Init();
    void 		Tick();

    int         no_of_channels;

    char **     channel_name_red;
    char **     channel_name_green;
    char **     channel_name_blue;

    int *       channel_size;
    int *       channel_LED_size;
    int *       channel_index;

    float **	channel_red;
    float **	channel_green;
    float **	channel_blue;

    Socket *    socket;

    pid_t       fcserver_pid;
	bool		startServer;
};

#endif

