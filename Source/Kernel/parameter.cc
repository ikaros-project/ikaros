// Ikaros 3.0

#include "ikaros.h"
#include "kernel_parsing.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <optional>

namespace ikaros
{
    parameter::parameter():
        state_(std::make_shared<parameter_state>())
    {
    }

    bool
    parameter::compare_string(const std::string & value) const
    {
        return as_string() == value;
    }

    namespace
    {
        std::optional<double> get_parameter_bound(const dictionary & info, const std::string & name)
        {
            if(!info.contains_non_null(name))
                return std::nullopt;

            double value = 0;
            if(!parse_double(std::string(info[name]), value))
                return std::nullopt;
            if(!std::isfinite(value))
                throw exception("Parameter " + name + " constraint must be finite.");
            return value;
        }

        int clamp_option_index(int index, const std::vector<std::string> & options)
        {
            if(options.empty())
                return 0;
            if(index < 0)
                return 0;
            if(index >= int(options.size()))
                return int(options.size()) - 1;
            return index;
        }

        matrix scalar_parameter_matrix(double value)
        {
            matrix m(1);
            m(0) = static_cast<float>(value);
            return m;
        }

        double get_scalar_matrix_value(const matrix & value, const std::string & conversion_name)
        {
            if(value.size() != 1)
                throw exception("Could not convert matrix to " + conversion_name + ". Matrix must contain exactly one element.");

            std::vector<int> zero_index(value.rank(), 0);
            return value.at(zero_index);
        }

    }

    parameter::parameter(dictionary info):
        state_(std::make_shared<parameter_state>())
    {
        state_->info = std::move(info);
        state_->has_options = state_->info.contains("options");
        if(state_->has_options)
            state_->options = split(std::string(state_->info["options"]), ",");
        state_->dynamic = state_->info.is_set("dynamic");
        state_->minimum = get_parameter_bound(state_->info, "min");
        state_->maximum = get_parameter_bound(state_->info, "max");

        std::string type_string = state_->info["type"];

        if(type_string=="float" || type_string=="int" || type_string=="double")  // Temporary
            type_string = "number";

        auto type_index = std::find(parameter_strings.begin(), parameter_strings.end(), type_string);
        if(type_index == parameter_strings.end())
            throw exception("Unknown parameter type: "+type_string+".");

        state_->type = parameter_type(std::distance(parameter_strings.begin(), type_index));

        if(state_->minimum && state_->maximum && *state_->minimum > *state_->maximum)
            throw exception("Parameter minimum must not exceed maximum.");

        if(state_->has_options)
        {
            state_->value = 0;
            return;
        }

        switch(state_->type)
        {
            case number_type:
            case rate_type:
                state_->value = 0.0;
                break;

            case bool_type:
                state_->value = false;
                break;

            case string_type: 
                state_->value = std::string("");
                break;

            case matrix_type: 
                state_->value = matrix();
                break;

            default: 
                break;
        } 
    }



    parameter::parameter(const std::string type, const std::string options):
        parameter(options.empty() ? dictionary({{"type", type}}) : dictionary({{"type", type},{"options", options}}))
    {}


    parameter::parameter(const parameter & p):
        state_(p.clone_state())
    {
    }


    parameter &
    parameter::operator=(const parameter & p)
    {
        if(this != &p)
            state_ = p.clone_state();
        return *this;
    }


    std::shared_ptr<parameter::parameter_state>
    parameter::clone_state() const
    {
        auto cloned_state = std::make_shared<parameter_state>();
        if(!state_)
            return cloned_state;

        cloned_state->info = state_->info.copy();
        cloned_state->has_options = state_->has_options;
        cloned_state->options = state_->options;
        cloned_state->resolved = state_->resolved;
        cloned_state->type = state_->type;
        cloned_state->dynamic = state_->dynamic;
        cloned_state->minimum = state_->minimum;
        cloned_state->maximum = state_->maximum;

        if(const matrix * stored_matrix = matrix_value())
        {
            matrix copied_matrix;
            copied_matrix.copy(*stored_matrix);
            cloned_state->value = copied_matrix;
        }
        else
            cloned_state->value = state_->value;

        return cloned_state;
    }


    void
    parameter::bind_to(const parameter & p)
    {
        state_ = p.state_;
    }



    double 
    parameter::operator=(double v)
    {
        if(state_->has_options)
        {
            state_->value = clamp_option_index(checked_truncating_int(std::round(v), "option index"),
                                               state_->options);
            state_->resolved = true;
            return v;
        }

        switch(state_->type)
        {
            case number_type:
            case rate_type:
                validate_numeric_value(v);
                state_->value = double(v);
                break;
            case bool_type:
                state_->value = (v != 0.0);
                break;
            case string_type:
                state_->value = formatNumber(v);
                break;
            case matrix_type:
                set_matrix(scalar_parameter_matrix(v));
                break;
            default:
                throw exception("Invalid parameter type for numeric assignment.");
        }
        state_->resolved = true;
        return v;
    }

