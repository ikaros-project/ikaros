#include <filesystem>
#include <stdexcept>
#include <string>

#include "ikaros.h"
#include "../image_sequence.h"

using namespace ikaros;

class InputImage : public Module
{
public:
    parameter iterations;
    int iteration = 1;
    parameter fileCount;
    int currentImage = 0;
    parameter readOnce;
    bool first = true;
    parameter filename;

    parameter sizeX;
    parameter sizeY;

    matrix intensity;
    matrix output;

    std::filesystem::path
    ResolveCurrentFilename()
    {
        const std::string formattedName =
            format_hash_image_sequence_filename(std::string(filename), currentImage);

        std::filesystem::path sanitizedPath;
        if(!kernel().SanitizeReadPath(formattedName, sanitizedPath))
            throw std::invalid_argument(
                "InputImage can only read files from the project directory or UserData");
        return sanitizedPath;
    }

    void
    SetParameters()
    {
        Bind(sizeX, "size_x");
        Bind(sizeY, "size_y");
        Bind(filename, "filename");

        filename = GetValue("filename");
        if(std::string(filename).empty())
            throw std::invalid_argument("No image filename supplied");
        validate_hash_image_sequence_filecount(std::string(filename),
                                               GetIntValue("filecount", 1));

        const std::filesystem::path imagePath = ResolveCurrentFilename();
        const auto [width, height, channels] = image_get_info(imagePath);
        Debug("Checking \"" + imagePath.string() + "\" (width = " +
              std::to_string(width) + " height = " + std::to_string(height) +
              " channels = " + std::to_string(channels) + ")");

        if(width < 1 || height < 1)
            throw std::runtime_error("Image size could not be found in \"" +
                                     imagePath.string() + "\"");
        sizeX = width;
        sizeY = height;
    }

    void
    Init()
    {
        iteration = 1;
        currentImage = 0;

        Bind(iterations, "iterations");
        Bind(fileCount, "filecount");
        Bind(readOnce, "read_once");

        if(contains_hash_image_sequence_format(std::string(filename)))
            readOnce = false;
        first = true;

        Bind(intensity, "INTENSITY");
        Bind(output, "OUTPUT");
    }

    void
    Tick()
    {
        if(first || !readOnce)
        {
            first = false;
            std::string imageName = std::string(filename);
            try
            {
                const std::filesystem::path imagePath = ResolveCurrentFilename();
                imageName = imagePath.string();
                image_get_image(output, intensity, imagePath);
            }
            catch(const std::exception & error)
            {
                output.reset();
                intensity.reset();
                Warning("Could not read image \"" + imageName + "\": " + error.what(),
                        path_);
            }
        }

        currentImage++;
        if(currentImage >= fileCount)
        {
            currentImage = 0;
            iteration++;
        }

        if(iterations != 0 && iteration > iterations)
            Notify(msg_terminate, "End of image sequence");
    }
};

INSTALL_CLASS(InputImage)
