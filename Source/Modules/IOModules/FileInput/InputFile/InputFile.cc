//
//	InputFile.cc		This file is a part of the IKAROS project
//                      A module for reading from files with data in column form
//
//    Copyright (C) 2001-2012  Christian Balkenius
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
//	Revision History
//
//	2002-01-15	Support for # comments and blank lines added
//	2002-01-17	Support for UNIX, Mac and DOS end of line tokens added
//	2012-11-04	Support for static output added

//#define DEBUG_INPUTFILE

#include "InputFile.h"

static void
skip_comment_lines(FILE * file)
{
    char c = fgetc(file);

    while (c=='#' || c==' ' || c=='\t' || c=='\n' || c=='\r')
    {
        if (c=='#')
        {
            fscanf(file, "%*[^\n\r]");
            fscanf(file, " \r");
            fscanf(file, " \n");
        }
        else if (c==' ' || c=='\t')
        {
            fscanf(file, " \r");
            fscanf(file, " \n");
        }
        else if (c=='\n')
        {
            fscanf(file, " \n");
        }
        else if (c=='\r')
        {
            fscanf(file, " \r");
            fscanf(file, " \n");
        }
        c = fgetc(file);
    }
    ungetc(c, file);
}


InputFile::InputFile(Parameter * p):
        Module(p)
{
    // Scan input file and add output connections

    /* char *  */

    filename = GetValue("filename");
    if (filename == NULL)
    {
        Notify(msg_fatal_error, "No input file parameter supplied.\n");
        return;
    }

    type = GetIntValueFromList("type");
    iteration	= 1;
    iterations  = GetIntValue("iterations");
    extend = GetIntValue("extend", 0);
    cur_extension = 0;
    extending = false;
    print_iteration = GetBoolValue("print_iteration");

    file = fopen(filename, "rb");
    if (file == NULL)
    {
        Notify(msg_fatal_error, "Could not open input file \"%s\" \n", filename);
        return;
    }

    no_of_columns = 0;
    char col_label[64];
    int	col_size;

    // Count number of columns in input file

    skip_comment_lines(file);
    fscanf(file, "%*[^A-Za-z\n\r]");		// Skip until letter or end of line

    while (fscanf(file, "%[^/\n\r]/%d", col_label, &col_size) == 2)
    {
        no_of_columns++;
        fscanf(file, "%*[^A-Za-z\n\r]");		// Skip until letter or end of line
#ifdef DEBUG_INPUTFILE
        printf("  Counting column \"%s\" with width %d in file \"%s\".\n", col_label, col_size, filename);
#endif
    }
    
    
    if(type == 0) // dynamic
    {
        column_name = new char * [no_of_columns];
        column_size = new int [no_of_columns];
        column_data = new float * [no_of_columns];
        static_column_data = NULL;

        for (int i=0; i<no_of_columns; i++)
        {
            column_name[i] = NULL;
            column_data[i] = NULL;
        }

        // Read the names and sizes of each column

        int col = 0;

        fseek(file, 0, SEEK_SET);

        skip_comment_lines(file);
        fscanf(file, "%*[^A-Za-z\n\r]");		// Skip until letter or end of line

        while (fscanf(file, "%[^/\n\r]/%d", col_label, &col_size) == 2)
        {
    #ifdef DEBUG_INPUTFILE
            //	printf("  Finding column \"%s\" with width %d in file \"%s\".\n", col_label, col_size, filename);
    #endif
            column_name[col] = create_string(col_label);		// Allocate memory for the column name
            column_size[col] = col_size;
            AddOutput(column_name[col], col_size);
            col++;
            fscanf(file, "%*[^A-Za-z\n\r]");		// Skip until letter or end of line
        }
    }
    
    
    
    else // static
    {        
        // Allocate memory
        
        column_name = new char * [no_of_columns];
        column_size = new int [no_of_columns];
        column_data = NULL;
        static_column_data = new float ** [no_of_columns];
        
        for (int i=0; i<no_of_columns; i++)
        {
            column_name[i] = NULL;
            static_column_data[i] = NULL;
        }
        
        // Read the names and sizes of each column
        
        int col = 0;
        
        fseek(file, 0, SEEK_SET);
        
        skip_comment_lines(file);
        fscanf(file, "%*[^A-Za-z\n\r]");		// Skip until letter or end of line
        
        while (fscanf(file, "%[^/\n\r]/%d", col_label, &col_size) == 2)
        {
#ifdef DEBUG_INPUTFILE
            //	printf("  Finding column \"%s\" with width %d in file \"%s\".\n", col_label, col_size, filename);
#endif
            column_name[col] = create_string(col_label);		// Allocate memory for the column name
            column_size[col] = col_size;
            col++;
            fscanf(file, "%*[^A-Za-z\n\r]");		// Skip until letter or end of line
        }
        
        // Count number of lines in data file
        
        int line_count = CountLines(file);
        
#ifdef DEBUG_INPUTFILE
        printf("line_count = %d\n", line_count);
#endif
        
        // Allocate outputs
        
        for(int j=0; j<col; j++)
            AddOutput(column_name[j], column_size[j], line_count);

        no_of_rows = line_count;
        
        fclose(file);
    }
}



