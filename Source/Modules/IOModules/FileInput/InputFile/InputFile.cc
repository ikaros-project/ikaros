
#include "ikaros.h"

#include <iostream>
#include <fstream>
#include <string>

using namespace ikaros;

class InputFile: public Module
{
public:
    parameter filename;
    //parameter iterations;
   // parameter repetitions;
    //parameter extend;
    parameter send_end_of_file;
    int column_count = 0;
    int current_row = 0;
    int current_repetition = 0;

    std::vector<std::string> column_name;
    std::vector<int> column_size;
    bool has_header = false;
    matrix data;
    std::vector<matrix> output;

    InputFile() : Module()
    {
        std::ifstream file(info_["filename"]);  // FIXME: Does not inherit value or process @; Enhance GetValue to make this work
        std::string line, word;

        if(!file.is_open())
            throw exception("Could not open file: "+filename.as_string());
    
        if(std::getline(file, line))
        {
            std::istringstream iss(line);
            while (iss >> word)
            {
                size_t pos = word.find('/');
                if(pos != std::string::npos) 
                {
                    std::string name = word.substr(0, pos);
                    std::string size = word.substr(pos + 1);
                    int width = std::stoi(size);
                    AddOutput(name, width);
                    column_count += width;
                    column_name.push_back(name);
                    column_size.push_back(std::stoi(size));
                    output.push_back(matrix());
                }
            }
            data.realloc(100, column_count);
            data.resize(0, column_count);
        }
    }



    void 
    Init()
    {
        Bind(filename, "filename");
    //    Bind(iterations, "iterations");
    //    Bind(repetitions, "repetitions");
    //    Bind(extend, "extend");
        Bind(send_end_of_file, "send_end_of_file");


        int c = 0;
        for(std::string name : column_name)
            Bind(output[c++], name);

        std::ifstream file(filename);
        std::string line, word;
        int line_number = 1;

        if(!file.is_open())
            throw fatal_error("Could not open file: "+filename.as_string());

        std::getline(file, line); // Skip header line
        try 
        {
            while(std::getline(file, line))
            {
                matrix row(column_count);
                row.resize(0);

                line_number++;
                line = replace_characters(remove_comment(line));

                if(std::all_of(line.begin(), line.end(), [](unsigned char c) { return std::isspace(c);})) // Only whitespace, skip line
                    continue;

                std::istringstream iss(line);
                while (iss >> word)
                    if(row.size_x() < column_count)
                        row.push(std::stof(word));
                    else
                        throw fatal_error("Row has too many columns");

                if(row.size_x() < column_count)
                    throw fatal_error("Row has too few columns");
                else
                    data.push(row);
            }
            data.print();
        }
        catch(std::exception & e)
        {
            throw fatal_error("Could not read file "+filename.as_string()+". Error on line: "+std::to_string(line_number)+". "+e.what());
        }
    }



    void
    CopyToOutput(int row)
    {
        int i=0;
        for(matrix & m : output)
            for(int j=0; j<column_size[i]; j++)
                m(j) = data(row,i++);
            i++;
    }



    void
    ResetOutput()
    {
        for(matrix & m : output)
            m.reset();
    }



    void
    Tick()
    {
        if(current_row < data.size_y())
        {
            CopyToOutput(current_row);
        }
        else if(current_row == data.size_y())
        {
            ResetOutput();
            if(send_end_of_file)
            {
                send_end_of_file = false; // Only send once
                Notify(msg_end_of_file, filename.as_string());

            }
        }
        current_row++;
    }
};



INSTALL_CLASS(InputFile)

