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
        
            bool attack_enabled;
            bool victim_enabled;

            json victim_config;
            json attack_config;
            
            RADAR<data_type> Victim;
            ATTACKER<data_type> Attacker;

        public:
            /**
             * @brief Construct a new FMCWHandler object
             * 
             * @param victim_config_obj a JSON config object for the victim
             * @param attack_config_obj a JSON config object for the attacker
             * @param run (default false) on true, runs the experiment
             */
            FMCWHandler(json victim_config_obj,json attack_config_obj, bool run = false)
                :victim_config(victim_config_obj),
                attack_config(attack_config_obj),
                Victim(victim_config_obj,true, false),
                Attacker(attack_config_obj,true,false){
                    
                    if(check_config())
                    {
                        get_enabled_status();
                        if (run)
                        {
                            run_FMCW();
                        }
                        
                    }
            }
            
            /**
             * @brief Check the json config files to make sure all necessary parameters are included
             * 
             * @return true - JSON is all good and has required elements
             * @return false - JSON is missing certain fields
             */
            bool check_config(){
                bool config_good = true;
                //check sampling rate
                if(attack_config["Attacker_enabled"].is_null()){
                    std::cerr << "FMCWHandler::check_config: Attacker_enabled not in JSON" <<std::endl;
                    config_good = false;
                }

                if(victim_config["Radar_enabled"].is_null()){
                    std::cerr << "FMCWHandler::check_config: Radar_enabled not in JSON" <<std::endl;
                    config_good = false;
                }
                
                return config_good;
            }

            /**
             * @brief Get the enabled status from the JSON
             * 
             */
            void get_enabled_status(void){
                attack_enabled = attack_config["Attacker_enabled"].get<bool>();
                victim_enabled = victim_config["Radar_enabled"].get<bool>();
            }

            /**
             * @brief Run the FMCW simulation with the vicitm and attacker in separate threads
             * 
             */
            void run_FMCW(void){

                if (attack_enabled && victim_enabled)
                    {
                        //create victim thread
                        std::thread victim_thread([&]() {
                            Victim.run_RADAR();
                        });

                        //run the attacker
                        std::cout << "running attack" << std::endl;
                        Attacker.run_attacker();

                        //wait for victim thread to finish
                        victim_thread.join();
                    }
                    else if (attack_enabled)
                    {
                        //run the attacker
                        std::cout << "running attack" << std::endl;
                        Attacker.run_attacker();
                    }
                    else
                    {
                        //run the victim
                        Victim.run_RADAR();
                    }
            }

    };
}

#endif