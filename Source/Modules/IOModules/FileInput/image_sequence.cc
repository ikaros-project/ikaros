#include "image_sequence.h"

#include <stdexcept>
#include <string>
#include <string_view>

#include "../../../Kernel/utilities.h"


namespace ikaros
{
    namespace
    {
        struct hash_sequence_pattern
        {
            std::size_t start = std::string_view::npos;
            std::size_t width = 0;
        };


        bool
        contains_printf_integer_placeholder(std::string_view pattern)
        {
            constexpr std::string_view flags = "-+ #0";
            constexpr std::string_view conversions = "diuoxX";

            for(std::size_t index = 0;
                index + 1 < pattern.size(); ++index)
            {
                if(pattern[index] != '%')
                    continue;
                ++index;
                if(pattern[index] == '%')
                    continue;
                while(index < pattern.size() &&
                      flags.find(pattern[index]) !=
                          std::string_view::npos)
                    ++index;
                while(index < pattern.size() &&
                      ascii_is_digit(static_cast<unsigned char>(pattern[index])))
                    ++index;
                if(index < pattern.size() &&
                   pattern[index] == '.')
                {
                    ++index;
                    while(index < pattern.size() &&
                          ascii_is_digit(static_cast<unsigned char>(pattern[index])))
                        ++index;
                }
                if(index < pattern.size() &&
                   conversions.find(pattern[index]) !=
                       std::string_view::npos)
                    return true;
            }
            return false;
        }


        hash_sequence_pattern
        parse_hash_image_sequence_pattern(
            std::string_view pattern)
        {
            if(contains_printf_integer_placeholder(pattern))
                throw std::invalid_argument(
                    "Image sequences do not support printf-style "
                    "formats here; use # or ####");

            hash_sequence_pattern result;
            for(std::size_t index = 0;
                index < pattern.size(); ++index)
            {
                if(pattern[index] == '\\' &&
                   index + 1 < pattern.size() &&
                   pattern[index + 1] == '#')
                {
                    ++index;
                    continue;
                }
                if(pattern[index] != '#')
                    continue;
                if(result.start != std::string_view::npos)
                    throw std::invalid_argument(
                        "Image sequence filename can only contain "
                        "one # placeholder");

                result.start = index;
                while(index < pattern.size() &&
                      pattern[index] == '#')
                {
                    ++result.width;
                    ++index;
                }
                --index;
            }
            return result;
        }
    }


    bool
    contains_hash_image_sequence_format(
        std::string_view pattern)
    {
        return parse_hash_image_sequence_pattern(pattern).start !=
               std::string_view::npos;
    }


    std::string
    format_hash_image_sequence_filename(
        std::string_view pattern, int image_index)
    {
        if(image_index < 0)
            throw std::invalid_argument(
                "Image sequence index must not be negative");

        const hash_sequence_pattern sequence =
            parse_hash_image_sequence_pattern(pattern);
        const std::string image_number =
            std::to_string(image_index);
        if(sequence.width > 1 &&
           image_number.size() > sequence.width)
            throw std::out_of_range(
                "Image sequence index " + image_number +
                " does not fit in its # placeholder");

        std::string result;
        result.reserve(pattern.size() + image_number.size());
        for(std::size_t index = 0;
            index < pattern.size(); ++index)
        {
            if(pattern[index] == '\\' &&
               index + 1 < pattern.size() &&
               pattern[index + 1] == '#')
            {
                result += '#';
                ++index;
            }
            else if(index == sequence.start)
            {
                if(sequence.width > image_number.size())
                    result.append(
                        sequence.width - image_number.size(),
                        '0');
                result += image_number;
                index += sequence.width - 1;
            }
            else
                result += pattern[index];
        }
        return result;
    }


    void
    validate_hash_image_sequence_filecount(
        std::string_view pattern, int filecount)
    {
        if(filecount < 1)
            throw std::invalid_argument(
                "Image sequence filecount must be at least one");

        const hash_sequence_pattern sequence =
            parse_hash_image_sequence_pattern(pattern);
        if(sequence.width < 2 || sequence.width >= 10)
            return;

        int capacity = 1;
        for(std::size_t index = 0;
            index < sequence.width; ++index)
            capacity *= 10;
        if(filecount > capacity)
            throw std::invalid_argument(
                "Image sequence filecount " +
                std::to_string(filecount) + " exceeds the " +
                std::to_string(capacity) +
                " files supported by " +
                std::string(sequence.width, '#'));
    }
}
