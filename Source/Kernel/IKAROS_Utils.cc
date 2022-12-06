//
//	IKAROS_Utils.cc		Various utilities for the IKAROS project
//
//    Copyright (C) 2001-2022  Christian Balkenius
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
#include "IKAROS_Utils.h"
#include "IKAROS_System.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <cstring>
#include <algorithm>
#include <stdexcept>

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

//
// Taken from http://www.cplusplus.com/articles/2wA0RXSz/
//
const std::vector<std::string> split_string(const std::string &s, const char &c)
{
    std::string buff{""};
    std::vector<std::string> v;

    for (auto n : s)
    {
        if (n != c)
            buff += n;
        else if (n == c && buff != "")
        {
            v.push_back(buff);
            buff = "";
        }
    }
    if (buff != "")
        v.push_back(buff);

    return v;
}



const std::vector<std::string>
split(const std::string & s, const std::string & sep, int maxsplit)
{
    std::vector<std::string> r;
    std::string::size_type i=0, j=0;
    std::string::size_type len = s.size();
    std::string::size_type n = sep.size();

    if (n == 0)
    {
        while(i<len)
        {
            while (i < len && ::isspace(s[i]))
                i++;
            j = i;
            while (i < len && !::isspace(s[i]))
                i++;

            if(j < i)
            {
                if(maxsplit != -1 && maxsplit-- <= 0)
                    break;
                r.push_back(s.substr(j, i-j));
                while(i < len && ::isspace(s[i]))
                    i++;
                j = i;
            }
        }
        if (j < len)
            r.push_back(s.substr(j, len - j));

        return r;
    }

    i = j = 0;
    while (i+n <= len)
    {
        if (s[i] == sep[0] && s.substr(i, n) == sep)
        {
            if(maxsplit != -1 && maxsplit-- <= 0)
                break;

            r.push_back(s.substr(j, i - j));
            i = j = i + n;
        }
        else
            i++;
    }

    r.push_back(s.substr(j, len-j));
    return r;
}



const std::vector<std::string>
rsplit(const std::string & str, const std::string & sep, int maxsplit)
{
    if (maxsplit < 0)   // FIXME: Is this necessary?? WHY???
        return split(str, sep, maxsplit);

    std::vector<std::string> r;
    std::string::size_type i=str.size();
    std::string::size_type j=str.size();
    std::string::size_type n=sep.size();

    if(n == 0)
    {
        while(i > 0)
        {
            while(i > 0 && ::isspace(str[i-1]))
                i--;
            j = i;
            while(i > 0 && !::isspace(str[i-1]))
                i--;

            if (j > i)
            {
                if(maxsplit != -1 && maxsplit-- <= 0)
                    break;
                r.push_back(str.substr(i, j-i));
                while(i > 0 && ::isspace(str[i-1]))
                    i--;
                j = i;
            }
        }
        if (j > 0)
            r.push_back( str.substr(0, j));
    }
    else
    {
        while(i >= n)
        {
            if(str[i-1] == sep[n-1] && str.substr(i-n, n) == sep)
            {
                if(maxsplit != -1 && maxsplit-- <= 0)
                    break;
                r.push_back(str.substr(i, j-i));
                i = j = i-n;
            }
            else
                i--;
        }
        r.push_back(str.substr(0, j));
    }
    
    std::reverse(r.begin(), r.end());
    return r;
}


std::string
join(const std::string & separator, const std::vector<std::string> & v, bool reverse)
{
    std::string s;
    std::string sep;
    if(reverse)
        for (auto & e : v)
        {
            s = e + sep + s;
            sep = separator;
        }    
    else
        for (auto & e : v)
        {
            s += sep + e;
            sep = separator;
        }
    return s;
}



bool
starts_with(const std::string & s, const std::string & start) // waiting for C++20
{
    return start.length() <= s.length() && equal(start.begin(), start.end(), s.begin()); // more efficient than find
}

bool ends_with(const std::string & s, const std::string & end) // waiting for C++20
{
    return  s.size() >= end.size() && s.compare(s.size() - end.size(), end.size(), end) == 0;
}

std::string
head(std::string & s, const std::string & delimiter)
{
    int end = s.find(delimiter);
    if(end == -1)
    {
        std::string h = s;
        s = "";
        return h;
    }
    else
    {
        std::string h = s.substr(0, end);
        s.erase(0, end+delimiter.length());
        return h;
    }
}

