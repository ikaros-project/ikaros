//
//	OutputFile.cc		This file is a part of the IKAROS project
//						A module for writing to files in column form
//
//    Copyright (C) 2001-2007  Christian Balkenius
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

using namespace ikaros;

OutputFile::OutputFile(Parameter * p):
        Module(p)
{
    const char * file_name = GetValue("filename");
    if (file_name == NULL)
    {
        Notify(msg_fatal_error, "No output file parameter supplied\n");
        return;
    }

    file = fopen(file_name, "wb");
    if (file == NULL)
    {
        Notify(msg_fatal_error, "Could not open output file \"%s\" \n", file_name);
        return;
    }

    time = 0;
    no_of_columns = 0;

    no_of_decimals = GetIntValue("decimals", 4);

    // Count columns *** parnent_group->appended_ekements

    XMLNode * par = xml->parent;
    for (XMLElement * c = ((XMLElement *)(par))->GetContentElement("column"); c != NULL; c = c->GetNextElement("column"))
        no_of_columns++;

    // Allocate memory

    column_name = new char * [no_of_columns];
    column_size = new int [no_of_columns];
    column_decimals = new int [no_of_columns];
    column_data = new float * [no_of_columns];

    for (int i=0; i<no_of_columns; i++)
    {
        column_name[i] = NULL;
        column_data[i] = NULL;
        column_size[i] = 0;
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
}



void
OutputFile::Init()
{
    for (int i=0; i<no_of_columns; i++)
    {
        column_data[i] = GetInputArray(column_name[i]);
        column_size[i] = GetInputSize(column_name[i]);
    }
}



OutputFile::~OutputFile()
{
    if (file != NULL)
        fclose(file);

    for (int i=0; i<no_of_columns; i++)
    {
#ifdef DEBUG
        Notify(msg_verbose, "      Deleting char * \"%s\".\n", column_name[i]);
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
    if (time == 0)
    {
        fprintf(file, "T/1\t");
        WriteHeader();
        fprintf(file, "\n");
    }

    fprintf(file, "%ld\t", time++);
    WriteData();
    fprintf(file, "\n");
}



void
OutputFile::WriteHeader()
{
    for (int i=0; i<no_of_columns; i++)
        if (column_size[i] != 0)
        {
            fprintf(file, "%s/%d", column_name[i], column_size[i]);
            for (int j=0; j<column_size[i]; j++)
                fprintf(file, "\t");
        }
}



void
OutputFile::WriteData()
{
    for (int col=0; col<no_of_columns; col++)
        for (int i=0; i<column_size[col]; i++)
            fprintf(file, "%.*f\t", column_decimals[col], column_data[col][i]);
}


