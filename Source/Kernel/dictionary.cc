
// dictionary.h  (c) Christian Balkenius 2023-2024

#include "dictionary.h"
#include "utilities.h"

#include <cctype>
#include <stdexcept>

using namespace ikaros;
namespace ikaros 
{

        static std::string escape_json_string(std::string str) // FIXME: Do this correctly later
        {
            std::replace(str.begin(), str.end(), '\n', ' ');
            std::replace(str.begin(), str.end(), '\t', ' ');
            return str;
        }


        static void skip_whitespace(const std::string& s, size_t& pos)
        {
            while (pos<s.length() && std::isspace(s[pos]))
                ++pos;
        }

    // null

    null::operator std::string () const
    {
        return "";  // FIXME: Or "null" 
    }

    std::string 
    null::json() const
    {
        return "null";
    }

    std::string 
    null::xml(std::string name, int depth, std::string exclude)
    {
        return tab(depth)+"<null/>\n";
    }

    std::ostream&
    operator<<(std::ostream& os, const null & v)
    {
        os << "null";
        return os;
    }


    // list

    list::list():
        list_(std::make_shared<std::vector<value>>())
    {
    }

    list::operator std::string () const
    {
        std::string s = "[";
        std::string sep = "";
        for(auto & v : *list_)
        {
            s += sep + std::string(v);
            sep = ", ";
        }
        s += "]";
        return s;
    }


        list & 
        list::erase(int index)  
        {
                list_->erase(list_->begin()+index); return *this;
        }

    std::string 
    list::json() const
    {
        std::string s = "[";
        std::string sep = "";
        for(auto & v : *list_)
        {
            s += sep + v.json();
            sep = ", ";
        }
        s += "]";
        return s;
    }



       value & 
       list::operator[] (int i)
        {
            if( list_->size() < i+1)
                 list_->resize(i+1);
            return  list_->at(i);
        }


    list list::copy() const
    {
        list new_list;
        for (const auto& v : *list_)
        {
            new_list.push_back(v.copy());
        }
        return new_list;
    }


        std::ostream& 
        operator<<(std::ostream& os, const list & v)
        {
            os << std::string(v);
            return os;
        }

// dictionary


    value & 
    dictionary::operator[](std::string s)
    {
        return (*dict_)[s];
    }

    value & 
    dictionary::at(std::string s)
    {
        return dict_->at(s);
    }

    int 
    dictionary::get_int(std::string s)
    {
        return (*dict_)[s].as_int();
    }

    bool
    dictionary::is_set(std::string s)
    {
        if(!contains(s))
            return false;

        return (*dict_)[s].is_true();
    }

    bool
    dictionary::is_not_set(std::string s)
    {
        return !is_set(s);
    }

    bool 
    dictionary::contains(std::string s)
        {
            return dict_->count(s);
        }


        size_t 
        dictionary::count(std::string s)
        {
            return dict_->count(s);
        }
/*
        dictionary::dictionary(const dictionary & d)
        {
            //std::cout << "COPY CONSTRUCTOR" << std::endl;
            dict_ = d.dict_;
        }
*/
        dictionary::dictionary():   
            dict_(std::make_shared<std::map<std::string, value>>())
        {};

        dictionary::dictionary(const std::initializer_list<std::pair<std::string, std::string>>& init_list)
        {
            dict_ = std::make_shared<std::map<std::string, value>>();
            for (const auto& [key, val] : init_list)
                (*dict_)[key] = value(val);
        }

        void dictionary::merge(const dictionary & source, bool overwrite) // shallow merge: copy from source to this
        {
            for(auto p : *(source.dict_))
                if(!dict_->count(p.first) || overwrite)
                    (*dict_)[p.first] = p.second;
        }
        
        int dictionary::get_index(std::string key) // Returns the index of the key in the dictionary
        {
            int index = 0;
            for (const auto& [k, v] : *dict_)
            {
                if (k == key)
                {
                    return index;
                }
                index++;
            }
            return -1; // Key not found
        }


    dictionary::operator std::string () const
    {
        std::string s = "{";
        std::string sep = "";
        for(auto & v : *dict_)
        {
            s += sep + "" + v.first + ": " + std::string(v.second);
            sep = ", ";
        }
        s += "}";
        return s;
    }



