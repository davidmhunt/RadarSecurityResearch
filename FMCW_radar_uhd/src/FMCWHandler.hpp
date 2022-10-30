#ifndef FMCWHANDLER
#define FMCWHANDLER


//Radar Class
    #include "RADAR.hpp"
    #include "Attacker.hpp"
//JSON class
    #include <nlohmann/json.hpp>

using RADAR_namespace::RADAR;
using ATTACKER_namespace::ATTACKER;
using json = nlohmann::json;

namespace FMCWHandler_namespace {
    
    template<typename data_type>
    class FMCWHandler {
        private:
            RADAR<data_type> Victim;
            ATTACKER<data_type> Attacker;
            

        public:
            /**
             * @brief Construct a new FMCWHandler object
             * 
             * @param victim_config a JSON config object for the victim
             * @param run (default false) on true, runs the experiment
             */
            FMCWHandler(json victim_config,json attack_config, bool run = false)
                :Victim(victim_config,true, false),
                Attacker(attack_config,true,false){
                    if (run)
                    {
                        run_FMCW();
                    }
            }

            /**
             * @brief Run the FMCW simulation with the vicitm and attacker in separate threads
             * 
             */
            void run_FMCW(void){
                //create victim thread
                std::thread victim_thread([&]() {
                    Victim.run_RADAR();
                });

                //run the sensing subsystem
                std::cout << "running attack" << std::endl;
                Attacker.run_attacker();

                //wait for victim thread to finish
                victim_thread.join();
            }

    };
}

#endif