InputFile::~InputFile()
{
    Notify(msg_verbose, "    Deleting InputFile \"%s\".\n", GetName());
    fclose(file);

    /* THESE WILL BE DELETED BY ~MODULE
    for(int i=0; i<no_of_columns; i++)
    {
    	printf("      Deleting char * \"%s\".\n", column_name[i]);
    	delete [] column_name[i];
    }
    */
    delete [] column_name; // possible memory leak - delete column_name[i] ??? ***
}



Module *
InputFile::Create(Parameter * p)
{
    return new InputFile(p);
}



void
InputFile::Init()
{
    if(type == 1)
    {
        file = fopen(filename, "rb");

        // Get outputs
        
        for(int j=0; j < no_of_columns; j++)
            static_column_data[j] = GetOutputMatrix(column_name[j]);
        
        // Read data
        
        skip_comment_lines(file);
        
        fscanf(file, "%*[^\n\r]");	// Skip format line
        fscanf(file, "\r");
        fscanf(file, "\n");

        for(int j=0; j<no_of_rows; j++)
        {
            skip_comment_lines(file);
            
            for (int col=0; col < no_of_columns; col++)
            {
                for (int i=0; i<column_size[col]; i++)
                    fscanf(file, "%f", &static_column_data[col][j][i]);
            }
            
            fscanf(file, "%*[^\n\r]");	// Skip until end-of-line
            fscanf(file, "\r");			// Read new-line token(s)
            fscanf(file, "\n");			// Read new-line token(s)
        }
        
        fclose(file);
        
        return;
    }
    
    for (int col = 0; col < no_of_columns; col++)
        column_data[col] = GetOutputArray(column_name[col]);
}



void
InputFile::ReadData()
{
    for (int col=0; col < no_of_columns; col++)
    {
        for (int i=0; i<column_size[col]; i++)
            fscanf(file, "%f", &column_data[col][i]);
    }
    fscanf(file, "%*[^\n\r]");	// Skip until end-of-line
    fscanf(file, "\r");			// Read new-line token(s)
    fscanf(file, "\n");			// Read new-line token(s)
}



void
InputFile::CountData()
{
    for (int col=0; col < no_of_columns; col++)
    {
        for (int i=0; i<column_size[col]; i++)
            fscanf(file, "%*f");
    }
    fscanf(file, "%*[^\n\r]");	// Skip until end-of-line
    fscanf(file, "\r");			// Read new-line token(s)
    fscanf(file, "\n");			// Read new-line token(s)
}



int
InputFile::CountLines(FILE * file)
{
    fseek(file, 0, SEEK_SET);
    
    // Skip first line
    
    skip_comment_lines(file);
    fscanf(file, "%*s\n");
    
    int line_count = -1;
    
    while(!feof(file))
    {
        skip_comment_lines(file);
        CountData();
        line_count++;
    }
    
    return line_count;
}



void
InputFile::Tick()
{
    if(type == 1)
        return;

    if (extending)
    {
        if (cur_extension >= extend-1)
            Notify(msg_end_of_file);

        else if (cur_extension == 0)
        {
            for (int col=0; col < no_of_columns; col++)
                for (int i=0; i<column_size[col]; i++)
                    column_data[col][i] = 0.0;
        }

        cur_extension++;
        return;
    }

    skip_comment_lines(file);
    ReadData();

    if (feof(file))
    {
        if (print_iteration)
            Notify(msg_verbose, "End of iteration (%d/%d)\n", iteration, iterations);

        if (iteration++ < iterations)
        {
            rewind(file);
            skip_comment_lines(file);
            fscanf(file, "%*[^\n\r]");	// Skip format line
            fscanf(file, "\r");
            fscanf(file, "\n");
        }

        else if (extend == 0)
            Notify(msg_end_of_file, "Reached in \"%s\".\n", filename);

        else
            extending = true;
    }
}

static InitClass init("InputFile", &InputFile::Create, "Source/Modules/IOModules/FileInput/InputFile/");
