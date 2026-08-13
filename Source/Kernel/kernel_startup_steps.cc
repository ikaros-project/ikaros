// Ikaros 3.0

#include "ikaros.h"

#include <limits>
#include <sstream>

namespace ikaros
{
    namespace
    {
        constexpr int unresolved_startup_step = std::numeric_limits<int>::max();

        int
        ConnectionDelayMin(const Connection & connection)
        {
            return connection.MinDelay();
        }


        int
        ConnectionDelayMax(const Connection & connection)
        {
            return connection.MaxDelay();
        }

    }


    std::map<std::string, int>
    Kernel::PropagateStartupBufferSteps(input_map incoming_connections)
    {
        std::map<std::string, int> buffer_first_real_step;

        for(auto & [buffer_name, buffer] : buffers)
        {
            (void)buffer;
            buffer_first_real_step[buffer_name] = unresolved_startup_step;
        }

        for(auto & [path, component] : components)
        {
            if(dynamic_cast<Module *>(component.get()) == nullptr)
                continue;

            bool has_connected_input = false;
            if(component->info_.contains("inputs") && component->info_["inputs"].is_list())
                for(auto & input : component->info_["inputs"])
                {
                    std::string input_name = path + "." + std::string(input["name"]);
                    if(incoming_connections.count(input_name))
                    {
                        has_connected_input = true;
                        break;
                    }
                }

            if(has_connected_input)
                continue;

            if(component->info_.contains("outputs") && component->info_["outputs"].is_list())
                for(auto & output : component->info_["outputs"])
                {
                    std::string output_name = path + "." + std::string(output["name"]);
                    buffer_first_real_step[output_name] = 0;
                }
        }

        size_t max_iterations = std::max<size_t>(1, buffers.size() + components.size() + connections.size());
        for(size_t iteration = 0; iteration < max_iterations; ++iteration)
        {
            bool changed = false;

            for(auto & [path, component] : components)
            {
                auto * module = dynamic_cast<Module *>(component.get());
                if(module == nullptr)
                    continue;

                int first_input_step = unresolved_startup_step;
                bool has_connected_input = false;

                if(component->info_.contains("inputs") && component->info_["inputs"].is_list())
                    for(auto & input : component->info_["inputs"])
                    {
                        std::string input_name = path + "." + std::string(input["name"]);
                        if(!incoming_connections.count(input_name))
                            continue;

                        has_connected_input = true;
                        first_input_step = std::min(first_input_step, buffer_first_real_step[input_name]);
                    }

                int first_output_step = has_connected_input ? first_input_step : 0;
                if(first_output_step == unresolved_startup_step)
                    continue;

                if(component->info_.contains("outputs") && component->info_["outputs"].is_list())
                    for(auto & output : component->info_["outputs"])
                    {
                        std::string output_name = path + "." + std::string(output["name"]);
                        if(first_output_step < buffer_first_real_step[output_name])
                        {
                            buffer_first_real_step[output_name] = first_output_step;
                            changed = true;
                        }
                    }
            }

            for(auto & connection : connections)
            {
                auto source_it = buffer_first_real_step.find(connection.source);
                if(source_it == buffer_first_real_step.end() || source_it->second == unresolved_startup_step)
                    continue;

                int candidate_step = source_it->second + ConnectionDelayMin(connection);
                int & target_step = buffer_first_real_step[connection.target];
                if(candidate_step < target_step)
                {
                    target_step = candidate_step;
                    changed = true;
                }
            }

            if(!changed)
                break;
        }

        return buffer_first_real_step;
    }


    void
    Kernel::ApplyStartupComponentSteps(
        input_map incoming_connections,
        const std::map<std::string, int> & buffer_first_real_step)
    {

        for(auto & [path, component] : components)
        {
            int first_real_input_step = unresolved_startup_step;
            int all_real_inputs_step = 0;
            bool has_connected_input = false;
            bool all_inputs_resolved = true;

            if(component->info_.contains("inputs") && component->info_["inputs"].is_list())
                for(auto & input : component->info_["inputs"])
                {
                    std::string input_name = path + "." + std::string(input["name"]);
                    if(!incoming_connections.count(input_name))
                        continue;

                    for(auto * connection : incoming_connections.at(input_name))
                    {
                        has_connected_input = true;

                        auto source_it = buffer_first_real_step.find(connection->source);
                        if(source_it == buffer_first_real_step.end() || source_it->second == unresolved_startup_step)
                        {
                            all_inputs_resolved = false;
                            continue;
                        }

                        int connection_first_step = source_it->second + ConnectionDelayMin(*connection);
                        int connection_all_step = source_it->second + ConnectionDelayMax(*connection);

                        first_real_input_step = std::min(first_real_input_step, connection_first_step);
                        all_real_inputs_step = std::max(all_real_inputs_step, connection_all_step);
                    }
                }

            if(!has_connected_input)
            {
                first_real_input_step = 0;
                all_real_inputs_step = 0;
            }
            else if(!all_inputs_resolved)
            {
                all_real_inputs_step = unresolved_startup_step;
            }

            component->startup_first_real_input_step = first_real_input_step;
            component->startup_all_real_inputs_step = all_real_inputs_step;
        }
    }


    void
    Kernel::CalculateStartupSteps()
    {
        connection_map incoming_connections = BuildIncomingConnections();
        std::map<std::string, int> buffer_first_real_step =
            PropagateStartupBufferSteps(incoming_connections);
        ApplyStartupComponentSteps(incoming_connections, buffer_first_real_step);
    }



    std::string
    Kernel::GetStartupStepsJSON() const
    {
        std::ostringstream body;
        body << "{";
        body << "\"tick\": " << tick << ", ";
        body << "\"run_mode\": " << run_mode.load() << ", ";
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
            body << "\"module_start\": " << component->module_start << ", ";
            body << "\"start_tick\": " << component->start_tick << ", ";
            body << "\"startup_first_real_input_step\": ";
            if(component->startup_first_real_input_step == std::numeric_limits<int>::max())
                body << "null";
            else
                body << component->startup_first_real_input_step;
            body << ", ";
            body << "\"startup_all_real_inputs_step\": ";
            if(component->startup_all_real_inputs_step == std::numeric_limits<int>::max())
                body << "null";
            else
                body << component->startup_all_real_inputs_step;
            body << "}";
            separator = ", ";
        }

        body << "]";
        body << "}";
        return body.str();
    }


}
