// FIXME: Should use code in image_file_format later on

#include <setjmp.h>
#include <cctype>
#include <sstream>
extern "C"
{
#include <stdio.h>
#include "jpeglib.h"
}

#include "ikaros.h"

using namespace ikaros;

struct my_error_mgr
{
    struct jpeg_error_mgr pub; /* "public" fields */
    jmp_buf setjmp_buffer;     /* for return to caller */
};

typedef struct my_error_mgr *my_error_ptr;

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

static void
my_error_exit(j_common_ptr cinfo)
{
    my_error_ptr myerr = (my_error_ptr)cinfo->err;
    (*cinfo->err->output_message)(cinfo);
    longjmp(myerr->setjmp_buffer, 1);
}

class InputJPEG : public Module
{
public:
    parameter iterations;
    int iteration;
    parameter filecount;
    int cur_image;
    parameter read_once;
    bool first;
    parameter filename;

    parameter size_x;
    parameter size_y;

    matrix intensity;
    matrix output;

    FILE *file;

    bool
    ResolveCurrentFilename(std::string & resolved_name)
    {
        std::string fn;
        std::string error;
        if(!FormatImageSequenceFilename(std::string(filename), cur_image, fn, error))
        {
            Notify(msg_fatal_error, "Invalid image filename format: " + error, path_);
            return false;
        }

        std::filesystem::path sanitized_path;
        if(!kernel().SanitizeReadPath(fn, sanitized_path))
        {
            Notify(msg_fatal_error, "InputImage can only read files from the project directory or UserData.", path_);
            return false;
        }

        resolved_name = sanitized_path.string();
        return true;
    }

    bool
    GetImageSize(int &x, int &y)
    {
        struct jpeg_decompress_struct cinfo;
        struct my_error_mgr jerr;
        FILE *infile;
        std::string fn;
        if(!ResolveCurrentFilename(fn))
            return false;

        if ((infile = fopen(fn.c_str(), "rb")) == NULL)
        {
            std::string temp = std::string("Could not open image file: ") + fn;
            Notify(msg_fatal_error, temp, path_);
            return false;
        }

        cinfo.err = jpeg_std_error(&jerr.pub);
        jerr.pub.error_exit = my_error_exit;

        if (setjmp(jerr.setjmp_buffer))
        {
            jpeg_destroy_decompress(&cinfo);
            fclose(infile);
            return false;
        }

        jpeg_create_decompress(&cinfo);
        jpeg_stdio_src(&cinfo, infile);
        (void)jpeg_read_header(&cinfo, TRUE);
        (void)jpeg_start_decompress(&cinfo);

        Notify(msg_debug, "Checking \"" + std::string(fn) + "\" (width = " + std::to_string(cinfo.output_width) + " height = " + std::to_string(cinfo.output_height) + " channels = " + std::to_string(cinfo.output_components) + ")");

        x = cinfo.output_width;
        y = cinfo.output_height;

        fclose(infile);

        return true;
    }

    void
    SetParameters() // Getimage size from JPEG file
    {
        Bind(size_x, "size_x");
        Bind(size_y, "size_y");
        Bind(filename, "filename");

        filename = GetValue("filename");

        // std::cout << "size_x: " << size_x << std::endl;
        // std::cout << "size_y: " << size_y << std::endl;
        // std::cout << "Filename: " << filename << std::endl;

        int sx, sy;
        if (!GetImageSize(sx, sy) || sx * sy < 1)
        {
            Notify(msg_fatal_error, "Image size could not be found in the file.\n");
            return;
        }
        size_x = sx;
        size_y = sy;
        // std::cout << "size_x: " << size_x << std::endl;
        // std::cout << "size_y: " << size_y << std::endl;
        // std::cout << "Filename: " << filename << std::endl;
    }

    void
    Init()
    {
        // Count no of images/filenames
        // Bind(filename, "filename");

        if (std::string(filename).empty())
        {
            Notify(msg_fatal_error, "No filename(s) supplied.\n");
            return;
        }

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

            struct jpeg_decompress_struct cinfo;
            struct my_error_mgr jerr;
            FILE *infile;      // source file
            JSAMPARRAY buffer; // Output row buffer
            int row_stride;    // physical row width in output buffer
            std::string fn;
            if(!ResolveCurrentFilename(fn))
                return;

            if ((infile = fopen(fn.c_str(), "rb")) == NULL)
            {
                Notify(msg_fatal_error, "Could not open image file \"" + fn + "\".");
                return;
            }

            cinfo.err = jpeg_std_error(&jerr.pub);
            jerr.pub.error_exit = my_error_exit;

            if (setjmp(jerr.setjmp_buffer))
            {
                jpeg_destroy_decompress(&cinfo);
                fclose(infile);
                return;
            }

            jpeg_create_decompress(&cinfo);
            jpeg_stdio_src(&cinfo, infile);
            (void)jpeg_read_header(&cinfo, TRUE);
            (void)jpeg_start_decompress(&cinfo);
            row_stride = cinfo.output_width * cinfo.output_components;
            Notify(msg_debug, "InputJPEG: width = " + std::to_string(cinfo.output_width) + " height = " + std::to_string(cinfo.output_height) + " components = " + std::to_string(cinfo.output_components) + "\n");

            if (cinfo.output_width != (unsigned int)(size_x) || cinfo.output_height != (unsigned int)(size_y))
            {
                Notify(msg_fatal_error, "Image \"" + fn + "\" has incorrect size.");
                fclose(infile);
                return;
            }

            buffer = (*cinfo.mem->alloc_sarray)((j_common_ptr)&cinfo, JPOOL_IMAGE, row_stride, 1);

            if (cinfo.output_components == 1) // Gray Scale Image
                while (cinfo.output_scanline < cinfo.output_height)
                {
                    // printf("scanline = %d\n", cinfo.output_scanline);
                    (void)jpeg_read_scanlines(&cinfo, buffer, 1);
                    for (int i = 0; i < size_x; i++) // CHECK BOUNDS
                    {
                        int ix = size_x * (cinfo.output_scanline - 1) + i;
                        intensity.data()[ix] = float(buffer[0][i]) / 255.0;
                    }
                }

            else // RGB Color Image
            {
                float c255 = 1.0 / 255.0;
                float c3 = 1.0 / 3.0;
                while (cinfo.output_scanline < cinfo.output_height)
                {
                    (void)jpeg_read_scanlines(&cinfo, buffer, 1);
                    unsigned char *buf = buffer[0];
                    int ix = size_x * (cinfo.output_scanline - 1);
                    float *r = &output[0].data()[ix];
                    float *g = &output[1].data()[ix];
                    float *b = &output[2].data()[ix];
                    float *iy = &intensity.data()[ix];
                    for (int i = 0; i < size_x; i++) // CHECK BOUNDS
                    {
                        *r = c255 * float(*buf++);
                        *g = c255 * float(*buf++);
                        *b = c255 * float(*buf++);
                        *iy++ = c3 * ((*r++) + (*g++) + (*b++)); // Do this correctly later!!!
                    }
                }
            }

            (void)jpeg_finish_decompress(&cinfo);
            jpeg_destroy_decompress(&cinfo);
            fclose(infile);

            // At this point you may want to check to see whether any corrupt-data
            // warnings occurred (test whether jerr.pub.num_warnings is nonzero).
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
