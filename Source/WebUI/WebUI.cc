//
//	WebUI.cc		HTTP support for the IKAROS kernel
//
//    Copyright (C) 2005-2011  Christian Balkenius
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
//    Created: August 31, 2005
//    Based on IKAROS_HTTP

#include "WebUI.h"

#ifdef USE_SOCKET
#include "Kernel/IKAROS_ColorTables.h"

#ifdef WINDOWS
#include <direct.h>
#else
#include <unistd.h>
#include <dirent.h> // for examples
#endif

#include <string.h>
#include <fcntl.h>

using namespace ikaros;



static bool
SendColorJPEG(ServerSocket * socket, float ** r, float ** g, float ** b, int sizex, int sizey)	// Compress image to jpg and send from memory
{
    Dictionary header;	
    header.Set("Content-Type", "image/jpeg");
	socket->SendHTTPHeader(&header);
    
    long  size;
    char * jpeg = create_jpeg(size, r, g, b, sizex, sizey);
    bool ok = socket->SendData(jpeg, size);
    destroy_jpeg(jpeg);
    return ok;
}



static bool
SendSpectrumJPEG(ServerSocket * socket, float ** matrix, int size_x, int size_y)
{
    Dictionary header;	
    header.Set("Content-Type", "image/jpeg");
	socket->SendHTTPHeader(&header);
	
    long  size;
    char * jpeg = create_jpeg(size, matrix, size_x, size_y, LUT_spectrum);
    bool ok = socket->SendData(jpeg, size);
    destroy_jpeg(jpeg);
    return ok;
}



static bool
SendFireJPEG(ServerSocket * socket, float ** matrix, int size_x, int size_y)
{
    Dictionary header;	
    header.Set("Content-Type", "image/jpeg");
	socket->SendHTTPHeader(&header);
	
    long  size;
    char * jpeg = create_jpeg(size, matrix, size_x, size_y, LUT_fire);
    bool ok = socket->SendData(jpeg, size);
    destroy_jpeg(jpeg);
    return ok;
}



static bool
SendGreenJPEG(ServerSocket * socket, float ** matrix, int size_x, int size_y)
{
    Dictionary header;	
    header.Set("Content-Type", "image/jpeg");
	socket->SendHTTPHeader(&header);
	
    long  size;
    char * jpeg = create_jpeg(size, matrix, size_x, size_y, LUT_green);
    bool ok = socket->SendData(jpeg, size);
    destroy_jpeg(jpeg);
    return ok;
}



static bool
SendGrayJPEG(ServerSocket * socket, float ** matrix, int size_x, int size_y)
{
    Dictionary header;	
    header.Set("Content-Type", "image/jpeg");
	socket->SendHTTPHeader(&header);
	
    float maximum, minimum;
    minmax(minimum, maximum, matrix, size_x, size_y);
    long  size;
    char * jpeg = create_jpeg(size, matrix, size_x, size_y, minimum, maximum);
    bool ok = socket->SendData(jpeg, size);
    destroy_jpeg(jpeg);
    return ok;
}



static bool
SendTextData(ServerSocket * socket, float ** matrix, int sizex, int sizey)
{
    Dictionary header;
    header.Set("Content-Type", "text/plain");
    socket->SendHTTPHeader(&header);
    
    for (int j=0; j<sizey; j++)
        for (int i=0; i<sizex; i++)
            if (i==0 && j==0)
                socket->Send("%.4f", matrix[0][0]);	// no space before first
            else
                socket->Send(" %.4f", matrix[j][i]);
	
    return true;
}



static float
checknan(float x)
{
	if(x == x && abs(x) > 0.00001) // Check NaN
		return x;
	else
		return 0;
}



static bool
SendJSONArrayData(ServerSocket * socket, char * module, char * source, float * array, int size) // TODO: use E-format
{
    if (array == NULL)
        return false;
	
    socket->Send("\t\"%s\":\n\t[\n", source);	// module
    socket->Send("\t\t[%.4E", checknan(array[0]));
    for (int i=1; i<size; i++)
        socket->Send(", %.4E", checknan(array[i]));
    socket->Send("]\n\t]");
	
    return true;
}



static bool
SendJSONMatrixData(ServerSocket * socket, char * module, char * source, float ** matrix, int sizex, int sizey) // TODO: use E-format
{
    if (matrix == NULL)
        return false;
	
    socket->Send("\t\"%s\":\n\t[\n", source);	// module
    for (int j=0; j<sizey; j++)
    {
        socket->Send("\t\t[%.4E", checknan(matrix[j][0]));
        for (int i=1; i<sizex; i++)
            socket->Send(", %.4E", checknan(matrix[j][i]));
        if (j<sizey-1)
            socket->Send("],\n");
        else
            socket->Send("]\n\t]");
    }
	
    return true;
}



void SendSyntheticModuleSVG(ServerSocket * socket, XMLElement * x, const char * module_name);

void
SendSyntheticModuleSVG(ServerSocket * socket, XMLElement * x, const char * module_name)
{
    char * module_ref = create_string(module_name);
    module_ref[strlen(module_ref)-16] = 0;
    
    // Go down the group tree (should use XPath functions)
    
    char * p = module_ref;
    char * group_in_path;
    XMLElement * group_xml = x;
    while((group_in_path = strsep(&p, "/")))
    {
        group_xml = group_xml->GetElement("group");
        for (XMLElement * xml_module = group_xml->GetContentElement("group"); xml_module != NULL; xml_module = xml_module->GetNextElement("group"))
            if(equal_strings(xml_module->GetAttribute("name"), group_in_path))
            {
                group_xml = xml_module;
                break;
            }
    }
    
    if(!group_xml)
        return;
    
    // TODO: Check if a specific svg exist for this class, otherwise use the code below
    
    // Send the SVG
    
    Dictionary header;
	
    //    header.Set("Connection",  "Close"); // TODO: check if socket uses persistent connections
    //    header.Set("Server", "Ikaros/1.2");
    header.Set("Content-Type", "image/svg+xml");
    
    socket->SendHTTPHeader(&header);
    
    socket->Send("<?xml version='1.0' encoding='utf-8'?>\n");
    socket->Send("\n");
    socket->Send("<g xmlns = 'http://www.w3.org/2000/svg'  name='%s'>\n", module_ref);
    socket->Send("<rect x='5' y='5' fill='#000' width='100' height='50' fill-opacity='0.25'/> \n");
    socket->Send("<rect x='0' y='0' fill='#FFF' stroke='#000' width='100' height='50' /> \n");
    socket->Send("<text x='50' y='30' text-anchor='middle' text-rendering='optimizeLegibility' style='font-size:11pt;font-family:sans-serif;' name='name'>ModuleName</text>\n");
    
    // Send Inputs
    
    if(x)
    {
        int c=0;
        for (XMLElement * xml_input = group_xml->GetContentElement("input"); xml_input != NULL; xml_input = xml_input->GetNextElement("input"))
            c++;
        int inc = 50/(c+1); // should also increase height of box if more inputs (or outputs) ***
        int i = inc;    
        
        for (XMLElement * xml_input = group_xml->GetContentElement("input"); xml_input != NULL; xml_input = xml_input->GetNextElement("input"))
        {
            socket->Send("<circle cx='0' cy='%d'  r='5' fill='red' stroke='black' name='%s' />\n", i, xml_input->GetAttribute("name"));
            i+= inc;
        }
        
        c=0;
        for (XMLElement * xml_input = group_xml->GetContentElement("output"); xml_input != NULL; xml_input = xml_input->GetNextElement("output"))
            c++;
        inc = 50/(c+1);
        i = inc;    
        
        for (XMLElement * xml_input = group_xml->GetContentElement("output"); xml_input != NULL; xml_input = xml_input->GetNextElement("output"))
        {
            socket->Send("<circle cx='100' cy='%d'  r='5' fill='green' stroke='black' name='%s' />\n", i, xml_input->GetAttribute("name"));
            i+= inc;
        }
    }
    
    socket->Send("<g name='selection' visibility='hidden'>\n");
    socket->Send("<rect x='0' y='0' width='100' height='50' fill='#555' fill-opacity='0.25'  />\n");
    socket->Send("</g>\n");
    socket->Send("</g>\n");
    
    destroy_string(module_ref);
}



