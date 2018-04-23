//
//    WebUI.h		Web UI code for the IKAROS project
//
//    Copyright (C) 2005-2018  Christian Balkenius
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
#include <atomic>

#include "IKAROS.h"

#ifdef USE_SOCKET

#ifndef     WEBUIPATH
#define		WEBUIPATH	"Source/WebUI_2.0/"
#endif

#define		PORT 8000

class WebUI;

// Data source type constants

const int data_source_float = bind_float;
const int data_source_int = bind_int;
const int data_source_bool = bind_bool;
const int data_source_list = bind_list;
const int data_source_array = bind_array;
const int data_source_matrix = bind_matrix;

const int data_source_gray_image = 6;
const int data_source_red_image = 7;
const int data_source_green_image = 8;
const int data_source_blue_image = 9;
const int data_source_fire_image = 10;
const int data_source_spectrum_image = 11; // not used
const int data_source_rgb_image = 12;
const int data_source_bmp_image = 13;


class DataSource
{
public:
    DataSource *	next;
    char *			name;    
    int             type;      
    void *          data;
    void *          data2; // for RGB-image data sources
    void *          data3;
    int             size_x;
    int             size_y;
    bool            base64;
    
    DataSource(const char * s, int type, void * data_ptr, int sx, int sy, DataSource * n);
    DataSource(const char * s, int type, void * data_ptr, void * data_ptr2, void * data_ptr3, int sx, int sy, DataSource * n);
    
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
    void			AddSource(const char * s, int type, void * value_ptr, void * value_ptr2, void * value_ptr3, int sx, int sy);
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
    bool            first_request;

    std::atomic<float *> ui_data;
    std::atomic<bool> copying_data;
    std::atomic<bool> dont_copy_data;
    std::atomic<bool> is_running;
    
    ModuleData *	view_data;
    
    WebUI(Kernel * kernel);
    ~WebUI();
    
    void			AddDataSource(const char * module, const char * source);
    void			AddImageDataSource(const char * module, const char * source, const char * type);
    void			AddParameterSource(const char * module, const char * source);
//    void			SendView(const char * view);
//    void            SendModule(Module * m);
//    void            SendGroups(XMLElement * xml);
//    void            SendInspector();
    void            SendXML();
    void			ReadXML(XMLDocument * xmlDoc);
    void            CopyUIData();
    void            SendUIData();
    void            Pause();
    void			HandleHTTPRequest();
    void            HandleHTTPThread();
    
    Thread *        httpThread;
    static void *   StartHTTPThread(void * webui);

    void			Run();
};

#endif
#endif

