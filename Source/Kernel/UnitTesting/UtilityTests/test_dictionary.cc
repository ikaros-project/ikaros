// dictionary_test.cc

//
// Test and demonstration code for dictionary
//

// Everything is included here to allow compilation of this file on its own

#include <string>
#include <iostream>

#include "../dictionary.h"
#include "../xml.h"
#include "../xml.cc"
#include "../dictionary.cc"
#include "../utilities.cc"

using namespace ikaros;

int
main()
{
    std::string s =  R"({"name": "John Doe", "age": 30, "is_student": false, "scores": [85.5, 90.2, 78], "address": {"city": "New York", "zip": "10001"}})";
  
    dictionary d = parse_json(s);
    d.print();

/**
    dictionary d;
    dictionary e;

    d["a"] = "AAA";
    d["b"] = true;


    e["c"] = 3.14;
    e["d"] = d.copy();

    d["b"] = false; // Changes value in e as well.

    e.print();      // {"b": false, "c": 3.14, "d": {"a": "AAA", "b": true}}

    list l;
    l.push_back("x");
    l.push_back("y");
    l.push_back("z");
    l.print();

    // Iterating over key and value:

      for (const auto& [key, val] : d)
        std::cout << "Key: " << key << ", Value: " << val << std::endl;

    // Iterating over items in a list

      for (const auto& val : l)
        std::cout << "Value: " << val << std::endl;
*/

    /*
    dictionary d;

    d["a"] = "AAA"; // string value
    d["b"] = true;  // bool value
    d["c"] = 3.14;  // double value
    d["d"] = 32;    // double value

   list l;

    l[0] = "First";
    l[1] = 2;
    l[5] = 5; // list will be padded with null values to hold 6 elements
    l.push_back(6); // add value to the back of list

    dictionary e;

    e["x"] = "XXX";
    e["y"] = d;     // dictionary value
    e["z"] = l;     // list value

    d.print();
    l.print();
    e.print();


    value v;
    v = 22;
    v.print();


    dictionary x;

    x["a"]["b"][3]["W"] = "c";
    x.print();



        dictionary xx;
        xx.parse_url("a=123&b=XXX&xxx=oiuy");
        std::cout << xx.json() << std::endl;



    dictionary d;
    dictionary e = d;

    d["a"] = "AAA";
    d["b"] = "BBB";
    d["c"] = true;
    d["d"] = 42;
    d["e"] = 3.14;

    d["a"] = "XXX";

 std::cout <<  d << std::endl;
 std::cout <<  e.json() << std::endl;


dictionary xmldict = dictionary("../Modules/TestModules/Test1.ikc");
//std::cout << xmldict.json() << std::endl;
std::cout <<  std::endl;
std::cout << xmldict.xml("group") << std::endl;



    dictionary d;

    d["a"] = "AAA";
    d["b"] = "BBB";

    d["c"] = dictionary();  // optional
    d["c"]["d"] = "CD";


    d["d"]["e"] = "DE";     // dictionary is created when accessed

    d["e"] = list();        // optional
    d["e"].push_back("X");

    d["f"].push_back("X");  // list at e is cerated when accessed
    d["f"][1] = "Y";        // create space in list when assigned

    std::cout << d << std::endl;


    dictionary d2;

    d2["A"]["B"][3]["C"][2] = "42";     // creates full data structure

    std::cout << d2 << std::endl;

        d2["A"]["B"][3]["D"][2] = "43";     // creates full data structure

    std::cout << d2 << std::endl;

    std::string s = d2["A"]["B"][3]["D"][2];

    std::cout << s << std::endl;

    value v = "xyz";    

    std::cout << v.size() << std::endl; // size is 1 since value contains one string

    value u;
    u["A"] = "1";
    u["B"] = "2";   

    std::cout << u.size() << std::endl; // size is 2 since u contains two entires

    value t;
    t[3] = "22";

     std::cout << t.size() << std::endl; // size is 4

    list z;
    z[3] = "22";

     std::cout << z << " size:"<< z.size() << std::endl; // size is 4


    std::cout << std::endl;

    for(auto & p : dictionary(u))
    {
        std::cout << p.first << ": " << p.second << std::endl;
    }


    dictionary d42("../Modules/TestModules/Test1.ikc");

        std::cout << std::endl;
        std::cout << std::endl;

        dictionary dddd;
        dddd["a"] = "32";
        dddd["l"][0] = "X";
        dddd["l"][1] = "Y";
        dddd["l"][3] = "Z";

        std::cout << dddd.xml("group") << std::endl;

    value xxx;

    int a = 42;
    int * ap = &a;
    int & b(*ap);

        std::cout << b << std::endl;


    dictionary p;
    dictionary q;

    p["a"] = "ap";
    p["b"] = "bp";

    q["b"] = "bq";
    q["c"] = "cq";

    p.merge(q, true);

        std::cout << p << std::endl;


   std::string json_str = R"({"key": "value", "number": 123, "array": [true, false, null]})";
    ikaros::value parsed_value = ikaros::parse_json(json_str);
    std::cout << parsed_value.json() << std::endl; // Output the JSON representation


    value x = "Hello!";
    std::cout << x.json() << std::endl;
*/
    return 0;
}