DataSource::DataSource(const char * s, int t, void * data_ptr, int sx, int sy, DataSource * n)
{
    next = n;
    name = create_string(s);
    type = t;
    data = data_ptr;
    size_x = sx;
    size_y = sy;
}



DataSource::~DataSource()
{
    destroy_string(name);
    delete next;
}



ModuleData::ModuleData(const char * module_name, Module * m, ModuleData * n)
{
    name = create_string(module_name);
    module = m;
    next = n;
    source = NULL;
}



ModuleData::~ModuleData()
{
    destroy_string(name);
    delete next;
    delete source;
}



void
ModuleData::AddSource(const char * s, Module_IO * io)
{
    for (DataSource * sd=source; sd != NULL; sd=sd->next)
        if (!strcmp(sd->name, s))
            return;
	
    source = new DataSource(s, bind_matrix, io->matrix[0], io->sizex, io->sizey, source);
}



void
ModuleData::AddSource(const char * s, int type, void * value_ptr, int sx, int sy)
{
    for (DataSource * sd=source; sd != NULL; sd=sd->next)
        if (!strcmp(sd->name, s))
            return;
	
    source = new DataSource(s, type, value_ptr, sx, sy, source);
}



void
WebUI::AddDataSource(const char * module, const char * source)
{
    Module * m;
    Module_IO * io;
    void * value_ptr;
    int type, size_x, size_y;
    
//    printf("Adding data source: %s.%s\n", module, source);
    
    XMLElement * group = current_xml_root;
    
    if (group == NULL)
    {
        printf("WebUI ERROR: Could not find <group> or <modules> element.\n");
        return;
    }
	
    if (k->GetSource(group, m, io, module, source))
    {
        for (ModuleData * md=view_data; md != NULL; md=md->next)
            if (!strcmp(md->name, module))
            {
                md->AddSource(source, bind_matrix, io->matrix[0], io->sizex, io->sizey); // FIXME: Check that one [0] is ok!!!
                return;
            }
        
        view_data = new ModuleData(module, m, view_data);
        view_data->AddSource(source, io);
    }
	
    else if(k->GetBinding(m, type, value_ptr, size_x, size_y, module, source))
    {
        for (ModuleData * md=view_data; md != NULL; md=md->next)
            if (!strcmp(md->name, module))
            {
                md->AddSource(source, type, value_ptr, size_x, size_y);
                return;
            }
        
        view_data = new ModuleData(module, m, view_data);
        view_data->AddSource(source, type, value_ptr, size_x, size_y);
    }
    
    else
        printf("WebUI ERROR: Could not add data source %s.%s (not found)\n", module, source);
}



void
WebUI::SendAllJSONData()
{
    Dictionary header;
	
    header.Set("Content-Type", "text/json");
    header.Set("Cache-Control", "no-cache");	// make sure Opera reloads every time
    header.Set("Cache-Control", "no-store");
    header.Set("Pragma", "no-cache");
    //    header.Set("Expires", "0");
	
    socket->SendHTTPHeader(&header);
    socket->Send("{\n");
    socket->Send("state: %d,\n", ui_state);  // ui_state; ui_state_run
    socket->Send("iteration: %d", k->GetTick());
	
    if (view_data != NULL)
        socket->Send(",\n");
    else
        socket->Send("\n");
	
    for (ModuleData * md=view_data; md != NULL; md=md->next)
    {
        socket->Send("\"%s\":\n{\n", md->name);
        for (DataSource * sd=md->source; sd != NULL; sd=sd->next)
        {
            switch(sd->type)
            {
                    //              case bind_io:
                    //                   SendJSONMatrixData(socket, md->name, sd->name, sd->module_io->matrix[0], sd->module_io->sizex, sd->module_io->sizey); // [0] for first matrix
                    //                   break;
                    
                case bind_int:
                case bind_list:
                    socket->Send("\t\"%s\": [[%d]]", sd->name, *(int *)(sd->data));  // TODO: allow number of decimals to be changed - or use E-format
                    break;
                    
                case bind_bool:
                    socket->Send("\t\"%s\": [[%d]]", sd->name, (*(bool *)(sd->data) ? 1 : 0));  // TODO: allow number of decimals to be changed - or use E-format
                    break;
                    
                case bind_float:
                    socket->Send("\t\"%s\": [[%.4E]]", sd->name, *(float *)(sd->data));  // TODO: allow number of decimals to be changed - or use E-format
                    break;
                    
                case bind_array:
                    SendJSONArrayData(socket, md->name, sd->name, (float *)(sd->data), sd->size_x);
                    break;
                    
                case bind_matrix:
                    SendJSONMatrixData(socket, md->name, sd->name, (float **)(sd->data), sd->size_x, sd->size_y);
                    break;
            }
            
            if (sd->next != NULL)
                socket->Send(",\n");
            else
                socket->Send("\n");
        }
		
        if (md->next != NULL)
            socket->Send("},\n");
        else
            socket->Send("}\n");
    }
	
    socket->Send("}\n");
}



void
WebUI::SendMinimalJSONData()
{
    Dictionary header;
	
    header.Set("Content-Type", "text/json");
    header.Set("Cache-Control", "no-cache");	// make sure Opera reloads every time
    header.Set("Cache-Control", "no-store");
    header.Set("Pragma", "no-cache");
    //  header.Set("Expires", "0");
	
    socket->SendHTTPHeader(&header);
    socket->Send("{\n");
    socket->Send("state: %d,\n", ui_state);
    socket->Send("iteration: %d", k->GetTick());
    socket->Send("}\n");
}



WebUI::WebUI(Kernel * kernel)
{
    k = kernel;
    webui_dir = NULL;
    xml = NULL;
    current_xml_root = NULL;
    current_xml_root_path = create_string("");
    ui_state = ui_state_pause;
    view_data = NULL;
    debug_mode = false;
    isRunning = false;
	iterations_per_runstep = 1;
    if(k->options->GetOption('u'))
        iterations_per_runstep = string_to_int(k->options->GetArgument('u'));
    
    if (k->options->GetOption('w'))
    {
        port = string_to_int(k->options->GetArgument('w'), PORT);
        k->Notify(msg_verbose, "Setting up WebUI port at %d\n", port);
    }
	
    if (k->options->GetOption('W'))
    {
        port = string_to_int(k->options->GetArgument('W'), PORT);
        k->Notify(msg_verbose, "Setting up WebUI port at %d in debug mode\n", port);
        debug_mode = true;
    }
	
    k->Notify(msg_print, "Starting Ikaros WebUI server.\n");
    k->Notify(msg_print, "Connect from a browser on this computer with the URL \"http://127.0.0.1:%d/\".\n", port);
    k->Notify(msg_print, "Use the URL \"http://<servername>:%d/\" from other computers.\n", port);
	
    webui_dir = create_formatted_string("%s%s", k->ikaros_dir, WEBUIPATH);    
	
	//    printf("WebUI Path: %s\n", webui_dir);
	
	//    char * p = create_formatted_string("%s%s", webui_dir, "/log.tmp");
	//    k->logfile = fopen(p, "wb+");
	//    destroy_string(p);
    
    ReadXML(k->xmlDoc);
	
    socket =  new ServerSocket(port);
}



