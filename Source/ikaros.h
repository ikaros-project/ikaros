// Ikaros 3.0

#pragma once

#include <stdexcept>
#include <atomic>
#include <algorithm>
#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <limits>
#include <random>
#include <set>
#include <stack>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>

#define INSTALL_CLASS(class_name)  static InitClass init_##class_name(#class_name, []() { return new class_name(); });

#include "Kernel/exceptions.h"
#include "Kernel/utilities.h"
#include "Kernel/dictionary.h"
#include "Kernel/options.h"
#include "Kernel/range.h"
#include "Kernel/expression.h"
#include "Kernel/maths.h"
#include "Kernel/matrix.h"
#include "Kernel/h_matrix.h"
#include "Kernel/socket.h"
#include "Kernel/timing.h"
#include "Kernel/deprecated.h"
#include "Kernel/image_file_formats.h"
#include "Kernel/serial.h"
#include "Kernel/thread_pool.h"
#include "Kernel/statistics.h"
#include "Kernel/profiler.h"


namespace ikaros 
{
const int run_mode_quit = 0;
const int run_mode_stop = 1;        // Kernel does not run and accepts open/save/save_as/pause/realtime
const int run_mode_pause = 2;       // Kernel is paused and accepts stop/step/realtime
const int run_mode_play = 3;        // Kernel runs as fast as possible
const int run_mode_realtime = 4;    // Kernel runs in real-time mode
const int run_mode_restart = 5;     // Kernel is restarting

// Messages to use with Notify

const int    msg_inherit		=    0;     // Inherit message level from parent; same as if no level is set
const int    msg_quiet		    =    1;
const int    msg_exception		=    2;
const int    msg_end_of_file	=    3;
const int    msg_terminate		=    4;
const int    msg_fatal_error	=    5;
const int    msg_warning		=    6;
const int    msg_print			=    7;
const int    msg_debug          =    8;
const int    msg_trace          =    9;

using tick_count = long long int;

std::string  validate_identifier(std::string s);

class Component;
class Module;
class Connection;
class Kernel;
class ComputeEngine;

using input_map = const std::map<std::string,std::vector<Connection *>> &;

Kernel& kernel();

//
// CIRCULAR BUFFER
//

class CircularBuffer
{
public:
    std::vector<matrix> buffer_;
    int                 index_;

    CircularBuffer() {}
    CircularBuffer(matrix &  m,  int size);
    void rotate(matrix &  m);
    matrix & get(int i);
};

//
// PARAMETERS // TODO: add bracket notation to set any type p(x,y) etc
//

enum parameter_type 
{
     no_type=0, 
     number_type,
     rate_type,
     bool_type, 
     string_type, 
     matrix_type, 

};

inline constexpr std::array<std::string_view, 6> parameter_strings = {
    "none",
    "number",
    "rate",
    "bool",
    "string",
    "matrix",
};



class parameter
{
private:
public:
    using parameter_value = std::variant<std::monostate, double, bool, int, std::string, matrix>;

    dictionary                      info_;
    bool                            has_options;
    std::shared_ptr<bool>           resolved;
    parameter_type                  type;
    std::shared_ptr<parameter_value> value;

    parameter();
    parameter(dictionary info);
    parameter(const std::string type, const std::string options="");

    void assign(const parameter & p); // this aliases data from p
    double operator=(double v);
    std::string operator=(std::string v);
    void set_matrix(const matrix & v);

    operator matrix & ();
    operator std::string() const;
    operator double() const;
    explicit operator bool() const;

    void print(std::string name="") const;
    void info() const;

    bool as_bool() const;
    float as_float() const;
    double as_double() const;
    long as_long() const;
    int as_int() const;
    std::string as_int_string() const; // as_int() converted to string
    std::string as_string() const;
    bool empty() const;

    const char* c_str() const noexcept;

    std::string json() const;    
    
    friend std::ostream& operator<<(std::ostream& os, const parameter & p);
    bool compare_string(const std::string& value) const;
};

//
// MESSAGE
//

class Message
{
    public:

        int         level_;
        std::string message_;
        std::string path_;

        Message(int level, std::string message, std::string path="");
        std::string json() const;
};

//
// COMPONENT
//

class Component : public Task
{
public:
    Component *     parent_;
    dictionary      info_;
    std::string     path_;
    int             module_start;
    int             start_tick;
    int             startup_first_real_input_step;
    int             startup_all_real_inputs_step;

    Component();

    virtual ~Component() {};

    std::string Info() const override;

    bool Notify(int msg, std::string message, std::string path=""); // Path to componenet with problem

