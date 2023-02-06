//
//  IKAROS.h        Kernel code for the IKAROS project
//
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
//    Created July 13, 2001
//

#ifndef IKAROS
#define IKAROS

#include <string>
#include <map>
#include <set>
#include <deque>
#include <thread>
#include <unordered_map>
#include <atomic> 

#define VERSION "2.0"

#include "IKAROS_System.h"
#include "Kernel/IKAROS_Timer.h"
#include "Kernel/IKAROS_Socket.h"
#include "Kernel/IKAROS_Utils.h"
#include "Kernel/IKAROS_Math.h"
#include "Kernel/IKAROS_Matrix.h"
#include "Kernel/IKAROS_Image_Processing.h"
#include "Kernel/IKAROS_Serial.h"
#include "Kernel/IKAROS_XML.h"

// Messages to use with Notify

const int    msg_exception		=    1;
const int    msg_end_of_file	=    2;
const int    msg_terminate		=    3;
const int    msg_fatal_error	=    4;
const int    msg_warning		=    5;
const int    msg_print			=    6;
const int    msg_debug          =    7;
const int    msg_trace          =    8;

// Log level for Notify

const int    log_level_off		=    0;
const int    log_level_fatal	=    3;
const int    log_level_error	=    4;
const int    log_level_warning	=    5;
const int    log_level_info		=    6;
const int    log_level_debug	=    7;
const int    log_level_trace	=    8;

// Binding constants - type constants

const int   bind_float    = 0;
const int   bind_int      = 1;
const int   bind_bool     = 2;
const int   bind_list     = 3;
const int   bind_array    = 4;
const int   bind_matrix   = 5;
const int   bind_string   = 6;

// WebUI constants

#ifndef     WEBUIPATH
#define     WEBUIPATH    "Source/WebUI/"
#endif

#define        PORT 8000

const int ui_state_stop = 0;
const int ui_state_pause = 1;
const int ui_state_step = 2;
const int ui_state_play = 3;
const int ui_state_realtime = 4;

const int data_source_float = bind_float;
const int data_source_int = bind_int;
const int data_source_bool = bind_bool;
const int data_source_list = bind_list;
const int data_source_array = bind_array;
const int data_source_matrix = bind_matrix;
const int data_source_string = bind_string;

const int data_source_gray_image = data_source_string + 1;
const int data_source_red_image = data_source_string + 2;
const int data_source_green_image = data_source_string + 3;
const int data_source_blue_image = data_source_string + 4;
const int data_source_fire_image = data_source_string + 5;
const int data_source_spectrum_image = data_source_string + 6; // not used
const int data_source_rgb_image = data_source_string + 7;
const int data_source_bmp_image = data_source_string + 8;


// Size constant

const int    unknown_size   =    -1;

// Forward declarations

class Module;
class Kernel;
class GroupElement;



//
// Parameter is used during the creation of a module to set internal values
//

class Parameter
{
public:
    Kernel * kernel;
    XMLElement * xml;   // TODO: remove this and all dependencies
    GroupElement * group;
    
    Parameter(Kernel * k, XMLElement * x, GroupElement * g)
    {
        kernel = k;
        xml = x;
        group = g;
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
    
    Binding(Module * m, const char * n, int t, void * v, int sx, int sy) :
        module(m), name(n), value(v), size_x(sx), size_y(sy), type(t)
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
public:
    ModuleCreator   module_creator;
    const char *    name;
    const char *    path;
    
    ModuleClass(const char * n, ModuleCreator mc, const char * p);
    ~ModuleClass();
    
    void            SetClassPath(const char * class_name);
    const char *	GetClassPath();
    Module *    CreateModule(Parameter * p);
};



//
// Module_IO is used internally by Module and Kernel to represent the input and output connections of a module
//
// data: matrix[***], optional, allow_multiple, module, name, max_delay
// methods: delay_output - could be a matrix function => no methods

class Module_IO
{
public:
    int                 sizex;        // no of columns    *** made public for WebUI ***
    int                 sizey;        // no of rows
    float        ***    matrix;       // matrix version of data; array of pointers to columns; Array of matrixes for delays
    bool                optional;
    bool                allow_multiple;
//private:
    Module_IO(Module_IO * nxt, Module * m, const char * n, int x, int y, bool opt=false, bool multiple=true);      // Create output from module m with name n and size x and y (default x=unkwon_size, y=1)
    ~Module_IO();                                                                                                   // Deletes the data
    
