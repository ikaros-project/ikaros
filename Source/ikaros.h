// Ikaros 3.0

#pragma once

#include <stdexcept>
#include <atomic>
#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <limits>
#include <optional>
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
#include "Kernel/maths.h"
#include "Kernel/matrix.h"
#include "Kernel/h_matrix.h"
#include "Kernel/socket.h"
#include "Kernel/timing.h"
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
    CircularBuffer() = delete;
    CircularBuffer(const matrix & m, int size);
    CircularBuffer(const CircularBuffer &) = delete;
    CircularBuffer & operator=(const CircularBuffer &) = delete;
    CircularBuffer(CircularBuffer &&) noexcept = default;
    CircularBuffer & operator=(CircularBuffer &&) noexcept = default;
    void rotate(const matrix & m);
    const matrix & get(int i) const;
    int size() const noexcept;

private:
    std::vector<matrix> buffer_;
    int index_;
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
    using parameter_value = std::variant<std::monostate, double, bool, int, std::string, matrix>;

    struct parameter_state
    {
        dictionary               info;
        bool                     has_options = false;
        std::vector<std::string> options;
        bool                     resolved = false;
        parameter_type           type = no_type;
        parameter_value          value;
        bool                     dynamic = false;
        std::optional<double>    minimum;
        std::optional<double>    maximum;
    };

    std::shared_ptr<parameter_state> state_;

    std::shared_ptr<parameter_state> clone_state() const;
    void bind_to(const parameter & p);
    void validate_numeric_value(double value) const;
    matrix * matrix_value() noexcept;
    const matrix * matrix_value() const noexcept;
    matrix & matrix_ref();
    const matrix & matrix_ref() const;
    void set_source_value(const std::string & value);

    friend class Component;
    friend class Kernel;

public:

    parameter();
    parameter(dictionary info);
    parameter(const std::string type, const std::string options="");
    parameter(const parameter & p);
    parameter(parameter && p) noexcept = default;

    parameter & operator=(const parameter & p);
    parameter & operator=(parameter && p) noexcept = default;
    double operator=(double v);
    std::string operator=(std::string v);
    void set_matrix(const matrix & v);

    operator const matrix & () const = delete;
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
    matrix as_matrix() const;
    std::string as_int_string() const; // as_int() converted to string
    std::string as_string() const;
    bool empty() const;
    int size() const;
    float get(int index, float default_value) const;
    float operator[](int index) const;

    parameter_type get_type() const noexcept;
    bool has_options() const noexcept;
    bool is_resolved() const noexcept;
    std::vector<std::string> options() const;
    dictionary metadata() const;

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
private:
    friend class ComputeEngine;
    friend class Connection;
    friend class Kernel;
    friend class Module;

    struct DeferredParameterChange
    {
        std::string parameter_path;
        bool is_matrix_cell = false;
        int x = 0;
        int y = 0;
        std::string value;
    };

    struct DeferredCommand
    {
        std::string command_name;
        dictionary parameters;
    };

    Profiler        profiler_;
    Component *     parent_;
    int             module_start;
    int             start_tick;
    int             startup_first_real_input_step;
    int             startup_all_real_inputs_step;
    bool            initialized_;
    bool            async_mode;
    std::atomic<bool> async_running;
    std::atomic<bool> async_failed;
    std::atomic<bool> async_publish_pending;
    std::atomic<tick_count> async_started_tick;
    std::atomic<tick_count> async_completed_tick;
    std::atomic<int> async_pending_action_count;
    std::mutex      async_state_mutex;
    std::future<std::exception_ptr> async_future;
    std::mutex      async_pending_mutex;
    std::map<std::string, DeferredParameterChange> deferred_parameter_changes;
    std::vector<DeferredCommand> deferred_commands;

    int EffectiveFirstTick() const;
    void SyncFirstTickFromParameter();
    bool IsAsyncPending() const;
    void SyncAsyncModeFromParameter();
    bool PollAsyncCompletion(bool apply_pending_actions = true);
    void LaunchAsyncTick();
    void WaitForAsyncCompletion(bool apply_pending_actions = true);
    void ClearPendingAsyncActions();
    static std::string AsyncParameterChangeKey(const DeferredParameterChange & change);
    void QueueDeferredParameterChange(const DeferredParameterChange & change);
    void QueueDeferredCommand(const std::string & command_name, const dictionary & parameters);
    void ApplyPendingAsyncActions();

