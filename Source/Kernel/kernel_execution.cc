// Ikaros 3.0

#include "ikaros.h"
#include "session_logging.h"

#include <cmath>

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
        if(auto failure = RunTasks())
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
        for(auto & c : connections)
            if(c.HasZeroDelay())
                continue;
            else if(!c.ShouldTick())
                continue;
            else
                try
                {
                    c.Tick();
                }
                catch(const std::exception & e)
                {
                    throw std::runtime_error("Error propagating connection \"" +
                                             c.Info() + "\": " + e.what());
                }
                catch(...)
                {
                    throw std::runtime_error("Unknown error propagating connection \"" +
                                             c.Info() + "\".");
                }
    }



    void
    Kernel::InitSocket(long port)
    {
        try
        {
            if(port < 0 || port > 65535)
                throw std::invalid_argument("Server port must be between 0 and 65535");
            shutdown.store(false, std::memory_order_release);
            socket = std::make_unique<ServerSocket>(static_cast<int>(port), GetOption("bind_address"));
        }
        catch (const exception& e)
        {
            throw socket_startup_error("Ikaros is unable to start the webserver on port " + std::to_string(port) + ": " + e.message(), e.path());
        }
        catch (const std::exception& e)
        {
            throw socket_startup_error("Ikaros is unable to start the webserver on port " + std::to_string(port) + ": " + std::string(e.what()));
        }

        httpThread = std::thread(Kernel::StartHTTPThread, this);
    }


    void
    Kernel::StopHTTPServer()
    {
        shutdown.store(true, std::memory_order_release);

        if(socket != nullptr)
            socket->StopListening();

        if(httpThread.joinable())
        {
            httpThread.join();
        }

        socket.reset();
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


/*************************
 * 
 *  Task sorting
 * 
 *************************/

    bool
    Kernel::HasCycle(const std::vector<std::string> & nodes, const std::vector<std::pair<std::string, std::string>> & edges)
    {
        std::unordered_map<std::string, std::vector<std::string>> graph;
        for(const auto & edge : edges)
            graph[edge.first].push_back(edge.second);

        enum class VisitState
        {
            unvisited,
            visiting,
            visited,
        };

        struct TraversalFrame
        {
            std::string node;
            size_t next_neighbor = 0;
        };

        std::unordered_map<std::string, VisitState> states;
        std::vector<TraversalFrame> traversal;

        for(const std::string & node : nodes)
        {
            if(states[node] != VisitState::unvisited)
                continue;

            states[node] = VisitState::visiting;
            traversal.push_back({node, 0});
            while(!traversal.empty())
            {
                TraversalFrame & frame = traversal.back();
                auto neighbors = graph.find(frame.node);
                if(neighbors == graph.end() || frame.next_neighbor >= neighbors->second.size())
                {
                    states[frame.node] = VisitState::visited;
                    traversal.pop_back();
                    continue;
                }

                const std::string & neighbor = neighbors->second[frame.next_neighbor++];
                if(states[neighbor] == VisitState::visiting)
                    return true;
                if(states[neighbor] == VisitState::unvisited)
                {
                    states[neighbor] = VisitState::visiting;
                    traversal.push_back({neighbor, 0});
                }
            }
        }

        return false;
    }


    std::vector<std::vector<std::string>>
    Kernel::FindSubgraphs(const std::vector<std::string> & nodes, const std::vector<std::pair<std::string, std::string>> & edges)
    {
        std::unordered_map<std::string, std::vector<std::string>> graph;
        for(const auto & edge : edges)
        {
            graph[edge.first].push_back(edge.second);
            graph[edge.second].push_back(edge.first);
        }

        struct TraversalFrame
        {
            std::string node;
            size_t next_neighbor = 0;
        };

        std::unordered_set<std::string> visited;
        std::vector<std::vector<std::string>> components;

        for(const std::string & node : nodes)
        {
            if(!visited.insert(node).second)
                continue;

            std::vector<std::string> component{node};
            std::vector<TraversalFrame> traversal{{node, 0}};
            while(!traversal.empty())
            {
                TraversalFrame & frame = traversal.back();
                auto neighbors = graph.find(frame.node);
                if(neighbors == graph.end() || frame.next_neighbor >= neighbors->second.size())
                {
                    traversal.pop_back();
                    continue;
                }

                const std::string & neighbor = neighbors->second[frame.next_neighbor++];
                if(visited.insert(neighbor).second)
                {
                    component.push_back(neighbor);
                    traversal.push_back({neighbor, 0});
                }
            }
            components.push_back(std::move(component));
        }

        return components;
    }


    std::vector<std::string>
    Kernel::TopologicalSort(const std::vector<std::string> & component, const std::unordered_map<std::string, std::vector<std::string>> & graph)
    {
        struct TraversalFrame
        {
            std::string node;
            size_t next_neighbor = 0;
        };

        std::unordered_set<std::string> visited;
        std::vector<std::string> finished;

        for(const std::string & node : component)
        {
            if(!visited.insert(node).second)
                continue;

            std::vector<TraversalFrame> traversal{{node, 0}};
            while(!traversal.empty())
            {
                TraversalFrame & frame = traversal.back();
                auto neighbors = graph.find(frame.node);
                if(neighbors == graph.end() || frame.next_neighbor >= neighbors->second.size())
                {
                    finished.push_back(frame.node);
                    traversal.pop_back();
                    continue;
                }

                const std::string & neighbor = neighbors->second[frame.next_neighbor++];
                if(visited.insert(neighbor).second)
                    traversal.push_back({neighbor, 0});
            }
        }

        std::vector<std::string> sorted_subgraph;
        sorted_subgraph.reserve(finished.size());
        for(auto node = finished.rbegin(); node != finished.rend(); ++node)
            sorted_subgraph.push_back(*node);
        return sorted_subgraph;
    }


    std::vector<std::vector<std::string>>
    Kernel::Sort(const std::vector<std::string> & nodes, const std::vector<std::pair<std::string, std::string>> & edges)
    {
        if(HasCycle(nodes, edges))
            throw setup_failed("Network has zero-delay loops");

        std::vector<std::vector<std::string>> components = FindSubgraphs(nodes, edges);

        std::unordered_map<std::string, std::vector<std::string>> graph;
        for(const auto & edge : edges)
            graph[edge.first].push_back(edge.second);

        std::vector<std::vector<std::string>> result;
        result.reserve(components.size());
        for(const auto & component : components)
            result.push_back(TopologicalSort(component, graph));
        return result;
    }



    void
    Kernel::SortTasks()
    {
        std::vector<std::string> nodes;
        std::vector<std::pair<std::string, std::string>> arcs;
        std::map<std::string, Task *> task_map;

        for(auto & [s,c] : components)
        {
            nodes.push_back(s);
            task_map[s] = c.get(); // Save in task map
        }

        for(size_t connection_index = 0; connection_index < connections.size(); ++connection_index)
        {
            auto & c = connections[connection_index];
            if(!c.HasZeroDelay())
                continue;

            std::string s = peek_rhead(c.source,".");
            std::string t = peek_rhead(c.target,".");
            std::string cc = "CON(" + std::to_string(connection_index) + ")";

            nodes.push_back(cc);
            arcs.push_back({s, cc});
            arcs.push_back({cc, t});
            task_map[cc] = &c; // Save in task map
        }

        auto r = Sort(nodes, arcs);

        // Fill task list

        tasks.clear();
        for(auto s : r)
        {
            std::vector<Task *> task_list;
            bool priority_task = false;
            for(auto ss: s)
            {
                if(task_map[ss]->Priority())
                    priority_task = true;
                task_list.push_back(task_map[ss]); // Get task pointer here
            }
            if(priority_task)
                tasks.insert(tasks.begin(), task_list);
            else
                tasks.push_back(task_list);

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
    Kernel::RunTask(Task * task)
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

        const bool profiling_started = task->TryProfilingBegin();
        try
        {
            task->Tick();
        }
        catch(const exception & e)
        {
            if(profiling_started)
                task->ProfilingEnd();
            throw exception("While running task \"" + task->Info() + "\": " + e.message(),
                            e.path().empty() ? task->Info() : e.path());
        }
        catch(const std::exception & e)
        {
            if(profiling_started)
                task->ProfilingEnd();
            throw exception("While running task \"" + task->Info() + "\": " + e.what(), task->Info());
        }
        catch(...)
        {
            if(profiling_started)
                task->ProfilingEnd();
            throw exception("While running task \"" + task->Info() + "\": Unknown error.", task->Info());
        }
        if(profiling_started)
            task->ProfilingEnd();
    }


    void
    Kernel::PollAsyncComponents()
    {
        for(auto & [path, component] : components)
        {
            if(!component->async_mode)
                continue;

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
        KernelTaskSequence(Kernel & kernel, const std::vector<Task *> & tasks):
            TaskSequence(tasks),
            kernel_(kernel)
        {
        }

    protected:
        void Tick() override
        {
            for(auto & task : tasks_)
            {
                try
                {
                    kernel_.RunTask(task);
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
    Kernel::SubmitTaskSequences(submitted_task_sequences & sequences)
    {
        sequences.reserve(tasks.size());
        std::optional<exception> failure;
        try
        {
            for(auto & task_sequence : tasks)
            {
                auto sequence = std::make_shared<KernelTaskSequence>(*this, task_sequence);
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
    Kernel::RunTasks()
    {
        submitted_task_sequences sequences;
        std::optional<exception> failure = SubmitTaskSequences(sequences);
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
        for(auto & task_group : tasks)
            for(auto & task: task_group)
                RunTask(task);
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
        bool has_async_workers = false;
        if(options_.is_set("batch_mode"))
            for(auto & [name, parameter] : parameters)
                if(ends_with(name, ".async") && parameter.as_bool())
                {
                    has_async_workers = true;
                    break;
                }

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

                if(run_mode.load() == run_mode_realtime || run_mode.load() == run_mode_play) 
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
}; // namespace ikaros
