//
//  WebUI.h		Web UI code for the IKAROS project
//
//    Copyright (C) 2005-2010  Christian Balkenius
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
//	Created August 31, 2005
//

#ifndef WEBUI
#define WEBUI

#include <stdio.h>
#include <stdarg.h>

#include "IKAROS.h"

#ifdef USE_SOCKET

#ifndef     WEBUIPATH
#define		WEBUIPATH	"Source/WebUI/UI/"
#endif

#define		PORT 8000

class WebUI;

/*
 const int data_source_IO = 0;
 const int data_source_float = 1;
 const int data_source_array = 2;
 const int data_source_matrix = 3;
 */


class DataSource
{
public:
    DataSource *	next;
    char *			name;    
    int             type;   // 0 = Module_IO, 1 = float, 2 = array, 3 = matrix ...        
    void *          data;
    int             size_x;
    int             size_y;
    
    DataSource(const char * s, int type, void * data_ptr, int sx, int sy, DataSource * n);
    
    ~DataSource();
};



class ModuleData
{
public:
    ModuleData *	next;
    char *			name;
    Module *		module;
    DataSource *	source;
    
    ModuleData(const char * module_name, Module * m, ModuleData * n);
    ~ModuleData();
    
    void			AddSource(const char * s, Module_IO * io);
    void			AddSource(const char * s, int type, void * value_ptr, int sx, int sy);
};



const int ui_state_stop = 0;
const int ui_state_pause = 1;
const int ui_state_step = 2;
const int ui_state_run = 3;
const int ui_state_realtime = 4;



class WebUI
{
public:
    Kernel *		k;
    char *          webui_dir;
    int             port;
    ServerSocket *	socket;
    XMLElement *	xml;
    XMLElement *    current_xml_root;
    char *          current_xml_root_path;
    bool			debug_mode;
    bool			isRunning;
    long			tick;
    int             ui_state;
    int             iterations_per_runstep;
    
    ModuleData *	view_data;
    
    WebUI(Kernel * kernel);
    ~WebUI();
    
    void			AddDataSource(const char * module, const char * source);
    void			AddParameterSource(const char * module, const char * source);
    void			SendAllJSONData();
    void            SendMinimalJSONData();
    void			SendMenu(float x, float y, const char * name);
    void			Send404();
    void			SendView(const char * view);
    void            SendExamplesDirectory(const char * directory, const char * path);
    void            SendExamples();
    void            SendModule(Module * m);
    void            SendGroups(XMLElement * xml);
    void            SendInspector();
    void            SendXML();
    void            RunIKCFile(char * filepath);
    void			ReadXML(XMLDocument * xmlDoc);
    void            HandleRGBImageRequest(const char * module, const char * output);
    void			HandleHTTPRequest();
    void            HandleHTTPThread();
    
    Thread *        httpThread;
    static void *   StartHTTPThread(void * webui);

    void			Run();
};

#endif
#endif

