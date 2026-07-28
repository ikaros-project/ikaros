// Ikaros 3.0

#include "ikaros.h"
#include "session_logging.h"

#include <cmath>
#include <thread>

using namespace ikaros;
using namespace std::chrono;

extern std::atomic<bool> global_terminate;

namespace ikaros
{
    bool
    Kernel::Tick()
    {
        auto run_stage = [](const char * description, auto && operation)
        {
            try
            {
                operation();
            }
            catch(const fatal_runtime_error & e)
            {
                throw fatal_runtime_error(std::string("While ") + description + ": " + e.message(), e.path());
            }
            catch(const exception & e)
            {
                throw exception(std::string("While ") + description + ": " + e.message(), e.path());
            }
            catch(const std::exception & e)
            {
                throw exception(std::string("While ") + description + ": " + e.what());
            }
            catch(...)
            {
                throw exception(std::string("While ") + description + ": Unknown error.");
            }
        };

        UpdateProfilingState();
        const bool profiling_enabled = ProfilingEnabled();
        const auto now = std::chrono::steady_clock::now();
        if(!run_clock_started)
        {
            run_clock_origin = now;
            run_clock_started = true;
        }
        run_time =
            std::chrono::duration<double>(now - run_clock_origin).count();
        tick++;

        PollAsyncComponents();
        if(auto failure = RunTasks(profiling_enabled))
        {
            Notify(msg_fatal_error, failure->message(), failure->path());
            return false;
        }
        //RunTasksInSingleThread();

        run_stage("saving matrix state after task execution", []() { save_matrix_states(); });
        run_stage("rotating delayed buffers", [this]() { RotateBuffers(); });
        run_stage("propagating connection buffers", [this]() { Propagate(); });
        run_stage("calculating CPU usage", [this]() { CalculateCPUUsage(); });
        return true;
    }


    void 
    Kernel::Propagate()
    {
        for(std::span<Connection> connection_span : post_task_connection_spans)
            for(Connection & connection : connection_span)
                if(!connection.ShouldTick())
                    continue;
                else
                    try
                    {
                        connection.Tick();
                    }
                    catch(const std::exception & e)
                    {
                        throw std::runtime_error("Error propagating connection \"" +
                                                 connection.Info() + "\": " + e.what());
                    }
                    catch(...)
                    {
                        throw std::runtime_error("Unknown error propagating connection \"" +
                                                 connection.Info() + "\".");
                    }
    }



    void 
    Kernel::PruneConnections()
    {
        for (auto it = connections.begin(); it != connections.end(); ) 
        {
            if(state_buffers.count(it->source) || state_buffers.count(it->target))
            {
                Notify(msg_warning, "Connection \"" + it->Info() + "\" uses private state and can not be connected.");
                it = connections.erase(it);
            }
            else
            if(buffers.count(it->source) && buffers.count(it->target))
                it++;
            else
            {
                Notify(msg_print, "Pruning " + it->source + "=>" + it->target);
                it = connections.erase(it);
            }
        }
    }


    Component *
    Kernel::ComponentForValuePath(const std::string & value_path) const
    {
        std::string component_path = peek_rhead(value_path, ".");
        auto component = components.find(component_path);
        if(component == components.end())
            return nullptr;
        return component->second.get();
    }


    bool
    Kernel::ValueOwnedByRunningAsyncComponent(const std::string & value_path) const
    {
        Component * component = ComponentForValuePath(value_path);
        return component != nullptr && component->IsAsyncRunning();
    }


