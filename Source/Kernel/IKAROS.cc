//
//	  IKAROS.cc		Kernel code for the IKAROS project
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
//	Created July 13, 2001
//
// Before 2.0: 3200 lines

#include "IKAROS.h"

#include <stdlib.h>
#include <string>
#include <stdio.h>
#include <stdarg.h>
#include <limits.h>
#include <fcntl.h>
#include <ctype.h>
#include <ctime>
#include <iostream>
#include <unistd.h>
#include <exception> // for std::bad_alloc
#include <new>

// Kernel 2.0

#include <string>
#include <unordered_map>
#include <vector>
#include <regex>
#include <thread>


using namespace ikaros;

bool        global_fatal_error = false;	// Must be global because it is used before the kernel is created
bool        global_terminate = false;	// Used to flag that CTRL-C has been received
int         global_error_count = 0;
int         global_warning_count = 0;

static std::string empty_string = "";

//#define USE_MALLOC_DEBUG

#ifndef USE_MALLOC_DEBUG

void* operator new (std::size_t size) throw (std::bad_alloc)
{
    void *p=calloc(size, sizeof(char)); 
    if (p==0) // did calloc succeed?
        throw std::bad_alloc();
    return p;
}

void operator delete (void *p) throw()
{
    free(p); 
}

#endif

//
// USE_MALLOC_DEBUG checks all memory allocations (Currently OS X only).
//

#ifdef USE_MALLOC_DEBUG

#include <malloc/malloc.h>

const int     mem_max_blocks = 100000;    // Ikaros will exit if storage is exhausted
unsigned long mem_allocated = 0;
int           mem_block_allocated_count = 0;
int           mem_block_deleted_count = 0;
void *        mem_block[mem_max_blocks];
size_t        mem_size[mem_max_blocks];
bool          mem_block_deleted[mem_max_blocks];

void *
operator new (std::size_t size) throw (std::bad_alloc)
{
    void *p=calloc(size, sizeof(char)); 
    if (p==0) // did calloc succeed?
        throw std::bad_alloc();
    
    mem_block[mem_block_allocated_count] = p;
    mem_size[mem_block_allocated_count] = size;
    mem_block_deleted[mem_block_allocated_count] = false;
    mem_block_allocated_count++;
    
    if(mem_block_allocated_count > mem_max_blocks)
    {
        printf("OUT OF MEMORY\n");
        exit(1);
    }
    
    return p;
}

void *
operator new [] (std::size_t size) throw (std::bad_alloc)
{
    return operator new(size);
}

void
operator delete (void *p) throw()
{
    if(p == NULL) // this is ok
        return;
    
    // Look for block (backwards to allow several allocation of the same memory)
    
    for(int i=mem_block_allocated_count; i>=0; i--)
        if(mem_block[i] == p)
        {
            if(mem_block_deleted[i])
            {
                printf("Attempting to delete already deleted memory [%d]: %p\n", i, p);
                return;
            }
            
            else
            {
                mem_block[i] == NULL;
                mem_block_deleted[i] = true;
                mem_block_deleted_count++;
                mem_allocated -= malloc_size(p);
                
                free(p);
                return;
            }
        }
    
    printf("Attempting to delete memory not allocated with new: %p\n", p);
}

void
operator delete [] (void *p) throw()
{
    operator delete (p);
}

void
dump_memory()
{
    printf("Allocated Memory\n");
    printf("=======================\n");
    int cnt = 0;
    for(int i=0; i<mem_block_allocated_count; i++)
        if(!mem_block_deleted[i])
        {
            printf("%4d: %p\t[%lu->%lu]\t", i, mem_block[i], mem_size[i], malloc_size(mem_block[i]));
            for(unsigned int j=0; j<mem_size[i]; j++)
            {
                char * p = (char *)(mem_block[i]);
                char c = p[j];
                if(' ' <= c && c < 'z')
                    printf("%c", c);
                else
                    printf("#");
            }
            printf("\n");
            cnt++;
        }
    printf("No of blocks: %d\n", cnt);
}

#endif



//
// Group (2.0)
//

class Element
{
public:
    GroupElement * parent;
    std::unordered_map<std::string, std::string> attributes;

    Element(XMLElement * xml_node=NULL)
    {
        if(!xml_node)
            return;
        
        for(XMLAttribute * attr=xml_node->attributes; attr!=NULL; attr = (XMLAttribute *)attr->next)
            attributes.insert({ attr->name, attr->value });
    }
    
    std::string GetAttribute(std::string a) // get attribute verbatim
    {
        if(attributes.count(a))
            return attributes[a];
        else
            return "";
    };

    const std::string & GetValue(std::string a) // FIXME: parameter renaming and inheritance is missing - partially
    {
        if(a.empty())
            return empty_string;
        else if(a[0] == '@')
            return GetValue(GetValue(a.substr(1)));
        else if(attributes.count(a))
            return attributes[a];
        else
            return empty_string;
    };

    const std::string & operator[](std::string a) // get attribute verbatim
    {
        return GetValue(a);
    };

    void PrintAttributes(int d=0)
    {
        for(auto a : attributes)
            printf((std::string(d+1, '\t')+"\t%s = \"%s\"\n").c_str(), a.first.c_str(), a.second.c_str());
    }

    std::string JSONAttributeString(int d=0)
    {
        std::string b;
        std::string s;
        for(auto a : attributes)
        {
            std::string value = std::regex_replace(a.second, std::regex("\\s+"), " "); // JSON does not allow line breaks in attribute values
            s += b + std::string(d, '\t') + "\"" + a.first + "\": \"" + value + "\"";
            b = ",\n";
        }
        return s;
    }
};



class ParameterElement: public Element
{
public:
    ParameterElement(XMLElement * xml_node=NULL) : Element(xml_node) {};
    
    void Print(int d=0)
    {
        printf("%s\n", (std::string(d, '\t')+"\tPARAMETER: "+GetAttribute("name")).c_str());
        PrintAttributes(d);
    };
    
    std::string JSONString(int d=0)
    {
        std::string s = std::string(d, '\t')+"{\n";
        s += JSONAttributeString(d+1);
        s += "\n" + std::string(d, '\t')+"}";
        return s;
    };
};

class InputElement: public Element
{
public:
    InputElement(XMLElement * xml_node=NULL) : Element(xml_node) {};

    void Print(int d=0)
    {
        printf("%s\n", (std::string(d, '\t')+"\tINPUT: "+GetAttribute("name")).c_str());
        PrintAttributes(d);
    };
    
    std::string JSONString(int d=0)
    {
        std::string s = std::string(d, '\t')+"{\n";
        s += JSONAttributeString(d+1);
        s += "\n" + std::string(d, '\t')+"}";
        return s;
    };
};

class OutputElement: public Element
{
public:
    OutputElement(XMLElement * xml_node=NULL) : Element(xml_node) {};

    void Print(int d=0)
    {
        printf("%s\n", (std::string(d, '\t')+"\tOUTPUT: "+GetAttribute("name")).c_str());
        PrintAttributes(d);
    };
    
    std::string JSONString(int d=0)
    {
        std::string s = std::string(d, '\t')+"{\n";
        s += JSONAttributeString(d+1);
        s += "\n" + std::string(d, '\t')+"}";
        return s;
    };
};

class ConnectionElement: public Element
{
public:
    ConnectionElement(XMLElement * xml_node=NULL) : Element(xml_node) {};
    
    void Print(int d=0)
    {
        printf("%s\n", (std::string(d, '\t')+"\tCONNECTION: ").c_str());
        PrintAttributes(d);
    }
    
    std::string JSONString(int d=0)
    {
        std::string s = std::string(d, '\t')+"{\n";
        s += JSONAttributeString(d+1);
        s += "\n" + std::string(d, '\t')+"}";
        return s;
    };
};

class ViewObjectElement: public Element
{
public:
    ViewObjectElement(XMLElement * xml_node=NULL) : Element(xml_node) {};

    void Print(int d=0)
    {
        printf("%s\n", (std::string(d, '\t')+"\tOBJECT: ").c_str());
        PrintAttributes(d);
    }

    std::string JSONString(int d=0)
    {
        std::string tab = std::string(d, '\t');
        std::string s = tab + "{\n";
        s += JSONAttributeString(d+1);
        s += "\n" + tab+"}";
        return s;
    };
};

class ViewElement: public Element
{
public:
    ViewElement(XMLElement * xml_node=NULL) : Element(xml_node) {};
    std::vector<ViewObjectElement> objects;

    void Print(int d=0)
    {
        printf("%s\n", (std::string(d, '\t')+"\tVIEW: ").c_str());
        PrintAttributes(d);
        printf("%s\n", (std::string(d, '\t')+"\tOBJECTS: ").c_str());
        for(auto o : objects)
            o.Print(d+1);
    }

    std::string JSONString(int d=0)
    {
        std::string tab = std::string(d, '\t');
        std::string tab2 = std::string(d+1, '\t');
        std::string b;
        std::string s = tab+"{\n";
        if(attributes.size())
            s += JSONAttributeString(d+1) + ",\n";
        
        if(objects.size())
        {
            s += tab2 + "\"objects\":\n" + tab2 + "[\n";
            for(auto o : objects)
            {
                s += b + o.JSONString(d+2);
                b = ",\n";
            }
            s += "\n";
            s += tab2 + "]";
        }
        else
            s += tab2 + "\"objects\": []";
        
        s += "\n" + std::string(d, '\t')+"}";
        return s;
    };
};

class GroupElement: public Element
{
public:
    std::unordered_map<std::string, GroupElement> groups;
    std::unordered_map<std::string, ParameterElement> parameters;
    std::vector<InputElement> inputs;
    std::unordered_map<std::string, OutputElement> outputs;
    std::vector<ConnectionElement> connections;
    std::vector<ViewElement> views;
    Module * module; // if this group is a 'class'; in this case goups should be empty // FIXME: remove ******

    GroupElement(XMLElement * xml_node=NULL) : Element(xml_node) {};

    GroupElement *
    GetGroup(const std::string & name)
    {
        if(name == "*" || name == "") // FIXME: remove later when stars are no longer needed
            return this;
        if(groups.count(name))
            return &groups[name];
        auto & path = split(name, ".", 1);
        if(path.size() > 1 && groups.count(path[0]))
            return groups[path[0]].GetGroup(path[1]);
        return NULL;
    }

    Module *
    GetModule(const std::string & module_name) // Get module from full or partial name relative to this group
    {
        if(auto g = GetGroup(module_name))
            return g->module;
        return NULL;
    }

    Module_IO *
    GetSource(const std::string & source_module_name, const std::string & source_name)
    {
        if(GroupElement * g = GetGroup(source_module_name))
        {
            if(g->module)
                return g->module->GetModule_IO(g->module->output_list, source_name.c_str());

            auto & output = g->outputs[source_name];
            auto new_module = output["sourcemodule"];
            auto new_source = output["source"];
            
            if(source_module_name == new_module && source_module_name == new_source)
                return NULL;
            
            return g->GetSource(new_module!="" ? new_module : source_module_name, new_source!="" ? new_source : source_name);
        }
        return NULL;
    }

    std::vector<Module_IO *>
    GetTargets(const std::string & target_module_name, const std::string & target_name)   // GetInputIO = GetModule + GetInputIO; a single Connect can result in many connections
    {
        std::vector<Module_IO *> tios;
        GroupElement * g = GetGroup(target_module_name);
        if(!g)
            return tios;
        
        if(g && g->module)
        {
            tios.push_back(g->module->GetModule_IO(g->module->input_list, target_name.c_str()));
            return tios;
        }
        
        for (auto & input : g->inputs) // we need to loop because there can be more than one input statement with the sama name for multiple connections
            if ((input["name"] == target_name) || (input["name"] == "*"))
            {
                auto new_module = input["targetmodule"];
                auto new_target = input["target"];
                auto targets = g->GetTargets(new_module!="" ? new_module : target_module_name, new_target!="" ? new_target : target_name);
                tios.insert(tios.end(), targets.begin(), targets.end());
            }

        return tios;
    }

    void Print(int d=0)
    {
        printf("%s\n", (std::string(d, '\t')+"GROUP:"+GetAttribute("name")).c_str());
        if(module)
            printf("%s\n", (std::string(d, '\t')+"MODULE:"+std::string(module->GetFullName())).c_str());

        printf("%s\n", (std::string(d, '\t')+"\tATTRIBUTES:").c_str());
        PrintAttributes(d+1);

        printf("%s\n", (std::string(d, '\t')+"\tPARAMETERS:").c_str());
        for(auto p : parameters)
            p.second.Print(d+1);

        printf("%s\n", (std::string(d, '\t')+"\tCONNECTIONS:").c_str());
        for(auto c : connections)
            c.Print(d+1);
/* TEMPORARY
        printf("%s\n", (std::string(d, '\t')+"\tVIEWS:").c_str());
        for(auto v : views)
            v->Print(d+1);
*/
        for(auto g : groups)
            g.second.Print(d+1);
        
        printf("\n");
    };

