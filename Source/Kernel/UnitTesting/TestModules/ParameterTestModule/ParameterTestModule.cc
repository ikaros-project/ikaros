#include <limits>
#include <stdexcept>

#include "ikaros.h"

using namespace ikaros;

namespace
{
void
require_true(bool condition, const std::string & message)
{
    if(!condition)
        throw exception("ParameterTestModule: " + message);
}


template<typename ExceptionType, typename Function>
void
require_throws_as(Function function, const std::string & message)
{
    try
    {
        function();
    }
    catch(const ExceptionType &)
    {
        return;
    }
    catch(const std::exception & e)
    {
        throw exception("ParameterTestModule: " + message + " (unexpected exception: " + e.what() + ")");
    }

    throw exception("ParameterTestModule: " + message + " (expected exception)");
}
}

class ParameterTestModule : public Module
{
    matrix matrixValue;

    void test_matrix_indexing()
    {
        parameter values("matrix");
        values = std::string("10, 20, 30");

        require_true(values.get(0, -1.0f) == 10.0f, "get() should return a valid element");
        require_true(values.get(-1, -7.0f) == -7.0f, "get() should reject a negative index");
        require_true(values.get(3, -8.0f) == -8.0f, "get() should reject an oversized index");
        require_true(values[2] == 30.0f, "operator[] should return a valid element");
        require_throws_as<std::out_of_range>([&]() { static_cast<void>(values[-1]); },
                                             "operator[] should reject a negative index");
        require_throws_as<std::out_of_range>([&]() { static_cast<void>(values[3]); },
                                             "operator[] should reject an oversized index");
    }


    void test_matrix_update_contract()
    {
        Bind(matrixValue, "matrix_value");
        float * storage = matrixValue.data();

        SetParameter("matrix_value", matrix("5, 6; 7, 8"));
        require_true(matrixValue.data() == storage, "same-shaped matrix replacement should preserve bound storage");
        require_true(matrixValue(0, 0) == 5.0f && matrixValue(1, 1) == 8.0f,
                     "direct matrix binding should observe programmatic replacement");

        SetParameter("matrix_value", std::string("9, 10; 11, 12"));
        require_true(matrixValue.data() == storage, "string replacement should preserve bound storage");
        require_true(matrixValue(0, 1) == 10.0f && matrixValue(1, 0) == 11.0f,
                     "direct matrix binding should observe string replacement");

        matrix rowGapped(2, 4);
        rowGapped.resize(2, 2);
        rowGapped(0, 0) = 13.0f;
        rowGapped(0, 1) = 14.0f;
        rowGapped(1, 0) = 15.0f;
        rowGapped(1, 1) = 16.0f;
        SetParameter("matrix_value", rowGapped);
        require_true(matrixValue.data() == storage, "row-gapped source should preserve bound storage");
        require_true(matrixValue(0, 0) == 13.0f && matrixValue(0, 1) == 14.0f &&
                     matrixValue(1, 0) == 15.0f && matrixValue(1, 1) == 16.0f,
                     "row-gapped source should be copied in logical order");

        matrix alias = matrixValue;
        SetParameter("matrix_value", alias);
        require_true(matrixValue.data() == storage && matrixValue(1, 1) == 16.0f,
                     "aliased replacement should preserve the parameter value");

        require_throws_as<exception>(
            [&]() { SetParameter("matrix_value", matrix("1, 2, 3")); },
            "matrix replacement should reject a different element count"
        );
        require_throws_as<exception>(
            [&]() { SetParameter("matrix_value", matrix("1, 2, 3, 4")); },
            "matrix replacement should reject a different shape with the same element count"
        );
        require_throws_as<exception>(
            [&]() { SetParameter("matrix_value", std::string("1, 2, 3")); },
            "string replacement should reject a different shape"
        );
        require_true(matrixValue.data() == storage && matrixValue(0, 0) == 13.0f && matrixValue(1, 1) == 16.0f,
                     "rejected replacement should leave the bound matrix unchanged");

        parameter scalar("matrix");
        scalar = 1.0;
        matrix scalarBinding = static_cast<matrix &>(scalar);
        float * scalarStorage = scalarBinding.data();
        scalar = 2.0;
        require_true(scalarBinding.data() == scalarStorage && scalarBinding(0) == 2.0f,
                     "numeric matrix assignment should preserve bound storage");
        require_throws_as<exception>([&]() { scalar = std::string("1, 2"); },
                                     "numeric matrix assignment should establish a fixed shape");
    }


