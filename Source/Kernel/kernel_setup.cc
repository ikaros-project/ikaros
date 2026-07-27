// Ikaros 3.0

#include "ikaros.h"
#include "kernel_parsing.h"

#include <charconv>
#include <cmath>

using namespace ikaros;
using namespace std::literals;

namespace ikaros
{
    namespace
    {
        constexpr int maximum_connection_delay = 100;
        constexpr size_t default_max_retained_webui_log_messages = 500;

        void
        ValidateConnectionDelayRange(const range & delays,
                                     const std::string & source,
                                     const std::string & target,
                                     const std::string & path)
        {
            const std::string connection = "Connection \"" + source + " => " + target + "\" delay range ";
            if(delays.rank() != 1)
                throw build_failed(connection + "must be one-dimensional.", path);
            const int delay_start = delays.start(0);
            const int delay_stop = delays.stop(0);
            const int delay_step = delays.step(0);
            if(delay_start == delay_stop)
                throw build_failed(connection + "must not be empty.", path);
            if(delay_step <= 0)
                throw build_failed(connection + "must have a positive increment.", path);
            if(delay_start < 0)
                throw build_failed(connection + "must be non-negative.", path);
            if(delay_stop <= delay_start)
                throw build_failed(connection + "must be an ascending, non-empty range.", path);

            const long long distance = static_cast<long long>(delay_stop) - delay_start;
            const long long count = 1 + (distance - 1) / delay_step;
            const long long max_delay = static_cast<long long>(delay_start) +
                                        (count - 1) * delay_step;
            if(max_delay > maximum_connection_delay)
                throw build_failed(connection + "must not exceed " +
                                   std::to_string(maximum_connection_delay) + " ticks.", path);
        }

        bool is_scalar_state_type(const std::string & type)
        {
            return type == "float" || type == "double" || type == "int" ||
                   type == "bool" || type == "string";
        }

        tick_count
        parse_stop_after(const std::string & value)
        {
            const std::string text = trim(value);
            tick_count result = 0;
            const char * begin = text.data();
            const char * end = begin + text.size();
            bool valid_sign = true;
            if(begin != end && *begin == '+')
            {
                ++begin;
                valid_sign = begin != end && *begin != '+' && *begin != '-';
            }
            const auto conversion = std::from_chars(begin, end, result);
            if(text.empty() || !valid_sign || conversion.ec != std::errc() ||
               conversion.ptr != end || result < -1)
                throw setup_failed("Invalid stop tick \"" + value +
                                   "\". Expected -1 or a non-negative integer.");
            return result;
        }

        double
        parse_tick_duration(const std::string & value)
        {
            double result = 0;
            if(!parse_double(value, result) || !std::isfinite(result) || result <= 0)
                throw setup_failed("Invalid tick duration \"" + value +
                                   "\". Expected a finite positive number of seconds.");
            return result;
        }

        dictionary make_color_parameter()
        {
            dictionary parameter;
            parameter["_tag"] = "parameter";
            parameter["name"] = "color";
            parameter["type"] = "string";
            parameter["default"] = "black";
            parameter["description"] = "Selected ui color";
            parameter["control"] = "ui_color";
            return parameter;
        }

        dictionary make_ui_snapshot_rgb_quality_parameter()
        {
            dictionary parameter;
            parameter["_tag"] = "parameter";
            parameter["name"] = "rgb_quality";
            parameter["type"] = "number";
            parameter["default"] = 75;
            parameter["description"] = "JPEG quality used for RGB images in WebUI update snapshots.";
            return parameter;
        }

        dictionary make_ui_snapshot_gray_quality_parameter()
        {
            dictionary parameter;
            parameter["_tag"] = "parameter";
            parameter["name"] = "gray_quality";
            parameter["type"] = "number";
            parameter["default"] = 70;
            parameter["description"] = "JPEG quality used for grayscale and pseudocolor images in WebUI update snapshots.";
            return parameter;
        }

        dictionary make_snapshot_interval_parameter()
        {
            dictionary parameter;
            parameter["_tag"] = "parameter";
            parameter["name"] = "snapshot_interval";
            parameter["type"] = "number";
            parameter["default"] = 0.1;
            parameter["description"] = "Minimum interval in seconds between image refreshes in WebUI update snapshots.";
            return parameter;
        }

        dictionary make_webui_request_interval_parameter()
        {
            dictionary parameter;
            parameter["_tag"] = "parameter";
            parameter["name"] = "webui_req_int";
            parameter["type"] = "number";
            parameter["default"] = 0.1;
            parameter["description"] = "WebUI update request and snapshot construction interval in seconds.";
            return parameter;
        }

        dictionary make_webui_log_buffer_limit_parameter()
        {
            dictionary parameter;
            parameter["_tag"] = "parameter";
            parameter["name"] = "webui_log_buffer_limit";
            parameter["type"] = "number";
            parameter["default"] = static_cast<int>(default_max_retained_webui_log_messages);
            parameter["description"] = "Maximum number of recent log messages retained for delivery to WebUI clients.";
            return parameter;
        }

    }

    bool
    Component::InputsReady(dictionary d,  input_map ingoing_connections)
    {
        if(d.contains("size"))
            return true;

        if(d.contains("inputs") && d["inputs"].is_list())
            return true;

        std::string full_name = path_ + "." + std::string(d["name"]);
        Trace("\t\t\tComponent::InputReady", full_name);
        Kernel& k = kernel();

        if(!ingoing_connections.count(full_name))
            return d.is_set("optional");

        for(auto & c : ingoing_connections.at(full_name))
            if(k.buffers.at(c->source).rank()==0)
                return false;
        return true;
    }


    std::string
    Component::ShapeString(const std::vector<int> & shape) const
    {
        std::vector<std::string> parts;
        parts.reserve(shape.size());
        for(int dimension : shape)
            parts.push_back(std::to_string(dimension));
        return join(",", parts);
    }


    void
    Component::ValidateFixedInputTarget(const std::string & name,
                                        const std::string & full_name,
                                        const Connection & connection,
                                        const range & target_range,
                                        bool flattened)
    {
        const matrix & input_buffer = kernel().buffers.at(full_name);
        const std::vector<int> & shape = input_buffer.shape();
        if(flattened && input_buffer.rank() != 1)
            throw setup_failed("Input \"" + name + "\" in \"" + path_ +
                               "\" uses flatten and must have a one-dimensional fixed size, got \"" +
                               ShapeString(shape) + "\".", path_);

        bool outside = target_range.rank() != input_buffer.rank();
        for(int dimension = 0; !outside && dimension < target_range.rank(); ++dimension)
            outside = (flattened ? target_range.step(dimension) <= 0
                                 : target_range.step(dimension) == 0) ||
                      target_range.start(dimension) < 0 ||
                      target_range.start(dimension) > target_range.stop(dimension) ||
                      target_range.stop(dimension) > input_buffer.size(dimension);

        if(outside)
            throw setup_failed("Connection \"" + connection.Info() +
                               "\" writes outside fixed size of input \"" + name +
                               "\" in \"" + path_ + "\" (" + ShapeString(shape) + ").",
                               path_);
    }