    std::string JSONString(int d=0)
    {
        std::string b;
        std::string tab = std::string(d, '\t');
        std::string tab2 = std::string(d+1, '\t');
        
        std::string s = tab + "{\n";

        s += JSONAttributeString(d+1);
        s += ",\n";
        
        if(parameters.size())
        {
            s += tab2 + "\"parameters\":\n" + tab2 + "[\n";
            for(auto p : parameters)
            {
                s += b + p.second.JSONString(d+2);
                b = ",\n";
            }
            s += "\n";
            s += tab2 + "]";
        }
        else
            s += tab2 + "\"parameters\": []";
        
        s += ",\n";
  
        
        if(connections.size())
        {
            b = "";
            s += tab2 + "\"connections\":\n" + tab2 + "[\n";
            for(auto c : connections)
            {
                s += b + c.JSONString(d+2);
                b = ",\n";
            }
            s += "\n";
            s += tab2 + "]";
        }
        else
            s += tab2 + "\"connections\": []";
        
        s += ",\n";

        if(views.size())
        {
            b = "";
            s += tab2 + "\"views\":\n" + tab2 + "[\n";
            for(auto v : views)
            {
                s += b + v.JSONString(d+2);
                b = ",\n";
            }
            s += "\n";
            s += tab2 + "]";
        }
        else
            s += tab2 + "\"views\": []";
        
        s += ",\n";
      
      
/*
        printf("%s\n", (std::string(d, '\t')+"\tVIEWS:").c_str());
        for(auto v : views)
            v->Print(d+1);
*/

        if(groups.size())
        {
            s += tab2 + "\"groups\":\n" + tab2 + "[\n";
            b = "";
            for(auto g : groups)
            {
                s += b + g.second.JSONString(d+2);
                b = ",\n";
            }
            s += "\n";
            s += tab2 + "]\n";
        }
        else
            s += tab2 + "\"groups\": []\n";

        s += tab + "}";
        
        return s;
    };

};



//
// ModuleClass
//

ModuleClass::ModuleClass(const char * n, ModuleCreator mc, const char * p, ModuleClass * nxt)
{
    name = n;
    module_creator = mc;
    path = create_string(p);
    next = nxt;
}

ModuleClass::~ModuleClass()
{
    delete path;
    delete next;
}


const char *
ModuleClass::GetClassPath(const char * class_name)
{
    if (equal_strings(name, class_name))
        return path;
    else if (next != NULL)
        return next->GetClassPath(class_name);
    else
        return NULL;
}

Module *
CreateModule(ModuleClass * c, const char * class_name, const char * module_name, Parameter * p)
{
    if (c == NULL)
        return NULL;
    
    else if (equal_strings(c->name, class_name))
    {
        Module * m = (*c->module_creator)(p);
        if(!m->input_list && !m->output_list)
            m->AddIOFromIKC();
        return m;
    }
    else
        return CreateModule(c->next, class_name, module_name, p);
}

void
Module_IO::Allocate()
{
    if (sizex == unknown_size || sizey == unknown_size)
    {
        if (module != NULL && !optional)
            module->Notify(msg_fatal_error, "Attempting to allocate io (\"%s\") with unknown size for module \"%s\" (%s). Check that all required inputs are connected.\n", name.c_str(), module->GetName(), module->GetClassName());
        return;
    }
    if(sizex*sizey <= 0)
    {
        if (module != NULL)
            module->Notify(msg_fatal_error, "Internal error while trying to allocate data of size 0.\n");
        return;
    }
    
    if (module != NULL) module->Notify(msg_debug, "Allocating data of size %d.\n", size);
    data	=   new float * [max_delay];
    matrix  =   new float ** [max_delay];
    for (int d=0; d<max_delay; d++)
    {
        matrix[d] = create_matrix(sizex, sizey);
        data[d] = matrix[d][0];
    }
}

Module_IO::Module_IO(Module_IO * nxt, Module * m, const char * n, int x, int y, bool opt, bool multiple)
{
    optional = opt;
    allow_multiple = multiple;
    next = nxt;
    module = m;
    name = n;
    data = NULL;
    matrix = NULL;
    size = x*y;
    sizex = x;
    sizey = y;
    max_delay = 1;
}

Module_IO::~Module_IO()
{
/*
    // Cannot print in destructor
    if (name != NULL)
    {
        if (module != NULL) module->Notify(msg_debug, "      Deleting Module_IO \"%s\".\n", name);
    }
    else
    {
        if (module != NULL) module->Notify(msg_debug, "      Deleting Module_IO\n");
    }
*/
    if (matrix)
        for (int d=0; d<max_delay; d++)
            destroy_matrix(matrix[d]);
    delete [] data;
    delete [] matrix;
    delete next;
}

void
Module_IO::SetSize(int x, int y)
{
    int s = x*y;
    if (x == unknown_size)
        return;
    if (size != unknown_size && s != size)
    {
        if (module != NULL)
            module->Notify(msg_fatal_error, "Module_IO::SetSize: Attempt to resize data array \"%s\" of module \"%s\" (%s) (%d <= %d). Ignored.\n",  name.c_str(), module->GetName(), module->GetClassName(), size, s);
        return;
    }
    if (s == size)
        return;
    if (s == 0)
        return;
    if (module != NULL)
        module->Notify(msg_debug, "Allocating memory for input/output \"%s\" of module \"%s\" (%s) with size %d and max_delay = %d (in SetSize).\n", name.c_str(), module->instance_name, module->GetClassName(), s, max_delay);
    sizex = x;
    sizey = y;
    size = x*y;
    if (module != NULL && module->kernel != NULL)
        module->kernel->NotifySizeChange();
}

void
Module_IO::DelayOutputs()
{
    for (int d=max_delay-1; d>0; d--)
        copy_matrix(matrix[d], matrix[d-1], sizex, sizey);
}

Module::~Module()
{
    Notify(msg_debug, "    Deleting Module \"%s\".\n", instance_name);
	destroy_string(full_instance_name);
    delete timer;
    delete input_list;
    delete output_list;
//    delete next;
}

void
Module::AddInput(const char * name, bool optional, bool allow_multiple_connections)
{
    if (GetModule_IO(input_list, name) != NULL)
    {
        Notify(msg_warning, "Input \"%s\" of module \"%s\" (%s) already exists.\n", name, GetName(), GetClassName());
        return;
    }
    input_list = new Module_IO(input_list, this, name, unknown_size, 1, optional, allow_multiple_connections);
    Notify(msg_debug, "  Adding input \"%s\".\n", name);
}

void
Module::AddOutput(const char * name, bool optional, int sizeX, int sizeY)
{
    if (GetModule_IO(output_list, name) != NULL)
    {
        Notify(msg_warning, "Output \"%s\" of module \"%s\" (%s) already exists.\n", name, GetName(), GetClassName());
        return;
    }
    output_list = new Module_IO(output_list, this, name, sizeX, sizeY, optional);
    Notify(msg_debug, "  Adding output \"%s\" of size %d x %d to module \"%s\" (%s).\n", name, sizeX, sizeY, GetName(), GetClassName());
}

const char *
Module::GetName()
{
    return instance_name;
}

const char *
Module::GetFullName()
{
	return full_instance_name;
}

const char *
Module::GetClassName()
{
    return class_name;
}

const char *
Module::GetClassPath()
{
    char * s = create_string(kernel->GetClassPath(GetClassName()));
    unsigned long p = strlen(s)-1;
    while(s[p] != '/')
        s[p--] = '\0';
    return s;
}

long
Module::GetTickLength()
{
    return kernel->GetTickLength();
}


long
Module::GetTick()
{
    return kernel->GetTick();
}



void
StoreArray(const char * path, const char * name, float * a, int size)
{
    store_array(path, name, a, size);
}



void
StoreMatrix(const char * path, const char * name, float ** m, int size_x, int size_y)
{
    store_matrix(path, name, m, size_x, size_y);
}



bool
LoadArray(const char * path, const char * name, float * a, int size)
{
    return load_array(path, name, a, size);

}



bool
LoadMatrix(const char * path, const char * name, float ** m, int size_x, int size_y)
{
    return load_matrix(path, name, m, size_x, size_y);
}



void
Module::Store(const char * path)
{
    // will implement default store behavior later
//    printf("Store: %s\n", path);
}



void
Module::Load(const char * path)
{
    // will implement default load behavior later
//    printf("Load: %s\n", path);
}



const char *
Module::GetList(const char * n) // TODO: Check that this complicated procedure is really necessary; join with GetDefault and GetValue
{
    const char * module_name = GetName();
    
    // loop up the group hierarchy
    for (XMLElement * parent = xml->GetParentElement(); parent != NULL; parent = parent->GetParentElement())
    {
        // Look for parameter element that redefines the attribute name
        for (XMLElement * parameter = parent->GetContentElement("parameter"); parameter != NULL; parameter = parameter->GetNextElement("parameter"))
        {
            if(equal_strings(kernel->GetXMLAttribute(parameter, "name"), n))
            {
                const char * d = kernel->GetXMLAttribute(parameter, "values");
                if(d)
                    return d;
            }
            
            const char * t = kernel->GetXMLAttribute(parameter, "target");
            if (equal_strings(t, n))
            {                
                // we have found our parameter
                // it controls this module if module name is set to the name of this module or if it is not set = relates to all modules
                const char * tm = kernel->GetXMLAttribute(parameter, "module");
                if (tm == NULL || (equal_strings(tm, module_name)))
                {
                    // use default if it exists
                    const char * d = kernel->GetXMLAttribute(parameter, "values");
                    if(d)
                        return d;
                    
                    // the parameter element redefines our parameter name; get the new name
                    const char * newname = kernel->GetXMLAttribute(parameter, "name");
                    if (newname == NULL)
                    {
                        Notify(msg_fatal_error, "A parameter element with target \"%s\" lacks a name attribute.\n", t);
                        return NULL;
                    }
                    // we have the new name; set it and see if it is defined in the current group element
                    n = newname;
                }
            }
        }
        
        // It was not found here; shift module name to that of the current group and continue up the group hierarchy...
        module_name = kernel->GetXMLAttribute(parent, "name"); // FIXME: check if this will ever happen with the new GetXMLAttribute call
    }
    return NULL; // No list value was found
}



const char *
Module::GetDefault(const char * n)
{
    const char * module_name = GetName();
    
    // loop up the group hierarchy
    for (XMLElement * parent = xml->GetParentElement(); parent != NULL; parent = parent->GetParentElement())
    {
        // Look for parameter element that redefines the attribute name
        for (XMLElement * parameter = parent->GetContentElement("parameter"); parameter != NULL; parameter = parameter->GetNextElement("parameter"))
        {
            if(equal_strings(kernel->GetXMLAttribute(parameter, "name"), n))
            {
                const char * d = kernel->GetXMLAttribute(parameter, "default");
                if(d)
                    return (*d != '\0' ? d : NULL); // return NULL if default is an empty string
            }
            
            const char * t = kernel->GetXMLAttribute(parameter, "target");
            if (equal_strings(t, n))
            {                
                // we have found our parameter
                // it controls this module if module name is set to the name of this module or if it is not set = relates to all modules
                const char * tm = kernel->GetXMLAttribute(parameter, "module");
                if (tm == NULL || (equal_strings(tm, module_name)))
                {
                    // use default if it exists
                    const char * d = kernel->GetXMLAttribute(parameter, "default");
                    if(d)
                        return (*d != '\0' ? d : NULL); // return NULL if default is an empty string;
                    
                    // the parameter element redefines our parameter name; get the new name
                    const char * newname = kernel->GetXMLAttribute(parameter, "name");
                    if (newname == NULL)
                    {
                        Notify(msg_fatal_error, "A parameter element with target \"%s\" lacks a name attribute.\n", t);
                        return NULL;
                    }
                    // we have the new name; set it and see if it is defined in the current group element
                    n = newname;
                }
            }
        }
        
        // It was not found here; shift module name to that of the current group and continue up the group hierarchy...
        module_name = kernel->GetXMLAttribute(parent, "name");
    }
    
    return NULL; // No default value was found
}



const char *
Module::GetValue(const char * n)	// This function implements attribute inheritance with renaming through the parameter element
{
    const char * module_name = GetName();
    // Check for local value in this element
    const char * value = kernel->GetXMLAttribute(xml, n);
    if (value != NULL)
    {
        if(value[0] == '@')
            return GetValue(&n[1]); // implements variables
        else
            return value;
    }
    // not found here, loop up the group hierarchy
    for (XMLElement * parent = xml->GetParentElement(); parent != NULL; parent = parent->GetParentElement())
    {
        // Look for parameter element that redefines the attribute name
        for (XMLElement * parameter = parent->GetContentElement("parameter"); parameter != NULL; parameter = parameter->GetNextElement("parameter"))
        {
            if (equal_strings(kernel->GetXMLAttribute(parameter, "target"), n))
            {
                // we have found our parameter
                // it controls this module if module name is set to the name of this module or if it is not set = relates to all modules
                const char * tm = kernel->GetXMLAttribute(parameter, "module");
                if (tm == NULL || (equal_strings(tm, module_name)))
                {
                    // the parameter element redefines our parameter name; get the new name
                    const char * newname = kernel->GetXMLAttribute(parameter, "name");
                    if (newname == NULL)
                    {
                        // Notify(msg_fatal_error, "A parameter element with target \"%s\" lacks a name attribute.\n", t);
                        return GetDefault(n);
                    }
                    // we have the new name; set it and see if it is defined in the current group element
                    n = newname;
                }
            }
        }
        value = kernel->GetXMLAttribute(parent, n);
        if (value != NULL)
        {
            if(value[0] == '@')
                return GetValue(&n[1]); // implements variables
            else
                return value;
        }
        // It was not found here; shift module name to that of the current group and continue up the group hierarchy...
        module_name = kernel->GetXMLAttribute(parent, "name");
    }
    
    // No value was found, check if we are in batch mode and look for batch a value
/*
    value = kernel->GetBatchValue(n);
    if (value != NULL)
        return value;
    
    // Look in the values assigned at start up
    
    value = kernel->options->GetValue(n);
    if(value != NULL)
        return value;
*/
    // As a last step, look for default instead
    
    return GetDefault(n);
}



float
Module::GetFloatValue(const char * n, float d)
{
    if(d != 0)
        Notify(msg_warning, "Default value for GetFloatValue(\"%s\") is deprecated and should be specified in IKC file instead.", n);

    return string_to_float(GetValue(n), d);
}

