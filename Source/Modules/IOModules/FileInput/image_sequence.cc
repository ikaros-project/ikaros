#include <cctype>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>

#include "image_sequence.h"

namespace ikaros
{
    static bool
    is_one_of(char character, std::string_view choices)
    {
        return choices.find(character) != std::string_view::npos;
    }


    static std::string
    unescape_literal_percents(std::string_view text)
    {
        std::string result;
        result.reserve(text.size());
        for(std::size_t i = 0; i < text.size(); ++i)
        {
            if(text[i] == '%' && i + 1 < text.size() && text[i + 1] == '%')
            {
                result += '%';
                ++i;
            }
            else
                result += text[i];
        }
        return result;
    }


    static std::string
    format_integer_value(int value, char conversion, int width, int precision,
                         bool zero_pad, bool left_align, bool show_sign,
                         bool space_prefix, bool show_base)
    {
        const bool signed_conversion = conversion == 'd' || conversion == 'i';
        const bool negative = signed_conversion && value < 0;
        const unsigned int unsigned_value = static_cast<unsigned int>(value);
        const unsigned long long magnitude = negative ?
            static_cast<unsigned long long>(-(static_cast<long long>(value) + 1)) + 1 :
            static_cast<unsigned long long>(signed_conversion ? value : unsigned_value);

        std::ostringstream digit_stream;
        switch(conversion)
        {
            case 'o':
                digit_stream << std::oct << unsigned_value;
                break;
            case 'x':
                digit_stream << std::hex << std::nouppercase << unsigned_value;
                break;
            case 'X':
                digit_stream << std::hex << std::uppercase << unsigned_value;
                break;
            default:
                digit_stream << magnitude;
                break;
        }

        std::string digits = digit_stream.str();
        if(precision == 0 && magnitude == 0)
            digits.clear();
        if(precision > static_cast<int>(digits.size()))
            digits.insert(0, static_cast<std::size_t>(precision) - digits.size(), '0');
        if(show_base && conversion == 'o' && unsigned_value == 0 && precision == 0)
            digits = "0";

        std::string prefix;
        if(negative)
            prefix = "-";
        else if(signed_conversion && show_sign)
            prefix = "+";
        else if(signed_conversion && space_prefix)
            prefix = " ";

        if(show_base && unsigned_value != 0)
        {
            if(conversion == 'o')
            {
                if(digits.empty() || digits[0] != '0')
                    prefix += "0";
            }
            else if(conversion == 'x')
                prefix += "0x";
            else if(conversion == 'X')
                prefix += "0X";
        }

        std::string result;
        const int padding = width - static_cast<int>(prefix.size() + digits.size());
        if(left_align)
        {
            result = prefix + digits;
            if(padding > 0)
                result += std::string(static_cast<std::size_t>(padding), ' ');
        }
        else if(zero_pad && precision < 0)
        {
            result = prefix;
            if(padding > 0)
                result += std::string(static_cast<std::size_t>(padding), '0');
            result += digits;
        }
        else
        {
            if(padding > 0)
                result += std::string(static_cast<std::size_t>(padding), ' ');
            result += prefix + digits;
        }
        return result;
    }


    static int
    append_decimal_digit(int value, char digit)
    {
        const int numeric_digit = digit - '0';
        if(value > (std::numeric_limits<int>::max() - numeric_digit) / 10)
            throw std::invalid_argument("Image sequence field width is too large");
        return value * 10 + numeric_digit;
    }


    bool
    contains_image_sequence_format(std::string_view pattern)
    {
        for(std::size_t i = 0; i + 1 < pattern.size(); ++i)
        {
            if(pattern[i] != '%')
                continue;
            ++i;
            if(pattern[i] == '%')
                continue;
            while(i < pattern.size() && is_one_of(pattern[i], "-+ #0"))
                ++i;
            while(i < pattern.size() &&
                  std::isdigit(static_cast<unsigned char>(pattern[i])))
                ++i;
            if(i < pattern.size() && pattern[i] == '.')
            {
                ++i;
                while(i < pattern.size() &&
                      std::isdigit(static_cast<unsigned char>(pattern[i])))
                    ++i;
            }
            if(i < pattern.size() && is_one_of(pattern[i], "diuoxX"))
                return true;
        }
        return false;
    }