    // Shortcut function for messages and logging

    bool Print(std::string message, std::string path="");
    bool Error(std::string message, std::string path="");
    bool Warning(std::string message, std::string path="");
    bool Debug(std::string message, std::string path="");
    bool Trace(std::string message, std::string path="");

    void AddLogLevel();
    void AddFirstTick();
    void AddInput(dictionary parameters);
    void AddOutput(dictionary parameters);
    void AddOutput(std::string name, int size, std::string description=""); // Must be called from creator function and not from Init
    void ClearOutputs();    // Must be called from creator function and not from Init
    void AddParameter(dictionary parameters);
    void SetParameter(std::string name, std::string value);
    void SetParameter(std::string name, const matrix & value, const std::string & source_value="");
    bool BindParameter(parameter & p,  std::string & name);
    bool ResolveParameter(parameter & p,  std::string & name);
    void Bind(parameter & p, std::string n);   // Bind to parameter in global parameter table
    void Bind(matrix & m, std::string n); // Bind to input or output in global parameter table, or matrix parameter

    parameter & GetParameter(std::string name);
    virtual void SetParameters(); // Can be overridden in modules to set parmeter values in code rather than from the ikc/ikg file; called before Init()
    void Tick() override;
    virtual void Init();
    virtual void Command(std::string command_name, dictionary & parameters); // Used to send commands and arbitrary data structures to modules

    void print() const;
    void info() const;
    std::string json() const;  // json representation of the component
    virtual std::string json(const std::string &); // json representation for name of component
    std::string xml();
    bool ShouldTick() const override;
    int EffectiveFirstTick() const;
    void SyncFirstTickFromParameter();
    std::string StartupFirstRealInputStepString() const;
    std::string StartupAllRealInputsStepString() const;

    bool KeyExists(const std::string & key) const;  // Check if a key exist here or in any parent; this means that GetValue will succeed
    std::string GetValue(const std::string & key) const; // Look up value in dictionary with inheritance
    const Component * GetValueOwner(const std::string & key) const; // Find component that defines a key in the inheritance chain

    Component * GetComponent(const std::string & s); // Get component; sensitive to variables and indirection
    int GetIntValue(const std::string & name, int d=0) const;    // Get value of a key in the context of this component; return deflt if not found

    std::string GetBind(const std::string & name) const;

    matrix & GetBuffer(const std::string & s);

    std::string ComputeValue(const std::string & s);
    double ComputeDouble(const std::string & s);
    int ComputeInt(const std::string & s);
    bool ComputeBool(const std::string & s);
    bool LookupParameter(parameter & p, const std::string & name);

    std::vector<int> EvaluateShapeList(std::string & s);

    bool InputsReady(dictionary d, input_map ingoing_connections);
    int SetInputShape_Flat(dictionary d, input_map ingoing_connections);
    int SetInputShape_Index(dictionary d, input_map ingoing_connections);

    void ResolveConnection(const range & output, range & source, range & target); // Move to connection

    virtual int SetInputSize(dictionary d, input_map ingoing_connections);
    virtual int SetInputSizes(input_map & ingoing_connections);

    virtual int SetOutputShape(dictionary d, input_map ingoing_connections);
    virtual int SetOutputShapes(input_map & ingoing_connections); // Uses the size/shape attribute
    int ApplyOutputAliases();

    virtual int SetSizes(input_map ingoing_connections); // Sets input and output if possible
    void CheckRequiredInputs();

    void CalculateCheckSum(long & check_sum, prime & prime_number); // Calculates a value that depends on all parameters and buffer size

private:
};

typedef std::function<Module *()> ModuleCreator;

class Group : public Component
{
};

//
// MODULE
//

class Module : public Component
{
public:
    Module();
    ~Module();

    int SetOutputShape(dictionary d, input_map ingoing_connections);
    int SetOutputShapes(input_map ingoing_connections); // Uses the size/shape attribute
    int SetSizes(input_map  ingoing_connections); // Sets input and output if possible

    tick_count GetTick() const;
    double GetTickDuration() const;
    double GetTime() const;           // actual or nominal time depending om run mode
    double GetRealTime() const;       // actual time since start
    double GetNominalTime() const;    // nominal time at current tick
    double GetTimeOfDay() const;      // seconds since midnight
    double GetLag() const;

    void ProfilingBegin();
    void ProfilingEnd();
};

//
// CONNECTION
//

class Connection: public Task
{
public:
    std::string source;             // FIXME: Add undescore to names ****
    range       source_range;
    std::string target;
    range       target_range;
    range       delay_range_;
    std::string label_;
    bool        flatten_;

