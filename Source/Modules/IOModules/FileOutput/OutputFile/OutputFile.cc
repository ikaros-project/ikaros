//
//	OutputFile.cc		This file is a part of the IKAROS project
//						A module for writing to files in column form
//
//    Copyright (C) 2001-2016  Christian Balkenius
//
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program; if not, write to the Free Software
//    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//

#include "OutputFile.h"

#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>

using namespace ikaros;

OutputFile::OutputFile(Parameter * p):
        Module(p)
{
    file = NULL;
    time = 0; 
    no_of_columns = 0;
    no_of_decimals = GetIntValue("decimals");

    // Count columns *** parnent_group->appended_ekements

    XMLNode * par = xml->parent;
    for (XMLElement * c = ((XMLElement *)(par))->GetContentElement("column"); c != NULL; c = c->GetNextElement("column"))
        no_of_columns++;

    // Allocate memory

    column_name = new char * [no_of_columns];
    column_size = new int [no_of_columns];
    column_size_x = new int [no_of_columns];
    column_size_y = new int [no_of_columns];
    column_decimals = new int [no_of_columns];
    column_data = new float * [no_of_columns];

    for (int i=0; i<no_of_columns; i++)
    {
        column_name[i] = NULL;
        column_data[i] = NULL;
        column_size[i] = 0;
        column_size_x[i] = 0;
        column_size_y[i] = 0;
        column_decimals[i] = 0;
    }

    // Add input for each column in the parameter list

    int col = 0;
    for (XMLElement * c = ((XMLElement *)(par))->GetContentElement("column"); c != NULL; c = c->GetNextElement("column"))
    {
//	printf("Column: %s\n", c->GetAttribute("name"));

        const char * col_name = c->GetAttribute("name");
        if (col_name == NULL)
        {
            Notify(msg_warning, "Column name missing in module \"%s\". Ignored.\n", GetName());
            break;
        }
        column_name[col] = create_string(col_name);		// Allocate memory for the column name
        AddInput(col_name);

        column_decimals[col] = string_to_int(c->GetAttribute("decimals"), no_of_decimals);

        col++;
    }

    AddInput("WRITE", true);
    AddInput("NEWFILE", true);
}



void
OutputFile::Init()
{
    Bind(single_trig, "single_trig");
    Bind(use_old_format, "use_old_format");
    
    last_trig = false;
    total_no_of_columns = 0;

    for (int i=0; i<no_of_columns; i++)
    {
        column_data[i] = GetInputArray(column_name[i]);
        column_size[i] = GetInputSize(column_name[i]);
        column_size_x[i] = GetInputSizeX(column_name[i]);
        column_size_y[i] = GetInputSizeY(column_name[i]);
        total_no_of_columns += column_size[i];
    }
    
    newfile = GetInputArray("NEWFILE");
    write = GetInputArray("WRITE");
    index = 0;
    dirname = NULL;
    timestamp = GetBoolValue("timestamp");
    
    const char * f =GetValue("format");
    if(f && f[0] =='b')
        format = 1;

    const char * n = GetValue("directory");

    if(n)
    {
        int ix = 0;
        dirname = create_formatted_string("%s.%03d", n, ix);
        DIR * dir = opendir(dirname);
        while(dir)
        {
            closedir(dir);
            ix++;
            dirname = create_formatted_string("%s.%03d", n, ix);
            dir =opendir(dirname);
        }
        mkdir(dirname, S_IRUSR | S_IWUSR | S_IXUSR);
    }
    else
    {
        dirname = create_string("");
    }
}



OutputFile::~OutputFile()
{
    if (file != NULL)
        fclose(file);

    for (int i=0; i<no_of_columns; i++)
    {
#ifdef DEBUG
        Notify(msg_debug, "      Deleting char * \"%s\".\n", column_name[i]);
#endif
        delete [] column_name[i];
    }
    delete [] column_name;
}



Module *
OutputFile::Create(Parameter * p)
{
    return new OutputFile(p);
}



void
OutputFile::Tick()
{
    if(newfile && *newfile > 0)
    {
        time = 0;
    }
    
    if(time == 0)
    {
        if(file != NULL)
            fclose(file);
           
        const char * file_name = GetValue("filename");
        if (file_name == NULL)
        {
            Notify(msg_fatal_error, "No output file parameter supplied\n");
            return;
        }

        char * fname = NULL;
        if(dirname && dirname[0] != '\0')
            fname = create_formatted_string("%s/%s", dirname, create_formatted_string(file_name, index++));
        else
            fname = create_formatted_string(file_name, index++);

        file = fopen(fname, "wb");
        if (file == NULL)
        {
            Notify(msg_fatal_error, "Could not open output file \"%s\" \n", file_name);
            return;
        }

        WriteHeader();
    }
    
    if(!write || (single_trig && !last_trig && *write > 0) || (!single_trig && *write > 0))
    {
        last_trig = true;
        if(format==0)
            WriteData();
        else
            WriteBinaryData();
    }

    if(write && *write == 0)
        last_trig = false;

    time++;
}



void
OutputFile::WriteHeader()
{
    if(use_old_format) // OLD FORMAT
    {
        if(timestamp)
            fprintf(file, "T/1\t");
        for (int i=0; i<no_of_columns; i++)
            if (column_size[i] != 0)
            {
                fprintf(file, "%s/%d", column_name[i], column_size[i]);
                for (int j=0; j<column_size[i]; j++)
                    fprintf(file, "\t");
            }
        fprintf(file, "\n");
    }
    else // NEW FORMAT
    {
        if(timestamp)
            fprintf(file, "T/1\t");
        for (int i=0; i<no_of_columns; i++)
            if (column_size[i] != 0)
            {
                if(column_size_y[i] == 1)
                {
                    for(int x=0; x<column_size_x[i]; x++)
                        fprintf(file, "%s:%d\t", column_name[i], x);
                }
                else
                {
                    for(int y=0; y<column_size_y[i]; y++)
                        for(int x=0; x<column_size_x[i]; x++)
                            fprintf(file, "%s:%d:%d\t", column_name[i], x, y);
                }
            }
        fprintf(file, "\n");
    }
}



void
OutputFile::WriteData()
{
    if(timestamp)
        fprintf(file, "%ld\t", time);
    for (int col=0; col<no_of_columns; col++)
        for (int i=0; i<column_size[col]; i++)
            fprintf(file, "%.*f\t", column_decimals[col], column_data[col][i]);
    fprintf(file, "\n");
}



void
OutputFile::WriteBinaryData()
{
    float data[total_no_of_columns];
    int j=0;
    for (int col=0; col<no_of_columns; col++)
        for (int i=0; i<column_size[col]; i++)
            data[j++] = column_data[col][i];
    fwrite(data, sizeof(data), 1, file);
    fwrite("\n", 1, 1, file); // write new line
}


static InitClass init("OutputFile", &OutputFile::Create, "Source/Modules/IOModules/FileOutput/OutputFile/");
