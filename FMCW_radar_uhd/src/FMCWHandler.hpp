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
                :Victim(victim_config,false),
                Attacker(attack_config,false){
                    if (run)
                    {
                        Victim.run_RADAR();
                    }
            }

    };
}

#endif