    void
    Component::ApplyInputLabel(const dictionary & input, const std::string & full_name,
                               const std::vector<Connection *> & connections)
    {
        if(!input.is_set("use_label"))
            return;
        if(connections.size() == 1 && !connections[0]->label_.empty())
            kernel().buffers.at(full_name).set_name(connections[0]->label_);
    }


    int
    Component::SetInputShape_Flat(dictionary d, input_map ingoing_connections)
    {
        Trace("\t\t\t\t\tComponent::SetInputShape_Flat", path_);

        std::string name = d.at("name");
        std::string full_name = path_ +"."+ name;
        bool has_fixed_size = d.contains("size");

        if(!ingoing_connections.count(full_name)) // Not connected
            return has_fixed_size ? 0 : 1;

        if(has_fixed_size)
        {
            std::string shape_expr = std::string(d.at("size"));
            if(shape_expr.empty())
                throw setup_failed("Input \""+name+"\" must have a value for \"size\".", path_);
            std::vector<int> shape = EvaluateShapeList(shape_expr);
            kernel().buffers[full_name].realloc(shape);
        }

        long long flattened_input_size = 0;
        for(auto & c : ingoing_connections.at(full_name))
        {
            c->flatten_ = true;

            matrix & output_buffer = kernel().buffers[c->source];
            if(output_buffer.is_dynamic())
                throw setup_failed("Connection \"" + c->Info() + "\" can not flatten dynamic output \"" + c->source + "\".", path_);

            range output_matrix = output_buffer.get_range();
            c->Resolve(output_matrix);  //**NEW  

            const long long required_size = static_cast<long long>(c->source_range.size()) * c->DelayCount();
            if(required_size > std::numeric_limits<int>::max() - flattened_input_size)
                throw setup_failed("Connection \"" + c->Info() + "\" requires an input larger than the supported size.", path_);
            const int begin_index = static_cast<int>(flattened_input_size);
            flattened_input_size += required_size;
            const int end_index = static_cast<int>(flattened_input_size);
            c->target_range = range(begin_index, end_index);
            if(has_fixed_size)
                ValidateFixedInputTarget(name, full_name, *c, c->target_range, true);
        }
    
        if(!has_fixed_size && flattened_input_size != 0)
        {
            kernel().buffers[full_name].realloc(static_cast<int>(flattened_input_size));
          Trace("\t\t\tComponent::SetInputShape_Index Alloc "+std::to_string(flattened_input_size), path_);
        }

        if(d.is_set("use_label"))
        {
            for(auto & c : ingoing_connections.at(full_name))
            {
                const long long required_size = static_cast<long long>(c->source_range.size()) * c->DelayCount();
                if(required_size > std::numeric_limits<int>::max())
                    throw setup_failed("Connection \"" + c->Info() + "\" requires an input larger than the supported size.", path_);
                int s = static_cast<int>(required_size);
                if(c->label_.empty())
                    kernel().buffers[full_name].push_label(0, c->source, s);
                else
                    kernel().buffers[full_name].push_label(0, c->label_, s);
            }
        }
        return 0;
    }


    int
    Component::SetStackedInputShape(const dictionary & input, const std::string & name,
                                    const std::string & full_name, bool has_fixed_size,
                                    const std::vector<Connection *> & connections)
    {
        range input_size;
        int target_rank = has_fixed_size ? kernel().buffers[full_name].rank() : 0;

        for(int stack_index = 0; stack_index < static_cast<int>(connections.size()); ++stack_index)
        {
            Connection * connection = connections[stack_index];
            matrix & output_buffer = kernel().buffers[connection->source];
            if(output_buffer.is_dynamic())
                throw setup_failed("Connection \"" + connection->Info() +
                                   "\" can not feed stacked input \"" + name +
                                   "\" from dynamic output \"" + connection->source + "\".",
                                   path_);

            if(!connection->stacked_)
            {
                range output_matrix = output_buffer.get_range();
                if(output_matrix.rank() == 0)
                    return 0;

                range resolved_target = connection->Resolve(output_matrix);
                resolved_target.push_front(stack_index, stack_index + 1);
                connection->target_range = resolved_target;
                connection->stacked_ = true;
            }
            target_rank = std::max(target_rank, connection->target_range.rank());
        }

        if(target_rank == 0)
            return 0;

        for(Connection * connection : connections)
        {
            while(connection->target_range.rank() < target_rank)
                connection->target_range.push(0, 1);

            if(has_fixed_size)
                ValidateFixedInputTarget(name, full_name, *connection,
                                         connection->target_range, false);
            else if(input_size.rank() == 0)
                input_size = connection->target_range;
            else
                input_size.extend(connection->target_range);
        }

        if(!has_fixed_size)
        {
            kernel().buffers[full_name].realloc(input_size.extent());
            Trace("\t\t\tComponent::SetInputShape Stacked Alloc" +
                  std::string(input_size), full_name);
        }

        ApplyInputLabel(input, full_name, connections);
        return 1;
    }


    int
    Component::SetSimpleInputShape(const dictionary & input, const std::string & full_name,
                                   Connection & connection,
                                   const std::vector<Connection *> & connections)
    {
        matrix & output_buffer = kernel().buffers[connection.source];
        range output_matrix = output_buffer.get_range();
        if(output_matrix.rank() == 0)
            return 0;

        if(output_buffer.is_dynamic())
        {
            kernel().buffers[full_name].reserve(output_buffer.capacity());
            kernel().buffers[full_name].set_dynamic().set_fixed_capacity();
            kernel().buffers[full_name].resize(output_buffer.shape());
        }
        else
            kernel().buffers[full_name].realloc(output_matrix.extent());

        Trace("\t\t\tComponent::SetInputShape Simple Alloc", full_name);
        ApplyInputLabel(input, full_name, connections);
        return 1;
    }


    int
    Component::SetGeneralInputShape(const dictionary & input, const std::string & name,
                                    const std::string & full_name, bool has_fixed_size,
                                    const std::vector<Connection *> & connections)
    {
        range input_size;
        for(Connection * connection : connections)
        {
            matrix & output_buffer = kernel().buffers[connection->source];
            if(output_buffer.is_dynamic())
            {
                if(connection->IsWholeMatrixConnection() && connection->DelayCount() > 1)
                    throw setup_failed("Connection \"" + connection->Info() +
                                       "\" requests multiple delay values from dynamic output \"" +
                                       connection->source +
                                       "\". Dynamic outputs support only a single whole-matrix delay.",
                                       path_);
                throw setup_failed("Connection \"" + connection->Info() +
                                   "\" uses an indexed or ranged connection from dynamic output \"" +
                                   connection->source +
                                   "\". Dynamic outputs only support whole-matrix connections.",
                                   path_);
            }

            range output_matrix = output_buffer.get_range();
            if(output_matrix.rank() == 0)
                return 0;
            range resolved_target = connection->Resolve(output_matrix);
            if(has_fixed_size)
                ValidateFixedInputTarget(name, full_name, *connection, resolved_target, false);
            else
                input_size.extend(resolved_target);
        }

        if(!has_fixed_size)
        {
            kernel().buffers[full_name].realloc(input_size.extent());
            Trace("\t\t\tComponent::SetInputShape Alloc" + std::string(input_size), full_name);
        }

        ApplyInputLabel(input, full_name, connections);
        return 1;
    }


