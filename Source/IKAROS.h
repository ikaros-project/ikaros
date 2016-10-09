//
//  IKAROS.h        Kernel code for the IKAROS project
//
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
//    Created July 13, 2001
//

#ifndef IKAROS
#define IKAROS


#define VERSION "1.4"

#include "IKAROS_System.h"
#include "Kernel/IKAROS_Timer.h"
#include "Kernel/IKAROS_Socket.h"
#include "Kernel/IKAROS_Utils.h"
#include "Kernel/IKAROS_Math.h"
#include "Kernel/IKAROS_Serial.h"
#include "Kernel/IKAROS_Threads.h"
#include "Kernel/IKAROS_XML.h"

// Messages to use with Notify

const int    msg_fatal_error	=    1;
const int    msg_exception		=    2;
const int    msg_warning		=    3;
const int    msg_print			=    4;
const int    msg_end_of_file	=    5;
const int    msg_terminate		=    6;
const int    msg_verbose		=    7;

// Print modes for Notify

const int    print_silent		=    0;
const int    print_normal		=    msg_print;
const int    print_verbose		=    msg_verbose;

// Size constant

const int    unknown_size		=    -1;

// Binding constants - type constants

const int bind_float    = 0;
const int bind_int      = 1;
const int bind_bool     = 2;
const int bind_list     = 3;
const int bind_array    = 4;
const int bind_matrix   = 5;


class Module;
class Kernel;
class WebUI;



//
// Parameter is used during the creation of a module to set internal values
//


class Parameter
{
public:
    Kernel * kernel;
    XMLElement * xml;
    
    Parameter(Kernel * k, XMLElement * x)
    {
        kernel = k;
        xml = x;
    }
    
    ~Parameter()
    {
    }
};



//
// Binding is used to bind a name to a variable
//

/*
 Binding types:          sent as
 OUTPUT  float **    [[]]
 matrix  float **    [[]]
 float   float *     [[]]
 int     int *       [[]]
 bool    bool *      [[]]     0/1
 */

class Binding
{
public:
    Module *        module;
    const char *    name;
    void *          value;
    int             size_x; // size for scalar [float, int, bool] = (0, 0) for array = (n, 1), for matrix = (n, m)
    int             size_y;
    int             type;
    Binding *       next;
    
    Binding(Module * m, const char * n, int t, void * v, int sx, int sy, Binding * nxt) :
    module(m), name(n), value(v), size_x(sx), size_y(sy), type(t), next(nxt)
    { }
    
    ~Binding()
    { }
};



//
// Class is used to hold pointers to module creators
//

typedef Module * (*ModuleCreator)(Parameter *);



class ModuleClass
{
private:
    ModuleCreator    module_creator;
public:
    const char *      name;
    const char *      path;
    ModuleClass *     next;
    
    ModuleClass(const char * n, ModuleCreator mc, const char * p, ModuleClass * nxt = NULL);
    ~ModuleClass();
    
    void            SetClassPath(const char * class_name);
    const char *	GetClassPath(const char * class_name);
    friend Module *	CreateModule(ModuleClass * c, const char * class_name, const char * n, Parameter * p);	// Return a module of class class_name initialized with parameters in p and attributes in a
};



//
// Module_IO is used internally by Module and Kernel to represent the input and output connections of a module
//

class Module_IO
{
public:
    int                 sizex;        // no of columns    *** made public for WebUI ***
    int                 sizey;        // no of rows
    float        ***    matrix;       // matrix version of data; array of pointers to columns; Array of matrixes in 0.8.0 for delays
    bool                optional;
    bool                allow_multiple;
private:
    Module_IO(Module_IO * nxt, Module * m, const char * n, int x, int y, bool opt=false, bool multiple=true);      // Create output from module m with name n and size x and y (default x=unkwon_size, y=1)
    ~Module_IO();                                                                                                   // Deletes the data
    
    void                Allocate();
    void                SetSize(int x, int y=1);
    void                DelayOutputs();
    
    
    Module_IO   *   next;
    Module      *   module;
    const char  *   name;
    float       **  data;        // Array for delays
    int             size;        // should equal sizex*sizey
    int             max_delay;   // maximum number of arrays/matrices
    
