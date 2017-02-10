//
//		HopfieldNetwork.h		This file is a part of the IKAROS project
//
//
//    Copyright (C) 2016 Christian Balkenius
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

#ifndef HopfieldNetwork_
#define HopfieldNetwork_

#include "IKAROS.h"
#include "RNN.h"

class HopfieldNetwork: public RNN
{
public:
	static Module * Create(Parameter * p) { return new HopfieldNetwork(p); }
	
				HopfieldNetwork(Parameter * p) : RNN(p) {};
    virtual		~HopfieldNetwork() {};

    void		Init();
    void		Tick();
};

#endif