    int
    Component::SetInputShape_Index(dictionary d, input_map ingoing_connections)
    {
       Trace("\t\t\tComponent::SetInputShape_Index ", path_ + "." + std::string(d["name"]));

        std::string name = d.at("name");
        std::string full_name = path_ +"."+ name;
        bool has_fixed_size = d.contains("size");
        bool stack = ComputeAttributeBool(d, "stack");

        if(!ingoing_connections.count(full_name)) // Not connected
            return 1;

        if(has_fixed_size)
        {
            std::string shape_expr = std::string(d.at("size"));
            if(shape_expr.empty())
                throw setup_failed("Input \""+name+"\" must have a value for \"size\".", path_);
            std::vector<int> shape = EvaluateShapeList(shape_expr);
            kernel().buffers[full_name].realloc(shape);
        }

        // Handle stacked inputs by assigning each connection to one slice along a new first dimension.

        if(stack)
            return SetStackedInputShape(d, name, full_name, has_fixed_size,
                                        ingoing_connections.at(full_name));

        // Handle single connection without inidices - do not collapse dimensions

        auto & input_connections = ingoing_connections.at(full_name);
        Connection * single_connection = input_connections.size() == 1 ? input_connections[0] : nullptr;
        bool old_style_simple_connection =
            !has_fixed_size &&
            ingoing_connections.size() == 1 &&
            ingoing_connections.begin()->second[0]->DelayCount() == 1 &&
            ingoing_connections.begin()->second[0]->source_range.rank() == 0 &&
            ingoing_connections.begin()->second[0]->target_range.rank() == 0;
        bool dynamic_simple_connection =
            !has_fixed_size &&
            single_connection != nullptr &&
            single_connection->DelayCount() == 1 &&
            kernel().buffers[single_connection->source].is_dynamic() &&
            single_connection->IsWholeMatrixConnection();

        if(old_style_simple_connection || dynamic_simple_connection)
        {
            Connection * connection = old_style_simple_connection ?
                                      ingoing_connections.begin()->second[0] : single_connection;
            return SetSimpleInputShape(d, full_name, *connection, input_connections);
        }

        return SetGeneralInputShape(d, name, full_name, has_fixed_size,
                                    ingoing_connections.at(full_name));
    }


// ****************************** COMPONENT Sizes ******************************


    int 
    Component::SetInputSize(dictionary d, input_map ingoing_connections)
    {
        Trace("\t\t\tComponent::SetInputSize ", path_ + "."+ std::string(d["name"]));

        if(d.is_set("flatten"))
            SetInputShape_Flat(d, ingoing_connections);
        else
            SetInputShape_Index(d, ingoing_connections);
        return 0;
    }



    int
    Component:: SetInputSizes(input_map ingoing_connections)
    {
        Kernel& k = kernel();

        Trace("\t\tComponent::SetInputSizes", path_);

        // Set input sizes (if possible)

        for(auto d : info_["inputs"])
        {
            dictionary input = d;
            std::string full_name = path_+"."+std::string(input["name"]);
            bool has_fixed_size = input.contains("size");
            if(has_fixed_size || k.buffers[full_name].is_uninitialized())
                if(InputsReady(input, ingoing_connections))
                    SetInputSize(input, ingoing_connections);
        }
        return 0;
    }


    int 
    Component::SetOutputShape(dictionary d, input_map ingoing_connections)
    {
       Trace("\t\t\tComponent::SetOutputShape " , path_ + "." + std::string(d["name"]));

        if(d.contains_non_null("alias"))
            return 0;

        if(d.contains("size") || d.contains("shape"))
        {
            std::string attribute = d.contains("size") ? "size" : "shape";
            throw setup_failed("Output \"" + std::string(d["name"]) + "\" in group \"" + path_ +
                               "\" can not have a " + attribute + " attribute.", path_);
        }

        if(d.is_set("dynamic"))
            throw setup_failed("Group output \"" + std::string(d["name"]) + "\" in \"" + path_ + "\" can not be dynamic.", path_);

        range output_range;
        std::string name = d.at("name");
        std::string full_name = path_ +"."+ name;

        if(!ingoing_connections.count(full_name))
            return 0;

        for(auto c : ingoing_connections.at(full_name))
        {
            matrix & output_buffer = kernel().buffers[c->source];
            if(output_buffer.is_dynamic())
                throw setup_failed("Connection \"" + c->Info() + "\" can not map dynamic output \"" + c->source + "\" through group output \"" + full_name + "\".", path_);

            range output_matrix = output_buffer.get_range();
            
            if(output_matrix.rank() == 0)
                return 0;
            
            output_range.extend(c->Resolve(output_matrix));
        }
        kernel().buffers[full_name].realloc(output_range);
      Trace("\t\t\t\t\tComponent:: Alloc" + std::string(output_range), path_);

        return 1;
    }


    int
    Component::ApplyOutputAliases()
    {
        Kernel & k = kernel();

        for(auto & output_value : info_["outputs"])
        {
            dictionary d = output_value;
            if(!d.contains_non_null("alias"))
                continue;

            std::string output_name = d["name"].as_string();
            std::string full_output_name = path_ + "." + output_name;
            std::string alias_spec = trim(d["alias"].as_string());

            if(alias_spec.empty())
                throw setup_failed("Output \"" + output_name + "\" has an empty alias.", path_);

            if(d.contains("size") || d.contains("shape"))
                throw setup_failed("Aliased output \"" + output_name + "\" can not also specify a size or shape.", path_);

            std::string alias_source_name = peek_head(alias_spec, "[");
            std::string alias_selector = peek_tail(alias_spec, "[", true);
            if(alias_source_name.empty())
                throw setup_failed("Output \"" + output_name + "\" has malformed alias \"" + alias_spec + "\".", path_);

            if(alias_source_name.find('.') == std::string::npos)
                alias_source_name = path_ + "." + alias_source_name;

            if(alias_source_name == full_output_name)
                throw setup_failed("Output \"" + output_name + "\" can not alias itself.", path_);

            if(!k.buffers.count(alias_source_name))
                throw setup_failed("Output \"" + output_name + "\" aliases unknown output \"" + alias_source_name + "\".", path_);

            matrix aliased_output = k.buffers[alias_source_name];
            if(aliased_output.is_uninitialized())
                return 0;

            try
            {
                if(!alias_selector.empty())
                {
                    range selector(alias_selector);
                    for(int i = 0; i < selector.rank(); ++i)
                    {
                        bool is_single_index = !selector.empty(i) && selector.step(i) == 1 &&
                                               selector.stop(i) == selector.start(i) + 1;
                        if(!is_single_index)
                            throw setup_failed("Output \"" + output_name + "\" alias must use single indices only.", path_);

                        if(aliased_output.rank() == 0)
                            throw setup_failed("Output \"" + output_name + "\" alias indexes deeper than its source output.", path_);

                        aliased_output = aliased_output[selector.start(i)];
                    }
                }
            }
            catch(const setup_failed &)
            {
                throw;
            }
            catch(const std::exception & e)
            {
                throw setup_failed("Output \"" + output_name + "\" has invalid alias \"" + alias_spec + "\". " + std::string(e.what()), path_);
            }

            if(!alias_selector.empty())
                aliased_output.set_name(output_name);
            k.buffers[full_output_name] = aliased_output;
        }

        return 1;
    }


