#include <iostream>
#include <stdexcept>
#include <string>

#include "ikaros.h"

using namespace ikaros;

namespace
{
    void
    require(bool condition, const std::string & message)
    {
        if(!condition)
            throw std::runtime_error("UtilitiesTestModule: " + message);
    }
}


class UtilitiesTestModule : public Module
{
public:
    void
    Init() override
    {
        double double_value = 0;
        require(parse_double("+1.25", double_value) && double_value == 1.25,
                "parse_double rejected a leading plus sign");
        require(parse_double("-1.25", double_value) && double_value == -1.25,
                "parse_double rejected a leading minus sign");

        double_value = 17;
        require(!parse_double("--1.25", double_value) && double_value == 17,
                "parse_double accepted a doubled minus sign");
        require(!parse_double("+-1.25", double_value) && double_value == 17,
                "parse_double accepted mixed signs");

        float float_value = 0;
        require(parse_float("+1.25", float_value) && float_value == 1.25f,
                "parse_float rejected a leading plus sign");
        float_value = 17;
        require(!parse_float("--1.25", float_value) && float_value == 17,
                "parse_float accepted a doubled minus sign");
        require(!parse_float("+-1.25", float_value) && float_value == 17,
                "parse_float accepted mixed signs");

        std::cout << "UTILITIES TEST OK\n";
    }
};

INSTALL_CLASS(UtilitiesTestModule)