WebUI::~WebUI()
{
    delete socket;
    delete view_data;
    
    destroy_string(webui_dir);
}



// Layout constants

const float margin_x = 8.5;
const float margin_y = 8.5;

const float padding_x = 5.0;
const float padding_y = 7.0;

// const float object_spacing = 15.0;
// const float object_size = 140.0;	// was 70 / 100
const float title_height = 0; // **************** was: 20.0;

const float offset_x = margin_x+padding_x;
const float offset_y = margin_y+padding_y+title_height;

const float menu_offset = 15.0;
const float menu_width = 150.0;

const float view_min_width = 700;
const float view_min_height = 500;

void
WebUI::SendMenu(float x, float y, const char * name)
{
    int v = 0;
    for (XMLElement * xml_view = xml->GetContentElement("view"); xml_view != NULL; xml_view = xml_view->GetNextElement("view"), v++)
    {
        char buf[32] = "";
		
        const char * n = xml_view->GetAttribute("name");
		
        if (n == NULL)
        {
            sprintf(buf, "view%d", v);
            n = buf;
        }
		
        socket->Send("<a xlink:href='%s.svg'>", n);
		
        if (equal_strings(n, name))
            socket->Send("<rect x='%.2f' y='%.2f' width='150' height='20' rx='10' ry='10' fill='#404040' stroke='none' />\n", x, y+25*v);
        else
        {
            socket->Send("<rect x='%.2f' y='%.2f' width='150' height='20' rx='10' ry='10' fill='#1C1C1C' />\n", x, y+25*v);
            socket->Send("<circle cx='%.2f' cy='%.2f' r='7' fill='#888888' />", x+139, y+25*v+10);
            float a = x+139;
            float b = y+25*v+10;
            socket->Send("<polygon points='%.1f,%.1f %.1f,%.1f %.1f,%.1f %.1f,%.1f %.1f,%.1f %.1f,%.1f %.1f,%.1f' />",
                         a+4.5, b,
                         a+0.0, b-4.0,
                         a+0.0, b-1.0,
                         a-4.0, b-1.0,
                         a-4.0, b+1.0,
                         a+0.0, b+1.0,
                         a+0.0, b+4.0
						 );
        }
		
        socket->Send("<text x='%.2f' y='%.2f' style='font-size:12px;font-family:sans-serif;fill:#BABABA;'>%s</text>\n", x+15, y+25*v+15, n);
        socket->Send("</a>");
    }
    
    // Send Special Menu Item (Module List)
    
    socket->Send("<a xlink:href='modules'>");
    socket->Send("<rect x='%.2f' y='%.2f' width='150' height='20' rx='10' ry='10' fill='#1C1C1C' />\n", x, y+25*v);
    socket->Send("<circle cx='%.2f' cy='%.2f' r='7' fill='#888888' />", x+139, y+25*v+10);
    float a = x+139;
    float b = y+25*v+10;
    socket->Send("<polygon points='%.1f,%.1f %.1f,%.1f %.1f,%.1f %.1f,%.1f %.1f,%.1f %.1f,%.1f %.1f,%.1f' />",
                 a+4.5, b,
                 a+0.0, b-4.0,
                 a+0.0, b-1.0,
                 a-4.0, b-1.0,
                 a-4.0, b+1.0,
                 a+0.0, b+1.0,
                 a+0.0, b+4.0
				 );
    socket->Send("<text x='%.2f' y='%.2f' style='font-size:12px;font-family:sans-serif;fill:#BABABA;'>%s</text>\n", x+15, y+25*v+15, "Module List");
    socket->Send("</a>");
}



void
WebUI::Send404()
{
	
    if (debug_mode)
        printf("Sending 404.html\n");
	
    // Send Header
	
    Dictionary header;
    header.Set("Content-Type", "image/svg+xml");
    socket->SendHTTPHeader(&header, "404 Not Found");
	
    float view_width_px = view_min_width;
    float view_height_px = view_min_height;
	
    // Send SVG Header
	
    socket->Send("<?xml version='1.0' encoding='iso-8859-1' standalone='no'?>\n");
	//			socket->Send("<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\"  \"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\">\n");
	//			socket->Send("<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 20001102//EN\"  \"http://www.w3.org/TR/2000/CR-SVG-20001102/DTD/svg-20001102.dtd\">\n");
	//          socket->Send("<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 20010904//EN\" \"http://www.w3.org/TR/2001/REC-SVG-20010904/DTD/svg10.dtd\">\n"); // This doctype will keep Adobe SVG 3.0 plugin and Safari happy
    socket->Send("<svg\n");
    socket->Send("   width='%.2f'\n", 2*margin_x + view_width_px);
    socket->Send("   height='%.2f'\n", margin_y + margin_x + view_height_px);
    socket->Send("   version='1.0'\n");
    socket->Send("   xmlns='http://www.w3.org/2000/svg'\n");
    socket->Send("   xmlns:xlink='http://www.w3.org/1999/xlink'\n");
    socket->Send("   style='background-color: #303030'\n");
    socket->Send(">\n\n");
    socket->Send("<title>Ikaros Viewer</title>\n\n");
    socket->Send("<text x='40' y='40' fill='white'>This view can not be found.</text>\n");
    socket->Send("</svg>\n");
}



