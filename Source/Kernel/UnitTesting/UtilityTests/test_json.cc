#include <cassert>
#include <cmath>
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

    value escaped = parse_json(R"({"message":"line\nquote\"tab\t"})");
    assert(escaped["message"].as_string() == "line\nquote\"tab\t");

    bool threw_on_raw_control_char = false;
    try
    {
        parse_json(std::string("{\"bad\":\"a\nb\"}"));
    }
    catch(const std::runtime_error &)
    {
        threw_on_raw_control_char = true;
    }
    assert(threw_on_raw_control_char);

    value nan_value(std::nan(""));
    value inf_value(std::numeric_limits<double>::infinity());
    assert(nan_value.json() == "null");
    assert(inf_value.json() == "null");

    std::cout << "test_json: ok\n";
    return 0;
}
