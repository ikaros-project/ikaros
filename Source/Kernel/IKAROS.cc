//
//	  IKAROS.cc		Kernel code for the IKAROS project
//
//    Copyright (C) 2001-2016  Christian Balkenius
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
//	Revision History:
//
//		2002-01-27	Added support for sizing of output arrays depending on input sizes
//		2002-02-06	Added x, y of each array instead of width
//		2002-02-10	Added new XML support that does not depend on expat for better portability
//		2002-02-16	Additional support for matrices: GetInputMatrix() and GetOutputMatrix()
//					to make image processing easier
//		2002-03-15	Added socket for UI communication
//		2002-05-20 	Added new communication protocol
//		2002-10-06	Minor changes in parameter class
//		2003-01-23	Bug in InitInputs() fixed
//		2003-08-09	Now catches ctrl-C to shut down gracefully
//
//		2004-03-02	Version 0.8.0 created
//		2004-11-15	New print and error functions; now use Notify(msg, ...) for both errors and printing
//		2004-11-27	Added defines around CTRL-C handler
//		2005-01-17	Changed Data to Array
//		2005-01-18	Completed timing functions
//		2005-08-31	All communication code moved to WebUI
//		2006-01-20	Major cleanup of the code; most system specific code moved out of the kernel
//		2006-02-10	Extended error handling
//		2006-05-05	Even more extended error handling; more informative error messages
//		2006-08-31	Most Windows-specific code included
//		2006-12-12	Fixed potential memory leaks caused by old XML parser
//		2007-01-10	Added new XML handling and new module creation for ikc files

//		2007-05-10	Version 1.0.0 created
//		2007-07-05	Malloc debugging added
//		2008-12-28  All legacy support and deprecated functions removed to simplify XML cleanup
//      Revision history now maintained at GitHUb

#include "IKAROS.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <limits.h>
#include <fcntl.h>
#include <ctype.h>


#ifdef WINDOWS
#include <direct.h>
#else
#include <unistd.h>
#endif

#ifdef WINDOWS
#include <windows.h>
#undef GetClassName
#else
#include <unistd.h>
#endif

#include <exception> // for std::bad_alloc
#include <new>



using namespace ikaros;

bool			global_fatal_error = false;	// Must be global because it is used before the kernel is created
bool			global_terminate = false;	// Used to flag that CTRL-C has been received
int             global_error_count = 0;
int             global_warning_count = 0;

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
        if (module != NULL)
            module->Notify(msg_fatal_error, "Attempting to allocate io (\"%s\") with unknown size for module \"%s\" (%s). Check that all required inputs are connected.\n", name, module->GetName(), module->GetClassName());
        return;
    }
    if (module != NULL) module->Notify(msg_verbose, "Allocating data of size %d.\n", size);
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
    if (name != NULL)
    {
        if (module != NULL) module->Notify(msg_verbose, "      Deleting Module_IO \"%s\".\n", name);
    }
    else
    {
        if (module != NULL) module->Notify(msg_verbose, "      Deleting Module_IO\n");
    }
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
            module->Notify(msg_fatal_error, "Module_IO::SetSize: Attempt to resize data array \"%s\" of module \"%s\" (%s) (%d <= %d). Ignored.\n",  name, module->GetName(), module->GetClassName(), size, s);
        return;
    }
    if (s == size)
        return;
    if (s == 0)
        return;
    if (module != NULL) module->Notify(msg_verbose, "Allocating memory for input/output \"%s\" of module \"%s\" (%s) with size %d and max_delay = %d (in SetSize).\n", name, module->instance_name, module->GetClassName(), s, max_delay);
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
    Notify(msg_verbose, "    Deleting Module \"%s\".\n", instance_name);
	destroy_string(full_instance_name);
    delete timer;
    delete input_list;
    delete output_list;
    delete next;
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
    Notify(msg_verbose, "  Adding input \"%s\".\n", name);
}

