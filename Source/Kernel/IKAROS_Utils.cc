//
//	IKAROS_Utils.cc		Various utilities for the IKAROS project
//
//    Copyright (C) 2001-2008  Christian Balkenius
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
//	Created: July 17 2001
//
#include "IKAROS_Utils.h"
#include "IKAROS_System.h"

#ifdef WINDOWS
#include <direct.h>
#else
#include <unistd.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>


#if defined USE_VDSP
#include <Accelerate/Accelerate.h>
#endif




char *
create_string(const char * c)
{
    if (c)
    {
        char * p = strcpy(new char [strlen(c)+1], c);
        return p;
    }
    else
        return NULL;
}

char *
create_string_head(const char * c, int n)
{
    if (c)
    {
        int l = int(strlen(c));
		
        if(n > l)
            n = l;
		
        char * p = strncpy(new char [n+1], c, n); // FIXME: replace with create string ***
        p[n] = 0;
        return p;
    }
    else
        return NULL;
}

char *
create_string_tail(const char * c, int i)
{
    if (c)
    {
        int l = int(strlen(c));
		
        if(i > l)
            return NULL;
		
        char * p = strncpy(new char [l-i+1], &c[i], l-i); // FIXME: replace with create string ***
        p[l-i] = 0;
        return p;
    }
    else
        return NULL;
}

char *
create_formatted_string(const char *format, ...)
{
    const int max_string = 1024;
    static char temp[max_string];
    va_list args;
    va_start(args, format);
    vsnprintf(temp, max_string, format, args);
    va_end(args);
    int length = int(strlen(temp))+1;
    char * p = strncpy(new char [length], temp, length); // FIXME: replace with create string ***
    return p;
}

void
destroy_string(char * c)
{
    delete [] c;
}

bool
equal_strings(const char * a, const char * b)
{
    if (a == b)		// Identical string or both NULL
        return true;
    if (a == NULL || b == NULL) // only one is NULL, they can't be equal
        return false;
    return !strcmp(a, b);	// Do the usual comparison
}

bool
strstart(const char * s1, const char *s2) // does s1 start with s2
{
	while(*s2)
		if(*s2++ != *s1++)
			return false;
	return true;
}

bool
strend(const char * s1, const char * s2) // does 1s end with s2
{
	int i1 = int(strlen(s1))-1;
	int i2 = int(strlen(s2))-1;
	do
	{
		if(s1[i1--] != s2[i2--])
			return false;
	}
	while(i1 && i2);
	return true;
}

char *
copy_string(char * dest, const char * src, int len)
{
    int size;
    
    if (!len)
        return dest;
    
    if(!src) // NULL is treated as an empty string
    {
        dest[0] = '\0';
        return dest;
    }
    
    size = int(strlen(src));
    if (size >= len)
        size = len-1;
    memcpy(dest, src, size);
    dest[size] = '\0';
    return dest;
}

char *
append_string(char * dest, const char * a, int len)
{
    if(!len || !a)
        return dest;
    
    int used = int(strlen(dest));
    
    int size = int(strlen(a));
    if(used+size >= len)
        size = len-used-1;
    memcpy(dest+used, a, size);
    dest[used+size] = '\0';
    
    return dest;
}

#ifdef WINDOWS32
/*
 * Get next token from string *stringp, where tokens are possibly-empty
 * strings separated by characters from delim.
 *
 * Writes NULs into the string at *stringp to end tokens.
 * delim need not remain constant from call to call.
 * On return, *stringp points past the last NUL written (if there might
 * be further tokens), or is NULL (if there are definitely no more tokens).
 *
 * If *stringp is NULL, strsep returns NULL.
 */
char* strsep(char **stringp, const char *delim)
{
    char *s;
    const char *spanp;
    int c, sc;
    char *tok;
    
    if ((s = *stringp) == NULL)
        return (NULL);
    for (tok = s;;) {
        c = *s++;
        spanp = delim;
        do {
            if ((sc = *spanp++) == c) {
                if (c == 0)
                    s = NULL;
                else
                    s[-1] = 0;
                *stringp = s;
                return (tok);
            }
        } while (sc != 0);
    }
}

#endif

