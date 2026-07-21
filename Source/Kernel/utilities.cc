// utilities.cc

#include <charconv>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <system_error>
#include <type_traits>

#include "utilities.h"

namespace ikaros
{
namespace
{
template<typename T>
T
checked_truncating_integral_conversion(double value, const std::string & conversion_name)
{
    static_assert(std::is_integral_v<T> && std::is_signed_v<T>);

    const long double converted_value = value;
    const long double lower_exclusive = static_cast<long double>(std::numeric_limits<T>::lowest()) - 1.0L;
    const long double upper_exclusive = static_cast<long double>(std::numeric_limits<T>::max()) + 1.0L;
    if(!std::isfinite(value) || converted_value <= lower_exclusive || converted_value >= upper_exclusive)
        throw std::out_of_range("Value cannot be represented as " + conversion_name + ".");

    return static_cast<T>(value);
}


void
validate_delimiter(const std::string & delimiter)
{
    if(delimiter.empty())
        throw std::invalid_argument("String delimiter must not be empty.");
}
}

std::string trim(const std::string &s)
{
   auto wsfront=std::find_if_not(s.begin(),s.end(),[](unsigned char c){return std::isspace(c);});
   auto wsback=std::find_if_not(s.rbegin(),s.rend(),[](unsigned char c){return std::isspace(c);}).base();
   return (wsback<=wsfront ? std::string() : std::string(wsfront,wsback));
}

const std::vector<std::string>
split(const std::string & s, const std::string & sep, int maxsplit)
{
    std::vector<std::string> r;
    std::string::size_type i=0, j=0;
    std::string::size_type len = s.size();
    std::string::size_type n = sep.size();

    if (n == 0)
    {
        while(i<len)
        {
            while (i < len && ::isspace(static_cast<unsigned char>(s[i])))
                i++;
            j = i;
            while (i < len && !::isspace(static_cast<unsigned char>(s[i])))
                i++;

            if(j < i)
            {
                if(maxsplit != -1 && maxsplit-- <= 0)
                    break;
                r.push_back(trim(s.substr(j, i-j)));
                while(i < len && ::isspace(static_cast<unsigned char>(s[i])))
                    i++;
                j = i;
            }
        }
        if (j < len)
            r.push_back(trim(s.substr(j, len - j)));

        return r;
    }

    i = j = 0;
    while (i+n <= len)
    {
        if (s[i] == sep[0] && s.substr(i, n) == sep)
        {
            if(maxsplit != -1 && maxsplit-- <= 0)
                break;

            r.push_back(trim(s.substr(j, i - j)));
            i = j = i + n;
        }
        else
            i++;
    }

    r.push_back(trim(s.substr(j, len-j)));
    return r;
}


const std::vector<std::string>
rsplit(const std::string & str, const std::string & sep, int maxsplit)
{
    if (maxsplit < 0)
        return split(str, sep, maxsplit);

    std::vector<std::string> r;
    std::string::size_type i=str.size();
    std::string::size_type j=str.size();
    std::string::size_type n=sep.size();

    if(n == 0)
    {
        while(i > 0)
        {
            while(i > 0 && ::isspace(static_cast<unsigned char>(str[i-1])))
                i--;
            j = i;
            while(i > 0 && !::isspace(static_cast<unsigned char>(str[i-1])))
                i--;

            if (j > i)
            {
                if(maxsplit != -1 && maxsplit-- <= 0)
                    break;
                r.push_back(str.substr(i, j-i));
                while(i > 0 && ::isspace(static_cast<unsigned char>(str[i-1])))
                    i--;
                j = i;
            }
        }
        if (j > 0)
            r.push_back( str.substr(0, j));
    }
    else
    {
        while(i >= n)
        {
            if(str[i-1] == sep[n-1] && str.substr(i-n, n) == sep)
            {
                if(maxsplit != -1 && maxsplit-- <= 0)
                    break;
                r.push_back(str.substr(i, j-i));
                i = j = i-n;
            }
            else
                i--;
        }
        r.push_back(str.substr(0, j));
    }
    
    std::reverse(r.begin(), r.end());
    return r;
}


std::string
join(const std::string & separator, const std::vector<std::string> & v, bool reverse)
{
    std::string s;
    std::string sep;
    if(reverse)
        for (auto & e : v)
        {
            s = e + sep + s;
            sep = separator;
        }    
    else
        for (auto & e : v)
        {
            s += sep + e;
            sep = separator;
        }
    return s;
}


// NEW FUNCTION

// head: return head of string and erase head and delimiter from original string. Returns full string if delimiter not found.

std::string
head(std::string & s, const std::string & delimiter)
{
    validate_delimiter(delimiter);
    const std::string::size_type end = s.find(delimiter);
    if(end == std::string::npos)
    {
        std::string h = s;
        s.clear();
        return h;
    }

    std::string h = s.substr(0, end);
    s.erase(0, end+delimiter.length());
    return h;
}


std::string
cut_head(std::string & s, const std::string & delimiter)
{
    return head(s, delimiter);
}


std::string
rhead(std::string & s, const std::string & delimiter)
{
    validate_delimiter(delimiter);
    const std::string::size_type end = s.rfind(delimiter);
    if(end == std::string::npos)
    {
        std::string h = s;
        s.clear();
        return h;
    }

    std::string h = s.substr(0, end);
    s.erase(0, end+delimiter.length());
    return h;
}



// tail: return tail of string and erase tail and delimiter from original string. Returns empty string if delimiter not found.

std::string
tail(std::string & s, const std::string & delimiter)
{
    validate_delimiter(delimiter);
    const std::string::size_type end = s.find(delimiter);
    if(end == std::string::npos)
        return "";

    std::string t = s.substr(end+delimiter.length());
    s.erase(end);
    return t;
}



std::string
rtail(std::string & s, const std::string & delimiter)
{
    validate_delimiter(delimiter);
    const std::string::size_type end = s.rfind(delimiter);
    if(end == std::string::npos)
        return "";

    std::string t = s.substr(end+delimiter.length());
    s.erase(end);
    return t;
}



// peek

// head: return head of string and erase head and delimiter from original string. Returns full string if delimiter not found.

std::string
peek_head(const std::string & s, const std::string & delimiter)
{
    validate_delimiter(delimiter);
    const std::string::size_type end = s.find(delimiter);
    if(end == std::string::npos)
    {
        std::string h = s;
        return h;
    }

    std::string h = s.substr(0, end);
    return h;
}


std::string
peek_rhead(const std::string & s, const std::string & delimiter)
{
    validate_delimiter(delimiter);
    const std::string::size_type end = s.rfind(delimiter);
    if(end == std::string::npos)
    {
        std::string h = s;
        return h;
    }

    std::string h = s.substr(0, end);
    return h;
}



// tail: return tail of string. Returns empty string if delimiter not found.

std::string
peek_tail(const std::string & s, const std::string & delimiter, bool keep_delimiter)
{
    validate_delimiter(delimiter);
    const std::string::size_type end = s.find(delimiter);
    if(end == std::string::npos)
        return "";

    std::string t;
    if(keep_delimiter)
        t = s.substr(end);
    else
        t = s.substr(end+delimiter.length());
    return t;
}



std::string
peek_rtail(const std::string & s, const std::string & delimiter)
{
    validate_delimiter(delimiter);
    const std::string::size_type end = s.rfind(delimiter);
    if(end == std::string::npos)
        return "";

    std::string t = s.substr(end+delimiter.length());
    return t;
}






bool
starts_with(const std::string & s, const std::string & start) // waiting for C++20
{
    return start.length() <= s.length() && equal(start.begin(), start.end(), s.begin()); // more efficient than find
}

bool ends_with(const std::string & s, const std::string & end) // waiting for C++20
{
    return  s.size() >= end.size() && s.compare(s.size() - end.size(), end.size(), end) == 0;
}


std::string add_extension(const std::string &  filename, const std::string & extension) 
{
    if(ends_with(filename, extension))
        return std::string(filename);
    else
        return std::string(filename) + extension;
}



/*
const std::string head(std::string s, char token) // without token
{
    int p = s.find(token);
    if(p==std::string::npos)
        return s;
    else
        return s.substr(0, p);
}


std::string
cut_head(std::string & s, const std::string & delimiter)
{
    int end = s.find(delimiter);
    if(end == -1)
    {
        std::string h = s;
        s = "";
        return h;
    }
    else
    {
        std::string h = s.substr(0, end);
        s.erase(0, end+delimiter.length());
        return h;
    }
}



const std::string tail(std::string s, char token, bool include_token) // including token
{
    int p = s.find(token);
    if(p==std::string::npos)
        return "";
    else if(include_token)
        return s.substr(p, s.size()-p);
    else
        return s.substr(p+1, s.size()-p-1);
}


const std::string rhead(std::string s, char token) // without token
{
    int p = s.rfind(token);
    if(p==std::string::npos)
        return s;
    else
        return s.substr(0, p);
}

const std::string rtail(std::string s, char token) // without token
{
    int p = s.rfind(token);
    if(p==std::string::npos)
        return s; // was ""
    else
        return s.substr(p+1, s.size()-p);
}
*/


bool contains(const std::string & s, const std::string & n)
{
    return s.find(n) != std::string::npos;
}



bool is_integer(const std::string & s)
    {
        for( char c: s)
            if(c<'0' || c>'9')
            return false;
        return true;
    }


bool is_true(const std::string & s)
{
    static const std::vector<std::string> true_list = {"true", "True", "yes", "YES", "on", "ON", "1"};
    return std::find(true_list.begin(), true_list.end(), s) != true_list.end();
}


bool
parse_bool(const std::string & s, bool & value)
{
    const std::string trimmed = trim(s);
    static const std::vector<std::string> false_list = {"false", "False", "no", "NO", "off", "OFF", "0"};

    if(is_true(trimmed))
    {
        value = true;
        return true;
    }
    if(std::find(false_list.begin(), false_list.end(), trimmed) != false_list.end())
    {
        value = false;
        return true;
    }
    return false;
}


bool
parse_double(const std::string & s, double & value)
{
    std::string trimmed = trim(s);
    if(trimmed.empty())
        return false;

    bool negative = false;
    if(trimmed.front() == '+' || trimmed.front() == '-')
    {
        negative = trimmed.front() == '-';
        trimmed = trimmed.substr(1);
        if(trimmed.empty() || trimmed.front() == '+' || trimmed.front() == '-')
            return false;
    }

    double parsed = 0;
    const char * begin = trimmed.data();
    const char * end = begin + trimmed.size();
    auto result = std::from_chars(begin, end, parsed);
    if(result.ec != std::errc() || result.ptr != end)
        return false;

    value = negative ? -parsed : parsed;
    return true;
}


bool
parse_float(const std::string & s, float & value)
{
    std::string trimmed = trim(s);
    if(trimmed.empty())
        return false;

    bool negative = false;
    if(trimmed.front() == '+' || trimmed.front() == '-')
    {
        negative = trimmed.front() == '-';
        trimmed = trimmed.substr(1);
        if(trimmed.empty() || trimmed.front() == '+' || trimmed.front() == '-')
            return false;
    }

    float parsed = 0;
    const char * begin = trimmed.data();
    const char * end = begin + trimmed.size();
    auto result = std::from_chars(begin, end, parsed);
    if(result.ec != std::errc() || result.ptr != end)
        return false;

    value = negative ? -parsed : parsed;
    return true;
}


double
parse_double(const std::string & s)
{
    double value = 0;
    if(!parse_double(s, value))
        throw std::invalid_argument("Invalid decimal number \"" + s + "\".");
    return value;
}


float
parse_float(const std::string & s)
{
    float value = 0;
    if(!parse_float(s, value))
        throw std::invalid_argument("Invalid decimal number \"" + s + "\".");
    return value;
}


int
checked_truncating_int(double value, const std::string & conversion_name)
{
    return checked_truncating_integral_conversion<int>(value, conversion_name);
}


long
checked_truncating_long(double value, const std::string & conversion_name)
{
    return checked_truncating_integral_conversion<long>(value, conversion_name);
}


bool
is_number(const std::string& s) 
{
    double value = 0;
    return parse_double(s, value);
}





std::ostream& operator<<(std::ostream& os, const std::vector<int> & v)
{
    std::string sep;
    os << "(";
    for(auto i : v)
    {
        os << sep << i;
        sep = ", ";
    }
    os << ")";
    return os;
}



// Utility functions

