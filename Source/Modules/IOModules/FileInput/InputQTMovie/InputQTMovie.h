//
//	InputQTMovie.h      This file is a part of the IKAROS project
//						A module for reading images from a QuickTime movie
//
//    Copyright (C) 2001-2011  Christian Balkenius
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
//    2009-07-01 Restart trig added (CB)
//    2011-08-18 Finally updated for QTKit (CB)


#ifndef INPUTQTMOVIE
#define INPUTQTMOVIE

#include "IKAROS.h"

#ifdef USE_QUICKTIME

class InputQTMovieData;

class InputQTMovie: public Module {
public:
    InputQTMovieData * data;
    
    const char *	filename;
    bool			loop;

    int				size_x;
    int				size_y;

    int				native_size_x;
    int				native_size_y;
    
    float *			intensity;
    float *			red;
    float *			green;
    float *			blue;

    float *         restart;

    static Module * Create(Parameter * p) {return new InputQTMovie(p); }
    
                    InputQTMovie(Parameter * p);
    virtual          ~InputQTMovie();

    void            Init();
    void            Tick();
};

#endif

#ifndef USE_QUICKTIME
class InputQTMovie {
public:
    static Module * Create(Parameter * p) { return NULL; }
};
#endif


#endif
