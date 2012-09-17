//
//	IKAROS_Signal.cc		Signal utilities for IKAROS
//
//    Copyright (C) 2006-2007  Christian Balkenius
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
//    NOTE: There is no .h file for IKAROS_Signal since there are no callable routines
//
//	This file installs a CTRL-C handler that allows IKAROS to terminate gracefully
//	In case of compile errors, this file can simply be removed.
//

#include "IKAROS_System.h"

#ifdef WINDOWS
#include <windows.h>
#include <stdio.h>

#include <signal.h>

extern bool global_terminate;				// Used to flag that CTRL-C has been received; defined in IKAROS.cc
BOOL WINAPI ConsoleHandler1(DWORD dwCtrlType)
{
    printf("\nIKAROS: Will terminate after this iteration.\n");
    global_terminate= true;
    SetConsoleCtrlHandler(ConsoleHandler1,FALSE);
    return true;

}
class Signal
{

private:

public:
    bool install_handler()
    {
        if (!(SetConsoleCtrlHandler(ConsoleHandler1,TRUE)))
        {
            printf("Unable to install (CTRL-C) handler!\n");
            return -1;
        }
        return true;
    }
    Signal()
    {
        install_handler();
    }
}
INIT;
#endif

#ifdef POSIX
#include <stdio.h>
#include <signal.h>

extern bool global_terminate;				// Used to flag that CTRL-C has been received; defined in IKAROS.cc
class Signal
{

private:
    static void Handler(int s)				// Catch CTRL-C, set the terminate flag and remove the handler
    {								// so that next CTRL-C will terminate the process immediately
        global_terminate = true;
        struct sigaction sa;
        sa.sa_handler = SIG_DFL;
        sigemptyset(&sa.sa_mask);
        sigaction(SIGINT, &sa, NULL);
        fflush(NULL);
        printf("\nIKAROS: Will terminate after this iteration.\n");
        fflush(NULL);
    }
public:
    Signal()							// Install the CTRL-C handler
    {
        struct sigaction sa;
        sa.sa_handler = &Signal::Handler;
        sigemptyset(&sa.sa_mask);
        sigaction(SIGINT, &sa, NULL);
    }
}
INIT;

#endif

