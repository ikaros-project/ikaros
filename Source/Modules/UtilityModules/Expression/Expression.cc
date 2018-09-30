//
//	Expression.cc		This file is a part of the IKAROS project
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
//  This example is intended as a starting point for writing new Ikaros modules
//  The example includes most of the calls that you may want to use in a module.
//  If you prefer to start with a clean example, use he module MinimalModule instead.
//

#include "Expression.h"

// use the ikaros namespace to access the math library
// this is preferred to using math.h

using namespace ikaros;

Expression::Expression(Parameter * p):
    Module(p)
{
    expression_str = GetValue("expression");
    std::string inputs = GetValue("inputs");
    // std::string output = GetValue("output");
    
    // split variables to get number of inputs
    // TODO input sanitation
    variables = split_string(inputs, ',');
    int num_inp = variables.size();
    // iterate over variables and create inputs
    
    for(int i = 0; i < num_inp; i++)
    {
        AddInput(variables[i].c_str());
    }
    AddOutput("OUTPUT");
    
}

void
Expression::SetSizes()
{
    int sz = GetInputSize(variables[0].c_str());
    // check that all inputs are same size
    for(int i=1; i<variables.size(); ++i)
    {
        int tst = GetInputSize(variables[i].c_str()); 
        if (tst == unknown_size) return;
        if (tst != sz)
        {
            std::stringstream ss(std::stringstream::in | std::stringstream::out);
            ss << "Expression '" << expression_str << "': Inputs have different sizes, must have same size.";
            Notify(msg_fatal_error, ss.str().c_str()); 
        }
        sz = tst;
    }
    SetOutputSize("OUTPUT", sz);

}
void
Expression::Init()
{
    //Bind(expression_str, "expression");
    //Bind(variables, "variables");
	Bind(debugmode, "debug");    

    //input_array = GetInputArray("INPUT");
    //input_array_size = GetInputSize("INPUT");
    
    for(int inp = 0; inp < variables.size(); inp++)
    {
        input.push_back(new float[GetInputSize(variables[0].c_str())]);
        input[inp] = GetInputArray(variables[inp].c_str());
    }
    

    output_array = GetOutputArray("OUTPUT");
    output_array_size = GetOutputSize("OUTPUT");

    internal_array = create_array(variables.size() + 1);
    // set up parser
    
    for(int i = 0; i < variables.size(); i++)
    {
        symbol_table.add_variable(variables[i], internal_array[i]);
    }
    symbol_table.add_constants();
    expression.register_symbol_table(symbol_table);
    parser.compile(expression_str, expression);
    
    
}



Expression::~Expression()
{
    // Destroy data structures that you allocated in Init.
    destroy_array(internal_array);
}



void
Expression::Tick()
{
    
    for(int i = 0; i < output_array_size; i++)
    {
        for(int var = 0; var < variables.size(); var++)
        {
            internal_array[var] = input[var][i];
        }
        output_array[i] = expression.value();
    }
    
	if(debugmode)
	{
		// print out debug info
        print_array("Expression: ", output_array, output_array_size);
	}
}



// Install the module. This code is executed during start-up.

static InitClass init("Expression", &Expression::Create, "Source/Modules/UtilityModules/Expression/");


