#include <filesystem>
#include <stdexcept>
#include <string>

#include "ikaros.h"
#include "../image_sequence.h"

using namespace ikaros;

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
        const std::string fn =
            format_image_sequence_filename(std::string(filename), cur_image);

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

        if(contains_image_sequence_format(std::string(filename)))
            read_once = false;
        first = true;

        Bind(intensity, "INTENSITY");
        Bind(output, "OUTPUT");
    }

    void
    Tick()
    {
        if(first || !read_once)
        {
            first = false;
            std::string image_name = std::string(filename);
            try
            {
                const std::filesystem::path image_path = ResolveCurrentFilename();
                image_name = image_path.string();
                jpeg_get_image(output, intensity, image_path);
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
        if(cur_image >= filecount)
        {
            cur_image = 0;
            iteration = iteration + 1;
        }

        if(iterations != 0 && iteration > iterations)
            Notify(msg_terminate, "End of image sequence");
    }
};

INSTALL_CLASS(InputJPEG)