bool
is_path(const char * p)
{
    if(!p)
        return false;
	
    for(unsigned int i=0; i<strlen(p); i++)
        if(p[i] == '/')
            return true;
        else if (p[i] == '\\')
            return true;
        else if (p[i] == ':')
            return true;
    return false;
}

bool
is_absolute_path(const char * p)
{
    if(!p)
        return false;
    if(p[0] == '/')
        return true;
    if(p[1] == ':')
        return true;
    return false;
}



void
print_array(const char * name, float * a, int size, int decimals)
{
    bool matlab_format = decimals & MATLAB;
    bool dim_format = decimals & DIM;
    decimals = decimals & 255;

    if(matlab_format) // use MATLAB format
    {
        decimals -= 1024;
        if(decimals == 0)
            decimals = 2;
        printf("%s = [", name);
        for (int i=0; i<size; i++)
            printf(" %*.*f", 5+decimals, decimals, a[i]);
        printf(" ];\n");
    }
    else
    {   
        if(dim_format)
            printf("%s[%d] = ", name, size);
        else
            printf("%s = ", name);
        
        for (int i=0; i<size; i++)
            printf("%*.*f", 5+decimals, decimals, a[i]);
        printf("\n");
    }
}



void
print_matrix(const char * name, float ** m, int sizex, int sizey, int decimals)
{
    bool matlab_format = decimals & MATLAB;
    bool dim_format = decimals & DIM;
    decimals = decimals & 255;
    
    if(matlab_format) // use MATLAB format
    {
        decimals -= 1024;
        if(decimals == 0)
            decimals = 2;
        printf("%s = [", name);
        for (int j=0; j<sizey; j++)
        {
            for (int i=0; i < sizex; i++)
                printf(" %*.*f", 6+decimals, decimals, m[j][i]);
            if(j<sizey-1)
                printf(";...\n");
        }
        printf(" ];\n");
    }
    else
    {
        if(dim_format)
            printf("%s[%d][%d] = \n", name, sizey, sizex);
        else
            printf("%s = \n", name);
        for (int j=0; j<sizey; j++)
        {
            for (int i=0; i < sizex; i++)
                printf("%*.*f", 6+decimals, decimals, m[j][i]);
            printf("\n");
        }
        printf("\n");
    }
}



float *
create_array(int size)
{
    return (float *)calloc(size, sizeof(float));
}



float *
create_array(const char * s, int & size)
{
    size = 0;

    if (s == NULL)
        return NULL;
    
    // Count values in s
    
    const char * v = s;
    for (; isspace(*v) && *v != '\0'; v++) ;
    while(sscanf(v, "%*f")!=-1)
    {
        size++;
        for (; !isspace(*v) && *v != ',' && *v != '\0'; v++) ;
        if(*v==',') v++;
        for (; isspace(*v) && *v != '\0'; v++) ;
    }
    
    float * a = create_array(size);
    
    // read values into array

    v = s;
    int i=0;
    for (; isspace(*v) && *v != '\0'; v++) ;
    while(sscanf(v, "%f", &a[i])!=-1)
    {
        i++;
        for (; !isspace(*v) && *v != ',' && *v != '\0'; v++) ;
        if(*v==',') v++;
        for (; isspace(*v) && *v != '\0'; v++) ;
    }
    return a;
}



float **
create_matrix(int sizex, int sizey)
{
    if(sizex == 0 || sizey == 0)
    {
        printf("IKAROS: Warning: Attempting to create matrix of size 0");
        return NULL;
    }
    
    float * a = (float *)calloc(sizex*sizey, sizeof(float));
    float ** b = (float **)malloc(sizey*sizeof(float *));
    for (int j=0; j<sizey; j++)
        b[j] = &a[j*sizex];
    return b;
}

float ***
create_matrix(int sizex, int sizey, int sizez)
{
    float ** a = create_matrix(sizex, sizey*sizez);
    float *** b = (float ***)malloc(sizez*sizeof(float **));
    for (int j=0; j<sizez; j++)
        b[j] = &a[j*sizey];
    return b;
}



