#include <algorithm>
#include <charconv>
#include <cctype>
#include <cmath>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>

#include "ikaros.h"


using namespace ikaros;


class InputFile : public Module
{
    enum class InputFormat
    {
        whitespace,
        delimited,
        jsonArray,
        jsonLines,
    };

    struct Field
    {
        std::string sourceName;
        std::string outputName;
        std::vector<int> shape;
        std::size_t offset = 0;
        int valueCount = 0;
    };

    parameter filename;
    parameter format;
    parameter delimiter;
    parameter header;
    parameter sendEndOfFile;

    std::filesystem::path resolvedFilename;
    InputFormat inputFormat = InputFormat::whitespace;
    char columnDelimiter = '\0';
    bool hasHeader = true;
    std::vector<Field> fields;
    std::vector<matrix> outputs;
    std::vector<float> records;
    std::size_t valuesPerRecord = 0;
    std::size_t recordCount = 0;
    std::size_t currentRecord = 0;
    bool endOfFileHandled = false;

    std::string
    Setting(const std::string & name,
            const std::string & defaultValue = "") const
    {
        if(!info_.contains_non_null(name))
            return defaultValue;
        return info_.at(name).as_string();
    }


    static std::string
    Lowercase(std::string value)
    {
        std::transform(
            value.begin(), value.end(), value.begin(),
            [](unsigned char character)
            {
                return ascii_to_lower(character);
            });
        return value;
    }


    void
    ParseOptions()
    {
        const std::string selectedFormat =
            Lowercase(Setting("format", "auto"));
        const std::string selectedDelimiter = Setting("delimiter");
        const std::string extension =
            Lowercase(resolvedFilename.extension().string());

        if(selectedFormat == "json")
            inputFormat = InputFormat::jsonArray;
        else if(selectedFormat == "jsonl")
            inputFormat = InputFormat::jsonLines;
        else if(selectedFormat == "csv")
        {
            inputFormat = InputFormat::delimited;
            columnDelimiter = ',';
        }
        else if(selectedFormat == "tsv")
        {
            inputFormat = InputFormat::delimited;
            columnDelimiter = '\t';
        }
        else if(selectedFormat == "text")
            inputFormat = InputFormat::whitespace;
        else if(selectedFormat == "auto")
        {
            if(extension == ".json")
                inputFormat = InputFormat::jsonArray;
            else if(extension == ".jsonl")
                inputFormat = InputFormat::jsonLines;
            else if(extension == ".csv")
            {
                inputFormat = InputFormat::delimited;
                columnDelimiter = ',';
            }
            else if(extension == ".tsv")
            {
                inputFormat = InputFormat::delimited;
                columnDelimiter = '\t';
            }
            else
                inputFormat = InputFormat::whitespace;
        }
        else
            throw std::invalid_argument(
                "InputFile format must be \"auto\", \"text\", \"csv\", "
                "\"tsv\", \"json\", or \"jsonl\"");

        if(!selectedDelimiter.empty())
        {
            if(inputFormat == InputFormat::jsonArray ||
               inputFormat == InputFormat::jsonLines)
                throw std::invalid_argument(
                    "InputFile delimiter is not used with JSON formats");
            if(selectedDelimiter.size() != 1)
                throw std::invalid_argument(
                    "InputFile delimiter must be exactly one character");

            const char requestedDelimiter = selectedDelimiter.front();
            if(requestedDelimiter == '\0' || requestedDelimiter == '\r' ||
               requestedDelimiter == '\n' || requestedDelimiter == '"')
                throw std::invalid_argument(
                    "InputFile delimiter cannot be a line break, NUL, or "
                    "quotation mark");
            inputFormat = InputFormat::delimited;
            columnDelimiter = requestedDelimiter;
        }

        bool parsedHeader = true;
        if(!parse_bool(Setting("header", "true"), parsedHeader))
            throw std::invalid_argument(
                "InputFile header must be true or false");
        hasHeader = parsedHeader;
    }


