//
//	OutputFile.h		This file is a part of the IKAROS project
//                      A module for writing to files in column form
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
//

#ifndef OUTPUTFILE
#define OUTPUTFILE

#include "IKAROS.h"



class OutputFile: public Module
{
public:
    OutputFile(Parameter * p);		// Parameter list should contain 'column' with 'inputs'
    virtual ~OutputFile();

    static Module * Create(Parameter * p);

    void	Init();
    void    Tick();							// Write the data at the inputs to the file
private:
    char *      dirname;                    // Used internally
    char *      directory;
    FILE	*	file;
    int         format;
    long		time;						// Used to generate the time column in the output file
    int         index;
    
    int         no_of_columns;
    int         total_no_of_columns;
    int         no_of_decimals;				// No of decimals in output file
    bool        timestamp;
    
    char **     column_name;
    int	*       column_decimals;
    int *       column_size;
    float **	column_data;

    float *     newfile;
    float *     write;
    
    bool        last_trig;
    bool        single_trig;

    void	WriteHeader();						// Iterates over the input_list and writes the single line header to the file
    void	WriteData();						// Iterates over the input_list and writes data to the file
    void    WriteBinaryData();                  // Iterates over the input_list and writes data to the file in binary format
};

#endif

