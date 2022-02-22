//
//	  IKAROS.cc		Kernel code for the IKAROS project
//
//    Copyright (C) 2001-2022  Christian Balkenius
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
// Before 2.0: 4500 lines

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

#include <sys/resource.h>
#include <sys/times.h>

#include "IKAROS.h"
#include "Kernel/IKAROS_ColorTables.h"

using namespace ikaros;

bool        global_fatal_error = false;	// Must be global because it is used before the kernel is created
bool        global_terminate = false;	// Used to flag that CTRL-C has been received
int         global_error_count = 0;
int         global_warning_count = 0;

static std::string empty_string = "";

//#include "IKAROS_Malloc_Debug.h"
#ifndef USING_MALLOC_DEBUG
void* operator new (std::size_t size) noexcept(false)
{
    void *p=calloc(size, sizeof(char)); 
    if(p==0) // did calloc succeed?
        throw std::bad_alloc();
    return p;
}

void operator delete (void *p) throw()
{
    free(p); 
}
#endif


//
// Group (2.0)
//

static int group_number = 0;

class GroupElement;

class Element
{
public:
    GroupElement * parent;
    std::unordered_map<std::string, std::string> attributes;
    
                                    Element(GroupElement * parent, XMLElement * xml_node=NULL);
    const std::string &             GetAttribute(const std::string & a) const; // FIXME: Remove
    virtual std::string             GetValue(const std::string & a) const;
    std::string                     operator[](const std::string & a) const;
    void                            PrintAttributes(int d=0); // const
    std::string                     JSONAttributeString(int d=0); // const
};

class ParameterElement: public Element
{
public:
    ParameterElement(GroupElement * parent, XMLElement * xml_node=NULL);
    
    void Print(int d=0);
    std::string JSONString(int d=0);
};

class InputElement: public Element
{
public:
    InputElement(GroupElement * parent, XMLElement * xml_node=NULL);
    std::string     MapTarget(std::string name);
    void            Print(int d=0);
    std::string     JSONString(int d=0);

};

class OutputElement: public Element
{
public:
    OutputElement(GroupElement * parent, XMLElement * xml_node=NULL);

    std::string     MapSource(std::string name);
    void            Print(int d=0);
    std::string     JSONString(int d=0);
};

class ConnectionElement: public Element
{
public:
    ConnectionElement(GroupElement * parent, XMLElement * xml_node=NULL);
    
    void            Print(int d=0);
    std::string     JSONString(int d=0);
};

class ViewObjectElement: public Element
{
public:
    ViewObjectElement(GroupElement * parent, XMLElement * xml_node=NULL);

    void            Print(int d=0);
    std::string     JSONString(int d=0);

};

class ViewElement: public Element
{
public:
    ViewElement(GroupElement * parent, XMLElement * xml_node=NULL);
    std::vector<ViewObjectElement> objects;

    void            Print(int d=0);
    std::string     JSONString(int d=0);
};

class GroupElement : public Element
{
public:
    std::unordered_map<std::string, GroupElement *> groups;
    std::unordered_map<std::string, ParameterElement *> parameters;
    std::vector<ParameterElement *> parameter_list;
    std::vector<InputElement> inputs;
    std::unordered_map<std::string, OutputElement *> outputs;
    std::vector<ConnectionElement> connections;
    std::vector<ViewElement> views;
    Module * module; // if this group is a 'class'; in this case goups should be empty // FIXME: remove and use ClassElement ******

                                GroupElement(GroupElement * parent, XMLElement * xml_node=NULL);
                                ~GroupElement();
    virtual std::string         GetValue(const std::string & name) const;
    GroupElement *              GetGroup(const std::string & name);
    Module *                    GetModule(const std::string & name);
    Module_IO *                 GetSource(const std::string & name);
    std::vector<Module_IO *>    GetTargets(const std::string & name); // A single Connect can result in many connections
    void                        Print(int d=0);
    std::string                 JSONString(int d=0);
    void                        CreateDefaultView();
};

Element::Element(GroupElement * parent, XMLElement * xml_node)
{
    this->parent = parent;

    if(!xml_node)
        return;
    
    for(XMLAttribute * attr=xml_node->attributes; attr!=NULL; attr = (XMLAttribute *)attr->next)
        attributes.insert({ attr->name, attr->value });
}

const std::string &
Element::GetAttribute(const std::string & a) const // get attribute verbatim // TODO: REMOVE so that it is not used by accident
{
    if(attributes.count(a))
        return attributes.at(a);
    else
        return empty_string;
};


// GetValue or the equivalent [] should be used for all access to parameters
// It takes care of variable substitutions, parameter substitution, and inheritance
// Together with the corresponding function for GroupElement

std::string
Element::GetValue(const std::string & name) const
{
    if(name.empty())
        return empty_string;

    if(attributes.count(name))
    {
        std::string value;  // We need to substitute all variables in this scope relative to the current element
        std::string sep;
        for(auto s : split(attributes.at(name), "."))
        {
            if(s[0] == '@')
                value += sep + GetValue(s.substr(1));
            else
                value += sep + s;
            sep = ".";
        }
        return value;
    }

    if(parent)
        return parent->GetValue(name);
    else
        return empty_string;
}

std::string
Element::operator[](const std::string & a) const // same as get value
{
    return GetValue(a);
};

void
Element::PrintAttributes(int d)
{
    for(auto a : attributes)
        printf((std::string(d+1, '\t')+"\t%s = \"%s\"\n").c_str(), a.first.c_str(), a.second.c_str());
}

std::string
Element::JSONAttributeString(int d)
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


ParameterElement::ParameterElement(GroupElement * parent, XMLElement * xml_node) : Element(parent, xml_node) {};

void ParameterElement::Print(int d)
{
    printf("%s\n", (std::string(d, '\t')+"\tPARAMETER: "+GetAttribute("name")).c_str());
    PrintAttributes(d);
};

std::string ParameterElement::JSONString(int d)
{
    std::string s = std::string(d, '\t')+"{\n";
    s += JSONAttributeString(d+1);
    s += "\n" + std::string(d, '\t')+"}";
    return s;
};


InputElement::InputElement(GroupElement * parent, XMLElement * xml_node) : Element(parent, xml_node) {};

std::string InputElement::MapTarget(std::string name)
{
    // TEMPORARY: will be removed when all targetmodule attributes have been removed
    auto t = attributes["target"];
    if(attributes["targetmodule"] != "")
    {
        t = attributes["targetmodule"]+"."+t;
        Kernel().Notify(msg_warning, "Attribute targetmodule=\"%s\" is deprecated in inputs.", attributes["targetmodule"].c_str());
    }
    
    auto target = rsplit(name, ".", 1);
    auto new_target = rsplit(t, ".", 1);    // t = attributes["target"] later
    
    return (new_target[0]!="" ? new_target[0] : target[0])+"."+(new_target[1]!="" ? new_target[1] : target[1]);
}

void InputElement::Print(int d)
{
    printf("%s\n", (std::string(d, '\t')+"\tINPUT: "+GetAttribute("name")).c_str());
    PrintAttributes(d);
};

std::string InputElement::JSONString(int d)
{
    std::string s = std::string(d, '\t')+"{\n";
    s += JSONAttributeString(d+1);
    s += "\n" + std::string(d, '\t')+"}";
    return s;
};


OutputElement::OutputElement(GroupElement * parent, XMLElement * xml_node) : Element(parent, xml_node) {};

std::string
OutputElement::MapSource(std::string name)
{
    // TEMPORARY: will be removed when all sourcemodule attributes have been removed
    auto s = attributes["source"];
    if(attributes["sourcemodule"] != "")
    {
        s = attributes["sourcemodule"]+"."+s;
        Kernel().Notify(msg_warning, "Attribute sourcemodule=\"%s\" is deprecated in inputs.", attributes["sourcemodule"].c_str());
    }

    auto source = rsplit(name, ".", 1);
    auto new_source = rsplit(s, ".", 1);    // s = attributes["source"] later

    return (new_source[0]!="" ? new_source[0] : source[0])+"."+(new_source[1]!="" ? new_source[1] : source[1]);
}

void
OutputElement::Print(int d)
{
    printf("%s\n", (std::string(d, '\t')+"\tOUTPUT: "+GetAttribute("name")).c_str());
    PrintAttributes(d);
};

std::string
OutputElement::JSONString(int d)
{
    std::string s = std::string(d, '\t')+"{\n";
    s += JSONAttributeString(d+1);
    s += "\n" + std::string(d, '\t')+"}";
    return s;
};


ConnectionElement::ConnectionElement(GroupElement * parent, XMLElement * xml_node) : Element(parent, xml_node) {};

void ConnectionElement::Print(int d)
{
    printf("%s\n", (std::string(d, '\t')+"\tCONNECTION: ").c_str());
    PrintAttributes(d);
}

std::string ConnectionElement::JSONString(int d)
{
    std::string s = std::string(d, '\t')+"{\n";
    s += JSONAttributeString(d+1);
    s += "\n" + std::string(d, '\t')+"}";
    return s;
};


ViewObjectElement::ViewObjectElement(GroupElement * parent, XMLElement * xml_node) : Element(parent, xml_node) {};

void ViewObjectElement::Print(int d)
{
    printf("%s\n", (std::string(d, '\t')+"\tOBJECT: ").c_str());
    PrintAttributes(d);
}

std::string ViewObjectElement::JSONString(int d)
{
    std::string tab = std::string(d, '\t');
    std::string s = tab + "{\n";
    s += JSONAttributeString(d+1);
    s += "\n" + tab+"}";
    return s;
};


ViewElement::ViewElement(GroupElement * parent, XMLElement * xml_node) : Element(parent, xml_node) {};

void ViewElement::Print(int d)
{
    printf("%s\n", (std::string(d, '\t')+"\tVIEW: ").c_str());
    PrintAttributes(d);
    printf("%s\n", (std::string(d, '\t')+"\tOBJECTS: ").c_str());
    for(auto o : objects)
        o.Print(d+1);
}

std::string ViewElement::JSONString(int d)
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


GroupElement::GroupElement(GroupElement * parent, XMLElement * xml_node) : Element(parent, xml_node)
{
};


GroupElement::~GroupElement()
{
    printf("ERROR GROUP GOING OUT OF SCOPE - SHOULD NEVER HAPPEN!!!\n");
}

