#ifndef FMCWHANDLER
#define FMCWHANDLER

    //c standard library
    #include <iostream>
    #include <cstdlib>
    #include <string>
    #include <complex>
    #include <csignal>

    //uhd specific libraries
    #include <uhd/usrp/multi_usrp.hpp>

    //JSON class
    #include <nlohmann/json.hpp>

    //source libraries
    #include "JSONHandler.hpp"
    #include "USRPHandler.hpp"
    #include "BufferHandler.hpp"

    using json = nlohmann::json;
    using USRPHandler_namespace::USRPHandler;
    using Buffers::FMCW_Buffer;
    using Buffers::Buffer_1D;

    namespace FMCW_namespace{

        /**
         * @brief FMCW Class - class to run all FMCW experiments out of
         * 
         * @tparam data_type 
         */
        template<typename data_type>
        class FMCW {
            //variables
            private:
                json config;
                USRPHandler usrp_handler;
                FMCW_Buffer<data_type> tx_buffer;
                FMCW_Buffer<data_type> rx_buffer;
            //functions
            public:
                
                /**
                 * @brief Construct a new FMCW object,
                 * initializes the usrp_handler, initializes the buffers, and runs the experiment
                 * 
                 * @param config_data a json config object
                 */
                FMCW(json config_data): config(config_data), usrp_handler(config_data){
                    init_buffers_for_radar();
                }

                /**
                 * @brief Destroy the FMCW object
                 * 
                 */
                ~FMCW() {}

                /**
                 * @brief get the tx chirp from its file and return it as a vector
                 * 
                 * @return std::vector<data_type> a single tx chirp as a vector
                 */
                std::vector<data_type> get_tx_chirp(void){
                    //initialize a vector to store the chirp data
                    Buffer_1D<data_type> tx_chirp_buffer;

                    if (config["FMCWSettings"]["tx_file_name"].is_null()){
                        std::cerr << "FMCW: tx_file_name not specified in JSON";
                        return tx_chirp_buffer.buffer;
                    }
                    std::string tx_file = config["FMCWSettings"]["tx_file_name"].get<std::string>();

                    //create a buffer to load the chirp data into it
                    tx_chirp_buffer.set_read_file(tx_file,true);
                    tx_chirp_buffer.import_from_file();
                    tx_chirp_buffer.close_read_file_stream();

                    tx_chirp_buffer.print_preview();
                    return tx_chirp_buffer.buffer;
                }
                
                /**
                 * @brief initialize the tx and rx buffers for FMCW radar operation
                 * 
                 */
                void init_buffers_for_radar(void){
                    std::vector<data_type> tx_chirp = get_tx_chirp();
                }
                void init_buffers_for_synchronization(void);                
                
                //functions to run streaming
                void run_FMCW(void);
        };
    }

#endif