    std::string
    ReadContents() const
    {
        std::ifstream input(resolvedFilename, std::ios::binary);
        if(!input)
            throw std::runtime_error(
                "Could not open InputFile \"" +
                resolvedFilename.string() + "\"");

        const std::string contents{
            std::istreambuf_iterator<char>(input),
            std::istreambuf_iterator<char>()
        };
        if(input.bad())
            throw std::runtime_error(
                "Could not read InputFile \"" +
                resolvedFilename.string() + "\"");
        return contents;
    }


    static bool
    IsBlankRecord(const std::vector<std::string> & record)
    {
        return record.size() == 1 && trim(record.front()).empty();
    }


    static std::vector<std::vector<std::string>>
    ParseDelimitedRecords(const std::string & contents, char separator)
    {
        std::vector<std::vector<std::string>> records;
        std::vector<std::string> record;
        std::string field;
        bool inQuotes = false;
        bool quoteClosed = false;
        bool fieldStarted = false;

        auto finishField = [&]()
        {
            record.push_back(std::move(field));
            field.clear();
            quoteClosed = false;
            fieldStarted = false;
        };
        auto finishRecord = [&]()
        {
            finishField();
            if(!IsBlankRecord(record))
                records.push_back(std::move(record));
            record.clear();
        };

        for(std::size_t index = 0; index < contents.size(); ++index)
        {
            char character = contents[index];
            if(inQuotes)
            {
                if(character == '"')
                {
                    if(index + 1 < contents.size() &&
                       contents[index + 1] == '"')
                    {
                        field += '"';
                        ++index;
                    }
                    else
                    {
                        inQuotes = false;
                        quoteClosed = true;
                    }
                }
                else if(character == '\r')
                {
                    if(index + 1 < contents.size() &&
                       contents[index + 1] == '\n')
                        ++index;
                    field += '\n';
                }
                else
                    field += character;
                continue;
            }

            if(quoteClosed)
            {
                if(character == separator)
                    finishField();
                else if(character == '\r' || character == '\n')
                {
                    if(character == '\r' &&
                       index + 1 < contents.size() &&
                       contents[index + 1] == '\n')
                        ++index;
                    finishRecord();
                }
                else
                    throw std::invalid_argument(
                        "Unexpected character after a quoted field");
                continue;
            }

            if(character == separator)
            {
                finishField();
                continue;
            }
            if(character == '\r' || character == '\n')
            {
                if(character == '\r' &&
                   index + 1 < contents.size() &&
                   contents[index + 1] == '\n')
                    ++index;
                finishRecord();
                continue;
            }
            if(character == '"')
            {
                if(fieldStarted)
                    throw std::invalid_argument(
                        "Quotation mark inside an unquoted field");
                inQuotes = true;
                fieldStarted = true;
                continue;
            }

            field += character;
            fieldStarted = true;
        }

        if(inQuotes)
            throw std::invalid_argument(
                "Unterminated quoted field");
        if(quoteClosed || fieldStarted || !record.empty())
            finishRecord();
        return records;
    }


    static std::vector<std::vector<std::string>>
    ParseWhitespaceRecords(const std::string & contents)
    {
        std::vector<std::vector<std::string>> records;
        std::istringstream input(contents);
        std::string line;
        while(std::getline(input, line))
        {
            line = replace_characters(remove_comment(line));
            std::vector<std::string> record = split(line, "");
            if(!record.empty())
                records.push_back(std::move(record));
        }
        return records;
    }


    static bool
    ParseNonNegativeInt(std::string_view text, int & result)
    {
        if(text.empty())
            return false;

        int parsed = 0;
        const char * begin = text.data();
        const char * end = begin + text.size();
        const auto conversion =
            std::from_chars(begin, end, parsed);
        if(conversion.ec != std::errc() ||
           conversion.ptr != end || parsed < 0)
            return false;
        result = parsed;
        return true;
    }


    static bool
    ParseIndexedLabel(const std::string & label,
                      std::string & base, int & index)
    {
        const std::size_t separator = label.rfind(':');
        if(separator == std::string::npos || separator == 0)
            return false;
        if(!ParseNonNegativeInt(
               std::string_view(label).substr(separator + 1), index))
            return false;
        base = label.substr(0, separator);
        return true;
    }