protected:
    dictionary      info_;
    std::string     path_;

    Component * Parent() const noexcept { return parent_; }
    bool IsAsyncRunning() const;
    bool IsAsyncFailed() const;
    std::string StartupFirstRealInputStepString() const;
    std::string StartupAllRealInputsStepString() const;

public:
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
    void AddState(dictionary parameters);
    void AddParameter(dictionary parameters);
    void SetParameter(std::string name, std::string value);
    void SetParameter(std::string name, const matrix & value, const std::string & source_value="");
    bool BindParameter(parameter & p,  std::string & name);
    bool ResolveParameter(parameter & p,  std::string & name);
    void Bind(parameter & p, std::string n);   // Bind to parameter in global parameter table
    void Bind(matrix & m, std::string n); // Bind to input or output in global parameter table, or matrix parameter
    void Bind(float & v, std::string n);
    void Bind(double & v, std::string n);
    void Bind(int & v, std::string n);
    void Bind(bool & v, std::string n);
    void Bind(std::string & v, std::string n);

    parameter & GetParameter(std::string name);
    virtual void SetParameters(); // Can be overridden in modules to set parmeter values in code rather than from the ikc/ikg file; called before Init()
    void Tick() override;
    virtual void Init();
    virtual void Stop();
    virtual void Reset();
    virtual void Command(std::string command_name, dictionary & parameters); // Used to send commands and arbitrary data structures to modules

    void print() const;
    void info() const;
    std::string json() const;  // json representation of the component
    virtual std::string json(const std::string &); // json representation for name of component
    std::string xml();
    bool ShouldTick() const override;

    bool KeyExists(const std::string & key) const;  // Check if a key exist here or in any parent; this means that GetValue will succeed
    std::string GetValue(const std::string & key) const; // Look up value in dictionary with inheritance
    const Component * GetValueOwner(const std::string & key) const; // Find component that defines a key in the inheritance chain

    Component * GetComponent(const std::string & s); // Get component; sensitive to variables and indirection
    int GetIntValue(const std::string & name, int d=0) const;    // Get value of a key in the context of this component; return deflt if not found

    std::string GetBind(const std::string & name) const;

    matrix & GetBuffer(const std::string & s);

    std::string ComputeValue(const std::string & s);
    std::string ComputeValueOf(const std::string & name);
    double ComputeDouble(const std::string & s);
    int ComputeInt(const std::string & s);
    bool ComputeBool(const std::string & s);
    bool ComputeAttributeBool(dictionary d, const std::string & name, bool default_value=false);
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
    virtual int SetStateShape(dictionary d);
    virtual int SetStateShapes(input_map ingoing_connections);

    virtual int SetSizes(input_map ingoing_connections); // Sets input and output if possible
    void CheckRequiredInputs();

    void CalculateCheckSum(long & check_sum, prime & prime_number); // Calculates a value that depends on all parameters and buffer size

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
    ~Module() override = default;

    int SetOutputShape(dictionary d, input_map ingoing_connections) override;
    int SetOutputShapes(input_map ingoing_connections) override; // Uses the size/shape attribute
    int SetStateShapes(input_map ingoing_connections) override;
    int SetSizes(input_map ingoing_connections) override; // Sets input and output if possible

    tick_count GetTick() const;
    double GetTickDuration() const;
    double GetTime() const;           // actual or nominal time depending om run mode
    double GetRealTime() const;       // actual time since start
    double GetNominalTime() const;    // nominal time at current tick
    double GetTimeOfDay() const;      // seconds since midnight
    double GetLag() const;
    double GetUptime() const;
    double GetActualTickDuration() const;
    double GetTickTimeUsage() const;
    double GetCPUUsage() const;
    double GetIdleTime() const;
    int GetRunMode() const;
    int GetCPUCoreCount() const;
    int GetModuleCount() const;
    int GetClassCount() const;
    tick_count GetStopAfter() const;

    bool TryProfilingBegin() override;
    void ProfilingBegin() override;
    void ProfilingEnd() override;
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
    bool        source_indexed_;
    bool        target_indexed_;
    bool        stacked_;
    bool        shared_memory_;
    matrix *    source_buffer_ = nullptr;
    matrix *    target_buffer_ = nullptr;
    CircularBuffer * circular_buffer_ = nullptr;
    Component * source_component_ = nullptr;
    Component * target_component_ = nullptr;
    bool        has_async_endpoint_ = false;

    Connection(std::string s, std::string t, range & delay_range, std::string label="");
    virtual ~Connection() = default;

    int DelayCount() const;
    int MinDelay() const;
    int MaxDelay() const;
    bool HasZeroDelay() const;
    bool IsSingleDelay(int delay) const;
    bool UsesCircularBuffer() const;
    bool ShouldTick() const override;
    void ResolveRuntimeState();

    range Resolve(const range & source_output);
    bool IsWholeMatrixConnection() const;

    void Tick() override;
    void Print() const;

    std::string Info() const override; // FIXME: Make consistent with other classes
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
        long       client_id;
        dictionary parameters;
        value      json_body;
        std::string url;
        std::string command; 
        std::string component_path;
        std::string body;

        Request(std::string uri, long sid=0, std::string body="", std::string content_type="", long cid=0);
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
    std::string                             top_group_path;

    double                                  idle_time = 0;         
    int                                     cpu_cores = 1;
    double                                  cpu_usage = 0;
    double                                  last_cpu = 0;
    bool                                    cpu_usage_initialized = false;
    std::chrono::steady_clock::time_point   cpu_usage_sample_time;

    Timer                                   uptime_timer;   // Measues kernel uptime
    Timer                                   session_timer;  // Measures elapsed wall-clock time for a logged run
    Timer                                   timer;          // Main timer
    Timer                                   intra_tick_timer;
    bool                                    start;          // Start automatically                   

    // Timing parameters and functions
    double                                  tick_duration;  // Desired actual or simulated duration for each tick
    double                                  task_timeout;   // Watchdog timeout for synchronous task execution; zero disables it
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
    uint64_t                                first_webui_log_sequence = 1;
    uint64_t                                next_webui_log_sequence = 1;
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
        uint64_t subscription_revision = 0;
        long session_id = 0;
        tick_count tick = -1;
        std::chrono::steady_clock::time_point timestamp;
        std::chrono::steady_clock::time_point image_timestamp;
        std::string status_json;
        std::unordered_map<std::string, std::string> serialized_values;
    };

    struct UIClientState
    {
        std::unordered_set<std::string> keys;
        std::chrono::steady_clock::time_point last_seen_time;
        uint64_t delivered_log_sequence = 0;
        bool log_delivery_initialized = false;
    };

    uint64_t                                next_ui_snapshot_id = 1;
    uint64_t                                ui_subscription_revision = 1;
    std::shared_ptr<const UISnapshot>       current_ui_snapshot;
    std::mutex                              ui_snapshot_mutex;
    std::unordered_map<long, UIClientState>       ui_client_states;
    std::mutex                                    ui_client_mutex;
    std::unordered_map<long, std::chrono::steady_clock::time_point> profiling_clients;
    std::mutex                                    profiling_clients_mutex;
    std::atomic<bool>                             profiling_enabled = false;

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
    double GetUptime();
    double GetActualTickDuration() const;
    double GetTickTimeUsage() const;
    double GetCPUUsage() const;
    double GetIdleTime() const;
    int GetRunMode() const;
    int GetCPUCoreCount() const;
    int GetModuleCount() const;
    int GetClassCount() const;
    tick_count GetStopAfter() const;
    bool ProfilingEnabled() const;

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
    friend class KernelTaskSequence;
    friend class ComputeEngine;
    friend void SendSessionLogEvent(Kernel & kernel, const std::string & endpoint, const std::string & event_name);
    friend void SendProcessStartLogEvent(Kernel & kernel);
    friend void SendProcessExitLogEvent(Kernel & kernel);

    options                                 options_;
    std::map<std::string, Class>            classes;
    std::map<std::string, std::string>      system_files; // ikg-files
    std::map<std::string, std::string>      examples_files; // ikg-files
    std::map<std::string, std::string>      user_files;   // ikg-files
    std::map<std::string, std::string>      user_state_files; // state-files
    std::map<std::string, std::unique_ptr<Component>> components;
    std::mutex                              component_lifecycle_mutex;
    std::vector<Connection>                 connections;
    std::map<std::string, matrix>           buffers;                // IO-structure
    std::set<std::string>                   state_buffers;
    std::set<std::string>                   persistent_outputs;
    std::set<std::string>                   persistent_state_buffers;
    struct ScalarState
    {
        std::string type;
        bool persistent = false;
        float float_value = 0;
        float default_float_value = 0;
        double double_value = 0;
        double default_double_value = 0;
        int int_value = 0;
        int default_int_value = 0;
        bool bool_value = false;
        bool default_bool_value = false;
        std::string string_value;
        std::string default_string_value;
        float * float_ptr = nullptr;
        double * double_ptr = nullptr;
        int * int_ptr = nullptr;
        bool * bool_ptr = nullptr;
        std::string * string_ptr = nullptr;
    };
    std::map<std::string, ScalarState>       scalar_states;
    struct DelayedSourceHistory
    {
        CircularBuffer buffer;
        matrix * source_buffer;
        Component * source_component;
        tick_count last_async_completion = -1;

        DelayedSourceHistory(matrix & source, int size, Component * component):
            buffer(source, size),
            source_buffer(&source),
            source_component(component)
        {
        }
    };

    std::map<std::string, int>                  max_delays;       // Maximum delay needed for each output
    std::map<std::string, DelayedSourceHistory> circular_buffers; // Histories for delayed outputs
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
    void ShareZeroDelayConnectionBuffers();
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
    void AddState(std::string name, dictionary parameters=dictionary());
    void AddParameter(std::string name, dictionary params=dictionary());
    void SetParameter(std::string name, std::string value);
    void SetParameter(std::string name, const matrix & value, const std::string & source_value="");
    void AddGroup(dictionary info, std::string path, bool is_top_group);
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
    void RunTask(Task * task);
    void PollAsyncComponents();
    bool ValueOwnedByRunningAsyncComponent(const std::string & value_path) const;
    Component * ComponentForValuePath(const std::string & value_path) const;
    void WaitForAsyncComponents(bool discard_pending_actions);
    std::optional<std::string> RunTasks();
    void RunTasksInSingleThread();
    void SetUp();
    void SetCommandLineParameters(dictionary & d);
    std::string GetTopLevelDefaultAttribute(const std::string & key) const;

