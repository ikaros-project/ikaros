//
//	InputFile.h			This file is a part of the IKAROS project
//							A module for reading from files with data in column form
//
//    Copyright (C) 2001-2002  Christian Balkenius
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

#ifndef INPUTFILE
#define INPUTFILE

#include "IKAROS.h"

//
// InputFile is used to read data from file
//
// First line in file specifies the outputs of the module and their size
// Ex: T/1 CS/2 US/1 specifies three output names "T", "CS" and "US" of size 1, 2, and 1 respectively
//

class InputFile: public Module
{
public:
    const char *		filename;
    long				iterations;
    long				iteration;
    int				extend;		// extra ticks with zero input before the module notifies the kernel of EOF
    int				cur_extension;
    bool				extending;
    bool				print_iteration;
    int				no_of_columns;

    char **			column_name;
    int *				column_size;
    float **			column_data;

    InputFile(Parameter * p);		// Create and InputFile module (parameters should contain 'file_name' for reading)
    virtual ~InputFile();

    static Module * Create(Parameter * p);

    void Init();
    void Tick();							// Reads the next lie of the file and stores the data at the outputs; notifies the kernel if at end-of-file
private:
    FILE	*	file;

    void	ReadData();						// Reads data from the file
};

#endif