    bool                Allocate();
    void                SetSize(int x, int y=1);
    void                DelayOutputs();
    
    Module_IO   *   next;
    Module      *   module;
    std::string     name;
    float       **  data;        // Array for delays
    int             size;        // should equal sizex*sizey
    int             max_delay;   // maximum number of arrays/matrices
    
    friend class Kernel;
    friend class Connection;
    friend class Module;

};


//
// Module is the base class for all modules
//
// Bind(io_matrix_or_variable, "name") should replace all GetValue and GetInput/Output and get_size

class GroupElement;

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

    void            io(float * & a, const char * name);   // When size is known
    void            io(float * & a, int & size, const char * name);   // Replaces all array functions above
    void            io(float ** & m, int & size_x, int & size_y, const char * name);   // Replaces all matrix functions above
    void            io(float * & a, int *& size, const char * name);   // Replaces all array functions above for variable array size
    void            io(float ** & m, int *& size_x, int *& size_y, const char * name);   // Replaces all matrix functions above for variable matrix size
   // void            io(ikaros::matrix & m, std::string name, bool required=true); // Get input or output matrix

    void            StoreArray(const char * path, const char * name, float * a, int size);
    void            StoreMatrix(const char * path, const char * name, float ** m, int size_x, int size_y);

    bool            LoadArray(const char * path, const char * name, float * a, int size);
    bool            LoadMatrix(const char * path, const char * name, float ** m, int size_x, int size_y);

    virtual void    Store(const char * path);                            // Request data to be stored in directory path
    virtual void    Load(const char * path);                            // Request data to be loaded from directory path

    virtual std::string GetJSONData(const std::string & name, const std::string & tab="");                 // Get JSON string for input, output or parameter, return empty string if data does not exists or not implemented
    
    virtual void    SetSizes();								// Calculate and set the sizes of unknown output arrays    (OVERRIDE IN SUBCLASSES)
    virtual void    Init()
    {}            // Init memory for internal data                        (OVERRIDE IN SUBCLASSES)
    virtual void    Tick()
    {}            // Update state of the module                            (OVERRIDE IN SUBCLASSES)
    
    bool            Notify(int msg);                    // Send message to the kernel (for example terminate or error, using constants defined above)
    bool            Notify(int msg, const char *format, ...);    // Send message to the kernel and print a massage to the user

    // zero connections in both directions

    //std::set<std::string> outgoing_connection;  // only one entry even if there are multiple connections
    std::vector<Module *> connects_to_with_zero_delay;
    std::vector<Module *> connects_from_with_zero_delay;

    int             mark;

//protected:
    void            AddInput(const char * name, bool optional=false, bool allow_multiple_connections=true);
    void            AddOutput(const char * name, bool optional=false, int size_x=unknown_size, int size_y=1);    // Allocate output
    void            AddIOFromIKC();
    void            SetOutputSize(const char * name, int size_x, int size_y=1);    // Set the output size for an output of unknown size; it is an error to change the output size
    
    int             GetSizeXFromList(const char * sizearg);
    int             GetSizeYFromList(const char * sizearg);
    
    Module(Parameter * p);        // Creates the module and possibly init connections from ikc file (MUST BE CALLED FROM SUBCLASS CREATOR)
    
    virtual            ~Module();
    
    // Get values from parameters in XML tree - implements the semantics of the ikc file format
    
    const char *    GetList(const char * n);                          // Get list of values for parameter
 // std::string     GetList(const std::string & name);                          // Get list of values for parameter
 
