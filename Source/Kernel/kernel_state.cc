// Ikaros 3.0

#include "ikaros.h"

#include <ctime>
#include <fstream>

using namespace ikaros;
using namespace std::chrono;

namespace ikaros
{
    namespace
    {
        bool path_is_in_scope(const std::string & path, const std::string & scope)
        {
            if(scope.empty())
                return true;
            return path == scope || (path.size() > scope.size() &&
                   path.compare(0, scope.size(), scope) == 0 && path[scope.size()] == '.');
        }

        std::string remap_scoped_state_path(const std::string & saved_path,
                                            const std::string & saved_scope,
                                            const std::string & target_scope)
        {
            if(target_scope.empty())
                return saved_path;

            if(!saved_scope.empty() && saved_scope != "network")
            {
                if(!path_is_in_scope(saved_path, saved_scope))
                    return "";
                return target_scope + saved_path.substr(saved_scope.size());
            }

            return path_is_in_scope(saved_path, target_scope) ? saved_path : "";
        }

        std::string current_utc_timestamp()
        {
            auto now = system_clock::now();
            std::time_t now_time = system_clock::to_time_t(now);
            std::tm utc {};
            gmtime_r(&now_time, &utc);
            auto padded = [](int value, int width)
            {
                std::string result = std::to_string(value);
                if(result.size() < static_cast<std::size_t>(width))
                    result.insert(0, static_cast<std::size_t>(width) - result.size(), '0');
                return result;
            };
            return padded(utc.tm_year + 1900, 4) + "-" + padded(utc.tm_mon + 1, 2) + "-" +
                   padded(utc.tm_mday, 2) + "T" + padded(utc.tm_hour, 2) + ":" +
                   padded(utc.tm_min, 2) + ":" + padded(utc.tm_sec, 2) + "Z";
        }

        constexpr const char * ikaros_version = "3.0";

        std::string format_shape(const std::vector<int> & shape)
        {
            std::string result = "{";
            std::string separator;
            for(int dimension : shape)
            {
                result += separator + std::to_string(dimension);
                separator = ", ";
            }
            result += "}";
            return result;
        }
    }


    dictionary
    Kernel::CaptureState(const std::string & component_path) const
    {
        const std::string scope = trim(component_path);
        if(!scope.empty() && components.find(scope) == components.end())
            throw exception("Component \"" + scope + "\" could not be found.");

        dictionary captured;
        captured["format"] = "ikaros-state-v1";
        captured["tick"] = static_cast<double>(tick);
        captured["saved_at_utc"] = current_utc_timestamp();
        captured["ikaros_version"] = ikaros_version;
        captured["model_filename"] = options_.filename();
        captured["model_name"] = std::string(info_["name"]);
        captured["scope"] = scope.empty() ? "network" : scope;

        dictionary items;
        auto capture_matrix = [&](const std::string & path, const std::string & kind)
        {
            if(!path_is_in_scope(path, scope))
                return;
            auto buffer = buffers.find(path);
            if(buffer == buffers.end())
                throw exception("Persistent " + kind + " \"" + path + "\" does not exist.");
            if(buffer->second.is_uninitialized())
                throw exception("Persistent " + kind + " \"" + path + "\" has no allocated value.");

            dictionary item;
            item["kind"] = kind;
            item["type"] = "matrix";
            list shape;
            for(int dimension : buffer->second.shape())
                shape.push_back(value(static_cast<double>(dimension)));
            item["shape"] = std::move(shape);
            item["value"] = parse_json(buffer->second.json());
            items[path] = std::move(item);
        };

        for(const auto & path : persistent_outputs)
            capture_matrix(path, "output");
        for(const auto & path : persistent_state_buffers)
            capture_matrix(path, "state");

        for(const auto & [path, state] : scalar_states)
            if(state.persistent && path_is_in_scope(path, scope))
            {
                dictionary item;
                item["kind"] = "state";
                item["type"] = state.type;
                if(state.type == "float")
                    item["value"] = static_cast<double>(state.float_ptr ? *state.float_ptr : state.float_value);
                else if(state.type == "double")
                    item["value"] = state.double_ptr ? *state.double_ptr : state.double_value;
                else if(state.type == "int")
                    item["value"] = state.int_ptr ? *state.int_ptr : state.int_value;
                else if(state.type == "bool")
                    item["value"] = state.bool_ptr ? *state.bool_ptr : state.bool_value;
                else if(state.type == "string")
                    item["value"] = state.string_ptr ? *state.string_ptr : state.string_value;
                else
                    throw exception("Unsupported scalar state type \"" + state.type + "\".");
                items[path] = std::move(item);
            }

        captured["item_count"] = static_cast<double>(items.dict_->size());
        captured["items"] = std::move(items);
        return captured;
    }


    void
    Kernel::SaveState(const std::string & filename, const std::string & component_path)
    {
        if(filename.empty())
            throw exception("State filename is empty.");

        dictionary state = CaptureState(component_path);
        std::ofstream file(filename);
        if(!file)
            throw exception("Could not open state file \"" + filename + "\" for writing.");

        file << state.json() << '\n';

        if(!file)
            throw exception("Could not write state file \"" + filename + "\".");

        const std::string scope = trim(component_path);
        Notify(msg_print, "Saved state to " + filename + (scope.empty() ? "" : " for " + scope));
    }


