#include <array>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "ikaros.h"
#include "../../../../Modules/IOModules/FileInput/image_sequence.h"

using namespace ikaros;

namespace
{
    class TemporaryFile
    {
    public:
        TemporaryFile(const std::string & name, const std::string & contents)
        {
            const auto suffix = std::chrono::steady_clock::now().time_since_epoch().count();
            path_ = std::filesystem::temp_directory_path() /
                    ("ikaros-utilities-" + std::to_string(suffix) + "-" + name);
            std::ofstream file(path_, std::ios::binary);
            if(!file)
                throw std::runtime_error("Could not create temporary utility test file");
            file.write(contents.data(), static_cast<std::streamsize>(contents.size()));
            if(!file)
                throw std::runtime_error("Could not write temporary utility test file");
        }

        ~TemporaryFile()
        {
            std::error_code error;
            std::filesystem::remove(path_, error);
        }

        const std::filesystem::path &
        path() const noexcept
        {
            return path_;
        }

    private:
        std::filesystem::path path_;
    };


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

        validate_hash_image_sequence_filecount("image_##.png", 100);
        require(contains_hash_image_sequence_format("image_#.png") &&
                contains_hash_image_sequence_format("image_####.png") &&
                !contains_hash_image_sequence_format("image_\\#.png") &&
                format_hash_image_sequence_filename("image_#.png", 12345) ==
                    "image_12345.png" &&
                format_hash_image_sequence_filename("image_####.png", 12) ==
                    "image_0012.png" &&
                format_hash_image_sequence_filename("image_\\#.png", 12) ==
                    "image_#.png" &&
                rejects_out_of_range([]
                {
                    static_cast<void>(
                        format_hash_image_sequence_filename("image_####.png", 10000));
                }) &&
                rejects_invalid_argument([]
                {
                    static_cast<void>(
                        format_hash_image_sequence_filename("image_#_#.png", 0));
                }) &&
                rejects_invalid_argument([]
                {
                    static_cast<void>(
                        format_hash_image_sequence_filename("image_%04d.png", 0));
                }) &&
                rejects_invalid_argument([]
                {
                    validate_hash_image_sequence_filecount("image_##.png", 101);
                }),
                "Hash image-sequence formatting or validation failed");

        auto recovered_from_jpeg_error = [](const std::function<jpeg_data()> & encode)
        {
            try
            {
                static_cast<void>(encode());
            }
            catch(const std::runtime_error & error)
            {
                return std::string(error.what()).find("JPEG encoding failed:") !=
                       std::string::npos;
            }
            return false;
        };

        auto encoded_jpeg = [](const std::function<jpeg_data()> & encode)
        {
            const jpeg_data data = encode();
            const auto bytes = data.bytes();
            return bytes.size() >= 4 && bytes[0] == 0xff && bytes[1] == 0xd8 &&
                   bytes[bytes.size() - 2] == 0xff && bytes.back() == 0xd9;
        };

        matrix gray_image(2, 2);
        matrix color_image(3, 2, 2);
        bool encoded_all_color_tables = true;
        for(const char * table : {"spectrum", "red", "green", "blue", "fire"})
            encoded_all_color_tables = encoded_all_color_tables &&
                                       encoded_jpeg([&gray_image, table]
                                       {
                                           return create_pseudocolor_jpeg(
                                               gray_image, 0, 1, table);
                                       });
        require(encoded_jpeg([&gray_image]
                {
                    return create_gray_jpeg(gray_image);
                }) &&
                encoded_jpeg([&gray_image]
                {
                    return create_pseudocolor_jpeg(gray_image);
                }) &&
                encoded_jpeg([&color_image]
                {
                    return create_color_jpeg(color_image);
                }) && encoded_all_color_tables,
                "JPEG encoders failed for valid matrices");
        require(create_color_jpeg(gray_image).empty() &&
                create_pseudocolor_jpeg(gray_image, 0, 1, "missing").empty(),
                "JPEG encoders accepted an incompatible shape or unknown color table");

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
        require(encoded_jpeg([&edge_gray_image]
                {
                    return create_gray_jpeg(edge_gray_image);
                }) &&
                encoded_jpeg([&edge_gray_image]
                {
                    return create_pseudocolor_jpeg(edge_gray_image);
                }) &&
                encoded_jpeg([&edge_color_image]
                {
                    return create_color_jpeg(edge_color_image);
                }) &&
                encoded_jpeg([&edge_gray_image]
                {
                    return create_gray_jpeg(edge_gray_image, 0.5f, 0.5f);
                }) &&
                encoded_jpeg([&edge_gray_image]
                {
                    return create_pseudocolor_jpeg(edge_gray_image, 0.5f, 0.5f);
                }),
                "JPEG encoders failed for clipped, nonfinite, or constant-range data");

