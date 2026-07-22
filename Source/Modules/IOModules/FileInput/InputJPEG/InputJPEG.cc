#include <cctype>
#include <filesystem>
#include <sstream>
#include <stdexcept>
#include <string>

#include "ikaros.h"

using namespace ikaros;

static bool
IsOneOf(char c, const std::string & choices)
{
    return choices.find(c) != std::string::npos;
}

static std::string
UnescapeLiteralPercents(const std::string & text)
{
    std::string result;
    for(size_t i=0; i<text.size(); ++i)
    {
        if(text[i] == '%' && i+1 < text.size() && text[i+1] == '%')
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
FormatIntegerValue(int value, char conversion, int width, int precision, bool zero_pad, bool left_align, bool show_sign, bool space_prefix, bool show_base)
{
    const bool signed_conversion = conversion == 'd' || conversion == 'i';
    const bool negative = signed_conversion && value < 0;
    const unsigned int unsigned_value = static_cast<unsigned int>(value);
    const unsigned long long magnitude = negative ?
        static_cast<unsigned long long>(-(static_cast<long long>(value)+1))+1 :
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
    while(precision > static_cast<int>(digits.size()))
        digits = "0" + digits;
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
    const int padding = width - static_cast<int>(prefix.size()+digits.size());
    if(left_align)
    {
        result = prefix + digits;
        if(padding > 0)
            result += std::string(static_cast<size_t>(padding), ' ');
    }
    else if(zero_pad && precision < 0)
    {
        result = prefix;
        if(padding > 0)
            result += std::string(static_cast<size_t>(padding), '0');
        result += digits;
    }
    else
    {
        if(padding > 0)
            result += std::string(static_cast<size_t>(padding), ' ');
        result += prefix + digits;
    }

    return result;
}

static bool
ContainsImageSequenceFormat(const std::string & pattern)
{
    for(size_t i=0; i+1<pattern.size(); ++i)
    {
        if(pattern[i] != '%')
            continue;
        ++i;
        if(pattern[i] == '%')
            continue;
        while(i < pattern.size() && IsOneOf(pattern[i], "-+ #0"))
            ++i;
        while(i < pattern.size() && std::isdigit(static_cast<unsigned char>(pattern[i])))
            ++i;
        if(i < pattern.size() && pattern[i] == '.')
        {
            ++i;
            while(i < pattern.size() && std::isdigit(static_cast<unsigned char>(pattern[i])))
                ++i;
        }
        if(i < pattern.size() && IsOneOf(pattern[i], "diuoxX"))
            return true;
    }
    return false;
}

static bool
FormatImageSequenceFilename(const std::string & pattern, int image_index, std::string & formatted_name, std::string & error)
{
    size_t conversion_start = std::string::npos;
    size_t conversion_end = std::string::npos;
    char conversion = '\0';
    int width = 0;
    int precision = -1;
    bool zero_pad = false;
    bool left_align = false;
    bool show_sign = false;
    bool space_prefix = false;
    bool show_base = false;

    for(size_t i=0; i<pattern.size(); ++i)
    {
        if(pattern[i] != '%')
            continue;

        ++i;
        if(i >= pattern.size())
        {
            error = "Filename format ends with an incomplete '%' sequence.";
            return false;
        }

        if(pattern[i] == '%')
            continue;

        if(conversion_start != std::string::npos)
        {
            error = "Filename format can only contain one image-number placeholder.";
            return false;
        }

        conversion_start = i-1;

        while(i < pattern.size() && IsOneOf(pattern[i], "-+ #0"))
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
        {
            error = "Filename format cannot use '*' width fields.";
            return false;
        }

        while(i < pattern.size() && std::isdigit(static_cast<unsigned char>(pattern[i])))
        {
            width = width * 10 + (pattern[i] - '0');
            ++i;
        }

        if(i < pattern.size() && pattern[i] == '.')
        {
            ++i;
            precision = 0;
            if(i < pattern.size() && pattern[i] == '*')
            {
                error = "Filename format cannot use '*' precision fields.";
                return false;
            }
            while(i < pattern.size() && std::isdigit(static_cast<unsigned char>(pattern[i])))
            {
                precision = precision * 10 + (pattern[i] - '0');
                ++i;
            }
        }

        if(i >= pattern.size())
        {
            error = "Filename format ends before a conversion specifier.";
            return false;
        }

        if(IsOneOf(pattern[i], "hljztL"))
        {
            error = "Filename format cannot use length modifiers.";
            return false;
        }

        if(!IsOneOf(pattern[i], "diuoxX"))
        {
            error = "Filename format must use an integer conversion such as %d or %02d.";
            return false;
        }

        conversion = pattern[i];
        conversion_end = i;
    }

    if(conversion_start == std::string::npos)
    {
        formatted_name = UnescapeLiteralPercents(pattern);
        return true;
    }

    formatted_name = UnescapeLiteralPercents(pattern.substr(0, conversion_start));
    formatted_name += FormatIntegerValue(image_index, conversion, width, precision, zero_pad, left_align, show_sign, space_prefix, show_base);
    formatted_name += UnescapeLiteralPercents(pattern.substr(conversion_end+1));

    return true;
}

class InputJPEG : public Module
{
public:
    parameter iterations;
    int iteration = 1;
    parameter filecount;
    int cur_image = 0;
    parameter read_once;
    bool first = true;
    parameter filename;

    parameter size_x;
    parameter size_y;

    matrix intensity;
    matrix output;

    std::filesystem::path
    ResolveCurrentFilename()
    {
        std::string fn;
        std::string error;
        if(!FormatImageSequenceFilename(std::string(filename), cur_image, fn, error))
            throw std::invalid_argument("Invalid JPEG filename format: " + error);

        std::filesystem::path sanitized_path;
        if(!kernel().SanitizeReadPath(fn, sanitized_path))
            throw std::invalid_argument(
                "InputJPEG can only read files from the project directory or UserData");

        return sanitized_path;
    }

    void
    SetParameters()
    {
        Bind(size_x, "size_x");
        Bind(size_y, "size_y");
        Bind(filename, "filename");

        filename = GetValue("filename");

        const std::filesystem::path image_path = ResolveCurrentFilename();
        const auto [width, height, channels] = jpeg_get_info(image_path);
        Debug("Checking \"" + image_path.string() + "\" (width = " +
              std::to_string(width) + " height = " + std::to_string(height) +
              " channels = " + std::to_string(channels) + ")");

        if(width < 1 || height < 1)
            throw std::runtime_error("Image size could not be found in \"" +
                                     image_path.string() + "\"");
        size_x = width;
        size_y = height;
    }

    void
    Init()
    {
        if(std::string(filename).empty())
            throw std::invalid_argument("No JPEG filename supplied");

        iteration = 1;
        cur_image = 0;

        Bind(iterations, "iterations");
        Bind(filecount, "filecount");
        Bind(read_once, "read_once");

        if (ContainsImageSequenceFormat(std::string(filename)))
            read_once = false;
        first = true;

        Bind(intensity, "INTENSITY");
        Bind(output, "OUTPUT");
    }

    void
    Tick()
    {
        if (first || !read_once)
        {
            first = false;
            std::string image_name = std::string(filename);
            try
            {
                const std::filesystem::path image_path = ResolveCurrentFilename();
                image_name = image_path.string();
                jpeg_get_image(output, image_path);

                const int pixel_count = intensity.size();
                const float * red = output.contiguous_data();
                const float * green = red + pixel_count;
                const float * blue = green + pixel_count;
                float * gray = intensity.contiguous_data();
                constexpr float one_third = 1.0f / 3.0f;
                for(int i = 0; i < pixel_count; ++i)
                    gray[i] = one_third * (red[i] + green[i] + blue[i]);
            }
            catch(const std::exception & error)
            {
                output.reset();
                intensity.reset();
                Warning("Could not read JPEG image \"" + image_name + "\": " +
                        error.what(), path_);
            }
        }

        cur_image++;
        if (cur_image >= filecount)
        {
            cur_image = 0;
            iteration = iteration + 1;
            //		printf("InputJPEG: Repeating (%ld/%ld)\n", iteration, iterations);
        }

        if (iterations != 0 && iteration > iterations)
            Notify(msg_terminate, "End of image sequence");
    }
};

INSTALL_CLASS(InputJPEG)
