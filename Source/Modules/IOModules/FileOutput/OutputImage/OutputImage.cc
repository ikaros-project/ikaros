#include <algorithm>
#include <cctype>
#include <filesystem>
#include <limits>
#include <stdexcept>
#include <string>

#include "ikaros.h"
#include "../../FileInput/image_sequence.h"

using namespace ikaros;

class OutputImage : public Module
{
    matrix input;
    matrix write;

    parameter filename;
    parameter quality;
    parameter startIndex;
    parameter singleTrigger;

    int imageIndex = 0;
    bool previousWrite = false;
    bool sequenceExhausted = false;

    static void
    ValidateImageShape(const matrix & image)
    {
        if(image.rank() == 2 && image.size(0) > 0 && image.size(1) > 0)
            return;
        if(image.rank() == 3 && image.size(0) == 3 &&
           image.size(1) > 0 && image.size(2) > 0)
            return;
        throw std::invalid_argument(
            "OutputImage INPUT must have shape [height, width] or [3, height, width]");
    }


    static void
    ValidateExtension(const std::filesystem::path & path)
    {
        std::string extension = path.extension().string();
        std::transform(extension.begin(), extension.end(), extension.begin(),
                       [](unsigned char character)
                       {
                           return static_cast<char>(std::tolower(character));
                       });
        if(extension != ".jpg" && extension != ".jpeg" && extension != ".png")
            throw std::invalid_argument(
                "OutputImage filename must end in .jpg, .jpeg, or .png");
    }


    std::filesystem::path
    ResolveFilename()
    {
        const std::string formatted =
            format_hash_image_sequence_filename(filename.as_string(), imageIndex);
        std::filesystem::path result;
        if(!kernel().SanitizeWritePath(formatted, result))
            throw std::invalid_argument(
                "OutputImage can only write files inside UserData");
        return result;
    }


    bool
    ShouldWrite()
    {
        if(!write.connected())
            return true;

        const bool active = write(0) > 0.0f;
        const bool result = static_cast<bool>(singleTrigger) ?
                            active && !previousWrite : active;
        previousWrite = active;
        return result;
    }


    void
    AdvanceSequence()
    {
        if(!contains_hash_image_sequence_format(filename.as_string()))
            return;
        if(imageIndex == std::numeric_limits<int>::max())
        {
            sequenceExhausted = true;
            Warning("OutputImage sequence reached the largest supported index", path_);
            return;
        }
        ++imageIndex;
    }

public:
    void
    Init() override
    {
        Bind(input, "INPUT");
        Bind(write, "WRITE");
        Bind(filename, "filename");
        Bind(quality, "quality");
        Bind(startIndex, "start_index");
        Bind(singleTrigger, "single_trigger");

        if(filename.as_string().empty())
            throw std::invalid_argument("OutputImage filename must not be empty");
        if(write.connected() && write.size() != 1)
            throw std::invalid_argument("OutputImage WRITE must be a scalar");
        ValidateImageShape(input);

        imageIndex = startIndex.as_int();
        if(imageIndex < 0)
            throw std::invalid_argument("OutputImage start_index must not be negative");
        if(quality.as_int() < 1 || quality.as_int() > 100)
            throw std::invalid_argument("OutputImage quality must be between 1 and 100");

        const std::filesystem::path initialPath = ResolveFilename();
        ValidateExtension(initialPath);
    }


    void
    Tick() override
    {
        if(sequenceExhausted || !ShouldWrite())
            return;

        std::filesystem::path outputPath;
        try
        {
            outputPath = ResolveFilename();
        }
        catch(const std::out_of_range & error)
        {
            sequenceExhausted = true;
            Warning("OutputImage sequence stopped: " + std::string(error.what()), path_);
            return;
        }
        catch(const std::exception & error)
        {
            Warning("Could not resolve OutputImage filename: " +
                    std::string(error.what()), path_);
            return;
        }

        try
        {
            ValidateExtension(outputPath);
            image_write_image(input, outputPath, quality.as_int());
            AdvanceSequence();
        }
        catch(const std::exception & error)
        {
            Warning("Could not write image \"" + outputPath.string() + "\": " +
                    error.what(), path_);
        }
    }
};

INSTALL_CLASS(OutputImage)
