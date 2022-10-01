#ifndef ATTACKERCLASS
#define ATTACKERCLASS

    //c standard library
    #include <iostream>
    #include <cstdlib>
    #include <string>
    #include <complex>
    #include <csignal>

    //JSON class
    #include <nlohmann/json.hpp>

    //source libraries
    #include "JSONHandler.hpp"
    #include "USRPHandler.hpp"
    #include "BufferHandler.hpp"

    using json = nlohmann::json;
    using USRPHandler_namespace::USRPHandler;
    using Buffers::RADAR_Buffer;
    using Buffers::Buffer_1D;

    namespace ATTACKER_namespace{
        
        /**
         * @brief Class for the attacker
         * 
         * @tparam data_type the data_type of the data that the attacker uses
         */
        template<typename data_type>
        class ATTACKER{
            private:
                json config;
                USRPHandler<data_type> usrp_handler;

            public:

                ATTACKER(json config_data, bool run = false): config(config_data),usrp_handler(config_data){
                    
                    //run if specified
                    if (run){
                        std::cout << "Run Attack not yet enabled" <<std::endl;
                    }
                }
        };
    }

#endif