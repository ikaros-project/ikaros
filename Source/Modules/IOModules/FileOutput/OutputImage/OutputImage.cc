#include <chrono>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>

#include "ikaros.h"
#include "../../FileInput/image_sequence.h"

using namespace ikaros;

class OutputImage : public Module
{
    matrix input;
    matrix write;

    parameter directory;
    parameter filename;
    parameter quality;
    parameter startIndex;
    parameter singleTrigger;

    static constexpr std::chrono::seconds failureRetryDelay{1};

    std::filesystem::path outputDirectory;
    std::string temporaryFileTag;
    int imageIndex = 0;
    bool previousWrite = false;
    bool writeActivated = false;
    bool sequenceExhausted = false;
    bool writeFailure = false;
    std::chrono::steady_clock::time_point nextFailureRetry;
    std::string lastWriteError;

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
    ValidateFormat(const std::filesystem::path & path)
    {
        validate_image_file_format(path);
    }


    static std::uint64_t
    StableTag(std::string_view text) noexcept
    {
        std::uint64_t result = 14695981039346656037ULL;
        for(unsigned char character : text)
        {
            result ^= character;
            result *= 1099511628211ULL;
        }
        return result;
    }


    static bool
    IsWithin(const std::filesystem::path & root,
             const std::filesystem::path & path)
    {
        auto rootIterator = root.begin();
        const auto rootEnd = root.end();
        auto pathIterator = path.begin();
        const auto pathEnd = path.end();

        for(; rootIterator != rootEnd && pathIterator != pathEnd;
            ++rootIterator, ++pathIterator)
            if(*rootIterator != *pathIterator)
                return false;
        return rootIterator == rootEnd;
    }


    void
    ResolveOutputDirectory()
    {
        if(directory.as_string().empty())
            return;

        const std::string pattern = directory.as_string();
        const bool numbered =
            contains_hash_image_sequence_format(pattern);
        if(!numbered)
        {
            const std::string formatted =
                format_hash_image_sequence_filename(pattern, 0);
            if(!kernel().SanitizeWritePath(formatted, outputDirectory))
                throw std::invalid_argument(
                    "OutputImage directory must be inside UserData");

            std::error_code error;
            std::filesystem::create_directories(outputDirectory, error);
            if(error)
                throw std::runtime_error(
                    "Could not create OutputImage directory \"" +
                    outputDirectory.string() + "\": " + error.message());
            return;
        }

        for(int index = 0; ; ++index)
        {
            std::string formatted;
            try
            {
                formatted =
                    format_hash_image_sequence_filename(pattern, index);
            }
            catch(const std::out_of_range &)
            {
                break;
            }

            std::filesystem::path candidate;
            if(!kernel().SanitizeWritePath(formatted, candidate))
                throw std::invalid_argument(
                    "OutputImage directory must be inside UserData");

            std::error_code error;
            std::filesystem::create_directories(candidate.parent_path(), error);
            if(error)
                throw std::runtime_error(
                    "Could not create parent directory for OutputImage \"" +
                    candidate.string() + "\": " + error.message());

            error.clear();
            if(std::filesystem::create_directory(candidate, error))
            {
                outputDirectory = candidate;
                return;
            }
            if(error && error != std::errc::file_exists)
                throw std::runtime_error(
                    "Could not create OutputImage directory \"" +
                    candidate.string() + "\": " + error.message());
            if(index == std::numeric_limits<int>::max())
                break;
        }

        throw std::runtime_error(
            "Could not find an available number for OutputImage directory \"" +
            pattern + "\"");
    }


    std::filesystem::path
    ResolveFilename()
    {
        const std::string formatted =
            format_hash_image_sequence_filename(filename.as_string(), imageIndex);
        std::filesystem::path candidate = formatted;
        if(!outputDirectory.empty())
        {
            if(candidate.is_absolute())
                throw std::invalid_argument(
                    "OutputImage filename must be relative when directory is set");
            candidate = outputDirectory / candidate;
        }

        std::filesystem::path result;
        if(!kernel().SanitizeWritePath(candidate, result))
            throw std::invalid_argument(
                "OutputImage can only write files inside UserData");
        if(!outputDirectory.empty() && !IsWithin(outputDirectory, result))
            throw std::invalid_argument(
                "OutputImage filename must stay inside its output directory");
        return result;
    }


