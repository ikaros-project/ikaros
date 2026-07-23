#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <limits>
#include <locale>
#include <stdexcept>
#include <string>
#include <system_error>

#include "ikaros.h"
#include "../../FileInput/image_sequence.h"


using namespace ikaros;


class OutputFile : public Module
{
    enum class TimestampMode
    {
        none,
        tick,
        time,
        realTime,
    };

    parameter directory;
    parameter filename;
    parameter format;
    parameter decimals;
    parameter timestamp;

    matrix input;
    matrix write;
    matrix newFile;

    std::ofstream file;
    std::filesystem::path outputDirectory;
    std::filesystem::path resolvedFilename;
    std::string columnSeparator;
    std::chrono::steady_clock::time_point realTimeOrigin;
    double realTimeTimestamp = 0.0;
    int fileIndex = 0;
    int decimalCount = 0;
    TimestampMode timestampMode = TimestampMode::time;
    bool previousNewFile = false;
    bool realTimeStarted = false;
    bool sequenceFilename = false;
    bool sequenceExhausted = false;
    bool writeFailed = false;
    bool warnedWithoutSequence = false;

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


    static std::string
    QuoteLabel(const std::string & label, char delimiter)
    {
        if(label.find(delimiter) == std::string::npos &&
           label.find('"') == std::string::npos &&
           label.find('\r') == std::string::npos &&
           label.find('\n') == std::string::npos)
            return label;

        std::string result;
        result.reserve(label.size() + 2);
        result += '"';
        for(char character : label)
        {
            if(character == '"')
                result += '"';
            result += character;
        }
        result += '"';
        return result;
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
                    "OutputFile directory must be inside UserData");

            std::error_code error;
            std::filesystem::create_directories(outputDirectory, error);
            if(error)
                throw std::runtime_error(
                    "Could not create OutputFile directory \"" +
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
                    "OutputFile directory must be inside UserData");

            std::error_code error;
            std::filesystem::create_directories(candidate.parent_path(), error);
            if(error)
                throw std::runtime_error(
                    "Could not create parent directory for OutputFile \"" +
                    candidate.string() + "\": " + error.message());