float ****
create_matrix(int sizex, int sizey, int sizez, int sizet)
{
    float *** a = create_matrix(sizex, sizey, sizez*sizet);
    float **** b = (float ****)malloc(sizez*sizeof(float ***));
    for (int j=0; j<sizet; j++)
        b[j] = &a[j*sizez];
    return b;
}



float **
create_matrix(const char * s, int & sizex, int & sizey)
{
    sizex = 0;
    sizey = 0;

    int sx = 0; // to allow &sizex == &sizey
    int sy = 1;

    if (s == NULL)
        return NULL;
    
    // Count rows and columns in s
    
    int row_elements = 0;

    char * ss = create_string(s);
    char * v = ss;

    unsigned long e = strlen(ss)-1;
    while((isspace(ss[e]) || ss[e] == ';') && e > 0)
        ss[e--] ='\0';
        
    for (; isspace(*v) && *v != '\0'; v++) ;

    while(sscanf(v, "%*f")!=-1)
    {
        row_elements++;
        for (; !isspace(*v) && *v != ',' && *v != ';' && *v != '\0'; v++) ;
        if(*v==',') v++;
        for (; isspace(*v) && *v != '\0'; v++) ;
        if(*v==';')
        {
            v++;
            if(row_elements > sx)
                sx = row_elements;
            row_elements = 0;
            sy++;
        }
        for (; isspace(*v) && *v != '\0'; v++) ;
    }
    if(row_elements > sx)
        sx = row_elements;

    float ** m = create_matrix(sx, sy);

    v = ss;
    int j=0;
    int i=0;
    while(sscanf(v, "%f", &m[j][i])!=-1)
    {
        i++;
        for (; !isspace(*v) && *v != ',' && *v != ';' && *v != '\0'; v++) ;
        if(*v==',') v++;
        for (; isspace(*v) && *v != '\0'; v++) ;
        if(*v==';')
        {
            v++;
            i = 0;
            j++;
        }
        for (; isspace(*v) && *v != '\0'; v++) ;
    }

    destroy_string(ss);

    sizex = sx;
    sizey = sy;

    return m;
}



float *
reset_array(float * a, int size)
{
#ifdef USE_VDSP
    vDSP_vclr(a, 1, size);
#else
    for (int i=0; i<size; i++)
        a[i] = 0.0;
#endif
    return a;
}

float **
reset_matrix(float ** m, int sizex, int sizey)
{
    reset_array(m[0], sizex*sizey);
    return m;
}

float *
set_array(float * a, float v, int size)
{
    for (int i=0; i<size; i++)
        a[i] = v;
    return a;
}

float **
set_matrix(float ** m, float v, int sizex, int sizey)
{
    set_array(m[0], v, sizex*sizey);
    return m;
}

void
destroy_array(float * a)
{
    if (!a) return;
    free(a);
}

void
destroy_matrix(float ** a)
{
    if (!a) return;
    free(*a);
    free(a);
}

void
destroy_matrix(float *** a)
{
    if (!a) return;
    free(**a);
    free(*a);
    free(a);
}

void
destroy_matrix(float **** a)
{
    if (!a) return;
    free(***a);
    free(**a);
    free(*a);
    free(a);
}

float * copy_array(float * r, float * a, int size)
{
    return (float *)memcpy(r, a, size*sizeof(float));
}

float ** copy_matrix(float ** r, float ** a, int sizex, int sizey)
{
    memcpy(r[0], a[0], sizex*sizey*sizeof(float));
    return r;
}