    void
    Kernel::RunTask(Task * task, bool profiling_enabled)
    {
        if(task == nullptr)
            return;
        if(!task->ShouldTick())
            return;

        if(task->kind() == Task::Kind::component)
        {
            auto * component = static_cast<Component *>(task);
            if(component->async_mode)
            {
                if(component->async_publish_pending.exchange(false))
                    return;
                if(component->IsAsyncRunning() || component->IsAsyncFailed())
                    return;
                if(stop_after != -1 && tick >= stop_after)
                    return;

                component->LaunchAsyncTick();
                return;
            }
        }

        if(profiling_enabled)
            task->ProfilingBegin();
        try
        {
            task->Tick();
        }
        catch(const exception & e)
        {
            if(profiling_enabled)
                task->ProfilingEnd();
            throw exception("While running task \"" + task->Info() + "\": " + e.message(),
                            e.path().empty() ? task->Info() : e.path());
        }
        catch(const std::exception & e)
        {
            if(profiling_enabled)
                task->ProfilingEnd();
            throw exception("While running task \"" + task->Info() + "\": " + e.what(), task->Info());
        }
        catch(...)
        {
            if(profiling_enabled)
                task->ProfilingEnd();
            throw exception("While running task \"" + task->Info() + "\": Unknown error.", task->Info());
        }
        if(profiling_enabled)
            task->ProfilingEnd();
    }


    void
    Kernel::PollAsyncComponents()
    {
        for(Component * component : async_components)
        {
            const std::string & path = component->path_;

            try
            {
                component->PollAsyncCompletion();
            }
            catch(const exception & e)
            {
                Notify(msg_fatal_error,
                       "While completing asynchronous Tick for module \"" + path + "\": " + e.message(),
                       e.path().empty() ? path : e.path());
            }
            catch(const std::exception & e)
            {
                Notify(msg_fatal_error,
                       "While completing asynchronous Tick for module \"" + path + "\": " + e.what(), path);
            }
            catch(...)
            {
                Notify(msg_fatal_error,
                       "While completing asynchronous Tick for module \"" + path + "\": Unknown error.", path);
            }
        }
    }


    class KernelTaskSequence: public TaskSequence
    {
    public:
        KernelTaskSequence(Kernel & kernel, const std::vector<Task *> & tasks,
                           bool profiling_enabled):
            TaskSequence(tasks),
            kernel_(kernel),
            profiling_enabled_(profiling_enabled)
        {
        }

    protected:
        void Tick() override
        {
            for(auto & task : tasks_)
            {
                try
                {
                    kernel_.RunTask(task, profiling_enabled_);
                }
                catch(const exception & e)
                {
                    throw exception("While executing task sequence: " + e.message(), e.path());
                }
                catch(const std::exception & e)
                {
                    throw exception("While executing task sequence: " + std::string(e.what()),
                                    task ? task->Info() : std::string());
                }
                catch(...)
                {
                    throw exception("While executing task sequence for task \"" +
                                    (task ? task->Info() : std::string("<null>")) + "\": Unknown error.",
                                    task ? task->Info() : std::string());
                }
            }
        }

    private:
        Kernel & kernel_;
        bool profiling_enabled_;
    };


    void
    Kernel::RecordTaskFailure(std::optional<exception> & failure,
                              const std::string & context,
                              const std::string & fallback_path)
    {
        if(failure)
            return;

        try
        {
            std::rethrow_exception(std::current_exception());
        }
        catch(const exception & e)
        {
            failure = exception(context + ": " + e.message(),
                                e.path().empty() ? fallback_path : e.path());
        }
        catch(const std::exception & e)
        {
            failure = exception(context + ": " + e.what(), fallback_path);
        }
        catch(...)
        {
            failure = exception(context + ": Unknown error.", fallback_path);
        }
    }


    std::optional<exception>
    Kernel::SubmitTaskSequences(submitted_task_sequences & sequences,
                                bool profiling_enabled)
    {
        sequences.reserve(tasks.size());
        std::optional<exception> failure;
        try
        {
            for(auto & task_sequence : tasks)
            {
                auto sequence = std::make_shared<KernelTaskSequence>(*this, task_sequence,
                                                                     profiling_enabled);
                thread_pool->submit(sequence);
                sequences.push_back(sequence);
            }
        }
        catch(...)
        {
            RecordTaskFailure(failure, "While submitting task sequences");
        }
        return failure;
    }