    std::string 
    parameter::operator=(std::string v)
    {
        double val = 0;
        bool has_numeric_value = false;
        if(state_->has_options)
        {
            auto it = std::find(state_->options.begin(), state_->options.end(), v);
            if(it != state_->options.end())
                state_->value = int(std::distance(state_->options.begin(), it));
            else if(is_number(v))
                state_->value = clamp_option_index(
                    checked_truncating_int(std::round(kernel_detail::parse_parameter_number(v, "option index")), "option index"),
                    state_->options
                );
            else
                throw exception("Invalid parameter value");

            state_->resolved = true;
            return v;
        }
        else if(is_number(v))
        {
            val = kernel_detail::parse_parameter_number(v, "number");
            has_numeric_value = true;
        }

        switch(state_->type)
        {
            case number_type:
            case rate_type:
                if(!has_numeric_value)
                    throw exception("Invalid numeric parameter value \"" + v + "\".");
                validate_numeric_value(val);
                state_->value = val;
                break;

            case bool_type:
            {
                bool bool_value = false;
                if(!parse_bool(v, bool_value))
                    throw exception("Invalid boolean parameter value \"" + v + "\".");
                state_->value = bool_value;
                break;
            }

            case string_type:
                state_->value = v;
                break;

            case matrix_type:
                set_matrix(matrix(v));
                break;

            default:
                throw exception("Invalid parameter type for string assignment.");
        }
        state_->resolved = true;
        return v;
    }


    void
    parameter::validate_numeric_value(double numeric_value) const
    {
        if(!state_->minimum && !state_->maximum)
            return;
        if(!std::isfinite(numeric_value))
            throw exception("Numeric parameter value must be finite when constraints are declared.");
        if(state_->minimum && numeric_value < *state_->minimum)
            throw exception("Numeric parameter value " + formatNumber(numeric_value) +
                            " is below minimum " + formatNumber(*state_->minimum) + ".");
        if(state_->maximum && numeric_value > *state_->maximum)
            throw exception("Numeric parameter value " + formatNumber(numeric_value) +
                            " is above maximum " + formatNumber(*state_->maximum) + ".");
    }


    matrix *
    parameter::matrix_value() noexcept
    {
        return std::get_if<matrix>(&state_->value);
    }


    const matrix *
    parameter::matrix_value() const noexcept
    {
        return std::get_if<matrix>(&state_->value);
    }


    void
    parameter::set_matrix(const matrix & v)
    {
        if(state_->type != matrix_type)
            throw exception("Invalid parameter value");

        matrix * stored_matrix = matrix_value();
        if(!stored_matrix)
            throw exception("Matrix parameter does not contain matrix storage.");

        matrix replacement;
        replacement.copy(v);
        const bool shape_changed = stored_matrix->shape() != replacement.shape() ||
                                   stored_matrix->size() != replacement.size();
        if(state_->resolved && shape_changed && !state_->dynamic)
            throw exception("Matrix parameter shape cannot change after startup from " +
                            matrix::format_shape(stored_matrix->shape()) + " to " +
                            matrix::format_shape(replacement.shape()) + ".");

        if(shape_changed)
            stored_matrix->realloc(replacement.shape());
        stored_matrix->copy(replacement);
        state_->resolved = true;
    }


    matrix &
    parameter::matrix_ref()
    {
        if(auto stored_matrix = matrix_value())
            return *stored_matrix;
        throw exception("Not a matrix value.");
    }


    const matrix &
    parameter::matrix_ref() const
    {
        if(auto stored_matrix = matrix_value())
            return *stored_matrix;
        throw exception("Not a matrix value.");
    }


    matrix
    parameter::as_matrix() const
    {
        matrix copied_matrix;
        copied_matrix.copy(matrix_ref());
        return copied_matrix;
    }


    int
    parameter::size() const
    {
        if(auto stored_matrix = matrix_value())
            return stored_matrix->size();
        throw exception("Not a matrix value.");
    }


    float
    parameter::get(int index, float default_value) const
    {
        if(auto stored_matrix = matrix_value())
        {
            if(index < 0 || index >= stored_matrix->size())
                return default_value;
            const int block_size = stored_matrix->logical_block_size();
            return stored_matrix->logical_block_data(index / block_size)[index % block_size];
        }
        throw exception("Not a matrix value.");
    }


