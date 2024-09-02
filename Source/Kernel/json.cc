// Temporary JSON library - not used - only here for reference

#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>
#include <cctype>
#include <stdexcept>
#include <variant>
#include <iterator>

using JSONValue = std::variant<std::nullptr_t, bool, double, std::string, std::vector<class JSON>, std::unordered_map<std::string, class JSON>>;

class JSON {
public:
    JSON() = default;
    JSON(JSONValue value) : value_(std::move(value)) {}

    // Getters for different JSON types
    bool isNull() const { return std::holds_alternative<std::nullptr_t>(value_); }
    bool isBool() const { return std::holds_alternative<bool>(value_); }
    bool isNumber() const { return std::holds_alternative<double>(value_); }
    bool isString() const { return std::holds_alternative<std::string>(value_); }
    bool isArray() const { return std::holds_alternative<std::vector<JSON>>(value_); }
    bool isObject() const { return std::holds_alternative<std::unordered_map<std::string, JSON>>(value_); }

    bool asBool() const { return std::get<bool>(value_); }
    double asNumber() const { return std::get<double>(value_); }
    const std::string& asString() const { return std::get<std::string>(value_); }
    const std::vector<JSON>& asArray() const { return std::get<std::vector<JSON>>(value_); }
    const std::unordered_map<std::string, JSON>& asObject() const { return std::get<std::unordered_map<std::string, JSON>>(value_); }

    static JSON parse(const std::string& input) {
        size_t pos = 0;
        return parseValue(input, pos);
    }

    // Overload operator[] for arrays and objects
    JSON& operator[](size_t index) {
        if (!isArray()) {
            throw std::runtime_error("JSON value is not an array");
        }
        auto& array = std::get<std::vector<JSON>>(value_);
        if (index >= array.size()) {
            throw std::out_of_range("Index out of bounds");
        }
        return array[index];
    }

    const JSON& operator[](size_t index) const {
        if (!isArray()) {
            throw std::runtime_error("JSON value is not an array");
        }
        const auto& array = std::get<std::vector<JSON>>(value_);
        if (index >= array.size()) {
            throw std::out_of_range("Index out of bounds");
        }
        return array[index];
    }

    JSON& operator[](const std::string& key) {
        if (!isObject()) {
            throw std::runtime_error("JSON value is not an object");
        }
        auto& object = std::get<std::unordered_map<std::string, JSON>>(value_);
        return object[key];
    }

    const JSON& operator[](const std::string& key) const {
        if (!isObject()) {
            throw std::runtime_error("JSON value is not an object");
        }
        const auto& object = std::get<std::unordered_map<std::string, JSON>>(value_);
        return object.at(key);
    }

    // Iterator support for arrays
    class array_iterator {
    public:
        using value_type = JSON;
        using difference_type = std::ptrdiff_t;
        using pointer = JSON*;
        using reference = JSON&;
        using iterator_category = std::forward_iterator_tag;

        array_iterator(pointer ptr) : ptr_(ptr) {}

        reference operator*() const { return *ptr_; }
        pointer operator->() { return ptr_; }

        array_iterator& operator++() {
            ++ptr_;
            return *this;
        }

        array_iterator operator++(int) {
            array_iterator tmp = *this;
            ++(*this);
            return tmp;
        }

        friend bool operator==(const array_iterator& a, const array_iterator& b) { return a.ptr_ == b.ptr_; }
        friend bool operator!=(const array_iterator& a, const array_iterator& b) { return a.ptr_ != b.ptr_; }

    private:
        pointer ptr_;
    };

    array_iterator begin() {
        if (!isArray()) {
            throw std::runtime_error("JSON value is not an array");
        }
        auto& array = std::get<std::vector<JSON>>(value_);
        return array_iterator(array.data());
    }

    array_iterator end() {
        if (!isArray()) {
            throw std::runtime_error("JSON value is not an array");
        }
        auto& array = std::get<std::vector<JSON>>(value_);
        return array_iterator(array.data() + array.size());
    }

    // Iterator support for objects
    class object_iterator {
    public:
        using map_iterator = std::unordered_map<std::string, JSON>::iterator;
        using value_type = std::pair<const std::string, JSON>;
        using difference_type = std::ptrdiff_t;
        using pointer = value_type*;
        using reference = value_type&;
        using iterator_category = std::forward_iterator_tag;

        object_iterator(map_iterator it) : it_(it) {}

        reference operator*() const { return *it_; }
        pointer operator->() { return &(*it_); }

        object_iterator& operator++() {
            ++it_;
            return *this;
        }

        object_iterator operator++(int) {
            object_iterator tmp = *this;
            ++(*this);
            return tmp;
        }

        friend bool operator==(const object_iterator& a, const object_iterator& b) { return a.it_ == b.it_; }
        friend bool operator!=(const object_iterator& a, const object_iterator& b) { return a.it_ != b.it_; }

    private:
        map_iterator it_;
    };

    object_iterator beginObject() {
        if (!isObject()) {
            throw std::runtime_error("JSON value is not an object");
        }
        auto& object = std::get<std::unordered_map<std::string, JSON>>(value_);
        return object_iterator(object.begin());
    }

