
#include "../utilities.cc"   
#include <string>

using namespace ikaros;

int
main()
{
    std::string s = "aaa++bbb++ccc++ddd"; 

    std::string s1 = s;
    std::string s2 = s;
    std::string s3 = s;
    std::string s4 = s;

    std::string h = head(s1, "++");
    std::string t = tail(s2, "++");
    
    std::string h1 = rhead(s3, "++");
    std::string t1 = rtail(s4, "++");

    std::cout << h << "|" << s1 << std::endl;
    std::cout << s2 << "|" << t << std::endl;

    std::cout << h1 << "|" << s3 << std::endl;
    std::cout << s4 << "|" << t1 << std::endl;
}