int
Module::GetIntValue(const char * n, int d)
{
    if(d != 0)
        Notify(msg_warning, "Default value for GetIntValue(\"%s\") is deprecated and should be specified in IKC file instead.", n);
    
    if(GetList(n))
        return GetIntValueFromList(n);
    else
        return string_to_int(GetValue(n), d);
}


bool
Module::GetBoolValue(const char * n, bool d) // TODO: Use above default
{
    const char * v = GetValue(n);
    if (v == NULL)
        return d;
    else
        return string_to_bool(v);
}

static int
findindex(const char * name, const char * list)
{
    int ix = 0;
    int lp = 0;
    while (true)
    {
        // try to match name in list
        int np = 0;
        while (list[lp] && list[lp] != '/'  && name[np] == list[lp])
        {
            np++;
            lp++;
        }
        // name matches, return index
        if ((list[lp] == '/' || list[lp] == 0) && name[np] == 0)
            return ix;
        // skip to next name
        ix++;
        while (list[lp] && list[lp] != '/')
            lp++;
        // no match
        if (list[lp] == 0)
            return -1;
        // prepare for next match
        lp++;
    }
}



int
Module::GetIntValueFromList(const char * n, const char * list)
{
    const char * l = GetList(n);
    if(!l)
        l = list;
    if(!l)
    {
        Notify(msg_warning, "List values not defined for \"%s\".\n", n);
        return 0;
    }
    const char * v = GetValue(n);
    if (!v)
        return 0;
    else
    {
        int ix = findindex(v, l);
        
        if(ix == -1)
        {
            Notify(msg_warning, "List value \"%s\" is not defined. Using default value.\n", v);
            return 0;
        }
        else
            return ix;
    }
}



float *
Module::GetArray(const char * n, int & size, bool fixed_size)
{
    return create_array(GetValue(n), size, fixed_size);
}



int *
Module::GetIntArray(const char * n, int & size, bool fixed_size)
{
    int requested_size = size;
    int data_size = 0;

    const char * v = GetValue(n);
    if (v == NULL)
    {
        if(requested_size > 0)
        {
            size = requested_size;
            int * a = new int [size];
            for(int i=0; i<size; i++)
                a[i] = 0;
            return a;
        }
        else
        {
            size = 0;
            return NULL;
        }
    }

    const char * vv = v;
    
    // Count values
    
    while(*v != '\0')
    {
        int x;
        for (; isspace(*v) && *v != '\0'; v++) ;
        if (sscanf(v, "%d", &x)!=-1)
            data_size++;
        for (; !isspace(*v) && *v != '\0'; v++) ;
    }
    
    if(size == 0)
    {
        requested_size = data_size;
        size = data_size;
    }
    
    int d = 0;
    int * a = new int[requested_size];
    v = vv;
    
    for (int i=0; i<requested_size;i++)
    {
        for (; isspace(*v) && *v != '\0'; v++) ;
        if (i >= requested_size || (sscanf(v, "%d", &a[i])==-1))
            a[i] = d;
        d = a[i]; // save last as default value
        for (; !isspace(*v) && *v != '\0'; v++) ;
    }
    
    return a;
}



float **
Module::GetMatrix(const char * n, int & sizex, int & sizey, bool fixed_size)
{
    return create_matrix(GetValue(n), sizex, sizey, fixed_size);
}



void
Module::Bind(float & v, const char * n)
{
    // TODO: check type here
    v = GetFloatValue(n);
    bindings = new Binding(this, n, bind_float, &v, 0, 0, bindings);
}



void
Module::Bind(float * & v, int size, const char * n, bool fixed_size)
{
    // TODO: check type here
    v = GetArray(n, size, fixed_size);
    bindings = new Binding(this, n, bind_array, v, size, 1, bindings);
}



void
Module::Bind(float ** & v, int & sizex, int & sizey, const char * n, bool fixed_size)
{
    // TODO: check type here
    v = GetMatrix(n, sizex, sizey, fixed_size);
    bindings = new Binding(this, n, bind_matrix, v, sizex, sizey, bindings);
}



void
Module::Bind(int & v, const char * n)
{
    // TODO: check type here
    if(GetList(n))
    {
        v = GetIntValueFromList(n);
        bindings = new Binding(this, n, bind_list, &v, 0, 0, bindings);
    }
    else
    {
        v = GetIntValue(n);
        bindings = new Binding(this, n, bind_int, &v, 0, 0, bindings);
    }
}



void
Module::Bind(bool & v, const char * n)
{
    // TODO: check type here
    v = GetBoolValue(n);
    bindings = new Binding(this, n, bind_bool, &v, 0, 0, bindings);
}



void
Module::Bind(std::string & v, const char * n)
{
    // TODO: check type here
    v = std::string(GetValue(n));
    bindings = new Binding(this, n, bind_string, &v, 0, 0, bindings);
}



Module_IO *
Module::GetModule_IO(Module_IO * list, const char * name)
{
    for (Module_IO * i = list; i != NULL; i = i->next)
        if (equal_strings(name, i->name.c_str()))
            return i;
    return NULL;
}



void
Module::AllocateOutputs()
{
    for (Module_IO * i = output_list; i != NULL; i = i->next)
        i->Allocate();
}

void
Module::DelayOutputs()
{
    for (Module_IO * i = output_list; i != NULL; i = i->next)
        i->DelayOutputs();
}

float *
Module::GetInputArray(const char * name, bool required)
{
    for (Module_IO * i = input_list; i != NULL; i = i->next)
        if (equal_strings(name, i->name.c_str()))
        {
            if (i->data == NULL)
            {
                if(required && !i->optional)
                    Notify(msg_fatal_error, "Input array \"%s\" of module \"%s\" (%s) has no allocated data. Returning NULL.\n", name, GetName(), GetClassName());
                return NULL;
            }
            else
                return i->data[0];
        }
    Notify(msg_warning, "Input array \"%s\" of module \"%s\" (%s) does not exist. Returning NULL.\n", name, GetName(), GetClassName());
    return NULL;
}

float *
Module::GetOutputArray(const char * name, bool required)
{
    for (Module_IO * i = output_list; i != NULL; i = i->next)
        if (equal_strings(name, i->name.c_str()))
        {
            if (i->data == NULL)
            {
                if(required && !i->optional)
                    Notify(msg_fatal_error, "Output array \"%s\" of module \"%s\" (%s) has no allocated data. Returning NULL.\n", name, GetName(), GetClassName());
                return NULL;
            }
            else
                return i->data[0];
        }
    Notify(msg_warning, "Output array  \"%s\" of module \"%s\" (%s) does not exist. Returning NULL.\n", name, GetName(), GetClassName());
    return NULL;
}

float **
Module::GetInputMatrix(const char * name, bool required)
{
    for (Module_IO * i = input_list; i != NULL; i = i->next)
        if (equal_strings(name, i->name.c_str()))
        {
            if (i->matrix == NULL)
            {
                if(required && !i->optional)
                    Notify(msg_fatal_error, "Input matrix \"%s\" of module \"%s\" (%s) has no allocated data. Returning NULL.\n", name, GetName(), GetClassName());
                return NULL;
            }
            else
                return i->matrix[0];
        }
    Notify(msg_warning, "Input matrix \"%s\" of module \"%s\" (%s) does not exist. Returning NULL.\n", name, GetName(), GetClassName());
    return NULL;
}

float **
Module::GetOutputMatrix(const char * name, bool required)
{
    for (Module_IO * i = output_list; i != NULL; i = i->next)
        if (equal_strings(name, i->name.c_str()))
        {
            if (i->matrix == NULL)
            {
                if(required && !i->optional)
                    Notify(msg_fatal_error, "Output matrix \"%s\" of module \"%s\" (%s) has no allocated data. Returning NULL.\n", name, GetName(), GetClassName());
                return NULL;
            }
            else
                return i->matrix[0];
        }
    Notify(msg_warning, "Output matrix \"%s\" of module \"%s\" (%s) does not exist. Returning NULL.\n", name, GetName(), GetClassName());
    return NULL;
}

int
Module::GetInputSize(const char * input_name)
{
    // Find the Module_IO for this input
    for (Module_IO * i = input_list; i != NULL; i = i->next)
        if (equal_strings(input_name, i->name.c_str()))
        {
            if (i->size != unknown_size)
                return i->size;
            else if (kernel != NULL)
                return kernel->CalculateInputSize(i);
            else
                break;
        }
    Notify(msg_fatal_error, "Cannot calculate input size for \"%s\" of module \"%s\" (%s).\n", input_name, instance_name, class_name);
    return unknown_size;
}

int
Module::GetInputSizeX(const char * input_name) // TODO: also used internally so cannot be removed
{
    // Find the Module_IO for this input
    for (Module_IO * i = input_list; i != NULL; i = i->next)
        if (equal_strings(input_name, i->name.c_str()))
        {
            if (i->sizex != unknown_size)
                return i->sizex;
            else if (kernel != NULL)
                return kernel->CalculateInputSizeX(i);
            else
                break;
        }
    Notify(msg_fatal_error, "Cannot calculate input x size for \"%s\" of module \"%s\" (%s).\n", input_name, instance_name, class_name);
    return unknown_size;
}

int
Module::GetInputSizeY(const char * input_name) // TODO: also used internally so cannot be removed
{
    // Find the Module_IO for this input
    for (Module_IO * i = input_list; i != NULL; i = i->next)
        if (equal_strings(input_name, i->name.c_str()))
        {
            if (i->sizex != unknown_size)	// Yes, sizeX is correct
                return i->sizey;
            else if (kernel != NULL)
                return kernel->CalculateInputSizeY(i);
            else
                break;
        }
    Notify(msg_fatal_error, "Cannot calculate input y size for \"%s\" of module \"%s\" (%s).\n", input_name, instance_name, class_name);
    return unknown_size;
}

int
Module::GetOutputSize(const char * name)
{
    for (Module_IO * i = output_list; i != NULL; i = i->next)
        if (equal_strings(name, i->name.c_str()))
            return i->size;
    Notify(msg_warning, "Attempting to get size of non-existing output %s.%s \n", this->instance_name, name);
    return 0;
}

int
Module::GetOutputSizeX(const char * name)
{
    for (Module_IO * i = output_list; i != NULL; i = i->next)
        if (equal_strings(name, i->name.c_str()))
            return  i->sizex;
    Notify(msg_warning, "Attempting to get size of non-existing output %s.%s \n", this->instance_name, name);
    return 0;
}

int
Module::GetOutputSizeY(const char * name)
{
    for (Module_IO * i = output_list; i != NULL; i = i->next)
        if (equal_strings(name, i->name.c_str()))
            return  i->sizey;
    Notify(msg_warning, "Attempting to get size of non-existing output %s.%s \n", this->instance_name, name);
    return 0;
}


void
Module::io(float * & a, const char * name)
{
    if((a = GetOutputArray(name, false)))
        return;
    else if((a = GetInputArray(name, false)))
        return;
}


void
Module::io(float * & a, int & size, const char * name)
{
    if((a = GetOutputArray(name, false)))
        size = GetOutputSize(name);
    else if((a = GetInputArray(name, false)))
        size = GetInputSize(name);
    else
        size = 0;
}


void
Module::io(float ** & m, int & size_x, int & size_y, const char * name)
{
    if((m = GetOutputMatrix(name, false)))
    {
        size_x = GetOutputSizeX(name);
        size_y = GetOutputSizeY(name);
    }
    else if((m = GetInputMatrix(name, false)))
    {
        size_x = GetInputSizeX(name);
        size_y = GetInputSizeY(name);
   }
    else
    {
        size_x = 0;
        size_y = 0;
    }
}



void
Module::SetOutputSize(const char * name, int x, int y)
{
    if (x < -1 || y < -1)
    {
        Notify(msg_warning, "Attempting to set negative size of %s.%s \n", this->instance_name, name);
        return;
    }
    for (Module_IO * i = output_list; i != NULL; i = i->next)
        if (equal_strings(name, i->name.c_str()))
            i->SetSize(x, y);
}

Module::Module(Parameter * p)
{
    input_list = NULL;
    output_list = NULL;
    bindings = NULL;
    timer = new Timer();
    time = 0;
    ticks = 0;
    kernel = p->kernel;
    xml = p->xml;
    instance_name = kernel->GetXMLAttribute(xml, "name"); // GetValue("name");
    class_name = kernel->GetXMLAttribute(xml, "class");
    period = (GetValue("period") ? GetIntValue("period") : 1);
    phase = (GetValue("phase") ? GetIntValue("phase") : 0);
	active = GetBoolValue("active", true);

	// Compute full name
	
	char n[1024] = "";
    const char * group[128];
    int i=0;
    for (XMLElement * parent = xml->GetParentElement(); parent != NULL; parent = parent->GetParentElement())
        if(kernel->GetXMLAttribute(parent,"name") && i<100)
            group[i++] = kernel->GetXMLAttribute(parent, "name");
    for(int j=i-1; j>=0; j--)
    {
        append_string(n, group[j], 1024);
        if(j>0) append_string(n, ".", 1024);
    }
    full_instance_name = create_string(n);
}

