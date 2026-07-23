#include <charconv>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <limits>
#include <locale>
#include <stdexcept>
#include <string>
#include <system_error>
#include <utility>

#include "ikaros.h"
#include "../../FileInput/image_sequence.h"


using namespace ikaros;


class OutputFile : public Module
{
    enum class TimestampMode
    {
        none,
        line,
        tick,
        time,
        realTime,
    };

    enum class ExistingFileMode
    {
        error,
        overwrite,
        append,
    };

    enum class NumberFormat
    {
        fixed,
        full,
    };

    struct ExistingFileInfo
    {
        bool exists = false;
        bool empty = true;
        bool endsWithNewline = true;
        std::uint64_t recordCount = 0;
        std::string firstRecord;
    };

    struct PreparedFile
    {
        std::ofstream stream;
        std::filesystem::path path;
        std::uint64_t lineNumber = 0;
    };

    parameter directory;
    parameter filename;
    parameter format;
    parameter delimiter;
    parameter numberFormat;
    parameter decimals;
    parameter timestamp;
    parameter existingFile;
    parameter startIndex;
    parameter flushInterval;

    matrix input;
    matrix write;
    matrix newFile;

    std::ofstream file;
    std::filesystem::path outputDirectory;
    std::filesystem::path resolvedFilename;
    char columnDelimiter = ',';
    std::uint64_t lineNumber = 0;
    std::uint64_t rowsSinceFlush = 0;
    std::uint64_t flushIntervalRows = 1;
    int fileIndex = 0;
    int decimalCount = 0;
    TimestampMode timestampMode = TimestampMode::time;
    ExistingFileMode existingFileMode = ExistingFileMode::error;
    NumberFormat dataNumberFormat = NumberFormat::fixed;
    bool previousNewFile = false;
    bool sequenceFilename = false;
    bool sequenceExhausted = false;
    bool writeFailed = false;
    bool warnedLineOverflow = false;
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


    static void
    IncrementRecordCount(std::uint64_t & count,
                         const std::filesystem::path & path)
    {
        if(count == std::numeric_limits<std::uint64_t>::max())
            throw std::runtime_error(
                "OutputFile record count overflow in \"" + path.string() +
                "\"");
        ++count;
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


    std::string
    TimestampHeader() const
    {
        switch(timestampMode)
        {
            case TimestampMode::none:
                return "";
            case TimestampMode::line:
                return "line/1";
            case TimestampMode::tick:
                return "tick/1";
            case TimestampMode::time:
                return "time/1";
            case TimestampMode::realTime:
                return "real_time/1";
        }
        throw std::logic_error("OutputFile has an invalid timestamp mode");
    }


    std::string
    HeaderRecord() const
    {
        const auto & labels = input.labels();
        const std::string timestampHeader = TimestampHeader();
        if(timestampHeader.empty() && labels.empty())
            return "";

        std::string result;
        bool first = true;
        auto appendField = [&](const std::string & value)
        {
            if(!first)
                result += columnDelimiter;
            result += value;
            first = false;
        };

        if(!timestampHeader.empty())
            appendField(timestampHeader);

        if(!labels.empty())
        {
            for(int i = 0; i < input.size(); ++i)
            {
                if(i < static_cast<int>(labels.size()))
                    appendField(
                        QuoteLabel(labels[static_cast<std::size_t>(i)],
                                   columnDelimiter));
                else
                    appendField("");
            }
        }
        return result;
    }


    ExistingFileInfo
    InspectExistingFile(const std::filesystem::path & path) const
    {
        ExistingFileInfo info;
        std::error_code error;
        info.exists = std::filesystem::exists(path, error);
        if(error)
            throw std::runtime_error(
                "Could not inspect OutputFile \"" + path.string() + "\": " +
                error.message());
        if(!info.exists)
            return info;

        if(!std::filesystem::is_regular_file(path, error) || error)
            throw std::runtime_error(
                "OutputFile path is not a regular file: \"" +
                path.string() + "\"");

        std::ifstream inputStream(path);
        if(!inputStream)
            throw std::runtime_error(
                "Could not inspect existing OutputFile \"" + path.string() +
                "\"");

        bool inQuotes = false;
        bool firstRecordComplete = false;
        bool hasCharacters = false;
        char lastCharacter = '\0';
        char character = '\0';
        while(inputStream.get(character))
        {
            hasCharacters = true;
            lastCharacter = character;

            if(character == '"')
            {
                if(inQuotes && inputStream.peek() == '"')
                {
                    if(!firstRecordComplete)
                        info.firstRecord += character;
                    inputStream.get(character);
                    lastCharacter = character;
                    if(!firstRecordComplete)
                        info.firstRecord += character;
                    continue;
                }
                inQuotes = !inQuotes;
            }

            if(character == '\n' && !inQuotes)
            {
                if(!firstRecordComplete)
                {
                    if(!info.firstRecord.empty() &&
                       info.firstRecord.back() == '\r')
                        info.firstRecord.pop_back();
                    firstRecordComplete = true;
                }
                IncrementRecordCount(info.recordCount, path);
            }
            else if(!firstRecordComplete)
                info.firstRecord += character;
        }

        if(inputStream.bad())
            throw std::runtime_error(
                "Could not read existing OutputFile \"" + path.string() +
                "\"");
        if(inQuotes)
            throw std::runtime_error(
                "Existing OutputFile has an unterminated quoted field: \"" +
                path.string() + "\"");

        info.empty = !hasCharacters;
        info.endsWithNewline = !hasCharacters || lastCharacter == '\n';
        if(hasCharacters && !info.endsWithNewline)
        {
            if(!firstRecordComplete && !info.firstRecord.empty() &&
               info.firstRecord.back() == '\r')
                info.firstRecord.pop_back();
            IncrementRecordCount(info.recordCount, path);
        }
        return info;
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
                "Could not create OutputFile subdirectory for \"" +
                path.string() + "\": " + error.message());
    }