    bool
    Kernel::WaitForTaskWatchdog(const submitted_task_sequences & sequences)
    {
        if(task_timeout <= 0)
            return false;

        const auto timeout = duration_cast<steady_clock::duration>(duration<double>(task_timeout));
        const auto deadline = steady_clock::now() + timeout;
        for(const auto & sequence : sequences)
        {
            if(sequence->isCompleted())
                continue;

            const auto now = steady_clock::now();
            if(now >= deadline ||
               !sequence->waitForCompletion(duration<double>(deadline - now).count()))
            {
                try
                {
                    Notify(msg_warning, "Task execution exceeded " + formatNumber(task_timeout) +
                           " seconds. Waiting for active tasks to finish before stopping safely.");
                }
                catch(...)
                {
                    // The completion barrier must still run if watchdog reporting fails.
                }
                return true;
            }
        }
        return false;
    }


    void
    Kernel::WaitForTaskCompletionBarrier(const submitted_task_sequences & sequences,
                                         std::optional<exception> & failure)
    {
        // Do not inspect failures or return while sequences can still access kernel data.
        for(const auto & sequence : sequences)
            try
            {
                sequence->waitForCompletion();
            }
            catch(...)
            {
                RecordTaskFailure(failure, "While waiting for task sequence completion");
            }
    }


    void
    Kernel::CollectTaskSequenceFailures(const submitted_task_sequences & sequences,
                                        std::optional<exception> & failure)
    {
        for(const auto & sequence : sequences)
            try
            {
                sequence->rethrowIfError();
            }
            catch(...)
            {
                RecordTaskFailure(failure, "During task execution");
            }
    }


    std::optional<exception>
    Kernel::RunTasks(bool profiling_enabled)
    {
        submitted_task_sequences sequences;
        std::optional<exception> failure = SubmitTaskSequences(sequences, profiling_enabled);
        bool timed_out = !failure && WaitForTaskWatchdog(sequences);

        WaitForTaskCompletionBarrier(sequences, failure);
        if(timed_out)
            failure = exception("Task execution timed out after " +
                                formatNumber(task_timeout) + " seconds.");
        CollectTaskSequenceFailures(sequences, failure);

        return failure;
    }



    void
    Kernel::RunTasksInSingleThread()
    {
        const bool profiling_enabled = ProfilingEnabled();
        for(auto & task_group : tasks)
            for(auto & task: task_group)
                RunTask(task, profiling_enabled);
    }


    void
    Kernel::WaitForRealtimeTick()
    {
        const double target_time = double(tick + 1) * tick_duration;
        lag = timer.WaitUntil(target_time);

        const bool catch_up =
            !info_.contains("real_time_catch_up") || info_.is_set("real_time_catch_up");
        double resync_lag = 1.0;
        if(info_.contains_non_null("real_time_resync_lag"))
            resync_lag = std::max(0.0, info_["real_time_resync_lag"].as_double());

        const bool resync_lag_exceeded = resync_lag > 0 && lag > resync_lag;
        if(!resync_lag_exceeded && (catch_up || lag <= 0))
            return;

        if(resync_lag_exceeded &&
           (!realtime_resync_warning_sent ||
            realtime_resync_warning_timer->GetTime() >= 1.0))
        {
            Notify(msg_warning, "Realtime lag exceeded " + formatNumber(resync_lag) +
                   " seconds. Resynchronizing realtime clock instead of catching up missed ticks.");
            if(!realtime_resync_warning_timer)
                realtime_resync_warning_timer.emplace();
            realtime_resync_warning_timer->Restart();
            realtime_resync_warning_sent = true;
        }

        timer.SetTime(target_time);
        lag = 0;
    }


    void
    Kernel::WaitForRunMode(bool has_async_workers)
    {
        if(run_mode.load() == run_mode_realtime)
            WaitForRealtimeTick();
        else if(run_mode.load() == run_mode_play)
        {
            timer.SetTime(double(tick + 1) * tick_duration);
            lag = 0;
            if(!options_.is_set("batch_mode") || has_async_workers)
                Sleep(0.01);
        }
        else if(run_mode.load() == run_mode_fast_forward)
        {
            timer.SetTime(double(tick + 1) * tick_duration);
            lag = 0;
            if(has_async_workers)
                Sleep(0.01);
        }
        else
            Sleep(0.01);
    }