    void
    EnsureParentDirectory(const std::filesystem::path & path) const
    {
        if(outputDirectory.empty())
            return;

        std::error_code error;
        std::filesystem::create_directories(path.parent_path(), error);
        if(error)
            throw std::runtime_error(
                "Could not create OutputImage subdirectory for \"" +
                path.string() + "\": " + error.message());
    }


    std::filesystem::path
    TemporaryFilename(const std::filesystem::path & outputPath) const
    {
        std::filesystem::path name(
            ".ikaros-" + temporaryFileTag + "-");
        name += outputPath.filename().native();
        return outputPath.parent_path() / name;
    }


    static void
    ReplaceFileAtomically(const std::filesystem::path & source,
                          const std::filesystem::path & target)
    {
        std::error_code error;
        std::filesystem::rename(source, target, error);
        if(error)
            throw std::system_error(
                error, "Could not atomically replace image \"" +
                           target.string() + "\"");
    }


    void
    WriteImageAtomically(const std::filesystem::path & outputPath)
    {
        const std::filesystem::path temporaryPath =
            TemporaryFilename(outputPath);
        std::error_code error;
        std::filesystem::remove(temporaryPath, error);
        if(error)
            throw std::runtime_error(
                "Could not remove temporary image \"" +
                temporaryPath.string() + "\": " + error.message());

        try
        {
            image_write_image(input, temporaryPath, quality.as_int());
            ReplaceFileAtomically(temporaryPath, outputPath);
        }
        catch(...)
        {
            error.clear();
            std::filesystem::remove(temporaryPath, error);
            throw;
        }
    }


    bool
    ShouldWrite()
    {
        if(!write.connected())
        {
            writeActivated = false;
            return true;
        }

        const bool active = write(0) > 0.0f;
        writeActivated = active && !previousWrite;
        const bool result = static_cast<bool>(singleTrigger) ?
                            writeActivated : active;
        previousWrite = active;
        return result;
    }


    bool
    CanAttemptWrite() const
    {
        return !writeFailure || writeActivated ||
               std::chrono::steady_clock::now() >= nextFailureRetry;
    }


    void
    ReportWriteFailure(const std::string & message)
    {
        const auto now = std::chrono::steady_clock::now();
        if(!writeFailure || message != lastWriteError ||
           now >= nextFailureRetry)
            Warning(message, path_);

        writeFailure = true;
        lastWriteError = message;
        nextFailureRetry = now + failureRetryDelay;
    }


    void
    ClearWriteFailure()
    {
        writeFailure = false;
        lastWriteError.clear();
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
        Bind(directory, "directory");
        Bind(filename, "filename");
        Bind(quality, "quality");
        Bind(startIndex, "start_index");
        Bind(singleTrigger, "single_trigger");

        if(filename.as_string().empty())
            throw std::invalid_argument("OutputImage filename must not be empty");
        if(write.connected() && write.size() != 1)
            throw std::invalid_argument("OutputImage WRITE must be a scalar");
        ValidateImageShape(input);

        const double requestedStartIndex = startIndex.as_double();
        if(!std::isfinite(requestedStartIndex) ||
           std::trunc(requestedStartIndex) != requestedStartIndex ||
           requestedStartIndex < 0.0 ||
           requestedStartIndex >
               static_cast<double>(std::numeric_limits<int>::max()))
            throw std::invalid_argument(
                "OutputImage start_index must be a non-negative integer");
        imageIndex = static_cast<int>(requestedStartIndex);
        if(quality.as_int() < 1 || quality.as_int() > 100)
            throw std::invalid_argument("OutputImage quality must be between 1 and 100");

        temporaryFileTag = std::to_string(StableTag(path_));
        ResolveOutputDirectory();
        const std::filesystem::path initialPath = ResolveFilename();
        EnsureParentDirectory(initialPath);
        ValidateFormat(initialPath);
    }


    void
    Tick() override
    {
        if(sequenceExhausted || !ShouldWrite())
            return;
        if(!CanAttemptWrite())
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
            ReportWriteFailure(
                "Could not resolve OutputImage filename: " +
                std::string(error.what()));
            return;
        }

        try
        {
            EnsureParentDirectory(outputPath);
            ValidateFormat(outputPath);
            WriteImageAtomically(outputPath);
            ClearWriteFailure();
            AdvanceSequence();
        }
        catch(const std::exception & error)
        {
            ReportWriteFailure(
                "Could not write image \"" + outputPath.string() + "\": " +
                error.what());
        }
    }
};

INSTALL_CLASS(OutputImage)
