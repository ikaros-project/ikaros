// options.h   (c) Christian Balkenius 2023

#pragma once

#include <string>
#include <map>
#include <set>
#include <vector>
#include <iostream>
#include <filesystem>
#include <stdexcept>

namespace ikaros {

    class options
    {
    public:
        std::string ikaros_root;            // Path to binary as indicated in argv
        std::filesystem::path path_;       // If only one, or empty path if none
        std::vector<std::string> filenames;
        std::map<std::string, std::string> d;
        std::set<std::string> explicitly_set;
        std::map<std::string, std::string> full;
        std::map<std::string, std::string> description;

        options() = default;

        void add_option(const std::string & short_name, const std::string & full_name, const std::string & desc, const std::string & default_value = "")
        {
            full[short_name] = full_name;
            description[full_name] = desc;
            if(!default_value.empty())
                d[full_name] = default_value;
        }


        void parse_args(int argc, char *argv[])
        {
            if (argc > 64)
                throw std::runtime_error("Too many input parameters");
            if (argc < 1)
                throw std::runtime_error("Too few input parameters");

            std::filesystem::path p(argv[0]);
            ikaros_root = std::filesystem::canonical(p.parent_path().string()+"/..");

            const std::vector<std::string> args(argv+1, argv+argc);
            for(auto & s: args)
            {
                int pos = s.find('=');
                if(pos != std::string::npos && s.front() !='-')
                {
                    if(pos < 1)
                        throw std::runtime_error("Assignment without variable");
                    if(s.size() >3 && s.at(pos+1)== '"' && s.back()== '"')
                        d[s.substr(0, pos)] = s.substr(pos+2, s.size()-pos-3);
                    else
                        d[s.substr(0, pos)] = s.substr(pos+1, s.size()-pos);
                    explicitly_set.insert(s.substr(0, pos));
                }
                else if(s.size()>2 && s.front() =='-')
                {
                    std::string attr = s.substr(1, 1);
                    if(!full.count(attr))
                        throw std::runtime_error("\"-"+attr + "\" is not a valid command line option");
                    d[full[attr]] = s.substr(2);  // option with parameter
                    explicitly_set.insert(full[attr]);
                }
                else if(s.size()>1 && s.front() =='-')
                {
                    std::string attr = s.substr(1, 1);
                    if(!full.count(attr))
                        throw std::runtime_error("\"-"+attr + "\" is not a valid command line option");
                    d[full[attr]] = "true";  // option without parameter
                    explicitly_set.insert(full[attr]);
                }
                else
                {
                    if (!std::filesystem::exists(s))
                            throw std::runtime_error("File not found: "+std::string(s));
                    filenames.push_back(s);
                    path_ = s; // TEST ME
                }
            }
        }


        void print_help() const
        {
            std::cout << "usage: ikaros [options] [variable=value] [filename]\n";
            std::cout << "\tCommand line options:\n";
            for(auto & p : full)
            {
                std::cout << "\t-"<< p.first << " (" << p.second << "): " << description.at(p.second);
                if(d.count(p.second))
                    std::cout << " [" << d.at(p.second) << "]"; 
                std::cout << '\n';
            }
        }


        std::string
        full_path() const
        {
            return path_.string();
        }

        std::string
        stem() const
        {
            return path_.stem();
        }

        std::string
        filename() const
        {
            return path_.filename();
        }

        std::string
        parent_path() const
        {
            return path_.parent_path();
        }

        bool is_set(const std::string & o) const
        {
            return d.count(o)>0;
        }

        bool is_explicitly_set(const std::string & o) const
        {
            return explicitly_set.count(o)>0;
        }

        void set(const std::string & o)
        {
            d[o] = "true";
        }

        std::string get(const std::string & o) const
        { 
            if(d.count(o)>0)
                return d.at(o);
            else
                return "";
        }

        long get_long(const std::string & o) const
        { 
            if(d.count(o)>0)
                return std::stol(d.at(o));
            else
                return 0;
        }

        double get_double(const std::string & o) const
        { 
            if(d.count(o)>0)
                return std::stod(d.at(o));
            else
                return 0;
        }

    };

};