    void
    Kernel::ReportRealtimeLag()
    {
        if(run_mode.load() != run_mode_realtime)
            return;

        if(!realtime_lag_warning_timer)
            realtime_lag_warning_timer.emplace();
        if(lag > 1.0 && realtime_lag_warning_timer->GetTime() >= 1.0)
        {
            Notify(msg_warning, "Performance warning: System is " + formatNumber(lag) +
                   " seconds behind real time. Consider increasing tick_duration.");
            realtime_lag_warning_timer->Restart();
        }
    }


    bool
    Kernel::Run()
    {
        bool stop_completed = run_mode.load() == run_mode_quit;
        const bool has_async_workers = !async_components.empty();

        // Main loop
        while(run_mode.load() > run_mode_quit &&
              run_mode.load() != run_mode_restart &&
              !global_terminate.load())  // Not quit
        {
            while(!Terminate() &&
                  run_mode.load() > run_mode_quit &&
                  run_mode.load() != run_mode_restart)
            {
#if !defined(LOGGING_OFF)
                ReportSessionLogStatus(*this);
#endif
                WaitForRunMode(has_async_workers);
                ReportRealtimeLag();

                // Run_mode may have changed during the delay - needs to be checked again

                if(run_mode.load() == run_mode_realtime ||
                   run_mode.load() == run_mode_play ||
                   run_mode.load() == run_mode_fast_forward)
                {
                    actual_tick_duration = intra_tick_timer.GetTime();
                    intra_tick_timer.Restart();
                    try
                    {
                        std::lock_guard<std::recursive_mutex> lock(kernelLock);
                        if(Tick() && socket != nullptr)
                            BuildUISnapshot(true);
                    }
                    catch(const exception & e)
                    {
                        Notify(msg_fatal_error, "During kernel tick " + std::to_string(tick) + ": " + e.message(),
                               e.path());
                        break;
                    }
                    catch(const std::exception & e)
                    {
                        Notify(msg_fatal_error, "During kernel tick " + std::to_string(tick) + ": " + e.what());
                        break;
                    }
                    catch(...)
                    {
                        Notify(msg_fatal_error, "Unknown error during kernel execution.");
                        break;
                    }
                    if(run_mode.load() == run_mode_fast_forward)
                        std::this_thread::yield();
                    tick_time_usage = intra_tick_timer.GetTime();
                    idle_time = std::max(0.0, tick_duration - tick_time_usage);
                }    
            }
            if(run_mode.load() == run_mode_restart)
                break;

            Stop();
            stop_completed = true;
            if(!options_.is_set("batch_mode"))
                Sleep(0.1);

        }
        return stop_completed;
    }

        bool
        Kernel::Notify(int msg, std::string message, std::string path)
        {
            const std::string timestamped_message = "[" + TimeString(GetTime()) + "] " + message;
            {
                std::lock_guard<std::mutex> lock(log_mutex);
                const size_t max_retained_webui_log_messages = MaxRetainedWebUILogMessages();
                while(log.size() >= max_retained_webui_log_messages)
                {
                    log.erase(log.begin());
                    ++first_webui_log_sequence;
                }
                log.push_back(Message(msg, timestamped_message, path));
                ++next_webui_log_sequence;
            }

        std::cout << timestamped_message;
        if(!path.empty())
            std::cout  << " ("<<path << ")";
        std::cout << '\n';

        if(msg == msg_fatal_error || msg == msg_terminate)
        {
            if(options_.is_set("batch_mode"))
            {
                process_exit_code = (msg == msg_fatal_error) ? 1 : 0;
                global_terminate = true;
            }
            else
            {
                notify_stop_requested = true;
            }
        }
        else if(msg <= msg_end_of_file)
        {
            global_terminate = true;
        }
        return true;
        }

    void
    Kernel::InitCircularBuffers()
    {
        for(const auto & [buffer_name, delay] : max_delays)
        {
            if(delay < 1)
                continue;
            auto source_buffer = buffers.find(buffer_name);
            if(source_buffer == buffers.end())
                continue;

            try
            {
                circular_buffers.try_emplace(buffer_name,
                                             source_buffer->second,
                                             delay,
                                             ComponentForValuePath(buffer_name));
            }
            catch(const out_of_memory_matrix_error &)
            {
                throw setup_failed("Could not allocate " + std::to_string(delay) +
                                   " ticks of delay history for \"" + buffer_name + "\".",
                                   buffer_name);
            }
            catch(const std::bad_alloc &)
            {
                throw setup_failed("Could not allocate " + std::to_string(delay) +
                                   " ticks of delay history for \"" + buffer_name + "\".", buffer_name);
            }
            catch(const std::length_error &)
            {
                throw setup_failed("Delay history for \"" + buffer_name + "\" is too large.", buffer_name);
            }
        }
    }


