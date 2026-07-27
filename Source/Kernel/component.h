// Ikaros 3.0

#pragma once

#include <atomic>
#include <exception>
#include <future>
#include <map>
#include <mutex>
#include <string>
#include <vector>

#include "dictionary.h"
#include "kernel_types.h"
#include "matrix.h"
#include "parameter.h"
#include "profiler.h"
#include "range.h"
#include "thread_pool.h"
#include "utilities.h"

namespace ikaros
{
    class ComputeEngine;
    class Connection;
    class Kernel;
    class Module;

    using connection_map = std::map<std::string, std::vector<Connection *>>;
    using input_map = const connection_map &;

    class Message
    {
    public:
        int level_;
        std::string message_;
        std::string path_;

        Message(int level, std::string message, std::string path = "");
        std::string json() const;
    };

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

        Profiler profiler_;
        Component * parent_;
        int module_start;
        int start_tick;
        int startup_first_real_input_step;
        int startup_all_real_inputs_step;
        bool initialized_;
        bool async_mode;
        std::atomic<bool> async_running;
        std::atomic<bool> async_failed;
        std::atomic<bool> async_publish_pending;
        std::atomic<tick_count> async_started_tick;
        std::atomic<tick_count> async_completed_tick;
        std::atomic<int> async_pending_action_count;
        std::mutex async_state_mutex;
        std::future<std::exception_ptr> async_future;
        std::mutex async_pending_mutex;
        std::map<std::string, DeferredParameterChange> deferred_parameter_changes;
        std::vector<DeferredCommand> deferred_commands;

        static dictionary LogLevelParameterInfo();
        static dictionary ModuleStartParameterInfo();
        static dictionary StartTickParameterInfo();
        static dictionary AsyncParameterInfo();
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
        bool GetRawParameterValue(const parameter & p, const std::string & name,
                                  std::string & raw_value, Component *& context) const;
        std::string MatrixParameterShapeExpression(const parameter & p) const;
        matrix ApplyParameterShape(const parameter & p, const matrix & value);
        void ResolveParameterValue(parameter & p, const std::string & name,
                                   const std::string & raw_value, Component * context);
        std::string ShapeString(const std::vector<int> & shape) const;
        void ValidateFixedInputTarget(const std::string & name, const std::string & full_name,
                                      const Connection & connection, const range & target_range,
                                      bool flattened);
        void ApplyInputLabel(const dictionary & input, const std::string & full_name,
                             const std::vector<Connection *> & connections);
        int SetStackedInputShape(const dictionary & input, const std::string & name,
                                 const std::string & full_name, bool has_fixed_size,
                                 const std::vector<Connection *> & connections);
        int SetSimpleInputShape(const dictionary & input, const std::string & full_name,
                                Connection & connection,
                                const std::vector<Connection *> & connections);
        int SetGeneralInputShape(const dictionary & input, const std::string & name,
                                 const std::string & full_name, bool has_fixed_size,
                                 const std::vector<Connection *> & connections);
        static std::string StartupStepString(int step);

    protected:
        dictionary info_;
        std::string path_;

        Component * Parent() const noexcept { return parent_; }
        bool IsAsyncRunning() const;
        bool IsAsyncFailed() const;
        std::string StartupFirstRealInputStepString() const;
        std::string StartupAllRealInputsStepString() const;

    public:
        Component();
        virtual ~Component() {};

        std::string Info() const override;
        bool Notify(int msg, std::string message, std::string path = "");
        bool Print(std::string message, std::string path = "");
        bool Error(std::string message, std::string path = "");
        bool Warning(std::string message, std::string path = "");
        bool Debug(std::string message, std::string path = "");
        bool Trace(std::string message, std::string path = "");

        void AddLogLevel();
        void AddFirstTick();
        void AddInput(dictionary parameters);
        void AddOutput(dictionary parameters);
        void AddOutput(std::string name, int size, std::string description = "");
        void AddState(dictionary parameters);
        void AddParameter(dictionary parameters);
        void SetParameter(std::string name, std::string value);
        void SetParameter(std::string name, const matrix & value,
                          const std::string & source_value = "");
        bool BindParameter(parameter & p, std::string & name);
        bool ResolveParameter(parameter & p, std::string & name);
        void Bind(parameter & p, std::string n);
        void Bind(matrix & m, std::string n);
        void Bind(float & v, std::string n);
        void Bind(double & v, std::string n);
        void Bind(int & v, std::string n);
        void Bind(bool & v, std::string n);
        void Bind(std::string & v, std::string n);

        parameter & GetParameter(std::string name);
        virtual void SetParameters();
        void Tick() override;
        virtual void Init();
        virtual void Stop();
        virtual void Reset();
        virtual void Command(std::string command_name, dictionary & parameters);

        void print() const;
        void info() const;
        std::string json() const;
        virtual std::string json(const std::string &);
        std::string xml();
        bool ShouldTick() const override;

        bool KeyExists(const std::string & key) const;
        std::string GetValue(const std::string & key) const;
        const Component * GetValueOwner(const std::string & key) const;
        Component * GetComponent(const std::string & s);
        int GetIntValue(const std::string & name, int d = 0) const;
        std::string GetBind(const std::string & name) const;
        matrix & GetBuffer(const std::string & s);

        std::string ComputeValue(const std::string & s);
        std::string ComputeValueOf(const std::string & name);
        double ComputeDouble(const std::string & s);
        int ComputeInt(const std::string & s);
        bool ComputeBool(const std::string & s);
        bool ComputeAttributeBool(dictionary d, const std::string & name,
                                  bool default_value = false);
        bool LookupParameter(parameter & p, const std::string & name);
        std::vector<int> EvaluateShapeList(std::string & s);

        bool InputsReady(dictionary d, input_map ingoing_connections);
        int SetInputShape_Flat(dictionary d, input_map ingoing_connections);
        int SetInputShape_Index(dictionary d, input_map ingoing_connections);
        void ResolveConnection(const range & output, range & source, range & target);
        virtual int SetInputSize(dictionary d, input_map ingoing_connections);
        virtual int SetInputSizes(input_map & ingoing_connections);
        virtual int SetOutputShape(dictionary d, input_map ingoing_connections);
        virtual int SetOutputShapes(input_map & ingoing_connections);
        int ApplyOutputAliases();
        virtual int SetStateShape(dictionary d);
        virtual int SetStateShapes(input_map ingoing_connections);
        virtual int SetSizes(input_map ingoing_connections);
        void CheckRequiredInputs();
        void CalculateCheckSum(long & check_sum, prime & prime_number);
    };

    class Group : public Component
    {
    };
}
