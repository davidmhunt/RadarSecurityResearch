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

                        //measure the relative noise power for the energy detector
                        mesaure_relative_noise_power();
                }

                ~SensingSubsystem(){};

                /**
                 * @brief measure the relative noise power and configure the energy detector
                 * 
                 */
                void mesaure_relative_noise_power(void){
                    std::cout << "SensingSubsystem::measure_relative_noise_power: measurig relative noise power" << std::endl;

                    //stream the ambient signal
                    attacker_usrp_handler -> rx_stream_to_buffer(& energy_detector.noise_power_measureent_signal);

                    energy_detector.compute_relative_noise_power();

                    std::cout << "relative noise power: " << energy_detector.relative_noise_power << "dB" << std::endl;
                    return;
                }

                /**
                 * @brief Run the sensing subsystem
                 * 
                 */
                void run(void){
                    
                    data_type detection_start_time_us;
                    double next_rx_sense_start_time = 0.0;
                    //process the detected chirp
                    for (size_t i = 0; i < spectrogram_handler.max_frames_to_capture; i++)
                    {
                        //have USRP sample until it detects a chirp
                        attacker_usrp_handler -> rx_record_next_frame(& spectrogram_handler, 
                            & energy_detector,
                            next_rx_sense_start_time);
                        detection_start_time_us = energy_detector.get_detection_start_time_us();
                        spectrogram_handler.set_detection_start_time_us(detection_start_time_us);
                        energy_detector.save_chirp_detection_signal_to_buffer(& (spectrogram_handler.rx_buffer));
                        spectrogram_handler.process_received_signal();
                        energy_detector.reset_chirp_detector();

                        next_rx_sense_start_time = spectrogram_handler.get_last_frame_start_time_s() * 1e-6
                            + spectrogram_handler.min_frame_periodicity_s;
                        
                        if ((attacking_subsystem -> enabled) && (i > attacking_subsystem -> attack_start_frame))
                        {
                            double next_frame_start_time = spectrogram_handler.get_next_frame_start_time_prediction_ms();
                            attacking_subsystem -> compute_frame_start_times(next_frame_start_time);
                            attacking_subsystem -> run_attack_subsystem();
                            break;
                        }
                        
                    }

                    save_sensing_subsystem_state();
                    std::cout << "SensingSubsystem::run: completed frame tracking" << std::endl;
                    spectrogram_handler.print_summary_of_estimated_parameters();
                    spectrogram_handler.save_estimated_parameters_to_file();
                }

                /**
                 * @brief Save key sensing subsystem buffers to a file
                 * 
                 */
                void save_sensing_subsystem_state(void){
                    //save the hanning window to a file to confirm correctness
                    std::string path;
                    path = "/home/david/Documents/MATLAB_generated/cpp_hanning_window.bin";
                    spectrogram_handler.hanning_window.set_write_file(path);
                    spectrogram_handler.hanning_window.save_to_file();
                    
                    //load and reshape the received signal to confirm correctness
                    path = "/home/david/Documents/MATLAB_generated/cpp_reshaped_and_windowed_for_fft.bin";
                    spectrogram_handler.reshaped__and_windowed_signal_for_fft.set_write_file(path,true);
                    spectrogram_handler.reshaped__and_windowed_signal_for_fft.save_to_file();

                    //compute the fft to confirm correctness
                    path = "/home/david/Documents/MATLAB_generated/cpp_generated_spectrogram.bin";
                    spectrogram_handler.generated_spectrogram.set_write_file(path,true);
                    spectrogram_handler.generated_spectrogram.save_to_file();

                    //detect the points in the spectrogram
                    path = "/home/david/Documents/MATLAB_generated/cpp_spectrogram_point_vals.bin";
                    spectrogram_handler.spectrogram_points_values.set_write_file(path,true);
                    spectrogram_handler.spectrogram_points_values.save_to_file();

                    //detect the times and frequencies
                    path = "/home/david/Documents/MATLAB_generated/cpp_detected_times.bin";
                    spectrogram_handler.detected_times.set_write_file(path,true);
                    spectrogram_handler.detected_times.save_to_file();
                    path = "/home/david/Documents/MATLAB_generated/cpp_detected_frequencies.bin";
                    spectrogram_handler.detected_frequencies.set_write_file(path,true);
                    spectrogram_handler.detected_frequencies.save_to_file();

                    //compute the clusters
                    path = "/home/david/Documents/MATLAB_generated/cpp_computed_clusters.bin";
                    spectrogram_handler.cluster_indicies.set_write_file(path,true);
                    spectrogram_handler.cluster_indicies.save_to_file();

                    //fit linear models
                    path = "/home/david/Documents/MATLAB_generated/cpp_detected_slopes.bin";
                    spectrogram_handler.detected_slopes.set_write_file(path,true);
                    spectrogram_handler.detected_slopes.save_to_file();
                    path = "/home/david/Documents/MATLAB_generated/cpp_detected_intercepts.bin";
                    spectrogram_handler.detected_intercepts.set_write_file(path,true);
                    spectrogram_handler.detected_intercepts.save_to_file();

                    //compute victim parameters
                    path = "/home/david/Documents/MATLAB_generated/cpp_captured_frames.bin";
                    spectrogram_handler.captured_frames.set_write_file(path,true);
                    spectrogram_handler.captured_frames.save_to_file();

                }

        };
    }
#endif