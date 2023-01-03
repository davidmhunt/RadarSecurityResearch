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
    #include "../attacking_subsystem/AttackingSubsystem.hpp"
    #include "SpectrogramHandler.hpp"
    #include "EnergyDetector.hpp"

    // add in namespaces as needed
    using json = nlohmann::json;
    using USRPHandler_namespace::USRPHandler;
    using Buffers::Buffer_2D;
    using Buffers::Buffer_1D;
    using SpectrogramHandler_namespace::SpectrogramHandler;
    using EnergyDetector_namespace::EnergyDetector;
    using AttackingSubsystem_namespace::AttackingSubsystem;

    namespace SensingSubsystem_namespace{

        template<typename data_type>
        class SensingSubsystem {
            private:
                //key parts of sensing subsystem
                EnergyDetector<data_type> energy_detector;
                SpectrogramHandler<data_type> spectrogram_handler;

                //pointer to usrp device
                USRPHandler<data_type> * attacker_usrp_handler;

                //pointer to the attacking subsystem
                AttackingSubsystem<data_type> * attacking_subsystem;

                //configuration
                json config;

                //debug status
                bool debug;


            public:
                /**
                 * @brief Construct a new Sensing Subsystem object
                 * 
                 * @param config_data JSON configuration object for the attacker
                 * @param usrp_handler pointer to a USRP handler for the sensing subsystem to use
                 */
                SensingSubsystem(json config_data,
                    USRPHandler<data_type> * usrp_handler,
                    AttackingSubsystem<data_type> * subsystem_attacking):
                    config(config_data),
                    attacker_usrp_handler(usrp_handler),
                    attacking_subsystem(subsystem_attacking),
                    energy_detector(config_data),
                    spectrogram_handler(config_data){

                        if(check_config()){
                            //measure the relative noise power for the energy detector
                            init_debug_status();
                            mesaure_relative_noise_power();
                        }
                        else{
                            std::cerr << "SensingSubsystem: config did not pass check" << std::endl;
                        }
                }

                ~SensingSubsystem(){};

                /**
             * @brief Check the json config file to make sure all necessary parameters are included
             * 
             * @return true - JSON is all good and has required elements
             * @return false - JSON is missing certain fields
             */
            bool check_config(){
                bool config_good = true;

                //check debug status
                if(config["SensingSubsystemSettings"]["debug"].is_null()){
                    std::cerr << "SensingSubsystem::check_config: debug not specified" <<std::endl;
                    config_good = false;
                }

                return config_good;
            }

            /**
             * @brief Initialize the debug flag for the sensing subsystem
             * 
             */
            void init_debug_status(){
                debug = config["SensingSubsystemSettings"]["debug"].get<bool>();
            }

                /**
                 * @brief measure the relative noise power and configure the energy detector
                 * 
                 */
                void mesaure_relative_noise_power(void){

                    //stream the ambient signal
                    attacker_usrp_handler -> rx_stream_to_buffer(& energy_detector.noise_power_measureent_signal);

                    energy_detector.compute_relative_noise_power();

                    if(debug)
                    {
                        std::cout << "SensingSubsystem::measure_relative_noise_power: measurig relative noise power" << std::endl;
                        std::cout << "relative noise power: " << energy_detector.relative_noise_power << "dB" << std::endl;
                    }

                    return;
                }

                /**
                 * @brief Run the sensing subsystem. If the sensing subsystem is being run as part of a 
                 * series of experiments, the multiple_runs and run_number parameters can be used to
                 * specify the run number so that each parameter estimation has a unique result file.
                 * 
                 * @param multiple_runs set to true if the sensing subystem is being run multiple times
                 * @param run_number the number of the run (experiment) being performed
                 */
                void run(bool multiple_runs = false,
                    size_t run_number = 0){
                    
                    data_type detection_start_time_us;
                    double next_rx_sense_start_time = 
                        (attacker_usrp_handler -> usrp -> get_time_now().get_real_secs()) 
                        + spectrogram_handler.min_frame_periodicity_s;
                    //process the detected chirp
                    for (size_t i = 0; i < spectrogram_handler.max_frames_to_capture; i++)
                    {
                        //have USRP sample until it detects a chirp
                        attacker_usrp_handler -> rx_record_next_frame(& spectrogram_handler, 
                            & energy_detector,
                            next_rx_sense_start_time,
                            spectrogram_handler.max_waiting_time);
                        detection_start_time_us = energy_detector.get_detection_start_time_us();
                        spectrogram_handler.set_detection_start_time_us(detection_start_time_us);
                        energy_detector.save_chirp_detection_signal_to_buffer(& (spectrogram_handler.rx_buffer));
                        spectrogram_handler.process_received_signal();
                        energy_detector.reset_chirp_detector();

                        next_rx_sense_start_time = spectrogram_handler.get_last_frame_start_time_s() * 1e-6
                            + spectrogram_handler.min_frame_periodicity_s;
                        
                        //send to attacker if enabled
                        if ((attacking_subsystem -> enabled) && (i > attacking_subsystem -> attack_start_frame))
                        {   spectrogram_handler.set_attack_in_progress(true);
                            double next_frame_start_time = spectrogram_handler.get_next_frame_start_time_prediction_ms();
                            attacking_subsystem -> load_new_frame_start_time(next_frame_start_time);
                        }
                    }

                    //tell attacking subsystem that sensing is not longer being performed
                    if (attacking_subsystem -> enabled)
                    {
                        attacking_subsystem -> set_sensing_complete(true);
                    }
                    

                    if (debug)
                    {
                        std::cout << "SensingSubsystem::run: completed frame tracking" << std::endl;
                        save_sensing_subsystem_state();
                        spectrogram_handler.print_summary_of_estimated_parameters();
                    }
                    
                    spectrogram_handler.save_estimated_parameters_to_file(multiple_runs,run_number);
                }
                
                /**
                 * @brief For multiple runs, the flushes the USRP buffer to prevent any spill-over
                 * of samples from the previous run
                 * 
                 */
                void flush_usrp_buffer(void){
                    size_t spb = config["USRPSettings"]["RX"]["spb"].get<size_t>();
                    size_t num_rows = 10;
                    Buffer_2D<std::complex<data_type>> temp_buffer(num_rows,spb);
                    attacker_usrp_handler -> rx_stream_to_buffer(& temp_buffer);
                }
                
                /**
                 * @brief resets the sensing subsystem and the spectrogram_handler. Does not reset
                 * the energy detectors estimated noise power though
                 * 
                 */
                void reset(void){
                    spectrogram_handler.reset();
                    energy_detector.reset_chirp_detector();
                    flush_usrp_buffer();
                    //energy_detector.initialize_chirp_detection_params();
                    //mesaure_relative_noise_power();
                }

                /**
                 * @brief Save key sensing subsystem buffers to a file
                 * 
                 */
                void save_sensing_subsystem_state(void){
                    //save the hanning window to a file to confirm correctness
                    std::string folder_path = spectrogram_handler.save_file_path;
                    std::string path;
                    path = folder_path + "cpp_hanning_window.bin";
                    spectrogram_handler.hanning_window.set_write_file(path);
                    spectrogram_handler.hanning_window.save_to_file();
                    
                    //load and reshape the received signal to confirm correctness
                    path = folder_path + "cpp_reshaped_and_windowed_for_fft.bin";
                    spectrogram_handler.reshaped__and_windowed_signal_for_fft.set_write_file(path,true);
                    spectrogram_handler.reshaped__and_windowed_signal_for_fft.save_to_file();

                    //compute the fft to confirm correctness
                    path = folder_path + "cpp_generated_spectrogram.bin";
                    spectrogram_handler.generated_spectrogram.set_write_file(path,true);
                    spectrogram_handler.generated_spectrogram.save_to_file();

                    //detect the points in the spectrogram
                    path = folder_path + "cpp_spectrogram_point_vals.bin";
                    spectrogram_handler.spectrogram_points_values.set_write_file(path,true);
                    spectrogram_handler.spectrogram_points_values.save_to_file();

                    //detect the times and frequencies
                    path = folder_path + "cpp_detected_times.bin";
                    spectrogram_handler.detected_times.set_write_file(path,true);
                    spectrogram_handler.detected_times.save_to_file();
                    path = folder_path + "cpp_detected_frequencies.bin";
                    spectrogram_handler.detected_frequencies.set_write_file(path,true);
                    spectrogram_handler.detected_frequencies.save_to_file();

                    //compute the clusters
                    path = folder_path + "cpp_computed_clusters.bin";
                    spectrogram_handler.cluster_indicies.set_write_file(path,true);
                    spectrogram_handler.cluster_indicies.save_to_file();

                    //fit linear models
                    path = folder_path + "cpp_detected_slopes.bin";
                    spectrogram_handler.detected_slopes.set_write_file(path,true);
                    spectrogram_handler.detected_slopes.save_to_file();
                    path = folder_path + "cpp_detected_intercepts.bin";
                    spectrogram_handler.detected_intercepts.set_write_file(path,true);
                    spectrogram_handler.detected_intercepts.save_to_file();

                    //compute victim parameters
                    path = folder_path + "cpp_captured_frames.bin";
                    spectrogram_handler.captured_frames.set_write_file(path,true);
                    spectrogram_handler.captured_frames.save_to_file();

                }

        };
    }
#endif