    static bool
    ParseSizedLabel(const std::string & label,
                    std::string & base, int & size)
    {
        const std::size_t separator = label.rfind('/');
        if(separator == std::string::npos || separator == 0)
            return false;
        if(!ParseNonNegativeInt(
               std::string_view(label).substr(separator + 1), size))
            return false;
        base = label.substr(0, separator);
        return true;
    }


    void
    AddField(std::string sourceName, std::vector<int> shape)
    {
        std::size_t valueCount = 1;
        for(int dimension : shape)
        {
            if(dimension <= 0)
                throw std::invalid_argument(
                    "InputFile fields must have positive dimensions");
            if(valueCount >
               static_cast<std::size_t>(
                   std::numeric_limits<int>::max()) /
                   static_cast<std::size_t>(dimension))
                throw std::overflow_error(
                    "InputFile field is too large");
            valueCount *= static_cast<std::size_t>(dimension);
        }

        if(valuesPerRecord >
           std::numeric_limits<std::size_t>::max() - valueCount)
            throw std::overflow_error(
                "InputFile record is too large");

        Field field;
        field.sourceName = std::move(sourceName);
        field.shape = std::move(shape);
        field.offset = valuesPerRecord;
        field.valueCount = static_cast<int>(valueCount);
        fields.push_back(std::move(field));
        valuesPerRecord += valueCount;
    }


    void
    ConfigureDelimitedFields(
        const std::vector<std::string> & columnNames)
    {
        if(!hasHeader)
        {
            if(columnNames.size() >
               static_cast<std::size_t>(
                   std::numeric_limits<int>::max()))
                throw std::overflow_error(
                    "InputFile has too many columns");
            AddField("OUTPUT",
                     {static_cast<int>(columnNames.size())});
            return;
        }

        for(std::size_t column = 0; column < columnNames.size();)
        {
            std::string indexedBase;
            int indexedPosition = -1;
            if(ParseIndexedLabel(
                   columnNames[column], indexedBase, indexedPosition) &&
               indexedPosition == 0)
            {
                std::size_t width = 1;
                while(column + width <
                      columnNames.size())
                {
                    std::string nextBase;
                    int nextPosition = -1;
                    if(!ParseIndexedLabel(
                           columnNames[column + width],
                           nextBase, nextPosition) ||
                       nextBase != indexedBase ||
                       nextPosition != static_cast<int>(width))
                        break;
                    if(width ==
                       static_cast<std::size_t>(
                           std::numeric_limits<int>::max()))
                        throw std::overflow_error(
                            "InputFile field has too many columns");
                    ++width;
                }
                AddField(
                    indexedBase, {static_cast<int>(width)});
                column += width;
                continue;
            }

            std::string sizedBase;
            int width = 1;
            if(ParseSizedLabel(
                   columnNames[column], sizedBase, width))
            {
                if(width <= 0)
                    throw std::invalid_argument(
                        "InputFile column size must be positive");
                AddField(sizedBase, {width});
            }
            else
            {
                std::string name = columnNames[column];
                if(name.empty())
                    name = "column_" + std::to_string(column);
                AddField(std::move(name), {1});
            }
            ++column;
        }
    }


    void
    AppendRecord(const std::vector<float> & values)
    {
        if(values.size() != valuesPerRecord)
            throw std::logic_error(
                "InputFile record width does not match its schema");
        if(recordCount == std::numeric_limits<std::size_t>::max())
            throw std::overflow_error(
                "InputFile has too many records");
        if(records.size() >
           std::numeric_limits<std::size_t>::max() - values.size())
            throw std::overflow_error(
                "InputFile data is too large");

        records.insert(records.end(), values.begin(), values.end());
        ++recordCount;
    }