        require(rejects_invalid_argument([&gray_image]()
                {
                    static_cast<void>(create_gray_jpeg(gray_image, 1, 0));
                }) &&
                rejects_invalid_argument([&gray_image]()
                {
                    static_cast<void>(create_pseudocolor_jpeg(
                        gray_image, 0, std::numeric_limits<float>::infinity()));
                }),
                "JPEG encoders accepted invalid conversion ranges");

        auto rejected_invalid_jpeg_quality = [&](int quality)
        {
            return rejects_invalid_argument([&]
                   {
                       static_cast<void>(create_gray_jpeg(gray_image, 0, 1, quality));
                   }) &&
                   rejects_invalid_argument([&]
                   {
                       static_cast<void>(create_pseudocolor_jpeg(
                           gray_image, 0, 1, "fire", quality));
                   }) &&
                   rejects_invalid_argument([&]
                   {
                       static_cast<void>(create_color_jpeg(color_image, quality));
                   });
        };
        require(rejected_invalid_jpeg_quality(0) &&
                rejected_invalid_jpeg_quality(101) &&
                encoded_jpeg([&color_image]
                {
                    return create_color_jpeg(color_image, 50);
                }),
                "JPEG encoders accepted an invalid quality or failed after rejecting one");

        matrix oversized_gray_image(1, 70000);
        matrix oversized_color_image(3, 1, 70000);
        require(recovered_from_jpeg_error([&oversized_gray_image]
                {
                    return create_gray_jpeg(oversized_gray_image);
                }) &&
                recovered_from_jpeg_error([&oversized_gray_image]
                {
                    return create_pseudocolor_jpeg(oversized_gray_image);
                }) &&
                recovered_from_jpeg_error([&oversized_color_image]
                {
                    return create_color_jpeg(oversized_color_image);
                }),
                "JPEG encoders did not recover from an unsupported image dimension");

        const jpeg_data readable_jpeg = create_color_jpeg(color_image);
        const TemporaryFile valid_jpeg(
            "valid.jpg",
            std::string(reinterpret_cast<const char *>(readable_jpeg.data()),
                        readable_jpeg.size()));
        const auto [decoded_width, decoded_height, decoded_channels] =
            jpeg_get_info(valid_jpeg.path());
        matrix decoded_jpeg = jpeg_get_image(valid_jpeg.path());
        float * decoded_jpeg_storage = decoded_jpeg.data();
        jpeg_get_image(decoded_jpeg, valid_jpeg.path());
        require(decoded_width == 2 && decoded_height == 2 && decoded_channels == 3 &&
                decoded_jpeg.rank() == 3 && decoded_jpeg.size(0) == 3 &&
                decoded_jpeg.size(1) == 2 && decoded_jpeg.size(2) == 2 &&
                decoded_jpeg.data() == decoded_jpeg_storage,
                "JPEG readers failed for valid input");

        matrix grayscale_source(2, 2);
        grayscale_source(0, 0) = 0.0f;
        grayscale_source(0, 1) = 0.25f;
        grayscale_source(1, 0) = 0.75f;
        grayscale_source(1, 1) = 1.0f;
        const jpeg_data grayscale_data = create_gray_jpeg(grayscale_source);
        const TemporaryFile grayscale_jpeg(
            "grayscale.jpg",
            std::string(reinterpret_cast<const char *>(grayscale_data.data()),
                        grayscale_data.size()));
        const matrix grayscale_image = jpeg_get_image(grayscale_jpeg.path());
        bool grayscale_planes_match = jpeg_get_info(grayscale_jpeg.path()).channels == 1;
        for(int y = 0; y < 2; ++y)
            for(int x = 0; x < 2; ++x)
                grayscale_planes_match = grayscale_planes_match &&
                                         grayscale_image(0, y, x) ==
                                             grayscale_image(1, y, x) &&
                                         grayscale_image(0, y, x) ==
                                             grayscale_image(2, y, x);
        require(grayscale_planes_match &&
                grayscale_image(0, 0, 0) < grayscale_image(0, 1, 1),
                "JPEG reader did not convert grayscale input to RGB safely");