    // auto tab = [](int d){ return std::string(3*d, ' ');};

    void 
    print_attribute_value(const std::string & name, int value, int indent)
    {
        std::cout << tab(indent) << name << " = " << value << '\n';
    }

    void 
    print_attribute_value(const std::string & name, const std::string & value, int indent)
    {
        std::cout << tab(indent) << name << " = " << value << '\n';
    }

    void 
    print_attribute_value(const std::string & name, const std::vector<int> & values, int indent, int max_items)
    {
        std::cout << tab(indent) << name << " = ";
        const size_t count = max_items > 0 ?
            std::min(values.size(), static_cast<size_t>(max_items)) : values.size();

        for(size_t i = 0; i < count; ++i)
            std::cout << values[i] << " ";
        if(count < values.size())
            std::cout << "...";
        std::cout << '\n';
    }
   
    void 
    print_attribute_value(const std::string & name, const std::vector<float> & values, int indent, int max_items)
    {
        std::cout << tab(indent) << name << " = ";
        const size_t count = max_items > 0 ?
            std::min(values.size(), static_cast<size_t>(max_items)) : values.size();

        for(size_t i = 0; i < count; ++i)
            std::cout << values[i] << " ";
        if(count < values.size())
            std::cout << "...";
        std::cout << '\n';
    }