    std::string
    format_image_sequence_filename(std::string_view pattern, int image_index)
    {
        std::size_t conversion_start = std::string_view::npos;
        std::size_t conversion_end = std::string_view::npos;
        char conversion = '\0';
        int width = 0;
        int precision = -1;
        bool zero_pad = false;
        bool left_align = false;
        bool show_sign = false;
        bool space_prefix = false;
        bool show_base = false;

        for(std::size_t i = 0; i < pattern.size(); ++i)
        {
            if(pattern[i] != '%')
                continue;

            ++i;
            if(i >= pattern.size())
                throw std::invalid_argument(
                    "Image sequence filename ends with an incomplete '%' sequence");
            if(pattern[i] == '%')
                continue;
            if(conversion_start != std::string_view::npos)
                throw std::invalid_argument(
                    "Image sequence filename can only contain one number placeholder");

            conversion_start = i - 1;
            while(i < pattern.size() && is_one_of(pattern[i], "-+ #0"))
            {
                if(pattern[i] == '-')
                    left_align = true;
                else if(pattern[i] == '+')
                    show_sign = true;
                else if(pattern[i] == ' ')
                    space_prefix = true;
                else if(pattern[i] == '#')
                    show_base = true;
                else if(pattern[i] == '0')
                    zero_pad = true;
                ++i;
            }

            if(i < pattern.size() && pattern[i] == '*')
                throw std::invalid_argument(
                    "Image sequence filename cannot use '*' width fields");
            while(i < pattern.size() &&
                  std::isdigit(static_cast<unsigned char>(pattern[i])))
            {
                width = append_decimal_digit(width, pattern[i]);
                ++i;
            }

            if(i < pattern.size() && pattern[i] == '.')
            {
                ++i;
                precision = 0;
                if(i < pattern.size() && pattern[i] == '*')
                    throw std::invalid_argument(
                        "Image sequence filename cannot use '*' precision fields");
                while(i < pattern.size() &&
                      std::isdigit(static_cast<unsigned char>(pattern[i])))
                {
                    precision = append_decimal_digit(precision, pattern[i]);
                    ++i;
                }
            }

            if(i >= pattern.size())
                throw std::invalid_argument(
                    "Image sequence filename ends before a conversion specifier");
            if(is_one_of(pattern[i], "hljztL"))
                throw std::invalid_argument(
                    "Image sequence filename cannot use length modifiers");
            if(!is_one_of(pattern[i], "diuoxX"))
                throw std::invalid_argument(
                    "Image sequence filename must use an integer conversion such as %d or %02d");

            conversion = pattern[i];
            conversion_end = i;
        }

        if(conversion_start == std::string_view::npos)
            return unescape_literal_percents(pattern);

        std::string formatted_name =
            unescape_literal_percents(pattern.substr(0, conversion_start));
        formatted_name += format_integer_value(image_index, conversion, width, precision,
                                               zero_pad, left_align, show_sign,
                                               space_prefix, show_base);
        formatted_name += unescape_literal_percents(pattern.substr(conversion_end + 1));
        return formatted_name;
    }


    struct hash_sequence_pattern
    {
        std::size_t start = std::string_view::npos;
        std::size_t width = 0;
    };


    static hash_sequence_pattern
    parse_hash_image_sequence_pattern(std::string_view pattern)
    {
        if(contains_image_sequence_format(pattern))
            throw std::invalid_argument(
                "InputImage no longer supports printf-style sequence formats; use # or ####");

        hash_sequence_pattern result;
        for(std::size_t i = 0; i < pattern.size(); ++i)
        {
            if(pattern[i] == '\\' && i + 1 < pattern.size() && pattern[i + 1] == '#')
            {
                ++i;
                continue;
            }
            if(pattern[i] != '#')
                continue;
            if(result.start != std::string_view::npos)
                throw std::invalid_argument(
                    "Image sequence filename can only contain one # placeholder");

            result.start = i;
            while(i < pattern.size() && pattern[i] == '#')
            {
                ++result.width;
                ++i;
            }
            --i;
        }
        return result;
    }


    bool
    contains_hash_image_sequence_format(std::string_view pattern)
    {
        return parse_hash_image_sequence_pattern(pattern).start != std::string_view::npos;
    }


    std::string
    format_hash_image_sequence_filename(std::string_view pattern, int image_index)
    {
        if(image_index < 0)
            throw std::invalid_argument("Image sequence index must not be negative");

        const hash_sequence_pattern sequence = parse_hash_image_sequence_pattern(pattern);
        const std::string image_number = std::to_string(image_index);
        if(sequence.width > 1 && image_number.size() > sequence.width)
            throw std::out_of_range("Image sequence index " + image_number +
                                    " does not fit in its # placeholder");

        std::string result;
        result.reserve(pattern.size() + image_number.size());
        for(std::size_t i = 0; i < pattern.size(); ++i)
        {
            if(pattern[i] == '\\' && i + 1 < pattern.size() && pattern[i + 1] == '#')
            {
                result += '#';
                ++i;
            }
            else if(i == sequence.start)
            {
                if(sequence.width > image_number.size())
                    result.append(sequence.width - image_number.size(), '0');
                result += image_number;
                i += sequence.width - 1;
            }
            else
                result += pattern[i];
        }
        return result;
    }


    void
    validate_hash_image_sequence_filecount(std::string_view pattern, int filecount)
    {
        if(filecount < 1)
            throw std::invalid_argument("Image sequence filecount must be at least one");

        const hash_sequence_pattern sequence = parse_hash_image_sequence_pattern(pattern);
        if(sequence.width < 2 || sequence.width >= 10)
            return;

        int capacity = 1;
        for(std::size_t i = 0; i < sequence.width; ++i)
            capacity *= 10;
        if(filecount > capacity)
            throw std::invalid_argument("Image sequence filecount " +
                                        std::to_string(filecount) + " exceeds the " +
                                        std::to_string(capacity) + " files supported by " +
                                        std::string(sequence.width, '#'));
    }
}
