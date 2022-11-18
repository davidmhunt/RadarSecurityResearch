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
    #include "../BufferHandler.hpp"

    using json = nlohmann::json;
    using namespace Buffers;

    namespace EnergyDetector_namespace{
        template<typename data_type>
        class EnergyDetector{
            private:
                //config object
                json config;
            public:
                //other configuration information
                data_type relative_noise_power; //dB
                data_type threshold_level; //dB
            private:
                data_type sampling_frequency; //Hz

                // parameters for initial 
                size_t num_samples_noise_power_measurement_signal;
                size_t num_rows_noise_power_measurement_signal;
                size_t samples_per_buffer;
            public:
                Buffers::Buffer_2D<std::complex<data_type>> noise_power_measureent_signal;

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
                // check for energy detection threshold
                if(config["SensingSubsystemSettings"]["energy_detection_threshold_dB"].is_null()){
                    std::cerr << "EnergyDetector::check_config: energy_detection_threshold_dB not specified" <<std::endl;
                    config_good = false;
                }

                //check for noise power measurement time
                if(config["SensingSubsystemSettings"]["noise_power_measurement_time_ms"].is_null()){
                    std::cerr << "EnergyDetector::check_config: noise_power_measurement_time_ms not specified" <<std::endl;
                    config_good = false;
                }

                //check for samples per buffer
                if(config["USRPSettings"]["RX"]["spb"].is_null()){
                    std::cerr << "EnergyDetector::check_config: spb for Rx not specified" <<std::endl;
                    config_good = false;
                }

                return config_good;
            }

            /**
             * @brief Initializes all energy detection parameters
             * 
             */
            void initialize_energy_detector_params(){
                //relative noise power
                relative_noise_power = 0;

                //threshold for detecting new chirps
                threshold_level = 
                    config["SensingSubsystemSettings"]["energy_detection_threshold_dB"].get<data_type>();

                //sampling frequency
                sampling_frequency = config["USRPSettings"]["Multi-USRP"]["sampling_rate"].get<data_type>();

                //samples per buffer
                samples_per_buffer = config["USRPSettings"]["RX"]["spb"].get<size_t>();

                //determine number of rows and samples in noise power measurement signal
                data_type row_period = static_cast<data_type>(samples_per_buffer)/sampling_frequency;
                data_type noise_power_measurement_time = config["SensingSubsystemSettings"]["noise_power_measurement_time_ms"].get<data_type>();
                num_rows_noise_power_measurement_signal = 
                    static_cast<size_t>(std::ceil((noise_power_measurement_time * 1e-3)/row_period));

                //determine the number of samples per rx signal
                num_samples_noise_power_measurement_signal = 
                    num_rows_noise_power_measurement_signal * samples_per_buffer;
                
                // initialize the noise sampling buffer
                noise_power_measureent_signal = 
                    Buffer_2D<std::complex<data_type>>(num_rows_noise_power_measurement_signal,samples_per_buffer);

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
             */
            void compute_relative_noise_power(){

                //flatten the noise power measurement signal
                std::vector<std::complex<data_type>> sampled_signal(num_samples_noise_power_measurement_signal);
                size_t to_idx = 0;
                for (size_t i = 0; i < num_rows_noise_power_measurement_signal; i++)
                {
                    for (size_t j = 0; j < samples_per_buffer; j++)
                    {
                        to_idx = (samples_per_buffer * i) + j;
                        sampled_signal[to_idx] = noise_power_measureent_signal.buffer[i][j];
                    }
                }


                //set the relative noise power
                relative_noise_power = compute_signal_power(sampled_signal);
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