public:
    void RegisterClass(const char * name, ModuleCreator mc);
    void LoadFile();

private:
    void Save();
    void SaveState(const std::string & filename, const std::string & component_path="");
    void LoadState(const std::string & filename, const std::string & component_path="");
    void ResetState(const std::string & component_path="");

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
    void StopComponents();
    void Pause();
    void Restart(); // Save and reload

    void CalculateCheckSum();
    dictionary GetModuleInstantiationInfo(); // Used for profiling
    std::string GetProfilingJSON() const;
    std::string GetStartupStepsJSON() const;
    void SetProfilingClientActive(long client_id, bool active);
    void UpdateProfilingState();

    void DoNew(Request & request);
    void DoOpen(Request & request);
    void DoSave(Request & request);
    void DoSaveState(Request & request);
    void DoLoadState(Request & request);
    void DoResetState(Request & request);

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
    const parameter * FindTopGroupParameter(const std::string & name) const;
    double WebUIRequestInterval() const;
    double SnapshotInterval() const;
    size_t MaxRetainedWebUILogMessages() const;
    int SnapshotJPEGQualityForFormat(const std::string & format) const;
    std::vector<RequestedUIValue> ParseRequestedUIValues(Request & request);
    RequestedUIValue ParseSubscribedUIValue(const std::string & subscription_key) const;
    std::string SubscriptionKeyFor(const RequestedUIValue & requested_value) const;
    bool SerializeRequestedValue(RequestedUIValue requested_value, std::string & serialized_value, long long * compute_us = nullptr, long long * value_us = nullptr);
    std::string ConsumeLogForClient(long ui_client_id);
    void ResetUISnapshotCache();
    void BuildUISnapshot(bool respect_rate_limit = false);

    void DoSendData(Request & request, bool refresh_paused_snapshot = true, bool use_snapshot_status = false);
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
    bool SanitizePathUnderRoot(const std::filesystem::path & root, const std::filesystem::path & candidate_path, std::filesystem::path & sanitized_path) const;
    bool SanitizeImportPath(const std::filesystem::path & candidate_path, std::filesystem::path & sanitized_path) const;
    void LoadXMLWithRestrictedIncludes(dictionary & d, const std::filesystem::path & filename) const;
    SendFileResult SendFileIfSafe(const std::filesystem::path & root, const std::string & file);
    SendFileResult SendPublicWebUIFileIfSafe(const std::filesystem::path & root, const std::string & file);
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
    bool Tick();
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
