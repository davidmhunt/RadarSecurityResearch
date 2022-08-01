
//C standard libraries
#include <iostream>
#include <cstdlib>
#include <string>
#include <chrono>
#include <complex>
#include <csignal>
#include <thread>

//uhd specific libraries
#include <uhd/exception.hpp>
#include <uhd/types/tune_request.hpp>
#include <uhd/usrp/multi_usrp.hpp>
#include <uhd/utils/safe_main.hpp>
#include <uhd/utils/thread.hpp>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/program_options.hpp>

//JSON class
#include <nlohmann/json.hpp>

//source libraries
#include "src/JSONHandler.hpp"
#include "src/USRPHandler.hpp"

//set namespaces
using json = nlohmann::json;
using USRPHandler_namespace::USRPHandler;

int UHD_SAFE_MAIN(int argc, char* argv[]) {

    std::string config_file = "/home/david/Documents/RadarSecurityResearch/FMCW_radar_uhd/Config_uhd.json";

    //read the config file
    std::cout << "\nMAIN: Parsing JSON\n";
    json config = JSONHandler::parse_JSON(config_file,false);

    USRPHandler usrp_handler =  USRPHandler(config);
    return EXIT_SUCCESS;
}