    void
    LoadDelimited()
    {
        const std::string contents = ReadContents();
        std::vector<std::vector<std::string>> table =
            inputFormat == InputFormat::whitespace ?
                ParseWhitespaceRecords(contents) :
                ParseDelimitedRecords(contents, columnDelimiter);
        if(table.empty())
            return;

        std::size_t firstDataRecord = 0;
        if(hasHeader)
        {
            ConfigureDelimitedFields(table.front());
            firstDataRecord = 1;
        }
        else
            ConfigureDelimitedFields(table.front());

        for(std::size_t recordIndex = firstDataRecord;
            recordIndex < table.size(); ++recordIndex)
        {
            const std::vector<std::string> & source = table[recordIndex];
            if(source.size() != valuesPerRecord)
                throw std::invalid_argument(
                    "InputFile record " +
                    std::to_string(recordIndex + 1) + " has " +
                    std::to_string(source.size()) +
                    " columns; expected " +
                    std::to_string(valuesPerRecord));

            std::vector<float> values;
            values.reserve(valuesPerRecord);
            for(std::size_t column = 0;
                column < source.size(); ++column)
            {
                float parsed = 0.0f;
                if(!parse_float(source[column], parsed))
                    throw std::invalid_argument(
                        "InputFile record " +
                        std::to_string(recordIndex + 1) +
                        ", column " + std::to_string(column + 1) +
                        " is not numeric: \"" + source[column] + "\"");
                values.push_back(parsed);
            }
            AppendRecord(values);
        }
    }


    static std::vector<int>
    InferJSONShape(const value & candidate,
                   const std::string & fieldName)
    {
        if(candidate.is_number() || candidate.is_null())
            return {};
        if(!candidate.is_list())
            throw std::invalid_argument(
                "InputFile JSON field \"" + fieldName +
                "\" must contain numbers, null, or nested arrays");

        const list & values = candidate.as_list();
        if(values.empty())
            throw std::invalid_argument(
                "InputFile JSON field \"" + fieldName +
                "\" cannot infer an output shape from an empty array");
        if(values.size() >
           static_cast<std::size_t>(
               std::numeric_limits<int>::max()))
            throw std::overflow_error(
                "InputFile JSON field \"" + fieldName +
                "\" is too large");

        const std::vector<int> elementShape =
            InferJSONShape(values[0], fieldName);
        for(std::size_t index = 1; index < values.size(); ++index)
            if(InferJSONShape(values[index], fieldName) != elementShape)
                throw std::invalid_argument(
                    "InputFile JSON field \"" + fieldName +
                    "\" contains a ragged array");

        std::vector<int> result;
        result.reserve(elementShape.size() + 1);
        result.push_back(static_cast<int>(values.size()));
        result.insert(
            result.end(), elementShape.begin(), elementShape.end());
        return result;
    }


    static int
    JSONKeyPriority(const std::string & key)
    {
        return key == "line" || key == "tick" ||
               key == "time" || key == "real_time" ? 0 : 1;
    }


    void
    ConfigureJSONFields(const dictionary & object)
    {
        std::vector<std::string> keys;
        keys.reserve(static_cast<std::size_t>(
            std::distance(object.begin(), object.end())));
        for(const auto & [key, ignored] : object)
        {
            static_cast<void>(ignored);
            keys.push_back(key);
        }
        std::sort(
            keys.begin(), keys.end(),
            [](const std::string & first,
               const std::string & second)
            {
                const int firstPriority = JSONKeyPriority(first);
                const int secondPriority = JSONKeyPriority(second);
                if(firstPriority != secondPriority)
                    return firstPriority < secondPriority;
                return first < second;
            });

        for(const std::string & key : keys)
            AddField(key, InferJSONShape(object.at(key), key));
    }


    static float
    JSONNumber(const value & candidate,
               const std::string & fieldName)
    {
        if(candidate.is_null())
            return std::numeric_limits<float>::quiet_NaN();
        if(!candidate.is_number())
            throw std::invalid_argument(
                "InputFile JSON field \"" + fieldName +
                "\" contains a non-numeric value");

        const double number = candidate.as_double();
        const float converted = static_cast<float>(number);
        if(std::isfinite(number) && !std::isfinite(converted))
            throw std::out_of_range(
                "InputFile JSON field \"" + fieldName +
                "\" contains a number outside the float range");
        return converted;
    }