void
Module::AddIOFromIKC()
{
	if(!xml->GetParentElement())
        return;
    
    for(XMLElement * e=xml->GetParentElement()->GetContentElement("input"); e != NULL; e = e->GetNextElement("input"))
    {
        const char * amc = kernel->GetXMLAttribute(e, "allow_multiple_connections");
        bool multiple = (amc ? string_to_bool(amc) : true); // True is defaut value
//        AddInput(kernel->GetXMLAttribute(e, "name"), string_to_bool(kernel->GetXMLAttribute(e, "optional")), multiple);

        const char * opt = kernel->GetXMLAttribute(e, "optional");
        if(!opt)
            AddInput(kernel->GetXMLAttribute(e, "name"), false, multiple);
        else
            AddInput(kernel->GetXMLAttribute(e, "name"), string_to_bool(opt), multiple);
    }
    
    for(XMLElement * e=xml->GetParentElement()->GetContentElement("output"); e != NULL; e = e->GetNextElement("output"))
    {
        const char * opt = kernel->GetXMLAttribute(e, "optional");
        if(!opt)
            AddOutput(kernel->GetXMLAttribute(e, "name"), false);
        else
            AddOutput(kernel->GetXMLAttribute(e, "name"), string_to_bool(opt));
    }
}

// Default SetSizes sets output sizes from IKC file based on size_set, size_param, and size attributes
//

int
Module::GetSizeXFromList(const char * sizearg)
{
    int sx = unknown_size;

    char * l = create_string(sizearg);
    char * ll = l;

    // strip blanks

    int i=0, j=0;
    while(l[j] != 0)
    {
        if(l[j] == ' ')
            j++;
        else
            l[i++]=l[j++];
    }
    l[i] = 0;
    
    char * s = l;
    char * input;
    input = strsep(&s, ",");
    while(input)
    {
        int new_sx = GetInputSizeX(input);
        if(sx == unknown_size )
            sx = new_sx;
        else if(new_sx != unknown_size && new_sx != sx)
        {
            Notify(msg_warning, "Incompatible sizes for set_size_x, using max(%s)", sizearg);
            sx = max(sx, new_sx);
        }
        input = strsep(&s, ",");
    }
    destroy_string(ll);
    
    return sx;
}



int
Module::GetSizeYFromList(const char * sizearg)
{
    int sy = unknown_size;

    char * l = create_string(sizearg);
    char * ll = l;

    // strip blanks

    int i=0, j=0;
    while(l[j] != 0)
    {
        if(l[j] == ' ')
            j++;
        else
            l[i++]=l[j++];
    }
    l[i] = 0;

    char * s = l;
    char * input;
    input = strsep(&s, ",");
    while(input)
    {
        int new_sy = GetInputSizeY(input);
        if(sy == unknown_size )
            sy = new_sy;
        else if(new_sy != unknown_size && new_sy != sy)
        {
            Notify(msg_warning, "Incompatible sizes for set_size_y, using max(%s)", sizearg);
            sy = max(sy, new_sy);
        }
        input = strsep(&s, ",");
    }
    destroy_string(ll);
    
    return sy;
}



// Default SetSizes sets output sizes from IKC file based on size_set, size_param, and size attributes

void
Module::SetSizes()
{
	if(xml->GetParentElement())
	{
		const char * sizearg;
		const char * sizeargy;
        const char * arg;
		for(XMLElement * e=xml->GetParentElement()->GetContentElement("output"); e != NULL; e = e->GetNextElement("output"))
        {
            const char * output_name = kernel->GetXMLAttribute(e, "name");

            // First get simple attributes
            
            int sx = unknown_size;
            int sy = unknown_size;

            if((sx == unknown_size) && (sizearg = e->GetAttribute( "size_param_x")) && (arg = GetValue(sizearg)))
                sx = string_to_int(arg);
            
            if((sy == unknown_size) && (sizearg = e->GetAttribute( "size_param_y")) && (arg = GetValue(sizearg)))
                sy = string_to_int(arg);
            
            if((sx == unknown_size) && (sizearg = e->GetAttribute( "size_param")) && (arg = GetValue(sizearg)))
            {
                sx = string_to_int(arg);
                sy = 1;
            }
            
            if((sx == unknown_size) && (sizearg = e->GetAttribute( "size_x")))
                sx = string_to_int(sizearg);
            
            if((sy == unknown_size) && (sizearg = e->GetAttribute( "size_y")))
                sy = string_to_int(sizearg);
            
            if((sx == unknown_size) && (sizearg = e->GetAttribute( "size")))
            {
                sx = string_to_int(sizearg);
                sy = 1;
            }
            
			if((sx == unknown_size) && (sy == unknown_size) && (sizearg = e->GetAttribute( "size_set"))) // Set output size x & y from one or multiple inputs
            {
                sx = GetSizeXFromList(sizearg);
                sy = GetSizeYFromList(sizearg);
			}
            
			else if((sx == unknown_size) && (sizearg = e->GetAttribute( "size_set_x")) && (sizeargy = e->GetAttribute( "size_set_y")) ) // Set output size x from one or multiple different inputs for both x and y
			{
                sx = GetSizeXFromList(sizearg) * GetSizeYFromList(sizearg);     // Use total input sizes
                sy = GetSizeXFromList(sizeargy) * GetSizeYFromList(sizeargy);   // TODO: Check that no modules assumes it is ony X or Y sizes
			}
            
			else if((sx == unknown_size) && (sizearg = e->GetAttribute( "size_set_x"))) // Set output size x from one or multiple inputs
                sx = GetSizeXFromList(sizearg);
            
			else if((sy == unknown_size) && (sizearg = e->GetAttribute( "size_set_y"))) // Set output size y from one or multiple inputs
                sy = GetSizeYFromList(sizearg);
            
            SetOutputSize(output_name, sx, sy);
        }
	}
}

void
Module::Notify(int msg)
{
    if (kernel != NULL)
        kernel->Notify(msg, "\n");
    else if (msg == msg_fatal_error)
    {
        global_fatal_error = true;
        global_error_count++;
    }
    else if(msg == msg_warning)
    {
//        global_warning_count++;
    }
}

void
Module::Notify(int msg, const char *format, ...)
{
    if(msg>log_level)
        return;
    char 	message[512];
    sprintf(message, "%s (%s): ", GetFullName(), GetClassName());
    size_t n = strlen(message);
    va_list args;
    va_start(args, format);
    vsnprintf(&message[n], 512, format, args);
    va_end(args);
    if (kernel != NULL && msg>=log_level)
    {
        kernel->Notify(-msg, message);
    }
    else if (msg == msg_fatal_error)
    {
        global_fatal_error = true;
        if(message[strlen(message)-1] == '\n')
            message[strlen(message)-1] = '\0';
        printf("IKAROS: ERROR: %s\n", message);
        global_error_count++;
    }
    else if(msg == msg_warning)
    {
 //        global_warning_count++;
   }
}

Connection::Connection(Connection * n, Module_IO * sio, int so, Module_IO * tio, int to, int s, int d, bool a)
{
    if(d == 0 && !a)
        kernel().Notify(msg_warning, "Zero-delay connection from \"%s\" of module \"%s\" to \"%s\" of module \"%s\" cannot be inactivated. Will be active.", sio->name.c_str(), sio->module->GetName(), tio->name.c_str(), tio->module->GetName());

    source_io = sio;
    source_offset = so;
    target_io = tio;
    target_offset = to;
    size = s;
    delay = d;
    active = a;
    next = n;
}

Connection::~Connection()
{
    if (source_io != NULL && source_io->module != NULL)
        source_io->module->Notify(msg_debug, "    Deleting Connection.\n");
    delete next;
}

void
Connection::Propagate(long tick)
{
    if(!active)
        return;
    if (delay == 0)
        return;
    // Return if both modules will not start in this tick - necessary when using threads
    if (tick % source_io->module->period != source_io->module->phase)
        return;
    if (tick % target_io->module->period != target_io->module->phase)
        return;
    if (source_io != NULL && source_io->module != NULL)
        source_io->module->Notify(msg_debug, "  Propagating %s.%s -> %s.%s (%p -> %p) size = %d\n", source_io->module->GetName(), source_io->name.c_str(), target_io->module->GetName(), target_io->name.c_str(), source_io->data, target_io->data, size);
    for (int i=0; i<size; i++)
        target_io->data[0][i+target_offset] = source_io->data[delay-1][i+source_offset];
}



ThreadGroup::ThreadGroup(Kernel * k)
{
    period = 1;
    phase = 0;
    thread = NULL; // new Thread();
}


ThreadGroup::ThreadGroup(Kernel * k, int period_, int phase_)
{
    period = period_;
    phase = phase_;
    thread = NULL; // new Thread();
}


ThreadGroup::~ThreadGroup()
{
//    delete thread;
//    delete next;  // FIXME: do this correctly
}

static void *
ThreadGroup_Tick(void * group)
{
    ((ThreadGroup *)group)->Tick();
    return NULL;
}

void
ThreadGroup::Start(long tick)
{
    // Test if group should be started
    if (tick % period == phase)
    {
//        if (thread->Create(ThreadGroup_Tick, (void*)this))
//            printf("Thread Creation Failed!\n");
        thread = new std::thread(ThreadGroup_Tick, (void*)this);
    }
}

void
ThreadGroup::Stop(long tick)
{
    // Test if group should be joined
    if ((tick + 1) % period == phase)
    {
        thread->join();
        delete thread;
        thread = NULL;
    }
}

void
ThreadGroup::Tick()
{
    for (Module * m: _modules)
    {
        m->timer->Restart();
        if(m->active)
            m->Tick();
        m->time += m->timer->GetTime();
        m->ticks += 1;
    }
}


Kernel::Kernel()
{
    options             = NULL;
    useThreads          = false;
    max_ticks           = -1;
    tick_length         = 0;
    nan_checks          = false;
    
    log_level           = log_level_info;
    ikaros_dir          = NULL;
    ikc_dir             = NULL;
    ikc_file_name       = NULL;

    tick                = 0;
    xmlDoc              = NULL;
    classes             = NULL;
//    modules             = NULL;
    connections         = NULL;
    module_count        = 0;
    period_count        = 0;
    phase_count         = 0;
    end_of_file_reached = false;
    fatal_error_occurred	= false;
    terminate			= false;
    sizeChangeFlag      = false;
//    threadGroups        = NULL;
    
    logfile     = NULL;
    timer		= new Timer();
}



void
Kernel::SetOptions(Options * opt)
{
    options             = opt;
    useThreads          = options->GetOption('t') || options->GetOption('T');
    max_ticks           = string_to_int(options->GetArgument('s'), -1);
    tick_length         = string_to_int(options->GetArgument('r'), 0);
    nan_checks          = options->GetOption('n');
    
    if (options->GetOption('q'))
        log_level = log_level_off;
        if (options->GetOption('v'))
            log_level = log_level_trace;

    
    // Compute ikaros root path
    
    ikaros_dir = NULL;
    if(is_absolute_path(IKAROSPATH))
        ikaros_dir = create_string(IKAROSPATH);
    else if(options->GetBinaryDirectory() != NULL)
        ikaros_dir = create_formatted_string("%s%s", options->GetBinaryDirectory(), IKAROSPATH);
    else
        Notify(msg_fatal_error, "The Ikaros root directory could not be established. Please set an absolute IKAROSPATH in IKAROS_System.h\n");
    
    // Compute ikc path and name
                        
   ikc_dir = options->GetFileDirectory();
   ikc_file_name =  options->GetFileName();
                        
    // Seed random number generator
                        
    if(options->GetOption('z'))
        srandom(string_to_int(options->GetArgument('z')));

    
    // Fix module paths
    
    for(ModuleClass * c = classes; c != NULL; c = c->next)
        if(c->path != NULL && c->path[0] != '/')
        {
            const char * t = c->path;
            c->path = create_formatted_string("%s%s", ikaros_dir, c->path);
            destroy_string((char *)t);
        }
}



Kernel::Kernel(Options * opt)
{
    options             = opt;
    useThreads          = options->GetOption('t') || options->GetOption('T');
    max_ticks           = string_to_int(options->GetArgument('s'), -1);
    tick_length         = string_to_int(options->GetArgument('r'), 0);
    nan_checks          = options->GetOption('n');
    
    log_level		= log_level_info;
    if (options->GetOption('q'))
        log_level = log_level_off;
    if (options->GetOption('v'))
        log_level = log_level_trace;
    
    tick                = 0;
    xmlDoc              = NULL;
    classes             = NULL;
//    modules             = NULL;
    connections         = NULL;
    module_count        = 0;
    period_count        = 0;
    phase_count         = 0;
    end_of_file_reached = false;
    fatal_error_occurred	= false;
    terminate			= false;
    sizeChangeFlag      = false;
//    threadGroups        = NULL;
    
    logfile     = NULL;
    timer		= new Timer();
    
    // Compute ikaros root path
    
    ikaros_dir = NULL;
    if(is_absolute_path(IKAROSPATH))
        ikaros_dir = create_string(IKAROSPATH);
    else if(options->GetBinaryDirectory() != NULL)
        ikaros_dir = create_formatted_string("%s%s", options->GetBinaryDirectory(), IKAROSPATH);
    else
        Notify(msg_fatal_error, "The Ikaros root directory could not be established. Please set an absolute IKAROSPATH in IKAROS_System.h\n");
    
    // Compute ikc path and name
    
    ikc_dir = options->GetFileDirectory();
    ikc_file_name =  options->GetFileName();
	
	// Seed random number generator
	
	if(options->GetOption('z'))
        srandom(string_to_int(options->GetArgument('z')));
}

Kernel::~Kernel()
{
    Notify(msg_debug, "Deleting Kernel.\n");
    Notify(msg_debug, "  Deleting Connections.\n");
    delete connections;
    Notify(msg_debug, "  Deleting Modules.\n");
//    delete modules; // FIXME: delete
    Notify(msg_debug, "  Deleting Thread Groups.\n");
//    delete threadGroups;  // FIXME: delete
    Notify(msg_debug, "  Deleting Classes.\n");
    delete classes;
    
    delete timer;
    delete xmlDoc;
    delete ikaros_dir;
    
    Notify(msg_debug, "Deleting Kernel Complete.\n");
    if (logfile) fclose(logfile);
    
#ifdef USE_MALLOC_DEBUG
    dump_memory();  // dump blocks that are still allocated
#endif
}


