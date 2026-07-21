#include <array>
#include <functional>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

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


    std::string
    capture_standard_output(const std::function<void()> & function)
    {
        std::ostringstream output;
        std::streambuf * previous = std::cout.rdbuf(output.rdbuf());
        try
        {
            function();
        }
        catch(...)
        {
            std::cout.rdbuf(previous);
            throw;
        }
        std::cout.rdbuf(previous);
        return output.str();
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

        require(formatNumber(0, 0) == "0",
                "zero-decimal formatting corrupted zero");
        require(formatNumber(10, 0) == "10" && formatNumber(-20, 0) == "-20",
                "zero-decimal formatting stripped integer zeros");
        require(formatNumber(1.25, 4) == "1.25" && formatNumber(0, 4) == "0",
                "fixed-decimal formatting did not trim only fractional zeros");

        const std::array<unsigned char, 3> man = {'M', 'a', 'n'};
        require(base64_encode(nullptr, 0).empty(),
                "Base64 encoding did not accept empty input");
        require(base64_encode(man.data(), 1) == "TQ==" &&
                base64_encode(man.data(), 2) == "TWE=" &&
                base64_encode(man.data(), 3) == "TWFu",
                "Base64 encoding produced an incorrect value");

        bool rejected_null_base64 = false;
        try
        {
            static_cast<void>(base64_encode(nullptr, 1));
        }
        catch(const std::invalid_argument &)
        {
            rejected_null_base64 = true;
        }
        require(rejected_null_base64, "Base64 encoding accepted null non-empty input");

        bool rejected_oversized_base64 = false;
        try
        {
            static_cast<void>(base64_encode(man.data(), std::numeric_limits<size_t>::max()));
        }
        catch(const std::length_error &)
        {
            rejected_oversized_base64 = true;
        }
        require(rejected_oversized_base64, "Base64 encoding accepted an impossible input size");

        std::ostringstream vector_stream;
        vector_stream << std::vector<int>{1, 2, 3};
        require(vector_stream.str() == "(1, 2, 3)",
                "vector insertion ignored its destination stream");

        std::string cut_value = "alpha::beta";
        require(cut_head(cut_value, "::") == "alpha" && cut_value == "beta",
                "cut_head did not consume the head and delimiter");
        require(cut_head(cut_value, "::") == "beta" && cut_value.empty(),
                "cut_head did not consume an undelimited remainder");

        require(capture_standard_output([]()
                {
                    print_attribute_value("value", 3, 2);
                }) == "      value = 3\n",
                "attribute printing ignored indentation");
        require(capture_standard_output([]()
                {
                    print_attribute_value("values", std::vector<float>{1, 2}, 0, 0);
                }) == "values = 1 2 \n",
                "unlimited float attribute printing added an ellipsis");
        require(capture_standard_output([]()
                {
                    print_attribute_value("values", std::vector<int>{1, 2}, 0, 2);
                }) == "values = 1 2 \n",
                "exactly limited attribute printing added an ellipsis");
        require(capture_standard_output([]()
                {
                    print_attribute_value("values", std::vector<float>{1, 2, 3}, 0, 2);
                }) == "values = 1 2 ...\n",
                "limited attribute printing did not mark omitted values");
        require(capture_standard_output([]()
                {
                    print_attribute_value("labels",
                                          std::vector<std::vector<std::string>>{{"a"}, {"b"}},
                                          1, 1);
                }) == "   labels = \n      a \n      ...\n\n",
                "nested attribute printing ignored indentation or limits");

        std::cout << "UTILITIES TEST OK\n";
    }
};

INSTALL_CLASS(UtilitiesTestModule)