    void
    Kernel::RotateBuffers()
    {
        for(auto & [name, history] : circular_buffers)
        {
            tick_count completed_tick = -1;
            bool record_async_completion = false;

            if(history.source_component != nullptr && history.source_component->async_mode)
            {
                if(history.source_component->IsAsyncRunning() ||
                   history.source_component->IsAsyncFailed())
                    continue;

                completed_tick = history.source_component->async_completed_tick.load();
                if(completed_tick < 0 || completed_tick == history.last_async_completion)
                    continue;
                record_async_completion = true;
            }

            try
            {
                history.buffer.rotate(*history.source_buffer);
            }
            catch(const std::exception & e)
            {
                throw fatal_runtime_error("Error updating delay history for \"" +
                                          name + "\": " + e.what(), name);
            }
            catch(...)
            {
                throw fatal_runtime_error("Unknown error updating delay history for \"" +
                                          name + "\".", name);
            }

            if(record_async_completion)
                history.last_async_completion = completed_tick;
        }
    }



    void
    Kernel::Stop()
    {
        notify_stop_requested = false;

        {
            std::lock_guard<std::recursive_mutex> lock(kernelLock);
            run_mode.store(std::min(run_mode_stop, run_mode.load()));
            timer.Pause();
            timer.SetPauseTime(0);
        }

        WaitForAsyncComponents(true);

        {
            std::lock_guard<std::recursive_mutex> lock(kernelLock);
            if(options_.is_explicitly_set("save_state") && !components.empty())
                SaveState(ResolveStateFilename("save_state"));
            tick = 0;
#if !defined(LOGGING_OFF)
            if(session_logging_active)
            {
                LogStop();
                session_logging_active = false;
            }
#endif
            if(!StopComponents())
            {
                int successful_exit = 0;
                process_exit_code.compare_exchange_strong(successful_exit, 1);
            }
            needs_reload = true;
        }
    }



    void
    Kernel::Pause()
    {
        if(needs_reload)
        {
            if(GetOptionFilename().empty())
                New();
            else
                LoadFile();
            run_mode = run_mode_pause;
        }
        else
        {
            run_mode = run_mode_pause;
            timer.Pause();
            timer.SetPauseTime(GetTime()+tick_duration);
            WaitForAsyncComponents(false);
        }
    }


    void
    Kernel::Realtime()
    {
        if(needs_reload)
        {
            if(GetOptionFilename().empty())
                New();
            else
                LoadFile();
        }

        Pause();
#if !defined(LOGGING_OFF)
        if(!session_logging_active)
        {
            session_timer.Restart();
            LogStart();
            session_logging_active = true;
        }
#endif
        if(tick > 0)
            timer.SetPauseTime(static_cast<double>(tick) * tick_duration);
        timer.Continue();
        run_mode = run_mode_realtime;
    }



    void
    Kernel::Play()
    {
        if(needs_reload)
        {
            if(GetOptionFilename().empty())
                New();
            else
                LoadFile();
        }

#if !defined(LOGGING_OFF)
        if(!session_logging_active)
        {
            session_timer.Restart();
            LogStart();
            session_logging_active = true;
        }
#endif
        run_mode = run_mode_play;
        timer.Continue();
    }


    void
    Kernel::FastForward()
    {
        if(needs_reload)
        {
            if(GetOptionFilename().empty())
                New();
            else
                LoadFile();
        }

#if !defined(LOGGING_OFF)
        if(!session_logging_active)
        {
            session_timer.Restart();
            LogStart();
            session_logging_active = true;
        }
#endif
        run_mode = run_mode_fast_forward;
        timer.Continue();
    }
}; // namespace ikaros