    int 
    Component::SetOutputShapes(input_map ingoing_connections)
    {
        Trace("\t\tComponent::SetOutputShapes", path_);
        for(auto & d : info_["outputs"])
            SetOutputShape(d, ingoing_connections);
        ApplyOutputAliases();

        return 0;
    }


    int
    Component::SetStateShape(dictionary d)
    {
        Trace("\t\t\tComponent::SetStateShape ", path_ + "." + std::string(d["name"]));

        if(d.contains_non_null("type") && std::string(d["type"]) != "matrix")
            return 0;

        if(!d.contains_non_null("type") || std::string(d["type"]) != "matrix")
            throw setup_failed("State \"" + std::string(d["name"]) + "\" in \"" + path_ + "\" must have type=\"matrix\" in this implementation.", path_);

        std::string shape_expr;
        if(d.contains("size"))
            shape_expr = std::string(d.at("size"));
        else if(d.contains("shape"))
            shape_expr = std::string(d.at("shape"));
        else
            throw setup_failed("State \"" + std::string(d["name"]) + "\" in \"" + path_ + "\" must have a value for \"size\" or \"shape\".", path_);

        if(shape_expr.empty())
            throw setup_failed("State \"" + std::string(d["name"]) + "\" in \"" + path_ + "\" must have a value for \"size\" or \"shape\".", path_);

        try
        {
            std::vector<int> shape = EvaluateShapeList(shape_expr);
            matrix state;
            Bind(state, d.at("name"));
            state.realloc(shape);
        }
        catch(const std::invalid_argument & e)
        {
            throw setup_failed("Size expression for state \"" + std::string(d["name"]) + "\" is invalid. " + e.what(), path_);
        }
        catch(const std::exception & e)
        {
            throw setup_failed("Size expression for state \"" + std::string(d["name"]) + "\" is invalid. " + std::string(e.what()), path_);
        }

        return 0;
    }


    int
    Component::SetStateShapes(input_map)
    {
        Trace("\t\tComponent::SetStateShapes", path_);
        for(auto & d : info_["states"])
            SetStateShape(d);

        return 0;
    }


    int
    Component::SetSizes(input_map ingoing_connections)
    {
        
        Trace("\tComponent::SetSizes",path_);
        SetInputSizes(ingoing_connections);
        SetOutputShapes(ingoing_connections);
        SetStateShapes(ingoing_connections);

        return 0;
    }


    void
    Component::CheckRequiredInputs()
    {
        Kernel & k = kernel();
        for(dictionary d : info_["inputs"])
        if(!d.is_set("optional") && k.buffers[path_+"."+d["name"].as_string()].is_uninitialized())
        {
            // Unconnected group inputs that are never referenced internally are harmless.
            if(dynamic_cast<Group *>(this) != nullptr)
            {
                std::string full_input_name = path_+"."+d["name"].as_string();
                bool consumed_inside_group = false;
                for(auto & c : k.connections)
                    if(c.source == full_input_name)
                    {
                        consumed_inside_group = true;
                        break;
                    }
                if(!consumed_inside_group)
                    continue;
            }

            throw setup_failed("Component \""+info_["name"].as_string()+"\" has required input \""+d["name"].as_string()+"\" that is not connected.", path_);
        }
    }



    void
    Component::CalculateCheckSum(long & check_sum, prime & prime_number) // Calculates a value that depends on all parameters and output sizes; used for integrity testing of kernel and module
    {
        // Iterate over all outputs
        for(auto & d : info_["outputs"])
        {
            matrix output;
            Bind(output, d["name"]);
            for(long d : output.shape())
                check_sum += prime_number.next() * d;
        } 

        // Iterate over all inputs

        for(auto & d : info_["inputs"])
        {
            matrix input;
            Bind(input, d["name"]);
            for(long d : input.shape())
                check_sum += prime_number.next() * d;
        } 


        // Iterate obver all parameters
    
        for(auto & d : info_["parameters"])
        {
            std::string parameter_name = d["name"].as_string();
            if(parameter_name == "log_level" || parameter_name == "module_start" || parameter_name == "start_tick" || parameter_name == "async" || parameter_name == "color" || parameter_name == "rgb_quality" || parameter_name == "gray_quality" || parameter_name == "snapshot_interval" || parameter_name == "webui_req_int" || parameter_name == "webui_log_buffer_limit")
                continue;

            parameter p;
            Bind(p, parameter_name);
            //std::cout << "Parameter: " << d["name"] << std::endl;

            if(p.get_type() == string_type)
                check_sum += prime_number.next() * character_sum(p);
            else
            if(p.get_type() == matrix_type)
            {
                const matrix & matrix_value = p.matrix_ref();
                check_sum += prime_number.next() * matrix_value.size();
            }
            else
                check_sum += prime_number.next() * p.as_long();
        }
        // std::cout << "Check sum: " << check_sum << std::endl;
    }



// Kernel

    void
    Kernel::ResolveParameter(parameter & p,  std::string & name)
    {
        if(p.is_resolved())
            return; // Already set from SetParameters

        std::size_t i = name.rfind(".");
        if(i == std::string::npos)
            throw exception("Malformed parameter name \"" + name + "\".");

        Component * c = components.at(name.substr(0, i)).get();
        std::string parameter_name = name.substr(i+1, name.size());
        c->ResolveParameter(p, parameter_name);
    }


    void 
    Kernel::ResolveParameters() // Find and evaluate value or default
    {
        // All all componenets to initialize parameters programmatically

        for(auto & [name, component] : components)
        {
            (void)name;
            component->SetParameters();
        }

        // resolve
        bool ok = true;
        for (auto p=parameters.rbegin(); p!=parameters.rend(); p++) // Reverse order equals outside in in groups
        {
            std::size_t i = p->first.rfind(".");
            if(i == std::string::npos)
                throw setup_failed("Malformed parameter name \""+p->first+"\".", p->first);

            Component * c = components.at(p->first.substr(0, i)).get();
            std::string parameter_name = p->first.substr(i+1, p->first.size());
            ok &= c->ResolveParameter(p->second, parameter_name);
        }

        for(auto & [name, component] : components)
        {
            (void)name;
            component->SyncFirstTickFromParameter();
            if(dynamic_cast<Module *>(component.get()))
                component->SyncAsyncModeFromParameter();
            else
                component->async_mode = false;
        }

        if(!ok)
        {
            for(auto & [name, parameter] : parameters)
                if(!parameter.is_resolved())
                    throw setup_failed("Parameter \""+name+"\" could not be resolved.", name);
            throw setup_failed("All parameters could not be resolved.");
        }
    }