    Connection(std::string s, std::string t, range & delay_range, std::string label="");
    virtual ~Connection() = default;

    range Resolve(const range & source_output);

    void Tick();
    void Print() const;

    std::string Info() const; // FIXME: Make consistent with other classes
};

//
// CLASS
//

class Class
{
public:
    dictionary      info_;
    ModuleCreator   module_creator;
    std::string     name;
    std::string     path;
    // std::map<std::string, std::string>  parameters;

    Class();
    Class(std::string n, std::string p);
    Class(std::string n, ModuleCreator mc);

    void Print() const;
};


//
// REQUEST
//

struct Request
    {
        long       session_id;
        dictionary parameters;
        value      json_body;
        std::string url;
        std::string command; 
        std::string component_path;
        std::string body;

        Request(std::string  uri, long sid=0, std::string body="", std::string content_type="");  // Add client id later
        bool HasJsonBody() const;
        void MergeJsonBodyIntoParameters(bool overwrite=true);
    };

    bool operator==(Request & r, const std::string s);

//
// KERNEL
//

class Kernel
{
public:
    dictionary                              info_;
    std::string                             webui_dir;
    std::string                             user_dir;
    bool                                    auth_enabled_ = false;
    std::string                             auth_password_;
    std::string                             auth_cookie_secret_;
    mutable std::mutex                      auth_mutex_;

    long                                    session_id;
    bool                                    needs_reload;
    bool                                    session_logging_active = false;
    bool                                    process_start_logged = false;
    bool                                    process_exit_logged = false;

    std::recursive_mutex                    kernelLock;  
    std::atomic<bool>                       shutdown;
    std::atomic<int>                        run_mode;
    std::atomic<bool>                       notify_stop_requested = false;
    std::atomic<int>                        process_exit_code = 0;

    dictionary                              current_component_info; // Implivit parameters to create Component
    std::string                             current_component_path;

    double                                  idle_time = 0;         
    int                                     cpu_cores = 1;
    double                                  cpu_usage = 0;
    double                                  last_cpu = 0;

    Timer                                   uptime_timer;   // Measues kernel uptime
    Timer                                   session_timer;  // Measures elapsed wall-clock time for a logged run
    Timer                                   timer;          // Main timer
    Timer                                   intra_tick_timer;
    bool                                    start;          // Start automatically                   

    // Timing parameters and functions
    double                                  tick_duration;  // Desired actual or simulated duration for each tick
    double                                  actual_tick_duration;   // actual time between ticks in real time
    double                                  tick_time_usage;        // Time used to execute each tick in real time
    tick_count                              tick;
    tick_count                              stop_after;
    double                                  lag;            // Lag of a tick in real-time mode
    double                                  lag_min;        // Largest negative lag
    double                                  lag_max;        // Largest positive lag
    double                                  lag_sum;        // Sum |lag|

    std::unique_ptr<ServerSocket>           socket;
    std::mutex                              log_mutex;
    std::vector<Message>                    log;
    size_t                                  dropped_webui_log_messages = 0;
    std::thread                             httpThread;

    struct RequestedUIValue
    {
        std::string root;
        std::string token;
        std::string key;
        std::string source;
        std::string format;
    };

    struct UISnapshot
    {
        uint64_t snapshot_id = 0;
        long session_id = 0;
        tick_count tick = -1;
        double image_timestamp = 0;
        std::string status_json;
        std::string log_json;
        std::unordered_map<std::string, std::string> serialized_values;
    };

    struct UISubscriptionState
    {
        std::unordered_set<std::string> keys;
        double last_seen_time = 0;
        uint64_t delivered_log_snapshot_id = 0;
    };

    uint64_t                                next_ui_snapshot_id = 1;
    std::shared_ptr<const UISnapshot>       current_ui_snapshot;
    std::mutex                              ui_snapshot_mutex;
    std::unordered_map<long, UISubscriptionState> ui_session_subscriptions;
    std::mutex                              ui_subscriptions_mutex;

    Kernel();
    ~Kernel();

    void Clear();        // Remove all non-persistent data and reset kernel variables

    static void *   StartHTTPThread(Kernel * k);

    tick_count GetTick();
    double GetTickDuration(); // Time for each tick in seconds (s)
    double GetTime();   // Time since start (in real time or simulated (tick) time depending on mode)
    double GetRealTime();
    double GetNominalTime(); 
    double GetTimeOfDay();
    double GetLag();

    void CalculateCPUUsage();