        auto rejected_malformed_jpeg = [](const std::function<void()> & read)
        {
            try
            {
                read();
            }
            catch(const std::runtime_error & error)
            {
                return std::string(error.what()).find("JPEG read failed for \"") == 0;
            }
            return false;
        };

        const TemporaryFile malformed_jpeg("malformed.jpg", "not a JPEG file");
        require(rejected_malformed_jpeg([&]
                {
                    static_cast<void>(jpeg_get_info(malformed_jpeg.path()));
                }) &&
                rejected_malformed_jpeg([&]
                {
                    static_cast<void>(jpeg_get_image(malformed_jpeg.path()));
                }),
                "JPEG readers did not recover from malformed input");

        const TemporaryFile truncated_jpeg(
            "truncated.jpg",
            std::string(reinterpret_cast<const char *>(readable_jpeg.data()),
                        readable_jpeg.size() - 2));
        const auto [truncated_width, truncated_height, truncated_channels] =
            jpeg_get_info(truncated_jpeg.path());
        require(truncated_width == 2 && truncated_height == 2 && truncated_channels == 3 &&
                rejected_malformed_jpeg([&]
                {
                    static_cast<void>(jpeg_get_image(truncated_jpeg.path()));
                }) &&
                jpeg_get_image(valid_jpeg.path()).shape() == std::vector<int>{3, 2, 2},
                "JPEG reader accepted truncated pixel data or did not recover afterward");

        const std::array<unsigned char, 72> valid_png_data
        {
            0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a,
            0x00, 0x00, 0x00, 0x0d, 0x49, 0x48, 0x44, 0x52,
            0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x01,
            0x08, 0x02, 0x00, 0x00, 0x00, 0x7b, 0x40, 0xe8,
            0xdd, 0x00, 0x00, 0x00, 0x0f, 0x49, 0x44, 0x41,
            0x54, 0x78, 0x9c, 0x63, 0xf8, 0xcf, 0xc0, 0xc0,
            0xd0, 0xf0, 0x1f, 0x00, 0x08, 0x00, 0x02, 0x7f,
            0x9c, 0x45, 0x40, 0x4e, 0x00, 0x00, 0x00, 0x00,
            0x49, 0x45, 0x4e, 0x44, 0xae, 0x42, 0x60, 0x82,
        };
        const std::string valid_png_contents(
            reinterpret_cast<const char *>(valid_png_data.data()), valid_png_data.size());
        const TemporaryFile valid_png("valid.png", valid_png_contents);
        const auto [png_width, png_height, png_channels] = png_get_info(valid_png.path());
        matrix decoded_png = png_get_image(valid_png.path());
        float * decoded_png_storage = decoded_png.data();
        png_get_image(decoded_png, valid_png.path());
        require(png_width == 2 && png_height == 1 && png_channels == 3 &&
                decoded_png.rank() == 3 && decoded_png.size(0) == 3 &&
                decoded_png.size(1) == 1 && decoded_png.size(2) == 2 &&
                decoded_png.data() == decoded_png_storage &&
                decoded_png(0, 0, 0) == 1.0f && decoded_png(1, 0, 0) == 0.0f &&
                decoded_png(2, 0, 0) == 0.0f && decoded_png(0, 0, 1) == 0.0f &&
                decoded_png(1, 0, 1) > 0.5f && decoded_png(2, 0, 1) == 1.0f,
                "PNG readers failed for valid RGB input");

        const TemporaryFile generic_jpeg(
            "generic.JPEG",
            std::string(reinterpret_cast<const char *>(readable_jpeg.data()),
                        readable_jpeg.size()));
        const TemporaryFile generic_png("generic.PNG", valid_png_contents);
        const auto [generic_jpeg_width, generic_jpeg_height, generic_jpeg_channels] =
            image_get_info(generic_jpeg.path());
        const auto [generic_png_width, generic_png_height, generic_png_channels] =
            image_get_info(generic_png.path());
        matrix generic_jpeg_image = image_get_image(generic_jpeg.path());
        float * generic_jpeg_storage = generic_jpeg_image.data();
        image_get_image(generic_jpeg_image, generic_jpeg.path());
        const matrix generic_png_image = image_get_image(generic_png.path());
        const TemporaryFile unsupported_image("unsupported.gif", valid_png_contents);
        require(generic_jpeg_width == 2 && generic_jpeg_height == 2 &&
                generic_jpeg_channels == 3 &&
                generic_png_width == 2 && generic_png_height == 1 &&
                generic_png_channels == 3 &&
                generic_jpeg_image.shape() == std::vector<int>{3, 2, 2} &&
                generic_jpeg_image.data() == generic_jpeg_storage &&
                generic_png_image.shape() == std::vector<int>{3, 1, 2} &&
                rejects_invalid_argument([&]
                {
                    static_cast<void>(image_get_info(unsupported_image.path()));
                }) &&
                rejects_invalid_argument([&]
                {
                    static_cast<void>(image_get_image(unsupported_image.path()));
                }),
                "Generic image dispatch failed for JPEG, PNG, or unsupported input");