    void
    Kernel::ShareZeroDelayConnectionBuffers()
    {
        std::map<std::string, std::vector<Connection *>> incoming_connections;
        for(auto & connection : connections)
            incoming_connections[connection.target].push_back(&connection);

        for(auto & connection : connections)
        {
            connection.shared_memory_ = false;

            if(!connection.IsSingleDelay(0))
                continue;

            if(Component * source_component = ComponentForValuePath(connection.source); source_component != nullptr && source_component->async_mode)
                continue;

            if(Component * target_component = ComponentForValuePath(connection.target); target_component != nullptr && target_component->async_mode)
                continue;

            if(!connection.IsWholeMatrixConnection() || connection.stacked_)
                continue;

            if(incoming_connections[connection.target].size() != 1)
                continue;

            auto source_buffer = buffers.find(connection.source);
            auto target_buffer = buffers.find(connection.target);
            if(source_buffer == buffers.end() || target_buffer == buffers.end())
                continue;

            if(connection.source == connection.target)
                continue;

            if(source_buffer->second.is_dynamic() || target_buffer->second.is_dynamic())
                continue;

            if(source_buffer->second.shape() != target_buffer->second.shape())
                continue;

            target_buffer->second.share_storage(source_buffer->second);
            connection.shared_memory_ = true;
        }
    }


    void 
    Kernel::CalculateDelays()
    {
        max_delays.clear();
        for(auto & c : connections)
        {
            if(!c.UsesCircularBuffer())
                continue;
            max_delays[c.source] = std::max(max_delays[c.source], c.MaxDelay());
        }
    }

    // Functions for creating the network

    void 
    Kernel::AddInput(std::string name, dictionary parameters) 
    {
        buffers[name] = matrix().set_name(parameters["name"]);
    }

    void 
    Kernel::AddOutput(std::string name, dictionary parameters)
    {
        buffers[name] = matrix().set_name(parameters["name"]);
        if(parameters.is_set("persistent"))
            persistent_outputs.insert(name);
    }


    void
    Kernel::AddState(std::string name, dictionary parameters)
    {
        if(!parameters.contains_non_null("type"))
            throw exception("State \"" + name + "\" must have a type.");

        std::string type = parameters["type"];
        if(type == "matrix")
        {
            buffers[name] = matrix().set_name(parameters["name"]);
            state_buffers.insert(name);
            if(parameters.is_set("persistent"))
                persistent_state_buffers.insert(name);
            return;
        }

        if(!is_scalar_state_type(type))
            throw exception("State \"" + name + "\" has unsupported type \"" + type + "\".");

        ScalarState state;
        state.type = type;
        state.persistent = parameters.is_set("persistent");
        std::string default_value = parameters.contains_non_null("default") ? std::string(parameters["default"]) : "";
        try
        {
            if(type == "float")
                state.default_float_value = state.float_value = default_value.empty() ? 0 : static_cast<float>(kernel_detail::parse_parameter_number(default_value, "float"));
            else if(type == "double")
                state.default_double_value = state.double_value = default_value.empty() ? 0 : kernel_detail::parse_parameter_number(default_value, "double");
            else if(type == "int")
                state.default_int_value = state.int_value = default_value.empty() ? 0 : kernel_detail::parse_strict_int(default_value);
            else if(type == "bool")
            {
                bool parsed_value = false;
                if(!default_value.empty() && !parse_bool(default_value, parsed_value))
                    throw std::invalid_argument("Expected true, false, yes, no, on, off, 1, or 0.");
                state.default_bool_value = state.bool_value = parsed_value;
            }
            else if(type == "string")
                state.default_string_value = state.string_value = default_value;
        }
        catch(const std::exception & e)
        {
            throw exception("State \"" + name + "\" has invalid default value \"" + default_value + "\": " + e.what());
        }

        scalar_states[name] = state;
    }

    void 
    Kernel::AddParameter(std::string name, dictionary params)
    {
         parameters.emplace(name, parameter(params));
    }


    void 
    Kernel::SetParameter(std::string name, std::string value)
    {
        if(!parameters.count(name))
            throw exception("Parameter \""+name+"\" could not be set because it does not exist.");

        try
        {
            parameters[name] = value;
            parameters[name].set_source_value(value);
        }
        catch(const exception & e)
        {
            throw exception("Parameter \""+name+"\" could not be set: "+e.message());
        }
        catch(const std::exception & e)
        {
            throw exception("Parameter \""+name+"\" could not be set: "+std::string(e.what()));
        }
        catch(...)
        {
            throw exception("Parameter \""+name+"\" could not be set. Check that the parameter exists and that the data type and value is correct.");
        }
    }


    void
    Kernel::SetParameter(std::string name, const matrix & value, const std::string & source_value)
    {
        if(!parameters.count(name))
            throw exception("Parameter \""+name+"\" could not be set because it does not exist.");

        try
        {
            parameters[name].set_matrix(value);
            matrix stored_value = value;
            parameters[name].set_source_value(source_value.empty() ? stored_value.json() : source_value);
        }
        catch(const exception & e)
        {
            throw exception("Parameter \""+name+"\" could not be set: "+e.message());
        }
        catch(const std::exception & e)
        {
            throw exception("Parameter \""+name+"\" could not be set: "+std::string(e.what()));
        }
        catch(...)
        {
            throw exception("Parameter \""+name+"\" could not be set. Check that the parameter exists and that the data type and value is correct.");
        }
    }


    void 
    Kernel::AddGroup(dictionary info, std::string path, bool is_top_group)
    {
        if(info["parameters"].is_null())
            info["parameters"] = list();

        if(is_top_group)
        {
            top_group_path = path;
            bool has_color = false;
            bool has_rgb_quality = false;
            bool has_gray_quality = false;
            bool has_snapshot_interval = false;
            bool has_webui_req_int = false;
            bool has_webui_log_buffer_limit = false;
            for(auto parameter : info["parameters"])
            {
                std::string parameter_name = parameter["name"];
                if(parameter_name == "color")
                    has_color = true;
                else if(parameter_name == "rgb_quality")
                    has_rgb_quality = true;
                else if(parameter_name == "gray_quality")
                    has_gray_quality = true;
                else if(parameter_name == "snapshot_interval")
                    has_snapshot_interval = true;
                else if(parameter_name == "webui_req_int")
                    has_webui_req_int = true;
                else if(parameter_name == "webui_log_buffer_limit")
                    has_webui_log_buffer_limit = true;
            }

            if(!has_color)
                info["parameters"].push_back(make_color_parameter().copy());
            if(!has_rgb_quality)
                info["parameters"].push_back(make_ui_snapshot_rgb_quality_parameter().copy());
            if(!has_gray_quality)
                info["parameters"].push_back(make_ui_snapshot_gray_quality_parameter().copy());
            if(!has_snapshot_interval)
                info["parameters"].push_back(make_snapshot_interval_parameter().copy());
            if(!has_webui_req_int)
                info["parameters"].push_back(make_webui_request_interval_parameter().copy());
            if(!has_webui_log_buffer_limit)
                info["parameters"].push_back(make_webui_log_buffer_limit_parameter().copy());
        }

        current_component_info = info;
        current_component_path = path;

        if(components.count(current_component_path)> 0)
            throw build_failed("Module or group named \""+current_component_path+"\" already exists.", path);

        components[current_component_path] = std::make_unique<Group>(); // Implicit argument passing as for components
    }


