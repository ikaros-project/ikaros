//
//	IKAROS_Threads_none.cc	Thread utilities for the IKAROS project
//
//    Copyright (C) 2006  Christian Balkenius
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
//	This file defines a dummy class for systems that lacks thread support
//

#include "IKAROS_System.h"

#ifndef USE_THREADS

#include "IKAROS_Threads.h"

int
Thread::Create(void *(*func)(void *), void *arg)
{
    // Since we have no threads we will simply call the function immediately

    func(arg);
    return 0;
}



int
Thread::Join()
{
    // There is nothing to join

    return 0;
}



Thread::Thread()
{}



Thread::~Thread()
{}

#endif
