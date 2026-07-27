// Ikaros 3.0

#include "ikaros.h"

#include <chrono>
#include <iostream>
#include <map>
#include <sstream>
#include <vector>

using namespace std::chrono;

namespace ikaros
{
    namespace
    {
        constexpr double profiling_subscription_timeout_seconds = 3.0;
    }

    void
    Kernel::ListComponents()
    {
        std::cout << "\nComponents:\n";
        for(auto & [name, component] : components)
        {
            (void)name;
            component->print();
        }
    }


    void
    Kernel::ListConnections()
    {
        std::cout << "\nConnections:\n";
        for(auto & c : connections)
            c.Print();
    }


    void
    Kernel::ListBuffers()
    {
        std::cout << "\nBuffers:\n";
        for(auto & [name, buffer] : buffers)
            std::cout << "\t" << name <<  buffer.shape() << '\n';
    }


    void
    Kernel::ListCircularBuffers()
    {
        if(circular_buffers.empty())
            return;

        std::cout << "\nCircularBuffers:\n";
        for(auto & [name, history] : circular_buffers)
        {
            const matrix & latest = history.buffer.get(1);
            std::cout << "\t" << name << " " << history.buffer.size() << " "
                      << latest.rank() << latest.shape() << '\n';
        }
    }


    void
    Kernel::ListTasks()
    {
        for(auto & task_group : tasks)
        {
            std::cout << "\nTasks:\n";
            for(auto & task: task_group)
                std::cout << "\t" << task->Info() << '\n';
        }
    }

   void
   Kernel::ListParameters()
    {
        std::cout << "\nParameters:\n";
        for(auto & [name, parameter] : parameters)
            std::cout  << "\t" << name << ": " << parameter << '\n';
    }


    void
    Kernel::PrintLog()
    {
        std::lock_guard<std::mutex> lock(log_mutex);
        for(auto & s : log)
            std::cout << "ikaros: " << s.level_ << ": " << s.message_ << '\n';
        log.clear();
        first_webui_log_sequence = next_webui_log_sequence;
    }


    bool
    Kernel::ProfilingEnabled() const
    {
        return profiling_enabled.load(std::memory_order_relaxed);
    }

    void
        Kernel::PrintProfiling()
        {
            for(auto & [name, component] : components)
            {
                (void)name;
                component->profiler_.print(component->path_);
            }
        }


    dictionary 
    Kernel::GetModuleInstantiationInfo()
    {
        dictionary d;
        std::map<std::string, int> class_counts;
        int module_count = 0;

        for(const auto & [path, component] : components)
        {
            (void)path;
            if(component == nullptr)
                continue;
            if(dynamic_cast<Group *>(component.get()) != nullptr)
                continue;

            std::string class_name = component->info_.contains_non_null("class") ? std::string(component->info_["class"]) : "";
            if(class_name.empty())
                class_name = component->Info();
            if(class_name.empty())
                class_name = "unknown";

            class_counts[class_name]++;
            module_count++;
        }

        std::vector<std::string> summary_entries;
        summary_entries.reserve(class_counts.size());
        for(const auto & [class_name, count] : class_counts)
            summary_entries.push_back(class_name + ":" + std::to_string(count));

        d["module_count"] = module_count;
        d["class_count"] = static_cast<int>(class_counts.size());
        d["classes"] = join(",", summary_entries);
        return d;
     }


    std::string
    Kernel::GetProfilingJSON() const
    {
        std::ostringstream body;
        body << "{";
        body << "\"tick\": " << tick << ", ";
        body << "\"run_mode\": " << run_mode.load() << ", ";
        body << "\"enabled\": "
             << (profiling_enabled.load(std::memory_order_relaxed) ? "true" : "false") << ", ";
        body << "\"components\": [";

        std::string separator;
        for(const auto & [path, component] : components)
        {
            if(component == nullptr)
                continue;

            body << separator;
            body << "{";
            body << "\"path\": \"" << escape_json_string(path) << "\", ";
            body << "\"name\": \"" << escape_json_string(component->Info()) << "\", ";

            std::string class_name;
            if(component->info_.contains_non_null("class"))
                class_name = std::string(component->info_["class"]);

            body << "\"class\": ";
            if(class_name.empty())
                body << "null";
            else
                body << "\"" << escape_json_string(class_name) << "\"";
            body << ", ";
            body << "\"profiling\": " << component->profiler_.json();
            body << "}";
            separator = ", ";
        }

        body << "]";
        body << "}";
        return body.str();
    }


    void
    Kernel::SetProfilingClientActive(long client_id, bool active)
    {
        const auto now = steady_clock::now();
        std::lock_guard<std::mutex> lock(profiling_clients_mutex);

        for(auto it = profiling_clients.begin(); it != profiling_clients.end();)
        {
            if(now - it->second > duration<double>(profiling_subscription_timeout_seconds))
                it = profiling_clients.erase(it);
            else
                ++it;
        }

        if(active)
            profiling_clients[client_id] = now;
        else
            profiling_clients.erase(client_id);

        profiling_enabled.store(!profiling_clients.empty(), std::memory_order_relaxed);
    }


    void
    Kernel::UpdateProfilingState()
    {
        if(!profiling_enabled.load(std::memory_order_relaxed))
            return;

        const auto now = steady_clock::now();
        std::lock_guard<std::mutex> lock(profiling_clients_mutex);
        for(auto it = profiling_clients.begin(); it != profiling_clients.end();)
        {
            if(now - it->second > duration<double>(profiling_subscription_timeout_seconds))
                it = profiling_clients.erase(it);
            else
                ++it;
        }
        profiling_enabled.store(!profiling_clients.empty(), std::memory_order_relaxed);
    }


}