    friend class Kernel;
    friend class Connection;
    friend class Module;
    friend class WebUI;
};



//
// Module is the base class for all simulation modules
//

class Module
{
public:
    const char *    GetName();                                // Get the name of the module assigned during creation
    const char *    GetFullName();                            // Get the name of the module with all enclosing groups
    const char *    GetClassName();
    const char *	GetClassPath();

    long            GetTickLength();                           // Get length of tick in ms for real time mode
    long            GetTick();
    
    float *    GetInputArray(const char * name, bool required=true);                  // Get a pointer to the input array of this module with a certain name
    float *    GetOutputArray(const char * name, bool required=true);                 // Get a pointer to the output array of this module with a certain name
    
    float **   GetInputMatrix(const char * name, bool required=true);                // Get a pointer to the input matrix of this module with a certain name
    float **   GetOutputMatrix(const char * name, bool required=true);               // Get a pointer to the output matrix of this module with a certain name
    
    int        GetInputSize(const char * input_name);               // Get the size of an input array (size_x * size_y if two-dimensional)
    int        GetInputSizeX(const char * name);                    // Get the horizontal size of an input array
    int        GetInputSizeY(const char * name);                    // Get the vertical size of an input array (1 if one-dimensional)
    
    int        GetOutputSize(const char * name);                    // Get the size of an output array (size_x * size_y if two-dimensional)
    int        GetOutputSizeX(const char * name);                   // Get the horizontal size of an output array
    int        GetOutputSizeY(const char * name);                   // Get the vertical size of an output array (1 if one-dimensional)
    
    bool        InputConnected(const char * name);                // True if input receives at least one connection
    bool        OutputConnected(const char * name);               // True if output is connected to at least one module
    
    virtual void    SetSizes();								// Calculate and set the sizes of unknown output arrays    (OVERRIDE IN SUBCLASSES)
    virtual void    Init()
    {}            // Init memory for internal data                        (OVERRIDE IN SUBCLASSES)
    virtual void    Tick()
    {}            // Update state of the module                            (OVERRIDE IN SUBCLASSES)
    
    void            Notify(int msg);                    // Send message to the kernel (for example terminate or error, using constants defined above)
    void            Notify(int msg, const char *format, ...);    // Send message to the kernel and print a massage to the user
    
protected:
    void            AddInput(const char * name, bool optional=false, bool allow_multiple_connections=true);
    void            AddOutput(const char * name, int size_x=unknown_size, int size_y=1, bool optional=false);    // Allocate output
    void            AddIOFromIKC();
    void            SetOutputSize(const char * name, int size_x, int size_y=1);    // Set the output size for an output of unknown size; it is an error to change the output size
    
    int             GetSizeXFromList(const char * sizearg);
    int             GetSizeYFromList(const char * sizearg);
    
    Module(Parameter * p);        // Creates the module and possibly init connections from ikc file (MUST BE CALLED FROM SUBCLASS CREATOR)
    
    virtual            ~Module();
    
    // Get values from parameters in XML tree - implements the semantics of the ikc file format
    
    const char *    GetList(const char * n);                          // Get list of values for parameter
    const char *    GetDefault(const char * n);                          // Get default value for parameter
	const char *    GetValue(const char * n);                            // Search through XML for parameter and return its value or NULL
    float           GetFloatValue(const char * n, float d=0);            // Search through XML for parameter and return its value as a float or default value d if not found
    int             GetIntValue(const char * n, int d=0);                // Search through XML for parameter and return its value as a float or default value d if not found
    bool            GetBoolValue(const char * n, bool d=false);        // Search through XML for parameter and return its value as a float or default value d if not found
    int             GetIntValueFromList(const char * n, const char * list=NULL);    // Search through XML for parameter and then search list for the index of the value in the parameter; return 0 if not foun
    float *         GetArray(const char * n, int size);		// Search through XML for parameter and return its value as an array
    float **        GetMatrix(const char * n, int & sizex, int & sizey);	// Search through XML for parameter and return its value as a matrix
    int *           GetIntArray(const char * n, int & size);  // If size == 0, get number of items from parameter otehrwise make sure that the requtested size is returned and filled with any available values; if there are too few values the last one will be used for the remaining elements