// Options
Options::Options(int argc, char *argv[])
{
    file_path = NULL;
    file_name = NULL;
    char * file_arg = NULL;
    getcwd(working_dir, 1024);
	
    for (int i=0; i<255; i++)
    {
        option[i] = false;
        argument[i] = NULL;
    }
    for (int i=1; i<argc; i++)
        if (argv[i][0] == '-')
        {
            char  o = argv[i][1];
            if (o != 0 && argv[i][2] != 0)
                SetOption(o, create_string(&argv[i][2]));
            else
                SetOption(o);
        }
        else if (file_arg == NULL)
        {
            file_arg = argv[i];
        }
	
    // Compute binary directory (if possible)
	
    binary_dir = NULL;
#ifdef WINDOWS
    if(is_path(argv[0]))
#endif
    {
        char * bin_path;
        if(is_absolute_path(argv[0]))
			bin_path = create_string(argv[0]);
        else
#ifdef WINDOWS
			bin_path = create_formatted_string("%s\\%s", working_dir, argv[0]);
#else
		bin_path = create_formatted_string("%s/%s", working_dir, argv[0]);
#endif
        int i;
        for(i=int(strlen(bin_path))-1; i>0 && bin_path[i] != '/' && bin_path[i] !='\\'; i--)  ; 
        binary_dir = create_string_head(bin_path, i+1);
        destroy_string(bin_path);
    }
    
    // Compute file directory and name
    
    if(file_arg != NULL)
    {
        if(is_absolute_path(file_arg))
            file_path = create_string(file_arg);
        else
            file_path = create_formatted_string("%s/%s", working_dir, file_arg);
		
        int i;
        for(i=int(strlen(file_path))-1; i>0 && file_path[i] != '/' && file_path[i] !='\\'; i--)  ; 
        file_dir = create_string_head(file_path, i+1);
        file_name =  create_string_tail(file_path, i+1);
    }
}

Options::~Options()
{
    for (int i=0; i<255; i++)
        destroy_string(argument[i]);
	
    destroy_string(binary_dir);
    destroy_string(file_path);
    destroy_string(file_dir);
    destroy_string(file_name);
}

void
Options::SetOption(char c, char * arg)
{
    option[int(c)] = true;
    argument[int(c)] = arg;
}

void
Options::ResetOption(char c)
{
    option[int(c)] = false;
}

bool
Options::GetOption(char c)
{
    return option[int(c)];
}

char *
Options::GetArgument(char c)
{
    return argument[int(c)];
}

char *  
Options::GetWorkingDirectory()
{
    return working_dir;
}

char *
Options::GetBinaryDirectory()
{
    return binary_dir;
}

char *
Options::GetFilePath()
{
    return file_path;
}

char *
Options::GetFileDirectory()
{
    return file_dir;
}

char *
Options::GetFileName()
{
    return file_name;
}

void
Options::Print()
{
    printf("Options:\n");
    for (int i=0; i<255; i++)
        if (option[i])
        {
            printf("-%c", i);
            if (argument[i])
                printf(" %s", argument[i]);
            printf("\n");
        }
    if (file_name)
    {
        printf("File Dir: %s\n", file_dir);
        printf("File Name: %s\n", file_name);
    }
}



// Dictionary

Dictionary::KeyValue::KeyValue(const char * k, const char * v, KeyValue * first)
{
    key = create_string(k);
    value = create_string(v);
    next = first;
    prev = NULL;
	
    if(next != NULL)
        next->prev = this;
}

Dictionary::KeyValue::~KeyValue()
{
    destroy_string(key);
    destroy_string(value);
}

Dictionary::Dictionary()
{
    first = NULL;
    last = NULL;
}

Dictionary::~Dictionary()
{
	Clear();
}

void
Dictionary::Clear()
{
    for(KeyValue * kv = first; kv != NULL;)
    {
        KeyValue * n = kv->next;
        delete kv;
        kv = n;
    }
    first = NULL;
    last = NULL;
}

void
Dictionary::Set(const char * k, const char * v)
{
    for(KeyValue * kv = first; kv != NULL; kv = kv->next)
        if(equal_strings(k, kv->key))
        {
            destroy_string(kv->value);
            kv->value = create_string(v);
            return;
        }
	
    first = new KeyValue(k, v, first);
}

const char *
Dictionary::Get(const char * k)
{
    for(KeyValue * kv = first; kv != NULL; kv = kv->next)
        if(equal_strings(k, kv->key))
            return kv->value;
    return NULL;
}

void
Dictionary::Print()
{
    for(KeyValue * kv = first; kv != NULL; kv = kv->next)
		printf("\"%s\" : \"%s\"\n", kv->key, kv->value);
}

const char *
Dictionary::Get(Dictionary::Iterator i)
{
	return i.kv->value;
}

const char *
Dictionary::GetKey(Dictionary::Iterator i)
{
	return i.kv->key;
}