void
Module::AddOutput(const char * name, int sizeX, int sizeY, bool optional)
{
    if (GetModule_IO(output_list, name) != NULL)
    {
        Notify(msg_warning, "Output \"%s\" of module \"%s\" (%s) already exists.\n", name, GetName(), GetClassName());
        return;
    }
    output_list = new Module_IO(output_list, this, name, sizeX, sizeY, optional);
    Notify(msg_verbose, "  Adding output \"%s\" of size %d x %d to module \"%s\" (%s).\n", name, sizeX, sizeY, GetName(), GetClassName());
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
        return value;
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
            return value;
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

static bool
tobool(const char * v)
{
    if (!v) return false;
    if (!strcmp(v, "true")) return true;
    if (!strcmp(v, "True")) return true;
    if (!strcmp(v, "TRUE")) return true;
    if (!strcmp(v, "yes")) return true;
    if (!strcmp(v, "Yes")) return true;
    if (!strcmp(v, "YES")) return true;
    if (!strcmp(v, "1")) return true;
    if (!strcmp(v, "false")) return false;
    if (!strcmp(v, "False")) return false;
    if (!strcmp(v, "FALSE")) return false;
    if (!strcmp(v, "no")) return false;
    if (!strcmp(v, "No")) return false;
    if (!strcmp(v, "NO")) return false;
    if (!strcmp(v, "0")) return false;
    return false;
}

bool
Module::GetBoolValue(const char * n, bool d)
{
    const char * v = GetValue(n);
    if (v == NULL)
        return d;
    else
        return tobool(v);
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
Module::GetArray(const char * n, int size)
{
    bool too_few = false;
    float * a = create_array(size);
    const char * v = GetValue(n);
    if (v == NULL)
	{
		destroy_array(a);
        return NULL;
	}
    for (int i=0; i<size;i++)
    {
        for (; isspace(*v) && *v != '\0'; v++) ;
        if (sscanf(v, "%f", &a[i])==-1)
        {
            too_few = true;
            a[i] = 0;
        }
        for (; !isspace(*v) && *v != '\0'; v++) ;
    }
    if(too_few)
        Notify(msg_warning, "Too few constants in array \"%s\" (0 assumed).\n", n);
    return a;
}



int *
Module::GetIntArray(const char * n, int & size)
{
    size = 0;
    const char * v = GetValue(n);
    if (v == NULL)
        return NULL;
    const char * vv = v;
    
    // Count values
    
    while(*v != '\0')
    {
        int x;
        for (; isspace(*v) && *v != '\0'; v++) ;
        if (sscanf(v, "%d", &x)!=-1)
            size++;
        for (; !isspace(*v) && *v != '\0'; v++) ;
    }
    
    int * a = new int[size];
    v = vv;
    
    for (int i=0; i<size;i++)
    {
        for (; isspace(*v) && *v != '\0'; v++) ;
        if (sscanf(v, "%d", &a[i])==-1)
            a[i] = 0; // this should never happen
        for (; !isspace(*v) && *v != '\0'; v++) ;
    }
    
    return a;
}



float **
Module::GetMatrix(const char * n, int & sizex, int & sizey)
{
    return create_matrix(GetValue(n), sizex, sizey);
}



/*
float **
Module::GetMatrix(const char * n, int sizex, int sizey)
{
    float ** m = create_matrix(sizex, sizey);
    const char * v = GetValue(n);
    if (v == NULL)
        return m;
    
    int sx, sy;
    float ** M = create_matrix(v, sx, sy);
    
    if(sy == 1 && sizey > 1) // for backward compatibility, get all data from one row
    {
        int p = 0;
        for(int j=0; j<sizey; j++)
            for(int i=0; i<sizex; i++)
            {
                m[j][i] = M[0][p++];
                if(p >= sx)
                    break;
            }
    }

    else
    {
        sx = min(sx, sizex);
        sy = min(sy, sizey);

        for(int i=0; i<sx; i++)
            for(int j=0; j<sy; j++)
                m[j][i] = M[j][i];
    }

    destroy_matrix(M);

    return m;
}
*/






void
Module::Bind(float & v, const char * n)
{
    // TODO: check type here
    v = GetFloatValue(n);
    bindings = new Binding(this, n, bind_float, &v, 0, 0, bindings);
}



void
Module::Bind(float * & v, int size, const char * n)
{
    // TODO: check type here
    v = GetArray(n, size);
    bindings = new Binding(this, n, bind_array, v, size, 1, bindings);
}



void
Module::Bind(float ** & v, int & sizex, int & sizey, const char * n)
{
    // TODO: check type here
    v = GetMatrix(n, sizex, sizey);
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



Module_IO *
Module::GetModule_IO(Module_IO * list, const char * name)
{
    for (Module_IO * i = list; i != NULL; i = i->next)
        if (equal_strings(name, i->name))
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
        if (equal_strings(name, i->name))
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
        if (equal_strings(name, i->name))
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
        if (equal_strings(name, i->name))
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
        if (equal_strings(name, i->name))
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
        if (equal_strings(input_name, i->name))
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
Module::GetInputSizeX(const char * input_name)
{
    // Find the Module_IO for this input
    for (Module_IO * i = input_list; i != NULL; i = i->next)
        if (equal_strings(input_name, i->name))
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
Module::GetInputSizeY(const char * input_name)
{
    // Find the Module_IO for this input
    for (Module_IO * i = input_list; i != NULL; i = i->next)
        if (equal_strings(input_name, i->name))
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
        if (equal_strings(name, i->name))
            return i->size;
    Notify(msg_warning, "Attempting to get size of non-existing output %s.%s \n", this->instance_name, name);
    return 0;
}

int
Module::GetOutputSizeX(const char * name)
{
    for (Module_IO * i = output_list; i != NULL; i = i->next)
        if (equal_strings(name, i->name))
            return  i->sizex;
    Notify(msg_warning, "Attempting to get size of non-existing output %s.%s \n", this->instance_name, name);
    return 0;
}

int
Module::GetOutputSizeY(const char * name)
{
    for (Module_IO * i = output_list; i != NULL; i = i->next)
        if (equal_strings(name, i->name))
            return  i->sizey;
    Notify(msg_warning, "Attempting to get size of non-existing output %s.%s \n", this->instance_name, name);
    return 0;
}

bool
Module::InputConnected(const char * name)
{
    return kernel->InputConnected(this, name);
}

bool
Module::OutputConnected(const char * name)
{
    return kernel->OutputConnected(this, name);
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
        if (equal_strings(name, i->name))
            i->SetSize(x, y);
}

Module::Module(Parameter * p)
{
    next = NULL;
    next_in_threadGroup = NULL;
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
        bool multiple = (amc ? tobool(amc) : true); // True is defaut value
        AddInput(kernel->GetXMLAttribute(e, "name"), tobool(kernel->GetXMLAttribute(e, "optional")), multiple);
    }
    
    for(XMLElement * e=xml->GetParentElement()->GetContentElement("output"); e != NULL; e = e->GetNextElement("output"))
        AddOutput(kernel->GetXMLAttribute(e, "name"));
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
    while(l[i] != 0)
    {
        if(l[j] == ' ')
            j++;
        else
            l[i++]=l[j++];
    }
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
    while(l[i] != 0)
    {
        if(l[j] == ' ')
            j++;
        else
            l[i++]=l[j++];
    }
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

            if((sx == unknown_size) && (sizearg = kernel->GetXMLAttribute(e, "size_param_x")) && (arg = GetValue(sizearg)))
                sx = string_to_int(arg);
            
            if((sy == unknown_size) && (sizearg = kernel->GetXMLAttribute(e, "size_param_y")) && (arg = GetValue(sizearg)))
                sy = string_to_int(arg);
            
            if((sx == unknown_size) && (sizearg = kernel->GetXMLAttribute(e, "size_param")) && (arg = GetValue(sizearg)))
            {
                sx = string_to_int(arg);
                sy = 1;
            }
            
            if((sx == unknown_size) && (sizearg = kernel->GetXMLAttribute(e, "size_x")))
                sx = string_to_int(sizearg);
            
            if((sy == unknown_size) && (sizearg = kernel->GetXMLAttribute(e, "size_y")))
                sy = string_to_int(sizearg);
            
            if((sx == unknown_size) && (sizearg = kernel->GetXMLAttribute(e, "size")))
            {
                sx = string_to_int(sizearg);
                sy = 1;
            }
            
			if((sx == unknown_size) && (sy == unknown_size) && (sizearg = kernel->GetXMLAttribute(e, "size_set"))) // Set output size x & y from one or multiple inputs
            {
                sx = GetSizeXFromList(sizearg);
                sy = GetSizeYFromList(sizearg);
			}
            
			else if((sx == unknown_size) && (sizearg = kernel->GetXMLAttribute(e, "size_set_x")) && (sizeargy = kernel->GetXMLAttribute(e, "size_set_y")) ) // Set output size x from one or multiple different inputs for both x and y
			{
                sx = GetSizeXFromList(sizearg) * GetSizeYFromList(sizearg);     // Use total input sizes
                sy = GetSizeXFromList(sizeargy) * GetSizeYFromList(sizeargy);   // TODO: Check that no modules assumes it is ony X or Y sizes
			}
            
			else if((sx == unknown_size) && (sizearg = kernel->GetXMLAttribute(e, "size_set_x"))) // Set output size x from one or multiple inputs
                sx = GetSizeXFromList(sizearg);
            
			else if((sy == unknown_size) && (sizearg = kernel->GetXMLAttribute(e, "size_set_y"))) // Set output size y from one or multiple inputs
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
        global_warning_count++;
    }
}

void
Module::Notify(int msg, const char *format, ...)
{
    char 	message[512];
    sprintf(message, "%s (%s): ", GetName(), GetClassName());
    size_t n = strlen(message);
    va_list args;
    va_start(args, format);
    vsnprintf(&message[n], 512, format, args);
    va_end(args);
    if (kernel != NULL)
    {
        kernel->Notify(msg, message);
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
         global_warning_count++;
   }
}

Connection::Connection(Connection * n, Module_IO * sio, int so, Module_IO * tio, int to, int s, int d)
{
    source_io = sio;
    source_offset = so;
    target_io = tio;
    target_offset = to;
    size = s;
    delay = d;
    next = n;
}

Connection::~Connection()
{
    if (source_io != NULL && source_io->module != NULL)
        source_io->module->Notify(msg_verbose, "    Deleting Connection.\n");
    delete next;
}

void
Connection::Propagate(long tick)
{
    if (delay == 0)
        return;
    // Return if both modules will not start in this tick - necessary when using threads
    if (tick % source_io->module->period != source_io->module->phase)
        return;
    if (tick % target_io->module->period != target_io->module->phase)
        return;
    if (source_io != NULL && source_io->module != NULL)
        source_io->module->Notify(msg_verbose, "  Propagating %s.%s -> %s.%s (%p -> %p) size = %d\n", source_io->module->GetName(), source_io->name, target_io->module->GetName(), target_io->name, source_io->data, target_io->data, size);
    for (int i=0; i<size; i++)
        target_io->data[0][i+target_offset] = source_io->data[delay-1][i+source_offset];
}

void
ThreadGroup::AddModule(Module * m)
{
    // Add module if thread is empty
    if (modules == NULL)
    {
        kernel->Notify(msg_verbose, "Adding module %s to new thread group\n", m->GetName());
        modules = m;
        last_module = m;
        period = m->period;
        phase = m->phase;
        return;
    }
	
    // Check if any module already in the group preceedes the new one
    
    bool p = false;
    for(Module * pm = modules; pm != NULL; pm = pm->next_in_threadGroup)
        p = p | kernel->Precedes(pm, m);
    
    // Check for the case where this module preceedes a module that will be added to the group in the future
	
    if(!p)
        for(Module * pm = modules; pm != NULL; pm = pm->next_in_threadGroup)
            for(Module * nm = m->next; nm != NULL; nm = nm->next)
                if(kernel->Precedes(pm, nm) && kernel->Precedes(m, nm))
                {
                    p = true;
                    break;
                }
    
    // Add module if this module should run in same thread as the other (and last) modules
    if (p)
    {
        if(m->period != period)
        {
            kernel->Notify(msg_fatal_error, "Module %s do not have the correct period for thread group (Should be %d rather than %d)\n", m->GetName(), period, m->period);
            return;
        }
        
        kernel->Notify(msg_verbose, "Adding module %s to thread groups after %s\n", m->GetName(), last_module->GetName());
        last_module->next_in_threadGroup = m;
        last_module = m;
        return;
    }
    // Add to a new group if this was the last one
    if (next == NULL)
    {
        next = new ThreadGroup(kernel);
        next->AddModule(m);
        return;
    }
    // Try to add the module to the next group
    next->AddModule(m);
}

ThreadGroup::ThreadGroup(Kernel * k)
{
    kernel = k;
    next = NULL;
    modules = NULL;
    last_module = NULL;
    period = 1;
    phase = 0;
    thread = new Thread();
}

ThreadGroup::~ThreadGroup()
{
    delete thread;
    delete next;
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
        if (thread->Create(ThreadGroup_Tick, (void*)this))
            printf("Thread Creation Failed!\n");
    }
}

void
ThreadGroup::Stop(long tick)
{
    // Test if group should be joined
    if ((tick + 1) % period == phase)
    {
        thread->Join();
    }
}

void
ThreadGroup::Tick()
{
    for (Module * m = modules; m != NULL; m = m->next_in_threadGroup)
    {
        m->timer->Restart();
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
    
    print_mode          = print_normal;
    ikaros_dir          = NULL;
    ikc_dir             = NULL;
    ikc_file_name       = NULL;

    tick                = 0;
    xmlDoc              = NULL;
    classes             = NULL;
    modules             = NULL;
    connections         = NULL;
    module_count        = 0;
    period_count        = 0;
    phase_count         = 0;
    end_of_file_reached = false;
    fatal_error_occured	= false;
    terminate			= false;
    sizeChangeFlag      = false;
    threadGroups        = NULL;
    
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
    
    print_mode		= print_normal;
    if (options->GetOption('q'))
        print_mode = print_silent;
        if (options->GetOption('v'))
            print_mode = print_verbose;

    
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
#ifdef WINDOWS
        srand(string_to_int(options->GetArgument('z')));
#else
        srandom(string_to_int(options->GetArgument('z')));
#endif
    
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
    
    print_mode		= print_normal;
    if (options->GetOption('q'))
        print_mode = print_silent;
    if (options->GetOption('v'))
        print_mode = print_verbose;
    
    tick                = 0;
    xmlDoc              = NULL;
    classes             = NULL;
    modules             = NULL;
    connections         = NULL;
    module_count        = 0;
    period_count        = 0;
    phase_count         = 0;
    end_of_file_reached = false;
    fatal_error_occured	= false;
    terminate			= false;
    sizeChangeFlag      = false;
    threadGroups        = NULL;
    
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
#ifdef WINDOWS
    srand(string_to_int(options->GetArgument('z')));
#else
    srandom(string_to_int(options->GetArgument('z')));
#endif
}

Kernel::~Kernel()
{
    Notify(msg_verbose, "Deleting Kernel.\n");
    Notify(msg_verbose, "  Deleting Connections.\n");
    delete connections;
    Notify(msg_verbose, "  Deleting Modules.\n");
    delete modules;
    Notify(msg_verbose, "  Deleting Thread Groups.\n");
    delete threadGroups;
    Notify(msg_verbose, "  Deleting Classes.\n");
    delete classes;
    
    delete timer;
    delete xmlDoc;
    delete ikaros_dir;
    
    Notify(msg_verbose, "Deleting Kernel Complete.\n");
    if (logfile) fclose(logfile);
    
#ifdef USE_MALLOC_DEBUG
    dump_memory();  // dump blocks that are still allocated
#endif
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
    if (max_ticks != -1)
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
        Notify(msg_verbose, "Max ticks reached.\n");
        return true;
    }
    return end_of_file_reached || fatal_error_occured  || global_fatal_error || terminate || global_terminate;
}

void
Kernel::Run()
{
    if (fatal_error_occured || global_fatal_error)
    {
        Notify(msg_fatal_error, "Terminating because a fatal error occured.\n");
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
    for (Module * m = modules; m != NULL; m = m->next)
    {
        for (Module_IO * i = m->output_list; i != NULL; i = i->next)
        {
            for(int j=0; j<i->sizex*i->sizey; j++)
            {
                float v = i->matrix[0][0][j];
                if((v) != (v))
                {
                    Notify(msg_fatal_error, "NAN in output \"%s\" of module \"%s\" (%s).\n", i->name, i->module->instance_name, i->module->GetClassName());
                    break;
                }
            }
        }
    }
}

void
Kernel::CheckInputs()
{
    for (Module * m = modules; m != NULL; m = m->next)
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
                    Notify(msg_fatal_error, "Size of input \"%s\" of module \"%s\" (%s) could not be resolved.\n", i->name, i->module->instance_name, i->module->GetClassName());
                }
                else
                    i->size = 0; // ok if not connected
            }
    }
}

