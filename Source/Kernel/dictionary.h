// dictionary.h (c) Christian Balkenius 2023-2024
// Recursive JSON-like dictionary

#pragma once


#include <string>
#include <vector>
#include <set>
#include <map>
#include <unordered_map>
#include <variant>
#include <memory>
#include <initializer_list>
#include <iterator>
#include <iostream>
#include <cctype>
#include <stdexcept>

#include "utilities.h"
#include "xml.h"

namespace ikaros 
{


    struct null;
    struct dictionary;
    struct list;
    struct value;

    using valueVariant = std::variant<bool, double, null, std::string, list, dictionary>;
    using mapPtr = std::shared_ptr<std::unordered_map<std::string, value>>;
    using listPtr = std::shared_ptr<std::vector<value>>;
    using exclude_set = const std::set<std::string> &;      // "a/b" = element b in a; "a.b" attribute b in a

    static std::vector<value> empty;

    struct null
    {
        operator std::string () const;
        [[nodiscard]] std::string json() const;
        std::string xml(std::string name, exclude_set exclude={}, int depth=0) const;
        friend std::ostream& operator<<(std::ostream& os, const null & v);

        void print() const { std::cout << "null\n"; };
    };


    struct dictionary
    {
        mapPtr dict_; // Shared storage: copied dictionaries refer to the same underlying map unless copy() is used.

        using iterator = std::unordered_map<std::string, value>::iterator;
        using const_iterator = std::unordered_map<std::string, value>::const_iterator;


        iterator begin() noexcept { return dict_->begin(); }
        iterator end() noexcept { return dict_->end(); }
        const_iterator begin() const noexcept { return dict_->begin(); }
        const_iterator end() const noexcept { return dict_->end(); }
        const_iterator cbegin() const noexcept { return dict_->cbegin(); }
        const_iterator cend() const noexcept { return dict_->cend(); }

        dictionary();
        dictionary(XMLElement * xml);
        [[deprecated("Use load_xml() for file loading instead of dictionary(std::string).")]]
        explicit dictionary(std::string filename);
        dictionary(const std::initializer_list<std::pair<std::string, std::string>>& init_list);

        //dictionary(const dictionary & d);
    
        value & operator[](const std::string & s); // Creates the key on demand if it does not exist.
        const value & operator[](const std::string & s) const;
        value & at(const std::string & s);  // throws if s is not in dictionary
        const value & at(const std::string & s) const;
        [[nodiscard]]
        bool contains(const std::string & s);
        [[nodiscard]]
        bool contains(const std::string & s) const;
        [[nodiscard]]
        bool contains_non_null(const std::string & s);
        [[nodiscard]]
        bool contains_non_null(const std::string & s) const;
        [[nodiscard]]
        size_t count(const std::string & s);
        [[nodiscard]]
        size_t count(const std::string & s) const;
        [[nodiscard]]
        bool empty() const { return dict_->empty(); }
        void merge(const dictionary & source, bool overwrite=false); // shallow merge: copy from source to this

        void erase(const std::string & key);

        operator std::string () const;
        
        [[nodiscard]] int get_int(const std::string & s) const;
        [[nodiscard]] bool is_set(const std::string & s) const;    // Returns true if set and true, and false if not set or not set to true, bool or string
        [[nodiscard]] bool is_not_set(const std::string & s) const;    // Negation of is_set

        [[nodiscard]] std::string json() const;
        std::string xml(std::string name="dictionary", exclude_set exclude={}, int depth=0) const;
        friend std::ostream& operator<<(std::ostream& os, const dictionary & v);
        //void print();

        void parse_url(const std::string & s);
        // void parse_json(std::string s);
        // void parse_xml(std::string s); // TODO

        [[nodiscard]] dictionary copy() const;

        void print() const { std::cout << this->json() << '\n'; };

        void load_xml(const std::string & filename);
        void load_json(const std::string & filename);
    };


    struct list
    {
        listPtr list_; // Shared storage: copied lists refer to the same underlying vector unless copy() is used.

        list();
    
        using iterator = std::vector<value>::iterator;
        using const_iterator = std::vector<value>::const_iterator;

