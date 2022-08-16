//
//	Stop.h		This file is a part of the IKAROS project
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

#ifndef Stop_
#define Stop_

#include "IKAROS.h"

class Stop: public Module
{
public:
    enum eComparator {eLess=0, eGreater, eEqual};
    static Module * Create(Parameter * p) { return new Stop(p); }

    Stop(Parameter * p) : Module(p) {}
    virtual ~Stop();

    void 		Init();
    void 		Tick();

    bool (Stop::*Compare)(float, float);
    bool (Stop::*CompareInverse)(float, float);
    bool Less(float a, float b);
    bool Greater(float a, float b);
    bool Equal (float a, float b);
    bool GreaterEqual(float a, float b);
    bool LessEqual(float a, float b);

    // pointers to inputs and outputs
    // and integers to represent their sizes

    float *     input_array;
    int         input_array_size;


    // parameter values

    float       termination_criterion;
    int         select;
    int         wait;
    eComparator comparator_ix;

	bool       	debugmode;
    int counter;
    float complete_perc;
};

#endif
