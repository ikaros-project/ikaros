//
//	GridWorld.cc		This file is a part of the IKAROS project
//					Implements a simple grid world with obstacles and rewards
//
//    Copyright (C) 2003-2007 Christian Balkenius
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
//	Created: 2003-07-14
//
//	surrounding: variable size + rotation, direction in image

#include "GridWorld.h"

#include "ctype.h"


using namespace ikaros;

Module * GridWorld::Create(Parameter * p)
{
    return new GridWorld(p);
}



GridWorld::GridWorld(Parameter * p):
        Module(p)
{
    AddInput("OBSTACLES");
    AddInput("VALUES");

    AddInput("MOVE");

    AddOutput("REWARD", 1);
    AddOutput("COLLISION", 1);
    AddOutput("COORDINATE", 2);
    AddOutput("LOCATION");
    AddOutput("LOCAL_OBSTACLES", 3, 3);
    AddOutput("LOCAL_VALUES", 3, 3);
	
    AddOutput("IMAGE");

    x_start	= GetIntValue("x_start", 1);
    y_start	= GetIntValue("y_start", 1);
   
    mode	= GetIntValueFromList("mode");
    normalize_coordinate = GetBoolValue("normalize_coordinate");
}



void
GridWorld::SetSizes()
{
   int  sizex = GetInputSizeX("OBSTACLES");
   int  sizey = GetInputSizeY("OBSTACLES");
   
   if(sizex == unknown_size || sizey == unknown_size)
	return;

   SetOutputSize("LOCATION", sizex, sizey);
   SetOutputSize("IMAGE", sizex, sizey);   
}



void
GridWorld::Draw(int _x, int _y)
{
    copy_matrix(image, obstacles, size_x, size_y);

    for (int j=0; j<size_y; j++)
        for (int i=0; i<size_x; i++)
	     if (values[j][i] > 0)
                image[j][i] = 3;

    image[_y][_x] = 2.0;
}



void
GridWorld::Init()
{
    move		=	GetInputArray("MOVE");
	
    location		=	GetOutputMatrix("LOCATION");
    coordinate	=	GetOutputArray("COORDINATE");

    local_obstacles	=	GetOutputMatrix("LOCAL_OBSTACLES");
    local_values		=	GetOutputMatrix("LOCAL_VALUES");
 
    obstacles	=	GetInputMatrix("OBSTACLES");
    values		=	GetInputMatrix("VALUES");

    reward		=	GetOutputArray("REWARD");
    collision   =   GetOutputArray("COLLISION");
    image		=	GetOutputMatrix("IMAGE");

   size_x = GetInputSizeX("OBSTACLES");
   size_y = GetInputSizeY("OBSTACLES");

    // Check inputs here
    
    if((size_x != GetInputSizeX("VALUES")) || (size_y != GetInputSizeY("VALUES")))
    {
        Notify(msg_fatal_error, "Size of VALUES input of module %s (%s) does not match OBSTACLES input\n", GetName(), GetClassName());
        return;
    }


    // CHECK INPUT SIZE 3 or 4 !!!!!

    x_start = max(min(x_start, size_x-2), 1);
    y_start = max(min(y_start, size_y-2), 1);

    x = x_start;
    y = y_start;

    Draw(x_start, y_start);
}



GridWorld::~GridWorld()
{
}



static int dx[4] = { 0, 1, 0, -1 };
static int dy[4] = { -1, 0, 1, 0 };

void
GridWorld::Tick()
{
    if (move == NULL || zero(move, (mode < 2 ? 4 : 3)))
    {
        reward[0] = 0;
        Draw(x, y);
        return;
    }

    int xr = 0;
    int yr = 0;
    int mv = 0;

    switch (mode)
    {
		case 0: // sum
		{
			float m[4] = {move[0]-move[2], move[1]-move[3], move[2]-move[0], move[3]-move[1]};
			mv = arg_max(m, 4);
			xr = dx[mv];
			yr = dy[mv];
		}	break;

		case 1: // max
		{
			mv = arg_max(move, 4);
			xr = dx[mv];
			yr = dy[mv];
		}	break;

		case 2: // relative sum
		{
			float m[3] = {move[0]-move[1], move[1]-move[0], move[3]};
			mv = arg_max(m, 3);
			if (mv == 2)
			{
				xr = dx[dir];
				yr = dy[dir];
			}
			else if (mv == 1)
				dir = (dir + 1) % 4;
			else if (mv == 1)
				dir = (dir - 1 + 4) % 4;
		}	break;

		case 3: // relative max
		{
			mv = arg_max(move, 3);
			if (mv == 2)
			{
				xr = dx[dir];
				yr = dy[dir];
			}
			else if (mv == 1)
				dir = (dir + 1) % 4;
			else if (mv == 1)
				dir = (dir - 1 + 4) % 4;
		}	break;

		default:
			;
    }

    // Agent can not occupy the outer border of the grid

    if (x+xr > 0 && x+xr < size_x-1 && y+yr > 0 && y+yr < size_y-1 && obstacles[y+yr][x+xr] != 1)
    {
        x += xr;
        y += yr;
        *collision = 1;
    }
    else
        *collision = 0;

    Draw(x, y);

    reward[0] = values[y][x];
    
    if (reward != NULL && reward[0] > 0)
    {
//        reward[0] = 0.0;
        x = x_start;
        y = y_start;
//        reward[0] = values[y][x];
        if(normalize_coordinate)
        {
            coordinate[0] = 1/(2*float(size_x)) + float(x)/float(size_x);
            coordinate[1] = 1/(2*float(size_y)) + float(y)/float(size_y);
        }
        else
        {
            coordinate[0] = x;
            coordinate[1] = y;
        }

        // The rest should be used in ONE place

        reset_matrix(location, size_x, size_y);
        location[y][x] = 1;
        Draw(x, y);

        // Set surrounding

        for (int i=0; i<3; i++)
            for (int j=0; j<3; j++)
                local_obstacles[j][i] = obstacles[y+j-1][x+i-1];

        for (int i=0; i<3; i++)
            for (int j=0; j<3; j++)
                local_values[j][i] = values[y+j-1][x+i-1];

        return;
    }

    if(normalize_coordinate)
    {
        coordinate[0] = 1/(2*float(size_x)) + float(x)/float(size_x);
        coordinate[1] = 1/(2*float(size_y)) + float(y)/float(size_y);
    }
    else
    {
        coordinate[0] = x;
        coordinate[1] = y;
    }

    reset_matrix(location, size_x, size_y);
    location[y][x] = 1.0;

    // Set surrounding

    for (int i=0; i<3; i++)
        for (int j=0; j<3; j++)
           local_obstacles[j][i] = obstacles[y+j-1][x+i-1];

    for (int i=0; i<3; i++)
        for (int j=0; j<3; j++)
            local_values[j][i] = values[y+j-1][x+i-1];
}