    std::string  
    dictionary::json() const
    {
        std::string s = "{";
        std::string sep = "";
        for(auto & v : *dict_)
        {
            s += sep + "\"" + v.first + "\": " + v.second.json();
            sep = ", ";
        }
        s += "}";
        return s;
    }


    // XML

    std::string 
    list::xml(std::string name, int depth, std::string exclude)
    {
        std::string s;
        for(auto & v : *list_)
        {
            s += v.xml(name, depth+1);
        }
        return s;
    }

    // FIXME: Use _tag for elememt name

    std::string  
    dictionary::xml(std::string name, int depth, std::string exclude)
    {
        std::string s = tab(depth)+"<"+name;
        for(auto & a : *dict_)
            if(!a.second.is_list())
                if(!a.second.is_null()) // Do not include null attributes - but include empty strings
                    if(a.first != "_tag") // Do not include tag attributes since the are used as element name
                        s += " "+a.first + "=\"" +std::string(a.second)+"\"";

        std::string sep = ">\n";
        for(auto & e : *dict_)
            if(e.second.is_list()) //  && e.first!="inputs" && e.first!="outputs" && e.first!="parameters"
            {
                s += sep + e.second.xml(e.first.substr(0, e.first.size()-1), depth);
                sep = "";
            }
        if(sep.empty())
            s += tab(depth)+"</"+name+">\n";
        else
            s +="/>\n";
        return s;
    }


    dictionary::dictionary(XMLElement * xml_node):
        dictionary()
    {
        (*dict_)["_tag"] = xml_node->name;
        for(XMLAttribute * a = xml_node->attributes; a!=nullptr; a=(XMLAttribute *)(a->next))
            (*dict_)[std::string(a->name)] = a->value;

        for (XMLElement * xml_element = xml_node->GetContentElement(); xml_element != nullptr; xml_element = xml_element->GetNextElement())
            //if(merge.empty())
                (*dict_)[std::string(xml_element->name)+"s"].push_back(dictionary(xml_element));
            //else
            //    (*dict_)["elements"].push_back(dictionary(xml_element));
    }

    dictionary::dictionary(std::string filename):
        dictionary(XMLDocument(filename.c_str()).xml)
    {
    }


    void 
    dictionary::parse_url(std::string s)
    {
        if(s.empty())
            return;

        for(auto p : split(s, "&"))
        {   
            std::string a = head(p,"=");
            (*this)[a] = p;
        }
    }

    dictionary dictionary::copy() const
    {
        dictionary new_dict;
        for (const auto& [key, val] : *dict_)
        {
            new_dict[key] = val.copy();
        }
        return new_dict;
    }



        std::ostream& operator<<(std::ostream& os, const dictionary & v)
        {
            os << std::string(v);
            return os;
        }
// value

    value & 
    value::operator[] (const char * s) // Captures literals as argument
        {
            return (*this)[std::string(s)];
        }

        value & 
        value::operator[] (const std::string & s)
        {
            if(!std::holds_alternative<dictionary>(value_))
                value_ = dictionary();
            return std::get<dictionary>(value_)[s];
        }


        value & 
        value::at(const std::string & s)
        {
            if(std::holds_alternative<dictionary>(value_))
                return std::get<dictionary>(value_).at(s);
            else
            throw std::out_of_range("Attribute \""+s+"\" not found.");  
        }


        std::ostream& operator<<(std::ostream& os, const value & v)
        {
            os << std::string(v);
            return os;
        }

        value &  
        value::push_back(const value & v)
        {
            if(!std::holds_alternative<list>(value_))
                value_ = list();
                std::get<list>(value_). list_->push_back(v);
                return *this;
        }

        int  
        value::size()
        {
            if(std::holds_alternative<list>(value_))
                return std::get<list>(value_). list_->size();
            else if(std::holds_alternative<dictionary>(value_))
                return std::get<dictionary>(value_).dict_->size();
            else if(std::holds_alternative<null>(value_))
                return 0;
            else
                return 1;
        }


        std::vector<value>::iterator value::begin()
        {
            if(std::holds_alternative<list>(value_))
                return std::get<list>(value_). list_->begin();
            else
                return empty.begin();
        }


        std::vector<value>::iterator value::end()
        {
            if(std::holds_alternative<list>(value_))
                return std::get<list>(value_). list_->end();
            else
                return empty.end();
        }


