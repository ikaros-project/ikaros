// options.h   (c) Christian Balkenius 2023

#ifndef OPTIONS
#define OPTIONS

#include <string>
#include <map>
#include <iostream>
#include <filesystem>

namespace ikaros {

    class options
    {
    public:
        std::string ikaros_root;    // Path to binary as indicated in argv
        std::string filename;       // If only one, or empty string if none
        std::vector<std::string> filenames;
        std::map<std::string, std::string> d;
        std::map<std::string, std::string> full;
        std::map<std::string, std::string> description;

        options() {};

        void add_option(std::string short_name, std::string full_name, std::string desc, std::string default_value="")
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
                }
                else if(s.size()>2 && s.front() =='-')
                {
                    std::string attr = s.substr(1, 1);
                    if(!full.count(attr))
                        throw std::runtime_error("\"-"+attr + "\" is not a valid command line option");
                    d[full[attr]] = s.substr(2);  // option with parameter
                }
                else if(s.size()>1 && s.front() =='-')
                {
                    std::string attr = s.substr(1, 1);
                    if(!full.count(attr))
                        throw std::runtime_error("\"-"+attr + "\" is not a valid command line option");
                    d[full[attr]] = "true";  // option without parameter
                }
                else
                {
                    if (!std::filesystem::exists(s))
                            throw std::runtime_error("File not found: "+std::string(s));
                    filenames.push_back(s);
                    filename = s;
                }
            }
        }


        void print_help()
        {
            std::cout << "usage: ikaros [options] [variable=value] [filename]" << std::endl;
            std::cout << "\tCommand line options:" << std::endl;
            for(auto & p : full)
            {
                std::cout << "\t-"<< p.first << " (" << p.second << "): " << description[p.second];
                if(d.count(p.second))
                    std::cout << " [" << d[p.second] << "]"; 
                std::cout << std::endl;
            }
        }


        bool is_set(std::string o)
        {
            return d.count(o)>0;
        }

        void set(std::string o)
        {
            d[o] = "true";
        }

        std::string get(std::string o)
        { 
            if(d.count(o)>0)
                return d.at(o);
            else
                return "";
        }

        long get_long(std::string o)
        { 
            if(d.count(o)>0)
                return std::stol(d.at(o));
            else
                return 0;
        }

        double get_double(std::string o)
        { 
            if(d.count(o)>0)
                return std::stod(d.at(o));
            else
                return 0;
        }

    };

};
#endif
