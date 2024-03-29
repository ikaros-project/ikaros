//
//	CameraSimulate.h		This file is a part of the IKAROS project
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

#ifndef CameraSimulate_
#define CameraSimulate_

#include "IKAROS.h"

class CameraSimulate : public Module
{
public:
  static Module *Create(Parameter *p) { return new CameraSimulate(p); }

  CameraSimulate(Parameter *p) : Module(p) {}
  virtual ~CameraSimulate();

  void Init();
  void Tick();

  float *target_input;
  int target_input_size;
  float *camera_input;
  int camera_input_size;
  float **r;
  float **g;
  float **b;
  float **intensity;
  int output_size;

  float HFOV;
  float VFOV;
  float **target_colors_RGB;
  int targetSize;

  int nrOfTargets;
  int sizeX;
  int sizeY;
    
    int cSizeX;
    int cSizeY;

  float target_x;
  float target_y;
  float target_z;

  h_matrix t;
};

#endif