    static void
    FlushPreparedStream(std::ofstream & stream,
                        const std::filesystem::path & path,
                        const std::string & description)
    {
        stream.flush();
        if(!stream)
            throw std::runtime_error(
                "Could not write and flush OutputFile " + description +
                " to \"" + path.string() + "\"");
    }


    PreparedFile
    PrepareFile(const std::filesystem::path & path)
    {
        EnsureParentDirectory(path);
        const ExistingFileInfo existing = InspectExistingFile(path);
        const std::string header = HeaderRecord();

        if(existingFileMode == ExistingFileMode::error && existing.exists)
            throw std::runtime_error(
                "OutputFile already exists: \"" + path.string() + "\"");

        PreparedFile prepared;
        prepared.path = path;
        if(existingFileMode == ExistingFileMode::append &&
           existing.exists && !existing.empty)
        {
            if(!header.empty())
            {
                if(existing.firstRecord != header)
                    throw std::runtime_error(
                        "Existing OutputFile header does not match the "
                        "configured columns: \"" + path.string() + "\"");
                prepared.lineNumber = existing.recordCount - 1;
            }
            else
                prepared.lineNumber = existing.recordCount;
        }

        std::ios::openmode mode = std::ios::out;
        if(existingFileMode == ExistingFileMode::append)
            mode |= std::ios::app;
        else
            mode |= std::ios::trunc;

        prepared.stream.imbue(std::locale::classic());
        prepared.stream.open(path, mode);
        if(!prepared.stream)
            throw std::runtime_error(
                "Could not open OutputFile \"" + path.string() + "\"");
        if(dataNumberFormat == NumberFormat::fixed)
            prepared.stream << std::fixed << std::setprecision(decimalCount);

        try
        {
            if(existingFileMode == ExistingFileMode::append &&
               existing.exists && !existing.empty &&
               !existing.endsWithNewline)
            {
                prepared.stream.put('\n');
                FlushPreparedStream(prepared.stream, path,
                                    "record separator");
            }

            if((!existing.exists || existing.empty ||
                existingFileMode != ExistingFileMode::append) &&
               !header.empty())
            {
                prepared.stream << header;
                prepared.stream.put('\n');
                FlushPreparedStream(prepared.stream, path, "header");
            }
        }
        catch(...)
        {
            prepared.stream.close();
            throw;
        }
        return prepared;
    }