    float
    parameter::operator[](int index) const
    {
        if(auto stored_matrix = matrix_value())
        {
            if(index < 0 || index >= stored_matrix->size())
                throw std::out_of_range("Parameter matrix index out of range.");
            const int block_size = stored_matrix->logical_block_size();
            return stored_matrix->logical_block_data(index / block_size)[index % block_size];
        }
        throw exception("Not a matrix value.");
    }


    parameter::operator std::string() const
    {
        if(state_->has_options)
        {
            auto option_index = std::get_if<int>(&state_->value);
            if(!option_index)
                throw exception("Option parameter missing index value.");
            int index = *option_index;
            if(index < 0 || static_cast<std::size_t>(index) >= state_->options.size())
                return std::to_string(index)+" (OUT-OF-RANGE)";
            else
                return state_->options[index];
        } 

        switch(state_->type)
        {
            case no_type: throw exception("Uninitialized or unbound parameter.");
            case number_type:
            case rate_type:
                if(auto number_value = std::get_if<double>(&state_->value))
                    return formatNumber(*number_value);
                break;
            case bool_type:
                if(auto bool_value = std::get_if<bool>(&state_->value))
                    return (*bool_value ? "true" : "false");
                break;
            case string_type:
                if(auto string_value = std::get_if<std::string>(&state_->value))
                    return *string_value;
                break;
            case matrix_type:
                if(auto stored_matrix = matrix_value())
                    return stored_matrix->json();
                break;
            default:
                break;
        }
        throw exception("Type conversion error for parameter.");
    }


    parameter::operator double() const
    {
        if(state_->has_options)
        {
            if(auto option_index = std::get_if<int>(&state_->value))
                return *option_index;
            throw exception("Option parameter missing index value.");
        }

        if(state_->type == rate_type)
        {
            if(auto number_value = std::get_if<double>(&state_->value))
                return *number_value * kernel().GetTickDuration();
        }
        if(auto number_value = std::get_if<double>(&state_->value))
            return *number_value;
        else if(auto bool_value = std::get_if<bool>(&state_->value))
            return *bool_value ? 1.0 : 0.0;
        else if(auto string_value = std::get_if<std::string>(&state_->value))
            return kernel_detail::parse_parameter_number(*string_value, "double");
        else if(auto stored_matrix = matrix_value())
            return get_scalar_matrix_value(*stored_matrix, "double");
        else
            throw exception("Type conversion error. Parameter does not have a type Check spelling IKC and cc file.");
    }


    parameter::operator bool() const
    {
        return as_bool();
    }


    bool
    parameter::as_bool() const
    {
        if(state_->has_options)
            return as_int() != 0;
        if(state_->type == bool_type)
        {
            if(auto bool_value = std::get_if<bool>(&state_->value))
                return *bool_value;
        }
        if(state_->type == string_type)
        {
            if(auto string_value = std::get_if<std::string>(&state_->value))
                return is_true(*string_value);
        }
        return as_double() != 0;
    }


    float
    parameter::as_float() const
    {
        return float(as_double());
    }


    double
    parameter::as_double() const
    {
        return double(*this);
    }


    long
    parameter::as_long() const
    {
        if(state_->has_options)
        {
            if(auto option_index = std::get_if<int>(&state_->value))
                return static_cast<long>(*option_index);
            throw exception("Option parameter missing index value.");
        }

        switch(state_->type)
        {
            case no_type: throw exception("Uninitialized_parameter.");
            case number_type:
            case rate_type:
                if(auto number_value = std::get_if<double>(&state_->value))
                    return checked_truncating_long(
                        state_->type == rate_type ? *number_value * kernel().GetTickDuration() : *number_value,
                        "long"
                    );
                break;
            case bool_type:
                if(auto bool_value = std::get_if<bool>(&state_->value))
                    return *bool_value ? 1L : 0L;
                break;
            case string_type:
                if(auto string_value = std::get_if<std::string>(&state_->value))
                    return checked_truncating_long(kernel_detail::parse_parameter_number(*string_value, "long"), "long");
                break;
            case matrix_type:
                if(auto stored_matrix = matrix_value())
                    return checked_truncating_long(get_scalar_matrix_value(*stored_matrix, "long"), "long");
                throw exception("Could not convert matrix to long");
            default: ;
        }
        throw exception("Type conversion error for parameter");
    }