    object_iterator endObject() {
        if (!isObject()) {
            throw std::runtime_error("JSON value is not an object");
        }
        auto& object = std::get<std::unordered_map<std::string, JSON>>(value_);
        return object_iterator(object.end());
    }

private:
    JSONValue value_;

    static void skipWhitespace(const std::string& input, size_t& pos) {
        while (pos < input.length() && std::isspace(input[pos])) {
            ++pos;
        }
    }

    static JSON parseValue(const std::string& input, size_t& pos) {
        skipWhitespace(input, pos);

        if (pos >= input.length()) {
            throw std::runtime_error("Unexpected end of input");
        }

        switch (input[pos]) {
            case 'n': return parseNull(input, pos);
            case 't': case 'f': return parseBool(input, pos);
            case '"': return parseString(input, pos);
            case '[': return parseArray(input, pos);
            case '{': return parseObject(input, pos);
            default: return parseNumber(input, pos);
        }
    }

    static JSON parseNull(const std::string& input, size_t& pos) {
        if (input.substr(pos, 4) == "null") {
            pos += 4;
            return JSON(nullptr);
        }
        throw std::runtime_error("Invalid null value");
    }

    static JSON parseBool(const std::string& input, size_t& pos) {
        if (input.substr(pos, 4) == "true") {
            pos += 4;
            return JSON(true);
        } else if (input.substr(pos, 5) == "false") {
            pos += 5;
            return JSON(false);
        }
        throw std::runtime_error("Invalid boolean value");
    }

    static JSON parseNumber(const std::string& input, size_t& pos) {
        size_t start = pos;
        if (input[pos] == '-') {
            ++pos;
        }
        while (pos < input.length() && std::isdigit(input[pos])) {
            ++pos;
        }
        if (pos < input.length() && input[pos] == '.') {
            ++pos;
            while (pos < input.length() && std::isdigit(input[pos])) {
                ++pos;
            }
        }
        if (pos < input.length() && (input[pos] == 'e' || input[pos] == 'E')) {
            ++pos;
            if (pos < input.length() && (input[pos] == '+' || input[pos] == '-')) {
                ++pos;
            }
            while (pos < input.length() && std::isdigit(input[pos])) {
                ++pos;
            }
        }
        double value = std::stod(input.substr(start, pos - start));
        return JSON(value);
    }

    static JSON parseString(const std::string& input, size_t& pos) {
        ++pos;  // Skip opening quote
        std::string value;
        while (pos < input.length()) {
            char c = input[pos++];
            if (c == '"') {
                return JSON(value);
            }
            if (c == '\\') {
                if (pos >= input.length()) {
                    throw std::runtime_error("Invalid string escape sequence");
                }
                c = input[pos++];
                switch (c) {
                    case '"': case '\\': case '/': value += c; break;
                    case 'b': value += '\b'; break;
                    case 'f': value += '\f'; break;
                    case 'n': value += '\n'; break;
                    case 'r': value += '\r'; break;
                    case 't': value += '\t'; break;
                    case 'u': {
                        if (pos + 4 > input.length()) {
                            throw std::runtime_error("Invalid unicode escape sequence");
                        }
                        std::string hex = input.substr(pos, 4);
                        pos += 4;
                        value += static_cast<char>(std::stoi(hex, nullptr, 16));
                        break;
                    }
                    default: throw std::runtime_error("Invalid string escape character");
                }
            } else {
                value += c;
            }
        }
        throw std::runtime_error("Unterminated string");
    }

    static JSON parseArray(const std::string& input, size_t& pos) {
        ++pos;  // Skip opening bracket
        std::vector<JSON> array;
        skipWhitespace(input, pos);
        if (pos < input.length() && input[pos] == ']') {
            ++pos;
            return JSON(array);
        }
        while (pos < input.length()) {
            array.push_back(parseValue(input, pos));
            skipWhitespace(input, pos);
            if (pos < input.length() && input[pos] == ']') {
                ++pos;
                return JSON(array);
            }
            if (pos >= input.length() || input[pos] != ',') {
                throw std::runtime_error("Expected ',' or ']' in array");
            }
            ++pos;
            skipWhitespace(input, pos);
        }
        throw std::runtime_error("Unterminated array");
    }

    static JSON parseObject(const std::string& input, size_t& pos) {
        ++pos;  // Skip opening brace
        std::unordered_map<std::string, JSON> object;
        skipWhitespace(input, pos);
        if (pos < input.length() && input[pos] == '}') {
            ++pos;
            return JSON(object);
        }
        while (pos < input.length()) {
            skipWhitespace(input, pos);
            if (pos >= input.length() || input[pos] != '"') {
                throw std::runtime_error("Expected string key in object");
            }
            std::string key = parseString(input, pos).asString();
            skipWhitespace(input, pos);
            if (pos >= input.length() || input[pos] != ':') {
                throw std::runtime_error("Expected ':' in object");
            }
            ++pos;
            skipWhitespace(input, pos);
            object[key] = parseValue(input, pos);
            skipWhitespace(input, pos);
            if (pos < input.length() && input[pos] == '}') {
                ++pos;
                return JSON(object);
            }
            if (pos >= input.length() || input[pos] != ',') {
                throw std::runtime_error("Expected ',' or '}' in object");
            }
            ++pos;
            skipWhitespace(input, pos);
        }
        throw std::runtime_error("Unterminated object");
    }
};
