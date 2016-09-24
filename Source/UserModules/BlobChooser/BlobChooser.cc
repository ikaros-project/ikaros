//
//	MinimalModule.cc		This file is a part of the IKAROS project
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


#include "BlobChooser.h"

using namespace ikaros;

void
BlobChooser::Init()
{
  input_matrix = GetInputMatrix("INPUT");
  input_matrix_size_x = GetInputSizeX("INPUT");
  input_matrix_size_y = GetInputSizeY("INPUT");

  output_matrix = GetOutputMatrix("OUTPUT");
  output_matrix_size_x = GetOutputSizeX("OUTPUT");
  output_matrix_size_y = GetOutputSizeY("OUTPUT");

  internal_matrix = create_matrix(input_matrix_size_x, input_matrix_size_y);
  p1 = create_array(2);

  origin = GetInputArray("ORIGIN");

  dist = 100.0;
}

BlobChooser::~BlobChooser()
{
    destroy_matrix(internal_matrix);
}

void
BlobChooser::Tick()
{
    copy_matrix(internal_matrix, input_matrix, input_matrix_size_x, input_matrix_size_y);
    dist = 100;
    for (int j=0; j<output_matrix_size_y; j++)
        for (int i=0; i<output_matrix_size_x; i++)
              output_matrix[j][i] = -1;

    for (int j=0; j<output_matrix_size_y; j++){
        p1[0] = internal_matrix[j][0];
        p1[1] = internal_matrix[j][1];

        if(p1[0] != -1){if(sqrt(
            origin[0]*origin[0] +
            origin[1]*origin[1] +
            p1[0]*p1[0] +
            p1[1]*p1[1]
          ) < dist){
            output_matrix[0][0] = p1[0];
            output_matrix[0][1] = p1[1];
          }
        }
    }
}

static InitClass init("BlobChooser", &BlobChooser::Create, "Source/UserModules/BlobChooser/");