    int
    parameter::as_int() const
    {
        if(state_->has_options)
        {
            if(auto option_index = std::get_if<int>(&state_->value))
                return *option_index;
            throw exception("Option parameter missing index value.");
        }

        switch(state_->type)
        {
            case no_type: throw exception("Uninitialized_parameter.");
            case number_type:
            case rate_type:
                if(auto number_value = std::get_if<double>(&state_->value))
                    return checked_truncating_int(
                        state_->type == rate_type ? *number_value * kernel().GetTickDuration() : *number_value,
                        "int"
                    );
                break;
            case bool_type:
                if(auto bool_value = std::get_if<bool>(&state_->value))
                    return *bool_value ? 1 : 0;
                break;
            case string_type:
                if(auto string_value = std::get_if<std::string>(&state_->value))
                    return checked_truncating_int(kernel_detail::parse_parameter_number(*string_value, "int"), "int");
                break;
            case matrix_type:
                if(auto stored_matrix = matrix_value())
                    return checked_truncating_int(get_scalar_matrix_value(*stored_matrix, "int"), "int");
                throw exception("Could not convert matrix to int");
            default: ;
        }
        throw exception("Type conversion error for  parameter");
    }


    std::string
    parameter::as_int_string() const
    {
        return std::to_string(as_int());    
    }


    parameter_type
    parameter::get_type() const noexcept
    {
        return state_->type;
    }


    bool
    parameter::has_options() const noexcept
    {
        return state_->has_options;
    }


    bool
    parameter::is_resolved() const noexcept
    {
        return state_->resolved;
    }


    std::vector<std::string>
    parameter::options() const
    {
        return state_->options;
    }


    dictionary
    parameter::metadata() const
    {
        return state_->info.copy();
    }


    void
    parameter::set_source_value(const std::string & source_value)
    {
        state_->info["value"] = source_value;
    }


    std::string
    parameter::as_string() const
    {
        return std::string(*this);
    }



    bool
    parameter::empty() const
    {
        return (*this).as_string().empty();
    }



    void
    parameter::print(std::string name) const
    {
        const dictionary & metadata = state_->info;
        if(name.empty() && metadata.contains_non_null("name"))
            name = std::string(metadata.at("name"));

        if(!name.empty())
            std::cout << name << " = ";
        if(state_->type == no_type)
            std::cout << "not initialized\n";
        else if(!state_->resolved)
            std::cout << "unresolved\n";
        else
            std::cout << as_string() << '\n';
    }


    void
    parameter::info() const
    {
        const dictionary & metadata = state_->info;
        const auto metadata_value = [&](const std::string & key, const std::string & fallback)
        {
            return metadata.contains_non_null(key) ? std::string(metadata.at(key)) : fallback;
        };

        const std::size_t type_index = static_cast<std::size_t>(state_->type);
        const std::string type_name = type_index < parameter_strings.size() ?
                                      std::string(parameter_strings[type_index]) : "unknown";

        std::cout << "name: " << metadata_value("name", "(unnamed)") << '\n';
        std::cout << "type: " << type_name << '\n';
        std::cout << "resolved: " << (state_->resolved ? "true" : "false") << '\n';
        std::cout << "default: " << metadata_value("default", "(none)") << '\n';
        std::cout << "source: " << metadata_value("value", "(none)") << '\n';
        std::cout << "has_options: " << (state_->has_options ? "true" : "false") << '\n';
        std::cout << "options: ";
        if(state_->options.empty())
            std::cout << "(none)";
        else
        {
            std::string separator;
            for(const std::string & option : state_->options)
            {
                std::cout << separator << option;
                separator = ", ";
            }
        }
        std::cout << '\n';
        std::cout << "minimum: " << (state_->minimum ? formatNumber(*state_->minimum) : "(none)") << '\n';
        std::cout << "maximum: " << (state_->maximum ? formatNumber(*state_->maximum) : "(none)") << '\n';
        std::cout << "value: " << (state_->resolved ? as_string() : "unresolved") << '\n';
    }

    std::string 
    parameter::json() const
    {
        if(state_->has_options)
        {
            if(state_->type == number_type || state_->type == rate_type)
                return "[["+format_json_number(as_double())+"]]";
            if(state_->type == bool_type)
                return (as_bool() ? "[[true]]" : "[[false]]");
            if(state_->type == string_type)
                return "\""+escape_json_string(as_string())+"\"";
            throw exception("Cannot convert parameter to string");
        }

        if((state_->type == number_type || state_->type == rate_type) &&
           std::holds_alternative<double>(state_->value))
            return "[["+format_json_number(std::get<double>(state_->value))+"]]";
        if(state_->type == bool_type && std::holds_alternative<bool>(state_->value))
            return (std::get<bool>(state_->value) ? "[[true]]" : "[[false]]");
        if(state_->type == string_type && std::holds_alternative<std::string>(state_->value))
            return "\""+escape_json_string(std::get<std::string>(state_->value))+"\"";
        if(state_->type == matrix_type)
            if(auto stored_matrix = matrix_value())
                return stored_matrix->json();
        throw exception("Cannot convert parameter to string");
    }


    std::ostream& operator<<(std::ostream& os, const parameter & p)
    {
        os << p.as_string();
        return os;
    }

}
