//
//	  WebUI.cc		HTTP support for the IKAROS kernel
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

#include "WebUI.h"

#ifdef USE_SOCKET
#include "Kernel/IKAROS_ColorTables.h"

#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <atomic>
#include <string>

using namespace ikaros;



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


static bool
SendPseudoColorJPEGbase64(ServerSocket * socket, float * m, int sizex, int sizey, int type)
{
    long  size;
    unsigned char * jpeg = NULL;

    switch(type)
    {
        case data_source_red_image:
            jpeg = (unsigned char *)create_jpeg(size, m, sizex, sizey, LUT_red);
            break;
            
         case data_source_green_image:
            jpeg = (unsigned char *)create_jpeg(size, m, sizex, sizey, LUT_green);
            break;
            
        case data_source_blue_image:
            jpeg = (unsigned char *)create_jpeg(size, m, sizex, sizey, LUT_blue);
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

    socket->Send("\t\t\"%s\":\n\t\t[\n", source);	// module
    for (int j=0; j<sizey; j++)
    {
        socket->Send("\t\t\t[%.4E", checknan(matrix[k++]));
        for (int i=1; i<sizex; i++)
            socket->Send(", %.4E", checknan(matrix[k++]));
        if (j<sizey-1)
            socket->Send("\t],\n");
        else
            socket->Send("\t]\n\t\t]");
    }
	
    return true;
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
	
    if (k->GetSource(group, m, io, module, source)) // FIXME: Allow inputs here as well
    {
        for (ModuleData * md=view_data; md != NULL; md=md->next)
            if (!strcmp(md->name, module))
            {
                if(!io->matrix)
                    return;

                md->AddSource(source, data_source_matrix, io->matrix[0], io->sizex, io->sizey);
                return;
            }
        
        view_data = new ModuleData(module, m, view_data);
        view_data->AddSource(source, io);
    }
    else if(k->GetBinding(group, m, type, value_ptr, size_x, size_y, module, source))
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
    
    else if(equal_strings(type, "red") && k->GetSource(group, m, io, module, source))
    {
        for (ModuleData * md=view_data; md != NULL; md=md->next)
            if (!strcmp(md->name, module))
            {
                if(io->matrix)
                    md->AddSource(source, data_source_red_image, io->matrix[0], NULL, NULL, io->sizex, io->sizey);
                return;
            }
        
        if(io->matrix)
        {
            view_data = new ModuleData(module, m, view_data);
            view_data->AddSource(source, data_source_red_image, io->matrix[0], NULL, NULL, io->sizex, io->sizey);
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
    
    else if(equal_strings(type, "blue") && k->GetSource(group, m, io, module, source))
    {
        for (ModuleData * md=view_data; md != NULL; md=md->next)
            if (!strcmp(md->name, module))
            {
                if(io->matrix)
                    md->AddSource(source, data_source_blue_image, io->matrix[0], NULL, NULL, io->sizex, io->sizey);
                return;
            }
        
        if(io->matrix)
        {
            view_data = new ModuleData(module, m, view_data);
            view_data->AddSource(source, data_source_blue_image, io->matrix[0], NULL, NULL, io->sizex, io->sizey);
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
        k->Notify(msg_debug, "Setting up WebUI port at %d\n", port);
    }
	
    if (k->options->GetOption('R'))
    {
        port = string_to_int(k->options->GetArgument('R'), PORT);
        k->Notify(msg_debug, "Setting up WebUI port at %d\n", port);
        if(k->options->GetOption('r'))
        {
            ui_state = ui_state_realtime;
            k->Notify(msg_debug, "Setting real-time mode.\n");
        }
        else
        {
            ui_state = ui_state_run;
            k->Notify(msg_debug, "Setting run mode.\n");
        }
        isRunning = true;
    }
	
    if (k->options->GetOption('W'))
    {
        port = string_to_int(k->options->GetArgument('W'), PORT);
        k->Notify(msg_debug, "Setting up WebUI port at %d in debug mode\n", port);
        debug_mode = true;
    }
	
    k->Notify(msg_print, "Starting Ikaros WebUI server.\n");
    k->Notify(msg_print, "Connect from a browser on this computer with the URL \"http://127.0.0.1:%d/\".\n", port);
    k->Notify(msg_print, "Use the URL \"http://<servername>:%d/\" from other computers.\n", port);
	
    webui_dir = create_formatted_string("%s%s", k->ikaros_dir, WEBUIPATH);

    ReadXML(k->xmlDoc);
	
    socket =  new ServerSocket(port);
}



WebUI::~WebUI()
{
    delete socket;
    delete view_data;
    
    destroy_string(webui_dir);
}



void
WebUI::Run()
{
    isRunning = true;   // FIXME: TEMPORARY START UP
    first_request = true;
    
    if(socket == NULL)
        return;
	
    chdir(k->ikc_dir); // TODO: Check if already set

    k->timer->Restart();
    tick = 0;

    httpThread = new std::thread(WebUI::StartHTTPThread, this);
//    httpThread->Create(WebUI::StartHTTPThread, this);

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
                k->lag = k->timer->WaitUntil(float(tick*k->tick_length));
                if (k->lag > 0.1) k->Notify(msg_warning, "Lagging %.2f ms at tick = %ld\n", k->lag, k->tick);
            }
        }
    }

//    if(k->max_ticks != -1)
//        httpThread->Kill(); // FIXME: This is a really ugly solution; but this code will soon be replaced anyway
    httpThread->join();
    delete httpThread;
//    chdir(k->ikc_dir);
}



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
                case data_source_red_image:
                case data_source_green_image:
                case data_source_blue_image:
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

                case data_source_string:
                {
                    const char * cs = ((std::string *)(sd->data))->c_str();
                    size += strlen(cs)/sizeof(float)+1;
                    break;
                }
                
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

                case data_source_string:
                {
                    char *  cp = (char *)p;
                    const char * cs = ((std::string *)(sd->data))->c_str();
                    strcpy(cp, cs);
                    p += strlen(cs)/sizeof(float)+1;
                    break;
                }

                case data_source_array:
                    copy_array(p, (float *)(sd->data), sd->size_x);
                    p += sd->size_x;
                    break;

                case data_source_matrix:
//                case data_source_gray_image:
                case data_source_red_image:
                case data_source_green_image:
                case data_source_blue_image:
                case data_source_spectrum_image:
                case data_source_fire_image:
                    copy_array(p, *(float **)(sd->data), sd->size_x*sd->size_y);
                    p += sd->size_x*sd->size_y;
                    break;

                case data_source_gray_image:
                    {
                        float * temp = copy_array(create_array(sd->size_x*sd->size_y), *(float **)(sd->data), sd->size_x*sd->size_y);
                        float mn, mx;
                        minmax(mn, mx, temp, sd->size_x*sd->size_y);
                        if(mx-mn > 0)
                        {
                            subtract(temp, mn, sd->size_x*sd->size_y);
                            multiply(temp, 1/(mx-mn), sd->size_x*sd->size_y);
                        }
                        copy_array(p, temp, sd->size_x*sd->size_y);
                        p += sd->size_x*sd->size_y;
                        destroy_array(temp);
                    }
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
	
    header.Set("Session-Id", std::to_string(k->session_id).c_str()); // FIXME: GetValue("session_id")
    header.Set("Content-Type", "application/json");
    header.Set("Cache-Control", "no-cache");
    header.Set("Cache-Control", "no-store");
    header.Set("Pragma", "no-cache");
    header.Set("Expires", "0");
    
    socket->SendHTTPHeader(&header);

    socket->Send("{\n");
    socket->Send("\t\"state\": %d,\n", ui_state);  // ui_state; ui_state_run
    socket->Send("\t\"iteration\": %d,\n", k->GetTick());
    socket->Send("\t\"progress\": %f,\n", (k->max_ticks > 0 ? float(k->tick)/float(k->max_ticks) : 0));
    
    // Timing information
    
    float total_time = k->timer->GetTime()/1000.0; // in seconds
    
    socket->Send("\t\"timestamp\": %ld,\n", Timer::GetRealTime());
    socket->Send("\t\"total_time\": %.2f,\n", total_time);
    socket->Send("\t\"ticks_per_s\": %.2f,\n", float(k->tick)/total_time);
    socket->Send("\t\"timebase\": %d,\n", k->tick_length);
    socket->Send("\t\"timebase_actual\": %.0f,\n", 1000*float(total_time)/float(k->tick));
    socket->Send("\t\"lag\": %.0f,\n", k->lag);
    socket->Send("\t\"cpu_cores\": %d", k->cpu_cores); // FIXME: send with initial package instead

    if(!p)
    {
        socket->Send("\n}\n");
		if (debug_mode)
			printf("SENT EMPTY PACKAGE\n");

        return;
    }

    if (view_data != NULL)
        socket->Send(",\n");
    else
        socket->Send("\n");
	
    for (ModuleData * md=view_data; md != NULL; md=md->next)
    {
        if(equal_strings(md->name, k->GetXMLAttribute(current_xml_root, "name")))
            socket->Send("\t\"*\":\n\t{\n");
        else
            socket->Send("\t\"%s\":\n\t{\n", md->name);
        
        for (DataSource * sd=md->source; sd != NULL; sd=sd->next)
        {
            switch(sd->type)
            {
                case data_source_int:
                case data_source_list:
                case data_source_bool:
                case data_source_float:
                    socket->Send("\t\t\"%s\": [[%f]]", sd->name, *p++);
                    break;

                case data_source_string:
                    socket->Send("\t\t\"%s\": \"%s\"", sd->name, (char *)p);
                    p += strlen((char *)p)/sizeof(float)+1;
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
                    socket->Send("\t\t\"%s:rgb\": ", sd->name);
                    s = sd->size_x * sd->size_y;
                    SendColorJPEGbase64(socket, p, &p[s], &p[2*s], sd->size_x, sd->size_y);
                    p += 3 * s;
                    break;
                    
                case data_source_bmp_image:
                    socket->Send("\t\t\"%s:bmp\": ", sd->name);
                    s = sd->size_x * sd->size_y;
                    SendColorBMPbase64(socket, p, &p[s], &p[2*s], sd->size_x, sd->size_y);
                    p += 3 * s;
                    break;
                    
                case data_source_gray_image:
                     socket->Send("\t\t\"%s:gray\": ", sd->name);
                    SendColorJPEGbase64(socket, p, p, p, sd->size_x, sd->size_y);
                    p += sd->size_x * sd->size_y;
                    break;
                    
                case data_source_red_image:
                    socket->Send("\t\t\"%s:red\": ", sd->name);
                    SendPseudoColorJPEGbase64(socket, p, sd->size_x, sd->size_y, sd->type);
                    p += sd->size_x * sd->size_y;
                    break;

                case data_source_green_image:
                    socket->Send("\t\t\"%s:green\": ", sd->name);
                    SendPseudoColorJPEGbase64(socket, p, sd->size_x, sd->size_y, sd->type);
                    p += sd->size_x * sd->size_y;
                    break;

                case data_source_blue_image:
                    socket->Send("\t\t\"%s:blue\": ", sd->name);
                    SendPseudoColorJPEGbase64(socket, p, sd->size_x, sd->size_y, sd->type);
                    p += sd->size_x * sd->size_y;
                    break;

                case data_source_spectrum_image:
                    socket->Send("\t\t\"%s:spectrum\": ", sd->name);
                    SendPseudoColorJPEGbase64(socket, p, sd->size_x, sd->size_y, sd->type);
                    p += sd->size_x * sd->size_y;
                    break;

                case data_source_fire_image:
                    socket->Send("\t\t\"%s:fire\": ", sd->name);
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
            socket->Send("\t},\n");
        else
            socket->Send("\t}\n");
    }

    socket->Send("}\n");
    
    destroy_array(q);
    
//    printf("SENT DATA PACKAGE\n");
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

    std::string s = socket->header.Get("URI");
    
    // Copy URI and remove index
    
    char * uri_p = create_string(socket->header.Get("URI"));
    char * uri = strsep(&uri_p, "?");
    char * args = uri_p;

    if(!strcmp(uri, "/update.json"))
    {
        if(!args || first_request) // not a data request - send view data
        {
            first_request = false;
            std::string s = k->JSONString();
            Dictionary rtheader;
            rtheader.Set("Session-Id", std::to_string(k->session_id).c_str());
            rtheader.Set("Content-Type", "application/json");
            rtheader.Set("Content-Length", int(s.size()));
            socket->SendHTTPHeader(&rtheader);
            socket->SendData(s.c_str(), int(s.size()));
        }
        else // possibly a data request - send requested data - very temporary version without thread or real-time support
        {
            //C++17 [[maybe_unused]] char * var =
            strsep(&args, "=");
            
            // Build data package

            char * root = strsep(&args, "#");
            //C++17 [[maybe_unused]] char * view_name =
            strsep(&args, "#");
            
            // set root (should be a separate function) // FIXME: include full name in all names to allow multiple clients
            
            if(current_xml_root_path)
                destroy_string(current_xml_root_path);
            current_xml_root_path = create_string(root); // create_string(&uri[8]);
            char * p = create_string(root);  // was uri
            char * group_in_path;
            XMLElement * group_xml = xml;
 //           strsep(&p, "/");    // group_in_path =
 //           strsep(&p, "/");
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

            while(args)
            {
                char * ms = strsep(&args, "#");
                char * module = strsep(&ms, ".");
                char * source = strsep(&ms, ":");
                char * format = ms;

                if(format)
                    AddImageDataSource(module, source, format);
                else
                    AddDataSource(module, source);
            }
            
//            while(dont_copy_data) // Wait for data to become available
//                printf("waiting\n");

            CopyUIData();
            SendUIData();
            return;
        }
    }
    else if (!strcmp(uri, "/stop"))
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
        k->Tick();
        CopyUIData();
        SendUIData();
    }
    else if (!strcmp(uri, "/play"))
    {
        Pause();
        ui_state = ui_state_run;
        
        for(int i=0; i<iterations_per_runstep; i++)
            k->Tick();
        
        CopyUIData();
        SendUIData();
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
    else if (strstart(uri, "/command/"))
    {
        char module_name[255];
        int x, y;
        char command[255];
        char value[1024]; // FIXME: no range chacks
        int c = sscanf(uri, "/command/%[^/]/%[^/]/%d/%d/%[^/]", module_name, command, &x, &y, value);
        if(c == 5)
        {
            XMLElement * group = current_xml_root;
            if(equal_strings(module_name, "*"))
            {
                strcpy(module_name, k->GetXMLAttribute(current_xml_root, "name"));
                group = group->GetParentElement();
            }

            k->SendCommand(group, module_name, command, x, y, value);
        }

        Dictionary header;
        header.Set("Content-Type", "text/plain");
        header.Set("Cache-Control", "no-cache");
        header.Set("Cache-Control", "no-store");
        header.Set("Pragma", "no-cache");
        socket->SendHTTPHeader(&header);
        socket->Send("OK\n");
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
    }
    else if (!strcmp(uri, "/getlog"))
    {
        if (k->logfile)
            socket->SendFile("logfile", webui_dir);
        else
            socket->Send("ERROR - No logfile found\n");
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
			 strend(uri, ".stl") ||
			 strend(uri, ".gltf") ||
			 strend(uri, ".glb") ||
			 strend(uri, ".ico"))
    {
        if(!socket->SendFile(&uri[1], k->ikc_dir))  // Check IKC-directory first to allow files to be overriden
        if(!socket->SendFile(&uri[1], webui_dir))   // Now look in WebUI directory
        {
			if (strend(uri, ".gltf") || strend(uri, ".glb"))
			{
				socket->SendFile("/Models/glTF/Error.gltf", webui_dir);   // Send error model
			}
			else
			{
				// Send 404 if not file found
				socket->SendFile("404.html", webui_dir);
			}
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
//        printf("*\n");
        if (socket->GetRequest(true))
        {
            if (equal_strings(socket->header.Get("Method"), "GET"))
            {
                while(copying_data)  // wait for copy operation to complete
                //    printf("waiting\n")
                    ;
//                dont_copy_data = true;
                HandleHTTPRequest();
//                dont_copy_data = false;
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



void
WebUI::ReadXML(XMLDocument * xmlDoc) // TODO: should be integrated into kernel tree
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
