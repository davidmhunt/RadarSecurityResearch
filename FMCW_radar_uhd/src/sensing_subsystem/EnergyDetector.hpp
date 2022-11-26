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

                // parameters for initial noise power measurement
                size_t num_samples_noise_power_measurement_signal;
                size_t num_rows_noise_power_measurement_signal;
                size_t samples_per_buffer;

                //parameters for chirp energy detection
                size_t current_chirp_detector_index;
                Buffers::Buffer_1D<data_type> chirp_detection_times;
            public:
                size_t num_rows_chirp_detector;
                Buffers::Buffer_2D<std::complex<data_type>> noise_power_measureent_signal;
                Buffers::Buffer_2D<std::complex<data_type>> chirp_detector_signal;
                


            public:

            /**
             * @brief Construct a new Energy Detector object
             * 
             * @param json_config JSON configuration object with required information
             */
            EnergyDetector(json json_config):config(json_config){
                if (check_config())
                {
                    initialize_energy_detector();
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
                
                //check the number of rows for the chirp detector
                if(config["SensingSubsystemSettings"]["energy_detector_num_rows_chirp_detector"].is_null()){
                    std::cerr << "EnergyDetector::check_config: energy_detector_num_rows_chirp_detector not specified" <<std::endl;
                    config_good = false;
                }


                return config_good;
            }

            /**
             * @brief initializes the energy detector
             * 
             */
            void initialize_energy_detector(){
                initialize_energy_detector_params();
                initialize_noise_power_detection();
                initialize_chirp_detection_params();
            }

            /**
             * @brief Initializes all energy detection parameters
             * 
             */
            void initialize_energy_detector_params(){

                //sampling frequency
                sampling_frequency = config["USRPSettings"]["Multi-USRP"]["sampling_rate"].get<data_type>();

                //samples per buffer
                samples_per_buffer = config["USRPSettings"]["RX"]["spb"].get<size_t>();
            }


            /**
             * @brief initialize parameters to measure the relative noise power level
             * 
             */
            void initialize_noise_power_detection(){
                //relative noise power
                relative_noise_power = 0;

                //threshold for detecting new chirps
                threshold_level = 
                    config["SensingSubsystemSettings"]["energy_detection_threshold_dB"].get<data_type>();

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
             * @brief initialize the parameters used to detect chirps
             * 
             */
            void initialize_chirp_detection_params(){
               num_rows_chirp_detector = config["SensingSubsystemSettings"]["energy_detector_num_rows_chirp_detector"].get<size_t>();
                
                //initialize the chirp detector index to 0
                current_chirp_detector_index = 0;

                chirp_detector_signal = Buffer_2D<std::complex<data_type>>(num_rows_chirp_detector,samples_per_buffer);
                chirp_detection_times = Buffer_1D<data_type>(num_rows_chirp_detector);
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

            /**
             * @brief Check to see if a chirp was detected in the current chirp detection signal buffer
             * 
             * @param signal_start_time the time that the current chirp detection signal started at
             * @return true - chirp detected
             * @return false - chirp not detected
             */
            bool check_for_chirp(double signal_start_time){

                chirp_detection_times.buffer[current_chirp_detector_index] = static_cast<data_type>(signal_start_time);

                bool chirp_detected = false;
                data_type signal_power = compute_signal_power(chirp_detector_signal.buffer[current_chirp_detector_index]);
                if ((signal_power
                     - relative_noise_power) >= threshold_level )
                {
                    chirp_detected = true;
                }
                else{
                    chirp_detected = false;
                }
                //update the current chirp_detector index if no chirp was detected
                if (! chirp_detected)
                {
                    current_chirp_detector_index += 1;
                    current_chirp_detector_index = current_chirp_detector_index % num_rows_chirp_detector;
                }

                return chirp_detected;
            }

            /**
             * @brief Get the current chirp detector index object
             * 
             * @return size_t the current chirp detector index
             */
            size_t get_current_chirp_detector_index(){
                return current_chirp_detector_index;
            }


            /**
             * @brief Get the detection start time in us
             * 
             * @return data_type 
             */
            data_type get_detection_start_time_us(){
                return chirp_detection_times.buffer[current_chirp_detector_index] * 1e6;
            }
            
            /**
             * @brief reset the chirp detector
             * 
             */
            void reset_chirp_detector(){
                //reset the detected times
                for (size_t i = 0; i < num_rows_chirp_detector; i++)
                {
                    for (size_t j = 0; j < samples_per_buffer; j++)
                    {
                        chirp_detector_signal.buffer[i][j] = 0;
                    }
                    chirp_detection_times.buffer[i] = 0;   
                }
                current_chirp_detector_index = 0;
            }
        
            
            void save_chirp_detection_signal_to_buffer(Buffer_2D<std::complex<data_type>> * rx_buffer){

                size_t detection_idx = current_chirp_detector_index;

                for (size_t i = 0; i < num_rows_chirp_detector; i++)
                {
                    //incremeent the index 
                    detection_idx += 1;
                    detection_idx = detection_idx % num_rows_chirp_detector;

                    for (size_t j = 0; j < samples_per_buffer; j++)
                    {
                        rx_buffer -> buffer[i][j] = chirp_detector_signal.buffer[detection_idx][j];
                    }
                    

                }
                
            }

        };
    }

#endif