std::string
Kernel::JSONString()
{
    if(main_group)
        return main_group->JSONString();
    else
        return "{}";
}



void
Kernel::AddClass(const char * name, ModuleCreator mc, const char * path)
{
    if (path == NULL)
    {
        Notify(msg_warning, "Path to ikc file is missing for class \"%s\".\n", name);
        classes = new ModuleClass(name, mc, NULL, classes); // Add class anyway
        return;
    }
    
    char * path_to_ikc_file = NULL;
    
    // Test for backward compatibility and remove initial paths if needed
    
    if(ikaros_dir)
        path_to_ikc_file = create_formatted_string("%s%s%s.ikc", ikaros_dir, path, name); // absolute path
    else
        path_to_ikc_file = create_formatted_string("%s%s.ikc", path, name); // relative path

    classes = new ModuleClass(name, mc, path_to_ikc_file, classes);
    destroy_string(path_to_ikc_file);
}

bool
Kernel::Terminate()
{
    if (max_ticks > 0)
    {
        const int segments = 50;
        int lp = int(100*float(tick-1)/float(max_ticks));
        int percent = int(100*float(tick)/float(max_ticks));
         if(tick > 0 && percent != lp)
        {
            int p = (segments*percent)/100;
            printf("  Progress: [");
            for(int i=0; i<segments; i++)
                if(i < p)
                    printf("=");
                else
                    printf(" ");
            printf("] %3d%%\r", percent);
            fflush(stdout);
            if(tick == max_ticks)
                printf("\n");
        }
    }
    
    if (max_ticks != -1 && tick >= max_ticks)
    {
        Notify(msg_debug, "Max ticks reached.\n");
        return true;
    }
    return end_of_file_reached || fatal_error_occurred  || global_fatal_error || terminate || global_terminate;
}

void
Kernel::Run()
{
    if (fatal_error_occurred || global_fatal_error)
    {
        Notify(msg_fatal_error, "Terminating because a fatal error occurred.\n");
        throw 4;
    }
    if (max_ticks == 0)
        return;
    
    Notify(msg_print, "Start\n");
    
	// Synchronize with master process if one is indicated in the IKC file
	if(xmlDoc)
	{
        const char * ip = GetXMLAttribute(xmlDoc->xml, "masterip");
        if(ip)
        {
            Socket s;
            char rr[100];
            int port = string_to_int(GetXMLAttribute(xmlDoc->xml, "masterport"), 9000);
            printf("Waiting for master: %s:%d\n", ip, port);
            fflush(stdout);
            if(!s.Get(ip, port, "*", rr, 100))
            {
                printf("Master not running.\n");
                exit(-1); // No master
            }
        }
	}
    
    timer->Restart();    
    while (!Terminate())
    {
        Tick();
        if (tick_length > 0)
        {
            float lag = timer->WaitUntil(float(tick*tick_length));
            if (lag > 0.1) Notify(msg_warning, "Lagging %.2f ms at tick = %ld\n", lag, tick);
        }
    }
}

void
Kernel::PrintTiming()
{
    total_time = timer->GetTime()/1000; // in seconds
    if (max_ticks != 0)
        Notify(msg_print, "Stop (%ld ticks, %.2f s, %.2f ticks/s, %.3f s/tick)\n", tick, total_time, float(tick)/total_time, total_time/float(tick));
}

void
Kernel::Propagate()
{
    for (Connection * c = connections; c != NULL; c = c->next)
        c->Propagate(tick);
}

void
Kernel::CheckNAN()
{
    for (Module * & m : _modules)
    {
        for (Module_IO * i = m->output_list; i != NULL; i = i->next)
        {
            for(int j=0; j<i->sizex*i->sizey; j++)
            {
                float v = i->matrix[0][0][j];
                if((v) != (v))
                {
                    Notify(msg_fatal_error, "NAN in output \"%s\" of module \"%s\" (%s).\n", i->name.c_str(), i->module->instance_name, i->module->GetClassName());
                    break;
                }
            }
        }
    }
}

void
Kernel::CheckInputs()
{
    for (Module * & m : _modules)
    {
        for (Module_IO * i = m->input_list; i != NULL; i = i->next)
            if (i->size == unknown_size)
            {
                // Check if connected
                bool connected = false;
                for (Connection * c = connections; c != NULL; c = c->next)
                    if (c->target_io == i)
                        connected = true;
                if (connected)
                {
                    Notify(msg_fatal_error, "Size of input \"%s\" of module \"%s\" (%s) could not be resolved.\n", i->name.c_str(), i->module->instance_name, i->module->GetClassName());
                }
                else
                    i->size = 0; // ok if not connected
            }
    }
}

void
Kernel::CheckOutputs()
{
    for (Module * & m : _modules)
    {
        for (Module_IO * i = m->output_list; i != NULL; i = i->next)
            if (i->size == unknown_size)
            {
                // Check if connected
                bool connected = false;
                for (Connection * c = connections; c != NULL; c = c->next)
                    if (c->source_io == i)
                        connected = true;
                if (connected)
                {
                    Notify(msg_fatal_error, "Size of output \"%s\" of module \"%s\" (%s) could not be resolved.\n", i->name.c_str(), i->module->instance_name, i->module->GetClassName());
                }
            }
    }
}

void
Kernel::InitInputs()
{
    for (Connection * c = connections; c != NULL; c = c->next)
    {
        if (c->source_io->size == unknown_size)
        {
            Notify(msg_fatal_error, "Output \"%s\" of module \"%s\" (%s) has unknown size.\n", c->source_io->name.c_str(), c->source_io->module->instance_name, c->source_io->module->GetClassName());
        }
        else if(c->target_io->data != NULL && c->target_io->allow_multiple == false)
        {
            Notify(msg_fatal_error, "Input \"%s\" of module \"%s\" (%s) does not allow multiple connections.\n", c->target_io->name.c_str(), c->target_io->module->instance_name, c->target_io->module->GetClassName());
        }
        else if (c->delay == 0)
        {
            Notify(msg_debug, "Short-circuiting zero-delay connection from \"%s\" of module \"%s\" (%s)\n", c->source_io->name.c_str(), c->source_io->module->instance_name, c->source_io->module->GetClassName());
            // already connected to 0 or longer delay?
            if (c->target_io->data != NULL)
            {
                Notify(msg_fatal_error, "Failed to connect zero-delay connection from \"%s\" of module \"%s\" (%s) because target is already connected.\n", c->source_io->name.c_str(), c->source_io->module->instance_name, c->source_io->module->GetClassName());
            }
            else
            {
                c->target_io->data = new float * [1];
                c->target_io->data[0] = c->source_io->data[0];
                c->target_io->matrix = new float ** [1];
                c->target_io->matrix[0] = c->source_io->matrix[0];
                c->target_io->sizex = c->source_io->sizex;
                c->target_io->sizey = c->source_io->sizey;
                c->target_io->size = c->source_io->size;
                c->target_io->max_delay = 0;
            }
        }
        else if(c->target_io && c->size == unknown_size)
        {
            // Check that this connection does not interfere with zero-delay connection
            if (c->target_io->max_delay == 0)
            {
                Notify(msg_fatal_error, "Failed to connect from \"%s\" of module \"%s\" (%s) because target is already connected with zero-delay.\n", c->source_io->name.c_str(), c->source_io->module->instance_name, c->source_io->module->GetClassName());
            }
            // First connection to this target: initialize
            if (c->target_io->size == unknown_size)	// start calculation with size 0
                c->target_io->size = 0;
            int target_offset = c->target_io->size;
            c->target_io->size += c->source_io->size;
            // Target not used previously: ok to connect anything
            if (c->target_io->sizex == unknown_size)
            {
                Notify(msg_debug, "New connection.\n");
                c->target_io->sizex = c->source_io->sizex;
                c->target_io->sizey = c->source_io->sizey;
            }
            // Connect one dimensional output
            else if (c->target_io->sizey == 1 && c->source_io->sizey == 1)
            {
                Notify(msg_debug, "Adding additional connection.\n");
                c->target_io->sizex = c->target_io->size;
            }
            // Collapse matrix to array
            else
            {
                Notify(msg_debug, "Multiple connections to \"%s.%s\" with different no of rows. Input flattened.\n", c->target_io->module->instance_name, c->target_io->name.c_str());
                c->target_io->sizex = c->target_io->size;
                c->target_io->sizey = 1;
            }
            // Set connection variables
            c->target_offset = target_offset;
            c->size = c->source_io->size;
            // Allocate input memory and reset
            Notify(msg_debug, "Allocating memory for input \"%s\" of module \"%s\" with size %d (%dx%d).\n", c->target_io->name.c_str(), c->target_io->module->instance_name, c->target_io->size, c->target_io->sizex, c->target_io->sizey);
            c->target_io->SetSize(c->target_io->sizex, c->target_io->sizey);
            c->target_io->Allocate();
        }
        else if(c->target_io) // fixed offset connection
        {
            Notify(msg_debug, "Adding fixed offset connection.\n");
            // Check that this connection does not interfere with zero-delay connection
            if (c->target_io->max_delay == 0)
            {
                Notify(msg_fatal_error, "Failed to connect from \"%s\" of module \"%s\" (%s) because target is already connected with zero-delay.\n", c->source_io->name.c_str(), c->source_io->module->instance_name, c->source_io->module->GetClassName());
            }
            // First connection to this target: initialize
            if (c->target_io->size == unknown_size)    // start calculation with size 0
                c->target_io->size = max(c->target_io->size, c->target_offset+c->size);

            // Target not used previously: ok to connect anything
            if (c->target_io->sizex == unknown_size)
            {
                Notify(msg_debug, "New connection.\n");
                c->target_io->sizex = c->target_io->size;
                c->target_io->sizey = 1;
            }
            // Connect one dimensional output
            else if (c->target_io->sizey == 1 && c->source_io->sizey == 1)
            {
                Notify(msg_debug, "Adding additional connection.\n");
                c->target_io->sizex = max(c->target_io->sizex, c->target_offset+c->size);
                c->target_io->size = c->target_io->sizex; // FIXME: Test if this breaks something
            }
            // Collapse matrix to array
            else
            {
                Notify(msg_debug, "Multiple connections to \"%s.%s\" with different no of rows. Input flattened.\n", c->target_io->module->instance_name, c->target_io->name.c_str());
                c->target_io->sizex = c->target_io->size;
                c->target_io->sizey = 1;
            }
            // Set connection variables
            // Allocate input memory and reset
            Notify(msg_debug, "Allocating memory for input \"%s\" of module \"%s\" with size %d (%dx%d).\n", c->target_io->name.c_str(), c->target_io->module->instance_name, c->target_io->size, c->target_io->sizex, c->target_io->sizey);
            c->target_io->SetSize(c->target_io->sizex, c->target_io->sizey);
            c->target_io->Allocate();
        }
    }
}

void
Kernel::InitOutputs()
{
    do
    {
        sizeChangeFlag = false;
        for (Module * & m : _modules)
            m->SetSizes();
        if (sizeChangeFlag)
            Notify(msg_debug, "InitOutput: Iteration with changes\n");
        else
            Notify(msg_debug, "InitOutput: Iteration with no changes\n");
    }
    while (sizeChangeFlag);
}

void
Kernel::AllocateOutputs()
{
    for (Module * & m : _modules)
        m->AllocateOutputs();
}

void
Kernel::InitModules()
{
    for (Module * & m : _modules)
    {
        m->Bind(m->log_level, "log_level");
        m->Init();
    }
}

void
Kernel::NotifySizeChange()
{
    sizeChangeFlag = true;
}

void
Kernel::Init()
{
    // Get system statistics // FIXME: move this to somewhere else
    cpu_cores = std::thread::hardware_concurrency();

    if (options->GetFilePath())
        ReadXML();
    else
        Notify(msg_warning, "No IKC file supplied.\n");
    
    if(fatal_error_occurred)
        return;

    // Fill data structures
 
    for (Module * & m : _modules)
       module_map.insert({ m->GetFullName(), m });

    for (Connection * c = connections; c != NULL; c = c->next)
    {
        module_map[c->source_io->module->GetFullName()]->outgoing_connection.insert(c->target_io->module->GetFullName());
        if(c->delay == 0)
        {
            module_map[c->source_io->module->GetFullName()]->connects_to_with_zero_delay.push_back(c->target_io->module);
            module_map[c->target_io->module->GetFullName()]->connects_from_with_zero_delay.push_back(c->source_io->module);
        }
    }

    printf("MODULES:\n");
    for(const auto& pair : module_map)
        printf("%s\n", pair.first.c_str());

    printf("CONNECTIONS:\n");
    for(const auto& pair : module_map)
        for(const auto& s : pair.second->outgoing_connection)
            printf("%s -> %s\n", pair.first.c_str(), s.c_str());

    printf("\n\n");

    SortModules();

    if(fatal_error_occurred)
        return;

    CalculateDelays();
    InitOutputs();      // Calculate the output sizes for outputs that have not been specified at creation
    AllocateOutputs();
    InitInputs();		// Calculate the input sizes and allocate memory for the inputs or connect 0-delays
    CheckOutputs();
    CheckInputs();
    if (fatal_error_occurred)
        return;
    InitModules();
}

