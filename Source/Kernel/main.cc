//
//    main.cc		The main code for the IKAROS project
//
//    Copyright (C) 2001-2018  Christian Balkenius
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
//	Created: August 8, 2001
//
//	The main function in IKAROS does two things:
//
//	1. It installs all modules and the kernel
//	2. It calls the kernel to start execution

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <new>
#include <stdexcept>

// Kernel

#include "IKAROS.h"
using namespace ikaros;


void PrintInfo();

void
PrintInfo()
{
    printf("\n");
    printf("Usage:\n");
    printf("\n");
    printf("\tikaros [-W#][-p][-P][-t][-T][-b#][-r#][-v][-q][-x][-X][-m][-l][-i][-a][file]\n\n");
    
    printf("\t-r    run; don't wait for webui\n");
    printf("\t-t#   set timebase in milliseconds for realtime mode\n");
    printf("\t-s#   stop after # ticks\n");
    printf("\t-w#   set port for webui; default is 8000\n");
    
    printf("\t-S#   store + optional path to storage directory\n");
    printf("\t-L#   load + optional path to storage directory\n");

    printf("\t-v    verbose mode\n");
    printf("\t-q    quiet mode\n");
    
    printf("\t-p    profile (>= 1 ms)\n");
    printf("\t-P    profile (>= 0.01 ms)\n");
    printf("\t-m    list modules and connections\n");
    printf("\t-c    list installed classes\n");
    printf("\t-l    list scheduling nad thread allocation\n");
    printf("\t-i    list installed functionality sockets, timer etc, type of target system\n");
    printf("\t-a    list all; implies -m, -i and -l\n");
    
    printf("\t-z#   seed random number generator\n");

    printf("\n");
    printf("Examples:\n");
    printf("\n");
    printf("\tAssuming the current path is the /Bin in the Ikaros directory,\n");
    printf("\tan example can be run like this:\n");
    printf("\n");
    printf("\t\t./ikaros  ../Examples/example.ikc\n");
    printf("\n");
    printf("\tThis example will wait for a request from the Web browser at\n");
    printf("\tthe default port (8000).\n");
    printf("\n");
    printf("\tConnect from a browser on this computer with the URL \"http://127.0.0.1:8000/\".\n");
    printf("\tUse the URL \"http://<servername>:8000/\" from other computers.\n");
    printf("\n");
}


/*
static int
count_elements(const char * s)
{
    if(!s) return 0;
    size_t c = 0;
    size_t l = strlen(s);
    if(l==0)
        return 0;
    for(size_t i=0; i<l; i++)
        if(s[i] > ' ' && ((s[i+1] <= ' ' )|( s[i+1] == '\0')))
            c++;
    return (int)c;
}
*/


int
main(int argc, char *argv[])
{
    if (argc == 1)
    {
        PrintInfo();
        return 0;
    }

    Options * options = new Options(argc, argv);

    if (options->GetOption('v'))
        options->Print();

    // Create and Init kernel

    Kernel & k = kernel();    // Get global kernel
    k.SetOptions(options);
    
    try
    {
        k.ListInfo();

        k.Init();
        if(k.fatal_error_occurred)
            return 1;

        k.ListClasses();
        k.ListModulesAndConnections();
        k.CalculateChecksum();
        k.ListScheduling();
        k.ListThreads();
        k.ListWarningsAndErrors();
        k.ListBindings();

//       k.Notify(msg_print, "Starting Ikaros WebUI server.\n");
//        k.Notify(msg_print, "Connect from a browser on this computer with the URL \"http://localhost:%d/\".\n", k.port);
//        k.Notify(msg_print, "Use the URL \"http://<servername>:%d/\" from other computers.\n", k.port);

        k.Load();
        k.Run();
        k.Store();
        
        k.PrintTiming();
        k.ListProfiling();

        delete options;    
    }

    // Catch Exceptions and Terminate Execution

    catch(std::runtime_error)
    {
        return 1;
    }
    
    catch (std::bad_alloc&)
    {
        k.Notify(msg_exception, "Could not allocate memory. Program terminates.\n");
        return 1;	// MEMORY ERROR
    }

    catch (SerialException se)
    {
        k.Notify(msg_exception, "Serial Exception: %s (%s, %d). Program terminates.\n", se.device, se.string, se.internal_reference);
    }

    catch (SocketException ex)
    {
        k.Notify(msg_exception, "Socket(%d): %s\n", ex.internal_reference, ex.string);
        return 3;	// SOCKET ERROR
    }

    catch (int i)
    {
        //	k.Init();
        k.Notify(msg_exception, "%d. Program terminates.\n", i);
        return i;	// OTHER ERROR
    }

    for (Module * m: k._modules)
        delete m;
    delete &k;
    return 0;
}


