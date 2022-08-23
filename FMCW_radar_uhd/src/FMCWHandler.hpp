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
                size_t samples_per_chirp;
            //functions
            public:
                
                /**
                 * @brief Construct a new FMCW object,
                 * initializes the usrp_handler, initializes the buffers, and runs the experiment
                 * 
                 * @param config_data a json config object
                 */
                FMCW(json config_data): config(config_data), usrp_handler(config_data){
                    
                    //initialize the buffers
                    init_buffers_for_radar();

                    //load the tx and rx buffers into the USRP handler
                    usrp_handler.load_Buffers(& tx_buffer,& rx_buffer);

                    usrp_handler.stream_frames();
                }

                /**
                 * @brief Destroy the FMCW object
                 * 
                 */
                ~FMCW() {}

                /**
                 * @brief get the tx chirp from its file, set the samples_per_chirp_variable, and
                 *  return the chirp as a vector
                 * 
                 * @return std::vector<data_type> a single tx chirp as a vector
                 */
                std::vector<data_type> get_tx_chirp(void){
                    //initialize a vector to store the chirp data
                    Buffer_1D<data_type> tx_chirp_buffer;

                    if (config["FMCWSettings"]["tx_file_name"].is_null()){
                        std::cerr << "FMCWHandler::get_tx_chirp: tx_file_name not specified in JSON";
                        return tx_chirp_buffer.buffer;
                    }
                    std::string tx_file = config["FMCWSettings"]["tx_file_name"].get<std::string>();

                    //create a buffer to load the chirp data into it
                    tx_chirp_buffer.set_read_file(tx_file,true);
                    tx_chirp_buffer.import_from_file();

                    std::cout << "FMCWHandler::get_tx_chirp: detected samples per chirp: " << tx_chirp_buffer.num_samples << std::endl;
                    //tx_chirp_buffer.print_preview();

                    samples_per_chirp = tx_chirp_buffer.num_samples;

                    return tx_chirp_buffer.buffer;
                }

                /**
                 * @brief initialize the Tx Buffer for USRP operations.
                 * 
                 * @param desired_num_chirps desired number of chirps to load into the tx buffer
                 * @param desired_samples_per_buffer desired samples per buffer (defaults to max 
                 * samples per buffer for USRP tx device)
                 */
                void init_tx_buffer(
                    size_t desired_num_chirps,
                    size_t desired_samples_per_buffer = 0){

                    //get the tx chirp buffer
                    std::vector<data_type> tx_chirp = get_tx_chirp();

                    //specify samples per buffer behavior
                    size_t samples_per_buffer;

                    if (desired_samples_per_buffer == 0)
                    {
                        samples_per_buffer = usrp_handler.tx_samples_per_buffer;
                    }
                    else
                    {
                        samples_per_buffer = desired_samples_per_buffer;
                    }
                    
                    
                    //configure the tx_buffer to be the correct size
                    tx_buffer.configure_fmcw_buffer(
                        samples_per_buffer,
                        samples_per_chirp,
                        desired_num_chirps
                    );

                    std::cout << "FMCWHandler::init_tx_buffer: Num Rows: " << tx_buffer.num_rows << " Excess Samples: " << tx_buffer.excess_samples << std::endl;
                    
                    //load tx chirp into the tx buffer
                    tx_buffer.load_chirp_into_buffer(tx_chirp);
                }
                
                /**
                 * @brief initializes the rx buffer for USRP operation
                 * 
                 * @param desired_num_chirps desired number of chirps
                 * @param desired_samples_per_buffer desired number of samples per buffer (defaults 
                 * to max for USRP device)
                 */
                void init_rx_buffer(size_t desired_num_chirps,
                    size_t desired_samples_per_buffer = 0){
                    
                    //configure the write file
                    if (config["FMCWSettings"]["rx_file_name"].is_null()){
                        std::cerr << "FMCWHandler::init_rx_buffer: rx_file_name not specified in JSON";
                        return;
                    }
                    std::string rx_file = config["FMCWSettings"]["rx_file_name"].get<std::string>();

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

                    rx_buffer.configure_fmcw_buffer(
                        samples_per_buffer,
                        samples_per_chirp,
                        desired_num_chirps
                    );

                    std::cout << "FMCWHandler::init_rx_buffer: Num Rows: " << rx_buffer.num_rows 
                        << " Excess Samples: " << rx_buffer.excess_samples << std::endl;

                }
                
                /**
                 * @brief initialize the tx and rx buffers for FMCW radar operation
                 * 
                 */
                void init_buffers_for_radar(void){
                    
                    //get the number of chirps in each frame
                    if (config["FMCWSettings"]["num_chirps"].is_null()){
                        std::cerr << "FMCW: num chirps not specified in JSON";
                        return;
                    }
                    size_t num_chirps = config["FMCWSettings"]["num_chirps"].get<size_t>();
                    
                    init_tx_buffer(num_chirps);
                    init_rx_buffer(num_chirps);
                    //std::vector<data_type> tx_chirp = get_tx_chirp();
                }

                void init_buffers_for_synchronization(void);           
                
                //functions to run streaming
                void run_FMCW(void);
        };
    }

#endif