    // Bind values to names and get values from XML tree if possible
    
    void            Bind(float & v, const char * n);                        // Bind a floating point value to a name
    void            Bind(int & v, const char * n);                          // Bind int OR list value to name
    void            Bind(bool & v, const char * n);                         // Bind boolean
    void            Bind(float * & v, int size, const char * n);              // Bind array
    void            Bind(float ** & v, int & sizex, int & sizey, const char * n); // Bind matrix; also gets the matrix size


    void            SetParameter(const char * parameter_name, int x, int y, float value);

    XMLElement *    xml;
    
private:
    Module     *        next;                   // Next module in list
    Module     *        next_in_threadGroup;    // Next module in ThreadGroup
    
    const char    *		class_name;
    const char *		instance_name;
	char *				full_instance_name;
	
    Kernel    *        kernel;
    
    Module_IO    *    input_list;        // List of inputs
    Module_IO    *    output_list;       // List of outputs
    Binding      *      bindings;
    
    int                period;           // How often should the module tick
    int                phase;            // Phase when the module ticks
    
    Timer *            timer;            // Statistics variables
    float            time;
    float            ticks;
    
    void            DelayOutputs();
    
    Module_IO *        GetModule_IO(Module_IO * list, const char * name);
    void            AllocateOutputs();                                // Allocate memory for outputs
    
    friend class Kernel;
    friend class Module_IO;
    friend class Connection;
    friend class ThreadGroup;
    
    friend Module *        CreateModule(ModuleClass * c, const char * class_name, const char * n, Parameter * p);
    friend class WebUI;
};



//
// Connection is used internally by Kernel to store pointers to data in Module_IO that are copied during Propagate()
//

class Connection
{                            // Connection contains info about data flow between modules
private:
    Connection        *    next;                // Next connection in list
    Module_IO        *    source_io;
    int                    source_offset;
    Module_IO        *    target_io;
    int                    target_offset;
    int                    size;                // The size of the data to be copied
    int                    delay;
    
    Connection(Connection * n, Module_IO * sio, int so, Module_IO * tio, int to, int s, int d = 1);
    ~Connection();
    
    void Propagate(long tick);
    
    friend class Kernel;
    friend class WebUI;
};



//
// ThreadGroup contains a linked list of modules that are executed in the same thread
//

class ThreadGroup
{
public:
    Kernel *        kernel;
    ThreadGroup *    next;
    Module *        modules;
    Module *        last_module;        // Last module in list
    
    int            period;            // How often should the thread be started
    int            phase;            // Phase when the thread should start
    
    Thread *        thread;
    
    void        AddModule(Module * m);        // Try to add module to this or next thread; return false if it is necessary to create a new thread
    
    ThreadGroup(Kernel * k);
    ~ThreadGroup();
    
    void        Start(long tick);
    void        Stop(long tick);
    void        Tick();
};



//
// Kernel is the main object and points to data structures for modules and connections
//

class Kernel
{
public:
    FILE *          logfile;
    const char *    ikaros_dir;
    char *          ikc_dir;
    char *          ikc_file_name;
    
    Options *       options;
    
    Kernel();
    Kernel(Options * opt);
    ~Kernel();
    
    void        SetOptions(Options * opt);
    void        AddClass(const char * name, ModuleCreator mcc, const char * path = NULL);    // Add a new class of modules to the kernel
    
    void        Notify(int msg, const char * format, ...);
    
    bool        Terminate();                                // True if the simulation should end
    void        Run();                                      // Run the simulation until kernel receives notification; for example end-of-file from a module
    
    void        Init();                                     // Call Init() for all modules
    void        Tick();                                     // Call Tick() for all modules
    
    void        AddModule(Module * m);                      // Add a module to the simulation
    Module *    GetModule(const char * n);                  // Find a module based on its name
    Module *    GetModuleFromFullName(const char * n);
    
	const char *    GetBatchValue(const char * n);          // Get a value from a batch element with the target n and the current batch rank
    