void
Kernel::Tick()
{
    Notify(msg_debug, "Kernel::Tick()\n");
    Propagate();
    DelayOutputs();
  
    if (useThreads)
    {
        for (auto & g : _threadGroups)
            g->Start(tick);
        for (auto & g : _threadGroups)
            g->Stop(tick);
    }
    else if (log_level < log_level_debug)
    {
        for (auto & m : _modules)
            if (tick % m->period == m->phase)
            {
                m->timer->Restart();
                if(m->active)
                    m->Tick();
                m->time += m->timer->GetTime();
                m->ticks += 1;
            }
    }
    else
    {
        for (auto & m : _modules)
            if (tick % m->period == m->phase)
            {
                m->timer->Restart();
                if(m->active)
                {
                    Notify(msg_debug, "%s::Tick (%s) Start\n", m->GetName(), m->GetClassName());
                    m->Tick();
                    Notify(msg_debug, "%s::Tick (%s) End\n", m->GetName(), m->GetClassName());
                }
                m->time += m->timer->GetTime();
                m->ticks += 1;
            }
        
    }
 
    if(nan_checks)
        CheckNAN();
    
    tick++;
}



void
Kernel::Store()
{
    if(!options->GetOption('S'))
        return;
    
    char * p = options->GetArgument('S');
    char * s = p;
    
    if(p == NULL)
        s = ikc_dir;
    else if(p[0] != '/') // not absolute path
        s = create_formatted_string("%s%s", ikc_dir, p);

    for (Module * & m : _modules)
    {
        char * sp = create_formatted_string("%s%s", ikc_dir, m->GetFullName());
        m->Store(sp);
        destroy_string(sp);
    }
}



void
Kernel::Load()
{
    if(!options->GetOption('L'))
        return;
    
    char * p = options->GetArgument('L');
    char * s = p;
    
    if(p == NULL)
        s = ikc_dir;
    else if(p[0] != '/') // not absolute path
        s = create_formatted_string("%s%s", ikc_dir, p);
    
    for(Module * & m : _modules)
    {
        char * sp = create_formatted_string("%s%s", ikc_dir, m->GetFullName());
        m->Load(sp);
        destroy_string(sp);
    }
}




void
Kernel::DelayOutputs()
{
    for (Module * & m : _modules)
        m->DelayOutputs();
}



void
Kernel::AddModule(Module * m)
{
    if (!m) return;
//    m->next = modules;
//    modules = m;
    m->kernel = this;

    // 2.0

    _modules.push_back(m);
}



Module *
Kernel::GetModule(const char * n)
{
    for (Module * & m : _modules)
        if (equal_strings(n, m->instance_name))
            return m;
    return NULL;
}



Module *
Kernel::GetModuleFromFullName(const char * n)
{
    for (Module * & m : _modules)
        if (equal_strings(n, m->full_instance_name))
            return m;
    return NULL;
}



bool // TO BE REMOVED - Used by WebUI
Kernel::GetSource(XMLElement * group, Module * &m, Module_IO * &io, const char * source_module_name, const char * source_name)
{
    for (XMLElement * xml = group->GetContentElement(); xml != NULL; xml = xml->GetNextElement())
    {
//        xml->Print(stdout, 0);
        
        if (xml->IsElement("module") && (equal_strings(GetXMLAttribute(xml, "name"), source_module_name) || equal_strings(source_module_name, "*")))
		{
			m = (Module *)(xml->aux);
			if (m != NULL)
				io = m->GetModule_IO(m->output_list, source_name);
			if (m != NULL && io != NULL)
				return true;
            
        // NEW: Get inputs as well
			if (m != NULL)
				io = m->GetModule_IO(m->input_list, source_name);
			if (m != NULL && io != NULL)
				return true;
        // END NEW
        
			return false;
		}
		else if (xml->IsElement("group") && equal_strings(GetXMLAttribute(xml, "name"), source_module_name)) // Translate output name
		{
			for (XMLElement * output = xml->GetContentElement("output"); output != NULL; output = output->GetNextElement("output"))
			{
				const char * n = GetXMLAttribute(output, "name");
				if (n == NULL)
					return false;
				if (equal_strings(n, source_name) || equal_strings(n, "*"))
				{
					const char * new_module = GetXMLAttribute(output, "sourcemodule");
					const char * new_source = GetXMLAttribute(output, "source");
					if (new_module == NULL)
						new_module = source_module_name;	// retain name
					if (new_source == NULL)
						new_source = source_name;	// retain name
					return GetSource(xml, m, io, new_module, new_source);
				}
			}
            
            // NEW: Get inputs as well
            for (XMLElement * input = xml->GetContentElement("input"); input != NULL; input = input->GetNextElement("input"))
			{
				const char * n = GetXMLAttribute(input, "name");
				if (n == NULL)
					return false;
				if (equal_strings(n, source_name) || equal_strings(n, "*"))
				{
					const char * new_module = GetXMLAttribute(input, "targetmodule");
					const char * new_source = GetXMLAttribute(input, "target");
					if (new_source == NULL)
						new_source = source_name;	// retain name
					return GetSource(xml, m, io, new_module, new_source);
				}
			}
            // NEW END
            
		}
    }
    
    return false;
}


/*
bool // NEW VERSION - do not get input here; but do somewhere else // TODO: distinguish between class and group later
Kernel::GetSource(GroupElement & group, Module * &m, Module_IO * &io, const char * source_module_name, const char * source_name)
{
    for (auto & g : group.groups)
    {
        if(g.second.module && ((g.first==source_module_name) || equal_strings(source_module_name, "*")))
        {
            m = g.second.module;
            io = m->GetModule_IO(m->output_list, source_name);
            if (m != NULL && io != NULL)
                return true;
            return false;
        }
        else if(g.first == source_module_name) // Translate output name - should not be tested again
        {
            for (auto & output : g.second.outputs)
            {
                const char * n = output.first.c_str();
                if (n == NULL)
                    return false;
                if (equal_strings(n, source_name) || equal_strings(n, "*"))
                {
                    const char * new_module = output.second["sourcemodule"].c_str();
                    const char * new_source = output.second["source"].c_str();
                    if (new_module == NULL)
                        new_module = source_module_name;    // retain name
                    if (new_source == NULL)
                        new_source = source_name;    // retain name
                    return GetSource(g.second, m, io, new_module, new_source);
                }
            }
        }
    }
    
    return false;
}




Module_IO *
Kernel::GetSource(GroupElement * group, const std::string & source_module_name, const  std::string & source_name)
{
    Module_IO * io = NULL;
    Module * m = NULL;
    for (auto & g : group->groups)
    {
        if(g.second->module && ((g.first==source_module_name) || (source_module_name=="*")))
        {
            m = g.second->module;
            io = m->GetModule_IO(m->output_list, source_name.c_str());
            if (m != NULL && io != NULL)
                return io;
            return NULL;
        }
        else if(g.first == source_module_name) // Translate output name - should not be tested again
        {
            for (auto & output : g.second->outputs)
            {
                const std::string & n = output.first;
                if (n == "")
                    return NULL;
                if ((n == source_name) || (n == "*"))
                {
                    const std::string & new_module = output.second->GetValue("sourcemodule");
                    const std::string & new_source = output.second->GetValue("source");
                    return GetSource(g.second, new_module!="" ? new_module : source_module_name, new_source!="" ? new_source : source_name );
                }
            }
        }
    }
    return NULL;
}



Module_IO *
Kernel::GetTarget(GroupElement * group, const std::string & target_module_name, const  std::string & target_name)   // GetInputIO = GetModule + GetInputIO
{
    Module_IO * io = NULL;
    Module * m = NULL;
    for (auto & g : group->groups)
    {
        if(g.second->module && ((g.first==target_module_name) || (target_module_name=="*")))
        {
            m = g.second->module;
            io = m->GetModule_IO(m->input_list, target_name.c_str());
            if (m != NULL && io != NULL)
                return io;
            return NULL;
        }
        else if(g.first == target_module_name) // Translate output name - should not be tested again
        {
            for (auto & input : g.second->inputs)
            {
                const std::string & n = input.first;
                if (n == "")
                    return NULL;
                if ((n == target_name) || (n == "*"))
                {
                    const std::string & new_module = input.second->GetValue("targetmodule");
                    const std::string & new_source = input.second->GetValue("target");
                    return GetTarget(g.second, new_module!="" ? new_module : target_module_name, new_source!="" ? new_source : target_name );
                }
            }
        }
    }
    return NULL;
}
*/


static const char *
find_nth_element(const char * s, int n)
{
    if(s==NULL)
        return NULL;
    
    int l = int(strlen(s));
    if(l==0)
        return NULL;
    
    int i=0;
    while(i < l && s[i] <= ' ')
        i++;
    
    for(int c=1; c<=n; c++)
    {
        if(s[i] > ' ' && c==n)
        {
            int j=0;
            while((i+j < l) && (s[i+j] > ' '))
                j++;
            return create_string_head(&s[i], j);
        }
        while(i < l && s[i] > ' ')
            i++;
        while(i < l && s[i] <= ' ')
            i++;
    }
    
    return NULL;
}



const char *
Kernel::GetBatchValue(const char * n)
{
    int rank = string_to_int(options->GetArgument('b'));
    
    XMLElement * xml = xmlDoc->xml->GetElement("group");
    if (xml == NULL)
        return NULL;
    
    for (XMLElement * xml_node = xml->GetContentElement(); xml_node != NULL; xml_node = xml_node->GetNextElement("batch"))
        if(equal_strings(xml_node->GetAttribute("target"), n))
        {
            if(rank == 0)
                rank = string_to_int(GetXMLAttribute(xml_node, "rank"));
                
            if(rank == 0)
                return NULL;
            
            const char * value = find_nth_element(GetXMLAttribute(xml_node, "values"), rank);
            printf("IKAROS: %s = \"%s\"\n", GetXMLAttribute(xml_node, "target"), value);
            return value;
        }
    
    return NULL;
}



const char *
Kernel::GetXMLAttribute(XMLElement * e, const char * attribute)
{
    const char * value = NULL;
    
    while(e != NULL && e->IsElement())
        if((value = e->GetAttribute(attribute)))
             return value;
        else
            e = (XMLElement *)(e->parent);

    if((value = GetBatchValue(attribute)))
        return value;
    
    if((value = options->GetValue(attribute)))
        return value;
    
    return NULL;
}



bool
Kernel::GetBinding(Module * &m, int &type, void * &value_ptr, int & sx, int & sy, const char * source_module_name, const char * source_name)
{
    m = GetModule(source_module_name);
    if(!m)
    {
        Notify(msg_warning, "Could not find binding. Module \"%s\" does not exist\n", source_module_name);
        return false;
    }
    for(Binding * b = m->bindings; b != NULL; b = b->next)
        if(equal_strings(source_name, b->name))
        {
            type = b->type;
            value_ptr = b->value;
            sx = b->size_x;
            sy = b->size_y;
            return true;
        }
    
    Notify(msg_warning, "Could not find binding. Binding \"%s\" of module \"%s\" does not exist\n", source_name, source_module_name);
    return false;
}


static char wildcard[2] = "*";


// Find FIRST binding (ignore the rest)

bool
Kernel::GetBinding(XMLElement * group, Module * &m, int &type, void * &value_ptr, int & sx, int & sy, const char * group_name, const char * parameter_name)
{
    for (XMLElement * xml = group->GetContentElement(); xml != NULL; xml = xml->GetNextElement())
        if (xml->IsElement("module") && (!GetXMLAttribute(xml, "name") || equal_strings(GetXMLAttribute(xml, "name"), group_name) || equal_strings(group_name, "*")))
		{
			m = (Module *)(xml->aux);
            
			if(m == NULL)
                return false;
            
            for(Binding * b = m->bindings; b != NULL; b = b->next)
                if(equal_strings(parameter_name, b->name))
                {
                    type = b->type;
                    value_ptr = b->value;
                    sx = b->size_x;
                    sy = b->size_y;
                    return true;
                }
            
			return false;
		}
		else if (xml->IsElement("group") && (equal_strings(GetXMLAttribute(xml, "name"), group_name) || equal_strings(group_name, "*"))) // Translate output name
		{
            const char * new_module = wildcard;
            const char * new_parameter = parameter_name;

			for (XMLElement * parameter = xml->GetContentElement("parameter"); parameter != NULL; parameter = parameter->GetNextElement("parameter"))
			{
				const char * n =GetXMLAttribute(parameter, "name");
                
				if (n!=NULL && (equal_strings(n, parameter_name) || equal_strings(n, "*")))
				{
 					new_module = GetXMLAttribute(parameter, "targetmodule");
					new_parameter = GetXMLAttribute(parameter, "target");
                    
					if (new_module == NULL)
						new_module = wildcard;	// match all modules
                    
					if (new_parameter == NULL)
						new_parameter = parameter_name;	// retain name
				}
                
                if(equal_strings("interval", n))
                {
//                    printf("STOP\n");
                }
			}

            return GetBinding(xml, m, type, value_ptr, sx, sy, new_module, new_parameter);
		}

    return false;
}


void
Module::SetParameter(const char * parameter_name, int x, int y, float value)
{
     for(Binding * b = bindings; b != NULL; b = b->next)
        if(equal_strings(parameter_name, b->name))
        {
            if(b->type == bind_float)
                *((float *)(b->value)) = value;
            else if(b->type == bind_int || b->type == bind_list)
                *((int *)(b->value)) = (int)value;
            else if(b->type == bind_bool)
                *((bool *)(b->value)) = (value > 0);
            else if(b->type == bind_array)
                ((float *)(b->value))[x] = value;     // TODO: add range check!!!
            else if(b->type == bind_matrix)
               ((float **)(b->value))[y][x] = value;
        }
}



