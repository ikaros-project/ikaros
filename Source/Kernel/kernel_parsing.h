// Ikaros 3.0

#pragma once

#include <string>

namespace ikaros::kernel_detail
{
    double parse_parameter_number(const std::string & value,
                                  const std::string & conversion_name);
    int parse_strict_int(const std::string & value);
}