void
WebUI::SendView(const char * view)
{
    if (debug_mode)
        printf("Sending View: %s\n", view);
    
    // Get last part of view path
    
    int z=int(strlen(view))-1;
    while(z>0 && view[z] != '/')
        z--;
    char * view_name = create_string(&view[z]);
    
    // Traverse XML to find views at the current level
    
    char * group_ref = create_string(view);
    unsigned long i = strlen(group_ref)-1;
    while(group_ref[i] != '/')
    {
        group_ref[i] = 0;
        i--;
    }
    group_ref[i] = 0;

    char * pp = group_ref;
    char * group_in_path;
    XMLElement * group_xml = xml;
    while((group_in_path = strsep(&pp, "/")))
    {
        group_xml = group_xml->GetElement("group");
        for (XMLElement * xml_module = group_xml->GetContentElement("group"); xml_module != NULL; xml_module = xml_module->GetNextElement("group"))
            if(equal_strings(xml_module->GetAttribute("name"), group_in_path))
            {
                group_xml = xml_module;
                break;
            }
    }

    if(!group_xml)
        return;

    delete view_data;
    view_data = NULL;

    int v = 0;
    int j = 0;
    while(view_name[j] < '0' || view_name[j] > '9')
        j++;
	int ix = string_to_int(&view_name[j]);
    
    for (XMLElement * xml_view = group_xml->GetContentElement("view"); xml_view != NULL; xml_view = xml_view->GetNextElement("view"), v++)
    {
        const char * n = xml_view->GetAttribute("name");
        if (
            (n != NULL && !strncmp(&view_name[1], n, strlen(view_name)-5)) ||	// test view name
            (!strncmp(view_name, "/view", 5) && v == ix)   // FIXME: test view number 0-9 for backward compatibility
			)
        {
			
            float object_spacing = string_to_float(xml_view->GetAttribute("object_spacing"), 15.0);
            float object_size = string_to_float(xml_view->GetAttribute("object_size"), 140.0);
			
            // Calculate View Size
			
            int view_width = 0;
            int view_height = 0;
			
            for (XMLElement * xml_uiobject = xml_view->GetContentElement("object"); xml_uiobject != NULL; xml_uiobject = xml_uiobject->GetNextElement("object"))
            {
                // char * object_class = xml_uiobject->FindAttribute("class");
                int x = string_to_int(xml_uiobject->GetAttribute("x"), -1);
                int y = string_to_int(xml_uiobject->GetAttribute("y"), -1);
                int width = string_to_int(xml_uiobject->GetAttribute("w"), 1);
                int height = string_to_int(xml_uiobject->GetAttribute("h"), 1);
				
                if (x+width > view_width)
                    view_width = x+width;
				
                if (y+height > view_height)
                    view_height = y+height;
            }
			
            float view_width_px = 2*padding_x + view_width*object_size +(view_width-1)*object_spacing + menu_offset + menu_width;
            float view_height_px = 2*padding_y + view_height*(object_size+title_height)+(view_height-1)*object_spacing;
			
            // set minimum view size here if larger than what is set above (from <view size=.../>)
			
            view_width_px = max(view_min_width, view_width_px);
            view_height_px = max(view_min_height, view_height_px);
			
            // Send Header
			
            Dictionary header;
            header.Set("Content-Type", "image/svg+xml");
            socket->SendHTTPHeader(&header);
			
            // Send SVG Header
			
            socket->Send("<?xml version='1.0' encoding='iso-8859-1' standalone='no'?>\n");
			//	    socket->Send("<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\"  \"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\">\n");
			//        socket->Send("<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 20001102//EN\"  \"http://www.w3.org/TR/2000/CR-SVG-20001102/DTD/svg-20001102.dtd\">\n");
			//        socket->Send("<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 20010904//EN\" \"http://www.w3.org/TR/2001/REC-SVG-20010904/DTD/svg10.dtd\">\n"); // This doctype will keep Adobe SVG 3.0 plugin and Safari happy
            socket->Send("<?xml-stylesheet type='text/css' href='/viewer.css' ?>\n");  
            socket->Send("<svg\n");
            socket->Send("   width='%.2f'\n", 2*margin_x + view_width_px);
            socket->Send("   height='%.2f'\n", margin_y + margin_x + view_height_px);
            socket->Send("   version='1.0'\n");
            socket->Send("   xmlns='http://www.w3.org/2000/svg'\n");
            socket->Send("   xmlns:xlink='http://www.w3.org/1999/xlink'\n");
            socket->Send("   style='background-color: #303030'\n");
            socket->Send(">\n\n");
            socket->Send("<title>Ikaros Viewer</title>\n\n");
			
            socket->Send("<rect x='0' y='0' width='%.2f' height='%.2f' fill='#303030' />\n\n", 2*margin_x + view_width_px, margin_y + margin_x + view_height_px);
			
            //            socket->Send("<rect x='%.2f' y='%.2f' width='%.2f' height='%.2f' rx='5' ry='5' stroke='#666666' fill='#303030' />\n\n", margin_x, margin_y, view_width_px, view_height_px);
			
            const char * a = k->xmlDoc->xml->GetAttribute("title");
            if (a)
                socket->Send("<text x='%.2f' y='%.2f' fill='#FFFFFF' text-anchor='end' style='font-size:30px;font-family:sans-serif;fill:#BABABA;'>%s</text>\n", view_width_px, margin_y-25, a);
			
            // Include the file "viewer.svg"
			
            char path[1024] = "";
            append_string(path, webui_dir, 1024);
            FILE * f = fopen(append_string(path, "viewer.svg", 1024), "r+b");
            if(!f)
            {
                printf("Cannot find \"viewer.svg\"; webui_dir=\"%s\"\n", webui_dir);
                socket->Send("</svg>\n");
                return;
            }
            fseek(f, 0, SEEK_END);
            size_t len  = ftell(f);
            fseek(f, 0, SEEK_SET);
            char * s = new char[len];
            fread(s, len, 1, f);
            socket->SendData(s, len);
            delete [] s;
            fclose(f);
			
            socket->Send("<script type='text/ecmascript'>\n");
            socket->Send("<![CDATA[\n");
			socket->Send("try {\n");
            
            // Send the UIObjects
			
            float ** occ = create_matrix(50, 50); // max elements in view
            for (XMLElement * xml_uiobject = xml_view->GetContentElement("object"); xml_uiobject != NULL; xml_uiobject = xml_uiobject->GetNextElement("object"))
            {
                const char * object_class = xml_uiobject->GetAttribute("class");
                int x = string_to_int(xml_uiobject->GetAttribute("x"), 0);
                int y = string_to_int(xml_uiobject->GetAttribute("y"), 0);
                int width = string_to_int(xml_uiobject->GetAttribute("w"), 1);
                int height = string_to_int(xml_uiobject->GetAttribute("h"), 1);
                
                // Check opacity
                
                bool behind = true;
                for(int j=0; j<height; j++)
                    for(int i=0; i<width; i++)
                        if(occ[y+j][x+i] == 1)
                            behind = false;
                        else
                            occ[y+j][x+i] = 1;
				
                socket->Send("add(new %s({", object_class);
                for (XMLAttribute * p = xml_uiobject->attributes; p != NULL; p = (XMLAttribute *)(p->next))
                    if (!(!strcmp(p->name, "x") || !strcmp(p->name, "y") || !strcmp(p->name, "w") || !strcmp(p->name, "h") || !strcmp(p->name, "class")))
                    {
                        // Test number - do this properly later
                        unsigned long l = strlen(p->value)-1;
                        if ((('0' <= p->value[0] && p->value[0] <='9') || p->value[0] == '-') && !strstr(p->value, ",") && ('0' <= p->value[l] && p->value[l] <='9'))
                            socket->Send("%s:%s, ", p->name, p->value);
                   //     else if (p->value[0] == '[')
                   //         socket->Send("%s:'%s', ", p->name, p->value);	// arrays are NOT sent verbatim anymore - check that this is never necessary
                        else
                            socket->Send("%s:'%s', ", p->name, p->value); // quote other content
                    }
				
                if(behind)
                    socket->Send("behind:true, ");
                else
					socket->Send("behind:false, ");
				
                socket->Send("x:%.2f, y:%.2f, width:%.2f, height:%.2f}));\n",
                             offset_x+(object_size+object_spacing)*x, offset_y+(title_height+object_size+object_spacing)*y,
                             object_size+(object_size+object_spacing)*float(width-1), object_size+(title_height+object_size+object_spacing)*float(height-1));
            }
            
            destroy_matrix(occ);
			
            // Send Footer
            
			socket->Send("} catch(err) { alert('Loading Object Error: '+err.message); }\n");
            socket->Send("]]>\n");
            socket->Send("</script>\n");
            socket->Send("</svg>\n");
            return;
        }
    }
	
    Send404();
}