    void 
    Kernel::InstantiatePythonModule(dictionary & info, const std::string & path)
    {
        current_component_info = info;
        current_component_path = path+"."+std::string(info["name"]);

        if(!classes.count("PythonModule") || classes["PythonModule"].module_creator == nullptr)
            throw build_failed("Internal PythonModule runtime class is not installed.", path);

        components[current_component_path] = std::unique_ptr<Component>(classes["PythonModule"].module_creator());
    }


    bool
    Kernel::PreparePythonModule(dictionary & info, const std::string & classname)
    {
        std::filesystem::path class_path = classes[classname].path;
        std::filesystem::path class_directory = class_path.parent_path();

        std::filesystem::path python_path = class_directory / (classname + ".py");
        bool is_python_backed = std::filesystem::exists(python_path);

        if(!is_python_backed)
            return false;

        std::error_code ec;
        std::filesystem::path canonical_class_directory = std::filesystem::weakly_canonical(class_directory, ec);
        if(ec)
            throw build_failed("Could not resolve python class directory for class \"" + classname + "\".");

        if(classes.count("PythonModule"))
        {
            dictionary python_runtime_info = classes["PythonModule"].info_.copy();
            python_runtime_info.erase("name");
            python_runtime_info.erase("description");
            info.merge(python_runtime_info);

            if(info["parameters"].is_null())
                info["parameters"] = list();

            std::set<std::string> parameter_names;
            for(auto parameter : info["parameters"])
                parameter_names.insert(std::string(parameter["name"]));

            for(auto parameter : python_runtime_info["parameters"])
            {
                std::string parameter_name = parameter["name"];
                if(!parameter_names.count(parameter_name))
                {
                    info["parameters"].push_back(parameter);
                    parameter_names.insert(parameter_name);
                }
            }
        }

        std::filesystem::path canonical_python_path = std::filesystem::weakly_canonical(python_path, ec);
        if(ec)
            throw build_failed("Python script for class \"" + classname + "\" could not be resolved: " + python_path.string());

        auto class_it = canonical_class_directory.begin();
        auto class_end = canonical_class_directory.end();
        auto python_it = canonical_python_path.begin();
        auto python_end = canonical_python_path.end();
        for(; class_it != class_end && python_it != python_end; ++class_it, ++python_it)
            if(*class_it != *python_it)
                throw build_failed("Python script for class \"" + classname + "\" must stay within its class directory.");
        if(class_it != class_end)
            throw build_failed("Python script for class \"" + classname + "\" must stay within its class directory.");

        info["python"] = canonical_python_path.string();
        return true;
    }


    void
    Kernel::InstantiateStandardModule(dictionary & info, const std::string & classname, const std::string & path)
    {
        current_component_info = info;
        current_component_path = path+"."+std::string(info["name"]);

        if(classes[classname].module_creator == nullptr)
        {
            if(info.is_not_set("no_code"))
                std::cout << "Class \""<< classname << "\" has no installed code. Creating group." << std::endl; // throw exception("Class \""+classname+"\" has no installed code. Check that it is included in CMakeLists.txt."); // TODO: Check that this works for classes that are allowed to have no code
            info["_tag"]="group";
            BuildGroup(info, path); 
        }
        else
            components[current_component_path] = std::unique_ptr<Component>(classes[classname].module_creator());
    }


    void 
    Kernel::AddModule(dictionary info, std::string path)
    {
        current_component_info = info;
        current_component_path = path+"."+std::string(info["name"]);

        if(components.count(current_component_path)> 0)
            throw build_failed("Module or group with this name already exists. \""+std::string(info["name"])+"\".", path);

        std::string classname = info["class"];

        if(!classname.empty() && (classname.find('@') != std::string::npos || classname.find('{') != std::string::npos))
        {
            Component * c = components.at(path).get();
            classname = c->ComputeValue(classname);
            info["class"] = classname;
        }

        if(!classes.count(classname))
            throw build_failed("Class \""+classname+"\" does not exist.", path);

        if(classes[classname].path.empty())
            throw build_failed("Class file \""+classname+".ikc\" could not be found.", path);

        info.merge(classes[classname].info_);  // merge with scanned class data, including injected defaults

        bool is_python_backed = PreparePythonModule(info, classname);

        if(info["parameters"].is_null())
            info["parameters"] = list();

        bool has_log_level = false;
        bool has_module_start = false;
        bool has_start_tick = false;
        bool has_async = false;
        bool has_color = false;
        for(auto parameter : info["parameters"])
        {
            std::string parameter_name = parameter["name"];
            if(parameter_name == "log_level")
                has_log_level = true;
            else if(parameter_name == "module_start")
                has_module_start = true;
            else if(parameter_name == "start_tick")
                has_start_tick = true;
            else if(parameter_name == "async")
                has_async = true;
            else if(parameter_name == "color")
                has_color = true;
        }

        if(!has_log_level)
        {
            info["parameters"].push_back(Component::LogLevelParameterInfo().copy());
        }

        if(!has_module_start)
            info["parameters"].push_back(Component::ModuleStartParameterInfo().copy());

        if(!has_start_tick)
            info["parameters"].push_back(Component::StartTickParameterInfo().copy());

        if(!has_async)
            info["parameters"].push_back(Component::AsyncParameterInfo().copy());

        if(!has_color)
        {
            dictionary color_param;
            color_param["_tag"] = "parameter";
            color_param["name"] = "color";
            color_param["type"] = "string";
            color_param["default"] = "black";
            color_param["description"] = "Selected ui color";
            color_param["control"] = "ui_color";
            info["parameters"].push_back(color_param);
        }

        if(is_python_backed)
            InstantiatePythonModule(info, path);
        else
            InstantiateStandardModule(info, classname, path);
    }


    void 
    Kernel::AddConnection(dictionary info, std::string path)
    {
         std::string source = path + "." + std::string(info["source"]); 
         std::string target = path + "." + std::string(info["target"]);

        if(state_buffers.count(source) || scalar_states.count(source))
            throw build_failed("Connection source \"" + source + "\" is private state and can not be connected.", source);
        if(state_buffers.count(target) || scalar_states.count(target))
            throw build_failed("Connection target \"" + target + "\" is private state and can not be connected.", target);

         std::string delay_range = info.contains_non_null("delay") ? info["delay"] : "";
         std::string label = info.contains_non_null("label") ? info["label"] : "";

        if(delay_range.empty() || delay_range=="null")
            delay_range = "[1]";
        else if(delay_range[0] != '[')
            delay_range = "["+delay_range+"]";
        range r;
        try
        {
            r = range(delay_range);
        }
        catch(const std::exception &)
        {
            throw build_failed("Connection \"" + source + " => " + target +
                               "\" has malformed delay range \"" + delay_range + "\".", path);
        }
        ValidateConnectionDelayRange(r, source, target, path);
        connections.push_back(Connection(source, target, r, label));
    }