    void
    ActivatePreparedFile(PreparedFile && prepared, int index)
    {
        file = std::move(prepared.stream);
        resolvedFilename = std::move(prepared.path);
        fileIndex = index;
        lineNumber = prepared.lineNumber;
        rowsSinceFlush = 0;
        warnedLineOverflow = false;
        writeFailed = false;
    }


    void
    WriteSeparator(bool & first)
    {
        if(!first)
            file.put(columnDelimiter);
        first = false;
    }


    void
    FlushRows()
    {
        file.flush();
        if(!file)
            throw std::runtime_error(
                "Could not flush OutputFile rows to \"" +
                resolvedFilename.string() + "\"");
        rowsSinceFlush = 0;
    }


    void
    FinishRow()
    {
        file.put('\n');
        if(!file)
            throw std::runtime_error(
                "Could not write OutputFile row to \"" +
                resolvedFilename.string() + "\"");

        if(rowsSinceFlush != std::numeric_limits<std::uint64_t>::max())
            ++rowsSinceFlush;
        if(flushIntervalRows > 0 && rowsSinceFlush >= flushIntervalRows)
            FlushRows();
    }


    void
    WriteTimestamp()
    {
        switch(timestampMode)
        {
            case TimestampMode::none:
                break;
            case TimestampMode::line:
                file << lineNumber;
                break;
            case TimestampMode::tick:
                file << GetTick();
                break;
            case TimestampMode::time:
                file << formatNumber(GetNominalTime());
                break;
            case TimestampMode::realTime:
                file << formatNumber(GetRunTime());
                break;
        }
    }


