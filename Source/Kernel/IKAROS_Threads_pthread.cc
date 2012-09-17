//
//	IKAROS_Threads.cc	Thread utilities for the IKAROS project
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
//	Wrapper for pthread - use this file for systems that supports pthread
//

#include "IKAROS_System.h"

#ifdef USE_THREADS

#include "IKAROS_Threads.h"

#include <pthread.h>

class ThreadData
{
public:
    pthread_t thread;
    int		thread_id;
};



int
Thread::Create(void *(*func)(void *), void *arg)
{
    return (data->thread_id = pthread_create( &(data->thread), NULL, func, arg));
}



int
Thread::Join()
{
    data->thread_id = -1;
    return pthread_join(data->thread, NULL);
}



Thread::Thread()
{
    data = new ThreadData();
    data->thread_id = -1;
}



Thread::~Thread()
{
    if (data->thread_id != -1)
        pthread_join(data->thread, NULL);
    delete data;
}

#endif
