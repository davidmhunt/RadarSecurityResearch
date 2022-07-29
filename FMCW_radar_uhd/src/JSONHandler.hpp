#include <iostream>
#include <cstdlib>
#include <fstream>
#include <string>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace JSONHandler {
    json parse_JSON(std::string & file_name);
    void print_file(std::string & file_name);
}