std::string
GroupElement::GetValue(const std::string & a) const
{
    if(parameters.count(a))
    {
        const auto & p = parameters.at(a);
        if(p->attributes.count("name"))
        {
            auto new_name = p->attributes.at("name");
            if(new_name == a)
                Kernel().Notify(msg_fatal_error, "A parameter cannot be mapped onto itself: %s", new_name.c_str());
            else
                return GetValue(new_name); // Can be called multiple times in principle, but a bad idea
        }
    }

    return Element::GetValue(a);
}


GroupElement *
GroupElement::GetGroup(const std::string & name)
{
    if(name.empty())
        return this;

    // We need to substitute all variables in this scope relative to the current group element
    std::string p;
    std::string sep;
    for(auto s : split(name, "."))
    {
        if(s[0] == '@')
            p += sep + GetValue(s.substr(1));
        else
            p += sep + s;
        sep = ".";
    }
    
    auto n = split(p, ".", 1);
    
    if(groups.count(n[0]))
    {
        if(n.size() == 1)
            return groups.at(n[0]);

        return groups.at(n[0])->GetGroup(n[1]);
    }
    
    if(parent && parent->GetValue("name") == n[0])
        return parent->GetGroup(n[1]);
    
    return NULL;
}


Module *
GroupElement::GetModule(const std::string & name) // Get module from full or partial name relative to this group
{
    if(auto g = GetGroup(name))
        return g->module;
    return NULL;
}


Module_IO *
GroupElement::GetSource(const std::string & name) // FIXME: simplify when sourcemodule is no longer used
{
    auto source = rsplit(name, ".", 1);
    if(GroupElement * g = GetGroup(source[0]))
    {
        if(g->module)
            return g->module->GetModule_IO(g->module->output_list, source[1].c_str());

        if(!g->outputs.count(source[1]))
            return NULL;

        auto output = g->outputs.at(source[1]);
        if(!output)
            return NULL;

        auto n = output->MapSource(name);
        return g->GetSource(n);
    }
    return NULL;
}


std::vector<Module_IO *>
GroupElement::GetTargets(const std::string & name) // FIXME: simplify when targetmodule is no longer used
{
    std::vector<Module_IO *> tios;
    auto target = rsplit(name, ".", 1);

    GroupElement * g = GetGroup(target[0]);
    if(!g)
        return tios; // empty vector
    
    if(g && g->module)
    {
        if(auto tio = g->module->GetModule_IO(g->module->input_list, target[1].c_str()))
            tios.push_back(tio);
        else
            g->module->Notify(msg_fatal_error, "Module \"%s\" has no input named \"%s\".\n", g->module->GetFullName(), target[1].c_str());
        return tios;
    }
    
    for (auto & input : g->inputs) // we need to loop because there can be more than one input statement with the same name for multiple connections
        if(input["name"] == target[1])
        {
            auto n = input.MapTarget(name);
            auto targets = g->GetTargets(n);
            tios.insert(tios.end(), targets.begin(), targets.end());
        }

    return tios;
}

void
GroupElement::Print(int d)
{
    printf("%s\n", (std::string(d, '\t')+"GROUP:"+GetAttribute("name")).c_str());
    if(module)
        printf("%s\n", (std::string(d, '\t')+"MODULE:"+std::string(module->GetFullName())).c_str());

    printf("%s\n", (std::string(d, '\t')+"\tATTRIBUTES:").c_str());
    PrintAttributes(d+1);

    printf("%s\n", (std::string(d, '\t')+"\tPARAMETERS:").c_str());
    for(auto p : parameters)
        p.second->Print(d+1);

    printf("%s\n", (std::string(d, '\t')+"\tCONNECTIONS:").c_str());
    for(auto c : connections)
        c.Print(d+1);

    for(auto g : groups)
        g.second->Print(d+1);
    
    printf("\n");
};

std::string GroupElement::JSONString(int d)
{
    std::string b;
    std::string tab = std::string(d, '\t');
    std::string tab2 = std::string(d+1, '\t');
    
    std::string s = tab + "{\n";

    if(module)
        s += tab2 + "\"is_group\": false,\n";
    else
        s += tab2 + "\"is_group\": true,\n";

    s += tab2 + "\"attributes\":\n" + tab2 + "{\n"; // FIXME: make nicer!
    s += JSONAttributeString(d+2);
    s += "\n" + tab2 + "},\n";
    
    if(parameter_list.size())
    {
        s += tab2 + "\"parameters\":\n" + tab2 + "[\n";
        for(auto p : parameter_list)
        {
            s += b + p->JSONString(d+2);
            b = ",\n";
        }
        s += "\n";
        s += tab2 + "]";
    }
    else
        s += tab2 + "\"parameters\": []";
    
    s += ",\n";

    if(inputs.size())
    {
        b = "";
        s += tab2 + "\"inputs\":\n" + tab2 + "[\n";
        for(auto p : inputs)
        {
            s += b + p.JSONString(d+2);
            b = ",\n";
        }
        s += "\n";
        s += tab2 + "]";
    }
    else
        s += tab2 + "\"inputs\": []";
    
    s += ",\n";
    
    if(outputs.size())
    {
        b = "";
        s += tab2 + "\"outputs\":\n" + tab2 + "[\n";
        for(auto p : outputs) // FIXME: not empty when there are no outputs; instead null pointer in second???
        {
            if(p.second)
            {
            s += b + p.second->JSONString(d+2);
            b = ",\n";
            }
            else
                kernel().Notify(msg_fatal_error, "Internal Eror – Empty output structure.");
        }

        s += "\n";
        s += tab2 + "]";
    }
    else
        s += tab2 + "\"outputs\": []";
    
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

    if(groups.size())
    {
        s += tab2 + "\"groups\":\n" + tab2 + "[\n";
        b = "";
        for(auto g : groups)
        {
            s += b + g.second->JSONString(d+2);
            b = ",\n";
        }
        s += "\n";
        s += tab2 + "]\n";
    }
    else
        s += tab2 + "\"groups\": []\n";

    s += tab + "}";
    
    return s;
}


void
GroupElement::CreateDefaultView()
{
    if(!outputs.size())
        return;
    
    auto * v = new ViewElement(this);

    int margin = 20;
    int x = 20;
    int y = 20;
    int w = 201;
    int h = 201;

    for(auto output : outputs)
    {
        auto * o = new ViewObjectElement((GroupElement *)v);    // FIXME: parent should be ViewElement not GroupElement

        v->attributes["name"] = "View";
        
        o->attributes["class"] = "plot";
        o->attributes["source"] = "."+output.first;
        o->attributes["title"] = "."+output.first;
        o->attributes["x"] = std::to_string(x);
        o->attributes["y"] = std::to_string(y);
        o->attributes["width"] = std::to_string(w);
        o->attributes["height"] = std::to_string(h);
     
        v->objects.push_back(*o);
        
        x += w+margin;
        if(x > 3*w)
        {
            x = 20;
            y += h+margin;
        }
    }
    
    views.push_back(*v);
}


//
// ModuleClass
//

ModuleClass::ModuleClass(const char * n, ModuleCreator mc, const char * p)
{
    name = n;
    module_creator = mc;
    path = create_string(p);
}

ModuleClass::~ModuleClass()
{
    delete path;
}


const char *
ModuleClass::GetClassPath()
{
    return path;
}

Module *
ModuleClass::CreateModule(Parameter * p)
{
    Module * m = (*module_creator)(p);
    if(!m->input_list && !m->output_list)
        m->AddIOFromIKC();
        if(m->GetBoolValue("power_output"))
        m->AddOutput("POWER", false, 1, 1); // TEST *******
    return m;
}

bool
Module_IO::Allocate()
{
    if(sizex == unknown_size || sizey == unknown_size)
    {
        if(module != NULL && !optional)
            return module->Notify(msg_fatal_error, "Attempting to allocate io (\"%s\") with unknown size for module \"%s\" (%s). Check that all required inputs are connected.\n", name.c_str(), module->GetName(), module->GetClassName());

    }
    if(sizex*sizey <= 0)
    {
        if(module != NULL && !optional)
            return module->Notify(msg_fatal_error, "Internal error while trying to allocate data of size 0.\n");
    }
    
    if(module != NULL) module->Notify(msg_debug, "Allocating data of size %d.\n", size);
    data	=   new float * [max_delay];
    matrix  =   new float ** [max_delay];
    for (int d=0; d<max_delay; d++)
    {
        matrix[d] = create_matrix(sizex, sizey);
        data[d] = matrix[d][0];
    }
    return true;
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
    if(matrix)
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
    if(x == unknown_size)
        return;
    if(size != unknown_size && s != size)
    {
        if(module != NULL)
            module->Notify(msg_fatal_error, "Module_IO::SetSize: Attempt to resize data array \"%s\" of module \"%s\" (%s) (%d <= %d). Ignored.\n",  name.c_str(), module->GetName(), module->GetClassName(), size, s);
        return;
    }
    if(s == size)
        return;
    if(s == 0)
        return;
    if(module != NULL)
        module->Notify(msg_debug, "Allocating memory for input/output \"%s\" of module \"%s\" (%s) with size %d and max_delay = %d (in SetSize).\n", name.c_str(), module->instance_name, module->GetClassName(), s, max_delay);
    sizex = x;
    sizey = y;
    size = x*y;
    if(module != NULL && module->kernel != NULL)
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
}

void
Module::AddInput(const char * name, bool optional, bool allow_multiple_connections)
{
    if(GetModule_IO(input_list, name) != NULL)
    {
        Notify(msg_warning, "Input \"%s\" of module \"%s\" (%s) already exists.\n", name, GetName(), GetClassName());
        return;
    }
    input_list = new Module_IO(input_list, this, name, unknown_size, 1, optional, allow_multiple_connections);
    Notify(msg_trace, "  Adding input \"%s\".\n", name);
}

