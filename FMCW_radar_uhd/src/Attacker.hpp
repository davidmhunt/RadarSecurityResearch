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

    using json = nlohmann::json;
    using USRPHandler_namespace::USRPHandler;
    using Buffers::RADAR_Buffer;
    using Buffers::Buffer_1D;

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
                Buffer_1D<data_type> rx_buffer;

            public:

                ATTACKER(json config_data, bool run = false): config(config_data),usrp_handler(config_data){
                    
                    //run if specified
                    if (run){
                        std::cout << "Run Attack not yet enabled" <<std::endl;
                    }
                }

                                /**
                 * @brief initializes the rx buffer for USRP operation
                 * 
                 * @param desired_samples_per_buffer desired number of samples per buffer (defaults 
                 * to max for USRP device)
                 */
                void init_rx_buffer(size_t desired_samples_per_buffer = 0){
                    
                    //configure the write file
                    if (config["AttackSettings"]["rx_file_name"].is_null()){
                        std::cerr << "Attacker::init_rx_buffer: rx_file_name not specified in JSON";
                        return;
                    }
                    std::string rx_file = config["AttackSettings"]["rx_file_name"].get<std::string>();

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

                double get_rx_stream_duration(void){
                    //get the desired stream duration from the config file
                    if (config["AttackSettings"]["rx_time_ms"].is_null()){
                        std::cerr << "Attacker::get_rx_stream_duration: rx_time_ms not specified in JSON";
                        return 0;
                    }
                    double stream_time_ms = config["AttackSettings"]["rx_time_ms"].get<double>();
                    return stream_time_ms * 1e-3;

                }

                void run_attacker(void){
                    init_rx_buffer();
                    double stream_time_s = get_rx_stream_duration();
                    usrp_handler.rx_stream_to_file(& rx_buffer,stream_time_s);
                }
        };
    }

#endif