        iterator begin() noexcept { return list_->begin(); }
        iterator end() noexcept { return list_->end(); }
        const_iterator begin() const noexcept { return list_->begin(); }
        const_iterator end() const noexcept { return list_->end(); }
        const_iterator cbegin() const noexcept { return list_->cbegin(); }
        const_iterator cend() const noexcept { return list_->cend(); }

        iterator erase(const_iterator pos) { return list_->erase(pos); }
        iterator insert(const_iterator pos, const value & v) { return list_->insert(pos, v); }

        value & operator[] (int i); // Auto-resizes with null values up to i before returning the element.
        value & operator[] (size_t i); // Auto-resizes with null values up to i before returning the element.
        [[nodiscard]] size_t size() const { return list_->size(); };
        [[nodiscard]] bool empty() const { return list_->empty(); }
        list & push_back(const value & v) { list_->push_back(v); return *this; };
        list & insert_front(const value & v) { list_->insert(list_->begin(), v);  return *this; }
        list & erase(int index);
        list & erase(size_t index);
        operator std::string ()  const;
        [[nodiscard]] std::string json() const;
        std::string xml(std::string name, exclude_set exclude={}, int depth=0) const;
        friend std::ostream& operator<<(std::ostream& os, const list & v);

        [[nodiscard]] list copy() const;

        void print() const { std::cout << this->json() << '\n'; };
    };


    struct value
    {
        valueVariant    value_;

        value(bool v)               { value_ = v; }
        value(double v)             { value_ = v; }
        value(null =null())         { value_ = null(); }
        value(std::nullptr_t)       { value_ = null(); }
        value(const char * s)       { value_ = s; }
        value(const std::string & s){ value_ = s; }
        value(const list & v)       { value_ = v; }
        value(const dictionary & d) { value_ = d; }

        value & operator =(bool v) { value_ = v; return *this; }
        value & operator =(int v) { value_ = double(v); return *this; }
        value & operator =(double v) { value_ = double(v); return *this; }
        value & operator =(null) { value_ = null(); return *this; };
        value & operator =(std::nullptr_t) { value_ = null(); return *this; }
        value & operator =(const std::string & s) { value_ = s; return *this; }
        value & operator =(const char * s) { value_ = s; return *this; }
        value & operator =(const list & v) { value_ = v; return *this; }
        value & operator =(const dictionary & d) { value_ = d; return *this; }

        [[nodiscard]] bool is_dictionary() const { return std::holds_alternative<dictionary>(value_); }
        [[nodiscard]] bool is_list() const { return std::holds_alternative<list>(value_); }
        [[nodiscard]] bool is_null() const { return std::holds_alternative<null>(value_); }
        [[nodiscard]] bool is_bool() const { return std::holds_alternative<bool>(value_); }
        [[nodiscard]] bool is_number() const { return std::holds_alternative<double>(value_); }
        [[nodiscard]] bool is_string() const { return std::holds_alternative<std::string>(value_); }
        [[nodiscard]] bool is_true() const;

        [[nodiscard]] bool as_bool() const { return double(*this) != 0; };
        [[nodiscard]] int as_int() const { return double(*this); };
        [[nodiscard]] float as_float() const { return double(*this); };
        [[nodiscard]] double as_double() const { return double(*this); };
        [[nodiscard]] std::string as_string() const { return std::string(*this); };

        value & operator[] (const char * s); // Captures literals as argument ***************
        value & operator[] (const std::string & s); // Converts null/non-dictionary values into a dictionary before indexing.
        value & at(const std::string & s); // throws if not dictionary or non-existent attribute
        value & operator[] (int i); // Converts null/non-list values into a list and auto-resizes with null values up to i.
        value & operator[] (size_t i); // Converts null/non-list values into a list and auto-resizes with null values up to i.

        value & push_back(const value & v);

        friend std::ostream& operator<<(std::ostream& os, const value & v);
        [[nodiscard]] size_t size() const;

        std::vector<value>::iterator begin();   // value iterator ******* over what?? **********
        std::vector<value>::iterator end();

        operator std::string () const;
        [[nodiscard]] std::string json() const;
        std::string xml(std::string name, exclude_set exclude={}, int depth=0) const;

        operator double () const;
        operator list ();
        operator dictionary ();

        void print() const { std::cout << this->json() << '\n'; };

        [[nodiscard]] value copy() const;
    };

    [[nodiscard]] value parse_json(const std::string& json_str);
};