    const char * GetXMLAttribute(XMLElement * e, const char * attribute);   // This function implements inheritance and checks batc and command line values
    bool        GetSource(XMLElement * group, Module * &m, Module_IO * &io, const char * source_module_name, const char * source_name);
    bool        GetBinding(Module * &m, int &type, void * &value_ptr, int & sx, int & sy, const char * source_module_name, const char * source_name);
    bool        GetBinding(XMLElement * group, Module * &m, int &type, void * &value_ptr, int & sx, int & sy, const char * source_module_name, const char * source_name);
    void        SetParameter(XMLElement * group, const char * group_name, const char * parameter_name, int select_x, int select_y, float value);

    int         Connect(Module_IO * sio, Module_IO * tio, const char * delay, int extra_delay = 0);
    int         Connect(XMLElement * group_xml, Module * sm, Module_IO * sio, const char * tm_name, const char * t_name, const char * delay, int extra_delay = 0);
    
    XMLElement *	BuildClassGroup(XMLElement * xml, const char * current_class = NULL);
    void			BuildGroup(XMLElement * xml, const char * current_class = NULL);
    void			ReadXML();
    
    bool		InputConnected(Module * m, const char * input_name);
    bool        OutputConnected(Module * m, const char * output_name);
    
    long        GetTick()
    {
        return tick;
    }
    
    long        GetTickLength()
    {
        return tick_length;
    }
    
    const char *	GetClassPath(const char * class_name)
    {
        return classes->GetClassPath(class_name);
    }

    
    void        ListInfo();
    void        ListModulesAndConnections();
    void        ListScheduling();
    void        ListThreads();
    void        ListClasses();
    void        ListWarningsAndErrors();
    void        ListProfiling();
    void        PrintTiming();           // Print total timing information
    
private:
    ModuleClass        *     classes;    // List of module classes
    long               tick;             // Updated every iteration
    long               max_ticks;        // Max iterations, stop after these many ticks
    
    long               tick_length;      // Desired length (in ms) of each tick
    Timer *            timer;            // Global timer
    float              total_time;       // Total execution time at termination
    
    int                  print_mode;     // How much to print
    
    XMLDocument     *    xmlDoc;
    
    Module          *    modules;        // List of modules
    Connection      *    connections;    // List of connections
    bool                 useThreads;
    
    ThreadGroup     *    threadGroups;   // List of ThreadGroups
    
    bool                nan_checks;      // Look for NANs in all outputs efter every tick - slow; use only for debugging
    
    // Execution Control
    
    int         module_count;
    int         period_count;
    int         phase_count;
    
    bool        end_of_file_reached;                        // Flags set on notification from modules
    bool        fatal_error_occured;
    bool        terminate;
    
    bool        sizeChangeFlag;
    
    void        CheckInputs();                                // Check that memory for all connected inputs have been allocated
    void        CheckOutputs();                               // Check that all outputs are correctly set
    
    bool        Precedes(Module * a, Module * b);
    void        DetectCycles();                                 // Find zero-delay loops in the connections
    void        SortModules();                                // Sort modules in precedence order
    void        CalculateDelays();                            // Calculate the maximum delay from each output
    
    void        InitInputs();                                 // Allocate memory for inputs in all modules
    void        InitOutputs();                                // Calculate and set output sizes that were not set at module creation; called by Init()
    void        AllocateOutputs();                            // Allocate memory for outputs
    void        InitModules();                                // Init all modules; called by Init()
    
    int         CalculateInputSize(Module_IO * i);             // Calculate the size of the input using connections to it
    int         CalculateInputSizeX(Module_IO * i);            // Calculate the size of the input using connections to it
    int         CalculateInputSizeY(Module_IO * i);            // Calculate the size of the input using connections to it
    
    void        NotifySizeChange();
    
    void        Propagate();                                    // Copy output data to input for all modules
    void        DelayOutputs();
    
    void        CheckNAN();                             // Test for NANs in module outputs
    
    friend class Module;
    friend class Module_IO;
    friend class ThreadGroup;
    friend class WebUI;
};


// Global reference to static kernel instance

Kernel & kernel();

// Initialization class

class InitClass
{
public:
    InitClass(const char * name, ModuleCreator mcc, const char * path = NULL)
    {
        kernel().AddClass(name, mcc, path);
    }
};


#endif