    static void
    FlattenJSONValue(const value & candidate,
                     const std::vector<int> & shape,
                     std::size_t dimension,
                     const std::string & fieldName,
                     std::vector<float> & result)
    {
        if(dimension == shape.size())
        {
            result.push_back(JSONNumber(candidate, fieldName));
            return;
        }
        if(!candidate.is_list())
            throw std::invalid_argument(
                "InputFile JSON field \"" + fieldName +
                "\" does not match its initial shape");

        const list & values = candidate.as_list();
        if(values.size() !=
           static_cast<std::size_t>(shape[dimension]))
            throw std::invalid_argument(
                "InputFile JSON field \"" + fieldName +
                "\" does not match its initial shape");
        for(const value & element : values)
            FlattenJSONValue(
                element, shape, dimension + 1,
                fieldName, result);
    }


    void
    AppendJSONRecord(const value & candidate,
                     std::size_t sourceRecord)
    {
        if(!candidate.is_dictionary())
            throw std::invalid_argument(
                "InputFile JSON record " +
                std::to_string(sourceRecord) +
                " is not an object");

        const dictionary & object = candidate.as_dictionary();
        if(recordCount == 0)
            ConfigureJSONFields(object);
        else
        {
            const std::size_t keyCount =
                static_cast<std::size_t>(
                    std::distance(object.begin(), object.end()));
            if(keyCount != fields.size())
                throw std::invalid_argument(
                    "InputFile JSON record " +
                    std::to_string(sourceRecord) +
                    " has different fields from the first record");
            for(const Field & field : fields)
                if(!object.contains(field.sourceName))
                    throw std::invalid_argument(
                        "InputFile JSON record " +
                        std::to_string(sourceRecord) +
                        " is missing field \"" +
                        field.sourceName + "\"");
        }

        std::vector<float> values;
        values.reserve(valuesPerRecord);
        for(const Field & field : fields)
            FlattenJSONValue(
                object.at(field.sourceName), field.shape, 0,
                field.sourceName, values);
        AppendRecord(values);
    }


    void
    LoadJSONArray()
    {
        const std::string contents = ReadContents();
        if(trim(contents).empty())
            throw std::invalid_argument(
                "InputFile JSON array file is empty");

        value root;
        try
        {
            root = parse_json(contents);
        }
        catch(const std::exception & error)
        {
            throw std::invalid_argument(
                "Could not parse InputFile JSON array: " +
                std::string(error.what()));
        }
        if(!root.is_list())
            throw std::invalid_argument(
                "InputFile JSON must contain a top-level array");

        const list & sourceRecords = root.as_list();
        for(std::size_t index = 0;
            index < sourceRecords.size(); ++index)
            AppendJSONRecord(sourceRecords[index], index + 1);
    }


    void
    LoadJSONLines()
    {
        std::ifstream input(resolvedFilename);
        if(!input)
            throw std::runtime_error(
                "Could not open InputFile \"" +
                resolvedFilename.string() + "\"");

        std::string line;
        std::size_t lineNumber = 0;
        while(std::getline(input, line))
        {
            ++lineNumber;
            if(!line.empty() && line.back() == '\r')
                line.pop_back();
            if(trim(line).empty())
                throw std::invalid_argument(
                    "InputFile JSONL contains an empty record at line " +
                    std::to_string(lineNumber));

            value parsed;
            try
            {
                parsed = parse_json(line);
            }
            catch(const std::exception & error)
            {
                throw std::invalid_argument(
                    "Could not parse InputFile JSONL line " +
                    std::to_string(lineNumber) + ": " +
                    error.what());
            }
            AppendJSONRecord(parsed, lineNumber);
        }
        if(input.bad())
            throw std::runtime_error(
                "Could not read InputFile \"" +
                resolvedFilename.string() + "\"");
    }


    static bool
    IsIdentifierCharacter(char character)
    {
        return character == '_' ||
               (character >= '0' && character <= '9') ||
               (character >= 'A' && character <= 'Z') ||
               (character >= 'a' && character <= 'z');
    }


