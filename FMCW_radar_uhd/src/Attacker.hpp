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
    #include "sensing_subsystem/SensingSubsystem.hpp"
    #include "attacking_subsystem/AttackingSubsystem.hpp"

    using json = nlohmann::json;
    using USRPHandler_namespace::USRPHandler;
    using Buffers::Buffer_2D;
    using Buffers::Buffer_1D;
    using SensingSubsystem_namespace::SensingSubsystem;
    using AttackingSubsystem_namespace::AttackingSubsystem;

    namespace ATTACKER_namespace{
        
        /**
         * @brief Class for the attacker
         * 
         * @tparam data_type the data_type of the data that the attacker uses
         */
        template<typename data_type>
        class ATTACKER{
            private:

                //configuration information
                json config;
                USRPHandler<data_type> usrp_handler;

                //key subsystems
                AttackingSubsystem<data_type> attacking_subsystem;
                SensingSubsystem<data_type> sensing_subsystem;


            public:

                /**
                 * @brief Construct a new ATTACKER object,
                 * loads the configuration, and initializes the usrp_handler 
                 * 
                 * @param config_data a json config object
                 * @param run on false (default) does not run attacker implementation,
                 * on true runs the attacker implementation
                 */
                ATTACKER(json config_data, bool run = false):
                    config(config_data),
                    usrp_handler(config_data),
                    attacking_subsystem(config_data, & usrp_handler),
                    sensing_subsystem(config_data, & usrp_handler, & attacking_subsystem){

                    
                    //run if specified
                    if (run){
                        //run the sensing subsystem
                        sensing_subsystem.run();
                    }
                }

                void run_attacker(void){
                    usrp_handler.reset_usrp_clock();
                    
                    sensing_subsystem.run();
                    //sensing subsystem will automatically call the attacking subsystem when needed
                }
        };
    }

#endif