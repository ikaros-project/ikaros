

#include "../../utilities.cc" 
#include "../../matrix.cc"
//#include "matrix.h"    
#include <string>

using namespace ikaros;

int
main()
{


    matrix A = {{1, 2, 3}, {-1, -2, -3}}; // 2x3
    matrix B = {{1, 1}, {2, 2}, {3, 3}}; // 3x2
    matrix C;

    C.matmul(A,B);

    C.print("C");



/*
    matrix m = "1, 2; 3, 4";

    std::cout << m.size() << std::endl;
    std::cout << m.sum() << std::endl;
    std::cout << m.average() << std::endl;

    std::cout << m << std::endl;
    m.print("XXX");


std::cout << m << std::endl;
    // matrix x = {{{1, 2}, {3, 4}}, {{69,70}, {9, 10}}};


matrix x;
 x = std::string("11, 22, 33; 4, 5, 6");

    std::cout << x.json() << std::endl;

    m.set_name("m");
    m.print();

    m.set_labels(0, "A", "B");
    m.set_labels(1, "X", "Y");
    m.print();


    std::cout << m << std::endl;

    std::cout << m.size() << std::endl;
*/

}