    void
    WriteValue(float value)
    {
        if(dataNumberFormat == NumberFormat::fixed)
        {
            file << value;
            return;
        }

        char buffer[64];
        const auto result =
            std::to_chars(buffer, buffer + sizeof(buffer), value);
        if(result.ec != std::errc())
            throw std::runtime_error(
                "Could not format a full-resolution OutputFile value");
        file.write(buffer, result.ptr - buffer);
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
                WriteValue(values[i]);
            }
        }
        FinishRow();
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
        rowsSinceFlush = 0;

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

        try
        {
            PreparedFile prepared = PrepareFile(nextFilename);
            CloseFile(true);
            ActivatePreparedFile(std::move(prepared), nextIndex);
        }
        catch(const std::exception & error)
        {
            Warning("Could not open the next OutputFile; continuing with \"" +
                    resolvedFilename.string() + "\": " + error.what(), path_);
        }
    }


    void
    ParseOptions()
    {
        const std::string selectedFormat = format.as_string();
        if(selectedFormat == "csv")
            columnDelimiter = ',';
        else if(selectedFormat == "tsv")
            columnDelimiter = '\t';
        else
            throw std::invalid_argument(
                "OutputFile format must be \"csv\" or \"tsv\"");

        const std::string selectedDelimiter = delimiter.as_string();
        if(!selectedDelimiter.empty())
        {
            if(selectedDelimiter.size() != 1)
                throw std::invalid_argument(
                    "OutputFile delimiter must be exactly one character");

            const char requestedDelimiter = selectedDelimiter.front();
            if(requestedDelimiter == '\0' || requestedDelimiter == '\r' ||
               requestedDelimiter == '\n' || requestedDelimiter == '"')
                throw std::invalid_argument(
                    "OutputFile delimiter cannot be a line break, NUL, or "
                    "quotation mark");
            columnDelimiter = requestedDelimiter;
        }

        const std::string selectedNumberFormat = numberFormat.as_string();
        if(selectedNumberFormat == "fixed")
            dataNumberFormat = NumberFormat::fixed;
        else if(selectedNumberFormat == "full")
            dataNumberFormat = NumberFormat::full;
        else
            throw std::invalid_argument(
                "OutputFile number_format must be \"fixed\" or \"full\"");

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
        else if(selectedTimestamp == "line")
            timestampMode = TimestampMode::line;
        else if(selectedTimestamp == "tick")
            timestampMode = TimestampMode::tick;
        else if(selectedTimestamp == "time")
            timestampMode = TimestampMode::time;
        else if(selectedTimestamp == "real_time")
            timestampMode = TimestampMode::realTime;
        else
            throw std::invalid_argument(
                "OutputFile timestamp must be \"none\", \"line\", \"tick\", "
                "\"time\", or \"real_time\"");

        const std::string selectedExistingFile = existingFile.as_string();
        if(selectedExistingFile == "error")
            existingFileMode = ExistingFileMode::error;
        else if(selectedExistingFile == "overwrite")
            existingFileMode = ExistingFileMode::overwrite;
        else if(selectedExistingFile == "append")
            existingFileMode = ExistingFileMode::append;
        else
            throw std::invalid_argument(
                "OutputFile existing_file must be \"error\", \"overwrite\", "
                "or \"append\"");

        const double requestedStartIndex = startIndex.as_double();
        if(!std::isfinite(requestedStartIndex) ||
           std::trunc(requestedStartIndex) != requestedStartIndex ||
           requestedStartIndex < 0.0 ||
           requestedStartIndex >
               static_cast<double>(std::numeric_limits<int>::max()))
            throw std::invalid_argument(
                "OutputFile start_index must be a non-negative integer");
        fileIndex = static_cast<int>(requestedStartIndex);

        const double requestedFlushInterval = flushInterval.as_double();
        if(!std::isfinite(requestedFlushInterval) ||
           std::trunc(requestedFlushInterval) != requestedFlushInterval ||
           requestedFlushInterval < 0.0 ||
           requestedFlushInterval >
               static_cast<double>(std::numeric_limits<int>::max()))
            throw std::invalid_argument(
                "OutputFile flush_interval must be an integer from 0 to " +
                std::to_string(std::numeric_limits<int>::max()));
        flushIntervalRows =
            static_cast<std::uint64_t>(requestedFlushInterval);
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
        Bind(delimiter, "delimiter");
        Bind(numberFormat, "number_format");
        Bind(decimals, "decimals");
        Bind(timestamp, "timestamp");
        Bind(existingFile, "existing_file");
        Bind(startIndex, "start_index");
        Bind(flushInterval, "flush_interval");

        if(filename.as_string().empty())
            throw std::invalid_argument(
                "OutputFile filename must not be empty");
        if(write.connected() && write.size() != 1)
            throw std::invalid_argument(
                "OutputFile WRITE must be a scalar");
        if(newFile.connected() && newFile.size() != 1)
            throw std::invalid_argument(
                "OutputFile NEWFILE must be a scalar");

        ParseOptions();
        sequenceFilename =
            contains_hash_image_sequence_format(filename.as_string());
        ResolveOutputDirectory();
        PreparedFile prepared = PrepareFile(ResolveFilename(fileIndex));
        ActivatePreparedFile(std::move(prepared), fileIndex);
    }


    void
    Stop() override
    {
        CloseFile(true);
    }


    void
    Tick() override
    {
        if(NewFileRequested())
            OpenNextFile();

        if(ShouldWrite() && !writeFailed && file.is_open())
        {
            try
            {
                WriteRow();
                if(timestampMode == TimestampMode::line)
                {
                    if(lineNumber !=
                       std::numeric_limits<std::uint64_t>::max())
                        ++lineNumber;
                    else if(!warnedLineOverflow)
                    {
                        Warning("OutputFile line number reached its largest "
                                "supported value", path_);
                        warnedLineOverflow = true;
                    }
                }
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