    void Kernel::LoadExternalGroup(dictionary & d)
    {
        std::filesystem::path sanitized_path;
        if(!SanitizeImportPath(std::string(d["external"]), sanitized_path))
            throw build_failed("External group path must stay within the project root or user data directory.");

        dictionary external;
        LoadXMLWithRestrictedIncludes(external, sanitized_path);
        external["name"] = d["name"];
        d.merge(external);
        d.erase("external");
    }



    void 
    Kernel::BuildGroup(dictionary d, std::string path) // Traverse dictionary and build all items at each level
    {
        try
        {
            if(std::string(d["_tag"]) != "group")
                throw build_failed("Main element is '"+std::string(d["_tag"])+"' but must be 'group' for ikg-file.");

            if(!d.contains("name"))
                throw build_failed("Groups must have a name.", path);

            if(path.empty())
            {
                std::string log_level = d.contains_non_null("log_level") ? std::string(d["log_level"]) : "";
                if(log_level.empty() || log_level == "0")
                    d["log_level"] = msg_warning;
            }

            std::string name = validate_identifier(d["name"]);
            if(!path.empty())
                name = path+"."+name;

            if(d.contains("external"))
                LoadExternalGroup(d);

            AddGroup(d, name, path.empty());

            for(auto g : d["groups"])
                BuildGroup(g, name);
            for(auto m : d["modules"])
                AddModule(m, name);
            for(auto c : d["connections"])
                AddConnection(c, name);

            if(d["widgets"].is_null())
                d["widgets"] = list();
        }
        catch(const exception& e)
        {
            throw build_failed("Build group failed for "+path+": "+e.message());
        }
        catch(const std::exception& e)
        {
            throw build_failed("Build group failed for "+path+": "+std::string(e.what()), path);
        }
    }


    void 
    Kernel::InitComponents()
    {
        // Call Init for all modules (after CalcalateSizes and Allocate)
        for(auto & [name, component] : components)
        {
            (void)name;
            try
            {
                component->Init();
                component->initialized_ = true;
            }
            catch(const fatal_error & e)
            {
                throw init_error("While initializing module \"" + component->path_ + "\": " + e.message(),
                                 e.path().empty() ? component->path_ : e.path());
            }
            catch(const exception & e)
            {
                throw init_error("While initializing module \"" + component->path_ + "\": " + e.message(),
                                 e.path().empty() ? component->path_ : e.path());
            }
            catch(const std::exception & e)
            {
                throw init_error("While initializing module \"" + component->path_ + "\": " + e.what(),
                                 component->path_);
            }
            catch(...)
            {
                throw init_error("While initializing module \"" + component->path_ + "\": Unknown error.",
                                 component->path_);
            }
        }
    }


    void 
    Kernel::SetCommandLineParameters(dictionary & d) // Add explicit command line overrides without clobbering file values with defaults
    {
        // user_data is intentionally CLI-only and must never be sourced from a model file.
        if(d.contains("user_data"))
            d.erase("user_data");

        for(auto & [name, value] : options_.d)
            if(options_.is_explicitly_set(name))
                if(name != "user_data" && name != "auth_password")
                    d[name] = value;

        if(d.contains("stop"))
            stop_after = parse_stop_after(std::string(d["stop"]));

        if(d.contains("tick_duration"))
            tick_duration = parse_tick_duration(std::string(d["tick_duration"]));

        if(d.contains_non_null("threads"))
        {
            std::string thread_pool_value = std::string(d["threads"]);
            std::string trimmed_thread_pool_value = trim(thread_pool_value);
            int requested_threads = 0;
            const char * begin = trimmed_thread_pool_value.data();
            const char * end = begin + trimmed_thread_pool_value.size();
            bool valid_sign = true;
            if(begin != end && *begin == '+')
            {
                ++begin;
                valid_sign = begin != end && *begin != '+' && *begin != '-';
            }
            const auto result = std::from_chars(begin, end, requested_threads);
            if(trimmed_thread_pool_value.empty() || !valid_sign ||
               result.ec != std::errc() || result.ptr != end)
                throw setup_failed("Invalid thread pool size \"" + thread_pool_value + "\". Expected a positive integer.");

            if(requested_threads < 1)
                throw setup_failed("Invalid thread pool size \"" + thread_pool_value + "\". Expected a positive integer.");

            thread_pool = std::make_unique<ThreadPool>(requested_threads);
        }
    }


    std::string
    Kernel::GetTopLevelDefaultAttribute(const std::string & key) const
    {
        if(key == "tick_duration")
            return formatNumber(tick_duration);
        if(key == "stop")
            return std::to_string(stop_after);
        if(key == "filename")
            return options_.stem();
        if(key == "batch_mode")
            return options_.is_set("batch_mode") ? "true" : "false";
        if(key == "info")
            return options_.is_set("info") ? "true" : "false";
        if(key == "real_time")
            return options_.is_set("real_time") ? "true" : "false";
        if(key == "start")
            return options_.is_set("start") ? "true" : "false";

        auto it = options_.d.find(key);
        if(it != options_.d.end())
            return it->second;

        return "";
    }

    void
    Kernel::RegisterClass(const char * name, ModuleCreator mc)
    {
        classes[name].name = name;
        classes[name].module_creator = mc;
    }


    void
    Kernel::HandleFailedFileLoad()
    {
        dictionary failed_info = info_.copy();
        run_mode = run_mode_stop;
        timer.Pause();
        timer.SetPauseTime(0);
        if(!components.empty())
            StopComponents();
        Clear();
        info_ = failed_info;
        needs_reload = true;
    }


    void
    Kernel::LoadFileConfiguration()
    {
        std::lock_guard<std::recursive_mutex> lock(kernelLock);
        try
        {
            if(components.size() > 0)
            {
                StopComponents();
                Clear();
            }
            if(!std::filesystem::exists(options_.full_path()))
                throw load_failed("File \""+options_.full_path()+"\" does not exist.");

            try
            {
                dictionary d;
                LoadXMLWithRestrictedIncludes(d, options_.full_path());
                d["filename"] = options_.stem();
                info_ = d.copy();
                session_id = NewSessionID();
                ResetUISnapshotCache();
                SetCommandLineParameters(d);
                info_ = d.copy();
            }
            catch(const load_failed & e)
            {
                throw load_failed("Load file failed for "s+options_.full_path()+". "+e.message(), e.path());
            }
            catch(const setup_failed & e)
            {
                throw setup_failed("Set-up file failed for "s+options_.full_path()+". "+e.message(), e.path());
            }
            catch(const std::exception & e)
            {
                throw load_failed("Load or set-up failed for "s+options_.full_path()+". "+e.what());
            }
        }
        catch(const exception &)
        {
            HandleFailedFileLoad();
            throw;
        }
    }


