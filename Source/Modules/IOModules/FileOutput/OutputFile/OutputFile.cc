#include <algorithm>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <limits>
#include <locale>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <unordered_set>
#include <utility>
#include <vector>

#include "ikaros.h"
#include "../../FileInput/image_sequence.h"


using namespace ikaros;


class OutputFile : public Module
{
    enum class OutputFormat
    {
        delimited,
        jsonArray,
        jsonLines,
    };

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
        std::uintmax_t jsonArrayAppendOffset = 0;
        std::string firstRecord;
    };

    struct PreparedFile
    {
        std::ofstream stream;
        std::filesystem::path path;
        std::uint64_t lineNumber = 0;
        std::uint64_t jsonRecordCount = 0;
    };

    struct JSONField
    {
        std::string label;
        std::string escapedLabel;
        std::vector<int> shape;
        int offset = 0;
        int valueCount = 0;
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
    parameter header;
    parameter singleTrigger;

    matrix input;
    matrix write;
    matrix newFile;

    std::ofstream file;
    std::filesystem::path outputDirectory;
    std::filesystem::path resolvedFilename;
    char columnDelimiter = ',';
    std::uint64_t lineNumber = 0;
    std::uint64_t jsonRecordCount = 0;
    std::uint64_t rowsSinceFlush = 0;
    std::uint64_t flushIntervalRows = 1;
    int fileIndex = 0;
    int decimalCount = 0;
    OutputFormat outputFormat = OutputFormat::delimited;
    TimestampMode timestampMode = TimestampMode::time;
    ExistingFileMode existingFileMode = ExistingFileMode::error;
    NumberFormat dataNumberFormat = NumberFormat::fixed;
    std::vector<JSONField> jsonFields;
    std::string jsonSchemaError;
    std::string recordBuffer;
    bool previousWrite = false;
    bool previousNewFile = false;
    bool sequenceFilename = false;
    bool sequenceExhausted = false;
    bool writeFailed = false;
    bool warnedLineOverflow = false;
    bool warnedWithoutSequence = false;
    bool includeHeader = true;

    bool
    IsJSONOutput() const
    {
        return outputFormat == OutputFormat::jsonArray ||
               outputFormat == OutputFormat::jsonLines;
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


    static bool
    IsJSONWhitespace(char character)
    {
        return character == ' ' || character == '\t' ||
               character == '\r' || character == '\n';
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


    std::string_view
    TimestampKey() const
    {
        switch(timestampMode)
        {
            case TimestampMode::none:
                return "";
            case TimestampMode::line:
                return "line";
            case TimestampMode::tick:
                return "tick";
            case TimestampMode::time:
                return "time";
            case TimestampMode::realTime:
                return "real_time";
        }
        throw std::logic_error("OutputFile has an invalid timestamp mode");
    }


    std::string
    HeaderRecord() const
    {
        if(IsJSONOutput() || !includeHeader)
            return "";

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


    std::vector<std::string>
    JSONSchemaKeys() const
    {
        std::vector<std::string> result;
        result.reserve(jsonFields.size() + 1);

        const std::string_view timestampKey = TimestampKey();
        if(!timestampKey.empty())
            result.emplace_back(timestampKey);
        for(const JSONField & field : jsonFields)
            result.push_back(field.label);
        return result;
    }


    static bool
    JSONValueMatchesShape(const value & candidate,
                          const std::vector<int> & shape,
                          std::size_t dimension)
    {
        if(dimension == shape.size())
            return candidate.is_number() || candidate.is_null();
        if(!candidate.is_list())
            return false;

        const list & values = candidate.as_list();
        if(values.size() !=
           static_cast<std::size_t>(shape[dimension]))
            return false;
        for(const value & element : values)
            if(!JSONValueMatchesShape(
                   element, shape, dimension + 1))
                return false;
        return true;
    }


    void
    ValidateJSONObject(
        const dictionary & object,
        const std::vector<std::string> & expectedKeys) const
    {
        const std::size_t keyCount =
            static_cast<std::size_t>(
                std::distance(object.begin(), object.end()));
        if(keyCount != expectedKeys.size())
            throw std::runtime_error(
                "record keys do not match the configured fields");
        for(const std::string & key : expectedKeys)
            if(!object.contains(key))
                throw std::runtime_error(
                    "record is missing key \"" + key + "\"");

        const std::string_view timestampKey = TimestampKey();
        if(!timestampKey.empty() &&
           !object.at(std::string(timestampKey)).is_number())
            throw std::runtime_error(
                "timestamp field is not numeric");
        for(const JSONField & field : jsonFields)
            if(!JSONValueMatchesShape(
                   object.at(field.label), field.shape, 0))
                throw std::runtime_error(
                    "field \"" + field.label +
                    "\" does not match its configured shape");
    }


    ExistingFileInfo
    InspectExistingJSONLines(const std::filesystem::path & path,
                             ExistingFileInfo info) const
    {
        std::ifstream inputStream(path, std::ios::binary);
        if(!inputStream)
            throw std::runtime_error(
                "Could not inspect existing OutputFile \"" + path.string() +
                "\"");

        std::error_code error;
        const std::uintmax_t fileSize =
            std::filesystem::file_size(path, error);
        if(error)
            throw std::runtime_error(
                "Could not inspect existing OutputFile \"" + path.string() +
                "\": " + error.message());

        info.empty = fileSize == 0;
        if(info.empty)
            return info;

        inputStream.seekg(-1, std::ios::end);
        char lastCharacter = '\0';
        inputStream.get(lastCharacter);
        if(!inputStream)
            throw std::runtime_error(
                "Could not inspect the end of existing OutputFile \"" +
                path.string() + "\"");
        info.endsWithNewline = lastCharacter == '\n';
        inputStream.clear();
        inputStream.seekg(0);

        const std::vector<std::string> expectedKeys = JSONSchemaKeys();
        std::string line;
        std::uint64_t physicalLine = 0;
        while(std::getline(inputStream, line))
        {
            IncrementRecordCount(physicalLine, path);
            if(!line.empty() && line.back() == '\r')
                line.pop_back();
            if(line.empty())
                throw std::runtime_error(
                    "Existing OutputFile contains an empty JSONL record at "
                    "line " + std::to_string(physicalLine) + ": \"" +
                    path.string() + "\"");

            try
            {
                const value parsed = parse_json(line);
                if(!parsed.is_dictionary())
                    throw std::runtime_error(
                        "record is not a JSON object");
                ValidateJSONObject(
                    parsed.as_dictionary(), expectedKeys);
            }
            catch(const std::exception & exception)
            {
                throw std::runtime_error(
                    "Existing OutputFile has an invalid JSONL record at line " +
                    std::to_string(physicalLine) + ": " + exception.what() +
                    ": \"" + path.string() + "\"");
            }

            IncrementRecordCount(info.recordCount, path);
        }

        if(inputStream.bad())
            throw std::runtime_error(
                "Could not read existing OutputFile \"" + path.string() +
                "\"");
        return info;
    }


    ExistingFileInfo
    InspectExistingJSONArray(const std::filesystem::path & path,
                             ExistingFileInfo info) const
    {
        std::ifstream inputStream(path, std::ios::binary);
        if(!inputStream)
            throw std::runtime_error(
                "Could not inspect existing OutputFile \"" + path.string() +
                "\"");

        try
        {
            std::uintmax_t offset = 0;
            char lastCharacter = '\0';
            auto readCharacter = [&](char & character)
            {
                if(!inputStream.get(character))
                    return false;
                if(offset == std::numeric_limits<std::uintmax_t>::max())
                    throw std::runtime_error(
                        "file is too large to inspect");
                ++offset;
                lastCharacter = character;
                return true;
            };

            char character = '\0';
            while(readCharacter(character) &&
                  IsJSONWhitespace(character))
            {
            }
            info.empty = offset == 0;
            if(info.empty)
                return info;
            if(!inputStream && inputStream.bad())
                throw std::runtime_error(
                    "could not read the file");
            if(character != '[')
                throw std::runtime_error(
                    "top-level value is not an array");

            const std::vector<std::string> expectedKeys = JSONSchemaKeys();
            const std::uintmax_t arrayContentOffset = offset;
            std::uintmax_t appendOffset = arrayContentOffset;
            bool closingBracketAllowed = true;
            bool arrayComplete = false;

            while(!arrayComplete)
            {
                while(readCharacter(character) &&
                      IsJSONWhitespace(character))
                {
                }
                if(!inputStream && inputStream.bad())
                    throw std::runtime_error(
                        "could not read the file");
                if(!inputStream)
                    throw std::runtime_error(
                        "array has no closing bracket");
                if(character == ']')
                {
                    if(!closingBracketAllowed)
                        throw std::runtime_error(
                            "array has a trailing comma");
                    arrayComplete = true;
                    break;
                }
                if(character == ',')
                    throw std::runtime_error(
                        "array contains an empty element");

                std::string record;
                std::size_t nestingDepth = 0;
                bool inString = false;
                bool escaped = false;
                char boundary = '\0';

                while(true)
                {
                    if(!inString && nestingDepth == 0 &&
                       (character == ',' || character == ']'))
                    {
                        boundary = character;
                        break;
                    }

                    const bool topLevelWhitespace =
                        !inString && nestingDepth == 0 &&
                        IsJSONWhitespace(character);
                    if(!topLevelWhitespace)
                    {
                        record += character;
                        appendOffset = offset;
                    }

                    if(inString)
                    {
                        if(escaped)
                            escaped = false;
                        else if(character == '\\')
                            escaped = true;
                        else if(character == '"')
                            inString = false;
                    }
                    else if(character == '"')
                        inString = true;
                    else if(character == '{' || character == '[')
                        ++nestingDepth;
                    else if(character == '}' || character == ']')
                    {
                        if(nestingDepth == 0)
                            throw std::runtime_error(
                                "array element has unmatched brackets");
                        --nestingDepth;
                    }

                    if(!readCharacter(character))
                    {
                        if(inputStream.bad())
                            throw std::runtime_error(
                                "could not read the file");
                        throw std::runtime_error(
                            "array has no closing bracket");
                    }
                }

                const value parsed = parse_json(record);
                if(!parsed.is_dictionary())
                    throw std::runtime_error(
                        "array element " +
                        std::to_string(info.recordCount) +
                        " is not a JSON object");
                ValidateJSONObject(parsed.as_dictionary(), expectedKeys);
                IncrementRecordCount(info.recordCount, path);

                if(boundary == ']')
                    arrayComplete = true;
                else
                    closingBracketAllowed = false;
            }

            while(readCharacter(character))
                if(!IsJSONWhitespace(character))
                    throw std::runtime_error(
                        "unexpected characters after the array");
            if(inputStream.bad())
                throw std::runtime_error(
                    "could not read the file");

            info.endsWithNewline = lastCharacter == '\n';
            info.jsonArrayAppendOffset = appendOffset;
        }
        catch(const std::exception & exception)
        {
            throw std::runtime_error(
                "Existing OutputFile is not a valid compatible JSON array: " +
                std::string(exception.what()) + ": \"" + path.string() +
                "\"");
        }
        return info;
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

        if(existingFileMode != ExistingFileMode::append)
            return info;
        if(outputFormat == OutputFormat::jsonArray)
            return InspectExistingJSONArray(path, std::move(info));
        if(outputFormat == OutputFormat::jsonLines)
            return InspectExistingJSONLines(path, std::move(info));

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
        const std::string headerRecord = HeaderRecord();

        if(existingFileMode == ExistingFileMode::error && existing.exists)
            throw std::runtime_error(
                "OutputFile already exists: \"" + path.string() + "\"");

        PreparedFile prepared;
        prepared.path = path;
        if(existingFileMode == ExistingFileMode::append &&
           existing.exists && !existing.empty)
        {
            if(outputFormat == OutputFormat::jsonArray)
            {
                prepared.lineNumber = existing.recordCount;
                prepared.jsonRecordCount = existing.recordCount;

                std::error_code error;
                std::filesystem::resize_file(
                    path, existing.jsonArrayAppendOffset, error);
                if(error)
                    throw std::runtime_error(
                        "Could not reopen existing OutputFile JSON array \"" +
                        path.string() + "\": " + error.message());
            }
            else if(!headerRecord.empty())
            {
                if(existing.firstRecord != headerRecord)
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
        try
        {
            if(outputFormat == OutputFormat::jsonArray)
            {
                if(existingFileMode != ExistingFileMode::append ||
                   !existing.exists || existing.empty)
                {
                    prepared.stream.put('[');
                    FlushPreparedStream(
                        prepared.stream, path, "JSON array opening");
                }
            }
            else
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
                   !headerRecord.empty())
                {
                    prepared.stream << headerRecord;
                    prepared.stream.put('\n');
                    FlushPreparedStream(
                        prepared.stream, path, "header");
                }
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
        jsonRecordCount = prepared.jsonRecordCount;
        rowsSinceFlush = 0;
        warnedLineOverflow = false;
        writeFailed = false;
    }


    void
    WriteSeparator(bool & first)
    {
        if(!first)
            recordBuffer += columnDelimiter;
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
    FinishRecord(bool addNewline)
    {
        if(addNewline)
            recordBuffer += '\n';
        if(recordBuffer.size() >
           static_cast<std::size_t>(
               std::numeric_limits<std::streamsize>::max()))
            throw std::length_error(
                "OutputFile row is too large to write to \"" +
                resolvedFilename.string() + "\"");
        file.write(
            recordBuffer.data(),
            static_cast<std::streamsize>(recordBuffer.size()));
        if(!file)
            throw std::runtime_error(
                "Could not write OutputFile row to \"" +
                resolvedFilename.string() + "\"");
        recordBuffer.clear();

        if(rowsSinceFlush != std::numeric_limits<std::uint64_t>::max())
            ++rowsSinceFlush;
        if(flushIntervalRows > 0 && rowsSinceFlush >= flushIntervalRows)
            FlushRows();
    }


    void
    FinishRow()
    {
        FinishRecord(true);
    }


    template<typename Integer>
    void
    WriteInteger(Integer value)
    {
        char buffer[32];
        const auto result =
            std::to_chars(buffer, buffer + sizeof(buffer), value);
        if(result.ec != std::errc())
            throw std::runtime_error(
                "Could not format an OutputFile integer");
        recordBuffer.append(
            buffer, static_cast<std::size_t>(result.ptr - buffer));
    }


    void
    WriteTimestamp()
    {
        switch(timestampMode)
        {
            case TimestampMode::none:
                break;
            case TimestampMode::line:
                WriteInteger(lineNumber);
                break;
            case TimestampMode::tick:
                WriteInteger(GetTick());
                break;
            case TimestampMode::time:
                recordBuffer += formatNumber(GetNominalTime());
                break;
            case TimestampMode::realTime:
                recordBuffer += formatNumber(GetRunTime());
                break;
        }
    }


    void
    WriteValue(float value)
    {
        char buffer[128];
        const auto result = dataNumberFormat == NumberFormat::fixed ?
            std::to_chars(buffer, buffer + sizeof(buffer), value,
                          std::chars_format::fixed, decimalCount) :
            std::to_chars(buffer, buffer + sizeof(buffer), value);
        if(result.ec != std::errc())
            throw std::runtime_error(
                "Could not format an OutputFile value");
        recordBuffer.append(
            buffer, static_cast<std::size_t>(result.ptr - buffer));
    }


    void
    WriteJSONNumber(float value)
    {
        if(!std::isfinite(value))
        {
            recordBuffer += "null";
            return;
        }
        WriteValue(value);
    }


    void
    WriteJSONArray(const float * values, int & offset,
                   const std::vector<int> & shape, int dimension)
    {
        recordBuffer += '[';
        for(int i = 0; i < shape[static_cast<std::size_t>(dimension)]; ++i)
        {
            if(i > 0)
                recordBuffer += ',';
            if(dimension + 1 == static_cast<int>(shape.size()))
                WriteJSONNumber(values[offset++]);
            else
                WriteJSONArray(values, offset, shape, dimension + 1);
        }
        recordBuffer += ']';
    }


    void
    WriteJSONFieldPrefix(bool & first, std::string_view escapedKey)
    {
        if(!first)
            recordBuffer += ',';
        recordBuffer += '"';
        recordBuffer.append(escapedKey.data(), escapedKey.size());
        recordBuffer += "\":";
        first = false;
    }


    void
    WriteJSONObject()
    {
        const float * values = input.contiguous_data();
        bool first = true;
        recordBuffer += '{';

        const std::string_view timestampKey = TimestampKey();
        if(!timestampKey.empty())
        {
            WriteJSONFieldPrefix(first, timestampKey);
            WriteTimestamp();
        }

        for(const JSONField & field : jsonFields)
        {
            WriteJSONFieldPrefix(first, field.escapedLabel);
            int offset = field.offset;
            if(field.shape.empty())
                WriteJSONNumber(values[offset++]);
            else
                WriteJSONArray(values, offset, field.shape, 0);
            if(offset != field.offset + field.valueCount)
                throw std::logic_error(
                    "OutputFile JSON field shape does not match its input");
        }

        recordBuffer += '}';
    }


    void
    WriteJSONRow()
    {
        if(outputFormat == OutputFormat::jsonArray)
        {
            if(jsonRecordCount ==
               std::numeric_limits<std::uint64_t>::max())
                throw std::runtime_error(
                    "OutputFile JSON array record count overflow");

            if(jsonRecordCount == 0)
                recordBuffer += '\n';
            else
                recordBuffer += ",\n";
            WriteJSONObject();
            FinishRecord(false);
            ++jsonRecordCount;
            return;
        }

        WriteJSONObject();
        FinishRow();
    }


    void
    WriteRow()
    {
        recordBuffer.clear();
        if(IsJSONOutput())
        {
            WriteJSONRow();
            return;
        }

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

        if(outputFormat == OutputFormat::jsonArray &&
           reportFailure && file)
        {
            if(jsonRecordCount == 0)
                file << "]\n";
            else
                file << "\n]\n";
        }
        file.flush();
        bool succeeded = static_cast<bool>(file);
        file.close();
        succeeded = succeeded && !file.fail();
        file.clear();
        rowsSinceFlush = 0;
        jsonRecordCount = 0;
        recordBuffer.clear();

        if(!succeeded && reportFailure)
            Warning("Could not flush and close OutputFile \"" +
                    resolvedFilename.string() + "\"", path_);
        return succeeded;
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
        {
            outputFormat = OutputFormat::delimited;
            columnDelimiter = ',';
        }
        else if(selectedFormat == "tsv")
        {
            outputFormat = OutputFormat::delimited;
            columnDelimiter = '\t';
        }
        else if(selectedFormat == "json")
            outputFormat = OutputFormat::jsonArray;
        else if(selectedFormat == "jsonl")
            outputFormat = OutputFormat::jsonLines;
        else
            throw std::invalid_argument(
                "OutputFile format must be \"csv\", \"tsv\", \"json\", or "
                "\"jsonl\"");

        const std::string selectedDelimiter = delimiter.as_string();
        if(!selectedDelimiter.empty())
        {
            if(IsJSONOutput())
                throw std::invalid_argument(
                    "OutputFile delimiter is not used with JSON formats");
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
        includeHeader = header.as_bool();
        const int reservedValues = std::min(input.size(), 65536);
        recordBuffer.reserve(
            128 + 16 * static_cast<std::size_t>(reservedValues));
    }


    void
    ValidateJSONFields()
    {
        if(!IsJSONOutput())
            return;
        if(!jsonSchemaError.empty())
            throw std::invalid_argument(jsonSchemaError);

        const std::string formatName =
            outputFormat == OutputFormat::jsonArray ? "JSON" : "JSONL";
        static const std::unordered_set<std::string> reservedKeys{
            "line", "tick", "time", "real_time",
        };
        std::unordered_set<std::string> labels;
        labels.reserve(jsonFields.size());

        for(JSONField & field : jsonFields)
        {
            if(field.label.empty())
                throw std::invalid_argument(
                    "OutputFile " + formatName +
                    " connections require explicit labels");
            if(reservedKeys.contains(field.label))
                throw std::invalid_argument(
                    "OutputFile " + formatName + " connection label \"" +
                    field.label +
                    "\" is reserved");
            if(!labels.insert(field.label).second)
                throw std::invalid_argument(
                    "OutputFile " + formatName + " connection label \"" +
                    field.label +
                    "\" is duplicated");
            field.escapedLabel = escape_json_string(field.label);
        }
    }


    int
    SetSizes(input_map ingoingConnections) override
    {
        const int result = Module::SetSizes(ingoingConnections);
        const std::string inputPath = path_ + ".INPUT";
        const auto connectionIterator =
            ingoingConnections.find(inputPath);
        if(connectionIterator == ingoingConnections.end() ||
           GetBuffer("INPUT").is_uninitialized())
            return result;

        std::vector<JSONField> fields;
        fields.reserve(connectionIterator->second.size());
        const int inputSize = GetBuffer("INPUT").size();
        std::string schemaError;
        for(const Connection * connection : connectionIterator->second)
        {
            if(connection->TargetRange().rank() != 1 ||
               connection->TargetRange().step(0) != 1)
            {
                schemaError =
                    "OutputFile flattened input connection has an invalid "
                    "target range";
                break;
            }

            JSONField field;
            field.label = connection->Label();
            field.offset = connection->TargetRange().start(0);
            field.valueCount = connection->TargetRange().size();
            if(field.offset < 0 || field.valueCount < 0 ||
               field.offset > inputSize - field.valueCount)
            {
                schemaError =
                    "OutputFile flattened input connection is outside its "
                    "input buffer";
                break;
            }
            if(connection->DelayCount() > 1)
                field.shape.push_back(connection->DelayCount());
            for(int dimension = 0;
                dimension < connection->SourceRange().rank(); ++dimension)
                field.shape.push_back(
                    connection->SourceRange().size(dimension));

            long long shapeSize = 1;
            for(int dimensionSize : field.shape)
                shapeSize *= dimensionSize;
            if(shapeSize != field.valueCount)
            {
                schemaError =
                    "OutputFile could not preserve the shape of connection \"" +
                    connection->Info() + "\"";
                break;
            }

            fields.push_back(std::move(field));
        }
        jsonFields = std::move(fields);
        jsonSchemaError = std::move(schemaError);
        return result;
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
        Bind(header, "header");
        Bind(singleTrigger, "single_trigger");

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
        ValidateJSONFields();
        if(IsJSONOutput() && !input.is_contiguous())
            throw std::invalid_argument(
                "OutputFile JSON input must have contiguous storage");
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
