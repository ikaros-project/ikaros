//
//    main.cc		The main code for the IKAROS project
//
//    Copyright (C) 2001-2007  Christian Balkenius
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

#ifdef WINDOWS
#include <direct.h>
#else
#include <unistd.h>
#endif

#ifdef WINDOWS32
#include <windows.h>
#undef GetClassName
#else
#include <unistd.h>
#endif

#include <new>

// Kernel

#include "IKAROS.h"
#include "WebUI/WebUI.h"

//
// Modules
//

#include "Modules/Modules.h"
#include "UserModules/UserModules.h"


void PrintInfo();

void
PrintInfo()
{
    printf("\n");
    printf("The file to use should be given at the command line:\n");
    printf("\n");
    printf("    IKAROS file.ikc\n");
    printf("\n");
    printf("Use the w-option to also start the WebUI:\n");
    printf("\n");
    printf("    IKAROS -w file.ikc\n");
    printf("\n");
    printf("This examaple will wait for a request from the Web browser at\n");
    printf("the default port (8000).\n");
    printf("\n");
    printf("Usage:\n");
    printf("\n");
    printf("\tIKAROS [-w#][-W#][-p][-t][-T][-b#][-r#][-v][-q][-x][-X][-m][-l][-i][-a][file]\n\n");
    printf("\t-w#   WebUI\n");
    printf("\t-W#   WebUI: debug mode, list requests\n");
    printf("\t-p    profile\n");
    printf("\t-t    threaded\n");
    printf("\t-T    list thread allocation\n");
    printf("\t-b#   run one item in batch\n");
    printf("\t-B    batch mode\n");
    printf("\t-r#   real time mode; # is time per tick in milliseconds\n");
    printf("\t-s#   stop after # ticks\n");
    printf("\t-v    verbose mode\n");
    printf("\t-q    quiet mode\n");
    printf("\t-x    list xml after parsing/combination\n");
    printf("\t-X    debug xml parser\n");
    printf("\t-n    look for NANs in outputs\n");
    printf("\t-m    list modules and connections\n");
    printf("\t-c    list installed classes\n");
    printf("\t-l    list scheduling\n");
    printf("\t-i    list installed functionality sockets, timer etc, type of target system\n");
    printf("\t-a    list all; implies -m -l and -T (if -t is set)\n");
    printf("\t-z#   seed random number generator\n");
    printf("\t-u#   number of ticks to run for each step in the WebUI\n");
    printf("\n");
}


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



int run_batch(Options * options);

int
run_batch(Options * options)
{
    printf("IKAROS: Running in batch mode.\n");
    
    // Set up path to load IKC file
    
    char * ikc_dir = options->GetFileDirectory();
    char * ikc_file_name = options->GetFileName();

    if (ikc_file_name[0] == 0)
    {
        printf( "Empty file name.\n");
        return -1;
    }
    
    char path[PATH_MAX];
    copy_string(path, ikc_dir, PATH_MAX);
    append_string(path, ikc_file_name, PATH_MAX);
        printf("IKAROS: Reading XML file \"%s\".\n", ikc_file_name);

    if (chdir(ikc_dir) < 0)
    {
        printf("IKAROS: The directory \"%s\" could not be found.\n", ikc_dir);
        return -1;
    }
    
    XMLDocument * xmlDoc = new XMLDocument(ikc_file_name, options->GetOption('X'));

    if (xmlDoc->xml == NULL)
    {
        printf("IKAROS: Could not read (or find) \"%s\".\n", ikc_file_name);
        return -1;
    }

    XMLElement * xml = xmlDoc->xml->GetElement("group");
    if (xml == NULL)
    {
        printf("IKAROS: Did not find <group> element in IKC file \"%s\".\n", ikc_file_name);
        return -1;
    }

    // Get max batch index


    printf("IKAROS: BATCH: targets: ");
    int batch_count = 0;
    bool first = true;
    for (XMLElement * xml_node = xml->GetContentElement(); xml_node != NULL; xml_node = xml_node->GetNextElement("batch"))
    {
        if(xml_node->GetAttribute("target") != NULL)
        {
            if(first)
                first = false;
            else
                printf(", ");
            printf("%s", xml_node->GetAttribute("target"));
        }
        int c = count_elements(xml_node->GetAttribute("values"));
        if(c > batch_count)
            batch_count = c;
    }
    printf("\n");
    printf("IKAROS: BATCH: count: %d\n", batch_count);
    
    // Change options
    
    options->ResetOption('B');
    // Loop over each execution
    
    for(int i=1; i<=batch_count; i++)
    {
        char n[6];
        snprintf(n, 5, "%d", i);
        options->SetOption('b', n);
        
        printf("\nIKAROS: BATCH: #%d\n", i);
        
        try
        {
            Kernel	k(options);

            k.ListInfo();

            InitModules(k);
            InitUserModules(k);

            k.Init();
            k.ListClasses();
            k.ListModulesAndConnections();
            k.ListScheduling();
            k.ListThreads();
            k.Run();
            k.PrintTiming();
            k.ListProfiling();
        }
        catch(...)
        {
            printf("IKAROS: BATCH: An error occured\n");
        }
    }

    // delete xml tree ***
    // delete options tree ***

    return 0;
}



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

    if (options->GetOption('B'))
    {
        return run_batch(options);
    }
    
    // Create and Init kernel

    Kernel	k(options);

    try
    {
        k.ListInfo();

        InitModules(k);
        InitUserModules(k);

        k.Init();

        k.ListClasses();
        k.ListModulesAndConnections();
        k.ListScheduling();
        k.ListThreads();

        // Select UI

        if (!k.Terminate() && (options->GetOption('w') || options->GetOption('W')))
        {
#ifdef USE_SOCKET
            WebUI webUI(&k);
            webUI.Run();
#else
            printf("IKAROS was compiled without support for sockets and WebUI\n");
#endif
        }
        else
        {
            k.Run();
        }

        k.PrintTiming();
        k.ListProfiling();

        delete options;
    }

    // Catch Exceptions and Terminate Execution

    catch (std::bad_alloc&)
    {
        k.Notify(msg_exception, "Could not allocate memory. Program terminates.\n");
        return 1;	// MEMORY ERROR
    }
    /*
    	catch(XMLError ex)
    	{
    		k.Notify(msg_exception, "%s at line %d. Program terminates (%d).\n", ex.string, ex.line, ex.internal_reference);
    		return 2;	// XML ERROR
    	}
    */
    
    catch (SerialException se)
    {
        k.Notify(msg_exception, "Serial Exception: %s (%d). Program terminates.\n", se.string, se.internal_reference);        
    }
#ifdef USE_SOCKET
    catch (SocketException ex)
    {
        k.Notify(msg_exception, "Socket(%d): %s\n", ex.internal_reference, ex.string);
        return 3;	// SOCKET ERROR
    }
#endif
    catch (int i)
    {
        //	k.Init();
        k.Notify(msg_exception, "%d. Program terminates.\n", i);
        return i;	// OTHER ERROR
    }
    catch (...)
    {
//        k.Init();
        k.Notify(msg_exception, "Undefined exception. Program terminates.\n");
        return -1;	// UNDEFINED ERROR
    }

    return 0;
}