void
WebUI::Run()
{
    if(socket == NULL)
        return; // ERROR
	
	// SYNCHRONIZATION
	
	if(xml)
	{
		const char * ip = xml->GetAttribute("masterip");
		if(ip)
		{
			Socket s;
			char rr[100];
			int masterport = string_to_int(xml->GetAttribute("masterport"), 9000);
			printf("Waiting for master: %s:%d\n", ip, masterport);
			fflush(stdout);
			if(!s.Get(ip, masterport, "*", rr, 100))
			{
				printf("Master not running.\n");
				throw -1; // No master
			}
		}
	}
	
    chdir(webui_dir);
    k->timer->Restart();
    tick = 0;
    
#ifdef USE_THREADED_WEBUI
    {
        httpThread = new Thread();
        httpThread->Create(WebUI::StartHTTPThread, this);

        while (!k->Terminate())
        {
            if (!isRunning)
            {
                Timer::Sleep(10);	 // Wait 10ms to avoid wasting cycles if there are no requests
            }
            
            if (isRunning)
            {
                chdir(k->ikc_dir);
                k->Tick();
                tick++;
                chdir(webui_dir);
                
                if (k->tick_length > 0)
                {
                    float lag = k->timer->WaitUntil(float(tick*k->tick_length));
                    if (lag > 0.1) k->Notify(msg_warning, "Lagging %.2f ms at tick = %ld\n", lag, k->tick);
                }
            }
        }

        httpThread->Join();
        chdir(k->ikc_dir);
    }
#else   // Old Unthreaded WebUI
    {
        while (!k->Terminate())
        {
            if (socket->GetRequest())
            {
                if (equal_strings(socket->header.Get("Method"), "GET"))
                    HandleHTTPRequest();
                socket->Close();
            }
            else if (!isRunning)
            {
                Timer::Sleep(10);	 // Wait 10ms to avoid wasting cycles if there are no requests
            }
            
            if (isRunning)
            {
                chdir(k->ikc_dir);
                k->Tick();
                tick++;
                chdir(webui_dir);
            
                if (k->tick_length > 0)
                {
                    float lag = k->timer->WaitUntil(float(tick*k->tick_length));
                    if (lag > 0.1) k->Notify(msg_warning, "Lagging %.2f ms at tick = %ld\n", lag, k->tick);
                }
            }

        }
        chdir(k->ikc_dir);
    }
#endif    
}



void
WebUI::HandleRGBImageRequest(const char * module, const char * output)
{
    char m1[256], m2[256], m3[256];
	
    if (sscanf(module, "%[^+]+%[^+]+%[^+]", m1, m2, m3) == 1) // use same module for all sources if only one name is given
    {
        copy_string(m2, m1, 255);
        copy_string(m3, m1, 255);
    }
	
    bool err = false;
    char red[256], green[256], blue[256];
    int cc = sscanf(output, "%[^+]+%[^+]+%[^+]", red, green, blue);
	
    Module * mod1, * mod2, * mod3;
    Module_IO * source1, * source2, * source3;
	
    err |= !k->GetSource(current_xml_root, mod1, source1, m1, red);     // was xml *********
    err |= !k->GetSource(current_xml_root, mod2, source2, m2, green);
    err |= !k->GetSource(current_xml_root, mod3, source3, m3, blue);
    
    if (cc != 3) err = true;	// Error
	
    if(!err)
    {
        if (source1->sizex != source2->sizex) err = true;	// ERROR
        if (source1->sizey != source2->sizey) err = true;	// ERROR
        if (source1->sizex != source3->sizex) err = true;	// ERROR
        if (source1->sizey != source3->sizey) err = true;	// ERROR
    }
    
    if (!err)
        SendColorJPEG(socket, source1->matrix[0], source2->matrix[0], source3->matrix[0], source1->sizex, source1->sizey);
    else
        socket->SendFile("404.jpg", webui_dir);
}



