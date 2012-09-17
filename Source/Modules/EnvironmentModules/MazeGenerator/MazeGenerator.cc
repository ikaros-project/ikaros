//
//	MazeGenerator.cc		This file is a part of the IKAROS project
//                          Implements a simple maze generator
//
//    Copyright (C) 2009 Christian Balkenius
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



#include "MazeGenerator.h"

using namespace ikaros;


void
MazeGenerator::SetSizes()
{
    size      = GetIntValue("size");

    SetOutputSize("OUTPUT", 2*size+1, 2*size+1);
    SetOutputSize("GOAL", 2*size+1, 2*size+1);
}



void
MazeGenerator::GeneratePerfectMaze()
{
    // Fill in walls
    
    for(int j=0; j<sizey; j+=2)
        for(int i=0; i<sizex; i++)
            output[j][i] = 1;
            
    for(int i=0; i<sizex; i+=2)
        for(int j=0; j<sizey; j++)
            output[j][i] = 1;

    // Generate the maze
    
    int total_cells = size*size;
    int * stack = new int [total_cells];
    int sp = 0;
    int xx, yy;

    int current_cell = random(total_cells);
    int visited_cells = 1;

    while(visited_cells < total_cells)
    {
        int x = current_cell % size;
        int y = current_cell / size;

        bool cell[4] = {false, false, false, false};
        int cells = 0;
        
        yy = 2*y+1;
        xx = 2*(x-1)+1;
        if(x>0 && output[yy][xx-1] == 1  && output[yy][xx+1] == 1  && output[yy-1][xx] == 1  && output[yy+1][xx] == 1 )
        {
            cell[0] = true;
            cells++;
        }
        
        yy = 2*y+1;
        xx = 2*(x+1)+1;
        if(x<size-1 && output[yy][xx-1] == 1  && output[yy][xx+1] == 1  && output[yy-1][xx] == 1  && output[yy+1][xx] == 1 )
        {
            cell[1] = true;
            cells++;
        }
        
        yy = 2*(y-1)+1;
        xx = 2*x+1;
        if(y>0 && output[yy][xx-1] == 1  && output[yy][xx+1] == 1  && output[yy-1][xx] == 1  && output[yy+1][xx] == 1 )
        {
            cell[2] = true;
            cells++;
        }
        
        yy = 2*(y+1)+1;
        xx = 2*x+1;
        if(y<size-1 && output[yy][xx-1] == 1  && output[yy][xx+1] == 1  && output[yy-1][xx] == 1  && output[yy+1][xx] == 1 )
        {
            cell[3] = true;
            cells++;
        }

        if(cells > 0)
        {
            int r = random(4);   // for random output, select directional for longer corridors?
            while(!cell[r])
                r = (r+1) % 4;
            
            stack[sp++] = current_cell;

            switch(r)
            {
                case 0:
                    yy = 2*y+1;
                    xx = 2*x+1-1;
                    output[yy][xx] = 0;
                    current_cell = y*size+(x-1);
                    break;

                case 1:
                    yy = 2*y+1;
                    xx = 2*x+1+1;
                    output[yy][xx] = 0;
                    current_cell = y*size+(x+1);
                    break;

                case 2:
                    yy = 2*y+1-1;
                    xx = 2*x+1;
                    output[yy][xx] = 0;
                    current_cell = (y-1)*size+x;
                    break;

                case 3:
                    yy = 2*y+1+1;
                    xx = 2*x+1;
                    output[yy][xx] = 0;
                    current_cell = (y+1)*size+x;
                    break;
            }
            visited_cells++;
        }
        
        else // pop
        {
            current_cell = stack[--sp];
        }
    }
}



void
MazeGenerator::Init()
{

    output  =	GetOutputMatrix("OUTPUT");
    sizex   =   GetOutputSizeX("OUTPUT");
    sizey   =   GetOutputSizeY("OUTPUT");

    regenerate = GetIntValue("regenerate");
    tick = 0;

    if(regenerate == 0)
        GeneratePerfectMaze();
}



void
MazeGenerator::Tick()
{
    if(regenerate != 0 && tick++ % regenerate == 0)
        GeneratePerfectMaze();
}

