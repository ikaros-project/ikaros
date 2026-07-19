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

    void test_numeric_precision_and_rates()
    {
        parameter precisionCopy;
        parameter precisionSource;
        parameter rateCopy;
        parameter rateExpression;
        parameter rateSource;
        Bind(precisionCopy, "precision_copy");
        Bind(precisionSource, "precision_source");
        Bind(rateCopy, "rate_copy");
        Bind(rateExpression, "rate_expression");
        Bind(rateSource, "rate_source");

        require_true(precisionSource.as_double() == 0.12345678901234566,
                     "numeric resolution should preserve double precision");
        require_true(precisionCopy.as_double() == precisionSource.as_double(),
                     "numeric indirection should preserve double precision");
        require_true(parse_double(precisionSource.as_string()) == precisionSource.as_double(),
                     "numeric string conversion should round-trip exactly");
        require_true(rateExpression.as_double() == 0.125 * GetTickDuration(),
                     "rate parameters should evaluate plain arithmetic expressions");
        require_true(parse_double(rateCopy.as_string()) == 0.5 * parse_double(rateSource.as_string()),
                     "rate expressions should preserve source precision");
    }

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


    void test_boolean_assignment()
    {
        parameter value("bool");
        value = std::string("true");
        require_true(value.as_bool(), "boolean assignment should accept true");
        value = std::string("off");
        require_true(!value.as_bool(), "boolean assignment should accept recognized false values");
        value = std::string("1");
        require_true(value.as_bool(), "boolean assignment should accept one");

        require_throws_as<exception>([&]() { value = std::string("truthy"); },
                                     "boolean assignment should reject unknown text");
        require_true(value.as_bool(), "rejected boolean text should leave the value unchanged");
        require_throws_as<exception>([&]() { value = std::string("2"); },
                                     "boolean assignment should reject numeric values other than zero and one");
        require_true(value.as_bool(), "rejected numeric boolean text should leave the value unchanged");
    }


    void test_constraints_and_option_cache()
    {
        parameter boundedBinding;
        Bind(boundedBinding, "bounded_value");
        SetParameter("bounded_value", std::string("0.75"));
        require_true(boundedBinding.as_double() == 0.75,
                     "kernel parameter updates should accept values within constraints");
        require_throws_as<exception>([&]() { SetParameter("bounded_value", std::string("1.1")); },
                                     "kernel parameter updates should enforce constraints");
        require_true(boundedBinding.as_double() == 0.75,
                     "a rejected kernel parameter update should leave the value unchanged");

        parameter bounded(dictionary({
            {"type", "number"},
            {"name", "bounded"},
            {"min", "-1.5"},
            {"max", "2.5"}
        }));
        bounded = -1.5;
        bounded = 2.5;
        require_throws_as<exception>([&]() { bounded = -1.5001; },
                                     "numeric assignment should enforce the minimum");
        require_true(bounded.as_double() == 2.5, "a rejected minimum violation should leave the value unchanged");
        require_throws_as<exception>([&]() { bounded = std::string("2.5001"); },
                                     "string assignment should enforce the maximum");
        require_true(bounded.as_double() == 2.5, "a rejected maximum violation should leave the value unchanged");
        require_throws_as<exception>([&]() { bounded = std::numeric_limits<double>::quiet_NaN(); },
                                     "constrained parameters should reject non-finite values");

        require_throws_as<exception>([]()
        {
            parameter invalid(dictionary({
                {"type", "number"},
                {"min", "2"},
                {"max", "1"}
            }));
            static_cast<void>(invalid);
        }, "parameter construction should reject reversed constraints");

        parameter option("string", "alpha, beta, gamma");
        require_true(option.has_options() && option.options().size() == 3,
                     "option metadata should be parsed once into the parameter");
        require_true(option.options()[0] == "alpha" && option.options()[1] == "beta" &&
                     option.options()[2] == "gamma", "cached options should be trimmed");
        option = std::string("beta");
        require_true(option.as_string() == "beta", "option assignment should use the cached options");
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
        test_numeric_precision_and_rates();
        test_matrix_indexing();
        test_boolean_assignment();
        test_constraints_and_option_cache();
        test_matrix_update_contract();
        test_integral_conversions();
        test_option_indices();
        test_compute_int();
        std::cout << "PARAMETER SAFETY TEST OK" << std::endl;
    }
};

INSTALL_CLASS(ParameterTestModule)