	const char *    GetValue(const char * n);                            // Search through XML for parameter and return its value or NULL
    float           GetFloatValue(const char * n, float d=0, bool deprecation_warning=false);            // Search through XML for parameter and return its value as a float or default value d if not found
    int             GetIntValue(const char * n, int d=0);                // Search through XML for parameter and return its value as a float or default value d if not found
    bool            GetBoolValue(const char * n, bool d=false);        // Search through XML for parameter and return its value as a float or default value d if not found
    int             GetIntValueFromList(const char * n, const char * list=NULL);    // Search through XML for parameter and then search list for the index of the value in the parameter; return 0 if not found
    float *         GetArray(const char * n, int & size, bool fixed_size=false);		// Search through XML for parameter and return its value as an array; fixed_size=true uses supplied size rathrer than data size
    float **        GetMatrix(const char * n, int & sizex, int & sizey, bool fixed_size=false);	// Search through XML for parameter and return its value as a matrix; fixed_size=true uses supplied size rathrer than data size
    int *           GetIntArray(const char * n, int & size, bool fixed_size=false);  // Search through XML for parameter and return its value as an array; fixed_size=true uses supplied size rathrer than data size

    // New way to get inputs and outputs
/*
    void            Bind(float *, const char * n, int & size) {}                        // output
    void            Bind(float **, const char * n, int & sizex, int & sizey) {}         // output
    void            Bind(const float *, const char * n, int & size) {}                  // input
    void            Bind(const float **, const char * n, int & sizex, int & sizey) {}   // input
  */  
//     void           Bind(matrix & m, std:string name) {};
    
    // Bind values to names and get values from XML tree if possible
    // FIXME: bind with read-only flag to replace all the getter functions
    
    void            Bind(float & v, const char * n);                        // Bind a floating point value to a name
    void            Bind(int & v, const char * n);                          // Bind int OR list value to name
    void            Bind(bool & v, const char * n);                         // Bind boolean

    void            Bind(float * & v, int size, const char * n, bool fixed_size = false);              //  // Creates and binds array; also gets the array size; uses the supplied size instead if fixed_size = true.
    void            Bind(float * & v, int * size, const char * n); // inconsistent naming - fix me

    void            Bind(float ** & v, int & sizex, int & sizey, const char * n, bool fixed_size = false); // Creates and binds matrix; also gets the matrix size; uses the supplied size instead if fixed_size = true.
    void            Bind(std::string & v, const char * n);                  // Bind string

    bool            SetParameter(const char * parameter_name, int x, int y, float value);
    virtual void    Command(std::string s, float x=0, float y=0, std::string value="") {};     // Receive a command, with optional x,y position and value

    XMLElement *        xml;
    GroupElement *      group;

    void            PrintAttributes();

//private:
    const char *		class_name;
    const char *		instance_name;
	char *				full_instance_name;

    Kernel *            kernel;

    Module_IO    *      input_list;        // List of inputs
    Module_IO    *      output_list;       // List of outputs
    Binding      *      bindings;          // FIXME: remove

    // Exposed as parameters

    int                 period;           // How often should the module tick
    int                 phase;            // Phase when the module ticks
    bool                active;           // If false, will not call Tick()
    int                 log_level;
    bool                high_priority;

    Timer *             timer;            // Statistics variables
    float               time;
    float *             power;      // Current power usage (in % or W if coefficient set)
    float               power_coefficient;

    float               ticks;

    void                DelayOutputs();

    void                SetInputSize(const char * name, int size); // Experiment
    
    Module_IO *         GetModule_IO(Module_IO * list, const char * name);
    void                AllocateOutputs(); // Allocate memory for outputs

    friend class Kernel;
    friend class Module_IO;
    friend class Connection;
    friend class ThreadGroup;

    friend Module *        CreateModule(ModuleClass * c, const char * class_name, const char * n, Parameter * p);
};


//
// Connection is used internally by Kernel to store pointers to data in Module_IO that are copied during Propagate()
//

class Connection
{                            // Connection contains info about data flow between modules
public:
    Connection        *    next;                // Next connection in list
    Module_IO         *    source_io;
    int                    source_offset;
    Module_IO         *    target_io;
    int                    target_offset;
    int                    size;                // The size of the data to be copied
    int                    delay;
    bool                   active;              // When false, data should not be propagated
    