            error.clear();
            if(std::filesystem::create_directory(candidate, error))
            {
                outputDirectory = candidate;
                return;
            }
            if(error && error != std::errc::file_exists)
                throw std::runtime_error(
                    "Could not create OutputFile directory \"" +
                    candidate.string() + "\": " + error.message());
            if(index == std::numeric_limits<int>::max())
                break;
        }

        throw std::runtime_error(
            "Could not find an available number for OutputFile directory \"" +
            pattern + "\"");
    }


    std::filesystem::path
    ResolveFilename(int index)
    {
        const std::string formatted =
            format_hash_image_sequence_filename(filename.as_string(), index);
        std::filesystem::path candidate = formatted;

        if(!outputDirectory.empty())
        {
            if(candidate.is_absolute())
                throw std::invalid_argument(
                    "OutputFile filename must be relative when directory is set");
            candidate = outputDirectory / candidate;
        }

        std::filesystem::path result;
        if(!kernel().SanitizeWritePath(candidate, result))
            throw std::invalid_argument(
                "OutputFile can only write files inside UserData");
        if(!outputDirectory.empty() && !IsWithin(outputDirectory, result))
            throw std::invalid_argument(
                "OutputFile filename must stay inside its output directory");
        return result;
    }


    void
    WriteSeparator(bool & first)
    {
        if(!first)
            file << columnSeparator;
        first = false;
    }


    void
    FinishRecord(const std::string & description)
    {
        file.put('\n');
        file.flush();
        if(!file)
            throw std::runtime_error(
                "Could not write and flush OutputFile " + description +
                " to \"" + resolvedFilename.string() + "\"");
    }


    void
    WriteHeader()
    {
        const auto & labels = input.labels();
        if(timestampMode == TimestampMode::none && labels.empty())
            return;

        bool first = true;
        if(timestampMode != TimestampMode::none)
        {
            WriteSeparator(first);
            file << "T/1";
        }

        if(!labels.empty())
        {
            const char delimiter = columnSeparator == "\t" ? '\t' : ',';
            for(int i = 0; i < input.size(); ++i)
            {
                WriteSeparator(first);
                if(i < static_cast<int>(labels.size()))
                    file << QuoteLabel(labels[static_cast<std::size_t>(i)],
                                       delimiter);
            }
        }
        FinishRecord("header");
    }


    void
    WriteTimestamp()
    {
        switch(timestampMode)
        {
            case TimestampMode::none:
                break;
            case TimestampMode::tick:
                file << GetTick();
                break;
            case TimestampMode::time:
                file << formatNumber(GetNominalTime());
                break;
            case TimestampMode::realTime:
                file << formatNumber(realTimeTimestamp);
                break;
        }
    }


    void
    WriteRow()
    {
        bool first = true;
        if(timestampMode != TimestampMode::none)
        {
            WriteSeparator(first);
            WriteTimestamp();
        }

        for(int block = 0; block < input.logical_block_count(); ++block)
        {
            const float * values = input.logical_block_data(block);
            for(int i = 0; i < input.logical_block_size(); ++i)
            {
                WriteSeparator(first);
                file << values[i];
            }
        }
        FinishRecord("row");
    }


    void
    OpenFile(const std::filesystem::path & path)
    {
        resolvedFilename = path;
        if(!outputDirectory.empty())
        {
            std::error_code error;
            std::filesystem::create_directories(path.parent_path(), error);
            if(error)
                throw std::runtime_error(
                    "Could not create OutputFile subdirectory for \"" +
                    path.string() + "\": " + error.message());
        }

        file.clear();
        file.open(path, std::ios::out | std::ios::trunc);
        if(!file)
            throw std::runtime_error(
                "Could not open OutputFile \"" + path.string() + "\"");

        file.imbue(std::locale::classic());
        file << std::fixed << std::setprecision(decimalCount);
        try
        {
            WriteHeader();
        }
        catch(...)
        {
            file.close();
            throw;
        }
        writeFailed = false;
    }


    bool
    CloseFile(bool reportFailure)
    {
        if(!file.is_open())
            return true;

        file.flush();
        bool succeeded = static_cast<bool>(file);
        file.close();
        succeeded = succeeded && !file.fail();
        file.clear();

        if(!succeeded && reportFailure)
            Warning("Could not flush and close OutputFile \"" +
                    resolvedFilename.string() + "\"", path_);
        return succeeded;
    }


    bool
    ShouldWrite() const
    {
        return !write.connected() || write(0) > 0.0f;
    }


    bool
    NewFileRequested()
    {
        if(!newFile.connected())
            return false;

        const bool active = newFile(0) > 0.0f;
        const bool requested = active && !previousNewFile;
        previousNewFile = active;
        return requested;
    }


    void
    OpenNextFile()
    {
        if(!sequenceFilename)
        {
            if(!warnedWithoutSequence)
            {
                Warning("OutputFile NEWFILE was ignored because filename has "
                        "no # placeholder", path_);
                warnedWithoutSequence = true;
            }
            return;
        }
        if(sequenceExhausted)
            return;
        if(fileIndex == std::numeric_limits<int>::max())
        {
            sequenceExhausted = true;
            Warning("OutputFile sequence reached the largest supported index",
                    path_);
            return;
        }

        const int nextIndex = fileIndex + 1;
        std::filesystem::path nextFilename;
        try
        {
            nextFilename = ResolveFilename(nextIndex);
        }
        catch(const std::out_of_range & error)
        {
            sequenceExhausted = true;
            Warning("OutputFile sequence stopped: " + std::string(error.what()),
                    path_);
            return;
        }
        catch(const std::exception & error)
        {
            Warning("Could not resolve the next OutputFile filename: " +
                    std::string(error.what()), path_);
            return;
        }

        CloseFile(true);
        try
        {
            OpenFile(nextFilename);
            fileIndex = nextIndex;
        }
        catch(const std::exception & error)
        {
            writeFailed = true;
            Warning("Could not open the next OutputFile: " +
                    std::string(error.what()), path_);
        }
    }


    void
    Init() override
    {
        Bind(input, "INPUT");
        Bind(write, "WRITE");
        Bind(newFile, "NEWFILE");
        Bind(directory, "directory");
        Bind(filename, "filename");
        Bind(format, "format");
        Bind(decimals, "decimals");
        Bind(timestamp, "timestamp");

        if(filename.as_string().empty())
            throw std::invalid_argument(
                "OutputFile filename must not be empty");
        if(write.connected() && write.size() != 1)
            throw std::invalid_argument(
                "OutputFile WRITE must be a scalar");
        if(newFile.connected() && newFile.size() != 1)
            throw std::invalid_argument(
                "OutputFile NEWFILE must be a scalar");

        const std::string selectedFormat = format.as_string();
        if(selectedFormat == "csv")
            columnSeparator = ",";
        else if(selectedFormat == "tsv")
            columnSeparator = "\t";
        else
            throw std::invalid_argument(
                "OutputFile format must be \"csv\" or \"tsv\"");

        const double requestedDecimals = decimals.as_double();
        if(!std::isfinite(requestedDecimals) ||
           std::trunc(requestedDecimals) != requestedDecimals ||
           requestedDecimals < 0.0 || requestedDecimals > 20.0)
            throw std::invalid_argument(
                "OutputFile decimals must be an integer from 0 to 20");
        decimalCount = static_cast<int>(requestedDecimals);

        const std::string selectedTimestamp = timestamp.as_string();
        if(selectedTimestamp == "none")
            timestampMode = TimestampMode::none;
        else if(selectedTimestamp == "tick")
            timestampMode = TimestampMode::tick;
        else if(selectedTimestamp == "time")
            timestampMode = TimestampMode::time;
        else if(selectedTimestamp == "real_time")
            timestampMode = TimestampMode::realTime;
        else
            throw std::invalid_argument(
                "OutputFile timestamp must be \"none\", \"tick\", \"time\", "
                "or \"real_time\"");

        sequenceFilename =
            contains_hash_image_sequence_format(filename.as_string());
        ResolveOutputDirectory();
        OpenFile(ResolveFilename(fileIndex));
    }


    void
    Stop() override
    {
        CloseFile(true);
    }


    void
    Tick() override
    {
        if(timestampMode == TimestampMode::realTime)
        {
            const auto now = std::chrono::steady_clock::now();
            if(!realTimeStarted)
            {
                realTimeOrigin = now;
                realTimeStarted = true;
            }
            realTimeTimestamp =
                std::chrono::duration<double>(now - realTimeOrigin).count();
        }

        if(NewFileRequested())
            OpenNextFile();

        if(ShouldWrite() && !writeFailed && file.is_open())
        {
            try
            {
                WriteRow();
            }
            catch(const std::exception & error)
            {
                writeFailed = true;
                Warning(error.what(), path_);
                CloseFile(false);
            }
        }
    }
};


INSTALL_CLASS(OutputFile)
