// Ikaros 3.0

#include "ikaros.h"

#include <ctime>
#include <fstream>

using namespace ikaros;
using namespace std::chrono;

namespace ikaros
{
    value
    Kernel::ScalarState::CurrentValue() const
    {
        if(type == "float")
            return static_cast<double>(float_ptr ? *float_ptr : float_value);
        if(type == "double")
            return double_ptr ? *double_ptr : double_value;
        if(type == "int")
            return static_cast<double>(int_ptr ? *int_ptr : int_value);
        if(type == "bool")
            return bool_ptr ? *bool_ptr : bool_value;
        if(type == "string")
            return string_ptr ? *string_ptr : string_value;
        throw exception("Unsupported scalar state type \"" + type + "\".");
    }


    void
    Kernel::ScalarState::RestoreValue(const value & saved_value)
    {
        if(type == "float")
        {
            float_value = saved_value.as_float();
            if(float_ptr)
                *float_ptr = float_value;
            return;
        }
        if(type == "double")
        {
            double_value = saved_value.as_double();
            if(double_ptr)
                *double_ptr = double_value;
            return;
        }
        if(type == "int")
        {
            int_value = saved_value.as_int();
            if(int_ptr)
                *int_ptr = int_value;
            return;
        }
        if(type == "bool")
        {
            if(!saved_value.is_bool())
                throw exception("Expected Boolean value.");
            bool_value = saved_value.is_true();
            if(bool_ptr)
                *bool_ptr = bool_value;
            return;
        }
        if(type == "string")
        {
            if(!saved_value.is_string())
                throw exception("Expected string value.");
            string_value = saved_value.as_string();
            if(string_ptr)
                *string_ptr = string_value;
            return;
        }
        throw exception("Unsupported scalar state type \"" + type + "\".");
    }


    void
    Kernel::ScalarState::Reset()
    {
        if(type == "float")
        {
            float_value = default_float_value;
            if(float_ptr)
                *float_ptr = float_value;
            return;
        }
        if(type == "double")
        {
            double_value = default_double_value;
            if(double_ptr)
                *double_ptr = double_value;
            return;
        }
        if(type == "int")
        {
            int_value = default_int_value;
            if(int_ptr)
                *int_ptr = int_value;
            return;
        }
        if(type == "bool")
        {
            bool_value = default_bool_value;
            if(bool_ptr)
                *bool_ptr = bool_value;
            return;
        }
        if(type == "string")
        {
            string_value = default_string_value;
            if(string_ptr)
                *string_ptr = string_value;
            return;
        }
        throw exception("Unsupported scalar state type \"" + type + "\".");
    }


    std::string
    Kernel::ResolveStateFilename(const std::string & option_name) const
    {
        std::string filename = options_.get(option_name);
        if(!filename.empty() && filename != "true")
        {
            std::filesystem::path state_path(filename);
            if(state_path.is_relative())
                state_path = options_.invocation_directory / state_path;
            return state_path.lexically_normal().string();
        }

        std::filesystem::path model_path = options_.full_path();
        if(model_path.empty())
            throw exception("Can not derive state filename because no model file is loaded.");

        model_path.replace_extension(".state");
        return model_path.string();
    }


    std::string
    Kernel::ResolveStateFilenameFromRequest(const Request & request,
                                            const std::string & option_name) const
    {
        std::string requested_filename;
        if(request.parameters.contains("filename"))
            requested_filename = std::string(request.parameters["filename"]);
        else if(request.parameters.contains("file"))
            requested_filename = std::string(request.parameters["file"]);

        requested_filename = trim(requested_filename);
        if(requested_filename.empty())
            return ResolveStateFilename(option_name);

        std::filesystem::path path = add_extension(requested_filename, ".state");
        std::filesystem::path filename = path.filename();
        if(filename.empty() || filename == "." || filename == ".." || filename.stem().empty())
            throw exception("State filename is invalid.");

        return (std::filesystem::path(user_dir) / filename).string();
    }


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
                item["value"] = state.CurrentValue();
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
                    scalar->second.RestoreValue(item["value"]);
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
                throw exception("State item \"" + path + "\" has shape " + matrix::format_shape(restored.shape()) + " but target " + kind + " has shape " + matrix::format_shape(target->second.shape()) + ".");

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
