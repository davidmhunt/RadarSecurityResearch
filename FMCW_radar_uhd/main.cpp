
//C standard libraries
#include <iostream>
#include <cstdlib>
#include <string>

//JSON class
#include <nlohmann/json.hpp>

//source libraries
#include "src/JSONHandler.hpp"

//set namespaces
using json = nlohmann::json;

int main(int, char**) {

    std::string config_file = "/home/david/Documents/RadarSecurityResearch/FMCW_radar_uhd/Config_uhd.json";

    //read the config file
    std::cout << "\nMAIN: Parsing JSON\n";
    json config = JSONHandler::parse_JSON(config_file);

    std::cout << "\nMAIN: printing JSON\n";
    JSONHandler::print_file(config_file);
}
