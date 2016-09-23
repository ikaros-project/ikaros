//
//	  WebUI.cc		HTTP support for the IKAROS kernel
//
//    Copyright (C) 2005-2015  Christian Balkenius
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
#endif

#include <string.h>
#include <fcntl.h>
#include <atomic> // required C++11


using namespace ikaros;


static char * //TODO: Move base64_encode to ikaros utils
base64_encode(const unsigned char * data,
              size_t size_in,
              size_t *size_out)
{
    static char encoding_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    static int mod_table[] = {0, 2, 1};
    *size_out = ((size_in - 1) / 3) * 4 + 4;
    
    char *encoded_data = (char *)malloc(*size_out);
    if (encoded_data == NULL) return NULL;
    
    for (int i = 0, j = 0; i < size_in;)
    {
        unsigned int octet_a = i < size_in ? data[i++] : 0;
        unsigned int octet_b = i < size_in ? data[i++] : 0;
        unsigned int octet_c = i < size_in ? data[i++] : 0;
        
        unsigned int triple = (octet_a << 0x10) + (octet_b << 0x08) + octet_c;
        
        encoded_data[j++] = encoding_table[(triple >> 3 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 2 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 1 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 0 * 6) & 0x3F];
    }
    
    for (int i = 0; i < mod_table[size_in % 3]; i++)
        encoded_data[*size_out - 1 - i] = '=';
    
    return encoded_data;
}



// TODO: Consolidate JPEG functions to a single one

static bool
SendColorJPEGbase64(ServerSocket * socket, float * r, float * g, float * b, int sizex, int sizey) // Compress image to jpg and send from memory after base64 encoding
{
    long  size;
    unsigned char * jpeg = (unsigned char *)create_jpeg(size, r, g, b, sizex, sizey, 90);

    size_t input_length = size;
    size_t output_length;
    char * jpeg_base64 = base64_encode(jpeg, input_length, &output_length);
    
    socket->Send("\"data:image/jpeg;base64,");
    bool ok = socket->SendData(jpeg_base64, output_length);
    socket->Send("\"\n");
    
    destroy_jpeg((char *)jpeg);
    free(jpeg_base64);
    return ok;
}


/*
static bool
SendGrayJPEGbase64(ServerSocket * socket, float * m, int sizex, int sizey) // Compress image to jpg and send from memory after base64 encoding
{
    long  size;
    unsigned char * jpeg = (unsigned char *)create_jpeg(size, m, sizex, sizey);
    
    size_t input_length = size;
    size_t output_length;
    char * jpeg_base64 = base64_encode(jpeg, input_length, &output_length);
    
    socket->Send("\"data:image/jpeg;base64,");
    bool ok = socket->SendData(jpeg_base64, output_length);
    socket->Send("\"\n");
    
    destroy_jpeg((char *)jpeg);
    free(jpeg_base64);
    return ok;
}
*/


static bool
SendPseudoColorJPEGbase64(ServerSocket * socket, float * m, int sizex, int sizey, int type)
{
    long  size;
    unsigned char * jpeg = NULL;

    switch(type)
    {
        case data_source_green_image:
            jpeg = (unsigned char *)create_jpeg(size, m, sizex, sizey, LUT_green);
            break;
            
        case data_source_fire_image:
            jpeg = (unsigned char *)create_jpeg(size, m, sizex, sizey, LUT_fire);
            break;
            
        case data_source_spectrum_image:
            jpeg = (unsigned char *)create_jpeg(size, m, sizex, sizey, LUT_spectrum);
            break;

        default:
            return false;
    }
    
    size_t input_length = size;
    size_t output_length;
    char * jpeg_base64 = base64_encode(jpeg, input_length, &output_length);
    
    socket->Send("\"data:image/jpeg;base64,");
    bool ok = socket->SendData(jpeg_base64, output_length);
    socket->Send("\"\n");
    
    destroy_jpeg((char *)jpeg);
    free(jpeg_base64);
    return ok;
}



static bool
SendColorBMPbase64(ServerSocket * socket, float * r, float * g, float * b, int sizex, int sizey) // Compress image to bmp and send from memory after base64 encoding
{
    long  size;
    unsigned char * bmp = (unsigned char *)create_bmp(size, r, g, b, sizex, sizey);
    
    size_t input_length = size;
    size_t output_length;
    char * bmp_base64 = base64_encode(bmp, input_length, &output_length);
    
    socket->Send("\"data:image/bmp;base64,");
    bool ok = socket->SendData(bmp_base64, output_length);
    socket->Send("\"\n");
    
    destroy_bmp((char *)bmp);
    free(bmp_base64);
    return ok;
}