        value &   
        value::operator[] (int i)
        {
            if(!std::holds_alternative<list>(value_))
                value_ = list();
            if(std::get<list>(value_). list_->size() < i+1)
                std::get<list>(value_). list_->resize(i+1);
            return std::get<list>(value_). list_->at(i);
        }

        value::operator std::string () const
        {
            if(std::holds_alternative<bool>(value_))
                return std::get<bool>(value_) ? "true" : "false";
            //if(std::holds_alternative<int>(value_))
            //    return std::to_string(std::get<int>(value_));
            if(std::holds_alternative<double>(value_))
                return formatNumber(std::get<double>(value_));
                //return std::to_string(std::get<double>(value_));
            if(std::holds_alternative<std::string>(value_))
                return std::get<std::string>(value_);
            else if(std::holds_alternative<list>(value_))
                return std::string(std::get<list>(value_));
            else if(std::holds_alternative<dictionary>(value_))
                return std::string(std::get<dictionary>(value_));
            else if(std::holds_alternative<null>(value_))
                return "null";

           throw std::runtime_error("Unknown variant");
        }

        bool 
        value::is_true()
        {
            if(std::holds_alternative<bool>(value_))
                return std::get<bool>(value_);
            else if(std::holds_alternative<null>(value_))
                return false;
            else if(std::holds_alternative<double>(value_))
                return std::get<double>(value_) != 0;
            else if(std::holds_alternative<std::string>(value_))
                return ::ikaros::is_true(std::get<std::string>(value_));
            else
                return false;
        }


        std::string  
        value::json() const
        {
            if(std::holds_alternative<std::string>(value_))
                return "\""+escape_json_string(std::get<std::string>(value_))+"\"";
            else if(std::holds_alternative<list>(value_))
                return std::get<list>(value_).json();
            else if(std::holds_alternative<dictionary>(value_))
                return std::get<dictionary>(value_).json();
            else if(std::holds_alternative<null>(value_))
                return "null";
            else
                return std::string(*this);
        }

        std::string  
        value::xml(std::string name, int depth, std::string exclude)
        {
            if(std::holds_alternative<std::string>(value_))
                return tab(depth)+"<string>"+std::get<std::string>(value_)+"</string>\n"; // FIXME: <'type' value="v" />    ???
            else if(std::holds_alternative<list>(value_))
                return std::get<list>(value_).xml(name, depth);
            else if(std::holds_alternative<dictionary>(value_))
                return std::get<dictionary>(value_).xml(name, depth);
            else if(std::holds_alternative<null>(value_))
                return tab(depth)+"<null/>\n";
            else
                return std::string(*this);                              // FIXME: <'type' value="v" />    ???
        }

          
        value::operator double ()
        { 
            if(std::holds_alternative<double>(value_))
                return std::get<double>(value_);
            if(std::holds_alternative<std::string>(value_))
                return std::stod(std::get<std::string>(value_));
            else  if(std::holds_alternative<null>(value_))
                return 0;

            throw std::runtime_error("Cannot convert to double");
        }

          
        value::operator list ()
        {
            return std::get<list>(value_);
        }

  
        value::operator dictionary ()
        {
            if(std::holds_alternative<null>(value_))
                return dictionary();
            else
                return std::get<dictionary>(value_);
        }


    value value::copy() const
{
        if(std::holds_alternative<null>(value_))
            return value(null());
        if(std::holds_alternative<bool>(value_))
            return value(std::get<bool>(value_));
        if(std::holds_alternative<double>(value_))
            return value(static_cast<double>(std::get<double>(value_)));
        if(std::holds_alternative<std::string>(value_))
            return value(std::get<std::string>(value_));
        if(std::holds_alternative<list>(value_))
            return value(std::get<list>(value_).copy());
        if(std::holds_alternative<dictionary>(value_))
            return value(std::get<dictionary>(value_).copy());    
        return value();
    }

