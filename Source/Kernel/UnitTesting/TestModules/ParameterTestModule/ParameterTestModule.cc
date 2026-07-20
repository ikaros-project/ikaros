#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <type_traits>

#include "ikaros.h"

using namespace ikaros;

static_assert(!std::is_convertible_v<parameter &, matrix &>);
static_assert(!std::is_convertible_v<const parameter &, const matrix &>);

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


template<typename Function>
std::string
capture_stdout(Function function)
{
    std::ostringstream output;
    std::streambuf * previous_buffer = std::cout.rdbuf(output.rdbuf());
    try
    {
        function();
    }
    catch(...)
    {
        std::cout.rdbuf(previous_buffer);
        throw;
    }
    std::cout.rdbuf(previous_buffer);
    return output.str();
}
}

class ParameterTestModule : public Module
{
    matrix matrixValue;
    matrix prefixShapeTest;

    void test_expression_and_compute_engine()
    {
        require_true(ComputeDouble("8/4*2") == 4.0,
                     "multiplication and division should be left-associative");
        require_true(ComputeDouble("8*4/2") == 16.0,
                     "mixed multiplication and division should preserve source order");
        require_true(ComputeDouble("1e-3*2") == 0.002,
                     "scientific notation should work inside arithmetic");
        require_true(ComputeDouble("1--2") == 3.0,
                     "binary and unary minus should compose correctly");

        require_true(ComputeDouble("1e-3*@precision_source") == 1e-3 * 0.12345678901234566,
                     "scientific notation should not introduce a variable named e");

        for(const std::string & invalid : {"", "()", "1+", "+", "1*", "*1"})
            require_throws_as<exception>([&]() { static_cast<void>(ComputeDouble(invalid)); },
                                         "malformed arithmetic should be rejected: " + invalid);

        require_true(ComputeDouble("3-1") == 2.0,
                     "ComputeDouble() should recognize subtraction without surrounding spaces");
        require_true(ComputeDouble("(1+2)") == 3.0,
                     "ComputeDouble() should recognize fully parenthesized arithmetic");
        require_true(ComputeDouble("1e-3*2") == 0.002,
                     "ComputeDouble() should evaluate scientific notation inside arithmetic");
        require_true(ComputeDouble("@precision_source-0.1") == 0.12345678901234566 - 0.1,
                     "ComputeDouble() should recognize subtraction after parameter indirection");
        require_throws_as<exception>([&]() { static_cast<void>(ComputeDouble("1+")); },
                                     "ComputeDouble() should reject a missing operand");
        require_throws_as<exception>([&]() { static_cast<void>(ComputeValue("bare+1")); },
                                     "bare variables in arithmetic should still require @ indirection");

        parameter dottedValue;
        parameter inheritedDottedValue;
        Bind(dottedValue, "dotted_value");
        Bind(inheritedDottedValue, "inherited_dotted_value");
        require_true(dottedValue.as_string() == "file.name.txt" &&
                     ComputeValue("@dotted_source") == "file.name.txt" &&
                     inheritedDottedValue.as_string() == "server.example.org",
                     "parameter indirection should preserve dotted literal strings");

        parameter liveComputeValue;
        Bind(liveComputeValue, "live_compute_value");
        require_true(ComputeDouble("@live_compute_value") == 2.0,
                     "parameter indirection should read an explicitly configured value");
        SetParameter("live_compute_value", std::string("5"));
        require_true(liveComputeValue.as_double() == 5.0 && ComputeDouble("@live_compute_value") == 5.0,
                     "parameter indirection should read the current value after an update");
        SetParameter("dotted_source", std::string());
        require_true(ComputeValue("@dotted_source").empty(),
                     "an empty current parameter value should not fall back to its model attribute");

        const std::string matrixShape = ComputeValue("matrix_value.shape");
        require_throws_as<exception>([&]()
        {
            static_cast<void>(ComputeValue("matrix_value.shape[999999999999999999999999999999]"));
        }, "shape selectors should reject indices that overflow size_t");
        require_true(ComputeValue("matrix_value.shape[999999999999999999:]").empty(),
                     "large representable shape slice starts should clamp to the rank");
        require_true(ComputeValue("matrix_value.shape[:999999999999999999]") == matrixShape,
                     "large representable shape slice ends should clamp to the rank");
        require_throws_as<exception>([&]()
        {
            static_cast<void>(ComputeValue("matrix_value.shape[:999999999999999999999999999999]"));
        }, "shape slices should reject endpoints that overflow size_t");

        std::string integratedShapeSource = "matrix_value.shape[0]+1,matrix_value.shape[1]";
        const std::vector<int> integratedShape = EvaluateShapeList(integratedShapeSource);
        require_true(integratedShape == std::vector<int>({3, 2}),
                     "shape lists should use the integrated arithmetic evaluator");
        std::string overflowingShapeSource = "matrix_value.shape[999999999999999999999999999999]";
        require_throws_as<exception>([&]() { static_cast<void>(EvaluateShapeList(overflowingShapeSource)); },
                                     "shape-list selectors should reject indices that overflow size_t");

        require_throws_as<exception>([&]() { static_cast<void>(ComputeValue("matrix_value..shape")); },
                                     "compute paths should reject empty path segments");
        require_throws_as<exception>([&]() { static_cast<void>(ComputeValue("matrix_value.shape[0")); },
                                     "compute functions should reject unclosed delimiters");
        require_throws_as<exception>([&]() { static_cast<void>(ComputeValue("1,([)]")); },
                                     "compute lists should reject mismatched delimiters");
        require_throws_as<exception>([&]()
        {
            static_cast<void>(ComputeValue("@missing.@precision_source"));
        }, "computed paths should reject segments that resolve to an empty value");

        bool preservedMatrixBindError = false;
        try
        {
            static_cast<void>(ComputeValue("precision_source.rows"));
        }
        catch(const exception & e)
        {
            preservedMatrixBindError = e.message().find("Not a matrix value") != std::string::npos;
        }
        require_true(preservedMatrixBindError,
                     "matrix functions should preserve errors from binding non-matrix parameters");

        parameter literalPunctuation;
        Bind(literalPunctuation, "literal_punctuation");
        require_true(literalPunctuation.as_string() == "hello (world" &&
                     ComputeValue("@literal_punctuation") == "hello (world",
                     "literal string parameters should not require balanced delimiters");

        Bind(prefixShapeTest, "PREFIX_SHAPE_TEST");
        require_true(prefixShapeTest.rank() == 1 && prefixShapeTest.size() == 5,
                     "shape substitution should distinguish parameter names that share a prefix");
    }

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
        require_true(boundedBinding.metadata().contains_non_null("value") &&
                     std::string(boundedBinding.metadata()["value"]) == "0.75",
                     "bound parameters should share current source metadata");
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
        std::vector<std::string> exposed_options = option.options();
        exposed_options[0] = "changed";
        const std::vector<std::string> current_options = option.options();
        require_true(current_options[0] == "alpha" && current_options[1] == "beta" &&
                     current_options[2] == "gamma", "options() should return an independent copy");
        option = std::string("beta");
        require_true(option.as_string() == "beta", "option assignment should use the cached options");
    }


    void test_debug_output()
    {
        parameter value(dictionary({
            {"name", "gain"},
            {"type", "number"},
            {"default", "1"},
            {"min", "0"},
            {"max", "2"}
        }));

        require_true(capture_stdout([&]() { value.print(); }) == "gain = unresolved\n",
                     "print() should identify unresolved named parameters");

        value = 1.25;
        require_true(capture_stdout([&]() { value.print(); }) == "gain = 1.25\n",
                     "print() should use the declared name and current value");

        const std::string details = capture_stdout([&]() { value.info(); });
        require_true(details.find("name: gain\n") != std::string::npos,
                     "info() should print the parameter name");
        require_true(details.find("type: number\n") != std::string::npos,
                     "info() should print the textual parameter type");
        require_true(details.find("resolved: true\n") != std::string::npos,
                     "info() should print the resolution state");
        require_true(details.find("default: 1\n") != std::string::npos,
                     "info() should print the declared default");
        require_true(details.find("minimum: 0\n") != std::string::npos &&
                     details.find("maximum: 2\n") != std::string::npos,
                     "info() should print numeric constraints");
        require_true(details.find("value: 1.25\n") != std::string::npos,
                     "info() should print the current value");

        parameter option(dictionary({
            {"name", "mode"},
            {"type", "string"},
            {"options", "alpha, beta"}
        }));
        option = std::string("beta");
        const std::string option_details = capture_stdout([&]() { option.info(); });
        require_true(option_details.find("options: alpha, beta\n") != std::string::npos,
                     "info() should print normalized cached options");

        parameter unnamed("number");
        const std::string unnamed_details = capture_stdout([&]() { unnamed.info(); });
        require_true(unnamed_details.find("name: (unnamed)\n") != std::string::npos &&
                     unnamed_details.find("default: (none)\n") != std::string::npos,
                     "info() should label missing metadata clearly");
        require_true(!unnamed.metadata().contains("name") && !unnamed.metadata().contains("default"),
                     "info() should not create missing metadata entries");
    }


    void test_public_state_isolation()
    {
        parameter original(dictionary({
            {"name", "isolated"},
            {"type", "number"},
            {"min", "0"},
            {"max", "1"}
        }));
        original = 0.25;

        parameter constructed = original;
        constructed = 0.75;
        require_true(original.as_double() == 0.25 && constructed.as_double() == 0.75,
                     "copy construction should create independent parameter values");
        require_throws_as<exception>([&]() { constructed = 1.25; },
                                     "copied parameters should preserve independent constraints");

        parameter assigned;
        assigned = original;
        assigned = 0.5;
        require_true(original.as_double() == 0.25 && assigned.as_double() == 0.5,
                     "copy assignment should create independent parameter values");

        dictionary exposed_metadata = original.metadata();
        exposed_metadata["type"] = "bool";
        exposed_metadata["min"] = "-10";
        dictionary current_metadata = original.metadata();
        require_true(original.get_type() == number_type && std::string(current_metadata["type"]) == "number" &&
                     std::string(current_metadata["min"]) == "0",
                     "metadata() should return an independent deep copy");

        parameter matrix_source("matrix");
        matrix_source = std::string("1, 2");
        parameter matrix_copy = matrix_source;
        matrix_source = std::string("3, 4");
        matrix copied_matrix = matrix_copy.as_matrix();
        require_true(copied_matrix(0) == 1.0f && copied_matrix(1) == 2.0f,
                     "parameter copies should own independent matrix storage");

        matrix matrix_snapshot = matrix_source.as_matrix();
        matrix_snapshot(0) = 9.0f;
        matrix current_matrix = matrix_source.as_matrix();
        require_true(current_matrix(0) == 3.0f,
                     "as_matrix() should return an independent matrix snapshot");

        parameter dynamic_matrix(dictionary({
            {"type", "matrix"},
            {"dynamic", "yes"}
        }));
        dynamic_matrix = std::string("1, 2");
        dynamic_matrix = std::string("3, 4; 5, 6");
        matrix dynamic_snapshot = dynamic_matrix.as_matrix();
        require_true(dynamic_snapshot.rows() == 2 && dynamic_snapshot.cols() == 2 && dynamic_snapshot(1, 1) == 6.0f,
                     "explicitly dynamic matrix parameters should allow shape changes");

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
        matrix scalarSnapshot = scalar.as_matrix();
        scalar = 2.0;
        matrix updatedScalar = scalar.as_matrix();
        require_true(scalarSnapshot(0) == 1.0f && updatedScalar(0) == 2.0f,
                     "numeric matrix snapshots should remain independent");
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
        test_expression_and_compute_engine();
        test_numeric_precision_and_rates();
        test_matrix_indexing();
        test_boolean_assignment();
        test_constraints_and_option_cache();
        test_debug_output();
        test_public_state_isolation();
        test_matrix_update_contract();
        test_integral_conversions();
        test_option_indices();
        test_compute_int();
        std::cout << "PARAMETER SAFETY TEST OK" << std::endl;
    }
};

INSTALL_CLASS(ParameterTestModule)
