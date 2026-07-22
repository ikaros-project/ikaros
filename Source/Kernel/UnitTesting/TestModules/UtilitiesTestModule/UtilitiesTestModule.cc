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


    bool
    rejects_invalid_argument(const std::function<void()> & function)
    {
        try
        {
            function();
        }
        catch(const std::invalid_argument &)
        {
            return true;
        }
        return false;
    }


    bool
    rejects_out_of_range(const std::function<void()> & function)
    {
        try
        {
            function();
        }
        catch(const std::out_of_range &)
        {
            return true;
        }
        return false;
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
            static_cast<void>(base64_encode(man.data(), std::numeric_limits<std::size_t>::max()));
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
                    print_attribute_value("name", std::string("value"), 1);
                }) == "   name = value\n",
                "string attribute printing returned incorrect output");
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

        std::string front = "alpha::beta::gamma";
        require(head(front, "::") == "alpha" && front == "beta::gamma",
                "head did not consume the first delimiter");
        front = "alpha::beta::gamma";
        require(tail(front, "::") == "beta::gamma" && front == "alpha",
                "tail did not split at the first delimiter");

        std::string back = "alpha::beta::gamma";
        require(rhead(back, "::") == "alpha::beta" && back == "gamma",
                "rhead did not consume through the last delimiter");
        back = "alpha::beta::gamma";
        require(rtail(back, "::") == "gamma" && back == "alpha::beta",
                "rtail did not split at the last delimiter");

        const std::string unchanged = "alpha::beta::gamma";
        require(peek_head(unchanged, "::") == "alpha" &&
                peek_tail(unchanged, "::") == "beta::gamma" &&
                peek_tail(unchanged, "::", true) == "::beta::gamma" &&
                peek_rhead(unchanged, "::") == "alpha::beta" &&
                peek_rtail(unchanged, "::") == "gamma",
                "non-mutating delimiter helpers returned incorrect segments");

        require(rejects_invalid_argument([]()
                {
                    std::string value = "value";
                    static_cast<void>(head(value, ""));
                }) &&
                rejects_invalid_argument([]()
                {
                    std::string value = "value";
                    static_cast<void>(tail(value, ""));
                }) &&
                rejects_invalid_argument([]()
                {
                    std::string value = "value";
                    static_cast<void>(rhead(value, ""));
                }) &&
                rejects_invalid_argument([]()
                {
                    std::string value = "value";
                    static_cast<void>(rtail(value, ""));
                }) &&
                rejects_invalid_argument([]() { static_cast<void>(peek_head("value", "")); }) &&
                rejects_invalid_argument([]() { static_cast<void>(peek_tail("value", "")); }) &&
                rejects_invalid_argument([]() { static_cast<void>(peek_rhead("value", "")); }) &&
                rejects_invalid_argument([]() { static_cast<void>(peek_rtail("value", "")); }),
                "delimiter helpers did not reject empty delimiters consistently");

        prime prime_number;
        require(prime_number.next() == 2 && prime_number.next() == 3 && prime_number.next() == 5,
                "prime sequence returned incorrect values");
        require(prime_number.test(2147483647L) && !prime_number.test(2147483646L),
                "primality testing failed beyond the safe int multiplication range");
        prime_number.last_prime = std::numeric_limits<long>::max();
        bool rejected_prime_overflow = false;
        try
        {
            static_cast<void>(prime_number.next());
        }
        catch(const std::overflow_error &)
        {
            rejected_prime_overflow = true;
        }
        require(rejected_prime_overflow, "prime sequence overflow was not rejected");

        const std::string non_ascii_bytes = {
            static_cast<char>(0x80),
            static_cast<char>(0xFF),
        };
        require(character_sum("ABC") == 198 && character_sum(non_ascii_bytes) == 383,
                "character sums were not based on unsigned byte values");

        require(split(" alpha, beta ,gamma ", ",") ==
                    std::vector<std::string>{"alpha", "beta", "gamma"},
                "delimiter splitting did not preserve trimmed-token behavior");
        require(split("alpha,beta,gamma", ",", 1) ==
                    std::vector<std::string>{"alpha", "beta,gamma"} &&
                split("alpha,beta", ",", 0) ==
                    std::vector<std::string>{"alpha,beta"},
                "delimiter splitting did not honor its split limit");
        require(split("  alpha  beta gamma  ", "", 1) ==
                    std::vector<std::string>{"alpha", "beta gamma"},
                "whitespace splitting did not honor its split limit");
        require(join(", ", std::vector<std::string>{"alpha", "beta", "gamma"}) ==
                    "alpha, beta, gamma" &&
                join(",", {}).empty(),
                "string joining returned an incorrect value");
        require(replace_characters("1,2;3\xC2\xA0" "4") == "1 2 3 4",
                "character replacement returned an incorrect value");

        require(trim(" \t value \n") == "value" && trim(" \t\n").empty(),
                "string trimming returned an incorrect value");
        require(starts_with("utilities", "util") && !starts_with("utilities", "til") &&
                ends_with("utilities", "ties") && !ends_with("utilities", "utility") &&
                contains("utilities", "lit") && !contains("utilities", "matrix"),
                "string predicates returned incorrect results");
        require(add_extension("model", ".ikg") == "model.ikg" &&
                add_extension("model.ikg", ".ikg") == "model.ikg",
                "filename extension handling returned an incorrect value");
        require(tab(2) == "      ", "indentation generation returned an incorrect value");

        require(is_number(" -1.25e2 ") && !is_number("1.2x"),
                "number recognition returned an incorrect result");
        require(parse_double("1.25") == 1.25 && parse_float("1.25") == 1.25f,
                "throwing numeric parsing returned an incorrect value");
        require(rejects_invalid_argument([]() { static_cast<void>(parse_double("invalid")); }) &&
                rejects_invalid_argument([]() { static_cast<void>(parse_float("invalid")); }),
                "throwing numeric parsing accepted invalid input");
        require(checked_truncating_int(1.9, "int") == 1 &&
                checked_truncating_int(-1.9, "int") == -1 &&
                checked_truncating_long(2.9, "long") == 2,
                "checked truncating conversion returned an incorrect value");
        require(rejects_out_of_range([]()
                {
                    static_cast<void>(checked_truncating_int(
                        std::numeric_limits<double>::infinity(), "int"));
                }) &&
                rejects_out_of_range([]()
                {
                    static_cast<void>(checked_truncating_long(
                        std::numeric_limits<double>::max(), "long"));
                }),
                "checked truncating conversion accepted an unrepresentable value");

        bool boolean_value = true;
        require(parse_bool(" off ", boolean_value) && !boolean_value &&
                is_true("YES") && !is_true("false"),
                "Boolean parsing returned an incorrect value");
        boolean_value = true;
        require(!parse_bool("perhaps", boolean_value) && boolean_value,
                "invalid Boolean parsing changed its output value");

        require(format_json_number(1.25) == "1.25" &&
                format_json_number(std::numeric_limits<double>::infinity()) == "null",
                "JSON number formatting returned an invalid value");
        require(to_hex(static_cast<char>(0xAF)) == "AF",
                "hexadecimal byte formatting returned an incorrect value");
        require(is_valid_utf8("valid \xC3\xA5") &&
                !is_valid_utf8("\xC0\x80") &&
                !is_valid_utf8("\xED\xA0\x80") &&
                !is_valid_utf8("\xE2\x82"),
                "UTF-8 validation accepted malformed input");
        require(escape_json_string("quote\"slash\\line\n") ==
                    "quote\\\"slash\\\\line\\n" &&
                escape_json_string(std::string("a\0b", 3)) == "a\\u0000b",
                "JSON string escaping returned an incorrect value");
        require(rejects_invalid_argument([]()
                {
                    static_cast<void>(escape_json_string("\xC0\x80"));
                }),
                "JSON string escaping accepted invalid UTF-8");

        require(decode_url_component("alpha%20beta") == "alpha beta" &&
                decode_url_component("alpha+beta", true) == "alpha beta" &&
                decode_url_component("alpha+beta") == "alpha+beta",
                "URL decoding returned an incorrect value");
        require(rejects_invalid_argument([]()
                {
                    static_cast<void>(decode_url_component("%"));
                }) &&
                rejects_invalid_argument([]()
                {
                    static_cast<void>(decode_url_component("%GG"));
                }),
                "URL decoding accepted malformed escapes");
        require(remove_comment("1 # first\n2#second\n3") == "1 \n2\n3",
                "comment removal returned an incorrect value");

        auto recovered_from_jpeg_error = [](const std::function<unsigned char *(long &)> & encode)
        {
            long jpeg_size = 17;
            unsigned char * jpeg = nullptr;
            bool recovered = false;
            try
            {
                jpeg = encode(jpeg_size);
            }
            catch(const std::runtime_error & error)
            {
                recovered =
                    std::string(error.what()).find("JPEG encoding failed:") != std::string::npos;
            }
            destroy_jpeg(jpeg);
            return recovered && jpeg_size == 0;
        };

        auto encoded_jpeg = [](const std::function<unsigned char *(long &)> & encode)
        {
            long jpeg_size = 0;
            unsigned char * jpeg = encode(jpeg_size);
            bool encoded = jpeg != nullptr && jpeg_size > 0;
            destroy_jpeg(jpeg);
            return encoded;
        };

        matrix gray_image(2, 2);
        matrix color_image(3, 2, 2);
        require(encoded_jpeg([&gray_image](long & size)
                {
                    return create_gray_jpeg(size, gray_image);
                }) &&
                encoded_jpeg([&gray_image](long & size)
                {
                    return create_pseudocolor_jpeg(size, gray_image);
                }) &&
                encoded_jpeg([&color_image](long & size)
                {
                    return create_color_jpeg(size, color_image);
                }),
                "JPEG encoders failed for valid matrices");

        const std::array<float, 7> edge_values =
        {
            -std::numeric_limits<float>::infinity(),
            -1.0f,
            0.0f,
            0.5f,
            1.0f,
            std::numeric_limits<float>::infinity(),
            std::numeric_limits<float>::quiet_NaN(),
        };
        matrix edge_gray_image(1, static_cast<int>(edge_values.size()));
        matrix edge_color_image(3, 1, static_cast<int>(edge_values.size()));
        for(int x = 0; x < static_cast<int>(edge_values.size()); ++x)
        {
            edge_gray_image(0, x) = edge_values[x];
            for(int channel = 0; channel < 3; ++channel)
                edge_color_image(channel, 0, x) = edge_values[x];
        }
        require(encoded_jpeg([&edge_gray_image](long & size)
                {
                    return create_gray_jpeg(size, edge_gray_image);
                }) &&
                encoded_jpeg([&edge_gray_image](long & size)
                {
                    return create_pseudocolor_jpeg(size, edge_gray_image);
                }) &&
                encoded_jpeg([&edge_color_image](long & size)
                {
                    return create_color_jpeg(size, edge_color_image);
                }) &&
                encoded_jpeg([&edge_gray_image](long & size)
                {
                    return create_gray_jpeg(size, edge_gray_image, 0.5f, 0.5f);
                }) &&
                encoded_jpeg([&edge_gray_image](long & size)
                {
                    return create_pseudocolor_jpeg(size, edge_gray_image, 0.5f, 0.5f);
                }),
                "JPEG encoders failed for clipped, nonfinite, or constant-range data");

        require(rejects_invalid_argument([&gray_image]()
                {
                    long size = 0;
                    unsigned char * jpeg = create_gray_jpeg(size, gray_image, 1, 0);
                    destroy_jpeg(jpeg);
                }) &&
                rejects_invalid_argument([&gray_image]()
                {
                    long size = 0;
                    unsigned char * jpeg = create_pseudocolor_jpeg(
                        size, gray_image, 0, std::numeric_limits<float>::infinity());
                    destroy_jpeg(jpeg);
                }),
                "JPEG encoders accepted invalid conversion ranges");

        matrix oversized_gray_image(1, 70000);
        matrix oversized_color_image(3, 1, 70000);
        require(recovered_from_jpeg_error([&oversized_gray_image](long & size)
                {
                    return create_gray_jpeg(size, oversized_gray_image);
                }) &&
                recovered_from_jpeg_error([&oversized_gray_image](long & size)
                {
                    return create_pseudocolor_jpeg(size, oversized_gray_image);
                }) &&
                recovered_from_jpeg_error([&oversized_color_image](long & size)
                {
                    return create_color_jpeg(size, oversized_color_image);
                }),
                "JPEG encoders did not recover from an unsupported image dimension");

        std::cout << "UTILITIES TEST OK\n";
    }
};

INSTALL_CLASS(UtilitiesTestModule)
