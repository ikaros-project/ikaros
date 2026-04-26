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

    void 
    Init()
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

        file.open(resolved_filename, std::ios::out);
        if (!file)
            throw std::runtime_error("File could not be opened: " + resolved_filename.string());

        WriteHeader();
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
        file.close();
    }


    void 
    Tick()
    {
        WriteRow();
    }
};

INSTALL_CLASS(OutputFile)
