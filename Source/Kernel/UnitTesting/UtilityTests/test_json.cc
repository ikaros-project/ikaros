#include "json.cc"


int main() {
    std::string jsonString = R"({"name": "John Doe", "age": 30, "is_student": false, "scores": [85.5, 90.2, 78, 1, 2, 3, 4, 5], "address": {"city": "New York", "zip": "10001"}})";
    try {
        JSON json = JSON::parse(jsonString);
        std::cout << "Parsed JSON successfully." << std::endl;

        // Access elements using the overloaded operator[]
        std::cout << "Name: " << json["name"].asString() << std::endl;
        std::cout << "Age: " << json["age"].asNumber() << std::endl;
        std::cout << "Is student: " << json["is_student"].asBool() << std::endl;
        std::cout << "First score: " << json["scores"][0].asNumber() << std::endl;
        std::cout << "City: " << json["address"]["city"].asString() << std::endl;

        // Iterate over array elements
        std::cout << "Scores: ";
        for (auto& score : json["scores"]) {
            std::cout << score.asNumber() << std::endl;
        }
        std::cout << std::endl;

        // Iterate over object properties
        std::cout << "Address: ";
        for (auto& [key, value] : json["address"].asObject()) {
            std::cout << key << ": " << value.asString() << std::endl;
        }
        std::cout << std::endl;

    } catch (const std::exception& ex) {
        std::cerr << "Error parsing JSON: " << ex.what() << std::endl;
    }
    return 0;
}