    static std::string
    IdentifierBase(const std::string & sourceName,
                   std::size_t fieldIndex)
    {
        std::string result = sourceName.empty() ?
            "column_" + std::to_string(fieldIndex) : sourceName;
        for(char & character : result)
            if(!IsIdentifierCharacter(character))
                character = '_';
        if(result.empty())
            result = "column_" + std::to_string(fieldIndex);
        if(result.front() >= '0' && result.front() <= '9')
            result.insert(result.begin(), '_');
        return result;
    }


    static std::string
    ShapeExpression(const std::vector<int> & shape)
    {
        std::string result;
        for(std::size_t dimension = 0;
            dimension < shape.size(); ++dimension)
        {
            if(dimension > 0)
                result += ",";
            result += std::to_string(shape[dimension]);
        }
        return result;
    }


    void
    RegisterOutputs()
    {
        std::unordered_set<std::string> usedNames;
        usedNames.reserve(fields.size());
        outputs.reserve(fields.size());

        for(std::size_t index = 0; index < fields.size(); ++index)
        {
            Field & field = fields[index];
            const std::string base =
                IdentifierBase(field.sourceName, index);
            field.outputName = base;
            for(std::size_t suffix = 2;
                usedNames.contains(field.outputName); ++suffix)
                field.outputName =
                    base + "_" + std::to_string(suffix);
            usedNames.insert(field.outputName);

            const std::vector<int> outputShape =
                field.shape.empty() ?
                    std::vector<int>{1} : field.shape;
            dictionary metadata{
                {"name", field.outputName},
                {"description",
                 "Values read from field \"" +
                 field.sourceName + "\""},
                {"_tag", "output"}
            };
            if(outputShape.size() == 1)
                metadata["size"] =
                    std::to_string(outputShape.front());
            else
                metadata["shape"] =
                    ShapeExpression(outputShape);
            list(info_["outputs"]).push_back(metadata);
            AddOutput(std::move(metadata));
            outputs.emplace_back();
        }
    }


    void
    CopyRecordToOutputs(std::size_t recordIndex)
    {
        const std::size_t recordOffset =
            recordIndex * valuesPerRecord;
        for(std::size_t index = 0; index < fields.size(); ++index)
        {
            const Field & field = fields[index];
            float * target = outputs[index].contiguous_data();
            std::copy_n(
                records.data() + recordOffset + field.offset,
                field.valueCount, target);
        }
    }


    void
    ResetOutputs()
    {
        for(matrix & output : outputs)
            output.reset();
    }

public:
    InputFile()
        : Module()
    {
        const std::string configuredFilename = Setting("filename");
        if(configuredFilename.empty())
            throw exception(
                "InputFile filename must not be empty", path_);
        if(!kernel().SanitizeReadPath(
               configuredFilename, resolvedFilename))
            throw exception(
                "InputFile can only read files from the project "
                "directory or UserData", path_);

        try
        {
            ParseOptions();
            if(inputFormat == InputFormat::jsonArray)
                LoadJSONArray();
            else if(inputFormat == InputFormat::jsonLines)
                LoadJSONLines();
            else
                LoadDelimited();
            RegisterOutputs();
        }
        catch(const std::exception & error)
        {
            throw exception(
                "Could not load InputFile \"" +
                resolvedFilename.string() + "\": " +
                error.what(), path_);
        }
    }


    void
    Init() override
    {
        Bind(filename, "filename");
        Bind(format, "format");
        Bind(delimiter, "delimiter");
        Bind(header, "header");
        Bind(sendEndOfFile, "send_end_of_file");

        for(std::size_t index = 0; index < fields.size(); ++index)
            Bind(outputs[index], fields[index].outputName);
    }


    void
    Reset() override
    {
        currentRecord = 0;
        endOfFileHandled = false;
        ResetOutputs();
    }


    void
    Tick() override
    {
        if(currentRecord < recordCount)
        {
            CopyRecordToOutputs(currentRecord);
            ++currentRecord;
            return;
        }

        if(endOfFileHandled)
            return;

        ResetOutputs();
        endOfFileHandled = true;
        if(sendEndOfFile.as_bool())
            Notify(msg_end_of_file, resolvedFilename.string());
    }
};


INSTALL_CLASS(InputFile)
