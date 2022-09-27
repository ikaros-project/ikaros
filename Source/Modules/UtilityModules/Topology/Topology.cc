//
//	Topology.cc		This file is a part of the IKAROS project
//
//    Copyright (C) 2012 <Author Name>
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
//  This example is intended as a starting point for writing new Ikaros modules
//  The example includes most of the calls that you may want to use in a module.
//  If you prefer to start with a clean example, use he module MinimalModule instead.
//

#include "Topology.h"

// use the ikaros namespace to access the math library
// this is preferred to using math.h

using namespace ikaros;

const int cEMPTY = 0;
const int cONE_TO_ONE = 1;
const int cNEAREST_NEIGHBOR_1D = 20;
const int cNEAREST_NEIGHBOR_2D = 2;
const int cNEAREST_NEIGHBOR_3D = 40;
const int cNEAREST_NEIGHBOR_3_1D = 50;
const int cCIRCLE = 3;
const int cDONUT = 70;
const int cRANDOM = 4;
const int cSMALL_WORLD = 90;

Topology::Topology(Parameter * p) : Module(p)
{
    //Bind(input_array_size_x, "size_x");
    //Bind(input_array_size_y, "size_y");
    Bind (tensor_x, "tensor_size_x");
    Bind (tensor_y, "tensor_size_y");

    AddOutput("OUTPUT");
}

void 
Topology::SetSizes()
{
    SetOutputSize("OUTPUT", tensor_x*tensor_y, tensor_x*tensor_y);
    
    
}

void
Topology::Init()
{
    Bind(type, "type");
    Bind(rnd_limit, "random_limit");
    Bind(self_conn, "allow_self_connection");
	Bind(debugmode, "debug");  
    // TODO check tensor sizes when type is n.neighbor
    // io(input_array_x, input_array_size_x, "SIZE_X");
    // io(input_array_y, input_array_size_y, "SIZE_Y");

    io(output_matrix, output_array_size_x, output_array_size_y,  "OUTPUT");

    switch (type)
    {
    
    case cONE_TO_ONE:
        SetOneToOne();
        break;
    case cNEAREST_NEIGHBOR_1D:
        SetNearestNeighbor(1);
        break;
    case cNEAREST_NEIGHBOR_2D:
        SetNearestNeighbor(2);
        break;
    case cNEAREST_NEIGHBOR_3D:
        SetNearestNeighbor(3);
        break;
    case cNEAREST_NEIGHBOR_3_1D:
        SetNearestNeighbor(4);
        break;
    case cCIRCLE:
        SetCircle();
        break;
    case cDONUT:
        SetDonut();
        break;
    case cRANDOM:
        SetRandom();
        break;
    
    case cEMPTY:
        // do nothing, filled with  zeros
        default:
        break;
    }

    // internal_array = create_array(10);
}



Topology::~Topology()
{
    // Destroy data structures that you allocated in Init.
    // destroy_array(internal_array);
}



void
Topology::Tick()
{
	if(debugmode)
	{
		// print out debug info
        print_matrix("output", output_matrix, output_array_size_x, output_array_size_y);
	}
}

void
Topology::SetOneToOne()
{
    // check that sizes x=y
    if(output_array_size_y==output_array_size_x)
    {
        for(int j=0; j<output_array_size_y; j++)
            for(int i=0; i<output_array_size_x; i++)
                if(j==i) output_matrix[j][i]=1.f;
    }
}

void
Topology::SetNearestNeighbor(int adim)
{
    int border = 1;
    float **ixm;
    int ctr = 0;
    set_matrix(output_matrix, 0, output_array_size_x, output_array_size_y);  

    switch (adim)
    {
    case 1:{
        std::vector<std::vector<int> > kernel {{-1}, {1}};
        // 1 dim, along line
        break;
    }
    case 2:{
        // 2 dim
        std::vector<std::vector<int> > kernel {{-1,-1}, {-1, 0}, {-1, 1},
                       {0,-1}, {0, 1},
                     {1,-1}, {1,0}, {1,1}};
         
        ixm = CreateIxMatrix(tensor_x,
                                    tensor_y,
                                    border);
        // print_matrix("ixmatrix", ixm, tensor_x+2*border, tensor_y+2*border);
        for(int j=border; j<tensor_y+border; j++)
        {
            for(int i=border; i<tensor_x+border; i++)
            {
                for(auto k : kernel)
                {
                    int ix = (int)ixm[j+k[0]] [i+k[1]]; 
                    // printf("j= %i, i= %i, ctr=%i, k= [%i %i], ix= %i\n",j,i,ctr,k[0], k[1],ix);
                    if(ix>=0) output_matrix[ctr][ix]=1; 
                } 
                ctr++;
            }
            
        }
        destroy_matrix(ixm);
        break;
    }
    case 3:{
        break;
    }
    case 4:{
        break;
    }
    default:
        break;
    }
 
}

void
Topology::SetCircle()
{
    for(int j=0; j<output_array_size_y; j++)
    {

        if(j==output_array_size_y-1)
            output_matrix[j][0] = 1;
        else
            output_matrix[j][j+1] = 1;
    }
    
}

void
Topology::SetDonut()
{
    return;
}

void
Topology::SetRandom()
{
    for(int j=0; j<output_array_size_y; j++)
    {
    	for(int i=0; i<output_array_size_x; i++)
        {
            if(random(0.f, 1.f) < rnd_limit)
                if(i!=j || (i==j && self_conn)) 
                    output_matrix[j][i] = 1;
        }
            
    }
}

float ** 
Topology::CreateIxMatrix(int x, int y, int border)
{
    float** retval = create_matrix(x+2*border, y+2*border);
    int ctr=0;
    set_matrix(retval, -1.f, x+2*border, y+2*border);
    for(int j=border; j < y+border; j++)
    {
        for(int i=border; i < x+border; i++)
        {
            retval[j][i] = (float)ctr;
            ctr++;
        }
    }
    return retval;
}

// Install the module. This code is executed during start-up.

static InitClass init("Topology", &Topology::Create, "Source/Modules/UtilityModules/Topology/");


