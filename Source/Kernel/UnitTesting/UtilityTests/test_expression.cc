//
//
// Code to test the expression avaluation
//
//

#include <iostream>
#include <string>
#include <vector>
#include <set>

#include "../expression.h"



int
main()
{   
    // Define variables and values

    variables vars = {{"x", "20"}, {"y", "10"}};

    // Define expression

    auto e = expression("x+y");

    // List variables in expression

    for(auto s : e.variables())
        std::cout <<s << std::endl;

    // Print expression tree

    e.print();

    // evaluate expression with the list of variables

    int x = e.evaluate(vars);

    // print result

    std::cout << x << '\n';

    std::cout << expression("1+2+3").evaluate() << '\n'; 
    std::cout << expression("(1+2)/(5-2)").evaluate() << '\n'; 
    std::cout << expression("1/2/3").evaluate() << '\n'; 
    std::cout << expression("10*(7-9)").evaluate() << '\n'; 
    std::cout << expression("(1+2)*3").evaluate() << '\n'; 
}