void
Kernel::SetParameter(XMLElement * group, const char * group_name, const char * parameter_name, int select_x, int select_y, float value)
{
    for (XMLElement * xml = group->GetContentElement(); xml != NULL; xml = xml->GetNextElement())
    {
        // Set parameters of modules in this group
    
       if (xml->IsElement("module") && (!GetXMLAttribute(xml, "name") || equal_strings(GetXMLAttribute(xml, "name"), group_name) || equal_strings(group_name, "*")))
		{
			Module * m = (Module *)(xml->aux);
			if(m != NULL)
                m->SetParameter(parameter_name, select_x, select_y, value);
 		}

        // Set parameters in included groups
        
		else if (xml->IsElement("group") && (equal_strings(GetXMLAttribute(xml, "name"), group_name) || equal_strings(group_name, "*"))) // Translate output name
		{
            const char * new_module = wildcard;
            const char * new_parameter = parameter_name;

			for (XMLElement * parameter = xml->GetContentElement("parameter"); parameter != NULL; parameter = parameter->GetNextElement("parameter"))
			{
				const char * n = GetXMLAttribute(parameter, "name");
                
				if (n!=NULL && (equal_strings(n, parameter_name) || equal_strings(n, "*")))
				{
 					new_module = GetXMLAttribute(parameter, "targetmodule");
					new_parameter = GetXMLAttribute(parameter, "target");
                    
					if (new_module == NULL)
						new_module = wildcard;	// match all modules
                    
					if (new_parameter == NULL)
						new_parameter = parameter_name;	// retain name
				}
			}
            
            SetParameter(xml, new_module, new_parameter, select_x, select_y, value);
		}
    }
}



void
Kernel::SendCommand(XMLElement * group, const char * group_name, const char * command_name, int x, int y, std::string value)
{
    for (XMLElement * xml = group->GetContentElement(); xml != NULL; xml = xml->GetNextElement())
    {
        // Set parameters of modules in this group
    
       if (xml->IsElement("module") && (!GetXMLAttribute(xml, "name") || equal_strings(GetXMLAttribute(xml, "name"), group_name) || equal_strings(group_name, "*")))
        {
            Module * m = (Module *)(xml->aux);
            if(m != NULL)
                m->Command(command_name, x, y, value);
         }

        // Set parameters in included groups
        
        else if (xml->IsElement("group") && (equal_strings(GetXMLAttribute(xml, "name"), group_name) || equal_strings(group_name, "*")))
        {
            const char * new_module = wildcard;
            SendCommand(xml, new_module, command_name, x, y, value);
        }
    }
}


int
Kernel::CalculateInputSize(Module_IO * i)
{
    // The input size has not yet been determined; use the connection list to calculate it
    // Scan through the connection list to find the size of the input
    // The size is the sum of the sizes of all outputs connected to this input
    // The input size is unspecified if one of the outputs has unknown size
    int s = 0;
    for (Connection * c = connections; c != NULL; c = c->next)
    {
        if (c->target_io == i)
        {
            if (c->source_io->size == unknown_size)
                return unknown_size;
            else
                s += c->source_io->size;
        }
    }
    return s;
}

int
Kernel::CalculateInputSizeX(Module_IO * i)
{
    // The input size has not yet been determined; use the connection list to calculate it
    // Scan through the connection list to find the size of the input
    // For matrices, there can only be one input
    // The input size is unspecified if one of the outputs has unknown size
    int s = 0;
    for (Connection * c = connections; c != NULL; c = c->next)
    {
        if (c->target_io == i)
        {
            if (c->source_io->sizex == unknown_size)
                return unknown_size;
            else if(c->target_offset>0 && c->size>0) // offset connection
                s = max(s, c->target_offset + c->size);
            else if (s == 0)
                s =  c->source_io->sizex;
            else
                return CalculateInputSize(i);
        }
    }
    return s;
}

int
Kernel::CalculateInputSizeY(Module_IO * i)
{
    // The input size has not yet been determined; use the connection list to calculate it
    // Scan through the connection list to find the size of the input
    // For matrices, there can only be one input
    // The input size is unspecified if one of the outputs has unknown size
    int s = 0;
    for (Connection * c = connections; c != NULL; c = c->next)
    {
        if (c->target_io == i)
        {
            if (c->source_io->sizey == unknown_size)
                return unknown_size;
            else if (s == 0)
                s =  c->source_io->sizey;
            else
                return 1;
        }
    }
    return s;
}



/*
    Partition Graph:

    For all unassigned modules:
    1. Recursively mark all connected modules
    2. Assign all marked modules to new group
 
*/

void
Kernel::MarkSubgraph(Module * m)  // recursively mark all connected modules
{
    m->mark = 3;
    for(auto & n : m->connects_to_with_zero_delay)
        if(n->mark != 3)
            MarkSubgraph(n);
    for(auto & n : m->connects_from_with_zero_delay)
        if(n->mark != 3)
            MarkSubgraph(n);
}



void
Kernel::CreateThreadGroups(std::deque<Module *> & sorted_modules)   // FIXME: possibly change to use emplace and no new
{
    for(auto & m : sorted_modules)
        if(m->mark < 3)
        {
            ThreadGroup * tg = new ThreadGroup(this, m->period, m->phase);
            _threadGroups.push_back(tg);
            MarkSubgraph(m);
            for(auto & m :sorted_modules)
                if(m->mark == 3)
                {
                    if(tg->period != m->period || tg->phase != m->phase)
                    {
                        Notify(msg_fatal_error, "Module period and phase does not match rest of subgraph (%s).", m->GetFullName());
                        return;
                    }
                    tg->_modules.push_back(m);
                    m->mark = 4;
                }
        }
}



/*

Topological Sort:

L ← Empty list that will contain the sorted nodes
while there are unmarked nodes do
    select an unmarked node n
    visit(n)

 function visit(node n)
    if n has a permanent mark (2) then return
    if n has a temporary mark (1) then stop   (not a DAG)
    mark n temporarily
    for each node m with an edge from n to m do
        visit(m)
    mark n permanently (2)
    add n to head of L

*/



bool
Kernel::Visit(std::deque<Module *> & sorted_modules, Module * n)
{
    if(n->mark == 2)
        return true;

    if(n->mark == 1)
    {
        Notify(msg_fatal_error, "Network contains zero-connection loop at %s", n->GetFullName());
        return false;
    }

    n->mark = 1;
    for(auto & m : n->connects_to_with_zero_delay)
        if(!Visit(sorted_modules, m))
            return false;
    
    n->mark = 2;
    sorted_modules.push_front(n);
    return true;
}



void
Kernel::SortModules()
{
    std::deque<Module *> sorted_modules;
    for(auto & m : _modules)
        if(!m->mark && !Visit(sorted_modules, m))
            return; // fail

    CreateThreadGroups(sorted_modules);
}



void
Kernel::CalculateDelays()
{
    for (Connection * c = connections; c != NULL; c = c->next)
    {
        if (c->delay > c->source_io->max_delay)
            c->source_io->max_delay = c->delay;
    }
}


// FIXME: move all list functions to end

void
Kernel::ListInfo()
{
    if (!options->GetOption('i') && !options->GetOption('a')) return;
    Notify(msg_print, "\n");
    Notify(msg_print, "Ikaros version %s\n", VERSION);
    Notify(msg_print, "\n");
    Notify(msg_print, "%s\n", PLATFORM);
#ifdef POSIX
    Notify(msg_print, "POSIX\n");
#endif
#ifdef USE_BSD_SOCKET
    Notify(msg_print, "BSD-socket\n");
#endif
#ifdef USE_LIBJPEG
    Notify(msg_print, "libjpeg\n");
#endif
#ifdef USE_THREADS
    Notify(msg_print, "threads\n");
#endif
#ifdef USE_BLAS
    Notify(msg_print, "BLAS\n");
#endif
#ifdef USE_QUICKTIME
    Notify(msg_print, "Quicktime\n");
#endif
#ifdef USE_VIMAGE
    Notify(msg_print, "vImage\n");
#endif
#ifdef USE_VFORCE
    Notify(msg_print, "vForce\n");
#endif
#ifdef USE_VDSP
    Notify(msg_print, "vDSP\n");
#endif
#ifdef USE_MPI
    Notify(msg_print, "MPI\n");
#endif
    Notify(msg_print, "\n");
    Notify(msg_print, "ikc file name: %s\n", ikc_file_name);
    Notify(msg_print, "ikc file directory: %s\n", ikc_dir);
    Notify(msg_print, "ikaros root directory: %s\n", ikaros_dir);
}



void
Kernel::ListModulesAndConnections()
{
    if (!options->GetOption('m') && !options->GetOption('a')) return;
    
    if(_modules.empty())
    {
        Notify(msg_print, "\n");
        Notify(msg_print, "No Modules.\n");
        Notify(msg_print, "\n");
        return;
    }
    
    Notify(msg_print, "\n");
    Notify(msg_print, "Modules:\n");
    Notify(msg_print, "\n");
    for (Module * & m : _modules)
    {
        //		Notify(msg_print, "  %s (%s) [%d, %d]:\n", m->name, m->class_name, m->period, m->phase);
        Notify(msg_print, "  %s (%s) [%d, %d, %s]:\n", m->GetFullName(), m->class_name, m->period, m->phase, m->active ? "active" : "inactive");
        for (Module_IO * i = m->input_list; i != NULL; i = i->next)
            if(i->data)
                Notify(msg_print, "    %-10s\t(Input) \t%6d%6d\t%12p\n", i->name.c_str(), i->sizex, i->sizey, (i->data == NULL ? NULL : i->data[0]));
            else
                Notify(msg_print, "    %-10s\t(Input) \t           no connection\n", i->name.c_str());
        for (Module_IO * i = m->output_list; i != NULL; i = i->next)
            Notify(msg_print, "    %-10s\t(Output)\t%6d%6d\t%12p\t(%d)\n", i->name.c_str(), i->sizex, i->sizey, (i->data == NULL ? NULL : i->data[0]), i->max_delay);
        Notify(msg_print, "\n");
    }
    Notify(msg_print, "Connections:\n");
    Notify(msg_print, "\n");
    for (Connection * c = connections; c != NULL; c = c->next)
        if (c->delay == 0)
            Notify(msg_print, "  %s.%s[%d..%d] == %s.%s[%d..%d] (%d) %s\n",
                   c->source_io->module->instance_name, c->source_io->name.c_str(), 0, c->source_io->size-1,
                   c->target_io->module->instance_name, c->target_io->name.c_str(), 0, c->source_io->size-1,
                   c->delay,
                   c->active ? "" : "[inactive]");
        else if (c->size > 1)
            Notify(msg_print, "  %s.%s[%d..%d] -> %s.%s[%d..%d] (%d) %s\n",
                   c->source_io->module->instance_name, c->source_io->name.c_str(), c->source_offset, c->source_offset+c->size-1,
                   c->target_io->module->instance_name, c->target_io->name.c_str(), c->target_offset, c->target_offset+c->size-1,
                   c->delay,
                   c->active ? "" : "[inactive]");
        else
            Notify(msg_print, "  %s.%s[%d] -> %s.%s[%d] (%d) %s\n",
                   c->source_io->module->instance_name, c->source_io->name.c_str(), c->source_offset,
                   c->target_io->module->instance_name, c->target_io->name.c_str(), c->target_offset,
                   c->delay,
                   c->active ? "" : "[inactive]");
    Notify(msg_print, "\n");
}

void
Kernel::ListThreads()
{
    if (!options->GetOption('T') && !(options->GetOption('a') && options->GetOption('t'))) return;
    Notify(msg_print, "\n");
    Notify(msg_print,"ThreadManagers:\n");
    Notify(msg_print, "\n");
    int i=0;
    for(ThreadGroup * tg : _threadGroups)
    {
        Notify(msg_print, "ThreadManager %d [Period:%d, Phase:%d]\n", i++, tg->period, tg->phase);
        for(Module * m : tg->_modules)
            Notify(msg_print, "\t Module: %s\n", m->GetFullName());
        i++;
    }
    Notify(msg_print, "\n");
}

void
Kernel::ListWarningsAndErrors()
{
    if(global_warning_count > 1)
        printf("IKAROS: %d WARNINGS.\n", global_warning_count);
    else if(global_warning_count == 1)
        printf("IKAROS: %d WARNING.\n", global_warning_count);
    
    if(global_error_count > 1)
        printf("IKAROS: %d ERRORS.\n", global_error_count);
    else if(global_error_count == 1)
        printf("IKAROS: %d ERROR.\n", global_error_count);
}

void
Kernel::ListScheduling()
{
    if (!options->GetOption('l') && !options->GetOption('a')) return;

    if(_modules.empty())
        return;
    
    Notify(msg_print, "Scheduling:\n");
    Notify(msg_print, "\n");
    for (int t=0; t<period_count; t++)
    {
        int tm = 0;
        for (Module * & m : _modules)
            if (t % m->period == m->phase)
                Notify(msg_print,"  %02d.%02d: %s (%s)\n", t, tm++, m->GetName(), m->GetClassName());
    }
    
    Notify(msg_print, "\n");
    Notify(msg_print, "\n");
}

void
Kernel::ListClasses()
{
    if (!options->GetOption('c') && !options->GetOption('a')) return;
    int i = 0;
    Notify(msg_print, "\n");
    Notify(msg_print, "Classes:\n");
    for (ModuleClass * c = classes; c != NULL; c = c->next)
    {
        Notify(msg_print, "\t%s\n", c->name);
        i++;
    }
    Notify(msg_print, "No of classes: %d.\n", i);
}