/*
static bool
SendTextData(ServerSocket * socket, float ** matrix, int sizex, int sizey)
{
    Dictionary header;
    header.Set("Content-Type", "text/plain");
    socket->SendHTTPHeader(&header);
    
    for (int j=0; j<sizey; j++)
    {
        for (int i=0; i<sizex; i++)
            if (i==0 && j==0)
                socket->Send("%.4f", matrix[0][0]);	// no space before first
            else
                socket->Send(" %.4f", matrix[j][i]);
        socket->Send("\n");
    }
    
    return true;
}
*/


static bool
SendHTMLData(ServerSocket * socket, const char * title, float ** matrix, int sizex, int sizey)
{
    Dictionary header;
    header.Set("Content-Type", "text/html");
    socket->SendHTTPHeader(&header);
    
    socket->Send("<html>\n");
    socket->Send("<header>\n");
    socket->Send("<title>%s</title>\n", title);
    socket->Send("<style>td {font:10pt Arial,sans-serif;text-align:right}</style>");
    socket->Send("</header>\n");
    socket->Send("<body><div><table>\n");

    for (int j=0; j<sizey; j++)
    {
        socket->Send("<tr>\n");
        for(int i=0; i<sizex; i++)
            socket->Send("<td>%.4f</td>", matrix[j][i]);
        socket->Send("</tr>\n");
    }

    socket->Send("</table></div></body></html>\n");

    return true;
}