void
WebUI::HandleHTTPRequest()
{
    if (debug_mode)
        printf("HTTP Request: %s %s\n", socket->header.Get("Method"), socket->header.Get("URI"));
	
    // Copy URI and remove index
    
    char * uri = create_string(socket->header.Get("URI"));
	if(char * x = strpbrk(uri, "?#")) *x = '\0';
    
    if (!strcmp(uri, "/stop"))
    {
        ui_state = ui_state_stop;
        SendAllJSONData();
        k->Notify(msg_terminate, "Sent by WebUI.\n");
    }
	
    else if (!strcmp(uri, "/step"))
    {
        ui_state = ui_state_pause;
        chdir(k->ikc_dir);
        
        for(int i=0; i<iterations_per_runstep; i++)
            k->Tick();
        
        chdir(webui_dir);
        SendAllJSONData();
        isRunning = false;
    }
	
    else if (!strcmp(uri, "/runstep"))
    {
        ui_state = ui_state_run;
        chdir(k->ikc_dir);
        
        for(int i=0; i<iterations_per_runstep; i++)
            k->Tick();
        
        chdir(webui_dir);
        SendAllJSONData();
        isRunning = false;
    }

    else if (!strcmp(uri, "/update"))
    {
        SendAllJSONData();
    }
	
    else if (!strcmp(uri, "/pause"))
    {
        ui_state = ui_state_pause;
        SendAllJSONData();
        isRunning = false;
    }
	
    else if (!strcmp(uri, "/realtime"))
    {
        ui_state = ui_state_realtime;
        socket->Send("REALTIME\n");
        k->timer->Restart();
        tick = 0;
        isRunning = true;
    }
	
    else if (strstart(uri, "/setroot"))
    {
        if(current_xml_root_path) destroy_string(current_xml_root_path);
        current_xml_root_path = create_string(&uri[8]);
        char * p = create_string(uri);  // TODO: write as separate function (XPath style?)
        char * group_in_path;
        XMLElement * group_xml = xml;
        group_in_path = strsep(&p, "/");
        group_in_path = strsep(&p, "/");
        while((group_in_path = strsep(&p, "/")))
        {
            group_xml = group_xml->GetElement("group");
            for (XMLElement * xml_module = group_xml->GetContentElement("group"); xml_module != NULL; xml_module = xml_module->GetNextElement("group"))
                if(equal_strings(xml_module->GetAttribute("name"), group_in_path))
                {
                    group_xml = xml_module;
                    break;
                }
        }
        current_xml_root = group_xml;
        destroy_string(p);
    }
    
    else if (strstart(uri, "/uses"))
    {
        char * module = new char [256];
        char * output = new char [256];
        int c = sscanf(uri, "/uses/%[^/]/%[^/]", module, output);
        if (c == 2)
            AddDataSource(module, output);
		
		Dictionary header;
		header.Set("Content-Type", "text/plain");
		socket->SendHTTPHeader(&header);
        socket->Send("OK\n");
        
		delete module;
        delete output;
    }

    else if (strstart(uri, "/run/"))
    {
        char filepath[1024];
        printf("RUN\n");
        sscanf(uri, "/run/%s", filepath);
        RunIKCFile(filepath);
        //        return;
    }
    
    else if (strstart(uri, "/control/"))
    {
        char module_name[255];
        char name[255];
        int x, y;
        float value;
        int c = sscanf(uri, "/control/%[^/]/%[^/]/%d/%d/%f", module_name, name, &x, &y, &value);
        if(c == 5)
        {
            Module * module = k->GetModule(module_name);
            if(!module)
            {
                k->Notify(msg_warning, "Module \"%s\" does not exist.\n", module_name);
                destroy_string(uri); // TODO: should use single exit point
                return;
            }
            
            for(Binding * b = module->bindings; b != NULL; b = b->next)
                if(equal_strings(name, b->name))
                {
                    if(b->type == bind_float)
                        *((float *)(b->value)) = value;
                    else if(b->type == bind_int || b->type == bind_list)
                        *((int *)(b->value)) = (int)value;
                    else if(b->type == bind_float)
                        *((bool *)(b->value)) = (value > 0);
                    else if(b->type == bind_array)
                        ((float *)(b->value))[x] = value;     // TODO: add range check!!!
                    else if(b->type == bind_matrix)
                        ((float **)(b->value))[y][x] = value;
                }
        }
        
		Dictionary header;
		header.Set("Content-Type", "text/plain");
		header.Set("Cache-Control", "no-cache");	// make sure Opera reloads every time
		header.Set("Cache-Control", "no-store");
		header.Set("Pragma", "no-cache");
		socket->SendHTTPHeader(&header);
        socket->Send("OK\n");
        // return;
    }
    
    else if (!strcmp(uri, "/getlog"))
    {
        if (k->logfile)
            socket->SendFile("logfile", webui_dir);
        else
            socket->Send("ERROR - No logfile found\n");
    }
    
    else if (strend(uri, "/editor.svg"))
    {
        socket->SendFile("editor.svg", webui_dir);
    }
    
    else if (!strcmp(uri, "/editor.js"))
    {
        socket->SendFile("editor.js", webui_dir);
    }
    
    else if (!strcmp(uri, "/ikcfile.js"))
    {
        socket->Send("getXML('/xml.ikc', function(xml) {bg.read_modules(xml);});\n");
    }
    
    else if(strend(uri, "/modulegraph.svg"))
    {
        SendSyntheticModuleSVG(socket, k->xmlDoc->xml, uri);
    }
    
    else if(strstart(uri, "/view") && strend(uri, ".svg")) // second test not necessary in the future
    {
        SendView(uri);
	}
    
    else if (strstart(uri, "/module/"))
    {
        char module[256], output[256], type[256];
        int c = sscanf(uri, "/module/%[^/]/%[^/]/%[^/]", module, output, type);
        if(!strcmp(type, "rgb.jpg"))
        {
            HandleRGBImageRequest(module, output);
            destroy_string(uri); // TODO: should use single exit point
            return;
        }
		
        Module * m;
        Module_IO * source;
        
        if(!k->GetSource(current_xml_root, m, source, module, output))
        {
            k->Notify(msg_warning, "The output \"%s.%s\" does not exist, or\n", module, output);
            k->Notify(msg_warning, "\"%s\" may be an unkown data type.\n", type);
            destroy_string(uri); // TODO: should use single exit point
            
            return;
        }
        
        if (c == 3 && !strcmp(type, "gray.jpg"))
        {
            if (!SendGrayJPEG(socket, source->matrix[0], source->sizex, source->sizey))
                socket->SendFile("no_image.jpg", webui_dir);
        }
		
        else if (c == 3 && !strcmp(type, "green.jpg"))
        {
            if (!SendGreenJPEG(socket, source->matrix[0], source->sizex, source->sizey))
                socket->SendFile("404.jpg", webui_dir);
        }
		
        else if (c == 3 && !strcmp(type, "spectrum.jpg"))
        {
            if (!SendSpectrumJPEG(socket, source->matrix[0], source->sizex, source->sizey))
                socket->SendFile("404.jpg", webui_dir);
        }
		
        else if (c == 3 && !strcmp(type, "fire.jpg"))
        {
            if (!SendFireJPEG(socket, source->matrix[0], source->sizex, source->sizey))
                socket->SendFile("404.jpg", webui_dir);
        }
		
        else if (c == 3 && !strcmp(type, "data.txt"))
        {
            int sx = m->GetOutputSizeX(output);
            int sy = m->GetOutputSizeY(output);
            if (!SendTextData(socket, m->GetOutputMatrix(output), sx, sy))
                k->Notify(msg_warning, "Could not send: data.txt\n");
        }
        
        else
        {
            k->Notify(msg_warning, "Unkown data type: %s\n", type);
        }
    }
	
    else if (!strcmp(uri, "/examples"))
    {
        SendExamples();
    }
	
    else if(strend(uri, "/inspector.html"))
    {
        SendInspector();
    }

    else if(strstart(uri, "/xml"))
    {
        SendXML();
    }
	
    else if(equal_strings(uri, "/"))
    {
        socket->SendFile("index.html", webui_dir);
    }
    
    else if (
			 strend(uri, ".xml") ||
			 strend(uri, ".jpg") ||
			 strend(uri, ".html") ||
			 strend(uri, ".css") ||
			 strend(uri, ".png") ||
			 strend(uri, ".svg") ||
			 strend(uri, ".js") ||
			 strend(uri, ".gif") ||
			 strend(uri, ".ico"))
    {
        if(!socket->SendFile(&uri[1], k->ikc_dir))  // Check IKC-directory first to allow files to be overriden
        if(!socket->SendFile(&uri[1], webui_dir))   // Now look in WebUI directory
        {
            // Send 404 if not file found
			
            if (strend(uri, ".jpg"))
                socket->SendFile("404.jpg", webui_dir);
            else if (strend(uri, ".html"))
                socket->SendFile("404.html", webui_dir);
        }
    }
	
    else if (strstart(uri,"/"))
    {
/*
        int v = 0;
        if (!xml || xml->GetContentElement("view") == NULL)
        {
            socket->Send("<html>\n<head>\n<title>Ikaros Web View</title>\n</head>\n<body>\n<h1>Ikaros Web View</h1>\n<p>No views were defined.</p></ul>\n</body>\n</html>");
            destroy_string(uri); // TODO: should use single exit point
            return;
        }
		
        // Send index page
		
        socket->Send("<html>\n<head>\n<title>Ikaros Web View</title>\n</head>\n");
        socket->Send("<body style='background-color: black; color: white; font-family: sans-serif; font-size: 0.9em; padding: 20px'>\n");
        socket->Send("<h1>Ikaros Web View</h1>\n<p>The following views are available:</p>\n<ul>");
        for (XMLElement * xml_view = xml->GetContentElement("view"); xml_view != NULL; xml_view = xml_view->GetNextElement("view"), v++)
        {
            const char * n = xml_view->GetAttribute("name");
            if (n == NULL)
                socket->Send("   <li style='padding: 2px'><a  style='color:white; text-decoration: none'  href=\"view%d.svg\">View %d</a></li>\n", v, v);
            else
                socket->Send("   <li style='padding: 2px'><a  style='color:white; text-decoration: none'  href=\"%s.svg\">%s</a></li>\n", n, n);
        }
*/		
        socket->Send("ERROR\n");
    }
    
    destroy_string(uri);
}



void
WebUI::HandleHTTPThread()
{
    while(!k->Terminate())
    {
        if (socket->GetRequest(true))
        {
            if (equal_strings(socket->header.Get("Method"), "GET"))
                HandleHTTPRequest();
            socket->Close();
        }
    }
}



void *
WebUI::StartHTTPThread(void * webui)
{
    ((WebUI *)(webui))->HandleHTTPThread();
    return NULL;
}



// This is an experimental POSIX only function which sends a HTML page with all ikc files in the Examples directory

void
WebUI::SendExamplesDirectory(const char * directory, const char * path)
{
#ifdef POSIX
	struct dirent * dp;
	DIR * dirp = opendir(path);
	if(!dirp)
        return;
	
	Dictionary header;
	header.Set("Content-Type", "text/xhtml");
	socket->SendHTTPHeader(&header);
    
    socket->Send("<ul>\n");
	while ((dp = readdir(dirp)) != NULL)
        if(strstr(dp->d_name,".ikc")) // should test only at end ***
		    socket->Send("<li><a href=\"/run/%s/%s\">%s</a></li>\n", directory, dp->d_name, dp->d_name);
        else if(!strstr(dp->d_name, ".") && !equal_strings(dp->d_name, "Media")) // assume if it is directory
        {
            socket->Send("<li>%s:</li>\n", dp->d_name);
            char * p = create_formatted_string("%s/%s", path, dp->d_name);
            char * d = create_formatted_string("%s/%s", directory, dp->d_name);
            SendExamplesDirectory(d, p);
            destroy_string(d);
            destroy_string(p);
		}
    socket->Send("</ul>\n");
	
	closedir(dirp);
#endif
}



