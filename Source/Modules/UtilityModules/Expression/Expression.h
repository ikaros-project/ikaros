//
//	Expression.h		This file is a part of the IKAROS project
//
//    Copyright (C) 2012 <Author Name>
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

#ifndef Expression_
#define Expression_

#include "IKAROS.h"
#include <vector>
#include "exprtk.hpp"
class Expression: public Module
{
public:
    static Module * Create(Parameter * p) { return new Expression(p); }

    Expression(Parameter * p);
    virtual ~Expression();

    void        SetSizes();
    void 		Init();
    void 		Tick();
    std::string do_replace( const std::string & in, const std::string & from,  const std::string & to );
    void myReplace(std::string& str,
               const std::string& oldStr,
               const std::string& newStr);
    // pointers to inputs and outputs
    // and integers to represent their sizes

    std::vector<float *>     input;
    int         input_array_size;

    float *     output_array;
    int         output_array_size;

    // internal data storage
    float *     internal_array;

    // parameter values

    std::string       expression_str;
    std::vector<std::string>       variables;
	bool       	debugmode;

    exprtk::symbol_table<float> symbol_table;
    exprtk::expression<float> expression;
    exprtk::parser<float> parser; //TODO make into a singleton
};

#endif