void
Module::AddOutput(const char * name, bool optional, int sizeX, int sizeY)
{
    if(GetModule_IO(output_list, name) != NULL)
    {
        Notify(msg_warning, "Output \"%s\" of module \"%s\" (%s) already exists.\n", name, GetName(), GetClassName());
        return;
    }
    output_list = new Module_IO(output_list, this, name, sizeX, sizeY, optional);
    Notify(msg_trace, "  Adding output \"%s\" of size %d x %d to module \"%s\" (%s).\n", name, sizeX, sizeY, GetName(), GetClassName());
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


const char * // FIXME: ***********************
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
            if(equal_strings(t, n))
            {                
                // we have found our parameter
                // it controls this module if module name is set to the name of this module or if it is not set = relates to all modules
                const char * tm = kernel->GetXMLAttribute(parameter, "module");
                if(tm == NULL || (equal_strings(tm, module_name)))
                {
                    // use default if it exists
                    const char * d = kernel->GetXMLAttribute(parameter, "values");
                    if(d)
                        return d;
                    
                    // the parameter element redefines our parameter name; get the new name
                    const char * newname = kernel->GetXMLAttribute(parameter, "name");
                    if(newname == NULL)
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


const char * // FIXME: ***********************
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
            if(equal_strings(t, n))
            {                
                // we have found our parameter
                // it controls this module if module name is set to the name of this module or if it is not set = relates to all modules
                const char * tm = kernel->GetXMLAttribute(parameter, "module");
                if(tm == NULL || (equal_strings(tm, module_name)))
                {
                    // use default if it exists
                    const char * d = kernel->GetXMLAttribute(parameter, "default");
                    if(d)
                        return (*d != '\0' ? d : NULL); // return NULL if default is an empty string;
                    
                    // the parameter element redefines our parameter name; get the new name
                    const char * newname = kernel->GetXMLAttribute(parameter, "name");
                    if(newname == NULL)
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
    std::string r = group->GetValue(n);
    if(!r.empty())
        return create_string(r.c_str()); // FIXME: leaks, but will be changed to const & std::string later
    else
    {
        std::string ss = std::string(instance_name)+"."+std::string(n);
        r = group->GetValue(ss.c_str());
        if(!r.empty())
            return create_string(r.c_str()); // FIXME: leaks, but will be changed to const & std::string later
    }
       
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
    if(v == NULL)
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
        if((list[lp] == '/' || list[lp] == 0) && name[np] == 0)
            return ix;
        // skip to next name
        ix++;
        while (list[lp] && list[lp] != '/')
            lp++;
        // no match
        if(list[lp] == 0)
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
    if(!v)
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
    if(v == NULL)
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
        if(sscanf(v, "%d", &x)!=-1)
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
        if(i >= requested_size || (sscanf(v, "%d", &a[i])==-1))
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
    kernel->bindings[std::string(full_instance_name)+"."+std::string(n)].push_back(new Binding(this, n, bind_float, &v, 0, 0));
}


void
Module::Bind(float * & v, int size, const char * n, bool fixed_size)
{
    // TODO: check type here
    v = GetArray(n, size, fixed_size);
    kernel->bindings[std::string(full_instance_name)+"."+std::string(n)].push_back(new Binding(this, n, bind_array, v, size, 1));
}


void
Module::Bind(float ** & v, int & sizex, int & sizey, const char * n, bool fixed_size)
{
    // TODO: check type here
    v = GetMatrix(n, sizex, sizey, fixed_size);
    kernel->bindings[std::string(full_instance_name)+"."+std::string(n)].push_back(new Binding(this, n, bind_matrix, v, sizex, sizey));
}


void
Module::Bind(int & v, const char * n)
{
    // TODO: check type here
    if(GetList(n))
    {
        v = GetIntValueFromList(n);
        kernel->bindings[std::string(full_instance_name)+"."+std::string(n)].push_back(new Binding(this, n, bind_list, &v, 0, 0));
    }
    else
    {
        v = GetIntValue(n);
        kernel->bindings[std::string(full_instance_name)+"."+std::string(n)].push_back(new Binding(this, n, bind_int, &v, 0, 0));
    }
}


void
Module::Bind(bool & v, const char * n)
{
    // TODO: check type here
    v = GetBoolValue(n);
    kernel->bindings[std::string(full_instance_name)+"."+std::string(n)].push_back(new Binding(this, n, bind_bool, &v, 0, 0));
}


void
Module::Bind(std::string & v, const char * n)
{
    // TODO: check type here
    auto x = GetValue(n);   // FIXME: temoprary; GetValue should never return NULL
    if(!x)
        v = std::string("");
    else
        v = std::string(x);
    auto p = std::string(full_instance_name)+"."+std::string(n);
    auto b = new Binding(this, n, bind_string, &v, 0, 0);
    kernel->bindings[p].push_back(b);
}


Module_IO *
Module::GetModule_IO(Module_IO * list, const char * name)
{
    for (Module_IO * i = list; i != NULL; i = i->next)
        if(equal_strings(name, i->name.c_str()))
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
        if(equal_strings(name, i->name.c_str()))
        {
            if(i->data == NULL)
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
        if(equal_strings(name, i->name.c_str()))
        {
            if(i->data == NULL)
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
        if(equal_strings(name, i->name.c_str()))
        {
            if(i->matrix == NULL)
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
        if(equal_strings(name, i->name.c_str()))
        {
            if(i->matrix == NULL)
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
        if(equal_strings(input_name, i->name.c_str()))
        {
            if(i->size != unknown_size)
                return i->size;
            else if(kernel != NULL)
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
        if(equal_strings(input_name, i->name.c_str()))
        {
            if(i->sizex != unknown_size)
                return i->sizex;
            else if(kernel != NULL)
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
        if(equal_strings(input_name, i->name.c_str()))
        {
            if(i->sizex != unknown_size)	// Yes, sizeX is correct
                return i->sizey;
            else if(kernel != NULL)
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
        if(equal_strings(name, i->name.c_str()))
        {
            if(i->size > 0)
            {
                return i->size;
            }
            else
            {
                Notify(msg_fatal_error, "Output size not set for %s.%s \n", this->instance_name, name);
                return 1;
            }
        }
    Notify(msg_warning, "Attempting to get size of non-existing output %s.%s \n", this->instance_name, name);
    return 0;
}

int
Module::GetOutputSizeX(const char * name)
{
    for (Module_IO * i = output_list; i != NULL; i = i->next)
        if(equal_strings(name, i->name.c_str()))
        {
            if(i->sizex > 0)
            {
                return  i->sizex;
            }
            else
            {
                Notify(msg_fatal_error, "Output size x not set for %s.%s \n", this->instance_name, name);
                return 1;
            }
        }

    Notify(msg_warning, "Attempting to get size of non-existing output %s.%s \n", this->instance_name, name);
    return 0;
}

int
Module::GetOutputSizeY(const char * name)
{
    for (Module_IO * i = output_list; i != NULL; i = i->next)
        if(equal_strings(name, i->name.c_str()))
        {
            if(i->sizey > 0)
            {
                return  i->sizey;
            }
            else
            {
                Notify(msg_fatal_error, "Output size y not set for %s.%s \n", this->instance_name, name);
                return 1;
            }
        }

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
Module::io(matrix & m, std::string name, bool required)
{
    float ** data;
    if((data = GetOutputMatrix(name.c_str(), required)))
    {
        m.size_x = GetOutputSizeX(name.c_str());
        m.size_y = GetOutputSizeY(name.c_str());
        m.data = data;
        return;
    }
    else if ((data = GetInputMatrix(name.c_str(), required)))
    {
        m.size_x = GetInputSizeX(name.c_str());
        m.size_y = GetInputSizeY(name.c_str());
        m.data = data;
        return;
    }
 //   else
 //       return matrix(); // FIXME: throw exception if required
}



void
Module::SetOutputSize(const char * name, int x, int y)
{
    if(x < -1 || y < -1)
    {
        Notify(msg_warning, "Attempting to set negative size of %s.%s \n", this->instance_name, name);
        return;
    }
    for (Module_IO * i = output_list; i != NULL; i = i->next)
        if(equal_strings(name, i->name.c_str()))
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
    group = p->group;
    log_level = kernel->log_level;
    instance_name = kernel->GetXMLAttribute(xml, "name"); // GetValue("name");
    class_name = kernel->GetXMLAttribute(xml, "class");
    period = (GetValue("period") ? GetIntValue("period") : 1);
    phase = (GetValue("phase") ? GetIntValue("phase") : 0);
	active = GetBoolValue("active", true);
    high_priority = GetBoolValue("high_priority", false);

	// Compute full name
    std::vector<std::string> path;
    for (GroupElement * g = group; g->parent != NULL; g = g->parent) // Skip the outermost group name
        path.push_back(g->GetAttribute("name"));
    std::string s = join(".", path, true);
    full_instance_name = create_string(s.c_str());
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
Module::SetSizes()  // FIXME: remove xml access, use output elements
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

            if((sx == unknown_size) && (sizearg = e->GetAttribute("size_param_x")) && (arg = GetValue(sizearg)))
                sx = string_to_int(arg);
            
            if((sy == unknown_size) && (sizearg = e->GetAttribute("size_param_y")) && (arg = GetValue(sizearg)))
                sy = string_to_int(arg);
            
            if((sx == unknown_size) && (sizearg = e->GetAttribute("size_param")) && (arg = GetValue(sizearg)))
            {
                sx = string_to_int(arg);
                sy = 1;
            }
            
            if((sx == unknown_size) && (sizearg = e->GetAttribute("size_x")))
                sx = string_to_int(sizearg);
            
            if((sy == unknown_size) && (sizearg = e->GetAttribute("size_y")))
                sy = string_to_int(sizearg);
            
            if((sx == unknown_size) && (sizearg = e->GetAttribute("size")))
            {
                sx = string_to_int(sizearg);
                sy = 1;
            }
            
			if((sx == unknown_size) && (sy == unknown_size) && (sizearg = e->GetAttribute("size_set"))) // Set output size x & y from one or multiple inputs
            {
                sx = GetSizeXFromList(sizearg);
                sy = GetSizeYFromList(sizearg);
			}
            
			else if((sx == unknown_size) && (sizearg = e->GetAttribute("size_set_x")) && (sizeargy = e->GetAttribute("size_set_y"))) // Set output size x from one or multiple different inputs for both x and y
			{
                sx = GetSizeXFromList(sizearg) * GetSizeYFromList(sizearg);     // Use total input sizes
                sy = GetSizeXFromList(sizeargy) * GetSizeYFromList(sizeargy);   // TODO: Check that no modules assumes it is ony X or Y sizes
			}
            
			else if((sx == unknown_size) && (sizearg = e->GetAttribute("size_set_x"))) // Set output size x from one or multiple inputs
                sx = GetSizeXFromList(sizearg);
            
			else if((sy == unknown_size) && (sizearg = e->GetAttribute("size_set_y"))) // Set output size y from one or multiple inputs
                sy = GetSizeYFromList(sizearg);
            
            SetOutputSize(output_name, sx, sy);
        }
	}
}

bool
Module::Notify(int msg)
{
    if(kernel != NULL)
        kernel->Notify(msg, "\n");
    else if(msg == msg_fatal_error)
    {
        global_fatal_error = true;
        global_error_count++;
    }
    else if(msg == msg_warning)
    {
//        global_warning_count++;
    }
    return false;
}


bool
Module::Notify(int msg, const char *format, ...)
{
    if(msg > GetIntValue("log_level"))
        return false;
    char 	message[512];
    sprintf(message, "%s (%s): ", GetFullName(), GetClassName());
    size_t n = strlen(message);
    va_list args;
    va_start(args, format);
    vsnprintf(&message[n], 512, format, args);
    va_end(args);
    if(kernel != NULL && msg>=log_level)
        return kernel->Notify(-msg, message);
    else if(msg == msg_fatal_error)
    {
        global_fatal_error = true;
        if(message[strlen(message)-1] == '\n')
            message[strlen(message)-1] = '\0';
        printf("IKAROS: ERROR: %s\n", message);
        global_error_count++;
    }
    return false;
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
    if(source_io != NULL && source_io->module != NULL)
        source_io->module->Notify(msg_debug, "    Deleting Connection.\n");
    delete next;
}

void
Connection::Propagate(long long tick)
{
    if(!active)
        return;
    if(delay == 0)
        return;
    // Return if both modules will not start in this tick - necessary when using threads
    if(tick % source_io->module->period != source_io->module->phase)
        return;
    if(tick % target_io->module->period != target_io->module->phase)
        return;
    if(source_io != NULL && source_io->module != NULL)
        source_io->module->Notify(msg_debug, "  Propagating %s.%s -> %s.%s (%p -> %p) size = %d\n", source_io->module->GetName(), source_io->name.c_str(), target_io->module->GetName(), target_io->name.c_str(), source_io->data, target_io->data, size);
    for (int i=0; i<size; i++)
        target_io->data[0][i+target_offset] = source_io->data[delay-1][i+source_offset];
}


ThreadGroup::ThreadGroup(Kernel * k)
{
    period = 1;
    phase = 0;
    thread = NULL;
}


ThreadGroup::ThreadGroup(Kernel * k, int period_, int phase_)
{
    period = period_;
    phase = phase_;
    thread = NULL;
}


ThreadGroup::~ThreadGroup()
{
//    delete thread;
//    delete next;  // FIXME: do this correctly
}

static void *
ThreadGroup_Tick(void *group, bool high_priority)
{
    if (high_priority)     // Set high priority of the thread
    {
        sched_param sch;
        int policy;
        pthread_t t = pthread_self();
        pthread_getschedparam(t, &policy, &sch);
        sch.sched_priority = 80;
        if (pthread_setschedparam(t, SCHED_FIFO, &sch))
            std::cout << "Failed to setschedparam: " << std::strerror(errno) << '\n';
    }
    ((ThreadGroup *)group)->Tick();
    return NULL;
}

void
ThreadGroup::Start(long long tick)
{
    // Test if group should be started
    if(tick % period == phase)
    {

        thread = new std::thread(ThreadGroup_Tick, (void *)this, high_priority);
    }
}

void
ThreadGroup::Stop(long long tick)
{
    // Test if group should be joined
    if((tick + 1) % period == phase)
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
            float time_used = m->timer->GetTime();
        m->time += time_used;
        m->ticks += 1;
    
        // Set power usage output here
        if(m->power)
            *(m->power) = time_used / m->GetTickLength();
    }
}


Kernel::Kernel()
{
    options                 = NULL;
    useThreads              = false; // FIXE: Should be set to true
    max_ticks               = -1;
    tick_length             = 0;
    
    log_level               = log_level_info;
    ikaros_dir              = NULL;
    ikc_dir                 = NULL;
    ikc_file_name           = NULL;

    tick                    = 0;
    xmlDoc                  = NULL;
    connections             = NULL;
    module_count            = 0;
    period_count            = 0;
    phase_count             = 0;
    end_of_file_reached     = false;
    fatal_error_occurred	= false;
    terminate			    = false;
    sizeChangeFlag          = false;
    
    logfile                 = NULL;
    timer		            = new Timer();
    
    // ------------ WebUI part --------------
    
    webui_dir               = NULL;
    xml                     = NULL;
    ui_state                = ui_state_pause;
    master_id               = 0;
    tick_is_running         = false;
    sending_ui_data         = false;
    debug_mode              = false;
    isRunning               = false;
    idle_time               = 0;
    time_usage              = 0;
}


void
Kernel::SetOptions(Options * opt)
{
    options             = opt;
    useThreads          = options->GetOption('t') || options->GetOption('T');
    max_ticks           = string_to_int(options->GetArgument('s'), -1);
    tick_length         = string_to_int(options->GetArgument('r'), 0);

    if(options->GetOption('q'))
        log_level = log_level_off;
    if(options->GetOption('v'))
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
    for(auto c : classes)
        if(c.second->path != NULL && c.second->path[0] != '/')
        {
            const char * t = c.second->path;
            c.second->path = create_formatted_string("%s%s", ikaros_dir, c.second->path);
            destroy_string((char *)t);
        }
    
    // WebUI Part
    
    port = PORT;
    if (options->GetOption('w'))
        port = string_to_int(options->GetArgument('w'), PORT);
    
    if (options->GetOption('W')) // TODO: Use log-level for this instead?
    {
        port = string_to_int(options->GetArgument('W'), PORT);
        Notify(msg_debug, "Setting up WebUI port at %d in debug mode\n", port);
        debug_mode = true;
    }
    
    if(options->GetOption('r'))
    {
        ui_state = ui_state_realtime;
        isRunning = true;
        Notify(msg_debug, "Setting real-time mode.\n");
    }
    
    if (options->GetOption('R'))
    {
        port = string_to_int(options->GetArgument('R'), PORT);
        Notify(msg_debug, "Setting up WebUI port at %d\n", port);
        if(options->GetOption('r'))
        {
            ui_state = ui_state_realtime;
            Notify(msg_debug, "Setting real-time mode.\n");
        }
        else
        {
//          ui_state = ui_state_play;
            Notify(msg_debug, "Setting play mode.\n");
        }
        isRunning = true;
    }
}


Kernel::~Kernel()
{
    return; // TODO: fix this later
/*
    Notify(msg_debug, "Deleting Kernel.\n");
    Notify(msg_debug, "  Deleting Connections.\n");
    delete connections;
    Notify(msg_debug, "  Deleting Modules.\n");
//    delete modules; // FIXME: delete
    Notify(msg_debug, "  Deleting Thread Groups.\n");
//    delete threadGroups;  // FIXME: delete
    Notify(msg_debug, "  Deleting Classes.\n");
//    delete classes;
    
    delete timer;
    delete xmlDoc;
    delete ikaros_dir;
    
    Notify(msg_debug, "Deleting Kernel Complete.\n");
    if(logfile) fclose(logfile);
    
#ifdef USE_MALLOC_DEBUG
    dump_memory();  // dump blocks that are still allocated
#endif
*/
}


std::string
Kernel::JSONString()
{
    if(main_group)
        return main_group->JSONString();
    else
        return "{}";
}


bool
Kernel::AddClass(const char * name, ModuleCreator mc, const char * path)
{
    if(path == NULL)
        return Notify(msg_fatal_error, "Path to ikc file is missing for class \"%s\".\n", name);
    
    char * path_to_ikc_file = NULL;
    
    // Test for backward compatibility and remove initial paths if needed
    
    if(ikaros_dir)
        path_to_ikc_file = create_formatted_string("%s%s%s.ikc", ikaros_dir, path, name); // absolute path
    else
        path_to_ikc_file = create_formatted_string("%s%s.ikc", path, name); // relative path

    classes.insert({ name, new ModuleClass(name, mc, path_to_ikc_file)} );
    
    destroy_string(path_to_ikc_file);
    return true;
}

bool
Kernel::Terminate()
{
    if(max_ticks != -1 && tick >= max_ticks)
        return !Notify(msg_debug, "Max ticks reached.\n");

    return end_of_file_reached || fatal_error_occurred  || global_fatal_error || terminate || global_terminate;
}


void
Kernel::Run()
{
    first_request = true;

    if(socket == NULL)
        return;

    chdir(ikc_dir);

    timer->Restart();
    tick = 0;
    httpThread = new std::thread(Kernel::StartHTTPThread, this);

    while (!Terminate())
    {
        if (!isRunning)
        {
            Timer::Sleep(10); // Wait 10ms to avoid wasting cycles if there are no requests
        }
        
        if (isRunning)
        {
            tick_is_running = true; // Flag that state changes are not allowed
            Tick();
            tick_is_running = false;
            
            // Calculate idle_time
            
            if(tick_length > 0)
            {
                idle_time = (float(tick*tick_length) - timer->GetTime()) / float(tick_length);
                time_usage = 1 - idle_time;
            }

            if (tick_length > 0)
            {
                lag = timer->WaitUntil(float(tick*tick_length));
                if (lag > 0.1) Notify(msg_warning, "Lagging %.2f ms at tick = %ld\n", lag, tick);
            }
            else if (ui_state == ui_state_realtime)
            {
                while(sending_ui_data)
                    {}
            }
        }
    }
}


void
Kernel::PrintTiming()
{
    total_time = timer->GetTime()/1000; // in seconds
    if(max_ticks != 0)
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
            if(i->size == unknown_size)
            {
                // Check if connected
                bool connected = false;
                for (Connection * c = connections; c != NULL; c = c->next)
                    if(c->target_io == i)
                        connected = true;
                if(connected)
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
            if(i->size == unknown_size)
            {
                // Check if connected
                bool connected = false;
                for (Connection * c = connections; c != NULL; c = c->next)
                    if(c->source_io == i)
                        connected = true;
                if(connected)
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
        if(c->source_io->size == unknown_size)
        {
            Notify(msg_fatal_error, "Output \"%s\" of module \"%s\" (%s) has unknown size.\n", c->source_io->name.c_str(), c->source_io->module->instance_name, c->source_io->module->GetClassName());
        }
        else if(c->target_io->data != NULL && c->target_io->allow_multiple == false)
        {
            Notify(msg_fatal_error, "Input \"%s\" of module \"%s\" (%s) does not allow multiple connections.\n", c->target_io->name.c_str(), c->target_io->module->instance_name, c->target_io->module->GetClassName());
        }
        else if(c->delay == 0)
        {
            Notify(msg_debug, "Short-circuiting zero-delay connection from \"%s\" of module \"%s\" (%s)\n", c->source_io->name.c_str(), c->source_io->module->instance_name, c->source_io->module->GetClassName());
            // already connected to 0 or longer delay?
            if(c->target_io->data != NULL)
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
            if(c->target_io->max_delay == 0)
            {
                Notify(msg_fatal_error, "Failed to connect from \"%s\" of module \"%s\" (%s) because target is already connected with zero-delay.\n", c->source_io->name.c_str(), c->source_io->module->instance_name, c->source_io->module->GetClassName());
            }
            // First connection to this target: initialize
            if(c->target_io->size == unknown_size)	// start calculation with size 0
                c->target_io->size = 0;
            int target_offset = c->target_io->size;
            c->target_io->size += c->source_io->size;
            // Target not used previously: ok to connect anything
            if(c->target_io->sizex == unknown_size)
            {
                Notify(msg_debug, "New connection.\n");
                c->target_io->sizex = c->source_io->sizex;
                c->target_io->sizey = c->source_io->sizey;
            }
            // Connect one dimensional output
            else if(c->target_io->sizey == 1 && c->source_io->sizey == 1)
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
            if(c->target_io->max_delay == 0)
                Notify(msg_fatal_error, "Failed to connect from \"%s\" of module \"%s\" (%s) because target is already connected with zero-delay.\n", c->source_io->name.c_str(), c->source_io->module->instance_name, c->source_io->module->GetClassName());

            // First connection to this target: initialize
            if(c->target_io->size == unknown_size)    // start calculation with size 0
                c->target_io->size = max(c->target_io->size, c->target_offset+c->size);

            // Target not used previously: ok to connect anything
            if(c->target_io->sizex == unknown_size)
            {
                Notify(msg_debug, "New connection.\n");
                c->target_io->sizex = c->target_io->size;
                c->target_io->sizey = 1;
            }
            // Connect one dimensional output
            else if(c->target_io->sizey == 1 && c->source_io->sizey == 1)
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
        if(sizeChangeFlag)
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
        m->power=m->GetOutputArray("POWER");
        m->power_coefficient = m->GetFloatValue("power_coefficient", 1.0);
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

    if(options->GetFilePath())
        ReadXML();
    else
        Notify(msg_print, "Running without input file.\n");
    
    if(fatal_error_occurred)
        return;

    // Fill data structures
 
    for (Module * & m : _modules)
    {
 //       if(!module_map.count(m->GetFullName()))
             module_map.insert({ m->GetFullName(), m });
 //       else
 //           Notify(msg_fatal_error, "Duplicate module name \"%s\".", m->GetFullName()); // Would be good to find this out before module creation
    }

    for (Connection * c = connections; c != NULL; c = c->next)
    {
        //module_map[c->source_io->module->GetFullName()]->outgoing_connection.insert(c->target_io->module->GetFullName());
        if(c->delay == 0)
        {
            module_map[c->source_io->module->GetFullName()]->connects_to_with_zero_delay.push_back(c->target_io->module);
            module_map[c->target_io->module->GetFullName()]->connects_from_with_zero_delay.push_back(c->source_io->module);
        }
    }

    SortModules();
    if(fatal_error_occurred)
    {
        ListModulesAndConnections(); // May cause crash but will be helpful if it works!
        return;
    }
    CalculateDelays();
    InitOutputs();      // Calculate the output sizes for outputs that have not been specified at creation
    AllocateOutputs();
    InitInputs();		// Calculate the input sizes and allocate memory for the inputs or connect 0-delays
    CheckOutputs();
    CheckInputs();
    if(fatal_error_occurred)
    {
        ListModulesAndConnections();
        return;
    }

    InitModules();
    
    webui_dir = create_formatted_string("%s%s", ikaros_dir, WEBUIPATH);
    socket =  new ServerSocket(port);
}


// This function may need to be linked with 'librt' on Linux
// Calculates CPU usage for this process

void
Kernel::CalculateCPUUsage()
{
    double cpu = 0;
    struct rusage rusage;
    if(getrusage(RUSAGE_SELF, &rusage) != -1)
        cpu = (double)(1000.0*rusage.ru_utime.tv_sec) + (double)rusage.ru_utime.tv_usec / 1000.0;
    float time = timer->GetTime();
    float tdiff = time - last_cpu_time;
    if(tdiff > 0)
        cpu_usage = (cpu-last_cpu)/(float(cpu_cores)*tdiff);

    last_cpu_time = time;
    last_cpu = cpu;
}



void
Kernel::Tick()
{
    Notify(msg_debug, "   Kernel::Tick()\n");
    Propagate();
    DelayOutputs();
  
    if(useThreads) // TODO: Threads are always used - remove other alternatives
    {
        for (auto & g : _threadGroups)
            g->Start(tick);
        for (auto & g : _threadGroups)
            g->Stop(tick);
    }
    else if(log_level < log_level_debug)
    {
        for (auto & m : _modules)
            if(tick % m->period == m->phase)
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
            if(tick % m->period == m->phase)
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
 
#ifdef NANCHECK
        CheckNAN();
#endif

    tick++;
    CalculateCPUUsage();
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
    if(!m) return;
    m->kernel = this;
    _modules.push_back(m);
}


Module *
Kernel::GetModule(const char * n)
{
    for (Module * & m : _modules)
        if(equal_strings(n, m->instance_name))
            return m;
    return NULL;
}


Module *
Kernel::GetModuleFromFullName(const char * n)
{
    for (Module * & m : _modules)
        if(equal_strings(n, m->full_instance_name))
            return m;
    return NULL;
}


// io, k->main_group, module, source
// FIXME: maybe get inputs as well for WebUI

bool // TODO: Rewrite
Kernel::GetSource(Module_IO * &io, GroupElement * group, const char * source_module_name, const char * source_name)
{
    if((io = group->GetSource(source_name)))
        return true;
    else
        return false;
}


const char *
Kernel::GetXMLAttribute(XMLElement * e, const char * attribute) // FIXME: DELETE
{
    const char * value = NULL;
    
    while(e != NULL && e->IsElement())
        if((value = e->GetAttribute(attribute)))
             return value;
        else
            e = (XMLElement *)(e->parent);

    if((value = options->GetValue(attribute)))
        return value;
    
    return NULL;
}


bool
Kernel::GetBinding(Module * &m, int &type, void * &value_ptr, int & sx, int & sy, const char * source_module_name, const char * source_name)    // FIXME: this function should probably be removed
{
    std::string name = std::string(source_module_name)+"."+std::string(source_name);
    if(!bindings.count(name))
        return Notify(msg_warning, "Could not find binding. \"%s\" does not exist\n", name.c_str());

    Binding * b = bindings.at(name).at(0);  // FIXME: allow iteraction over vector
    type = b->type;
    value_ptr = b->value;
    sx = b->size_x;
    sy = b->size_y;
    return true;
}


bool
Module::SetParameter(const char * parameter_name, int x, int y, float value)
{
    std::string name = std::string(full_instance_name)+"."+std::string(parameter_name);
    if(!kernel->bindings.count(name))
        return Notify(msg_warning, "Could not find binding. \"%s\" does not exist\n", name.c_str());

    Binding * b = kernel->bindings.at(name).at(0);  // FIXME: allow iteration over vector of bindings

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
    
    return true;
}


bool // FIXME: ***************************** PARAMETER INHERITANCE MISSING ********************, remove XML
Kernel::SetParameter(const char * name, int x, int y, float value)
{
    // OLD with variable substitution
    //auto * m = GetModule(group_name);
    //m->SetParameter(parameter_name, select_x, select_y, value);

    // New with bidnings, but without variable substitution - can't have both
    
    if(bindings.count(name))
    {
        Binding * b = bindings.at(name).at(0);  // FIXME: allow iteration over vector of bindings
        if(b->type == bind_float)
            *((float *)(b->value)) = value;
        else if(b->type == bind_int || b->type == bind_list)
            *((int *)(b->value)) = (int)value;
        else if(b->type == bind_bool)
            *((bool *)(b->value)) = (value > 0);
        else if(b->type == bind_array)
        {
            if(x < 0 || x >= b->size_x)
                return Notify(msg_warning, "Parameter index x out of range for %s\"", name);
            ((float *)(b->value))[x] = value;
        }
        else if(b->type == bind_matrix)
        {
            if(x < 0 || x >= b->size_x)
                return Notify(msg_warning, "Parameter index x out of range for \"%s\"", name);
            if(y < 0 || y >= b->size_y)
                return Notify(msg_warning, "Parameter index y out of range for \"%s\"", name);
           ((float **)(b->value))[y][x] = value;
        }
    }

    return true;
}


void
Kernel::SendCommand(const char * command, float x, float y, std::string value)
{
    std::string c = command;
    auto s = rsplit(c, ".", 1);
    if(auto * g = main_group->GetGroup(s[0]))
        if(Module * m = g->module)
            m->Command(s[1], x, y, value);
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
        if(c->target_io == i)
        {
            if(c->source_io->size == unknown_size)
                return unknown_size;
            else if(c->target_offset>0 || c->size>0) // offset connection
                s = max(s, c->target_offset + c->size);
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
        if(c->target_io == i)
        {
            if(c->source_io->sizex == unknown_size)
                return unknown_size;
            else if(c->target_offset>0 || c->size>0) // offset connection
                s = max(s, c->target_offset + c->size);
            else if(s == 0)
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
        if(c->target_io == i)
        {
            if(c->source_io->sizey == unknown_size)
                return unknown_size;
            else if(s == 0)
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



bool
Kernel::CreateThreadGroups(std::deque<Module *> & sorted_modules)
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
                        Notify(msg_fatal_error, "Module period and phase does not match rest of subgraph (%s).", m->GetFullName());

                    tg->_modules.push_back(m);
                    m->mark = 4;
                    if (m->high_priority  && !tg->high_priority) // thread group priority
                        tg->high_priority = true;
                }
        }
    return true;
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
        return Notify(msg_fatal_error, "Network contains zero-connection loop at %s", n->GetFullName());
 
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
        if(c->delay > c->source_io->max_delay)
            c->source_io->max_delay = c->delay;
    }
}


bool
Kernel::Notify(int msg, const char * format, ...)
{
    switch (abs(msg))
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
        return false;
    char 	message[512];
    int n = 0;
    switch (abs(msg))
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
    fflush(stdout);
    if(logfile != NULL)
        fprintf(logfile, "%5lld: %s", tick, message);	// Print in both places
    return false;
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
        if(d == 0 && (s_offset > 0 || t_offset > 0 || size >= 0))
        {
            Notify(msg_fatal_error, "delay=\"0\" cannot be combined with range attributes in connections."); // TODO: print module and connection names as well
            return 0;
        }
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
	if(path != NULL)
	{
		FILE * t = fopen(path, "rb");
		bool exists = (t != NULL);
		if(t) fclose(t);
		return (exists ? path : NULL);
	}
    
	return NULL;
}


// Read class file (or included file) and merge with the current XML-tree

XMLElement * 
Kernel::BuildClassGroup(GroupElement * group, XMLElement * xml_node, const char * class_name, const char * current_filename)
{
 //   char * name = create_string(GetXMLAttribute(xml_node, "name"));
//    printf("BuildClassGroup: %s\n", name);
    
    // THREE ALTERNATIVES:
    //  Global path - starts with /
    //  Local path  - does not start with /
    //  No path     - path is NULL or equals ""

    char include_file[PATH_MAX] ="";
    const char * path = xml_node->GetAttribute("path"); // FIXME: use GetValue with variables, inheritance etc
    if(path) //  && path[0]=='/'
    {
        if(path[0]=='/') // gloabl path
            copy_string(include_file, ikaros_dir, PATH_MAX);
        append_string(include_file, path, PATH_MAX);
    }
    if(path && include_file[strlen(include_file)-1] != '/')
        append_string(include_file, "/", PATH_MAX);
    
    append_string(include_file, class_name, PATH_MAX);
    append_string(include_file, ".ikg", PATH_MAX);
    
	const char * filename = file_exists(include_file);
    
    if (!filename && classes.find(class_name)==classes.end())
        Notify(msg_warning, "Group ikg for \"%s\" could not be found with path %s. HINT: Check if path is correct relative to including file.\n", class_name, include_file);
    else if(classes.count(class_name))
        filename = (filename ? filename : file_exists(classes.at(class_name)->GetClassPath())); // not found in path, search built in classes // FIXME: crashes if class_name does not exist

	if(!filename)
	{
		Notify(msg_warning, "Class ikc for \"%s\" could not be found.\n", class_name);
		return xml_node;
	}

    if(equal_strings(filename, current_filename))
    {
        Notify(msg_fatal_error, "Class ikc for \"%s\" can not include itself. Check that <link> is used to set the code class.\n", class_name);
        return xml_node;
    }

	XMLDocument * cDoc = new XMLDocument(filename);
	XMLElement * cgroup = cDoc->xml;
	cDoc->xml = NULL;
	delete cDoc;
	
	// 1. Replace the module element with the group element of the included file
	
	cgroup->next = xml_node->next;
	if(xml_node->prev == NULL)
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
	if(last == NULL)
		cgroup->content = xml_node->content;
	else
	{
		while (last->next != NULL)
			last = last->next;
		last->next = xml_node->content;
	}
	xml_node->content = NULL;
	
	// 4. Build the class group
	
	BuildGroup(group, cgroup, class_name, filename);
	
	// 5. Delete original element and replace it with the newly merged group
	
	xml_node->next = NULL;
	delete xml_node;
	
	return cgroup;
}


// Parse XML for a group
//
// FIXME: names of should never be inherited!!!
//

GroupElement *
Kernel::BuildGroup(GroupElement * group, XMLElement * group_xml, const char * current_class, const char * current_filename)
{
    if(!group_xml->GetActualAttribute("name"))
        group_xml->SetAttribute("name", create_formatted_string("Group-%d", group_number++));

    // Add attributes to group element
    
    for(XMLAttribute * attr=group_xml->attributes; attr!=NULL; attr = (XMLAttribute *)attr->next)
        group->attributes.insert({ attr->name, attr->value });
    
    for (XMLElement * xml_node = group_xml->GetContentElement(); xml_node != NULL; xml_node = xml_node->GetNextElement())
    {
        if(xml_node->IsElement("link"))    // Link to code
        {
            const char * class_name = GetXMLAttribute(group_xml, "class");
            Parameter * parameter = new Parameter(this, xml_node, group);
            Module * m = classes.count(class_name) ? classes.at(class_name)->CreateModule(parameter) : NULL;
            delete parameter;
            if(m == NULL)
                Notify(msg_warning, "Could not create module: Class \"%s\" does not exist.\n", class_name);
            else if(useThreads && m->phase != 0)
                Notify(msg_fatal_error, "phase != 0 not yet supported in threads.");
            xml_node->aux = (void *)m;
            group->module = m;  // FIXME: test if correct
            AddModule(m);
        }
        
        else if(xml_node->IsElement("module"))	// Add module
        {
            char * class_name = create_string(GetXMLAttribute(xml_node, "class"));
            GroupElement * subgroup = new GroupElement(group);
            xml_node = BuildClassGroup(subgroup, xml_node, class_name, current_filename); // TODO: merge with line above
            auto n = xml_node->GetActualAttribute("name");
            if(group->groups.count(n))
                Notify(msg_fatal_error, "Duplicate module name \"%s\".", n);
            else
                group->groups.insert({ n, subgroup });
            destroy_string(class_name);
        }
        else if(xml_node->IsElement("group"))	// Add group
        {
            auto n = xml_node->GetActualAttribute("name");
            if(!n)
                xml_node->SetAttribute("name", create_formatted_string("Group-%d", group_number++));
            GroupElement * g = new GroupElement(group);

            if(group->groups.count(n))
                Notify(msg_fatal_error, "Duplicate group name \"%s\".", n);
            else
                group->groups.insert( { n, BuildGroup(g, xml_node) });
        }
    
        else if(xml_node->IsElement("parameter"))
        {
            auto * p = new ParameterElement(group, xml_node);
            group->parameter_list.push_back(p);
            if(const char * target = xml_node->GetAttribute("target"))
                group->parameters.insert( { target, p }); // parameter do not have targets in classes
        }
        
        else if(xml_node->IsElement("input"))
            group->inputs.push_back(InputElement(group, xml_node));
            
        else if(xml_node->IsElement("output"))
        {
            if(const char * name = xml_node->GetAttribute("name"))
                group->outputs.insert( { name, new OutputElement(group, xml_node) }); // LOOK HERE!
            else
                Notify(msg_fatal_error, "Output element does not have a name.");
        }

        else if(xml_node->IsElement("connection"))
             group->connections.push_back(ConnectionElement(group, xml_node));
     
        else if(xml_node->IsElement("view"))
        {
            ViewElement v(group, xml_node);
            for(XMLElement * xml_obj = xml_node->GetContentElement(); xml_obj != NULL; xml_obj = xml_obj->GetNextElement()) // WAS "object"
            {
                ViewObjectElement o((GroupElement *)&v, xml_obj); // FIXME: is this correct?
                o.attributes.insert({ "class", xml_obj->name }); // FIXME: WHY??? Needed for widgets to work for some reason
                v.objects.push_back(o);
            }
            group->views.push_back(v);
        }
    }
    
    if(group->views.empty())
        group->CreateDefaultView();
    
    return group;
}


void
Kernel::ConnectModules(GroupElement * group, std::string indent) // FIXME: remove indent
{
    for(auto & c : group->connections)
    {
        auto source = c["source"]; // FIXME: use count and at instead
        auto target = c["target"];
        try {
            Module_IO * source_io = NULL;
            if(source.starts_with("."))
                source_io = main_group->GetSource(source.substr(1));
            else
                source_io = group->GetSource(source);

            if(!source_io)
                Notify(msg_fatal_error, "Connection source %s not found.\n", source.c_str());

            int cnt = 0;
            if(target.starts_with("."))
                for(auto target_io : main_group->GetTargets(target.substr(1)))
                    cnt += Connect(source_io, string_to_int(c["sourceoffset"]), target_io, string_to_int(c["targetoffset"]), string_to_int(c["size"], unknown_size), c["delay"], 0, string_to_bool(c["active"], true));
            else
                for(auto target_io : group->GetTargets(target))
                    cnt += Connect(source_io, string_to_int(c["sourceoffset"]), target_io, string_to_int(c["targetoffset"]), string_to_int(c["size"], unknown_size), c["delay"], 0, string_to_bool(c["active"], true));

            if(cnt == 0)
                Notify(msg_fatal_error, "Connection target %s not found.\n", target.c_str());
        }
        catch(...)
        {
            Notify(msg_fatal_error, "Could not connect %s to %s.\n", source.c_str(), target.c_str());
        }
    }

    for(auto & g : group->groups) // Connect in subgroups
        ConnectModules(g.second, indent+"\t");
}


bool
Kernel::ReadXML()
{
    if(ikc_file_name[0] == 0)
        return Notify(msg_fatal_error, "Empty file name.\n");

    char path[PATH_MAX];
    copy_string(path, ikc_dir, PATH_MAX);
    append_string(path, ikc_file_name, PATH_MAX);
    Notify(msg_print, "Running \"%s\".\n", ikc_file_name);
    if(chdir(ikc_dir) < 0)
        return Notify(msg_fatal_error, "The directory \"%s\" could not be found.\n", ikc_dir);

    xmlDoc = new XMLDocument(ikc_file_name, false);
    if(xmlDoc->xml == NULL)
        return Notify(msg_fatal_error, "Could not read (or find) \"%s\".\n", ikc_file_name);

    XMLElement * xml = xmlDoc->xml->GetElement("group");
    if(xml == NULL)
        return Notify(msg_fatal_error, "Did not find <group> element in IKG/XML file \"%s\".\n", ikc_file_name);
    
    // Set default parameters
    
    xml->SetAttribute("log_level", create_formatted_string("%d", log_level)); // FIXME: period???

    // 2.0 create top group
    
    main_group = new GroupElement(NULL);
    
    // set session id
    
    std::time_t result = std::time(nullptr);
    main_group->attributes.insert({ "session-id", std::to_string(result) });
    session_id = result; // temporary, get from top level group
    
    BuildGroup(main_group, xml);
    if(fatal_error_occurred)
        return false;
    
    ConnectModules(main_group);

#ifdef XML_PRINT
    xmlDoc->Print(stdout);
#endif
    
    return true;
}


void
Kernel::ListInfo()
{
    if(!options->GetOption('i') && !options->GetOption('a')) return;
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
Kernel::CalculateChecksum()
{
    if(_modules.empty())
        return;

    std::string c = main_group->GetValue("checksum");
    if(c.empty())
        return;

    int target_checksum = std::stoi(c);
    int checksum = 1;
    for (Module * & m : _modules)
        for (Module_IO * i = m->output_list; i != NULL; i = i->next)
            checksum *= i->sizex * i->sizey;

    if(checksum == target_checksum)
        Notify(msg_print, "Checksum match (%d).", checksum);
    else
        Notify(msg_print, "Checksum mismatch: %d != %d.\n", checksum, target_checksum);
}


void
Kernel::ListModulesAndConnections()
{
    if(!options->GetOption('m') && !options->GetOption('a')) return;
    
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
        //        Notify(msg_print, "  %s (%s) [%d, %d]:\n", m->name, m->class_name, m->period, m->phase);
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
        if(c->delay == 0)
            Notify(msg_print, "  %s.%s[%d..%d] == %s.%s[%d..%d] (%d) %s\n",
                   c->source_io->module->instance_name, c->source_io->name.c_str(), 0, c->source_io->size-1,
                   c->target_io->module->instance_name, c->target_io->name.c_str(), 0, c->source_io->size-1,
                   c->delay,
                   c->active ? "" : "[inactive]");
        else if(c->size > 1)
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
Kernel::ListBindings()
{
    return;
    Notify(msg_print, "\n");
    Notify(msg_print, "Bindings:\n");
    Notify(msg_print, "\n");
    for(auto b : bindings)
        Notify(msg_print, "\t%s\n", b.first.c_str());
}


void
Kernel::ListThreads()
{
    if(!options->GetOption('T') && !(options->GetOption('a') && options->GetOption('t'))) return;
    Notify(msg_print, "\n");
    Notify(msg_print,"ThreadManagers:\n");
    Notify(msg_print, "\n");
    int i=0;
    for(ThreadGroup * tg : _threadGroups)
    {
        Notify(msg_print, "ThreadManager %d [Period:%d, Phase:%d, High_priority:%d]\n", i++, tg->period, tg->phase, tg->high_priority);
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
    if(!options->GetOption('l') && !options->GetOption('a')) return;

    if(_modules.empty())
        return;
    
    Notify(msg_print, "Scheduling:\n");
    Notify(msg_print, "\n");
    for (int t=0; t<period_count; t++)
    {
        int tm = 0;
        for (Module * & m : _modules)
            if(t % m->period == m->phase)
                Notify(msg_print,"  %02d.%02d: %s (%s)\n", t, tm++, m->GetName(), m->GetClassName());
    }
    
    Notify(msg_print, "\n");
    Notify(msg_print, "\n");
}


void
Kernel::ListClasses()
{
    if(!options->GetOption('c') && !options->GetOption('a')) return;
    int i = 0;
    Notify(msg_print, "\n");
    Notify(msg_print, "Classes:\n");
    
    for(auto & c: classes)
    {
        Notify(msg_print, "\t%s\n", c.first.c_str());
        i++;
    }
    Notify(msg_print, "No of classes: %d.\n", i);
}


void
Kernel::ListProfiling()
{
    if(!options->GetOption('p') && !options->GetOption('P')) return;
    // Calculate Total Time
    float total_module_time = 0;
    float non_listed_modules = 0;
    float limit = (options->GetOption('p') ? 1.0 : 0.01);
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
    if(total_module_time == 0)
        return;
    Notify(msg_print, "Time in Each Module:\n");
    Notify(msg_print, "\n");
    Notify(msg_print, "%-20s%-20s%10s%10s%10s\n", "Module", "Class", "Count", "Avg (ms)", "Time %");
    Notify(msg_print, "----------------------------------------------------------------------\n");
    for (Module * & m : _modules)
        if(m->ticks > 0 && m->time/m->ticks >= limit)
            Notify(msg_print, "%-20s%-20s%10.0f%10.2f%10.1f\n", m->GetName(), m->GetClassName(), m->ticks, (m->time/m->ticks), 100*(m->time/total_module_time));
        else
            non_listed_modules += (m->time/m->ticks);
//            Notify(msg_print, "%-20s%-20s%       ---f\n", m->GetName(), m->GetClassName());
    Notify(msg_print, "----------------------------------------------------------------------\n");
    if(useThreads)
        Notify(msg_print, "Note: Time is real-time, not time in thread.\n");
    if(non_listed_modules > 0)
        Notify(msg_print, "Additional modules used on average %0.2f ms together per tick\n", non_listed_modules);
    Notify(msg_print, "\n");
}


// The following lines will create the kernel the first time it is accessed by one of the modules

Kernel& kernel()
{
    static Kernel * kernelInstance = new Kernel();
    return * kernelInstance;
}



// ------------------------------------------------------------------------------------------------------------------------------------------------------------
// WebUI STARTS HERE
// ------------------------------------------------------------------------------------------------------------------------------------------------------------

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
    socket->Send("\"");
    
    destroy_jpeg((char *)jpeg);
    free(jpeg_base64);
    return ok;
}


static bool
SendPseudoColorJPEGbase64(ServerSocket * socket, float * m, int sizex, int sizey, std::string type)
{
    long  size;
    unsigned char * jpeg = NULL;

    if(type == "red")
        jpeg = (unsigned char *)create_jpeg(size, m, sizex, sizey, LUT_red);

    else if(type == "green")
        jpeg = (unsigned char *)create_jpeg(size, m, sizex, sizey, LUT_green);

    else if(type == "blue")
        jpeg = (unsigned char *)create_jpeg(size, m, sizex, sizey, LUT_blue);

    else if(type == "fire")
        jpeg = (unsigned char *)create_jpeg(size, m, sizex, sizey, LUT_fire);

    else if(type == "spectrum")
        jpeg = (unsigned char *)create_jpeg(size, m, sizex, sizey, LUT_spectrum);

    else
        return false;
    
    size_t input_length = size;
    size_t output_length;
    char * jpeg_base64 = base64_encode(jpeg, input_length, &output_length);
    
    socket->Send("\"data:image/jpeg;base64,");
    bool ok = socket->SendData(jpeg_base64, output_length);
    socket->Send("\"");
    
    destroy_jpeg((char *)jpeg);
    free(jpeg_base64);
    return ok;
}




std::string
checkNumber(float x)
{
    if(std::isfinite(x)) // Check data
        return std::to_string(x);
    else
        return "\"NAN\"";
}



static bool
SendJSONArrayData(ServerSocket * socket, const std::string & source, float * array, int size)
{
    if (array == NULL)
        return false;
    
    socket->fillBuffer("\t\t\"" + source + "\":\n\t\t[\n");
    socket->fillBuffer("\t\t[" + checkNumber(array[0]));
    for (int i=1; i<size; i++)
        socket->fillBuffer("," + checkNumber(array[i]));
    socket->fillBuffer("]\n\t]");
    socket->SendBuffer();
    socket->clearBuffer();
    return true;
}



static bool
SendJSONMatrixData(ServerSocket * socket, const std::string & source, float * matrix, int sizex, int sizey)  // FIXME: use float ** instead
{
    if (matrix == NULL)
        return false;
    
    int k = 0;

    socket->fillBuffer("\t\t\"" + source + "\":\n\t\t[\n");
    for (int j=0; j<sizey; j++)
    {
        socket->fillBuffer("\t\t\t[" + checkNumber(matrix[k++]));
        for (int i=1; i<sizex; i++)
            socket->fillBuffer("," + checkNumber(matrix[k++]));
        if (j<sizey-1)
            socket->fillBuffer("],\n");
        else
            socket->fillBuffer("]\n\t\t]");
    }
    socket->SendBuffer();
    socket->clearBuffer();
    return true;
}




void
Kernel::DoSendData(std::string uri, std::string args)
{    
    sending_ui_data = true; // must be set while main thread is still running
    while(tick_is_running)
        {}

std::string data = cut(args, "data=");
std::string root = head(data, "#");

    Dictionary header;
    header.Set("Session-Id", std::to_string(session_id).c_str()); // FIXME: GetValue("session_id")
    header.Set("Package-Type", "data");
    header.Set("Content-Type", "application/json");
    header.Set("Cache-Control", "no-cache");
    header.Set("Cache-Control", "no-store");
    header.Set("Pragma", "no-cache");
    header.Set("Expires", "0");
    
    socket->SendHTTPHeader(&header);
    
    socket->Send("{\n");
    socket->Send("\t\"state\": %d,\n", ui_state);
    
    if(max_ticks > 0)
    {
        socket->Send("\t\"iteration\": \"%d / %d\",\n", GetTick(), max_ticks);
        socket->Send("\t\"progress\": %f,\n", float(tick)/float(max_ticks));
    }
    else
    {
        socket->Send("\t\"iteration\": %lld,\n", GetTick());
        socket->Send("\t\"progress\": 0,\n");
    }

    // Timing information
    
    float total_time = timer->GetTime()/1000.0; // in seconds

    socket->Send("\t\"timestamp\": %ld,\n", Timer::GetRealTime());
    socket->Send("\t\"total_time\": %.2f,\n", total_time);
    socket->Send("\t\"ticks_per_s\": %.2f,\n", float(tick)/total_time);
    socket->Send("\t\"timebase\": %d,\n", tick_length);
    socket->Send("\t\"timebase_actual\": %.0f,\n", tick > 0 ? 1000*float(total_time)/float(tick) : 0);
    socket->Send("\t\"lag\": %.0f,\n", lag);
    socket->Send("\t\"cpu_cores\": %d,\n", cpu_cores);
    socket->Send("\t\"time_usage\": %.3f,\n", time_usage);  // TODO: move to kernel from WebUI
    socket->Send("\t\"cpu_usage\": %.3f", cpu_usage);

    socket->Send(",\n\t\"data\":\n\t{\n");
    std::string sep = "";

    while(!data.empty())
    {
        std::string source = head(data, "#");
        std::string  format = rcut(source, ":");

        auto root_group = main_group->GetGroup(root);
        
        std::string src = source;
        if(!root.empty())    // FIXME: Empty or Not empty?
            src = root+"."+src;
        
        if(root_group && !source.empty())
        {
            if(format == "") // as default, send a matrix
            {
                Module_IO * io = root_group->GetSource(source); // FIXME: Also look for inputs here
                if(io)
                {
                    socket->Send(sep.c_str());
                    SendJSONMatrixData(socket, source, *io->matrix[0], io->sizex, io->sizey);
                    sep = ",\n";
                }
                else if(bindings.count(src))
                {
                    socket->Send(sep.c_str());
                    auto bs = bindings.at(src);
                    Binding * b = bs.at(0);   // Use first binding
                    switch(b->type)
                    {
                        case data_source_int:
                        case data_source_list:
                            socket->Send("\t\t\"%s\": [[%d]]", source.c_str(), *(int *)(b->value));
                            break;

                        case data_source_bool:
                            socket->Send("\t\t\"%s\": [[%d]]", source.c_str(), int(*(bool *)(b->value)));
                            break;

                        case data_source_float:
                            socket->Send("\t\t\"%s\": [[%f]]", source.c_str(), *(float *)(b->value));
                            break;

                        case data_source_string:
                            socket->Send("\t\t\"%s\": \"%s\"", source.c_str(), ((std::string *)(b->value))->c_str());
                            break;
         
                        case data_source_matrix:
                            SendJSONMatrixData(socket, source, *(float **)(b->value), b->size_x, b->size_y);
                            break;
                            
                        case data_source_array:
                            SendJSONArrayData(socket, source, (float *)(b->value), b->size_x);
                            break;
                            
                        default:
                            socket->Send("\"ERRROR_type_is\": %d", b->type);
                    }
                    sep = ",\n";
                }
            }
            else if(format == "gray" && source[0])
            {
                if(Module_IO * io = root_group->GetSource(source))
                {
                    socket->Send(sep.c_str());
                    socket->Send("\t\t\"%s:gray\": ", source.c_str());
                    SendColorJPEGbase64(socket, *io->matrix[0], *io->matrix[0], *io->matrix[0], io->sizex, io->sizey);
                    sep = ",\n";
                }
            }
            else if(format == "rgb" && !source.empty())
            {
                auto a = rsplit(source, ".", 1); // separate out outputs
                auto o = split(a[1], "+"); // split channel names
                
                if(o.size() == 3)
                {
                    auto c1 = a[0]+"."+o[0];
                    auto c2 = a[0]+"."+o[1];
                    auto c3 = a[0]+"."+o[2];

                    Module_IO * io1 = root_group->GetSource(c1);
                    Module_IO * io2 = root_group->GetSource(c2);
                    Module_IO * io3 = root_group->GetSource(c3);
                    
                    // TODO: check that all outputs have the same size

                    if(io2 && io2 && io3)
                    {
                        socket->Send(sep.c_str());
                        socket->Send("\t\t\"%s:rgb\": ", source.c_str());
                        SendColorJPEGbase64(socket, *io1->matrix[0], *io2->matrix[0], *io3->matrix[0], io1->sizex, io1->sizey);
                        sep = ",\n";
                    }
                }
            }
            else if(source[0] && (format == "fire" || format == "spectrum" || format == "red" || format == "green" || format == "blue"))
            {
                if(Module_IO * io = root_group->GetSource(source))
                {
                    socket->Send(sep.c_str());
                    socket->Send("\t\t\"%s:%s\": ", source.c_str(), format.c_str());
                    SendPseudoColorJPEGbase64(socket, *io->matrix[0], io->sizex, io->sizey, format.c_str());
                    sep = ",\n";
                }
            }
        }
    }
    socket->Send("\n\t}");

    if(tick_is_running) // new tick has started during sending
    {
        socket->Send(",\n\t\"has_data\": 0\n"); // there may be data but it cannot be trusted
    }
    else
    {
        socket->Send(",\n\t\"has_data\": 1\n");
    }
    socket->Send("}\n");
    
    sending_ui_data = false;
}


void
Kernel::Pause()
{
    isRunning = false;
    while(tick_is_running)
        ;
}



long
get_client_id(std::string & args)
{
    std::string id_string =  head(args, "&");
    std::string id_number = cut(id_string, "id=");

    try
    {
        return stol(id_number);
    }
    catch(const std::invalid_argument)
    {
        return 0;
    }
}



void
Kernel::DoSendNetwork(std::string uri, std::string args)
{
        std::string s = JSONString();
        Dictionary rtheader;
        rtheader.Set("Session-Id", std::to_string(session_id).c_str());
        rtheader.Set("Package-Type", "network");
        rtheader.Set("Content-Type", "application/json");
        rtheader.Set("Content-Length", int(s.size()));
        socket->SendHTTPHeader(&rtheader);
        socket->SendData(s.c_str(), int(s.size()));
}



void
Kernel::DoStop(std::string uri, std::string args)
{
        Pause();
        ui_state = ui_state_stop;
        Notify(msg_terminate, "Sent by WebUI.\n");
        DoSendData(uri, args);
}



void
Kernel::DoPause(std::string uri, std::string args)
{
    Pause();
    ui_state = ui_state_pause;
    master_id = get_client_id(args);
    DoSendData(uri, args);
}



void
Kernel::DoStep(std::string uri, std::string args)
{
    Pause();
    ui_state = ui_state_pause;
    master_id = get_client_id(args);
    Tick();
    DoSendData(uri, args);
}




void
Kernel::DoPlay(std::string uri, std::string args)
{
        Pause();
        ui_state = ui_state_play;
        master_id = get_client_id(args);
        Tick();
    DoSendData(uri, args);
}




void
Kernel::DoRealtime(std::string uri, std::string args)
{
    ui_state = ui_state_realtime;
    master_id = get_client_id(args);
    isRunning = true;
    DoSendData(uri, args);
}



void
Kernel::DoCommand(std::string uri, std::string args)
{
    float x, y;
    char command[255];
    char value[1024]; // FIXME: no range chacks
    int c = sscanf(uri.c_str(), "/command/%[^/]/%f/%f/%[^/]", command, &x, &y, value);
    if(c == 4)
        SendCommand(command, x, y, value);
    DoSendData(uri, args);
}



void
Kernel::DoControl(std::string uri, std::string args)
{
    char module_name[255];
    char parameter[255];
    int x, y;
    float value;
    int c = sscanf(uri.c_str(), "/control/%[^/]/%d/%d/%f", parameter, &x, &y, &value);
    if(c == 4)

        SetParameter(parameter, x, y, value); // TODO: check if groups are handled correctly
    DoSendData(uri, args);
}




void
Kernel::DoUpdate(std::string uri, std::string args)
{
    if(args.empty() || first_request) // not a data request - send view data
    {
        first_request = false;
            DoSendNetwork(uri, args);
    }
    else if(ui_state == ui_state_play && master_id == get_client_id(args))
    {
        Pause();
        Tick();
        DoSendData(uri, args);
    }
    else 
        DoSendData(uri, args);
}



void
Kernel::DoGetLog(std::string uri, std::string args)
{
    if (logfile)
        socket->SendFile("logfile", webui_dir);
    else
        socket->Send("ERROR - No logfile found\n");
}




void
Kernel::DoSendClasses(std::string uri, std::string args)
{
    Dictionary header;
    header.Set("Content-Type", "text/json");
    header.Set("Cache-Control", "no-cache");
    header.Set("Cache-Control", "no-store");
    header.Set("Pragma", "no-cache");
    socket->SendHTTPHeader(&header);
    socket->Send("{\"classes\":[\n\t\"");
    std::string s = "";
    for(auto & c: classes)
    {
        socket->Send(s.c_str());
        socket->Send(c.first.c_str());
        s = "\",\n\t\"";
    }
    socket->Send("\"\n]\n}\n");
}



void
Kernel::DoSendFile(std::string file)
{
        if(file[0] == '/')
            file = file.erase(0,1); // Remove initial slash

        if(socket->SendFile(file.c_str(), ikc_dir))  // Check IKC-directory first to allow files to be overriden
            return;

        if(socket->SendFile(file.c_str(), webui_dir))   // Now look in WebUI directory
            return;
        
        file = "error." + rcut(file, ".");
        if(socket->SendFile(("error." + rcut(file, ".")).c_str(), webui_dir)) // Try to send error file
            return;

        DoSendError();
}


void
Kernel::DoSendError()
{
    Dictionary header;
    header.Set("Content-Type", "text/plain");
    socket->SendHTTPHeader(&header);
    socket->Send("ERROR\n");
}



void
Kernel::HandleHTTPRequest()
{
    std::string uri = socket->header.Get("URI");
    //printf(">>>%s\n", uri.c_str());
    if(uri.empty())
    {
        Notify(msg_warning, "No URI");
        return;
    }

    std::string args = cut(uri, "?");

    // SELECT METHOD

    if(uri == "/update")
        DoUpdate(uri, args);

    else if(uri == "/pause")
        DoPause(uri, args);

    else if(uri == "/step")
        DoStep(uri, args);


    else if(uri == "/play")
        DoPlay(uri, args);

    else if(uri == "/realtime")
        DoRealtime(uri, args);

    else if(uri == "/stop")
        DoStop(uri, args);

    else if(uri == "/getlog")
        DoGetLog(uri, args);
    
    else if(uri == "/classes") 
        DoSendClasses(uri, args);

    else if(uri == "/")
       DoSendFile("index.html");

    else if(uri.starts_with("/command/"))
        DoCommand(uri, args);
        
    else if(uri.starts_with("/control/"))
        DoControl(uri, args);

    else 
        DoSendFile(uri);
}



void
Kernel::HandleHTTPThread()
{
    while(!Terminate())
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
Kernel::StartHTTPThread(Kernel * k)
{
    k->HandleHTTPThread();
    return NULL;
}
