// utilities.cc

#include "utilities.h"

namespace ikaros
{

std::string trim(const std::string &s)
{
   auto wsfront=std::find_if_not(s.begin(),s.end(),[](int c){return std::isspace(c);});
   auto wsback=std::find_if_not(s.rbegin(),s.rend(),[](int c){return std::isspace(c);}).base();
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
            while (i < len && ::isspace(s[i]))
                i++;
            j = i;
            while (i < len && !::isspace(s[i]))
                i++;

            if(j < i)
            {
                if(maxsplit != -1 && maxsplit-- <= 0)
                    break;
                r.push_back(trim(s.substr(j, i-j)));
                while(i < len && ::isspace(s[i]))
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
    if (maxsplit < 0)   // FIXME: Is this necessary?? WHY???
        return split(str, sep, maxsplit);

    std::vector<std::string> r;
    std::string::size_type i=str.size();
    std::string::size_type j=str.size();
    std::string::size_type n=sep.size();

    if(n == 0)
    {
        while(i > 0)
        {
            while(i > 0 && ::isspace(str[i-1]))
                i--;
            j = i;
            while(i > 0 && !::isspace(str[i-1]))
                i--;

            if (j > i)
            {
                if(maxsplit != -1 && maxsplit-- <= 0)
                    break;
                r.push_back(str.substr(i, j-i));
                while(i > 0 && ::isspace(str[i-1]))
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
    int end = s.find(delimiter);
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
rhead(std::string & s, const std::string & delimiter)
{
    int end = s.rfind(delimiter);
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
    int end = s.find(delimiter);
    if(end == std::string::npos)
        return "";

    std::string t = s.substr(end+delimiter.length());
    s.erase(end);
    return t;
}



std::string
rtail(std::string & s, const std::string & delimiter)
{
    int end = s.rfind(delimiter);
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
    int end = s.find(delimiter);
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
    int end = s.rfind(delimiter);
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
    int end = s.find(delimiter);
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
    int end = s.rfind(delimiter);
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


bool contains(std::string & s, std::string n)
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
    static std::vector<std::string> true_list = {"true", "True", "yes", "YES", "on", "ON","1"};
    return std::find(true_list.begin(), true_list.end(), s) != true_list.end();
}





bool is_number(const std::string& s) 
{
    char* endPtr = nullptr;
    errno = 0;
    double val = std::strtod(s.c_str(), &endPtr);
    if (endPtr == s.c_str() || *endPtr != '\0') 
        return false;

    if (errno == ERANGE)
        return false;

    if (val == std::numeric_limits<double>::infinity() || val == -std::numeric_limits<double>::infinity())
        return false;

    return true;
}





std::ostream& operator<<(std::ostream& os, const std::vector<int> & v)
{
    std::string sep;
    std::cout << "(";
    for(auto i : v)
    {
        std::cout << sep << i;
        sep = ", ";
    }
    std::cout << ")";
    return os;
}



// Utility functions

    // auto tab = [](int d){ return std::string(3*d, ' ');};

    void 
    print_attribute_value(const std::string & name, int value, int indent)
    {
        std::cout << name << " = " << value <<  std::endl;
    }

    void 
    print_attribute_value(const std::string & name, const std::string & value, int indent)
    {
        std::cout << name << " = " << value <<  std::endl;
    }

    void 
    print_attribute_value(const std::string & name, const std::vector<int> & values, int indent, int max_items)
    {
                std::cout << name << " = ";
        int s = values.size();
        if(max_items>0 && s>max_items)
            s = max_items;

        for(int i=0; i<s; i++)
            std::cout << values.at(i) << " ";
        if(values.size() >= max_items && max_items>0)
            std::cout << "..." << std::endl;
        std::cout << std::endl;
    }
   
    void 
    print_attribute_value(const std::string name, std::vector<float> & values, int indent, int max_items)
    {
                std::cout << name << " = ";
        int s = values.size();
        if(max_items>0 && s>max_items && max_items>0)
            s = max_items;

        for(int i=0; i<s; i++)
            std::cout << values.at(i) << " ";
        if(values.size() >= max_items)
            std::cout << "..." << std::endl;
        std::cout << std::endl;
    }

    void
    print_attribute_value(const std::string & name, const std::vector<std::vector<std::string>> &  values, int indent, int max_items)
    {
        std::cout << name << " = " << std::endl;
        for(auto d : values)
        {
            std::cout << tab(1);
            if(d.empty())
                std::cout << "none" << std::endl;
            else
                for(auto s : d)
                    std::cout << s << " ";
           std::cout << std::endl; 
        }
        std::cout << std::endl;
    }


char *
base64_encode(const unsigned char * data,
              size_t size_in,
              size_t *size_out)
{
    static char encoding_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    static int mod_table[] = {0, 2, 1};
    *size_out = ((size_in - 1) / 3) * 4 + 4;
    
    char *encoded_data = (char *)malloc(*size_out);
    if (encoded_data == NULL) return NULL;
    
    for (int i = 0, j = 0; i < size_in;)
    {
        unsigned int octet_a = i < size_in ? data[i++] : 0;
        unsigned int octet_b = i < size_in ? data[i++] : 0;
        unsigned int octet_c = i < size_in ? data[i++] : 0;
        
        unsigned int triple = (octet_a << 0x10) + (octet_b << 0x08) + octet_c;
        
        encoded_data[j++] = encoding_table[(triple >> 3 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 2 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 1 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 0 * 6) & 0x3F];
    }
    
    for (int i = 0; i < mod_table[size_in % 3]; i++)
        encoded_data[*size_out - 1 - i] = '=';
    
    return encoded_data;
}


    std::string formatNumber(double value, int decimals)
    {
        std::ostringstream oss;
        oss.precision(decimals); // Set precision to handle floating-point accuracy
        oss << std::fixed << value;
        std::string str = oss.str();

        // Remove trailing zeros
        str.erase(str.find_last_not_of('0') + 1, std::string::npos);

        // Remove the decimal point if it's the last character
        if (str.back() == '.') {
            str.pop_back();
        }

        return str;
    }


    prime::prime(): last_prime(1) {}

    bool 
    prime::test(long n)
    {
        if (n <= 1) return false;
        if (n <= 3) 
            return true;
        if (n % 2 == 0 || n % 3 == 0) 
            return false;
        for (int i = 5; i * i <= n; i += 6)
        {
            if (n % i == 0 || n % (i + 2) == 0) 
                return false;
        }
        return true;
    }


    long 
    prime::next()
    {

        long candidate = last_prime + 1;

        while(!test(candidate))
            candidate++;

        last_prime = candidate;
        return last_prime;
    }

    long
    character_sum(std::string s)
    {
        long sum = 0;
        for(char c : s)
            sum += c;
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

    std::string escape_json_string(const std::string& str)
    {
        std::string escaped;
        for (char c : str)
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
                    //if (c < 0x20 || c > 0x7E)
                    //    escaped += "\\u" + to_hex(c);
                    //else
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
        std::string result = input;
        size_t pos = result.find('#'); 
        if(pos != std::string::npos) 
            result.erase(pos);
    
        return result;
    }
    
};