    Connection(Connection * n, Module_IO * sio, int so, Module_IO * tio, int to, int s, int d, bool a); // int d = 1, bool a = true
    ~Connection();
    
    void Propagate(long long tick);
    
    friend class Kernel;
};


//
// ThreadGroup contains a linked list of modules that are executed in the same thread
//

class ThreadGroup
{
public:
    std::vector<Module *> _modules;
    
    int             period;                     // How often should the thread be started
    int             phase;                      // Phase when the thread should start
    bool            high_priority = false;              // Priority of the thread group
    std::thread *   thread;
    
    ThreadGroup(Kernel * k);
    ThreadGroup(Kernel * k, int period, int phase);
    ~ThreadGroup();
    
    void        Start(long long tick);
    void        Stop(long long tick);
    void        Tick();
};



//
// Kernel is the main object and points to data structures for modules and connections
//

class Kernel
{
public:
    // --------------- WebUI Part ---------------
    
    char *          webui_dir;
    int             port;
    ServerSocket *  socket;
    XMLElement *    xml;
    bool            debug_mode;
    bool            isRunning;
    int             ui_state;
    int             iterations_per_runstep;
    float           idle_time;
    float           time_usage;
    bool            first_request;
    long            master_id;

    std::atomic<bool> tick_is_running;
    std::atomic<bool> sending_ui_data;
    std::atomic<bool> handling_request;
    
    void            SendXML();
    void            ReadXML(XMLDocument * xmlDoc);
    void            SendUIData(std::string args);
    void            Pause();


    void DoStop(std::string uri, std::string args);
    void DoPause(std::string uri, std::string args);
    void DoStep(std::string uri, std::string args);
    void DoPlay(std::string uri, std::string args);
    void DoRealtime(std::string uri, std::string args);
    void DoCommand(std::string uri, std::string args);
    void DoControl(std::string uri, std::string args);
    void DoSendNetwork(std::string uri, std::string args);
    void DoSendData(std::string uri, std::string args);
    void DoUpdate(std::string uri, std::string args);
    void DoGetLog(std::string uri, std::string args);
    void DoSendClasses(std::string uri, std::string args);
    void DoSendFile(std::string file);
    void DoSendError();

    void HandleHTTPRequest();
    void HandleHTTPThread();
    
    std::thread *   httpThread;
    static void *   StartHTTPThread(Kernel * k);

    // --------------- Kernel Part ---------------
    
    FILE *          logfile;
    const char *    ikaros_dir;
    char *          ikc_dir;
    char *          ikc_file_name;
    
    Options *       options;
    
    Kernel();
    ~Kernel();
    
    void        SetOptions(Options * opt);
    bool        AddClass(const char * name, ModuleCreator mcc, const char * path = NULL);    // Add a new class of modules to the kernel
    
    bool        Notify(int msg, const char * format, ...);  // Always returns false
    
    bool        Terminate();                                // True if the simulation should end
    void        Run();                                      // Run the simulation until kernel receives notification; for example end-of-file from a module
    
    void        Init();                                     // Call Init() for all modules
    void        Tick();                                     // Call Tick() for all modules
    
    void        Store();                                    // Call Store() for all modules
    void        Load();                                     // Call Load() for all modules
    
    void        AddModule(Module * m);                      // Add a module to the simulation
    Module *    GetModule(const char * n);                  // Find a module based on its name
    Module *    GetModuleFromFullName(const char * n);

    const char * GetXMLAttribute(XMLElement * e, const char * attribute);   // This function implements inheritance and checks batch and command line values

    bool        GetSource(Module_IO * &io, GroupElement * group, const char * source_module_name, const char * source_name);
    bool        GetBinding(Module * &m, int &type, void * &value_ptr, int & sx, int & sy, const char * source_module_name, const char * source_name);
    bool        SetParameter(const char * name, int x, int y, float value); // returns false on failure
    void        SendCommand(const char * command, float x, float y, std::string value);
    