    bool Notify(int msg, std::string message, std::string path="");
    bool Print(std::string message);
    bool Warning(std::string message, std::string path="");
    bool Debug(std::string message);
    bool Trace(std::string message);

    void LogProcessStart();
    void LogProcessExit();
    void SetOptions(const options & opts);
    bool HasOption(const std::string & key) const;
    bool IsOptionExplicitlySet(const std::string & key) const;
    std::string GetOption(const std::string & key) const;
    long GetOptionLong(const std::string & key) const;
    std::string GetOptionFilename() const;
    std::string GetOptionFullPath() const;
    std::filesystem::path GetClassDirectory(const std::string & class_name) const;
    bool SanitizeReadPath(const std::filesystem::path & candidate_path, std::filesystem::path & sanitized_path) const;
    bool SanitizeWritePath(const std::filesystem::path & candidate_path, std::filesystem::path & sanitized_path) const;

private:
    friend class Component;
    friend class Connection;
    friend class ComputeEngine;
    friend void SendSessionLogEvent(Kernel & kernel, const std::string & endpoint, const std::string & event_name);
    friend void SendProcessStartLogEvent(Kernel & kernel);
    friend void SendProcessExitLogEvent(Kernel & kernel);

    options                                 options_;
    std::map<std::string, Class>            classes;
    std::map<std::string, std::string>      system_files; // ikg-files
    std::map<std::string, std::string>      examples_files; // ikg-files
    std::map<std::string, std::string>      user_files;   // ikg-files
    std::map<std::string, std::unique_ptr<Component>> components;
    std::vector<Connection>                 connections;
    std::map<std::string, matrix>           buffers;                // IO-structure
    std::map<std::string, int>              max_delays;             // Maximum delay needed for each output
    std::map<std::string, CircularBuffer>   circular_buffers;       // Circular circular_buffers for delayed buffers
    std::map<std::string, parameter>        parameters;

    std::vector<std::vector<Task *>>        tasks;                  // Sorted tasks in groups
    std::unique_ptr<ThreadPool>             thread_pool;

public:
    bool Terminate();
    void ScanClasses(std::string path);

private:
    void ScanFiles(std::string path, bool system=true, bool examples=false);

    void ListClasses();
    void ResolveParameter(parameter & p,  std::string & name);

    void ResolveParameters(); // Find and evaluate value or default
    void CalculateSizes();
    void CalculateDelays();
    void CalculateStartupSteps();
    void InitCircularBuffers();
    void RotateBuffers();
    void ListComponents();
    void ListConnections();
    void ListInputs();
    void ListOutputs();
    void ListBuffers();
    void ListCircularBuffers();
    void ListParameters();
    void ListTasks();
    void PrintLog();
    void PrintProfiling();

    // Functions for creating the network

    void AddInput(std::string name, dictionary parameters=dictionary());
    void AddOutput(std::string name, dictionary parameters=dictionary());
    void AddParameter(std::string name, dictionary params=dictionary());
    void SetParameter(std::string name, std::string value);
    void SetParameter(std::string name, const matrix & value, const std::string & source_value="");
    void AddGroup(dictionary info, std::string path);
    void AddModule(dictionary info, std::string path);
    bool PreparePythonModule(dictionary & info, const std::string & classname);
    void InstantiatePythonModule(dictionary & info, const std::string & path);
    void InstantiateStandardModule(dictionary & info, const std::string & classname, const std::string & path);
    void AddConnection(dictionary info, std::string path);
    void LoadExternalGroup(dictionary & d);
    void BuildGroup(dictionary d, std::string path="");

    void AllocateInputs();
    void InitComponents();
    void PruneConnections();
    void SortTasks();
    void RunTasks();
    void RunTasksInSingleThread();
    void SetUp();
    void SetCommandLineParameters(dictionary & d);
    std::string GetTopLevelDefaultAttribute(const std::string & key) const;

public:
    void RegisterClass(const char * name, ModuleCreator mc);
    void LoadFile();

private:
    void Save();

    void LogStart();
    void LogStop();
    void LogSessionEvent(const std::string & endpoint, const std::string & event_name);

    std::string json();
    std::string xml();

public:
    void InitSocket(long port);
    void StopHTTPServer();

    void New();
    void Stop();
    void Play();
    void Realtime();

private:
    void Pause();
    void Restart(); // Save and reload

    void CalculateCheckSum();
    dictionary GetModuleInstantiationInfo(); // Used for profiling
    std::string GetProfilingJSON() const;
    std::string GetStartupStepsJSON() const;