        const std::array<unsigned char, 72> interlaced_png_data
        {
            0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a,
            0x00, 0x00, 0x00, 0x0d, 0x49, 0x48, 0x44, 0x52,
            0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x02,
            0x08, 0x02, 0x00, 0x00, 0x01, 0x8a, 0xd3, 0xaa,
            0xe5, 0x00, 0x00, 0x00, 0x0f, 0x49, 0x44, 0x41,
            0x54, 0x78, 0x9c, 0x63, 0xf8, 0xcf, 0x00, 0x04,
            0x10, 0x02, 0x08, 0x00, 0x20, 0xee, 0x05, 0xfb,
            0xf5, 0x2b, 0xe9, 0xca, 0x00, 0x00, 0x00, 0x00,
            0x49, 0x45, 0x4e, 0x44, 0xae, 0x42, 0x60, 0x82,
        };
        const TemporaryFile interlaced_png(
            "interlaced.png",
            std::string(reinterpret_cast<const char *>(interlaced_png_data.data()),
                        interlaced_png_data.size()));
        const matrix decoded_interlaced_png = png_get_image(interlaced_png.path());
        require(decoded_interlaced_png.shape() == std::vector<int>{3, 2, 2} &&
                decoded_interlaced_png(0, 0, 0) == 1.0f &&
                decoded_interlaced_png(1, 0, 0) == 0.0f &&
                decoded_interlaced_png(2, 0, 0) == 0.0f &&
                decoded_interlaced_png(0, 0, 1) == 0.0f &&
                decoded_interlaced_png(1, 0, 1) == 1.0f &&
                decoded_interlaced_png(2, 0, 1) == 0.0f &&
                decoded_interlaced_png(0, 1, 0) == 0.0f &&
                decoded_interlaced_png(1, 1, 0) == 0.0f &&
                decoded_interlaced_png(2, 1, 0) == 1.0f &&
                decoded_interlaced_png(0, 1, 1) == 1.0f &&
                decoded_interlaced_png(1, 1, 1) == 1.0f &&
                decoded_interlaced_png(2, 1, 1) == 1.0f,
                "PNG reader did not reconstruct an Adam7-interlaced image");

        matrix wrong_image_destination(3, 1, 1);
        require(rejects_invalid_argument([&]
                {
                    jpeg_get_image(wrong_image_destination, valid_jpeg.path());
                }) &&
                rejects_invalid_argument([&]
                {
                    png_get_image(wrong_image_destination, valid_png.path());
                }) &&
                wrong_image_destination.shape() == std::vector<int>{3, 1, 1},
                "Image readers accepted or resized an initialized destination of the wrong shape");

        auto rejected_malformed_png = [](const std::function<void()> & read)
        {
            try
            {
                read();
            }
            catch(const std::runtime_error & error)
            {
                return std::string(error.what()).find("PNG read failed for \"") == 0;
            }
            return false;
        };

        const TemporaryFile short_png("short.png", std::string("\x89PNG", 4));
        require(rejected_malformed_png([&]
                {
                    static_cast<void>(png_get_info(short_png.path()));
                }) &&
                rejected_malformed_png([&]
                {
                    static_cast<void>(png_get_image(short_png.path()));
                }),
                "PNG readers did not reject a short signature safely");

        const TemporaryFile truncated_png(
            "truncated.png",
            std::string(reinterpret_cast<const char *>(valid_png_data.data()), 41));
        require(rejected_malformed_png([&]
                {
                    static_cast<void>(png_get_image(truncated_png.path()));
                }),
                "PNG image reader did not recover from truncated pixel data");

        std::cout << "UTILITIES TEST OK\n";
    }
};

INSTALL_CLASS(UtilitiesTestModule)
