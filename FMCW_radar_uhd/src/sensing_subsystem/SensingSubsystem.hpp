#ifndef SENSINGSUBSYSTEM
#define SENSINGSUBSYSTEM

    //c standard library
    #include <iostream>
    #include <cstdlib>
    #include <string>
    #include <complex>
    #include <csignal>

    //JSON class
    #include <nlohmann/json.hpp>

    //source libraries
    #include "../JSONHandler.hpp"
    #include "../USRPHandler.hpp"
    #include "../BufferHandler.hpp"
    #include "SpectrogramHandler.hpp"
    #include "EnergyDetector.hpp"

    // add in namespaces as needed
    using json = nlohmann::json;
    using USRPHandler_namespace::USRPHandler;
    using Buffers::Buffer_2D;
    using Buffers::Buffer_1D;
    using SpectrogramHandler_namespace::SpectrogramHandler;
    using EnergyDetector_namespace::EnergyDetector;

    namespace SensingSubsystem_namespace{

        template<typename data_type>
        class SensingSubsystem {
            private:
                //key parts of sensing subsystem
                EnergyDetector<data_type> energy_detector;
                SpectrogramHandler<data_type> spectrogram_handler;

                //pointer to usrp device
                USRPHandler<data_type> * attacker_usrp_handler;

                //configuration
                json config;

            public:
                /**
                 * @brief Construct a new Sensing Subsystem object
                 * 
                 * @param config_data JSON configuration object for the attacker
                 * @param usrp_handler pointer to a USRP handler for the sensing subsystem to use
                 */
                SensingSubsystem(json config_data, USRPHandler<data_type> * usrp_handler):
                    config(config_data),
                    attacker_usrp_handler(usrp_handler),
                    energy_detector(config_data),
                    spectrogram_handler(config_data){

                        //measure the relative noise power for the energy detector
                        mesaure_relative_noise_power();
                }

                ~SensingSubsystem(){};

                void mesaure_relative_noise_power(void){
                    std::cout << "SensingSubsystem::measure_relative_noise_power: measurig relative noise power" << std::endl;

                    //stream the ambient signal
                    attacker_usrp_handler -> rx_stream_to_buffer(& energy_detector.noise_power_measureent_signal);

                    energy_detector.compute_relative_noise_power();

                    std::cout << "relative noise power: " << energy_detector.relative_noise_power << "dB" << std::endl;
                    return;
                }

        };
    }
#endif