std::string 
cut(std::string & s, const std::string & delimiter)
{
    int end = s.find(delimiter);
    if(end == -1)
        return "";

    std::string tail = s.substr(end+delimiter.length(), s.length());
    s.erase(end, s.length());

    return tail;
}



std::string 
rcut(std::string & s, const std::string & delimiter)
{
    int end = s.rfind(delimiter);
    if(end == -1)
        return "";

    std::string tail = s.substr(end+delimiter.length(), s.length());
    s.erase(end, s.length());

    return tail;
}


// trim from start (in place)
std::string ltrim(const std::string &ss)
{
    std::string s = ss;
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
                return !std::isspace(ch);
            }));
    return s;
}


// trim from end (in place)
std::string rtrim(const std::string &ss)
{
    std::string s = ss;
    s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
                return !std::isspace(ch);
            }).base(),
            s.end());
    return s;
}

// trim from both ends (in place)
std::string trim(const std::string &ss)
{
    std::string s = ss;
    s = ltrim(s);
    s = rtrim(s);
    return s;
}




bool
replace(std::string& str, const std::string& from, const std::string& to)
{
    size_t start_pos = str.find(from);
    if(start_pos == std::string::npos)
        return false;
    str.replace(start_pos, from.length(), to);
    return true;
}


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
print_array(const char * name, int * a, int size)
{
    printf("%s = ", name);
    for (int i=0; i<size; i++)
        printf("%i ", a[i]);
    printf("\n");
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
create_array(const char * s, int & size, bool fixed_size)
{
    int sz = 0;

    if (s == NULL)
    {
        if(fixed_size)
            return create_array(size);
        else
            return NULL;
    }
    
    // Count values in s
    
    const char * v = s;
    for (; isspace(*v) && *v != '\0'; v++) ;
    while(sscanf(v, "%*f")!=-1)
    {
        sz++;
        for (; !isspace(*v) && *v != ',' && *v != '\0'; v++) ;
        if(*v==',') v++;
        for (; isspace(*v) && *v != '\0'; v++) ;
    }
    
    if(sz == 0)
        return NULL;

    if(!fixed_size)
        size = sz;
    
    float * a = create_array(size);
    
    // read values into array

    v = s;
    int i=0;
    float scanned_value;
    for (; isspace(*v) && *v != '\0'; v++) ;
    while(sscanf(v, "%f", &scanned_value)!=-1)
    {
        if(i<size)
            a[i] = scanned_value;
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
        printf("IKAROS: Warning: Attempting to create matrix of size 0\n");
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
    float **** b = (float ****)malloc(sizet*sizeof(float ***));
    for (int j=0; j<sizet; j++)
        b[j] = &a[j*sizez];
    return b;
}



float **
create_matrix(const char * s, int & sizex, int & sizey, bool fixed_size)
{
    int sx = 0; // to allow &sizex == &sizey
    int sy = 1;

    if (s == NULL)
    {
        if(fixed_size)
            return create_matrix(sizex, sizey);
        else
            return NULL;
    }
    
    // Skip leading whitespace in s
    
    while(*s && isspace(*s))
        s++;
    
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

    if(!fixed_size)
    {
        sizex = sx;
        sizey = sy;
    }
    float ** m = create_matrix(sizex, sizey);
    
    v = ss;
    int j=0;
    int i=0;
    float scanned_value;
    while(sscanf(v, "%f", &scanned_value)!=-1)
    {
        if(j*sizex+i<sizex*sizey) // this condition makes sure that the matrix can also be filled (but not overfilled) from array 
            m[j][i] = scanned_value;
        
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

    return m;
}



float *
resize_array(float * a, int size, bool clear=false)
{
    return NULL;
}



float **
resize_matrix(int sizex, int sizey, bool clear=false)
{
    return NULL;
}



float ***
resize_matrix(int sizex, int sizey, int sizez, bool clear=false)
{
    return NULL;
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

float ***
reset_matrix(float *** m, int sizex, int sizey, int sizez)
{
    reset_array(m[0][0], sizex*sizey*sizez);
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

float ***
set_matrix(float *** m, float v, int sizex, int sizey, int sizez)
{
    set_array(m[0][0], v, sizex*sizey*sizez);
    return m;
}

// row/col operations

float **
reset_row(float ** m, int row, int sizex, int sizey)
{
    reset_array(m[row], sizex);
    return m;
}

float **
reset_col(float ** m, int col, int sizex, int sizey)
{
    for(int i=0; i<sizey; i++)
        m[i][col] = 0;
    return m;
}

float **
set_row(float ** m, float * a, int row, int sizex)
{
    copy_array(m[row], a, sizex);
    return m;
}



float **
set_col(float ** m, float * a, int col, int sizey)
{
    for(int j=0; j<sizey; j++)
        m[j][col] = a[j];
    return m;
}



float *
get_row(float * a, float ** m, int row, int sizex)
{
    return copy_array(a, m[row], sizex);
}



float *
get_col(float * a, float ** m, int col, int sizey)
{
    for(int j=0; j<sizey; j++)
        a[j] = m[j][col];
    return a;
}


float *
set_one(float * a, int index, int size)
{
    reset_array(a, size);
    if(0 <= index && index < size)
        a[index] = 1;
    return a;
}

float **
set_one_row(float ** m, int x, int y, int sizex, int sizey)
{
    reset_row(m, y, sizex, sizey);
    m[y][x] = 1;
    return m;
}

float **
set_one_col(float ** m, int x, int y, int sizex, int sizey)
{
    reset_col(m, x, sizex, sizey)[y][x] = 1;
    m[y][x] = 1;
    return m;
}


//
// put source into target with given indeces
//
float *
put(float *target, const int *indices, const float *source, const int len)
{
    for (int i=0; i<len; ++i)
        target[indices[i]] = source[i];
   return target;
}

//
// repeat values 1d
//

float *
repeat(float *target, const float *src, const int repeats, const int srclen)
{
    float *start = target;
    for(int i=0; i<srclen; ++i)
    {
        std::fill_n(start, repeats, src[i]);
        start += repeats;
    }
    return target;
}


//
// take source into target using array of indeces
//
float *
take(float *target, const int *indices, const float *source, const int len)
{
    for (int i=0; i< len; ++i)
        target[i] = source[indices[i]];
    return target;
}


//
// tile an array 1d
//


float *
tile(float *target, const float *src, const int tiles, const int srclen)
{
    int datasz = 4; // float is 4 bytes
    float* start = target;
    for (int i=0; i< tiles; ++i)
    {
        std::memcpy(start, src, srclen*datasz);
        start += srclen;
    }
   return target;
}

//
// stack two matrices horizontally
//
void hstack(float **target,
            float **a,
            int ax,
            float **b,
            int bx,
            int rety)
{
    for (int j = 0, rj = 0, aj = 0, bj = 0;
         j < rety;
         j++, rj += ax + bx, aj += ax, bj += bx)
    {
        copy_array(target[0] + rj, a[0] + aj, ax);
        copy_array(target[0] + rj + ax, b[0] + bj, bx);
    }
}

bool
store_array(const char * path, const char * name, float * a, int size)
{
    char * s = create_formatted_string("%s.%s", path, name);
    FILE * f = fopen(s, "w");
    if(!f)
    {
        destroy_string(s);
        return false;
    }
    
    fprintf(f, "T %d 0\n", size);   // text-format, size_x, size_y
    for(int i=0; i<size; i++)
        fprintf(f, "%.10f\t", a[i]);
    fclose(f);
    destroy_string(s);
    return true;
}



bool
store_matrix(const char * path, const char * name, float ** m, int size_x, int size_y)
{
    char * s = create_formatted_string("%s.%s", path, name);
    FILE * f = fopen(s, "w");
    if(!f)
    {
        destroy_string(s);
        return false;
    }
    
    fprintf(f, "T %d %d\n", size_x, size_y);   // text-format, size_x, size_y
    for(int j=0; j<size_y; j++)
    {
        for(int i=0; i<size_x; i++)
            fprintf(f, "%.10f\t", m[j][i]);
        fprintf(f, "\n");
    }
    fclose(f);
    destroy_string(s);
    return true;
}



bool
load_array(const char * path, const char * name, float * a, int size)
{
    char * s = create_formatted_string("%s.%s", path, name);
    FILE * f = fopen(s, "r");
    if(!f)
     {
        destroy_string(s);
        return false;
    }
 
    int sx, sy;
    fscanf(f, "T %d %d\n", &sx, &sy);
 
    if(sx != size)
    {
        destroy_string(s);
        return false;
    }
    
    for(int i=0; i<size; i++)
        fscanf(f, "%f ", &a[i]);

    return true;
}



bool
load_matrix(const char * path, const char * name, float ** m, int size_x, int size_y)
{
    char * s = create_formatted_string("%s.%s", path, name);
    FILE * f = fopen(s, "r");
    if(!f)
     {
        destroy_string(s);
        return false;
    }
 
    int sx, sy;
    fscanf(f, "T %d %d\n", &sx, &sy);
 
    if(sx != size_x || sy != size_y)
    {
        destroy_string(s);
        return false;
    }
    
    for(int j=0; j<size_y; j++)
    {
        for(int i=0; i<size_x; i++)
            fscanf(f, "%f ", &m[j][i]);
        fprintf(f, "\n");
    }

    return true;
}

// legacy
void 
Serialize2d(FILE *afile, float **adata, int si, int sj)
{
    for (int i=0; i<si; i++)
        for (int j=0; j<sj; j++)
            fprintf(afile, "%.*f\t", 4, adata[i][j]);
}

void 
Serialize4d(FILE *afile, float ****adata, int si, int sj, int sk, int sl)
{
    for (int i=0; i<si; i++)
        for (int j=0; j<sj; j++)
            Serialize2d(afile, adata[i][j], sk, sl);
}

void 
Deserialize2d(FILE *afile, float **adata, int si, int sj)
{
    for (int i=0; i<si; i++)
        for (int j=0; j<sj; j++)
            fscanf(afile, "%f", &adata[i][j]);
}

void 
Deserialize4d(FILE *afile, float ****adata, int si, int sj, int sk, int sl)
{
    for (int i=0; i<si; i++)
        for (int j=0; j<sj; j++)
            Deserialize2d(afile, adata[i][j], sk, sl);
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

float *
copy_array(float * r, float * a, int size)
{
    return (float *)memcpy(r, a, size*sizeof(float));
}

float **
copy_matrix(float ** r, float ** a, int sizex, int sizey)
{
    memcpy(r[0], a[0], sizex*sizey*sizeof(float));
    return r;
}

float ***
copy_matrix(float *** r, float *** a, int sizex, int sizey, int sizez)
{
    memcpy(r[0][0], a[0][0], sizex*sizey*sizez*sizeof(float));
    return r;
}


char *
base64_encode(const unsigned char * data,
              size_t size_in,
              size_t *size_out)
{
    static char encoding_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    static int mod_table[] = {0, 2, 1};
    *size_out = ((size_in - 1) / 3) * 4 + 4;
    
    char *encoded_data = (char *)malloc(*size_out);
    if (encoded_data == NULL) return NULL;
    
    for (int i = 0, j = 0; i < size_in;)
    {
        unsigned int octet_a = i < size_in ? data[i++] : 0;
        unsigned int octet_b = i < size_in ? data[i++] : 0;
        unsigned int octet_c = i < size_in ? data[i++] : 0;
        
        unsigned int triple = (octet_a << 0x10) + (octet_b << 0x08) + octet_c;
        
        encoded_data[j++] = encoding_table[(triple >> 3 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 2 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 1 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 0 * 6) & 0x3F];
    }
    
    for (int i = 0; i < mod_table[size_in % 3]; i++)
        encoded_data[*size_out - 1 - i] = '=';
    
    return encoded_data;
}

// mark - TAT additions
// void        set_submatrix(float *A, int ncols, float *S, int mrows, int mcols, int row, int col);
    //  Taken from: http://www.mymathlib.com/
    //  Arguments:                                                                //
    //     double *A    (Destination) Pointer to the first element of the matrix A[n][n].       //
    //     int    ncols The number of columns of the matrix A.                    //
    //     double *S    (Source) Source address of the submatrix.                     //
    //     int    mrows The number of rows matrix S.                              //
    //     int    mcols The number of columns of the matrix S.                    //
    //     int    row   (Offset) The row of A corresponding to the first row of S.         //
    //     int    col   (Offset) The column of A corresponding to the first column of S.   //
    //                                                                            //
void set_submatrix(float *A, int ncols, float *S, int mrows, int mcols,
                    int row, int col)
{
    int i,j;
    
    for ( A += row * ncols + col, i = 0; i < mrows; A += ncols, i++)
        for (j = 0; j < mcols; j++) *(A + j) = *S++;
}

//  Taken from: http://www.mymathlib.com/
//  Description:                                                              //
//     Copy the mrows and mcols of the nrows x ncols matrix A starting with   //
//     A[row][col] to the submatrix S.                                        //
//     Note that S should be declared double S[mrows][mcols] in the calling   //
//     routine.                                                               //
//                                                                            //
//  Arguments:                                                                //
//     double *S    (Target) Destination address of the submatrix.                     //
//     int    mrows The number of rows of the matrix S.                       //
//     int    mcols The number of columns of the matrix S.                    //
//     double *A    Pointer to the first element of the matrix A[nrows][ncols]//
//     int    ncols The number of columns of the matrix A.                    //
//     int    row   The row of A corresponding to the first row of S.         //
//     int    col   The column of A corresponding to the first column of S.   //
void get_submatrix(float *S, int mrows, int mcols,
                    float *A, int ncols, int row, int col)
{
    int number_of_bytes = sizeof(float) * mcols;
    
    for (A += row * ncols + col; mrows > 0; A += ncols, S+= mcols, mrows--)
        memcpy(S, A, number_of_bytes);
}

// void 		repeat(float *r, float *a, int repeats);
//
// Repeats each value in source array a given number of times
// r - result containing repeated array, must be size len*repeats
// a - source array
// len - length of source
// repeats - number of repeats
void repeat(float *r, float *a, int len, int repeats)
{ 
float *start = r;
    for(int i=0; i<len; ++i)
    {
        std::fill_n(start, repeats, a[i]);
        start += repeats;
    }
}

// void 		tile(float *r, float *a, int tiles);
//
// Tiles source array a given number of times
// r - result containing repeated array, must be size len*tiles
// a - source array
// len - length of source
// tiles - number of tiles 
void tile(float *r, float *a, int len, int tiles)
{
    int datasz = sizeof(float); // float is 4 bytes
    float* start = r;
    for (int i=0; i< tiles; ++i)
    {
        memcpy(start, a, len*datasz);
        start += len;
    }
}


// void			put(float *r, float *a, int *indeces);
//
// Puts source array into result array mapped by given indeces
// r - result containing mapped values 
// a - source array
// len - length of source
// indeces - array of target indeces
void put(float *r, float *a, int len, int *indeces)
{
    for (int i=0; i<len; ++i)
        r[indeces[i]] = a[i];
}


// void 		take(float *r, float *a, int *indeces);
//
// Takes values from source mapped by given indeces array, into result array 
// r - result containing mapped values 
// a - source array
// len - length of source
// indeces - array of source indeces
void take(float *r, float *a,int len, int *indeces)
{
    for (int i=0; i< len; ++i)
        r[i] = a[indeces[i]];
}


// Options
Options::Options(int argc, char *argv[])
{
    char * p;
    int v = 0;
    file_path = NULL;
    file_name = NULL;
    char * file_arg = NULL;
    getcwd(working_dir, 1024);
	
    for (int i=0; i<255; i++)
    {
        option[i] = false;
        argument[i] = NULL;
    }
    
    for(int i=0; i<32; i++)
    {
        attribute[i] = NULL;
        value[i] = NULL;
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
        else if ((p = strchr(argv[i], '=')) && v<32)
        {
            attribute[v] = create_string_head(argv[i], int(p-argv[i]));
            if(*(p+1) == '"')
            {
                value[v] = create_string(p+2);
                value[v][strlen(value[v]-1)] = 0;
            }
            else
                value[v] = create_string(p+1);
            v++;

        }
        else if (file_arg == NULL)
        {
            file_arg = argv[i];
        }
	
    // Compute binary directory (if possible)
	
    binary_dir = NULL;
    {
        char * bin_path;
        if(is_absolute_path(argv[0]))
			bin_path = create_string(argv[0]);
        else
		bin_path = create_formatted_string("%s/%s", working_dir, argv[0]);
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

const char *
Options::GetValue(const char * a)
{
    for(int i=0; i<32 && attribute[i]; i++)
        if(equal_strings(a, attribute[i]))
            return value[i];
           
    return NULL;
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
    
    for(int i=0; i<32 && attribute[i]; i++)
        printf("%s = \"%s\"\n", attribute[i], value[i]);

    if (file_name)
    {
        printf("File Dir: %s\n", file_dir);
        printf("File Name: %s\n", file_name);
    }
}



// Dictionary

Dictionary::KeyValue::KeyValue(const char * k, const char * v, KeyValue * first)
{
    type = 0;
    key = create_string(k);
    value = create_string(v);
    next = first;
    prev = NULL;
	
    if(next != NULL)
        next->prev = this;
}



Dictionary::KeyValue::KeyValue(const char * k, const int v, KeyValue * first)
{
    type = 1;
    key = create_string(k);
    int_value = v;
    next = first;
    prev = NULL;
	
    if(next != NULL)
        next->prev = this;
}



Dictionary::KeyValue::KeyValue(const char * k, const float v, KeyValue * first)
{
    type = 2;
    key = create_string(k);
    float_value = v;
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
            kv->type = 0;
            kv->value = create_string(v);
            return;
        }
	
    first = new KeyValue(k, v, first);
}

void
Dictionary::Set(const char * k, const int v)
{
    for(KeyValue * kv = first; kv != NULL; kv = kv->next)
        if(equal_strings(k, kv->key))
        {
            destroy_string(kv->value);
            kv->type = 1;
            kv->value = create_formatted_string("%d", v);
            kv->int_value = v;
            return;
        }
	
    first = new KeyValue(k, v, first);
}

void
Dictionary::Set(const char * k, const float v)
{
    for(KeyValue * kv = first; kv != NULL; kv = kv->next)
        if(equal_strings(k, kv->key))
        {
            destroy_string(kv->value);
            kv->type = 2;
            kv->value = create_formatted_string("%.4f", v);
            kv->float_value = v;
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

const int
Dictionary::GetInt(const char * k)
{
    for(KeyValue * kv = first; kv != NULL; kv = kv->next)
        if(equal_strings(k, kv->key))
            return kv->int_value;
    return 0;
}

const float
Dictionary::GetFloat(const char * k)
{
    for(KeyValue * kv = first; kv != NULL; kv = kv->next)
        if(equal_strings(k, kv->key))
            return kv->float_value;
    return 0;
}

void
Dictionary::Print()
{
    for(KeyValue * kv = first; kv != NULL; kv = kv->next)
        switch(kv->type)
        {
            case 0:
                printf("\"%s\" : \"%s\"\n", kv->key, kv->value);
                break;

            case 1:
                printf("\"%s\" : \"%d\"\n", kv->key, kv->int_value);
                break;

            case 2:
                printf("\"%s\" : \"%.4f\"\n", kv->key, kv->float_value);
                break;

        }
}

const char *
Dictionary::Get(Dictionary::Iterator i)
{
	return i.kv->value;
}

const int
Dictionary::GetInt(Dictionary::Iterator i)
{
	return i.kv->int_value;
}

const float
Dictionary::GetFloat(Dictionary::Iterator i)
{
	return i.kv->float_value;
}

char *
Dictionary::GetString(Iterator i)
{
    switch(i.kv->type)
    {
        case 0:
            return create_string(i.kv->value);

        case 1:
            return create_formatted_string("%d", i.kv->int_value);

        case 2:
            return create_formatted_string("%.4f", i.kv->float_value);
    }
    
    return create_string("");
}

const char *
Dictionary::GetKey(Dictionary::Iterator i)
{
	return i.kv->key;
}



vector::vector():
size(0),
data(nullptr),
owns_data(false)
{}


vector::vector(size_t s):
    size(s),
    data(create_array(s)),
    owns_data(true)
{}

vector::vector(float * d, size_t s):
    size(s),
    data(d),
    owns_data(false)
{}

vector::vector(matrix m, size_t row):
    size(m.size_x),
    data(m[row]),
    owns_data(false)
{
    if(m.size_y < row-1)
        throw std::out_of_range("size_y != 1");
}



vector::vector(std::initializer_list<float> d)
{   
    size = d.size();
    owns_data = true;
    data = create_array(size);

    size_t i=0;
    for(auto value: d)
        data[i++] = value;
}



/*
vector &
vector::operator=(const vector & v)
{
    size = v.size;
    data = v.data;
    owns_data = false; // TODO: Need to check transfer of ownership later
    return *this;
}
*/


vector::~vector()
{}

    float &  
    vector::operator*()
    {
        return data[0];
    }

    float &     
    vector::operator[](size_t i)
    {
            return data[i];
    }

    float &     
    vector::operator()(size_t i)
    {
        if(i <0)
            throw std::out_of_range("i < 0");
           if(i > size-1)
        throw std::out_of_range("i out of range");  

            return data[i];
    }



float dot(vector & a, vector & b)
{
    if(a.size != b.size)
        throw std::out_of_range("a.size != b.size");
         
    float r = 0;
    for(int i=0; i<a.size; i++)
        r += a[i]*b[i];
    return r;
}



matrix::matrix():
    allocated_size_x(0), 
    allocated_size_y(0), 
    size_x(0), 
    size_y(0), 
    dims(2), 
    data(nullptr),
    owns_data(true)
{
}



matrix::matrix(size_t sx, size_t sy):
    allocated_size_x(sx), 
    allocated_size_y(sy), 
    size_x(sx), 
    size_y(sy), 
    dims(2), 
    data(create_matrix(sx,sy)),
    owns_data(true)
{
}


matrix::matrix(float ** data, size_t sx, size_t sy):
    allocated_size_x(sx), 
    allocated_size_y(sy), 
    size_x(sx), 
    size_y(sy), 
    dims(2), 
    data(data),
    owns_data(false)
{
}


matrix::matrix(std::initializer_list<std::initializer_list<float>> d)
{   
    size_x = 0;
    for(auto x: d)
    {
        size_t sx = x.size();
        if(sx > size_x)
            size_x = sx;
    }
    size_y = d.size();
    allocated_size_x = size_x;
    allocated_size_y = size_y;
    dims = 2;
    owns_data = true;
    data = create_matrix(size_x, size_y);

    size_t y = 0;
    for(auto row: d)
    {
        size_t x = 0;
        for(float value: row)
            data[y][x++] = value;
        y++;
    }
}


matrix& matrix::operator=(const matrix& m) // TODO: optimize
{
    if(size_x != m.size_x || size_y != m.size_y)
        throw std::out_of_range("out of range");

        for(size_t y=0; y<size_y; y++)
            for(size_t x=0; x<size_x; x++)
                data[y][x] = m.data[y][x]; 
        return *this;
}

float * matrix::operator[](size_t y)
{
    return data[y];
}

float * matrix::operator*()
{
    return data[0];
}

bool matrix::resize(size_t sx, size_t sy)
{
    if(sx > allocated_size_x)
        return false;
    
    if(sy > allocated_size_y)
        return false;
    size_x = sx;
    size_y = sy;
    return true;
}

void matrix::clear()
{
    resize(0, 0);
}

bool matrix::add_row(float * row) 
{
    return true; // NOT COMPLETE
}

bool matrix::filter(matrix & m, bool (*f)(float * row))
{
    clear();
    for(size_t y=0; y<m.size_y; y++)
        if(f(m[y]))
            ; // // COPY ARRAY m -> this
        // CHECK BOUNDS
    return true; // The rows from m fit in this
}

float & matrix::operator()(size_t y, size_t x)
{
    if(y <0)
        throw std::out_of_range("y<0");
    if(x <0)
        throw std::out_of_range("x<0");
    if(y >=size_y)
        throw std::out_of_range("y>=size_y");
    if(x >= size_x)
        throw std::out_of_range("x>=size_x");
    return data[y][x];
}

float matrix::max()
{
    float m = data[0][0];
    for(int i=1; i<size_x*size_y; i++)
    if(data[0][i] > m)
        m = data[0][i];
    return m;
}

void matrix::print(std::string name, int decimals)
{
    if(name != "")
        printf("%s =\n", name.c_str());
    int nums = 4; // int(log10(max()));
    for(size_t y=0; y<size_y; y++)
    {
        for(size_t x=0; x<size_x; x++)
            printf("%*.*f\t", nums+decimals+2, decimals, data[y][x]);
        printf("\n");
    }
}

void matrix::print(int decimals)
{
    print("", decimals);
}


const char *
check_file_exists(const char * path)
{
	if(path != NULL)
	{
		FILE * t = fopen(path, "rb");
		bool exists = (t != NULL);
		if(t) fclose(t);
		return (exists ? path : NULL);
	}
    
	return NULL;
}
