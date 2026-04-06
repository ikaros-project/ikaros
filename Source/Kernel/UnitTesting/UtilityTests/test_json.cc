#include <cassert>
#include <iostream>
#include <string>

#include "../../dictionary.h"
#include "../../xml.h"
#include "../../xml.cc"
#include "../../dictionary.cc"
#include "../../utilities.cc"

using namespace ikaros;

int
main()
{
    value json = parse_json(R"({"name":"John Doe","age":30,"is_student":false,"scores":[85.5,90.2,78],"address":{"city":"New York","zip":"10001"}})");

    assert(json.is_dictionary());
    assert(json["name"].as_string() == "John Doe");
    assert(json["age"].as_int() == 30);
    assert(json["is_student"].is_bool());
    assert(!json["is_student"].is_true());
    assert(json["scores"][0].as_double() == 85.5);
    assert(json["address"]["city"].as_string() == "New York");
    assert(json["address"]["zip"].as_string() == "10001");

    std::cout << "test_json: ok\n";
    return 0;
}
