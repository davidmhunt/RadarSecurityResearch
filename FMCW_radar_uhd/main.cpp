
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
#include "src/BufferHandler.hpp"
#include "src/FMCWHandler.hpp"

//set namespaces
using json = nlohmann::json;
using USRPHandler_namespace::USRPHandler;
using Buffers::Buffer;
using Buffers::Buffer_1D;
using FMCW_namespace::FMCW;

int UHD_SAFE_MAIN(int argc, char* argv[]) {

    std::string config_file = "/home/david/Documents/RadarSecurityResearch/FMCW_radar_uhd/Config_uhd.json";
    //std::string config_file = "/home/david/Documents/RadarSecurityResearch/FMCW_radar_uhd/Config_uhd_highbw.json";
    //std::string config_file = "/home/david/Documents/RadarSecurityResearch/FMCW_radar_uhd/Config_uhd_highvres.json";

    //read the config file
    std::cout << "\nMAIN: Parsing JSON\n";
    json config = JSONHandler::parse_JSON(config_file,false);

    //initialize the FMCW device
    //determine that there is a valid cpu format and type
    if(config["USRPSettings"]["Multi-USRP"]["type"].is_null() ||
        config["USRPSettings"]["Multi-USRP"]["cpufmt"].is_null()){
            std::cerr << "MAIN: type or cpu format is not specified in config file" <<std::endl;
            return EXIT_FAILURE;
    }
    std::string type = config["USRPSettings"]["Multi-USRP"]["type"].get<std::string>();
    std::string cpufmt = config["USRPSettings"]["Multi-USRP"]["cpufmt"].get<std::string>();

    if (type == "double" && cpufmt == "fc64"){
        FMCW<std::complex<double>> fmcw_handler(config);
    }
    else if (type == "float" && cpufmt == "fc32")
    {
        FMCW<std::complex<float>> fmcw_handler(config);
    }
    else if (type == "int16_t" && cpufmt == "sc16")
    {
        FMCW<std::complex<int16_t>> fmcw_handler(config);
    }
    else if (type == "int8_t" && cpufmt == "sc8")
    {
        FMCW<std::complex<int8_t>> fmcw_handler(config);
    }
    else{
        std::cerr << "MAIN: type and cpufmt don't match valid combination" << std::endl;
    }


/*
    //configure USRP
    std::cout << "\nMAIN: Initializing USRP Handler\n";
    USRPHandler usrp_handler(config);
 
 
    usrp_handler.load_BufferHandler( & buffer_handler);
    
    //stream the frame
    std::cout << "\nMAIN: Streaming Frames\n";
    usrp_handler.stream_frames();

    //EXTRA CODE FOR DEBUGGING PURPOSES

    //std::cout << "Rx Buffer Preview" <<std::endl;
    //buffer_handler.print_2d_buffer_preview(buffer_handler.rx_buffer);

    //buffer_handler.rx_buffer = buffer_handler.tx_buffer;
    //buffer_handler.save_rx_buffer_to_file();
*/
    return EXIT_SUCCESS;
}
