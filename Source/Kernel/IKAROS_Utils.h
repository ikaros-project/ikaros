//
//	IKAROS_Utils.h		Various utilities for the IKAROS project
//
//    Copyright (C) 2001-2011  Christian Balkenius
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
//    See http://www.ikaros-project.org/ for more information.
//
//	Created: July 17 2001
//

#ifndef IKAROS_UTILS
#define IKAROS_UTILS

#include <stddef.h> // for NULL

// Basic string functions

char *	create_string(const char * c); // copy string c
char *  create_string_head(const char * c, int n); // copy at most n characters
char *  create_string_tail(const char * c, int i); // copy starting at character i
char *	create_formatted_string(const char * format, ...);
void    destroy_string(char * c);

char *  copy_string(char * dest, const char * src, int len); // copy src into dest, len is max size of dest; return dest
char *  append_string(char * dest, const char * a, int len); // append a to dest, len is max size of dest, return dest

bool    equal_strings(const char * a, const char * b);

bool	strstart(const char * s1, const char *s2); // does s1 start with s2
bool	strend(const char * s1, const char * s2); // does 1s end with s2

#ifdef WINDOWS
char* strsep(char **stringp, const char *delim);
#endif

// File path operations

bool    is_path(const char * p);
bool    is_absolute_path(const char * p);

// Array and matrix functions

const int   MATLAB = 1024;
const int   DIM = 2048;

void        print_array(const char * name, float * a, int size, int decimals=2);
void        print_matrix(const char * name, float ** m, int sizex, int sizey, int decimals=2);

float *     create_array(int size);
float *     create_array(const char * s, int & size);

float **    create_matrix(int sizex, int sizey);
float **    create_matrix(const char * s, int & sizex, int & sizey); // create matrix and set size x and y from data
float ***   create_matrix(int sizex, int sizey, int sizez);
float ****  create_matrix(int sizex, int sizey, int sizez, int sizet);


void        destroy_array(float * a);
void        destroy_matrix(float ** m);
void        destroy_matrix(float *** m);
void        destroy_matrix(float **** m);

float *     copy_array(float * r, float * a, int size);
float **    copy_matrix(float ** r, float ** a, int sizex, int sizey);

float *     reset_array(float * a, int size);
float **	reset_matrix(float ** m, int sizex, int sizey);
float ***	reset_matrix(float *** m, int sizex, int sizey, int sizez);

float *     set_array(float * a, float v, int size);
float **    set_matrix(float ** m, float v, int sizex, int sizey);
float ***   set_matrix(float *** m, float v, int sizex, int sizey, int sizez);

float **    set_row(float ** m, float * a, int row, int sizex);
float **    set_col(float ** m, float * a, int col, int sizey);

float *     get_row(float * a, float ** m, int row, int sizex);
float *     get_col(float * a, float ** m, int col, int sizey);

// Delay Line

class DelayLine
{
public:
    int size;
    int pos;
    float * data;

    DelayLine(int s) : size(s), pos(0)
    {
        data = new float[s];
    }	// s > 0; s = 1 is no delay
    ~DelayLine()
    {
        delete [] data;
    }
    void	put(float v)
    {
        data[pos] = v;
        pos = (pos + 1) % size;
    }
    float	get()
    {
        return data[(pos + 1) % size];
    }
};



// Options

class Options
{
public:
    Options(int argc, char *argv[]);
    ~Options();

    void    SetOption(char c, char * arg = 0);
    bool    GetOption(char c);
    char *  GetArgument(char c);
    void    ResetOption(char c);

    const char *  GetValue(const char * a);

    char *  GetWorkingDirectory();
    char *  GetBinaryDirectory();
    
    char *  GetFilePath();
    char *  GetFileDirectory();
    char *  GetFileName();
    
    
    void    Print();
private:
    bool    option[256];
    char *  argument[256];
    
    char *  attribute[32];
    char *  value[32];
    
    char    working_dir[1024];
    char *  binary_dir;
    char *  file_path;
    char *  file_dir;
    char *  file_name;
};


// Dictionary

class Dictionary
	{
	public:
		
		class KeyValue
		{
		public:
			KeyValue * prev;
			KeyValue * next;
			char *  key;
            int     type;
			char *  value;
            int     int_value;
            float   float_value;
			
			KeyValue(const char * k, const char * v, KeyValue * first);
			KeyValue(const char * k, const int v, KeyValue * first);
			KeyValue(const char * k, const float v, KeyValue * first);
			~KeyValue();
		};
		
		class Iterator
		{
		public:
			KeyValue * kv;
			Iterator(KeyValue * e = NULL): kv(e) {}
			Iterator	*  operator++() { if(kv) kv = kv->next; return this; }
		};
		
		KeyValue *  first;
		KeyValue *  last;
		
		Dictionary();
		~Dictionary();
		
		void			Clear();
		void			Set(const char * k, const char * v);
		void			Set(const char * k, const int v);
		void			Set(const char * k, const float v);
        
		const char *		Get(const char * k);
		const int           GetInt(const char * k);
		const float         GetFloat(const char * k);
		
		const char *		Get(Iterator i);
		const int           GetInt(Iterator i);
		const float         GetFloat(Iterator i);
		const char *		GetKey(Iterator i);
		
		void			Print();
		
		static Iterator      First(Dictionary * d) { return (d ? Iterator(d->first): NULL); }
	};


#endif

