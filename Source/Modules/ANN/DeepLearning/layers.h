//
//	layers.h		This file is a part of the IKAROS project
// 						
//    Copyright (C) 2022 Amandus Krantz
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


#ifndef LAYER
#define LAYER

class Layer
{
    public:
        Layer(std::string name);
        virtual ~Layer();

        virtual matrix compute_output() = 0;
        virtual void load_weights(const std::ifstream &fin) = 0;

        vector bias;
        matrix weights;
};

class LayerDense : Public Layer
{
    public:
        LayerDense(): Layer () {}

        load_weights(std::ifstream &fin);
};

class ActivationReLU : Public ActivationLayer
{
    public:
        float ** compute_output();
};

class ActivationSigmoid  Public ActivationLayer
{
    public:
        float ** compute_output();
};

/* TODO:
 * Layers
 * - LayerConv2D
 */

#endif