    std::string parse_string(const std::string& s, size_t& pos)
    {
        if(s[pos] != '"')
            throw std::runtime_error("Expected '\"' at the beginning of string");

        ++pos; // Skip the opening quote
        std::string result;
        while (pos < s.length() && s[pos] != '"')
        {
            if(s[pos] == '\\')
            {
                ++pos; // Skip the escape character
                if(pos >= s.length())
                    throw std::runtime_error("Unexpected end of string");
                switch (s[pos])
                {
                    case '"': result += '"'; break;
                    case '\\': result += '\\'; break;
                    case '/': result += '/'; break;
                    case 'b': result += '\b'; break;
                    case 'f': result += '\f'; break;
                    case 'n': result += '\n'; break;
                    case 'r': result += '\r'; break;
                    case 't': result += '\t'; break;
                    default:
                        throw std::runtime_error("Invalid escape sequence");
                }
            }
            else
            {
                result += s[pos];
            }
            ++pos;
        }
        if(pos >= s.length() || s[pos] != '"')
            throw std::runtime_error("Expected '\"' at the end of string");

        ++pos; // Skip the closing quote
        return result;
    }

    value parse_value(const std::string& s, size_t& pos);

    list parse_array(const std::string& s, size_t& pos)
    {
        if(s[pos] != '[')
            throw std::runtime_error("Expected '[' at the beginning of array");

        ++pos; // Skip the opening bracket
        list result;
        skip_whitespace(s, pos);
        if(s[pos] == ']')
        {
            ++pos; // Skip the closing bracket
            return result;
        }

        while (pos < s.length())
        {
            result.push_back(parse_value(s, pos));
            skip_whitespace(s, pos);
            if(s[pos] == ']')
            {
                ++pos; // Skip the closing bracket
                return result;
            }
            if(s[pos] != ',')
                throw std::runtime_error("Expected ',' in array");
            ++pos; // Skip the comma
            skip_whitespace(s, pos);
        }

        throw std::runtime_error("Unexpected end of array");
    }

    dictionary parse_object(const std::string& s, size_t& pos)
    {
        if(s[pos] != '{')
            throw std::runtime_error("Expected '{' at the beginning of object");

        ++pos; // Skip the opening brace
        dictionary result;
        skip_whitespace(s, pos);
        if(s[pos] == '}')
        {
            ++pos; // Skip the closing brace
            return result;
        }

        while (pos < s.length())
        {
            skip_whitespace(s, pos);
            std::string key = parse_string(s, pos);
            skip_whitespace(s, pos);
            if(s[pos] != ':')
                throw std::runtime_error("Expected ':' in object");
            ++pos; // Skip the colon
            skip_whitespace(s, pos);
            result[key] = parse_value(s, pos);
            skip_whitespace(s, pos);
            if(s[pos] == '}')
            {
                ++pos; // Skip the closing brace
                return result;
            }
            if(s[pos] != ',')
                throw std::runtime_error("Expected ',' in object");
            ++pos; // Skip the comma
            skip_whitespace(s, pos);
        }

        throw std::runtime_error("Unexpected end of object");
    }

    value parse_value(const std::string& s, size_t& pos)
    {
        skip_whitespace(s, pos);
        if(pos >= s.length())
            throw std::runtime_error("Unexpected end of JSON");

        if(s[pos] == 'n')
        {
            if(s.compare(pos, 4, "null") == 0)
            {
                pos += 4;
                return value(null());
            }
        }
        else if(s[pos] == 't')
        {
            if(s.compare(pos, 4, "true") == 0)
            {
                pos += 4;
                return value(true);
            }
        }
        else if(s[pos] == 'f')
        {
            if(s.compare(pos, 5, "false") == 0)
            {
                pos += 5;
                return value(false);
            }
        }
        else if(s[pos] == '"')
        {
            return value(parse_string(s, pos));
        }
        else if(s[pos] == '[')
        {
            return value(parse_array(s, pos));
        }
        else if(s[pos] == '{')
        {
            return value(parse_object(s, pos));
        }
        else if(std::isdigit(s[pos]) || s[pos] == '-')
        {
            size_t end_pos = pos;
            while (end_pos < s.length() && (std::isdigit(s[end_pos]) || s[end_pos] == '.' || s[end_pos] == 'e' || s[end_pos] == 'E' || s[end_pos] == '+' || s[end_pos] == '-'))
            {
                ++end_pos;
            }
            value num_value = std::stod(s.substr(pos, end_pos - pos));
            pos = end_pos;
            return num_value;
        }

        throw std::runtime_error("Invalid JSON value");
    }


value parse_json(const std::string& json_str)
{
    size_t pos = 0;
    return parse_value(json_str, pos);
}

};