    int         Connect(XMLElement * group_xml, Module * sm, Module_IO * sio, int s_offset, const char * tm_name, const char * t_name, int t_offset, int size=unknown_size, const char * delay = NULL, int extra_delay = 0, bool is_active = true);
    int         Connect(Module_IO * sio, int s_offset, Module_IO * tio, int t_offset, int size, const std::string & delay, int extra_delay, bool is_active);
    
    XMLElement *	BuildClassGroup(GroupElement * group, XMLElement * xml, const char * current_class = NULL, const char * current_filename = NULL);
    GroupElement *  BuildGroup(GroupElement * group, XMLElement * xml, const char * current_class = NULL, const char * current_filename = NULL);
    void            ConnectModules(GroupElement * group, std::string indent="");
    bool			ReadXML();

    std::string JSONString();

    long long        GetTick()
    {
        return tick;
    }

    long        GetTickLength()
    {
        return tick_length;
    }

    const char *	GetClassPath(const char * class_name)
    {
        return classes.at(class_name)->path;
    }

    void        ListInfo();
    void        CalculateChecksum();
    void        ListModulesAndConnections();
    void        ListBindings();
    void        ListScheduling();
    void        ListThreads();
    void        ListClasses();
    void        ListWarningsAndErrors();
    void        ListProfiling();
    void        PrintTiming();           // Print total timing information
    
    long long           tick;             // Updated every iteration
    long long           max_ticks;        // Max iterations, stop after these many ticks
    
    long                tick_length;      // Desired length (in ms) of each tick
    Timer *             timer;            // Global timer
    float               total_time;       // Total execution time at termination

    float               actual_tick_length; // Actual lengt of a tick in real-time mode
    float               lag;                // Lag of a tick in real-time mode
    
    int                 cpu_cores;
    double              cpu_usage;
    double              last_cpu;
    float               last_cpu_time;
    
    void                CalculateCPUUsage();
    
    std::unordered_map<std::string, ModuleClass *> classes;
    
    int                  log_level;        // Global log level
    
    XMLDocument     *    xmlDoc;
    
    GroupElement    *    main_group;     // 2.0 main group
    long                 session_id;     // 2.0 temporary
    
    std::map<std::string, std::vector<Binding *>>   bindings;   // 2.0
    std::map<std::string, std::string>              attributes; // 2.0 - attribute value set in the xml-file with full path

    std::map<std::string, Module *>     module_map; // 2.0
    std::vector <Module *>              _modules;   // 2.0

    Connection      *    connections;    // List of connections
    bool                 useThreads;

    std::vector<ThreadGroup *>  _threadGroups;

    // Execution Control
    
    int         module_count;
    int         period_count;
    int         phase_count;
    
    bool        end_of_file_reached;                        // Flags set on notification from modules
    bool        fatal_error_occurred;
    bool        terminate;
    
    bool        sizeChangeFlag;
    
    void        CheckInputs();                                // Check that memory for all connected inputs have been allocated
    void        CheckOutputs();                               // Check that all outputs are correctly set
    
    void        MarkSubgraph(Module * m);
    bool        CreateThreadGroups(std::deque<Module *> & sorted_modules);

    bool        Visit(std::deque<Module *> & sorted_modules, Module * n); // Returns false if not a DAG
    void        SortModules();                               // Sort modules using topological sort
    
    void        CalculateDelays();                            // Calculate the maximum delay from each output
    
    void        InitInputs();                                 // Allocate memory for inputs in all modules
    void        InitOutputs();                                // Calculate and set output sizes that were not set at module creation; called by Init()
    void        AllocateOutputs();                            // Allocate memory for outputs
    void        InitModules();                                // Init all modules; called by Init()
    
    int         CalculateInputSize(Module_IO * i);            // Calculate the size of the input using connections to it
    int         CalculateInputSizeX(Module_IO * i);           // Calculate the size of the input using connections to it
    int         CalculateInputSizeY(Module_IO * i);           // Calculate the size of the input using connections to it
    
    void        NotifySizeChange();
    
    void        Propagate();                                    // Copy output data to input for all modules
    void        DelayOutputs();
    
    void        CheckNAN();                             // Test for NANs in module outputs
    
    friend class Module;
    friend class Module_IO;
    friend class ThreadGroup;
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


