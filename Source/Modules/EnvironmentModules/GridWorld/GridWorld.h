//
//	GridWorld.h		This file is a part of the IKAROS project
// 					Implements a simple grid world with obstacles and rewards
//
//    Copyright (C) 2003-2007 Christian Balkenius
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
//	Created: 2003-07-14
//	Revised: 2007-07-18
 
#ifndef GridWorld_
#define GridWorld_


#include "IKAROS.h"
#include "ctype.h"

class GridWorld: public Module
{
public:
    float	*	move;
    float	**	location;
    float	*	coordinate;
    float	**	obstacles;
    float	**	values;
    float	**	local_obstacles;
    float	**	local_values;
    float	*	reward;
    float	*	collision;
    float	**	image;

    int		x_start;
    int		y_start;

    int		mode;
    bool    normalize_coordinate;

    int		x;
    int		y;
    int		dir;

    int		size_x;
    int		size_y;

    GridWorld(Parameter * p);
    virtual ~GridWorld();

    static Module * Create(Parameter * p);

    void		Draw(int x, int y);

    void		SetSizes();
    void 		Init();
    void 		Tick();

};

#endif
