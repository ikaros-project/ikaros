#include "ikaros.h"

#include <iostream>
#include <fstream>

using namespace ikaros;

class OutputFile: public Module
{
    parameter       directory;
    parameter       filename;
    std::string     dirname;                    // Unsed internally
    parameter       format;
    std::string     column_separator;
    std::fstream    file;
    long		    time;						// Used to generate the time column in the output file
    int             index;
    matrix          input;
    std::filesystem::path resolved_filename;
    bool            has_opened_file = false;

    void
    OpenFile(bool append=false)
    {
        file.open(resolved_filename, std::ios::out | (append ? std::ios::app : std::ios::trunc));
        if(!file)
            throw std::runtime_error("File could not be opened: " + resolved_filename.string());

        if(!append)
            WriteHeader();
        has_opened_file = true;
    }

    void 
    Init() override
    {
        Bind(input, "INPUT");
        Bind(filename, "filename");
        Bind(format, "format");

        if(std::string(format) == "csv")
            column_separator = ", ";
        else
            column_separator = "\t";

        if(!kernel().SanitizeWritePath(filename.as_string(), resolved_filename))
            throw std::runtime_error("OutputFile can only write files inside UserData.");

        OpenFile(false);
    }


    void 
    WriteHeader()
    {
        if(input.labels().empty())
            return;
            
        std::string sep;
        for(const std::string & label : input.labels())
        {
            file << sep << label;
            sep = column_separator;
        }
        file << std::endl;
    }


    void 
    WriteRow()
    {
        std::string sep;
        for(float v : input)
        {
            file << sep << v;
            sep = column_separator;
        }
        file << std::endl;
    }

    public:

    ~OutputFile()
    {
        Stop();
    }

    void
    Stop() override
    {
        if(file.is_open())
            file.close();
    }


    void 
    Tick() override
    {
        if(!file.is_open())
            OpenFile(has_opened_file);
        WriteRow();
    }
};

INSTALL_CLASS(OutputFile)
