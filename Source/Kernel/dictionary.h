// dictionary.h (c) Christian Balkenius 2023-2024
// Recursive JSON-like dictionary

#ifndef DICTIONARY
#define DICTIONARY


#include <string>
#include <vector>
#include <set>
#include <map>
#include <variant>
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
    using mapPtr = std::shared_ptr<std::map<std::string, value>>;
    using listPtr = std::shared_ptr<std::vector<value>>;

    static std::vector<value> empty;

    struct null
    {
        operator std::string () const;
        std::string json() const;
        std::string xml(std::string name, int depth=0, std::string exclude = "");
        friend std::ostream& operator<<(std::ostream& os, const null & v);

        void print() const { std::cout << "null" << std::endl; };
    };


    struct dictionary
    {
        mapPtr dict_;

        using iterator = std::map<std::string, value>::iterator;
        using const_iterator = std::map<std::string, value>::const_iterator;

        iterator begin() { return dict_->begin(); }
        iterator end() { return dict_->end(); }
        const_iterator begin() const { return dict_->begin(); }
        const_iterator end() const { return dict_->end(); }
        const_iterator cbegin() const { return dict_->cbegin(); }
        const_iterator cend() const { return dict_->cend(); }

        dictionary();
        dictionary(XMLElement * xml);
        dictionary(std::string filename);
        dictionary(const std::initializer_list<std::pair<std::string, std::string>>& init_list);

        //dictionary(const dictionary & d);
    
        value & operator[](std::string s);
        value & at(std::string s);  // throws if s is not in dictionary
        bool contains(std::string s);
        size_t count(std::string s);
        bool empty() { return dict_->empty(); }
        void merge(const dictionary & source, bool overwrite=false); // shallow merge: copy from source to this

        operator std::string () const;
        
        int get_int(std::string s);
        bool is_set(std::string s);    // Returns true if set and true, and false if not set or not set to true, bool or string
        bool is_not_set(std::string s);    // Negation of is_set
        int get_index(std::string key); // Returns the index of the key in the dictionary

        std::string json() const;
        std::string xml(std::string name, int depth=0, std::string exclude = "");
        friend std::ostream& operator<<(std::ostream& os, const dictionary & v);
        //void print();

        void parse_url(std::string s);
        // void parse_json(std::string s);
        // void parse_xml(std::string s); // TODO

        dictionary copy() const;

        void print() const { std::cout << this->json() << std::endl; };
    };


    struct list
    {
        listPtr list_;

        list();
    
        using iterator = std::vector<value>::iterator;
        using const_iterator = std::vector<value>::const_iterator;

        iterator begin() { return list_->begin(); }
        iterator end() { return list_->end(); }
        const_iterator begin() const { return list_->begin(); }
        const_iterator end() const { return list_->end(); }
        const_iterator cbegin() const { return list_->cbegin(); }
        const_iterator cend() const { return list_->cend(); }

        value & operator[] (int i);
        int size() { return list_->size(); };
        list & push_back(const value & v) { list_->push_back(v); return *this; };
        list & insert_front(const value & v) { list_->insert(list_->begin(), v);  return *this; }
        list & erase(int index);
        operator std::string ()  const;
        std::string json() const;
        std::string xml(std::string name, int depth=0, std::string exclude = "");
        friend std::ostream& operator<<(std::ostream& os, const list & v);

        list copy() const;

        void print() const { std::cout << this->json() << std::endl; };
    };


    struct value
    {
        valueVariant    value_;

        value(bool v)               { value_ = v; }
        value(double v)             { value_ = v; }
        value(null n=null())        { value_ = null(); }
        value(const char * s)       { value_ = s; }
        value(const std::string & s){ value_ = s; }
        value(const list & v)       { value_ = v; }
        value(const dictionary & d) { value_ = d; }

        value & operator =(bool v) { value_ = v; return *this; }
        value & operator =(double v) { value_ = double(v); return *this; }
        value & operator =(null n) { value_ = null(); return *this; };
        value & operator =(const std::string & s) { value_ = s; return *this; }
        value & operator =(const char * s) { value_ = s; return *this; }
        value & operator =(const list & v) { value_ = v; return *this; }
        value & operator =(const dictionary & d) { value_ = d; return *this; }

        bool is_dictionary()    { return std::holds_alternative<dictionary>(value_); }
        bool is_list()          { return std::holds_alternative<list>(value_); }
        bool is_null()          { return std::holds_alternative<null>(value_); }
        bool is_true();

        int as_int()            { return double(*this); };  // FIXME: CHECK THIS ONE

        value & operator[] (const char * s); // Captures literals as argument ***************
        value & operator[] (const std::string & s);
        value & at(const std::string & s); // throws if not dictionary or non-existent attribute
        value & operator[] (int i); // list shortcut ***************

        value & push_back(const value & v);

        friend std::ostream& operator<<(std::ostream& os, const value & v);
        int size();

        std::vector<value>::iterator begin();   // value iterator ******* over what?? **********
        std::vector<value>::iterator end();

        operator std::string () const;
        std::string json() const;
        std::string xml(std::string name, int depth=0, std::string exclude = "");

        operator double ();                                        // FIXME: Add other types - both from and to
        operator list ();
        operator dictionary ();

        void print() const { std::cout << this->json() << std::endl; };

        value copy() const;
    };

    value parse_json(const std::string& json_str);
};



#endif

