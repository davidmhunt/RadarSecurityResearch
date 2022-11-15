#ifndef ENERGYDETECTOR
#define ENERGYDETECTOR

    //C standard libraries
    #include <iostream>
    #include <cstdlib>
    #include <string>
    #include <complex.h>
    #include <vector>
    #include <csignal>
    #include <thread>

    #define _USE_MATH_DEFINES
    #include <cmath>

    //include the JSON handling capability
    #include <nlohmann/json.hpp>

    using json = nlohmann::json;

    namespace EnergyDetector_namespace{
        template<typename data_type>
        class EnergyDetector{
            private:
                //config object
                json config;

                //other configuration information
                data_type relative_noise_power; //dB
                data_type threshold_level; //dB
                data_type sampling_frequency; //Hz
            public:

            /**
             * @brief Construct a new Energy Detector object
             * 
             * @param json_config JSON configuration object with required information
             */
            EnergyDetector(json json_config):config(json_config){
                if (check_config())
                {
                    initialize_energy_detector_params();
                }
                
            }

            /**
             * @brief Destroy the Energy Detector object
             * 
             */
            ~EnergyDetector () {};

            /**
             * @brief Check the json config file to make sure all necessary parameters are included
             * 
             * @return true - JSON is all good and has required elements
             * @return false - JSON is missing certain fields
             */
            bool check_config(){
                bool config_good = true;
                //check sampling rate
                if(config["USRPSettings"]["Multi-USRP"]["sampling_rate"].is_null()){
                    std::cerr << "EnergyDetector::check_config: no sampling_rate in JSON" <<std::endl;
                    config_good = false;
                }

                if(config["SensingSubsystemSettings"]["energy_detection_threshold_dB"].is_null()){
                    std::cerr << "EnergyDetector::check_config: energy_detection_threshold_dB not specified" <<std::endl;
                    config_good = false;
                }

                return config_good;
            }

            /**
             * @brief Initializes all energy detection parameters
             * 
             */
            void initialize_energy_detector_params(){
                relative_noise_power = 0;
                threshold_level = 
                    config["SensingSubsystemSettings"]["energy_detection_threshold_dB"].get<data_type>();
                sampling_frequency = config["USRPSettings"]["Multi-USRP"]["sampling_rate"].get<data_type>();
            }
            
            /**
             * @brief Compute the power of a given rx signal
             * 
             * @param rx_signal the rx signal to compute the power of
             * @return data_type the computed signal power level
             */
            data_type compute_signal_power (std::vector<std::complex<data_type>> & rx_signal){
                
                //get determine the sampling period of the rx_signal
                data_type sampling_period = static_cast<data_type>(rx_signal.size()) / sampling_frequency;
                
                //compute the sum of the elements
                data_type sum;

                for (size_t i = 0; i < rx_signal.size(); i++)
                {
                    sum += ((real(rx_signal[i]) * real(rx_signal[i])) + (imag(rx_signal[i]) * imag(rx_signal[i])));
                }

                //convert to dB and return
                data_type power = 10 * std::log10(sum/sampling_period);
                return power;
            }

            /**
             * @brief Set the relative noise power level which will be used to detect chirps
             * 
             * @param rx_signal the rx signal to sample the noise power level over
             */
            void compute_relative_noise_power(std::vector<std::complex<data_type>> & rx_signal){
                
                //set the relative noise power
                relative_noise_power = compute_signal_power(rx_signal);
                return;
            }

            /**
             * @brief Checks to see if a chirp was detected in the given rx_signal
             * 
             * @param rx_signal the rx_signal
             * @return true - returned if there was a chirp detected
             * @return false - returned if no chirp was detected
             */
            bool check_for_chirp(std::vector<std::complex<data_type>> & rx_signal){

                //compute the rx signal power and determine if it is sufficiently higher than the threshold
                if ((compute_signal_power(rx_signal) - relative_noise_power) >= threshold_level )
                {
                    return true;
                }
                else{
                    return false;
                }
                
            }
        };
    }

#endif