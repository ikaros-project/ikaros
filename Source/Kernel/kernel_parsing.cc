// Ikaros 3.0

#include "ikaros.h"
#include "kernel_parsing.h"

#include <charconv>

namespace ikaros::kernel_detail
{
    double
    parse_parameter_number(const std::string & value,
                           const std::string & conversion_name)
    {
        try
        {
            return parse_double(value);
        }
        catch(const std::invalid_argument &)
        {
            throw exception("Could not convert string \"" + value + "\" to " + conversion_name + ".");
        }
        catch(const std::out_of_range &)
        {
            throw exception("String \"" + value + "\" is out of range for " + conversion_name + ".");
        }
    }


    int
    parse_strict_int(const std::string & value)
    {
        const std::string trimmed_value = trim(value);
        if(trimmed_value.empty())
            throw std::invalid_argument("Expected an integer.");

        int parsed_value = 0;
        const char * begin = trimmed_value.data();
        const char * end = begin + trimmed_value.size();
        const auto result = std::from_chars(begin, end, parsed_value);
        if(result.ec == std::errc::result_out_of_range)
            throw std::out_of_range("Integer is outside the supported range.");
        if(result.ec != std::errc() || result.ptr != end)
            throw std::invalid_argument("Expected an integer.");
        return parsed_value;
    }
}
