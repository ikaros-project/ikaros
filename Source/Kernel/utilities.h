// utilities.h

#pragma once

#include <string>
#include <vector>
#include <iostream>
#include <sstream>

namespace ikaros
{
    const std::vector<std::string> split(const std::string & s, const std::string & sep, int maxsplit=-1);
    const std::vector<std::string> rsplit(const std::string & str, const std::string & sep, int maxsplit=-1);
    std::string join(const std::string & separator, const std::vector<std::string> & v, bool reverse);

    bool starts_with(const std::string & s, const std::string & start); // waiting for C++20
    bool ends_with(const std::string & s, const std::string & end); // waiting for C++20
    bool contains(std::string & s, std::string n); // does s contain n? // waiting for C++20
    
    // NEW HEAD AND TAIL FUNCTIONS

    std::string head(std::string & s, const std::string & delimiter); // return string before first delimiter and remove it from s together with the delimiter, peek = do not change s
    std::string tail(std::string & s, const std::string & delimiter); // return string after first delimiter and remove it from s together with the delimiter
    std::string rhead(std::string & s, const std::string & delimiter); // return string before last delimiter and remove it from s together with the delimiter
    std::string rtail(std::string & s, const std::string & delimiter); // return string after last delimiter and remove it from s together with the delimiter

    std::string peek_head(const std::string & s, const std::string & delimiter); // return string before first delimiter
    std::string peek_tail(const std::string & s, const std::string & delimiter, bool keep_delimiter=false); // return string after first delimiter
    std::string peek_rhead(const std::string & s, const std::string & delimiter); // return string before last delimiter
    std::string peek_rtail(const std::string & s, const std::string & delimiter); // return string after last delimiter

    std::string trim(const std::string &s);

    std::string cut_head(std::string & s, const std::string & delimiter); // return string before delimiter and remove it from s


    std::string add_extension(const std::string &  filename, const std::string & extension);

    bool is_integer(const std::string & s); // is s an interger?
    bool is_number(const std::string &s); // is s an int, float or double?
    // is_bool(s):; // Can s be converted to a boolean value?
    bool is_true(const std::string & s); // True if equal to 1 or start with T, t, Y, or y.

    std::ostream& operator<<(std::ostream& os, const std::vector<int> & v);

    // Utility functions

    auto tab = [](int d){ return std::string(3*d, ' ');};

    void print_attribute_value(const std::string & name, int value, int indent=0);
    void print_attribute_value(const std::string & name, const std::string & value, int indent=0);
    void print_attribute_value(const std::string & name, const std::vector<int> & values, int indent=0, int max_items=0);
    void print_attribute_value(const std::string name, std::vector<float> & values, int indent=0, int max_items=0);
    void print_attribute_value(const std::string & name, const std::vector<std::vector<std::string>> &  values, int indent=0, int max_items=0);

    char * base64_encode(const unsigned char * data, size_t size_in, size_t *size_out);

    std::string formatNumber(double value, int decimals=10); // remove trailing zeros

    class prime
    {
        public:
            long last_prime;
            bool test(long);    // test if number is a prime
            long next();        // generate next prime number
            prime();
    };

    long character_sum(std::string s); // sums the character codes interpreded as numbers

    std::string to_hex(char c);
    std::string escape_json_string(const std::string& str);

};  


