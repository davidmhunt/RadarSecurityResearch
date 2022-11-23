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

    using json = nlohmann::json;
    using USRPHandler_namespace::USRPHandler;
    using Buffers::Buffer_2D;
    using Buffers::Buffer_1D;
    using SensingSubsystem_namespace::SensingSubsystem;

    namespace ATTACKER_namespace{
        
        /**
         * @brief Class for the attacker
         * 
         * @tparam data_type the data_type of the data that the attacker uses
         */
        template<typename data_type>
        class ATTACKER{
            private:
                json config;
                USRPHandler<data_type> usrp_handler;
                SensingSubsystem<data_type> sensing_subsystem;

                Buffer_1D<std::complex<data_type>> rx_buffer;
                Buffer_2D<std::complex<data_type>> tx_buffer;

                //timing variables
                double stream_duration_s;

                //status flags
                bool attacker_initialized;

                //attacker parameters
                    //Variables to keep track of frame start times
                
                    //timing arguments
                    double stream_start_time;
                    std::vector<uhd::time_spec_t> frame_start_times;
                    
                    //FMCW arguments
                    size_t num_frames;
                    double frame_periodicity;

            public:

                /**
                 * @brief Construct a new ATTACKER object,
                 * loads the configuration, and initializes the usrp_handler 
                 * 
                 * @param config_data a json config object
                 * @param initialize on true (default) automatically initializes the 
                 * buffers and rx stream duration.
                 * @param run on false (default) does not run attacker implementation,
                 * on true runs the attacker implementation
                 */
                ATTACKER(json config_data, bool initialize = true, bool run = false):
                    config(config_data),
                    usrp_handler(config_data),
                    sensing_subsystem(config_data, & usrp_handler),
                    attacker_initialized(initialize){
                    
                    if (attacker_initialized)
                    {
                        //not implemented
                    }
                    
                    //run if specified
                    if (run){
                        //run the sensing subsystem
                        sensing_subsystem.run();
                    }
                }

                                /**
                 * @brief NO LONGER USED initializes the rx buffer for USRP operation
                 * 
                 * @param desired_samples_per_buffer desired number of samples per buffer (defaults 
                 * to max for USRP device)
                 */
                void init_rx_buffer(size_t desired_samples_per_buffer = 0){
                    
                    //configure the write file
                    if (config["SensingSubsystemSettings"]["rx_file_name"].is_null()){
                        std::cerr << "Attacker::init_rx_buffer: rx_file_name not specified in JSON";
                        return;
                    }
                    std::string rx_file = config["SensingSubsystemSettings"]["rx_file_name"].get<std::string>();

                    //create a buffer to load the chirp data into it
                    rx_buffer.set_write_file(rx_file,true);

                    //specify samples per buffer behavior
                    size_t samples_per_buffer;

                    if (desired_samples_per_buffer == 0)
                    {
                        samples_per_buffer = usrp_handler.rx_samples_per_buffer;
                    }
                    else
                    {
                        samples_per_buffer = desired_samples_per_buffer;
                    }

                    rx_buffer.set_buffer_size(samples_per_buffer);

                    std::cout << "Attacker::init_rx_buffer: Num Samples: " 
                        << rx_buffer.num_samples << std::endl;

                    return;
                }

                /**
                 * @brief NO LONGER USED Get the duration of the rx stream (i.e: how long to listen to the victim) from the JSON config file
                 * 
                 * @return double the stream time in ms
                 */
                double get_rx_stream_duration(void){
                    //get the desired stream duration from the config file
                    if (config["SensingSubsystemSettings"]["rx_time_ms"].is_null()){
                        std::cerr << "Attacker::get_rx_stream_duration: rx_time_ms not specified in JSON";
                        return 0;
                    }
                    double stream_time_ms = config["SensingSubsystemSettings"]["rx_time_ms"].get<double>();
                    return stream_time_ms * 1e-3;

                }


                /**
                 * @brief Computes the frame start times in advance, and applies an offset if one is necessary
                 * 
                 */
                void init_attack_frame_start_times(void){
                    //set stream start time
                    if(config["USRPSettings"]["Multi-USRP"]["stream_start_time"].is_null() == false){
                        stream_start_time = config["USRPSettings"]["Multi-USRP"]["stream_start_time"].get<double>();
                    }
                    else{
                        std::cerr << "Attacker::init_attack_frame_start_times: couldn't find stream start time in JSON" <<std::endl;
                    }

                    //set num_frames
                    if(config["AttackSubsystemSettings"]["num_frames"].is_null() == false){
                        num_frames = config["AttackSubsystemSettings"]["num_frames"].get<size_t>();
                    }
                    else{
                        std::cerr << "Attacker::init_attack_frame_start_times: couldn't find num_frames in JSON" <<std::endl;
                    }

                    //set frame_periodicity
                    if(config["AttackSubsystemSettings"]["frame_periodicity_ms"].is_null() == false){
                        frame_periodicity = config["AttackSubsystemSettings"]["frame_periodicity_ms"].get<double>() * 1e-3;
                    }
                    else{
                        std::cerr << "Attacker::init_attack_frame_start_times: couldn't find frame_periodicity_ms in JSON" <<std::endl;
                    }

                    //initialize the frame start times vector
                    frame_start_times = std::vector<uhd::time_spec_t>(num_frames);
                    std::cout << "Attacker::init_attack_frame_start_times: computed start times: " << std::endl;
                    for (size_t i = 0; i < num_frames; i++)
                    {
                        frame_start_times[i] = uhd::time_spec_t(stream_start_time + 
                                        (frame_periodicity * static_cast<double>(i)));
                        std::cout << frame_start_times[i].get_real_secs() << ", ";
                    }
                    std::cout << std::endl;
                }


                void run_attacker(void){
                    usrp_handler.reset_usrp_clock();
                    if (! attacker_initialized)
                    {
                        //not implemented
                    }
                    
                    sensing_subsystem.run();
                }
        };
    }

#endif