void
WebUI::SendExamples()
{
#ifdef POSIX
    socket->Send("<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">\n");
    socket->Send("<html xmlns=\"http://www.w3.org/1999/xhtml\" xml:lang=\"en\" lang=\"en\">\n");
    socket->Send("<head profile=\"http://www.w3.org/2005/11/profile\">\n");
    socket->Send("<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\" />\n");
    socket->Send("<title>Modules</title>\n");
    socket->Send("<style>\n");
    socket->Send("body {background-color: black; color: white; font-family: sans-serif; font-size: 0.9em; padding: 20px}\n");
    socket->Send("table {background-color: #333; color: white; font-family: sans-serif; font-size: 0.9em; border: 1px solid gray; border-collapse: collapse; margin-top: 20px; width: 400px}\n");  //  display: none;
    socket->Send(".io {background-color: #555; font-size:  1em; margin: 0px; border: 1px solid gray; width: 400px }\n");
    socket->Send("td {font-size: 0.9em; padding: 2px; vertical-align: top; padding: 5px}\n");
    socket->Send("li {padding: 5px}\n");
    socket->Send("a {color: white; text-decoration: none}\n");
    socket->Send("a:hover {color: white; text-decoration: underline}\n");
    socket->Send("\n");
    socket->Send("</style>\n");
    socket->Send("</head>\n");
    socket->Send("<body>\n");
    socket->Send("<h1>Ikaros Web View</h1>\n<p>The following examples are available in the Examples directory.<br />Start an example by clicking on its title.</p>\n");
	
    char * directory = create_formatted_string("%sExamples/", k->ikaros_dir);
	struct dirent * dp;
	DIR * dirp = opendir(directory);
	if(!dirp)
        return;
	
	while ((dp = readdir(dirp)) != NULL)
        if(strend(dp->d_name,".ikc"))
		    socket->Send("<p style='padding: 3px'><a href=\"/run/%s\">%s</a></p>\n", dp->d_name, dp->d_name);
        else if(!strstr(dp->d_name, ".") && !equal_strings(dp->d_name, "Media")) // assume it is a directory
        {
            socket->Send("<table style='border-collapse: collapse; border: 1px solid gray'>\n");
            socket->Send("<tr><th style='background-color: gray; border: 1px solid gray; padding: 3px; color: black'>%s</th></tr>\n", dp->d_name);
            socket->Send("<tr><td>\n");
            char * d = create_formatted_string("%s/%s", directory, dp->d_name);
            SendExamplesDirectory(dp->d_name, d);
            destroy_string(d);
            socket->Send("</td></tr>\n");
            socket->Send("</table>\n");
		}
	
	closedir(dirp);
    destroy_string(directory);
    socket->Send("</body>\n</html>\n");
    socket->Close();
#endif
}



// Send XML sends the XML tree

void
WebUI::SendXML()
{
    FILE * f = fopen("temp.xml","w");
    if(!f) return;
    if(k->xmlDoc)
        k->xmlDoc->xml->Print(f, 0);
    fclose(f);
    
    socket->SendFile("temp.xml");
    socket->Close();
}



void
WebUI::SendModule(Module * m)
{
        socket->Send("<table>\n");
		
        m->GetFullName();
		
        socket->Send("<tr><th colspan='2' align='center' style='background-color: gray; border: 1px solid gray; padding: 3px; color: black'><id=\"%s\">\n", m->GetName());
        const char * n = m->GetFullName();
        socket->Send("%s", n);
        socket->Send("</th></tr>\n");
		
        socket->Send("<tr><td>Class:</td><td><span class='classname' onclick='toggle_class_info(this)'>%s</span>\n", m->class_name, m->class_name);
		
        XMLElement * gg = m->xml->GetParentElement();
        if(gg)
        {
            XMLElement * x = gg->GetContentElement("description");
            if(x)
            {
                XMLCharacterData * c = (XMLCharacterData *)(x->content);
                if(c)
                {
                    char * description = c->data;
                    socket->Send("<br /><div class='hidden' style='width: 400px; padding-top: 10px; color: #BBB'>%s <a href=\"http://www.ikaros-project.org/module/%s\" target='_window'><img style='vertical-align: middle' src='link.png'/></a></div>\n", description, m->class_name);
                }
            }
        }

        socket->Send("</td></tr>\n");

        // Parameters
        
        socket->Send("<tr><td>Parameters:</td><td>\n");
        socket->Send("<table class=\"io\">\n");
        
        socket->Send("<tr><td>period</td><td width='172' align='right'>%d</td><td width='25' align='right'>int</td></tr>\n", m->period);
        socket->Send("<tr><td>phase</td><td width='172' align='right'>%d</td><td width='25' align='right'>int</td></tr>\n", m->phase);
		
        XMLElement * g = m->xml->GetParentElement();
        if(g)
            for (XMLElement * parameter = g->GetContentElement("parameter"); parameter != NULL; parameter = parameter->GetNextElement("parameter"))
            {
                const char * name = parameter->GetAttribute("name");
                const char * type = parameter->GetAttribute("type");
                const char * description = parameter->GetAttribute("description");
				
                // TODO: Allow editing if bound
                // ****
                
                socket->Send("<tr>\n");
                socket->Send("<td  title='%s'><div class='paramname' onclick='toggle_parameter_info(this)'>%s</div><div class='hidden' style='padding-top: 10px; padding-left: 10px; color: #BBB;'>%s</div></td>\n", description, name, description);
                if(type)
                {
                    if(equal_strings(type, "int"))
                        socket->Send("<td width='172' align='right'>%d</td>\n", m->GetIntValue(name));
                    else if(equal_strings(type, "float"))
                        socket->Send("<td width='172' align='right'>%f</td>\n", m->GetFloatValue(name));
                    else if(equal_strings(type, "bool"))
                        socket->Send("<td width='172' align='right'>%s</td>\n", (m->GetBoolValue(name) ? "yes" : "no"));
                    else if(equal_strings(type, "list"))
                        socket->Send("<td width='172' align='right'>%s</td>\n", m->GetValue(name));
                    else if(equal_strings(type, "string"))
                        socket->Send("<td width='172' align='right'>%s</td>\n", m->GetValue(name));
                    else
                        socket->Send("<td width='172' align='right'>-</td>\n");
					
                    socket->Send("<td width='25' align='right'>%s</td>\n", type);
                }
                else
                {
                    socket->Send("<td width='172' align='right'></td>\n");
                    socket->Send("<td width='25' align='right' style='color: red' title='The type of \"%s\" has not been specified in the class file \"%s.ikc\".'>ERR</td>\n", name, m->class_name);
                }
				
                socket->Send("</tr>\n");
            }
        socket->Send("</table>\n");
        socket->Send("</td></tr>\n");
		
        // Inputs

        if(m->input_list)
        {
            socket->Send("<tr><td>Inputs:</td><td>\n");
            socket->Send("<table class=\"io\">\n");
            for (Module_IO * i = m->input_list; i != NULL; i = i->next)
            {
                socket->Send("<tr>\n");
                if(!i->data)
                {
                    socket->Send("<td>%-10s</td>\n", i->name);
                    socket->Send("<td width='25' align='right'> </td>\n");
                    socket->Send("<td width='25' align='right'> </td>\n");
                    socket->Send("<td width='100' align='right' title='Not connected'>nc</td>\n");
                    socket->Send("<td width='25' align='right'>&nbsp;</td>\n");
				}
                else
                {
                    socket->Send("<td>%-10s</td>\n", i->name);
                    socket->Send("<td width='25' align='right'>%d</td>\n", i->sizex);
                    socket->Send("<td width='25' align='right'>%d</td>\n", i->sizey);
                    socket->Send("<td width='100' align='right'>%p</td>\n", (i->data == NULL ? NULL : i->data[0]));
                    socket->Send("<td width='25' align='right'>&nbsp;</td>\n");
                }
                socket->Send("</tr>\n", i->name, i->sizex, i->sizey, (i->data == NULL ? NULL : i->data[0]));
            }
            socket->Send("</table>\n");
            socket->Send("</td></tr>\n");
        }
        
        // Outputs
        
        if(m->output_list)
        {
            socket->Send("<tr><td>Outputs:</td><td>\n");
            socket->Send("<table class=\"io\">\n");
			for (Module_IO * i = m->output_list; i != NULL; i = i->next)
			{
				socket->Send("<tr>\n");
				socket->Send("<td>%-10s</td>\n", i->name);
				socket->Send("<td width='25' align='right'>%d</td>\n", i->sizex);
				socket->Send("<td width='25' align='right'>%d</td>\n", i->sizey);
				if(m->OutputConnected(i->name))
					socket->Send("<td width='100' align='right'>%p</td>\n", (i->data == NULL ? NULL : i->data[0]));
				else
					socket->Send("<td width='100' align='right' title='Not connected'>nc, %p</td>\n", (i->data == NULL ? NULL : i->data[0]));
				socket->Send("<td width='25' align='right'>%d</td>\n", i->max_delay);
				socket->Send("</tr>\n");
			}
            socket->Send("</table>\n");
            socket->Send("</td></tr>\n");
        }
        
		socket->Send("</table><p></p>\n");
}



void
WebUI::SendGroups(XMLElement * xml)
{
    for(XMLNode * e = xml->content; e != NULL; e = e->next)
        if(e->IsElement())
        {
            if(equal_strings(((XMLElement *)(e))->name, "group"))
                SendGroups((XMLElement *)(e));
            else if(equal_strings( ((XMLElement *)(e))->name, "module") && ((XMLElement *)(e))->aux)
                SendModule((Module *)((XMLElement *)(e)->aux));
        }
}



void
WebUI::SendInspector()
{
    socket->Send("<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">\n");
    socket->Send("<html xmlns=\"http://www.w3.org/1999/xhtml\" xml:lang=\"en\" lang=\"en\">\n");
    socket->Send("<head profile=\"http://www.w3.org/2005/11/profile\">\n");
    socket->Send("<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\" />\n");
    socket->Send("<title>Modules</title>\n");
    socket->Send("<link rel='stylesheet' type='text/css' href='/inspector.css' />\n");
    socket->Send("<script src='/inspector.js'></script>");
    socket->Send("</head>\n");
    socket->Send("<body>\n");
    
    if(!k->modules)
    {
        socket->Send("<p>No modules</p>\n");
    }
/*    
    for (Module * m = k->modules; m != NULL; m = m->next)
    {
        SendModule(m);
    }
*/	
    SendGroups(current_xml_root);
    
    // Connections
    
    socket->Send("<table>\n");
    
    socket->Send("<tr><th colspan='4' align='center' style='background-color: gray; border: 1px solid gray; padding: 3px; color: black'>Connections</th></tr>\n");
	
	if(!k->connections)
    {
        socket->Send("<tr><td colspan='4' style='border: 1px solid gray;' >No connections</td></tr>\n");
    }
	
    for (Connection * c = k->connections; c != NULL; c = c->next)
    {
        if(!c->source_io->module->GetName() || !c->target_io->module->GetName())
            socket->Send("<tr><td>XXX</td></tr>X");
        if (c->delay == 0)
            socket->Send("<tr><td><a href=\"#%s\">%s</a>.%s[%d..%d]</td><td>==</td><td><a href=\"#%s\">%s</a>.%s[%d..%d]</td><td>%d</td></tr>\n",
						 c->source_io->module->GetName(), c->source_io->module->GetName(), c->source_io->name, 0, c->source_io->size-1,
						 c->target_io->module->GetName(), c->target_io->module->GetName(), c->target_io->name, 0, c->source_io->size-1,
						 c->delay);
        else if (c->size > 1)
            socket->Send("<tr><td><a href=\"#%s\">%s</a>.%s[%d..%d]</td><td>-></td><td><a href=\"#%s\">%s</a>.%s[%d..%d]</td><td>%d</td></tr>\n",
						 c->source_io->module->GetName(), c->source_io->module->GetName(), c->source_io->name, c->source_offset, c->source_offset+c->size-1,
						 c->target_io->module->GetName(), c->target_io->module->GetName(), c->target_io->name, c->target_offset, c->target_offset+c->size-1,
						 c->delay);
        else
            socket->Send("<tr><td><a href=\"#%s\">%s</a>.%s[%d]</td><td>></td><td><a href=\"#%s\">%s</a>.%s[%d]</td><td>%d</td></tr>\n",
						 c->source_io->module->GetName(), c->source_io->module->GetName(), c->source_io->name, c->source_offset,
						 c->target_io->module->GetName(), c->target_io->module->GetName(), c->target_io->name, c->target_offset,
						 c->delay);
    }
    socket->Send("</table>\n");
    socket->Send("</body>\n</html>");
    
    socket->Close();
}



void
WebUI::RunIKCFile(char * filepath)
{
#ifdef POSIX
	
    // Send reply
	
    socket->Send("HTTP/1.1 200 OK\n");
    socket->Send("Content-Type: text/html\n");
    socket->Send("\n");
    socket->Send("<head><META HTTP-EQUIV='Refresh' CONTENT='1; URL=/view0.svg'></gead><body style='background-color: black; color: white; font-family: sans-serif'>Restarting Ikaros...</body>\n");
    socket->Close();
    
	char * binpath = create_formatted_string("%sBin/IKAROS", k->ikaros_dir);
	printf("%s: %s\n", binpath, filepath);
	delete socket;
    char * s = create_formatted_string("%sExamples/%s", k->ikaros_dir, filepath);
	printf("%s\n", s);
    
    char p[256];
    snprintf(p, 255, "-w%d", port);
	execl(binpath, binpath, p, s, NULL);
#endif
}



void
WebUI::ReadXML(XMLDocument * xmlDoc)
{
    if (xmlDoc == NULL)
    {
        return;
    }
	
    if (xmlDoc->xml == NULL)
    {
        k->Notify(msg_fatal_error, "WebUI: xmlDoc missing\n");
        return;
    }
	
    xml = xmlDoc->xml->GetElement("group");
    current_xml_root = xml;
    
    if (xml == NULL)
    {
        xml = xmlDoc->xml->GetElement("network");
        if (xml != NULL)
            xml = xml->GetElement("views");
    }
	
    // Catch exceptions here!!!
	
    if (xml == NULL)
    {
        k->Notify(msg_warning, "WebUI: No views found\n"); // add exception based error handling later
        return;
    }
}

#endif
