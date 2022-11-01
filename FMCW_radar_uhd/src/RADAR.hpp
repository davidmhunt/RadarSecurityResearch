#ifndef RADARCLASS
#define RADARCLASS

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

    namespace RADAR_namespace{

        /**
         * @brief RADAR Class - class to run all RADAR experiments out of
         * 
         * @tparam data_type 
         */
        template<typename data_type>
        class RADAR {
            //variables
            private:
                json config;
                USRPHandler<data_type> usrp_handler;
                RADAR_Buffer<data_type> tx_buffer;
                RADAR_Buffer<data_type> rx_buffer;
                size_t samples_per_chirp;

                //Variables to keep track of frame start times
                
                //timing arguments
                double stream_start_time;
                std::vector<uhd::time_spec_t> frame_start_times;
                
                //FMCW arguments
                size_t num_frames;
                double frame_periodicity;

                //status flags
                bool radar_initialized;

            //functions
            public:
                
                /**
                 * @brief Construct a new RADAR object,
                 * loads the configuration, and initializes the usrp_handler 
                 * 
                 * @param config_data a json config object
                 * @param initialize on true (default) automatically initializes the 
                 * buffers and computes the frame times for radar operation.
                 * @param run on false (default) does not run radar implementation,
                 * on true runs the radar implementation
                 */
                RADAR(json config_data, bool initialize = true, bool run = false):
                    config(config_data),
                    usrp_handler(config_data),
                    radar_initialized(initialize){
                    
                    //initialize the radar (if specified)
                    if (radar_initialized)
                    {
                        //initialize the buffers
                        init_buffers_for_radar();

                        //compute the frame start times
                        init_frame_start_times();
                    }
                    
                    
                    //run if specified
                    if (run){
                        run_RADAR();
                    }
                }

                /**
                 * @brief Destroy the RADAR object
                 * 
                 */
                ~RADAR() {}

                /**
                 * @brief get the tx chirp from its file, set the samples_per_chirp_variable, and
                 *  return the chirp as a vector
                 * 
                 * @return std::vector<data_type> a single tx chirp as a vector
                 */
                std::vector<std::complex<data_type>> get_tx_chirp(void){
                    //initialize a vector to store the chirp data
                    Buffer_1D<std::complex<data_type>> tx_chirp_buffer;

                    if (config["RadarSettings"]["tx_file_name"].is_null()){
                        std::cerr << "Radar::get_tx_chirp: tx_file_name not specified in JSON";
                        return tx_chirp_buffer.buffer;
                    }
                    std::string tx_file = config["RadarSettings"]["tx_file_name"].get<std::string>();

                    //create a buffer to load the chirp data into it
                    tx_chirp_buffer.set_read_file(tx_file,true);
                    tx_chirp_buffer.import_from_file();

                    std::cout << "Radar::get_tx_chirp: detected samples per chirp: " << tx_chirp_buffer.num_samples << std::endl;
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
                    std::vector<std::complex<data_type>> tx_chirp = get_tx_chirp();

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

                    std::cout << "Radar::init_tx_buffer: Num Rows: " << tx_buffer.num_rows << " Excess Samples: " << tx_buffer.excess_samples << std::endl;
                    
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
                    if (config["RadarSettings"]["rx_file_name"].is_null()){
                        std::cerr << "Radar::init_rx_buffer: rx_file_name not specified in JSON";
                        return;
                    }
                    std::string rx_file = config["RadarSettings"]["rx_file_name"].get<std::string>();

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

                    std::cout << "Radar::init_rx_buffer: Num Rows: " << rx_buffer.num_rows 
                        << " Excess Samples: " << rx_buffer.excess_samples << std::endl;

                }
                
                /**
                 * @brief initialize the tx and rx buffers for RADAR radar operation
                 * 
                 */
                void init_buffers_for_radar(void){
                    
                    //get the number of chirps in each frame
                    if (config["RadarSettings"]["num_chirps"].is_null()){
                        std::cerr << "RADAR: num chirps not specified in JSON";
                        return;
                    }
                    size_t num_chirps = config["RadarSettings"]["num_chirps"].get<size_t>();
                    
                    init_tx_buffer(num_chirps);
                    init_rx_buffer(num_chirps);
                    //std::vector<std::complex<data_type>> tx_chirp = get_tx_chirp();
                }          
                

                /**
                 * @brief Computes the frame start times in advance, and applies an offset if one is necessary
                 * 
                 */
                void init_frame_start_times(void){
                    //set stream start time
                    if(config["USRPSettings"]["Multi-USRP"]["stream_start_time"].is_null() == false){
                        stream_start_time = config["USRPSettings"]["Multi-USRP"]["stream_start_time"].get<double>();
                    }
                    else{
                        std::cerr << "RADAR::init_frame_start_times: couldn't find stream start time in JSON" <<std::endl;
                    }

                    //set num_frames
                    if(config["RadarSettings"]["num_frames"].is_null() == false){
                        num_frames = config["RadarSettings"]["num_frames"].get<size_t>();
                    }
                    else{
                        std::cerr << "RADAR::init_frame_start_times: couldn't find num_frames in JSON" <<std::endl;
                    }

                    //set frame_periodicity
                    if(config["RadarSettings"]["frame_periodicity_ms"].is_null() == false){
                        frame_periodicity = config["RadarSettings"]["frame_periodicity_ms"].get<double>() * 1e-3;
                    }
                    else{
                        std::cerr << "RADAR::init_frame_start_times: couldn't find frame_periodicity_ms in JSON" <<std::endl;
                    }

                    //initialize the frame start times vector
                    frame_start_times = std::vector<uhd::time_spec_t>(num_frames);
                    std::cout << "RADAR::init_frame_start_times: computed start times: " << std::endl;
                    for (size_t i = 0; i < num_frames; i++)
                    {
                        frame_start_times[i] = uhd::time_spec_t(stream_start_time + 
                                        (frame_periodicity * static_cast<double>(i)));
                        std::cout << frame_start_times[i].get_real_secs() << ", ";
                    }
                    std::cout << std::endl;
                }

                
                /**
                 * @brief Configures the buffers for radar operation,
                 * loads the buffers into the USRP device, and runs the radar
                 * for the desired number of frames
                 * 
                 */
                void run_RADAR(void){

                    if (! radar_initialized)
                    {
                        //initialize the buffers
                        init_buffers_for_radar();

                        //compute the frame start times
                        init_frame_start_times();
                    }

                    //stream the frames
                    usrp_handler.stream_frames(frame_start_times,& tx_buffer,& rx_buffer); 
                }
        };
    }

#endif