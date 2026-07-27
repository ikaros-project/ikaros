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
#include <future>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <limits>
#include <optional>
#include <random>
#include <set>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

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
#include "Kernel/parameter.h"
#include "Kernel/component.h"
#include "Kernel/module.h"
#include "Kernel/circular_buffer.h"
#include "Kernel/connection.h"
#include "Kernel/request.h"
#include "Kernel/module_class.h"


namespace ikaros 
{
class Kernel;
class KernelMainAccess;
class KernelSessionLoggingAccess;

Kernel& kernel();

//
// KERNEL
//

class Kernel
{
private:
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
    std::chrono::steady_clock::time_point   run_clock_origin;
    double                                  run_time = 0;
    bool                                    run_clock_started = false;

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
    std::optional<Timer>                    realtime_resync_warning_timer;
    bool                                    realtime_resync_warning_sent = false;
    std::optional<Timer>                    realtime_lag_warning_timer;

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

    struct UISnapshotBuildPlan
    {
        std::chrono::steady_clock::time_point now;
        std::shared_ptr<const UISnapshot> previous_snapshot;
        std::unordered_set<std::string> subscriptions;
        uint64_t subscription_revision = 0;
        bool has_active_clients = false;
        bool snapshot_due = false;
    };

    struct DataSnapshotItem
    {
        std::string prefix;
        std::string value;
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

public:
    Kernel();
    ~Kernel();

    void Clear();        // Remove all non-persistent data and reset kernel variables

    static void *   StartHTTPThread(Kernel * k);

    tick_count GetTick(); // First execution tick is 1; tick 0 is the initialized state
    double GetTickDuration(); // Time for each tick in seconds (s)
    double GetTime();   // Time since start (in real time or simulated (tick) time depending on mode)
    double GetRealTime();
    double GetNominalTime(); // Current tick * tick duration; endpoint of the tick interval
    double GetRunTime();
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
    friend class KernelMainAccess;
    friend class KernelSessionLoggingAccess;
    friend void QueueSessionLogEvent(Kernel & kernel, const std::string & endpoint, const std::string & event_name);
    friend void QueueProcessStartLogEvent(Kernel & kernel);
    friend void QueueProcessExitLogEvent(Kernel & kernel);

    static long NewSessionID();

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

        value CurrentValue() const;
        void RestoreValue(const value & saved_value);
        void Reset();
    };
    std::map<std::string, ScalarState>       scalar_states;
    template<typename T>
    void BindScalarState(T & value, const std::string & name,
                         const std::string & component_path,
                         const std::string & expected_type,
                         T ScalarState::* stored_value,
                         T * ScalarState::* bound_value);
    dictionary CaptureState(const std::string & component_path) const;
    void RestoreState(const dictionary & state, const std::string & component_path,
                      const std::string & source_name);
    connection_map BuildIncomingConnections();
    std::vector<std::string> PendingBufferSizes(input_map incoming_connections);
    std::size_t BufferSizeSignature() const;
    void PropagateBufferSizes(input_map incoming_connections);
    std::map<std::string, int> PropagateStartupBufferSteps(
        input_map incoming_connections);
    void ApplyStartupComponentSteps(input_map incoming_connections,
                                    const std::map<std::string, int> & buffer_first_real_step);
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
    std::atomic<bool>                       automatic_reload_suppressed_until_save = false;

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

    void InitComponents();
    void PruneConnections();
    void SortTasks();
    void RunTask(Task * task);
    void PollAsyncComponents();
    bool ValueOwnedByRunningAsyncComponent(const std::string & value_path) const;
    Component * ComponentForValuePath(const std::string & value_path) const;
    void WaitForAsyncComponents(bool discard_pending_actions);
    using submitted_task_sequences = std::vector<std::shared_ptr<TaskSequence>>;
    void RecordTaskFailure(std::optional<exception> & failure,
                           const std::string & context,
                           const std::string & fallback_path = "");
    std::optional<exception> SubmitTaskSequences(submitted_task_sequences & sequences);
    bool WaitForTaskWatchdog(const submitted_task_sequences & sequences);
    void WaitForTaskCompletionBarrier(const submitted_task_sequences & sequences,
                                      std::optional<exception> & failure);
    void CollectTaskSequenceFailures(const submitted_task_sequences & sequences,
                                     std::optional<exception> & failure);
    void WaitForRealtimeTick();
    void WaitForRunMode(bool has_async_workers);
    void ReportRealtimeLag();
    std::optional<exception> RunTasks();
    void RunTasksInSingleThread();
    void SetUp();
    void SetCommandLineParameters(dictionary & d);
    void HandleFailedFileLoad();
    std::string GetTopLevelDefaultAttribute(const std::string & key) const;

public:
    void RegisterClass(const char * name, ModuleCreator mc);
    void LoadFileConfiguration();
    void SetUpLoadedFile();
    void LoadFile();
    bool AutomaticReloadSuppressed() const;
    void SuppressAutomaticReloadUntilSave();

private:
    std::string ResolveStateFilename(const std::string & option_name) const;
    std::string ResolveStateFilenameFromRequest(const Request & request,
                                                const std::string & option_name) const;
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
    bool StopComponents();
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
    UISnapshotBuildPlan PlanUISnapshotBuild(bool respect_rate_limit);
    void PopulateUISnapshot(UISnapshot & snapshot, const UISnapshotBuildPlan & plan);
    void PublishUISnapshot(std::shared_ptr<UISnapshot> snapshot);
    void BuildUISnapshot(bool respect_rate_limit = false);
    bool UpdateUIClientSubscriptions(long client_id,
                                     const std::vector<RequestedUIValue> & requested_values);
    std::shared_ptr<const UISnapshot> CurrentUISnapshot();
    std::string BuildUIDataResponse(const std::string & status,
                                    const std::vector<DataSnapshotItem> & response_items,
                                    const std::string & log_json) const;

    void DoSendData(Request & request, bool refresh_paused_snapshot = true, bool use_snapshot_status = false);
    void DoUpdate(Request & request);
    void DoAuthStatus();
    void DoLogin(Request & request);
    void DoUnauthorized();

    void DoNetwork(Request & request);
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
    std::string SendImage(const matrix & image, const std::string & format, int quality=90);
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
    bool Run(); // Returns true when no additional model Stop is required.

private:

    // TASK SORTING

    bool HasCycle(const std::vector<std::string> & nodes, const std::vector<std::pair<std::string, std::string>> & edges);
    std::vector<std::vector<std::string>> FindSubgraphs(const std::vector<std::string> & nodes, const std::vector<std::pair<std::string, std::string>> & edges);
    std::vector<std::string> TopologicalSort(const std::vector<std::string> & component, const std::unordered_map<std::string, std::vector<std::string>> & graph);
    std::vector<std::vector<std::string>> Sort(const std::vector<std::string> & nodes, const std::vector<std::pair<std::string, std::string>> & edges);
};

}; // namespace ikaros