static inline float
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
SendJSONMatrixData(ServerSocket * socket, char * module, char * source, float * matrix, int sizex, int sizey) // TODO: use E-format
{
    if (matrix == NULL)
        return false;
	
    int k = 0;

    socket->Send("\t\"%s\":\n\t[\n", source);	// module
    for (int j=0; j<sizey; j++)
    {
        socket->Send("\t\t[%.4E", checknan(matrix[k++]));
        for (int i=1; i<sizex; i++)
            socket->Send(", %.4E", checknan(matrix[k++]));
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
    data2 = NULL;
    data3 = NULL;
    size_x = sx;
    size_y = sy;
}



DataSource::DataSource(const char * s, int t, void * data_ptr, void * data_ptr2, void * data_ptr3, int sx, int sy, DataSource * n)
{
    next = n;
    name = create_string(s);
    type = t;
    data = data_ptr;
    data2 = data_ptr2;
    data3 = data_ptr3;
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
        if (!strcmp(sd->name, s) && sd->type == type)
            return;
	
    source = new DataSource(s, type, value_ptr, sx, sy, source);
}



void
ModuleData::AddSource(const char * s, int type, void * value_ptr, void * value_ptr2, void * value_ptr3, int sx, int sy)
{
    for (DataSource * sd=source; sd != NULL; sd=sd->next)
        if (!strcmp(sd->name, s) && sd->type == type)
            return;
	
    source = new DataSource(s, type, value_ptr, value_ptr2, value_ptr3, sx, sy, source);
}



void
WebUI::AddDataSource(const char * module, const char * source)
{
    Module * m;
    Module_IO * io;
    void * value_ptr;
    int type, size_x, size_y;
    
    XMLElement * group = current_xml_root;
    if(equal_strings(module, "*"))
    {
        module = k->GetXMLAttribute(current_xml_root, "name");
        group = group->GetParentElement();
    }
    
    if (group == NULL)
    {
        k->Notify(msg_warning, "WebUI: Could not find <group> or <modules> element.\n");
        return;
    }
	
    if (k->GetSource(group, m, io, module, source))
    {
        for (ModuleData * md=view_data; md != NULL; md=md->next)
            if (!strcmp(md->name, module))
            {
                if(!io->matrix)
                {
//                    TEST_SIZE();
                    return;
                }
                md->AddSource(source, data_source_matrix, io->matrix[0], io->sizex, io->sizey);
//                TEST_SIZE();
                return;
            }
        
        view_data = new ModuleData(module, m, view_data);
        view_data->AddSource(source, io);
//        TEST_SIZE();
    }
    else if(k->GetBinding(group, m, type, value_ptr, size_x, size_y, module, source))
    {
        for (ModuleData * md=view_data; md != NULL; md=md->next)
            if (!strcmp(md->name, module))
            {
                md->AddSource(source, type, value_ptr, size_x, size_y);
//                TEST_SIZE();
                return;
            }
        
        view_data = new ModuleData(module, m, view_data);
        view_data->AddSource(source, type, value_ptr, size_x, size_y);
//        TEST_SIZE();
    }
    
    else
        k->Notify(msg_warning, "WebUI: Could not add data source %s.%s (not found)\n", module, source);
}



void
WebUI::AddImageDataSource(const char * module, const char * source, const char * type) // FIXME: Clean up ugly code
{
    Module * m;
    
    Module_IO * io;
    Module_IO * io2;
    Module_IO * io3;
    
    XMLElement * group = current_xml_root;
    if(equal_strings(module, "*"))
    {
        module = k->GetXMLAttribute(current_xml_root, "name");
        group = group->GetParentElement();
    }
    
    if (group == NULL)
    {
        k->Notify(msg_warning, "WebUI: Could not find <group> or <modules> element.\n");
        return;
    }
	
    if(equal_strings(type, "rgb"))
    {
        // Get the three sources
        
        char s1[256], s2[256], s3[256];
        
        if (sscanf(source, "%[^+]+%[^+]+%[^+]", s1, s2, s3) != 3)
        {
            k->Notify(msg_warning, "WebUI: RGB Image source needs three source matrices\n");
            return;
        }
        else if(!(k->GetSource(group, m, io, module, s1) &
                  k->GetSource(group, m, io2, module, s2) &
                  k->GetSource(group, m, io3, module, s3)))
        {
            k->Notify(msg_warning, "WebUI: All sources for RGB Image could not be found\n");
            return;
        }
        else // all ok
        {
            for (ModuleData * md=view_data; md != NULL; md=md->next)
                if (!strcmp(md->name, module))
                {
                    if(io->matrix  && io2->matrix && io3->matrix)
                        md->AddSource(source, data_source_rgb_image, io->matrix[0], io2->matrix[0], io3->matrix[0], io->sizex, io->sizey);
                     return;
                }
            
            if(io->matrix && io2->matrix && io3->matrix)
            {
                view_data = new ModuleData(module, m, view_data);
                view_data->AddSource(source, data_source_rgb_image, io->matrix[0], io2->matrix[0], io3->matrix[0], io->sizex, io->sizey);
            }
        }
    }

    else if(equal_strings(type, "bmp"))
    {
        // Get the three sources
        
        char s1[256], s2[256], s3[256];
        
        if (sscanf(source, "%[^+]+%[^+]+%[^+]", s1, s2, s3) != 3)
        {
            k->Notify(msg_warning, "WebUI: Color BMP Image source needs three source matrices\n");
            return;
        }
        else if(!(k->GetSource(group, m, io, module, s1) &
                  k->GetSource(group, m, io2, module, s2) &
                  k->GetSource(group, m, io3, module, s3)))
        {
            k->Notify(msg_warning, "WebUI: All sources for Color BMP Image could not be found\n");
            return;
        }
        else // all ok
        {
            for (ModuleData * md=view_data; md != NULL; md=md->next)
                if (!strcmp(md->name, module))
                {
                    if(io->matrix  && io2->matrix && io3->matrix)
                        md->AddSource(source, data_source_bmp_image, io->matrix[0], io2->matrix[0], io3->matrix[0], io->sizex, io->sizey);
                    return;
                }
            
            if(io->matrix  && io2->matrix && io3->matrix)
            {
                view_data = new ModuleData(module, m, view_data);
                view_data->AddSource(source, data_source_bmp_image, io->matrix[0], io2->matrix[0], io3->matrix[0], io->sizex, io->sizey);
            }
        }
    }
    
    else if(equal_strings(type, "gray") && k->GetSource(group, m, io, module, source))
    {
        for (ModuleData * md=view_data; md != NULL; md=md->next)
            if (!strcmp(md->name, module))
            {
                if(io->matrix)
                    md->AddSource(source, data_source_gray_image, io->matrix[0], NULL, NULL, io->sizex, io->sizey);
                return;
            }
        
        if(io->matrix)
        {
            view_data = new ModuleData(module, m, view_data);
            view_data->AddSource(source, data_source_gray_image, io->matrix[0], NULL, NULL, io->sizex, io->sizey);
        }
    }
    
    else if(equal_strings(type, "fire") && k->GetSource(group, m, io, module, source))
    {
        for (ModuleData * md=view_data; md != NULL; md=md->next)
            if (!strcmp(md->name, module))
            {
                if(io->matrix)
                    md->AddSource(source, data_source_fire_image, io->matrix[0], NULL, NULL, io->sizex, io->sizey);
                return;
            }
        
        if(io->matrix)
        {
            view_data = new ModuleData(module, m, view_data);
            view_data->AddSource(source, data_source_fire_image, io->matrix[0], NULL, NULL, io->sizex, io->sizey);
        }
    }
    
    else if(equal_strings(type, "spectrum") && k->GetSource(group, m, io, module, source))
    {
        for (ModuleData * md=view_data; md != NULL; md=md->next)
            if (!strcmp(md->name, module))
            {
                if(io->matrix)
                    md->AddSource(source, data_source_spectrum_image, io->matrix[0], NULL, NULL, io->sizex, io->sizey);
                return;
            }
        
        if(io->matrix)
        {
            view_data = new ModuleData(module, m, view_data);
            view_data->AddSource(source, data_source_spectrum_image, io->matrix[0], NULL, NULL, io->sizex, io->sizey);
        }
    }
    
    else if(equal_strings(type, "green") && k->GetSource(group, m, io, module, source))
    {
        for (ModuleData * md=view_data; md != NULL; md=md->next)
            if (!strcmp(md->name, module))
            {
                if(io->matrix)
                    md->AddSource(source, data_source_green_image, io->matrix[0], NULL, NULL, io->sizex, io->sizey);
                return;
            }
        
        if(io->matrix)
        {
            view_data = new ModuleData(module, m, view_data);
            view_data->AddSource(source, data_source_green_image, io->matrix[0], NULL, NULL, io->sizex, io->sizey);
        }
    }
    
    else
        k->Notify(msg_warning, "WebUI: Could not add data source %s.%s (not found)\n", module, source);
}




WebUI::WebUI(Kernel * kernel)
{
    k = kernel;
    webui_dir = NULL;
    xml = NULL;
    current_xml_root = NULL;
    current_xml_root_path = create_string("");
    ui_state = ui_state_pause;
    
    ui_data = NULL;
    copying_data = false;
    dont_copy_data = false;
    is_running = false;

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
	
    if (k->options->GetOption('R'))
    {
        port = string_to_int(k->options->GetArgument('R'), PORT);
        k->Notify(msg_verbose, "Setting up WebUI port at %d\n", port);
        if(k->options->GetOption('r'))
        {
            ui_state = ui_state_realtime;
            k->Notify(msg_verbose, "Setting real-time mode.\n");
        }
        else
        {
            ui_state = ui_state_run;
            k->Notify(msg_verbose, "Setting run mode.\n");
        }
        isRunning = true;
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
WebUI::SendView(const char * view)
{
//    while(copying_data) ;

    if (debug_mode)
        printf("Sending HTML View: %s\n", view);
    
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
            if(equal_strings(k->GetXMLAttribute(xml_module, "name"), group_in_path))
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
        const char * n = k->GetXMLAttribute(xml_view, "name");
        if (
            (n != NULL && !strncmp(&view_name[1], n, strlen(view_name)-5)) ||	// test view name
            (!strncmp(view_name, "/view", 5) && v == ix)   // FIXME: test view number 0-9 for backward compatibility
			)
        {
			
            float object_spacing = string_to_float(k->GetXMLAttribute(xml_view, "object_spacing"), 15.0);
            float object_size = string_to_float(k->GetXMLAttribute(xml_view, "object_size"), 140.0);
			
            // Calculate View Size
			
            int view_width = 0;
            int view_height = 0;
			
            for (XMLElement * xml_uiobject = xml_view->GetContentElement("object"); xml_uiobject != NULL; xml_uiobject = xml_uiobject->GetNextElement("object"))
            {
                // char * object_class = xml_uiobject->FindAttribute("class");
                int x = string_to_int(k->GetXMLAttribute(xml_uiobject, "x"), -1);
                int y = string_to_int(k->GetXMLAttribute(xml_uiobject, "y"), -1);
                int width = string_to_int(k->GetXMLAttribute(xml_uiobject, "w"), 1);
                int height = string_to_int(k->GetXMLAttribute(xml_uiobject, "h"), 1);
				
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
            header.Set("Content-Type", "text/html");
            socket->SendHTTPHeader(&header);

//          socket->SendFile("viewer.html"); //FIXME: This should work
            // Include the file "viewer.html"

            char path[1024] = "";
            append_string(path, webui_dir, 1024);
            FILE * f = fopen(append_string(path, "Viewer/viewer.html", 1024), "r");
            if(!f)
            {
                printf("Cannot find \"viewer.html\"; webui_dir=\"%s\"\n", webui_dir);
                socket->Send("</html>\n");
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

            socket->Send("<body>\n");
            socket->Send("<div style='position: absolute; width:%.2fpx; height:%.2fpx' id='frame' >\n", 2*margin_x + view_width_px, margin_y + margin_x + view_height_px);
            socket->Send("</div>\n");
            socket->Send("<script>\n");

             // Send the UIObjects
			
            float ** occ = create_matrix(50, 50); // max elements in view
            for (XMLElement * xml_uiobject = xml_view->GetContentElement("object"); xml_uiobject != NULL; xml_uiobject = xml_uiobject->GetNextElement("object"))
            {
                const char * object_class = k->GetXMLAttribute(xml_uiobject, "class");
                int x = string_to_int(k->GetXMLAttribute(xml_uiobject, "x"), 0);
                int y = string_to_int(k->GetXMLAttribute(xml_uiobject, "y"), 0);
                int width = string_to_int(k->GetXMLAttribute(xml_uiobject, "w"), 1);
                int height = string_to_int(k->GetXMLAttribute(xml_uiobject, "h"), 1);
                
                // Check opacity
                
                bool behind = true;
                for(int j=0; j<height; j++)
                    for(int i=0; i<width; i++)
                        if(occ[y+j][x+i] == 1)
                            behind = false;
                        else
                            occ[y+j][x+i] = 1;
                
                // Trick to avoid namespace clashes in JavaScript; yes it leaks but we don't care
                
                if(equal_strings(object_class, "Image"))
                    object_class = create_string("ImageStream");

                socket->Send("add(new %s({", object_class);
                for (XMLAttribute * p = xml_uiobject->attributes; p != NULL; p = (XMLAttribute *)(p->next))
                    if (!(!strcmp(p->name, "x") || !strcmp(p->name, "y") || !strcmp(p->name, "w") || !strcmp(p->name, "h") || !strcmp(p->name, "class")))
                    {
                        // TODO: Test number - do this properly later
                        unsigned long l = strlen(p->value)-1;
                        if ((('0' <= p->value[0] && p->value[0] <='9') || p->value[0] == '-') && !strstr(p->value, ",") && ('0' <= p->value[l] && p->value[l] <='9') && (!equal_strings(p->name, "title")))
                            socket->Send("%s:%s, ", p->name, p->value);
                    //    else if(equal_strings(p->value, "*"))
                    //        socket->Send("%s:'%s', ", p->name, current_xml_root->GetAttribute("name"));
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
			
            socket->Send("</script>\n");
            socket->Send("</body>\n");
            socket->Send("</html>\n");
            return;
        }
    }
	
    socket->SendFile("404.html");
}



void
WebUI::Run()
{
    if(socket == NULL)
        return;
	
	// Synchronization
	
	if(xml)
	{
		const char * ip = k->GetXMLAttribute(xml, "masterip");
		if(ip)
		{
			Socket s;
			char rr[100];
			int masterport = string_to_int(k->GetXMLAttribute(xml, "masterport"), 9000);
			k->Notify(msg_print, "Waiting for master: %s:%d\n", ip, masterport);
			fflush(stdout);
			if(!s.Get(ip, masterport, "*", rr, 100))
			{
				k->Notify(msg_terminate, "Master not running.\n");
				throw -1; // No master
			}
		}
	}

    chdir(k->ikc_dir); // TODO: Check if already set

    k->timer->Restart();
    tick = 0;

    httpThread = new Thread();
    httpThread->Create(WebUI::StartHTTPThread, this);

    while (!k->Terminate())
    {
        if (!isRunning)
        {
            Timer::Sleep(10); // Wait 10ms to avoid wasting cycles if there are no requests
        }
        
        if (isRunning)
        {
            is_running = true; // Flag that state changes are not allowed
            k->Tick();
            CopyUIData();
            is_running = false;
            tick++; // FIXME: should not be separate from kernel; remove from WebUI class
            
            if (k->tick_length > 0)
            {
                float lag = k->timer->WaitUntil(float(tick*k->tick_length));
                if (lag > 0.1) k->Notify(msg_warning, "Lagging %.2f ms at tick = %ld\n", lag, k->tick);
            }
        }
    }

    httpThread->Join();
//    chdir(k->ikc_dir);
}


/*
void
WebUI::TEST_SIZE()
{
    int size = 0;

    for (ModuleData * md=view_data; md != NULL; md=md->next)
    {
        for (DataSource * sd=md->source; sd != NULL; sd=sd->next)
        {
            switch(sd->type)
            {
                 case data_source_array:
                    size += sd->size_x;
                    break;
                    
                case data_source_matrix:
                case data_source_gray_image:
                case data_source_green_image:
                case data_source_spectrum_image:
                case data_source_fire_image:
                    size += sd->size_x * sd->size_y;
                    break;
            
                case data_source_rgb_image:
                case data_source_bmp_image:
                    size += 3 * sd->size_x * sd->size_y;
                    break;
                    
                case data_source_float:
                case data_source_int:
                case data_source_bool:
                case data_source_list:
                    size++; // int, list, bool, float
                    break;

                default:
                    k->Notify(msg_warning, "WebUI: Unkown data type");
                    break;
            }
        }
    }

//    printf("\tDATA SIZE: %d\n", size);

}
*/


void
WebUI::CopyUIData()
{
    if(!view_data)
        return;
    
    if(dont_copy_data)
        return;

    copying_data = true;

    // Step 1: calculate size
    
    int size = 0;

    for (ModuleData * md=view_data; md != NULL; md=md->next)
    {
        for (DataSource * sd=md->source; sd != NULL; sd=sd->next)
        {
            switch(sd->type)
            {
                 case data_source_array:
                    size += sd->size_x;
                    break;
                    
                case data_source_matrix:
                case data_source_gray_image:
                case data_source_green_image:
                case data_source_spectrum_image:
                case data_source_fire_image:
                    size += sd->size_x * sd->size_y;
                    break;
            
                case data_source_rgb_image:
                case data_source_bmp_image:
                    size += 3 * sd->size_x * sd->size_y;
                    break;
                    
                case data_source_float:
                case data_source_int:
                case data_source_bool:
                case data_source_list:
                    size++; // int, list, bool, float
                    break;

                default:
                    k->Notify(msg_warning, "WebUI: Unkown data type");
                    break;
            }
        }
    }

//    printf("CopyUIData: %ld: DATA SIZE: %d\n", tick, size);

    // Allocate memory
    
    float * local_ui_data = create_array(size);
     
    if(!local_ui_data)
    {
        k->Notify(msg_warning, "WebUI: Cannot allocate memory for data");
        return;
    }

    float * p = local_ui_data;
    
    // Step 2: copy the data

    for (ModuleData * md=view_data; md != NULL; md=md->next)
    {
        for (DataSource * sd=md->source; sd != NULL; sd=sd->next)
        {
            switch(sd->type)
            {
                case data_source_int:
                case data_source_list:
                    *(p++) = *(int *)(sd->data);
                    break;
                    
                case data_source_bool:
                    *(p++) = (*(bool *)(sd->data) ? 1 : 0);
                    break;
                    
                case data_source_float:
                    *(p++) = *(float *)(sd->data);
                    break;
                    
                case data_source_array:
                    copy_array(p, (float *)(sd->data), sd->size_x);
                    p += sd->size_x;
                    break;
                    
                case data_source_matrix:
                case data_source_gray_image:
                case data_source_green_image:
                case data_source_spectrum_image:
                case data_source_fire_image:
                    copy_array(p, *(float **)(sd->data), sd->size_x*sd->size_y);
                    p += sd->size_x*sd->size_y;
                    break;
                    
                case data_source_rgb_image:
                case data_source_bmp_image:
                    copy_array(p, *(float **)(sd->data), sd->size_x*sd->size_y);
                    p += sd->size_x*sd->size_y;
                    copy_array(p, *(float **)(sd->data2), sd->size_x*sd->size_y);
                    p += sd->size_x*sd->size_y;
                    copy_array(p, *(float **)(sd->data3), sd->size_x*sd->size_y);
                    p += sd->size_x*sd->size_y;
                    break;
            }
        }
    }
    
    // Step 3: store in ui_data

    float * old_ui_data = atomic_exchange(&ui_data, local_ui_data);

    if(old_ui_data)
        destroy_array(old_ui_data);
    
    copying_data = false;
}



void
WebUI::SendUIData() // TODO: allow number of decimals to be changed - or use E-format
{
    // Grab ui data
        
    float * p = atomic_exchange(&ui_data, (float *)(NULL));
    float * q = p;

//    printf("MEM: %lx\n", (unsigned long)p);

    long int s = 0;

    // Send
    
    Dictionary header;
	
    header.Set("Content-Type", "text/json");
    header.Set("Cache-Control", "no-cache");
    header.Set("Cache-Control", "no-store");
    header.Set("Pragma", "no-cache");
    header.Set("Expires", "0");

    socket->SendHTTPHeader(&header);

    socket->Send("{\n");
    socket->Send("state: %d,\n", ui_state);  // ui_state; ui_state_run
    socket->Send("iteration: %d", k->GetTick());
	
    if(!p)
    {
        socket->Send("\n}\n");
        return;
    }

    if (view_data != NULL)
        socket->Send(",\n");
    else
        socket->Send("\n");
	
    for (ModuleData * md=view_data; md != NULL; md=md->next)
    {
        if(equal_strings(md->name, k->GetXMLAttribute(current_xml_root, "name")))
            socket->Send("\"*\":\n{\n");
        else
            socket->Send("\"%s\":\n{\n", md->name);
        
        for (DataSource * sd=md->source; sd != NULL; sd=sd->next)
        {
            switch(sd->type)
            {
                case data_source_int:
                case data_source_list:
                case data_source_bool:
                case data_source_float:
                    socket->Send("\t\"%s\": [[%f]]", sd->name, *p++);
                     break;

                 case data_source_array:
                    SendJSONArrayData(socket, md->name, sd->name, p, sd->size_x);
                    p += sd->size_x;
                    break;
                    
                case data_source_matrix:
                    SendJSONMatrixData(socket, md->name, sd->name, p, sd->size_x, sd->size_y);
                    p += sd->size_x * sd->size_y;
                    break;
                     
                case data_source_rgb_image:
                    socket->Send("\t\"%s:rgb\": ", sd->name);
                    s = sd->size_x * sd->size_y;
                    SendColorJPEGbase64(socket, p, &p[s], &p[2*s], sd->size_x, sd->size_y);
                    p += 3 * s;
                    break;
                    
                case data_source_bmp_image:
                    socket->Send("\t\"%s:bmp\": ", sd->name);
                    s = sd->size_x * sd->size_y;
                    SendColorBMPbase64(socket, p, &p[s], &p[2*s], sd->size_x, sd->size_y);
                    p += 3 * s;
                    break;
                    
                case data_source_gray_image:
                     socket->Send("\t\"%s:gray\": ", sd->name);
                    SendColorJPEGbase64(socket, p, p, p, sd->size_x, sd->size_y);
                    p += sd->size_x * sd->size_y;
                    break;
                    
                case data_source_green_image:
                    socket->Send("\t\"%s:green\": ", sd->name);
                    SendPseudoColorJPEGbase64(socket, p, sd->size_x, sd->size_y, sd->type);
                    p += sd->size_x * sd->size_y;
                    break;

                case data_source_spectrum_image:
                    socket->Send("\t\"%s:spectrum\": ", sd->name);
                    SendPseudoColorJPEGbase64(socket, p, sd->size_x, sd->size_y, sd->type);
                    p += sd->size_x * sd->size_y;
                    break;

                case data_source_fire_image:
                    socket->Send("\t\"%s:fire\": ", sd->name);
                    SendPseudoColorJPEGbase64(socket, p, sd->size_x, sd->size_y, sd->type);
                    p += sd->size_x * sd->size_y;
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
    
    destroy_array(q);
}



void
WebUI::Pause()
{
    isRunning = false;
    while(is_running)
        ;
    dont_copy_data = false;
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
        Pause();
        ui_state = ui_state_stop;
        CopyUIData();
        SendUIData();
        k->Notify(msg_terminate, "Sent by WebUI.\n");
    }
	
    else if (!strcmp(uri, "/step"))
    {
        Pause();
        ui_state = ui_state_pause;
        
        for(int i=0; i<iterations_per_runstep; i++)
            k->Tick();
        
        CopyUIData();
        SendUIData();
    }
	
    else if (!strcmp(uri, "/runstep"))
    {
        Pause();
        ui_state = ui_state_run;
        
        for(int i=0; i<iterations_per_runstep; i++)
            k->Tick();
        
        CopyUIData();
        SendUIData();
    }

    else if (!strcmp(uri, "/update"))
    {
        SendUIData();   // Send last stored ui data package
    }
	
    else if (!strcmp(uri, "/pause"))
    {
        Pause();
        ui_state = ui_state_pause;
        CopyUIData();
        SendUIData();
    }

    else if (!strcmp(uri, "/realtime"))
    {
        ui_state = ui_state_realtime;

		Dictionary rtheader;
		rtheader.Set("Content-Type", "text/plain");
		socket->SendHTTPHeader(&rtheader);
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
        strsep(&p, "/");    // group_in_path =
        strsep(&p, "/");
        while((group_in_path = strsep(&p, "/")))
        {
            group_xml = group_xml->GetElement("group");
            for (XMLElement * xml_module = group_xml->GetContentElement("group"); xml_module != NULL; xml_module = xml_module->GetNextElement("group"))
                if(equal_strings(k->GetXMLAttribute(xml_module, "name"), group_in_path))
                {
                    group_xml = xml_module;
                    break;
                }
        }
        current_xml_root = group_xml;
        destroy_string(p);
		Dictionary rtheader;
		rtheader.Set("Content-Type", "text/plain");
		socket->SendHTTPHeader(&rtheader);
        socket->Send("OK\n");
    }

    else if (strstart(uri, "/usesBase64"))
    {
        char * module = new char [256];
        char * output = new char [256];
        char * type = new char [256];
        int c = sscanf(uri, "/usesBase64/%[^/]/%[^/]/%[^/]", module, output, type);
        if (c == 3)
        {
            AddImageDataSource(module, output, type);
            float * old_ui_data = atomic_exchange(&ui_data, (float *)(NULL));  //Invalidate old buffer
            if(old_ui_data)
                destroy_array(old_ui_data);
        }
		
		Dictionary header;
		header.Set("Content-Type", "text/plain");
		socket->SendHTTPHeader(&header);
        socket->Send("OK\n");
        
		delete[] module;
        delete[] output;
        delete[] type;
    }

    else if (strstart(uri, "/uses"))
    {
        char * module = new char [256];
        char * source = new char [256];
        int c = sscanf(uri, "/uses/%[^/]/%[^/]", module, source);
        if (c == 2)
        {
            AddDataSource(module, source);
            float * old_ui_data = atomic_exchange(&ui_data, (float *)(NULL));  //Invalidate old buffer
            if(old_ui_data)
                destroy_array(old_ui_data);
        }

		Dictionary header;
		header.Set("Content-Type", "text/plain");
		socket->SendHTTPHeader(&header);
        socket->Send("OK\n");
        
		delete [] module;
        delete [] source;
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
            XMLElement * group = current_xml_root;
            if(equal_strings(module_name, "*"))
            {
                strcpy(module_name, k->GetXMLAttribute(current_xml_root, "name"));
                group = group->GetParentElement();
            }

            k->SetParameter(group, module_name, name, x, y, value);
        }

		Dictionary header;
		header.Set("Content-Type", "text/plain");
		header.Set("Cache-Control", "no-cache");
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
    
    else if(strstart(uri, "/view") && strend(uri, ".html"))
    {
        SendView(uri);
	}
    
    else if (strstart(uri, "/module/"))
    {
        char module[256], output[256], type[256];
        int c = sscanf(uri, "/module/%[^/]/%[^/]/%[^/]", module, output, type);

        if(c != 3)
        {
            socket->Send( "Incorrect data: %s/%s/%s\n", module, output, type);
            destroy_string(uri);
            return;
        }

        Module * m = k->GetModuleFromFullName(module);        
        if(m)
        {
            int sx = m->GetOutputSizeX(output);
            int sy = m->GetOutputSizeY(output);
            char * s = create_formatted_string("%s.%s [%dx%d]", module, output, sx, sy);
            if (!SendHTMLData(socket, s, m->GetOutputMatrix(output), sx, sy))
                k->Notify(msg_warning, "Could not send: data.txt\n");
            destroy_string(uri);
            return;
        }
        
        else
        {
            socket->Send( "The output \"%s.%s\" does not exist, or\n", module, output);
            socket->Send( "\"%s\" may be an unkown data type.\n", type);
            destroy_string(uri);
            return;
        }
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
    
    else if (!strcmp(uri, "/Buttons/realtime.png"))
    {
        if(k->GetTickLength() > 0)
            socket->SendFile("Buttons/realtime.png", webui_dir);
        else
            socket->SendFile("Buttons/ff.png", webui_dir);
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
			
            socket->SendFile("404.html", webui_dir);
        }
    }
	
    else 
    {
		Dictionary header;
		header.Set("Content-Type", "text/plain");
		socket->SendHTTPHeader(&header);
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
            {
                while(copying_data)  // wait for copy operation to complete
                //    printf("waiting\n")
                    ;
                dont_copy_data = true;
                HandleHTTPRequest();
                dont_copy_data = false;
            }
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
    
    remove("temp.xml");
//    socket->Close();
}



void
WebUI::SendModule(Module * m) // TODO: Use stylesheet for everything
{
        socket->Send("<table>\n");
		
//        m->GetFullName();
		
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
                    socket->Send("<br /><div class='hidden' style='width: 400px; padding-top: 10px; color: #BBB'>%s <a href=\"http://www.ikaros-project.org/module/%s\" target='_window'><img style='vertical-align: middle' src='Icons/link.png'/></a></div>\n", description, m->class_name);
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
                const char * name = parameter->GetAttribute("name");    // No inheritance here
                const char * type = parameter->GetAttribute("type");    // No inheritance here
                const char * description = parameter->GetAttribute("description");    // No inheritance here
				
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
				socket->Send("<td><a onclick=\"var w = window.open('/module/%s/%s/data.txt','','status=no,width=500, height=700');\">%-10s</a></td>\n", m->GetFullName(), i->name, i->name);
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
    socket->Send("<link rel='stylesheet' type='text/css' href='/Inspector/inspector.css' />\n");
    socket->Send("<script src='/Inspector/inspector.js'></script>");
    socket->Send("</head>\n");
    socket->Send("<body>\n");
    
    if(!k->modules)
    {
        socket->Send("<p>No modules</p>\n");
    }

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