    void test_integral_conversions()
    {
        parameter number("number");
        number = 12.9;
        require_true(number.as_int() == 12, "as_int() should truncate positive values");
        require_true(number.as_long() == 12L, "as_long() should truncate positive values");
        number = -12.9;
        require_true(number.as_int() == -12, "as_int() should truncate negative values toward zero");
        require_true(number.as_long() == -12L, "as_long() should truncate negative values toward zero");

        number = static_cast<double>(std::numeric_limits<int>::max());
        require_true(number.as_int() == std::numeric_limits<int>::max(), "as_int() should accept INT_MAX");
        number = static_cast<double>(std::numeric_limits<int>::lowest());
        require_true(number.as_int() == std::numeric_limits<int>::lowest(), "as_int() should accept INT_MIN");

        number = std::numeric_limits<double>::quiet_NaN();
        require_throws_as<std::out_of_range>([&]() { static_cast<void>(number.as_int()); },
                                             "as_int() should reject NaN");
        number = std::numeric_limits<double>::infinity();
        require_throws_as<std::out_of_range>([&]() { static_cast<void>(number.as_long()); },
                                             "as_long() should reject infinity");
        number = std::numeric_limits<double>::max();
        require_throws_as<std::out_of_range>([&]() { static_cast<void>(number.as_int()); },
                                             "as_int() should reject oversized values");
        require_throws_as<std::out_of_range>([&]() { static_cast<void>(number.as_long()); },
                                             "as_long() should reject oversized values");

        parameter text("string");
        text = std::string("1e100");
        require_throws_as<std::out_of_range>([&]() { static_cast<void>(text.as_int()); },
                                             "string-to-int conversion should reject oversized values");

        parameter scalar("matrix");
        scalar = std::numeric_limits<double>::infinity();
        require_throws_as<std::out_of_range>([&]() { static_cast<void>(scalar.as_int()); },
                                             "matrix-to-int conversion should reject infinity");
    }


    void test_option_indices()
    {
        parameter option("number", "A,B,C");
        option = 1.6;
        require_true(option.as_int() == 2, "numeric option assignment should round to the nearest index");
        option = std::string("1.6");
        require_true(option.as_int() == 2, "string option assignment should use the same rounding rule");
        option = std::string("-1.6");
        require_true(option.as_int() == 0, "negative option indices should clamp after conversion");

        require_throws_as<std::out_of_range>(
            [&]() { option = std::numeric_limits<double>::infinity(); },
            "option assignment should reject infinity"
        );
        require_throws_as<std::out_of_range>(
            [&]() { option = std::string("1e100"); },
            "string option assignment should reject oversized indices"
        );
    }


    void test_compute_int()
    {
        require_true(ComputeInt("12.9") == 12, "ComputeInt() should preserve truncation");
        require_true(ComputeInt("-12.9") == -12, "ComputeInt() should truncate toward zero");
        require_throws_as<std::out_of_range>([&]() { static_cast<void>(ComputeInt("1e100")); },
                                             "ComputeInt() should reject oversized values");
    }


    void Init() override
    {
        test_matrix_indexing();
        test_matrix_update_contract();
        test_integral_conversions();
        test_option_indices();
        test_compute_int();
        std::cout << "PARAMETER SAFETY TEST OK" << std::endl;
    }
};

INSTALL_CLASS(ParameterTestModule)