void
Kernel::CheckOutputs()
{
    for (Module * m = modules; m != NULL; m = m->next)
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
                    Notify(msg_fatal_error, "Size of output \"%s\" of module \"%s\" (%s) could not be resolved.\n", i->name, i->module->instance_name, i->module->GetClassName());
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
            Notify(msg_fatal_error, "Output \"%s\" of module \"%s\" (%s) has unknown size.\n", c->source_io->name, c->source_io->module->instance_name, c->source_io->module->GetClassName());
        }
        else if(c->target_io->data != NULL && c->target_io->allow_multiple == false)
        {
            Notify(msg_fatal_error, "Input \"%s\" of module \"%s\" (%s) does not allow multiple connections.\n", c->target_io->name, c->target_io->module->instance_name, c->target_io->module->GetClassName());
        }
        else if (c->delay == 0)
        {
            Notify(msg_verbose, "Short-circuiting zero-delay connection from \"%s\" of module \"%s\" (%s)\n", c->source_io->name, c->source_io->module->instance_name, c->source_io->module->GetClassName());
            // already connected to 0 or longer delay?
            if (c->target_io->data != NULL)
            {
                Notify(msg_fatal_error, "Failed to connect zero-delay connection from \"%s\" of module \"%s\" (%s) because target is already connected.\n", c->source_io->name, c->source_io->module->instance_name, c->source_io->module->GetClassName());
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
        else if(c->target_io)
        {
            // Check that this connection does not interfere with zero-delay connection
            if (c->target_io->max_delay == 0)
            {
                Notify(msg_fatal_error, "Failed to connect from \"%s\" of module \"%s\" (%s) because target is already connected with zero-delay.\n", c->source_io->name, c->source_io->module->instance_name, c->source_io->module->GetClassName());
            }
            // First connection to this target: initialize
            if (c->target_io->size == unknown_size)	// start calculation with size 0
                c->target_io->size = 0;
            int target_offset = c->target_io->size;
            c->target_io->size += c->source_io->size;
            // Target not used previously: ok to connect anything
            if (c->target_io->sizex == unknown_size)
            {
                Notify(msg_verbose, "New connection.\n");
                c->target_io->sizex = c->source_io->sizex;
                c->target_io->sizey = c->source_io->sizey;
            }
            // Connect one dimensional output
            else if (c->target_io->sizey == 1 && c->source_io->sizey == 1)
            {
                Notify(msg_verbose, "Adding additional connection.\n");
                c->target_io->sizex = c->target_io->size;
            }
            // Collapse matrix to array
            else
            {
                Notify(msg_verbose, "Multiple connections to \"%s.%s\" with different no of rows. Input flattened.\n", c->target_io->module->instance_name, c->target_io->name);
                c->target_io->sizex = c->target_io->size;
                c->target_io->sizey = 1;
            }
            // Set connection variables
            c->target_offset = target_offset;
            c->size = c->source_io->size;
            // Allocate input memory and reset
            Notify(msg_verbose, "Allocating memory for input \"%s\" of module \"%s\" with size %d (%dx%d).\n", c->target_io->name, c->target_io->module->instance_name, c->target_io->size, c->target_io->sizex, c->target_io->sizey);
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
        for (Module * m = modules; m != NULL; m = m->next)
            m->SetSizes();
        if (sizeChangeFlag)
            Notify(msg_verbose, "InitOutput: Iteration with changes\n");
        else
            Notify(msg_verbose, "InitOutput: Iteration with no changes\n");
    }
    while (sizeChangeFlag);
}

void
Kernel::AllocateOutputs()
{
    for (Module * m = modules; m != NULL; m = m->next)
        m->AllocateOutputs();
}

void
Kernel::InitModules()
{
    for (Module * m = modules; m != NULL; m = m->next)
        m->Init();
}

void
Kernel::NotifySizeChange()
{
    sizeChangeFlag = true;
}

void
Kernel::Init()
{
    if (options->GetFilePath())
        ReadXML();
    else
        Notify(msg_fatal_error, "No IKC file supplied.\n"); // Maybe this should only be a warning
    
    DetectCycles();
    if(fatal_error_occured)
        return;
    SortModules();
    CalculateDelays();
    InitOutputs();      // Calculate the output sizes for outputs that have not been specified at creation
    AllocateOutputs();
    InitInputs();		// Calculate the input sizes and allocate memory for the inputs or connect 0-delays
    CheckOutputs();
    CheckInputs();
    if (fatal_error_occured)
        return;
    InitModules();
}

void
Kernel::Tick()
{
    Notify(msg_verbose, "Kernel::Tick()\n");
    Propagate();
    DelayOutputs();
    if (useThreads)
    {
        for (ThreadGroup * g = threadGroups; g != NULL; g = g->next)
            g->Start(tick);
        for (ThreadGroup * g = threadGroups; g != NULL; g = g->next)
            g->Stop(tick);
    }
    else if (print_mode < msg_verbose)
    {
        for (Module * m = modules; m != NULL; m = m->next)
            if (tick % m->period == m->phase)
            {
                m->timer->Restart();
                m->Tick();
                m->time += m->timer->GetTime();
                m->ticks += 1;
            }
    }
    else
    {
        for (Module * m = modules; m != NULL; m = m->next)
            if (tick % m->period == m->phase)
            {
                m->timer->Restart();
                Notify(msg_verbose, "%s::Tick (%s) Start\n", m->GetName(), m->GetClassName());
                m->Tick();
                Notify(msg_verbose, "%s::Tick (%s) End\n", m->GetName(), m->GetClassName());
                m->time += m->timer->GetTime();
                m->ticks += 1;
            }
        
    }
    
    if(nan_checks)
        CheckNAN();
    
    tick++;
}


void
Kernel::DelayOutputs()
{
    for (Module * m = modules; m != NULL; m = m->next)
        m->DelayOutputs();
}



void
Kernel::AddModule(Module * m)
{
    if (!m) return;
    m->next = modules;
    modules = m;
    m->kernel = this;
}



Module *
Kernel::GetModule(const char * n)
{
    for (Module * m = modules; m != NULL; m = m->next)
        if (equal_strings(n, m->instance_name))
            return m;
    return NULL;
}



Module *
Kernel::GetModuleFromFullName(const char * n)
{
    for (Module * m = modules; m != NULL; m = m->next)
        if (equal_strings(n, m->full_instance_name))
            return m;
    return NULL;
}



bool
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
                    printf("STOP\n");
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



bool
Kernel::Precedes(Module * a, Module * b)
{
    // Base case
    
    for (Connection * c = connections; c != NULL; c = c->next)
        if (c->delay == 0 && c->source_io->module == a && c->target_io->module == b)
            return true;
    
    // Transitivity test (also checks for direct loop)
    
    for (Connection * c = connections; c != NULL; c = c->next)
        if (c->delay == 0 && c->source_io->module == a && Precedes(c->target_io->module,  b))
            return true;
    
    return false;
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



void
Kernel::DetectCycles()
{
    for (Module * m = modules; m != NULL; m = m->next)
        if(Precedes(m, m))
            Notify(msg_fatal_error, "Module \"%s\" (%s) has a zero-delay connection to itself (directly or indirectly).\n", m->GetName(), m->GetClassName());
}



void
Kernel::SortModules()
{
    // For statistics only
    for (Module * m = modules; m != NULL; m = m->next)
    {
        phase_count = (m->phase < phase_count ? phase_count : m->phase);
        period_count = (m->period < period_count ? period_count : m->period);
        module_count++;
    }
    if (phase_count > period_count)
        period_count = phase_count;
    // Build a new sorted list of modules (precedence order) using selection sort
    Module * sorted_modules = NULL;
    while (modules != NULL)
    {
        // Find smallest module
        Module * sm = modules;
        for (Module * m = modules; m != NULL; m = m->next)
            if (Precedes(sm, m))
                sm = m;

        // Remove from list
        if (sm == modules)  // First
        {
            modules = modules->next;
            sm->next = sorted_modules;
            sorted_modules = sm;
        }
        else
        {
            // Find prev (double linked list would have been better)
            Module * psm = NULL;
            for (psm = modules; psm->next != sm; psm = psm->next)
                ;
            psm->next = sm->next;
            sm->next = sorted_modules;
            sorted_modules = sm;
        }
    }
    modules = sorted_modules;

    // Check for loops
    for (Module * m = modules; m != NULL; m = m->next)
        if (Precedes(m, m))
            Notify(msg_fatal_error, "Module \"%s\" (%s) has a zero-delay connection to itself.\n", m->GetName(), m->GetClassName());

    // Create Thread Groups
    threadGroups = new ThreadGroup(this);
    for (Module * m = modules; m != NULL; m = m->next)
        threadGroups->AddModule(m);
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

bool
Kernel::InputConnected(Module * m, const char * input_name) // TODO: Test it ***
{
    return m->GetInputArray(input_name, false) != NULL;
}

bool
Kernel::OutputConnected(Module * m, const char * output_name)
{
    if (connections == NULL)
        return false;
    for (Connection * c = connections; c != NULL; c = c->next)
        if (c->source_io->module == m && m->GetModule_IO(m->output_list, output_name) == c->source_io)
            return true;
    return false;
}

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
#ifdef USE_WIN_SOCKET
    Notify(msg_print, "WIN-socket\n");
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
    
    if(!modules)
    {
        Notify(msg_print, "\n");
        Notify(msg_print, "No Modules.\n");
        Notify(msg_print, "\n");
        return;
    }
    
    Notify(msg_print, "\n");
    Notify(msg_print, "Modules:\n");
    Notify(msg_print, "\n");
    for (Module * m = modules; m != NULL; m = m->next)
    {
        //		Notify(msg_print, "  %s (%s) [%d, %d]:\n", m->name, m->class_name, m->period, m->phase);
        Notify(msg_print, "  %s (%s) [%d, %d]:\n", m->GetFullName(), m->class_name, m->period, m->phase);
        for (Module_IO * i = m->input_list; i != NULL; i = i->next)
            if(i->data)
                Notify(msg_print, "    %-10s\t(Input) \t%6d%6d%12p\n", i->name, i->sizex, i->sizey, (i->data == NULL ? NULL : i->data[0]));
            else
                Notify(msg_print, "    %-10s\t(Input) \t           no connection\n", i->name);
        for (Module_IO * i = m->output_list; i != NULL; i = i->next)
            Notify(msg_print, "    %-10s\t(Output)\t%6d%6d%12p\t(%d)\n", i->name, i->sizex, i->sizey, (i->data == NULL ? NULL : i->data[0]), i->max_delay);
        Notify(msg_print, "\n");
    }
    Notify(msg_print, "Connections:\n");
    Notify(msg_print, "\n");
    for (Connection * c = connections; c != NULL; c = c->next)
        if (c->delay == 0)
            Notify(msg_print, "  %s.%s[%d..%d] == %s.%s[%d..%d] (%d)\n",
                   c->source_io->module->instance_name, c->source_io->name, 0, c->source_io->size-1,
                   c->target_io->module->instance_name, c->target_io->name, 0, c->source_io->size-1,
                   c->delay);
        else if (c->size > 1)
            Notify(msg_print, "  %s.%s[%d..%d] -> %s.%s[%d..%d] (%d)\n",
                   c->source_io->module->instance_name, c->source_io->name, c->source_offset, c->source_offset+c->size-1,
                   c->target_io->module->instance_name, c->target_io->name, c->target_offset, c->target_offset+c->size-1,
                   c->delay);
        else
            Notify(msg_print, "  %s.%s[%d] -> %s.%s[%d] (%d)\n",
                   c->source_io->module->instance_name, c->source_io->name, c->source_offset,
                   c->target_io->module->instance_name, c->target_io->name, c->target_offset,
                   c->delay);
    Notify(msg_print, "\n");
}

void
Kernel::ListThreads()
{
    if (!options->GetOption('T') && !(options->GetOption('a') && options->GetOption('t'))) return;
    Notify(msg_print, "\n");
    Notify(msg_print,"ThreadManagers:\n");
    Notify(msg_print, "\n");
    int tt = 0;
    for (ThreadGroup * t = threadGroups; t != NULL; t = t->next)
    {
        Notify(msg_print,"ThreadManager %d [Period:%d, Phase:%d]\n", tt++, t->period, t->phase);
        for (Module * m = t->modules; m != NULL; m=m->next_in_threadGroup)
            Notify(msg_print,"\tModule: %s\n", m->GetName());
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

    if(!modules)
        return;
    
    Notify(msg_print, "Scheduling:\n");
    Notify(msg_print, "\n");
    for (int t=0; t<period_count; t++)
    {
        int tm = 0;
        for (Module * m = modules; m != NULL; m = m->next)
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
    for (Module * m = modules; m != NULL; m = m->next)
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
    for (Module * m = modules; m != NULL; m = m->next)
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
    switch (msg)
    {
        case msg_fatal_error:
            fatal_error_occured	= true;
            break;
        case msg_end_of_file:
            end_of_file_reached	= true;
            break;
        case msg_terminate:
            terminate			= true;
            break;
        default:
            break;
    }
    if (msg > print_mode)
        return;
    char 	message[512];
    int n = 0;
    switch (msg)
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
    vsnprintf(&message[n], 512-n, format, args); // Fix #22 (public)
    va_end(args);
    printf("IKAROS: %s", message);
    if(message[strlen(message)-1] != '\n')
        printf("\n");
    if (logfile != NULL)
        fprintf(logfile, "%5ld: %s", tick, message);	// Print in both places
}



// Create one or several connections with different delays between two ModuleIOs

int
Kernel::Connect(Module_IO * sio, Module_IO * tio, const char * delay, int extra_delay)
{
    int c = 0;
    char * dstring = create_string(delay);
    
    if(!dstring || (!strchr(dstring, ':') && !strchr(dstring, ',')))
    {
        int d = string_to_int(dstring, 1);
        connections = new Connection(connections, sio, 0, tio, unknown_size, unknown_size, d+extra_delay);
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
                connections = new Connection(connections, sio, 0, tio, unknown_size, unknown_size, i+extra_delay);
                c++;
            }
            p = strtok(NULL, ",");
        }
		destroy_string(d);
    }
    
    destroy_string(dstring);
    return c;
}



int
Kernel::Connect(XMLElement * group_xml, Module * sm, Module_IO * sio, const char * tm_name, const char * t_name, const char * delay, int extra_delay)
{
    int c = 0; // no of generated connections
    
    // iterate over all modules
    
    for (XMLElement * xml_module = group_xml->GetContentElement("module"); xml_module != NULL; xml_module = xml_module->GetNextElement("module"))
        if(tm_name==NULL || equal_strings(tm_name, "*") || equal_strings(tm_name, GetXMLAttribute(xml_module, "name"))) // also matches anonymous module as it should; change first test to *?
        {
            Module * tm = (Module *)(xml_module->aux);
            if (tm)
            {
                Module_IO * tio = tm->GetModule_IO(tm->input_list, t_name);
                if(sio == NULL)
                    Notify(msg_fatal_error, "Could not make connection. Source missing\n.");
                else if(tio == NULL)
                    Notify(msg_fatal_error, "Could not make connection. Target \"%s\" of module \"%s\" missing.\n", t_name, tm_name);
                else
                    c += Connect(sio, tio, delay, extra_delay);
            }
        }
    
    bool connection_made = false;
    for (XMLElement * xml_group = group_xml->GetContentElement("group"); xml_group != NULL; xml_group = xml_group->GetNextElement("group"))
        if(equal_strings(tm_name, "*") || equal_strings(tm_name, GetXMLAttribute(xml_group, "name"))) // Found group
        {
            // iterate over all input elements
            for (XMLElement * xml_input = xml_group->GetContentElement("input"); xml_input != NULL; xml_input = xml_input->GetNextElement("input"))
                if(equal_strings(t_name, GetXMLAttribute(xml_input, "name"))) // Found input with target
                {
                    connection_made = true;
                    const char * tm = GetXMLAttribute(xml_input, "targetmodule");
                    const char * t = GetXMLAttribute(xml_input, "target");
                    int d = string_to_int(GetXMLAttribute(xml_input, "delay")); // TODO: We should merge d with the range in d instead
                    if(!equal_strings(t,""))
                        c += Connect(xml_group, sm, sio, tm, (t ? t : t_name), delay, d+extra_delay);   // TODO: Extra delay should be replaced with a merge interval function
                    else
                        c++; // ignore connections if both are nil
                }
        }
    
    if(!connection_made) // look of wildcard connections
        for (XMLElement * xml_group = group_xml->GetContentElement("group"); xml_group != NULL; xml_group = xml_group->GetNextElement("group"))
            if(equal_strings(tm_name, "*") || equal_strings(tm_name, GetXMLAttribute(xml_group, "name"))) // Found group
            {
                // iterate over all input elements
                for (XMLElement * xml_input = xml_group->GetContentElement("input"); xml_input != NULL; xml_input = xml_input->GetNextElement("input"))
                    if(equal_strings("*", GetXMLAttribute(xml_input, "name"))) // Found input with target
                    {
                        const char * tm = GetXMLAttribute(xml_input, "targetmodule");
                        const char * t = GetXMLAttribute(xml_input, "target");
                        int d = string_to_int(GetXMLAttribute(xml_input, "delay"));
 
                                                if(!equal_strings(t,""))
                            c += Connect(xml_group, sm, sio, tm, (t ? t : t_name), delay, d+extra_delay);   // TODO: Extra delay should be replaced with a merge interval function
                        else
                            c++; // ignore connections if both are nil
                    }
            }
    
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
Kernel::BuildClassGroup(XMLElement * xml_node, const char * class_name)
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
	
	BuildGroup(cgroup, class_name);
	
	// 5. Delete original element and replace it with the newly merged group
	
	xml_node->next = NULL;
	delete xml_node;
	
	return cgroup;
}



// Parse XML for a group

static int group_number = 0;

void
Kernel::BuildGroup(XMLElement * group_xml, const char * current_class)
{
    const char * name = GetXMLAttribute(group_xml, "name");
    if(name == NULL)
        group_xml->SetAttribute("name", create_formatted_string("Group-%d", group_number++));

    for (XMLElement * xml_node = group_xml->GetContentElement(); xml_node != NULL; xml_node = xml_node->GetNextElement())
        if (xml_node->IsElement("module"))	// Add module
        {
            char * class_name = create_string(GetXMLAttribute(xml_node, "class"));
            if (!equal_strings(class_name, current_class))  // Check that we are not in a class file
            {
				xml_node = BuildClassGroup(xml_node, class_name);
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
				AddModule(m);
			}
            else  // TODO: could add check for command line values here
            {
                Notify(msg_warning, "Could not create module: Class name is missing.\n");
            }
            destroy_string(class_name);
        }
        else if (xml_node->IsElement("group"))	// Add group
            BuildGroup(xml_node);
    
    // Create connections in group
    
    for (XMLElement * xml_connection = group_xml->GetContentElement("connection"); xml_connection != NULL; xml_connection = xml_connection->GetNextElement("connection"))
    {
        const char * sm_name    = GetXMLAttribute(xml_connection, "sourcemodule");
        const char * s_name     = GetXMLAttribute(xml_connection, "source");
        const char * tm_name    = GetXMLAttribute(xml_connection, "targetmodule");
        const char * t_name     = GetXMLAttribute(xml_connection, "target");
        const char * delay      = GetXMLAttribute(xml_connection, "delay");
        
        Module * sm;
        Module_IO * sio;
        int c = 0;
        if (GetSource(group_xml, sm, sio, sm_name, s_name))
            c = Connect(group_xml, sm, sio, tm_name, t_name, delay);
        else
            Notify(msg_fatal_error, "Connection source %s.%s not found.\n", sm_name, s_name);
        
        if(c == 0)
            Notify(msg_fatal_error, "Connection target %s.%s not found.\n", tm_name, t_name);
    }
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
    xmlDoc = new XMLDocument(ikc_file_name, options->GetOption('X'));
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
    // Build The Main Group
    
    if(!xml->GetAttribute("name")) // This test is necessary since we are not alllowed to change a value of an attribute
    {
        const char * name = GetXMLAttribute(xml, "name"); // Instantiate name and title from command line options if not set in the file
        if(name)
            xml->SetAttribute("name", name);
    }
    
    if(!xml->GetAttribute("title"))
    {
        const char * title = GetXMLAttribute(xml, "title");
        if(title)
            xml->SetAttribute("title", title);
    }
    
    BuildGroup(xml);
    if (options->GetOption('x'))
        xmlDoc->Print(stdout);
}



Kernel& kernel()
{
    static Kernel * kernelInstance = new Kernel();
    return * kernelInstance;
}