    void DoNew(Request & request);
    void DoOpen(Request & request);
    void DoSave(Request & request);

    void DoQuit(Request & request);
    void DoStop(Request & request);
    void DoPause(Request & request);
    void DoStep(Request & request);
    void DoPlay(Request & request);
    void DoRealtime(Request & request);
    
    void DoData(Request & request);
    void DoJSON(Request & request);
    void DoCSV(Request & request);
    void DoImage(Request & request);
    void DoProfiling(Request & request);
    void DoStartupSteps(Request & request);

    void DoCommand(Request & request);
    void DoControl(Request & request);
    
    void DoSendNetwork(Request & request);

    void SendStringResponse(ikaros::dictionary header, const std::string & body, const char * response=nullptr);
    std::string DoSendDataStatus();
    std::string NormalizeUIRoot(const std::string & component_path) const;
    double SnapshotInterval() const;
    size_t MaxPendingWebUILogMessages() const;
    int SnapshotJPEGQualityForFormat(const std::string & format) const;
    std::vector<RequestedUIValue> ParseRequestedUIValues(Request & request);
    RequestedUIValue ParseSubscribedUIValue(const std::string & subscription_key) const;
    std::string SubscriptionKeyFor(const RequestedUIValue & requested_value) const;
    bool SerializeRequestedValue(RequestedUIValue requested_value, std::string & serialized_value, long long * compute_us = nullptr, long long * value_us = nullptr);
    std::string SerializePendingLog(bool clear_pending_log);
    std::string ConsumeSnapshotLogForSession(long ui_session_id, const UISnapshot & snapshot);
    void ResetUISnapshotCache();
    void BuildUISnapshot();

    void DoSendData(Request & request);
    void DoUpdate(Request & request);
    void DoAuthStatus();
    void DoLogin(Request & request);
    void DoUnauthorized();

    void DoNetwork(Request & request);
    std::string DoSendLog(Request & request);
    void DoSendClasses(Request & request);
    void DoSendClassInfo(Request & request);
    void DoSendClassReadMe(Request & request);
    void DoSendFileList(Request & request);
    enum class SendFileResult { sent, forbidden, not_found };
    bool SanitizeProjectPath(const std::filesystem::path & candidate_path, std::filesystem::path & sanitized_path) const;
    bool SanitizeImportPath(const std::filesystem::path & candidate_path, std::filesystem::path & sanitized_path) const;
    SendFileResult SendFileIfSafe(const std::filesystem::path & root, const std::string & file);
    void DoSendFile(std::string file);
    void DoSendPublicWebUIFile(std::string file);
    void DoSendError(const std::string & status = "404 Not Found", const std::string & message = "404 Not Found\n");
    std::string SendImage(matrix & image, const std::string & format, int quality=90);
    bool AuthEnabled() const;
    bool IsRequestAuthenticated() const;
    bool IsPublicRequest(const Request & request) const;
    bool CheckPassword(const std::string & candidate) const;
    std::string CreateSessionToken();
    std::string PasswordMarker() const;
    bool LoadOrCreateAuthCookieSecret();

    void HandleHTTPRequest();
    void HandleHTTPThread();
    void Tick();
    void Propagate();

public:
    void Run();

private:

    // TASK SORTING

    bool DFSCycleCheck(const std::string& node, const std::unordered_map<std::string, std::vector<std::string>>& graph, std::unordered_set<std::string>& visited, std::unordered_set<std::string>& recStack);
    bool HasCycle(const std::vector<std::string>& nodes, const std::vector<std::pair<std::string, std::string>>& edges);
    void DFSSubgroup(const std::string& node, const std::unordered_map<std::string, std::vector<std::string>>& graph, std::unordered_set<std::string>& visited, std::vector<std::string>& component) ;
    std::vector<std::vector<std::string>> FindSubgraphs(const std::vector<std::string>& nodes, const std::vector<std::pair<std::string, std::string>>& edges);
    void TopologicalSortUtil(const std::string& node, const std::unordered_map<std::string, std::vector<std::string>>& graph, std::unordered_set<std::string>& visited, std::stack<std::string>& Stack);
    std::vector<std::string>  TopologicalSort(const std::vector<std::string>& component, const std::unordered_map<std::string, std::vector<std::string>>& graph);
    std::vector<std::vector<std::string>> Sort(std::vector<std::string> nodes, std::vector<std::pair<std::string, std::string>> edges);
};

//
// INITIALIZATION CLASS
//

class InitClass
{
public:
    InitClass(const char * name, ModuleCreator mc);
};

}; // namespace ikaros
