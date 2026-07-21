// options.h   (c) Christian Balkenius 2023

#pragma once

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

#include "utilities.h"

namespace ikaros {

    class options
    {
    public:
        std::string ikaros_root;            // Repository or installation root resolved from argv[0]
        std::filesystem::path path_;       // If only one, or empty path if none
        std::vector<std::string> filenames;
        std::map<std::string, std::string> d;
        std::set<std::string> explicitly_set;
        std::map<std::string, std::string> full;
        std::map<std::string, std::string> description;
        std::map<std::string, bool> requires_value;
        std::map<std::string, bool> optional_value;

        options() = default;

        void add_option(const std::string & short_name, const std::string & full_name,
                        const std::string & desc, bool takes_value = false,
                        const std::string & default_value = "", bool optional = false,
                        bool sensitive = false)
        {
            full[short_name] = full_name;
            description[full_name] = desc;
            requires_value[full_name] = takes_value;
            optional_value[full_name] = optional;
            if(!default_value.empty())
            {
                d[full_name] = default_value;
                default_values_[full_name] = default_value;
            }
            if(sensitive)
                sensitive_options_.insert(full_name);
        }


        void parse_args(int argc, char *argv[])
        {
            if (argc > 64)
                throw std::runtime_error("Too many input parameters");
            if (argc < 1)
                throw std::runtime_error("Too few input parameters");

            const std::filesystem::path executable_path = resolve_executable_path(argv[0]);
            ikaros_root = executable_path.parent_path().parent_path().string();

            const std::vector<std::string> args(argv+1, argv+argc);
            for(std::size_t i = 0; i < args.size(); ++i)
            {
                const auto & s = args[i];
                const auto pos = s.find('=');
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
                    std::string option_name = full[attr];
                    if(requires_value[option_name] || optional_value[option_name])
                        d[option_name] = s.substr(2);  // option with attached parameter
                    else
                        throw std::runtime_error("\"-"+attr + "\" does not accept an attached value");
                    explicitly_set.insert(option_name);
                }
                else if(s.size()>1 && s.front() =='-')
                {
                    std::string attr = s.substr(1, 1);
                    if(!full.count(attr))
                        throw std::runtime_error("\"-"+attr + "\" is not a valid command line option");
                    std::string option_name = full[attr];
                    if(optional_value[option_name])
                    {
                        const bool model_already_selected = !filenames.empty();
                        const bool later_model_candidate = has_later_positional_argument(args, i + 2);
                        if(i + 1 < args.size() &&
                           (model_already_selected || later_model_candidate) &&
                           !args[i + 1].empty() && args[i + 1].front() != '-' &&
                           args[i + 1].find('=') == std::string::npos)
                            d[option_name] = args[++i];
                        else
                            d[option_name] = "";
                    }
                    else if(requires_value[option_name])
                    {
                        if(i + 1 >= args.size())
                            throw std::runtime_error("\"-"+attr + "\" requires a value");
                        d[option_name] = args[++i];
                    }
                    else
                        d[option_name] = "true";  // option without parameter
                    explicitly_set.insert(option_name);
                }
                else
                {
                    if (!std::filesystem::exists(s))
                            throw std::runtime_error("File not found: "+std::string(s));
                    if(!filenames.empty())
                        throw std::runtime_error("Only one model file may be specified: \"" +
                                                 filenames.front() + "\" and \"" + s + "\"");
                    std::filesystem::path filename = std::filesystem::absolute(s).lexically_normal();
                    filenames.push_back(filename.string());
                    path_ = filename;
                }
            }
        }


        void print_help(std::ostream & output = std::cout) const
        {
            output << "usage: ikaros [options] [variable=value] [filename]\n";
            output << "\tCommand line options:\n";
            for(const auto & [short_name, option_name] : full)
            {
                output << "\t-"<< short_name << " (" << option_name << "): " << description.at(option_name);
                if(default_values_.count(option_name) && !sensitive_options_.count(option_name))
                    output << " [" << default_values_.at(option_name) << "]";
                output << '\n';
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
                return parse_double(d.at(o));
            else
                return 0;
        }

    private:
        std::map<std::string, std::string> default_values_;
        std::set<std::string> sensitive_options_;

        bool has_later_positional_argument(const std::vector<std::string> & arguments,
                                           std::size_t start) const
        {
            for(std::size_t i = start; i < arguments.size(); ++i)
            {
                const std::string & argument = arguments[i];
                if(argument.find('=') != std::string::npos &&
                   !argument.empty() && argument.front() != '-')
                    continue;

                if(argument.size() > 1 && argument.front() == '-')
                {
                    if(argument.size() == 2)
                    {
                        const std::string short_name = argument.substr(1, 1);
                        auto option = full.find(short_name);
                        if(option != full.end() && requires_value.at(option->second) &&
                           !optional_value.at(option->second) && i + 1 < arguments.size())
                            ++i;
                    }
                    continue;
                }

                return true;
            }
            return false;
        }

        static std::filesystem::path canonical_executable(const std::filesystem::path & candidate)
        {
            std::error_code error;
            std::filesystem::path resolved = std::filesystem::canonical(candidate, error);
            if(error || !std::filesystem::is_regular_file(resolved, error) || error)
                return {};
            return resolved;
        }

        static std::filesystem::path resolve_executable_path(const char * argv0)
        {
            if(argv0 == nullptr || argv0[0] == 0)
                throw std::runtime_error("Could not determine the Ikaros executable path");

            const std::filesystem::path argument(argv0);
            if(argument.has_parent_path())
            {
                const std::filesystem::path resolved = canonical_executable(argument);
                if(!resolved.empty())
                    return resolved;
            }
            else
            {
                const char * path_value = std::getenv("PATH");
                if(path_value != nullptr)
                {
                    const std::string search_path(path_value);
                    std::size_t start = 0;
                    while(start <= search_path.size())
                    {
                        const std::size_t separator = search_path.find(':', start);
                        const std::string directory = search_path.substr(
                            start, separator == std::string::npos ? std::string::npos : separator - start);
                        const std::filesystem::path candidate =
                            (directory.empty() ? std::filesystem::current_path() : std::filesystem::path(directory)) /
                            argument;
                        const std::filesystem::path resolved = canonical_executable(candidate);
                        if(!resolved.empty())
                            return resolved;
                        if(separator == std::string::npos)
                            break;
                        start = separator + 1;
                    }
                }

                const std::filesystem::path resolved = canonical_executable(argument);
                if(!resolved.empty())
                    return resolved;
            }

            throw std::runtime_error("Could not resolve the Ikaros executable path \"" +
                                     std::string(argv0) + "\"");
        }

    };

};