    void
    print_attribute_value(const std::string & name, const std::vector<std::vector<std::string>> &  values, int indent, int max_items)
    {
        std::cout << tab(indent) << name << " = \n";
        const size_t count = max_items > 0 ?
            std::min(values.size(), static_cast<size_t>(max_items)) : values.size();
        for(size_t i = 0; i < count; ++i)
        {
            const auto & row = values[i];
            std::cout << tab(indent + 1);
            if(row.empty())
                std::cout << "none\n";
            else
            {
                for(const auto & item : row)
                    std::cout << item << " ";
                std::cout << '\n';
            }
        }
        if(count < values.size())
            std::cout << tab(indent + 1) << "...\n";
        std::cout << '\n';
    }


std::string
base64_encode(const unsigned char * data, size_t size)
{
    static constexpr char encoding_table[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    static constexpr size_t padding[] = {0, 2, 1};

    if(size == 0)
        return {};
    if(data == nullptr)
        throw std::invalid_argument("Cannot Base64-encode null data.");
    if(size > (std::numeric_limits<size_t>::max() / 4) * 3)
        throw std::length_error("Base64 input is too large.");

    const size_t encoded_size = ((size + 2) / 3) * 4;
    std::string encoded_data(encoded_size, '\0');
    
    for(size_t i = 0, j = 0; i < size;)
    {
        const unsigned int octet_a = i < size ? data[i++] : 0;
        const unsigned int octet_b = i < size ? data[i++] : 0;
        const unsigned int octet_c = i < size ? data[i++] : 0;
        
        const unsigned int triple = (octet_a << 0x10) + (octet_b << 0x08) + octet_c;
        
        encoded_data[j++] = encoding_table[(triple >> 3 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 2 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 1 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 0 * 6) & 0x3F];
    }
    
    for(size_t i = 0; i < padding[size % 3]; ++i)
        encoded_data[encoded_size - 1 - i] = '=';
    
    return encoded_data;
}


    std::string formatNumber(double value, int decimals)
    {
        if(decimals < 0)
        {
            char buffer[64];
            auto result = std::to_chars(buffer, buffer + sizeof(buffer), value);
            if(result.ec != std::errc())
                throw std::runtime_error("Could not format number.");
            return std::string(buffer, result.ptr);
        }

        std::ostringstream oss;
        oss.precision(decimals); // Set precision to handle floating-point accuracy
        oss << std::fixed << value;
        std::string str = oss.str();

        const std::string::size_type decimal_point = str.find('.');
        if(decimal_point != std::string::npos)
        {
            while(str.size() > decimal_point + 1 && str.back() == '0')
                str.pop_back();
            if(str.back() == '.')
                str.pop_back();
        }

        return str;
    }

    std::string format_json_number(double value)
    {
        if(!std::isfinite(value))
            return "null";

        char buffer[64];
        auto result = std::to_chars(buffer, buffer + sizeof(buffer), value);
        if(result.ec != std::errc())
            throw std::runtime_error("Could not format JSON number.");
        return std::string(buffer, result.ptr);
    }


    prime::prime(): last_prime(1) {}

    bool 
    prime::test(long n)
    {
        if(n <= 1)
            return false;
        if(n <= 3)
            return true;
        if(n % 2 == 0 || n % 3 == 0)
            return false;
        for(long divisor = 5; divisor <= n / divisor; divisor += 6)
        {
            if(n % divisor == 0 || n % (divisor + 2) == 0)
                return false;
        }
        return true;
    }


    long 
    prime::next()
    {
        if(last_prime == std::numeric_limits<long>::max())
            throw std::overflow_error("No larger prime can be represented as long.");
        long candidate = last_prime + 1;

        while(!test(candidate))
        {
            if(candidate == std::numeric_limits<long>::max())
                throw std::overflow_error("No larger prime can be represented as long.");
            candidate++;
        }

        last_prime = candidate;
        return last_prime;
    }

    long
    character_sum(const std::string & s)
    {
        long sum = 0;
        for(unsigned char byte : s)
        {
            if(sum > std::numeric_limits<long>::max() - byte)
                throw std::overflow_error("Character sum cannot be represented as long.");
            sum += byte;
        }
        return sum;
    }

        std::string to_hex(char c)
    {
        const char* hex_digits = "0123456789ABCDEF";
        std::string hex_str;
        hex_str += hex_digits[(c >> 4) & 0xF];
        hex_str += hex_digits[c & 0xF];
        return hex_str;
    }

    bool
    is_valid_utf8(const std::string & str)
    {
        for(size_t i = 0; i < str.size();)
        {
            const unsigned char first = static_cast<unsigned char>(str[i]);
            if(first <= 0x7F)
            {
                ++i;
                continue;
            }

            size_t length = 0;
            unsigned char second_min = 0x80;
            unsigned char second_max = 0xBF;
            if(first >= 0xC2 && first <= 0xDF)
                length = 2;
            else if(first == 0xE0)
            {
                length = 3;
                second_min = 0xA0;
            }
            else if(first >= 0xE1 && first <= 0xEC)
                length = 3;
            else if(first == 0xED)
            {
                length = 3;
                second_max = 0x9F;
            }
            else if(first >= 0xEE && first <= 0xEF)
                length = 3;
            else if(first == 0xF0)
            {
                length = 4;
                second_min = 0x90;
            }
            else if(first >= 0xF1 && first <= 0xF3)
                length = 4;
            else if(first == 0xF4)
            {
                length = 4;
                second_max = 0x8F;
            }
            else
                return false;

            if(i + length > str.size())
                return false;

            const unsigned char second = static_cast<unsigned char>(str[i + 1]);
            if(second < second_min || second > second_max)
                return false;
            for(size_t j = 2; j < length; ++j)
            {
                const unsigned char continuation = static_cast<unsigned char>(str[i + j]);
                if(continuation < 0x80 || continuation > 0xBF)
                    return false;
            }
            i += length;
        }
        return true;
    }


    std::string
    decode_url_component(const std::string & str, bool plus_as_space)
    {
        std::string decoded;
        decoded.reserve(str.size());
        for(size_t i = 0; i < str.size(); ++i)
        {
            if(str[i] == '+' && plus_as_space)
            {
                decoded += ' ';
                continue;
            }
            if(str[i] != '%')
            {
                decoded += str[i];
                continue;
            }
            if(i + 2 >= str.size())
                throw std::invalid_argument("Incomplete URL escape sequence.");

            auto hex_value = [](char c)
            {
                if(c >= '0' && c <= '9')
                    return c - '0';
                if(c >= 'a' && c <= 'f')
                    return 10 + c - 'a';
                if(c >= 'A' && c <= 'F')
                    return 10 + c - 'A';
                return -1;
            };

            int high = hex_value(str[i + 1]);
            int low = hex_value(str[i + 2]);
            if(high < 0 || low < 0)
                throw std::invalid_argument("Invalid URL escape sequence.");
            decoded += static_cast<char>((high << 4) | low);
            i += 2;
        }
        return decoded;
    }


    std::string escape_json_string(const std::string& str)
    {
        if(!is_valid_utf8(str))
            throw std::invalid_argument("JSON strings must contain valid UTF-8.");

        std::string escaped;
        escaped.reserve(str.size());
        for (unsigned char c : str)
        {
            switch (c)
            {
                case '"': escaped += "\\\""; break;
                case '\\': escaped += "\\\\"; break;
                case '\b': escaped += "\\b"; break;
                case '\f': escaped += "\\f"; break;
                case '\n': escaped += "\\n"; break;
                case '\r': escaped += "\\r"; break;
                case '\t': escaped += "\\t"; break;
                default:
                    if(c < 0x20)
                        escaped += "\\u00" + to_hex(static_cast<char>(c));
                    else
                        escaped += c;
            }
        }
        return escaped;
    }

    std::string 
    replace_characters(const std::string& str) // Removes , ; and non-breaking space
    {
        std::string result;
        for (size_t i = 0; i < str.size(); ++i) 
        {
            unsigned char c = static_cast<unsigned char>(str[i]);
            if(c == 0xC2 && i + 1 < str.size() && static_cast<unsigned char>(str[i + 1]) == 0xA0) 
            {
                result += ' ';
                ++i; // Skip next byte
            }
            else if(str[i] == ',' || str[i] == ';')
                result += " ";
            else
                result += str[i];
        }
        return result;
    }
    
    
    std::string 
    remove_comment(const std::string& input) 
    {
        std::string result;
        result.reserve(input.size());

        bool in_comment = false;
        for(char c : input)
        {
            if(in_comment)
            {
                if(c == '\n')
                {
                    in_comment = false;
                    result += c;
                }
                continue;
            }

            if(c == '#')
            {
                in_comment = true;
                continue;
            }

            result += c;
        }

        return result;
    }
    
};