void
Kernel::ListProfiling()
{
    if (!options->GetOption('p')) return;
    // Calculate Total Time
    float total_module_time = 0;
    for (Module * & m : _modules)
        total_module_time += m->time;
    Notify(msg_print, "\n");
    Notify(msg_print, "Time (ms):\n");
    Notify(msg_print, "-------------------\n");
    Notify(msg_print, "Modules: %10.2f\n", total_module_time);
    Notify(msg_print, "Other:   %10.2f\n", 1000*total_time-total_module_time);
    Notify(msg_print, "-------------------\n");
    Notify(msg_print, "Total:   %10.2f\n", 1000*total_time);
    Notify(msg_print, "\n");
    if (total_module_time == 0)
        return;
    Notify(msg_print, "Time in Each Module:\n");
    Notify(msg_print, "\n");
    Notify(msg_print, "%-20s%-20s%10s%10s%10s\n", "Module", "Class", "Count", "Avg (ms)", "Time %");
    Notify(msg_print, "----------------------------------------------------------------------\n");
    for (Module * & m : _modules)
        if (m->ticks > 0)
            Notify(msg_print, "%-20s%-20s%10.0f%10.2f%10.1f\n", m->GetName(), m->GetClassName(), m->ticks, (m->time/m->ticks), 100*(m->time/total_module_time));
        else
            Notify(msg_print, "%-20s%-20s%       ---f\n", m->GetName(), m->GetClassName());
    Notify(msg_print, "----------------------------------------------------------------------\n");
    if (useThreads)
        Notify(msg_print, "Note: Time is real-time, not time in thread.\n");
    Notify(msg_print, "\n");
}

void
Kernel::Notify(int msg, const char * format, ...)
{
    switch (ikaros::abs(msg))
    {
        case msg_fatal_error:
            fatal_error_occurred = true;
            break;
        case msg_end_of_file:
            end_of_file_reached = true;
            break;
        case msg_terminate:
            terminate = true;
            break;
        default:
            break;
    }
    if(msg > log_level)
        return;
    char 	message[512];
    int n = 0;
    switch (ikaros::abs(msg))
    {
        case msg_fatal_error:
            n = snprintf(message, 512, "ERROR: ");
            global_error_count++;
            break;
        case msg_end_of_file:
            n = snprintf(message, 512, "END-OF-FILE: ");
            break;
        case msg_terminate:
            n = snprintf(message, 512, "TERMINATE. ");
            break;
        case msg_warning:
            n = snprintf(message, 512, "WARNING: ");
            global_warning_count++;
            break;
        default:
            break;
    }
    va_list 	args;
    va_start(args, format);
    vsnprintf(&message[n], 512-n, format, args);
    va_end(args);
    printf("IKAROS: %s", message);
    if(message[strlen(message)-1] != '\n')
        printf("\n");
    if (logfile != NULL)
        fprintf(logfile, "%5ld: %s", tick, message);	// Print in both places
}



// Create one or several connections with different delays between two ModuleIOs

int
Kernel::Connect(Module_IO * sio, int s_offset, Module_IO * tio, int t_offset, int size, const std::string & delay, int extra_delay, bool is_active)
{
    int c = 0;
    char * dstring = create_string(delay.c_str());
    
    if(!dstring || (!strchr(dstring, ':') && !strchr(dstring, ',')))
    {
        int d = string_to_int(dstring, 1);
        connections = new Connection(connections, sio, s_offset, tio, t_offset, size, d+extra_delay, is_active);
        c++;
    }
    
    else // parse delay string for multiple delays
    {
		char * d = create_string(dstring);
        char * p = strtok(d, ",");
        while (p != NULL)
        {
            int a, b, n;
            n = sscanf(p, "%d:%d", &a, &b);
            if(n==1)
                b = a;
            
            for(int i=a; i<=b; i++)
            {
                connections = new Connection(connections, sio, s_offset, tio, t_offset, size, i+extra_delay, is_active);
                c++;
            }
            p = strtok(NULL, ",");
        }
		destroy_string(d);
    }
    
    destroy_string(dstring);
    return c;
}



// Check if class file exists and return path if valid

static const char *
file_exists(const char * path)
{
	if (path != NULL)
	{
		FILE * t = fopen(path, "rb");
		bool exists = (t != NULL);
		if (t) fclose(t);
		return (exists ? path : NULL);
	}
    
	return NULL;
}



// Read class file (or included file) and merge with the current XML-tree

XMLElement * 
Kernel::BuildClassGroup(GroupElement & group, XMLElement * xml_node, const char * class_name)
{
	char include_file[PATH_MAX] ="";
	const char * filename = file_exists(append_string(copy_string(include_file, class_name, PATH_MAX), ".ikc", PATH_MAX));
	filename = (filename ? filename : file_exists(classes->GetClassPath(class_name)));
	if(!filename)
	{
		Notify(msg_warning, "Class ikc for \"%s\" could not be found.\n", class_name);
		return xml_node;
	}

	XMLDocument * cDoc = new XMLDocument(filename);
	XMLElement * cgroup = cDoc->xml;
	cDoc->xml = NULL;
	delete cDoc;
	
	// 1. Replace the module element with the group element of the included file
	
	cgroup->next = xml_node->next;
	if (xml_node->prev == NULL)
		xml_node->GetParentElement()->content = cgroup;
	else
		xml_node->prev->next = cgroup;
	cgroup->parent = xml_node->parent;
	
	// 2. Copy attributes
    
	for(XMLNode * n = xml_node->attributes; n != NULL; n=n->next)
		cgroup->SetAttribute(((XMLAttribute *)n)->name, ((XMLAttribute *)n)->value);
    delete xml_node->attributes;
	xml_node->attributes = NULL;
	
	// 3. Copy elements
	
	XMLNode * last = cgroup->content;
	if (last == NULL)
		cgroup->content = xml_node->content;
	else
	{
		while (last->next != NULL)
			last = last->next;
		last->next = xml_node->content;
	}
	xml_node->content = NULL;
	
	// 4. Build the class group
	
	BuildGroup(group, cgroup, class_name);
	
	// 5. Delete original element and replace it with the newly merged group
	
	xml_node->next = NULL;
	delete xml_node;
	
	return cgroup;
}



// Parse XML for a group

static int group_number = 0;

GroupElement *
Kernel::BuildGroup(GroupElement & group, XMLElement * group_xml, const char * current_class)
{
    const char * name = GetXMLAttribute(group_xml, "name");
    if(name == NULL)
        group_xml->SetAttribute("name", create_formatted_string("Group-%d", group_number++));   // TODO: add this kind of thing to unnamed views as well

    // 2.0 add attributes to group element
    
    for(XMLAttribute * attr=group_xml->attributes; attr!=NULL; attr = (XMLAttribute *)attr->next)
        group.attributes.insert({ attr->name, attr->value });
    
    for (XMLElement * xml_node = group_xml->GetContentElement(); xml_node != NULL; xml_node = xml_node->GetNextElement())
        if (xml_node->IsElement("module"))	// Add module
        {
            char * class_name = create_string(GetXMLAttribute(xml_node, "class"));
            
            if (!equal_strings(class_name, current_class))  // Check that we are not in a class file
            {
                GroupElement * subgroup = new GroupElement();
				xml_node = BuildClassGroup(*subgroup, xml_node, class_name);
                group.groups.insert( { subgroup->GetAttribute("name"), *subgroup });    // FIXME: perfect place to use emplace instead
            }
            
			else if (class_name != NULL)	// Create the module using standard class
			{
				Parameter * parameter = new Parameter(this, xml_node);
				Module * m = CreateModule(classes, class_name, GetXMLAttribute(xml_node, "name"), parameter);
				delete parameter;
				if (m == NULL)
					Notify(msg_warning, "Could not create module: Class \"%s\" does not exist.\n", class_name);
				else if (useThreads && m->phase != 0)
					Notify(msg_fatal_error, "phase != 0 not yet supported in threads.");
				xml_node->aux = (void *)m;
                group.module = m;  // FIXME: test if correct
				AddModule(m);
			}
            else  // TODO: could add check for command line values here
            {
                Notify(msg_warning, "Could not create module: Class name is missing.\n");
            }
            destroy_string(class_name);
        }
        else if (xml_node->IsElement("group"))	// Add group
        {
            GroupElement g;
            group.groups.insert( { xml_node->GetAttribute("name"), *BuildGroup(g, xml_node) });  // FIXME: really ugly
        }
    
        else if (xml_node->IsElement("parameter"))
            group.parameters.insert( { xml_node->GetAttribute("name"), ParameterElement(xml_node) });
    
        else if (xml_node->IsElement("input"))
            group.inputs.push_back(InputElement(xml_node));
            
        else if (xml_node->IsElement("output"))
            group.outputs.insert( { xml_node->GetAttribute("name"), OutputElement(xml_node) });

        else if (xml_node->IsElement("connection"))
            group.connections.push_back(ConnectionElement(xml_node));

        else if (xml_node->IsElement("view"))
        {
            ViewElement v(xml_node);
            for(XMLElement * xml_obj = xml_node->GetContentElement(); xml_obj != NULL; xml_obj = xml_obj->GetNextElement())     // WAS "object"
            {
                ViewObjectElement o(xml_obj);
                o.attributes.insert({ "class", xml_obj->name }); // FIXME: WHY??? Needed for widgets to work for some reason
                v.objects.push_back(o);
            }
            group.views.push_back(v);
        }
    
        return &group;
}



void
Kernel::ConnectModules(GroupElement & group, std::string indent)
{
//    printf("%sConnecting in %s\n", indent.c_str(), group.GetAttribute("name").c_str());

    // Connect in this group
    
    for(auto & c : group.connections)
    {
        std::string sm = c["sourcemodule"];
        std::string tm = c["targetmodule"];
        if(sm != "")
            sm += ".";
        if(tm != "")
            tm += ".";
        auto & source = rsplit(sm+c["source"], ".", 1);  // Merge then split again
        auto & target = rsplit(tm+c["target"], ".", 1);
        std::string source_group = source.size() == 1 ? "" : source[0];
        std::string source_output = source.size() == 1 ? source[0] : source[1]; // tail?
        std::string target_group = target.size() == 1 ? "" : target[0];
        std::string target_input = target.size() == 1 ? target[0] : target[1]; // tail?
        
//        printf("%sConnecting: %s -> %s\n", indent.c_str(), sm.c_str(), tm.c_str());
        
        Module_IO * source_io = NULL;
        if(starts_with(source_group, "."))
            source_io = main_group->GetSource(split(source_group, ".", 1)[1], source_output);
        else
            source_io = group.GetSource(source_group, source_output);
        
        if(!source_io)
            Notify(msg_fatal_error, "Connection source %s not found.\n", (c["sourcemodule"]+":"+c["source"]).c_str());

        int cnt = 0;

        if(starts_with(target_group, "."))
            for(auto target_io : main_group->GetTargets(split(target_group, ".", 1)[1], target_input))
                cnt += Connect(source_io, string_to_int(c["sourceoffset"]), target_io, string_to_int(c["targetoffset"]), string_to_int(c["size"], unknown_size), c["delay"], 0, string_to_bool(c["active"], true));
        else
            for(auto target_io : group.GetTargets(target_group, target_input))
                cnt += Connect(source_io, string_to_int(c["sourceoffset"]), target_io, string_to_int(c["targetoffset"]), string_to_int(c["size"], unknown_size), c["delay"], 0, string_to_bool(c["active"], true));
        
        if(cnt == 0)
            Notify(msg_fatal_error, "Connection target %s not found.\n", (c["targetmodule"]+":"+c["target"]).c_str());
    }

    // Connect in subgroups

    for(auto & g : group.groups)
        ConnectModules(g.second, indent+"\t");
}



void
Kernel::ReadXML()
{
    if (ikc_file_name[0] == 0)
    {
        Notify(msg_fatal_error, "Empty file name.\n");
        return;
    }
    char path[PATH_MAX];
    copy_string(path, ikc_dir, PATH_MAX);
    append_string(path, ikc_file_name, PATH_MAX);
    Notify(msg_print, "Reading XML file \"%s\".\n", ikc_file_name);
    if (chdir(ikc_dir) < 0)
    {
        Notify(msg_fatal_error, "The directory \"%s\" could not be found.\n", ikc_dir);
        return;
    }
    xmlDoc = new XMLDocument(ikc_file_name, false, options->GetOption('X'));
    if (xmlDoc->xml == NULL)
    {
        Notify(msg_fatal_error, "Could not read (or find) \"%s\".\n", ikc_file_name);
        return;
    }
    XMLElement * xml = xmlDoc->xml->GetElement("group");
    if (xml == NULL)
    {
        Notify(msg_fatal_error, "Did not find <group> element in IKC/XML file \"%s\".\n", ikc_file_name);
        return;
    }
    
    // Set default parameters
    
    xml->SetAttribute("log_level", create_formatted_string("%d", log_level)); // FIXME: period???
    
    // Build The Main Group
    // FIXME: what is this?
/*
    if(!xml->GetAttribute("name")) // This test is necessary since we are not alllowed to change a value of an attribute // TODO: SHOULD PROBABLY BE REMOVED
    {
        const char * name = GetXMLAttribute(xml, "name"); // Instantiate name and title from command line options if not set in the file
        if(name)
            xml->SetAttribute("name", name);
    }
    
    if(!xml->GetAttribute("title")) // TODO: SHOULD PROBABLY BE REMOVED
    {
        const char * title = GetXMLAttribute(xml, "title");
        if(title)
            xml->SetAttribute("title", title);
    }
*/

    // 2.0 create top group
    
    main_group = new GroupElement();
    
    // set session id
    
    std::time_t result = std::time(nullptr);
    main_group->attributes.insert({ "session-id", std::to_string(result) });
    session_id = result; // temporary, get from top level group
    
    BuildGroup(*main_group, xml);
    // FIXME: make connections here

    ConnectModules(*main_group);

    if (options->GetOption('x'))
        xmlDoc->Print(stdout);
    
//    main_group->Print();
}


// The following lines will create the kernel the first time it is accessed by on of the modules

Kernel& kernel()
{
    static Kernel * kernelInstance = new Kernel();
    return * kernelInstance;
}