    void
    Kernel::LoadState(const std::string & filename, const std::string & component_path)
    {
        if(filename.empty())
            throw exception("State filename is empty.");

        dictionary state;
        try
        {
            state.load_json(filename);
        }
        catch(const std::exception & e)
        {
            throw exception("Could not load state file \"" + filename + "\": " + e.what());
        }

        RestoreState(state, component_path, filename);
        const std::string target_scope = trim(component_path);
        Notify(msg_print, "Loaded state from " + filename +
                          (target_scope.empty() ? "" : " into " + target_scope));
    }


    void
    Kernel::RestoreState(const dictionary & state, const std::string & component_path,
                         const std::string & source_name)
    {
        const std::string target_scope = trim(component_path);
        if(!target_scope.empty() && components.find(target_scope) == components.end())
            throw exception("Component \"" + target_scope + "\" could not be found.");

        if(std::string(state["format"]) != "ikaros-state-v1")
            throw exception("State file \"" + source_name + "\" has unsupported format \"" +
                            std::string(state["format"]) + "\".");
        if(!state["items"].is_dictionary())
            throw exception("State file \"" + source_name + "\" does not contain an items object.");

        std::string saved_scope = state["scope"].is_string() ? state["scope"].as_string() : "";
        dictionary items = std::get<dictionary>(state["items"].value_);
        for(const auto & [path, saved_value] : *items.dict_)
        {
            std::string target_path = remap_scoped_state_path(path, saved_scope, target_scope);
            if(target_path.empty())
                continue;

            if(!saved_value.is_dictionary())
                throw exception("State item \"" + path + "\" is not an object.");

            dictionary item = std::get<dictionary>(saved_value.value_);
            std::string kind = item["kind"];
            std::string type = item["type"];
            if(kind != "output" && kind != "state")
                throw exception("State item \"" + path + "\" has unsupported kind \"" + kind + "\".");
            if(kind == "output" && type != "matrix")
                throw exception("State item \"" + path + "\" is an output but does not have type matrix.");
            if(kind == "output" && !persistent_outputs.count(target_path))
                throw exception("State item \"" + path + "\" does not match a persistent output in the loaded model.");

            if(kind == "state" && type != "matrix")
            {
                auto scalar = scalar_states.find(target_path);
                if(scalar == scalar_states.end() || !scalar->second.persistent)
                    throw exception("State item \"" + path + "\" does not match a persistent private state in the loaded model.");
                if(scalar->second.type != type)
                    throw exception("State item \"" + path + "\" has type \"" + type + "\" but target state has type \"" + scalar->second.type + "\".");

                try
                {
                    if(type == "float")
                    {
                        double value = item["value"].as_double();
                        scalar->second.float_value = static_cast<float>(value);
                        if(scalar->second.float_ptr)
                            *scalar->second.float_ptr = static_cast<float>(value);
                    }
                    else if(type == "double")
                    {
                        double value = item["value"].as_double();
                        scalar->second.double_value = value;
                        if(scalar->second.double_ptr)
                            *scalar->second.double_ptr = value;
                    }
                    else if(type == "int")
                    {
                        int value = item["value"].as_int();
                        scalar->second.int_value = value;
                        if(scalar->second.int_ptr)
                            *scalar->second.int_ptr = value;
                    }
                    else if(type == "bool")
                    {
                        if(!item["value"].is_bool())
                            throw exception("Expected Boolean value.");
                        bool value = item["value"].is_true();
                        scalar->second.bool_value = value;
                        if(scalar->second.bool_ptr)
                            *scalar->second.bool_ptr = value;
                    }
                    else if(type == "string")
                    {
                        if(!item["value"].is_string())
                            throw exception("Expected string value.");
                        std::string value = item["value"].as_string();
                        scalar->second.string_value = value;
                        if(scalar->second.string_ptr)
                            *scalar->second.string_ptr = value;
                    }
                    else
                        throw exception("Unsupported scalar state type \"" + type + "\".");
                }
                catch(const exception &)
                {
                    throw;
                }
                catch(const std::exception & e)
                {
                    throw exception("State item \"" + path + "\" has invalid value: " + std::string(e.what()));
                }
                continue;
            }

            if(kind == "state" && !persistent_state_buffers.count(target_path))
                throw exception("State item \"" + path + "\" does not match a persistent private state in the loaded model.");

            auto target = buffers.find(target_path);
            if(target == buffers.end())
                throw exception("State item \"" + path + "\" does not match a value in the loaded model.");

            matrix restored(item["value"].json());
            if(restored.shape() != target->second.shape())
                throw exception("State item \"" + path + "\" has shape " + format_shape(restored.shape()) + " but target " + kind + " has shape " + format_shape(target->second.shape()) + ".");

            target->second.copy(restored);
        }

    }


    void
    Kernel::ResetState(const std::string & component_path)
    {
        if(component_path.empty())
        {
            for(auto & [path, component] : components)
                component->Reset();
            Notify(msg_print, "Reset state");
            return;
        }

        std::string path = component_path;
        if(!path.empty() && path[0] == '.')
            path = path.substr(1);

        if(components.find(path) == components.end())
            throw exception("Component \"" + component_path + "\" could not be found.");

        for(auto & [candidate_path, component] : components)
            if(path_is_in_scope(candidate_path, path))
                component->Reset();
        Notify(msg_print, "Reset state for " + path);
    }
}; // namespace ikaros