    void
    Kernel::SetUpLoadedFile()
    {
        std::lock_guard<std::recursive_mutex> lock(kernelLock);
        try
        {
            try
            {
                dictionary d = info_.copy();
                BuildGroup(d);
                info_ = d;
                Notify(msg_print, "Loaded "s+options_.full_path());
                SetUp();
                if(options_.is_explicitly_set("load_state"))
                    LoadState(ResolveStateFilename("load_state"));
                CalculateCheckSum();
                BuildUISnapshot();
                needs_reload = false;
                automatic_reload_suppressed_until_save.store(false, std::memory_order_release);
                Pause(); // Reset clocks
            }
            catch(const load_failed & e)
            {
                throw load_failed("Load file failed for "s+options_.full_path()+". "+e.message(), e.path());
            }
            catch(const setup_failed & e)
            {
                throw setup_failed("Set-up file failed for "s+options_.full_path()+". "+e.message(), e.path());
            }
            catch(const std::exception & e)
            {
                throw load_failed("Load or set-up failed for "s+options_.full_path()+". "+e.what());
            }
        }
        catch(const exception &)
        {
            HandleFailedFileLoad();
            throw;
        }
    }


    void
    Kernel::LoadFile()
    {
        LoadFileConfiguration();
        SetUpLoadedFile();
    }


    bool
    Kernel::AutomaticReloadSuppressed() const
    {
        return automatic_reload_suppressed_until_save.load(std::memory_order_acquire);
    }


    void
    Kernel::SuppressAutomaticReloadUntilSave()
    {
        automatic_reload_suppressed_until_save.store(true, std::memory_order_release);
    }


    void
    Kernel::ScanClasses(std::string path)
    {
        if(!std::filesystem::exists(path))
        {
            std::cout << "Could not scan for classes \"" + path + "\". Directory not found.\n";
            return;
        }
        for(auto& p: std::filesystem::recursive_directory_iterator(path))
            if(std::string(p.path().extension())==".ikc")
            {
                const std::string name = p.path().stem();
                auto existing_class = classes.find(name);
                if(existing_class != classes.end() && !existing_class->second.path.empty())
                    throw exception("Duplicate class \"" + name +
                                    "\" was found in more than one .ikc file: \"" +
                                    existing_class->second.path + "\" and \"" +
                                    p.path().string() + "\".", p.path().string());

                try
                {
                    dictionary class_info;
                    LoadXMLWithRestrictedIncludes(class_info, p.path());

                    const std::string root_element = class_info["_tag"];
                    if(root_element != "class")
                        throw exception("Root element must be <class>, not <" + root_element + ">.");

                    const std::string declared_name = class_info["name"];
                    if(declared_name != name)
                        throw exception("Declared class name \"" + declared_name +
                                        "\" does not match filename \"" + name + "\".");

                    class_info.ensure_list("parameters");

                    bool has_log_level = false;
                    bool has_module_start = false;
                    bool has_start_tick = false;
                    bool has_async = false;
                    bool has_color = false;
                    for(auto parameter : class_info["parameters"])
                    {
                        std::string parameter_name = parameter["name"];
                        if(parameter_name == "log_level")
                            has_log_level = true;
                        else if(parameter_name == "module_start")
                            has_module_start = true;
                        else if(parameter_name == "start_tick")
                            has_start_tick = true;
                        else if(parameter_name == "async")
                            has_async = true;
                        else if(parameter_name == "color")
                            has_color = true;
                    }

                    if(!has_log_level)
                        class_info["parameters"].push_back(Component::LogLevelParameterInfo().copy());

                    if(!has_module_start)
                        class_info["parameters"].push_back(Component::ModuleStartParameterInfo().copy());

                    if(!has_start_tick)
                        class_info["parameters"].push_back(Component::StartTickParameterInfo().copy());

                    if(!has_async)
                        class_info["parameters"].push_back(Component::AsyncParameterInfo().copy());

                    if(!has_color)
                        class_info["parameters"].push_back(make_color_parameter().copy());

                    Class & scanned_class = classes[name];
                    scanned_class.info_ = std::move(class_info);
                    scanned_class.name = name;
                    scanned_class.path = p.path();
                }
                catch(const exception & e)
                {
                    Notify(msg_warning, "Could not load class file \"" + p.path().string() + "\": " + e.message(), p.path().string());
                }
                catch(const std::exception & e)
                {
                    Notify(msg_warning, "Could not load class file \"" + p.path().string() + "\": " + e.what(), p.path().string());
                }
            }
    }


    void
    Kernel::ScanFiles(std::string path, bool system, bool examples)
    {
        if(!std::filesystem::exists(path))
        {
            std::cout << "Could not scan for files in \"" + path + "\". Directory not found.\n";
            return;
        }
        for(auto& p: std::filesystem::recursive_directory_iterator(path))
        {
            const std::string extension = p.path().extension().string();
            if(extension==".ikg")
            {
                try
                {
                    dictionary file_info;
                    LoadXMLWithRestrictedIncludes(file_info, p.path());
                    if(file_info.is_set("internal"))
                        continue;
                }
                catch(const std::exception &)
                {
                }

                std::string name = p.path().stem();

                if(system)
                     system_files[name] = p.path();
                else if(examples)
                     examples_files[name] = p.path();
                else
                     user_files[name] = p.path();
            }
            else if(!system && !examples && extension==".state")
            {
                std::string name = p.path().filename().string();
                user_state_files[name] = p.path();
            }
        }
    }


    void
    Kernel::ListClasses()
    {
        std::cout << "\nClasses:\n";
        for(auto & [name, component_class] : classes)
        {
            (void)name;
            component_class.Print();
        }
    }



    void
    Kernel::CalculateCheckSum()
    {
        if(!info_.contains("check_sum"))
            return;

        long correct_check_sum = info_["check_sum"];
        long calculated_check_sum = 0;
        prime prime_number;

        // Iterate over task lists to test partitioning

        calculated_check_sum += prime_number.next() * tasks.size();
        for(auto & t : tasks)
            calculated_check_sum += prime_number.next() * t.size();

        // Iterate over components

        for(auto & [n,c] : components)
            c->CalculateCheckSum(calculated_check_sum, prime_number);
        if(correct_check_sum == calculated_check_sum)
            std::cout << "Correct Check Sum: " << calculated_check_sum << '\n';
        else
        {
            const std::string msg = "Incorrect Check Sum: " +
                                    std::to_string(calculated_check_sum) + " != " +
                                    std::to_string(correct_check_sum);
            if(info_.is_set("batch_mode"))
                throw setup_failed(msg);
            Notify(msg_fatal_error, msg);
        }
    }


    void
    Kernel::SetUp()
    {
        try
        {
            task_timeout = 5.0;
            if(info_.contains_non_null("task_timeout"))
            {
                task_timeout = info_["task_timeout"].as_double();
                if(!std::isfinite(task_timeout) || task_timeout < 0)
                    throw setup_failed("task_timeout must be a finite non-negative number of seconds.");
            }

            PruneConnections();
            SortTasks();
            CalculateStartupSteps();
            ResolveParameters();
            CalculateDelays();
            CalculateSizes();
            ShareZeroDelayConnectionBuffers();

            InitCircularBuffers();
            for(auto & connection : connections)
                connection.ResolveRuntimeState();
            InitComponents();

            if(info_.is_set("info"))
            {
                ListParameters();
                ListConnections();
                ListBuffers();
            }
        }
        catch(exception & e)
        {
            throw setup_failed("SetUp Failed. "+e.message(), e.path());
        }
        catch(std::exception & e)
        {
            throw setup_failed("SetUp Failed. "+std::string(+e.what()));
        }
    }


    //
    //  Serialization
}; // namespace ikaros
