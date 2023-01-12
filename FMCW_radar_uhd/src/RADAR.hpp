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
                bool debug;

            //functions
            public:
                
                /**
                 * @brief Construct a new RADAR object - DEFAULT CONSTRUTOR, DOES NOT INITIALIZE USRP
                 * 
                 */
                RADAR():usrp_handler(){}
                
                /**
                 * @brief Construct a new RADAR object,
                 * loads the configuration, and initializes the usrp_handler 
                 * 
                 * @param config_data a json config object
                 * @param fmcw_config_data a json config object with the FMCW experiment configuration
                 * 
                 */
                RADAR(json config_data):
                    config(config_data),
                    usrp_handler(config_data),
                    radar_initialized(false){
                    
                    if (! check_config())
                    {
                        std::cerr << "RADAR: JSON config did not check out" << std::endl;
                    }
                    else
                    {
                        init_debug_status();
                    }
                }

                /**
                 * @brief Copy Constructor
                 * 
                 * @param rhs pointer to existing radar object
                 */
                RADAR(const RADAR & rhs) : config(rhs.config),
                                            usrp_handler(rhs.usrp_handler),
                                            tx_buffer(rhs.tx_buffer),
                                            rx_buffer(rhs.rx_buffer),
                                            samples_per_chirp(rhs.samples_per_chirp),
                                            stream_start_time(rhs.stream_start_time),
                                            frame_start_times(rhs.frame_start_times),
                                            num_frames(rhs.num_frames),
                                            frame_periodicity(rhs.frame_periodicity),
                                            radar_initialized(rhs.radar_initialized),
                                            debug(rhs.debug)
                                            {}

                /**
                 * @brief Assignment Operator
                 * 
                 * @param rhs existing RADAR object
                 * @return RADAR& 
                 */
                RADAR & operator=(const RADAR & rhs){
                    if (this != &rhs)
                    {
                        config = rhs.config;
                        usrp_handler = rhs.usrp_handler;
                        tx_buffer = rhs.tx_buffer;
                        rx_buffer = rhs.rx_buffer;
                        samples_per_chirp = rhs.samples_per_chirp;
                        stream_start_time = rhs.stream_start_time;
                        frame_start_times = rhs.frame_start_times;
                        num_frames = rhs.num_frames;
                        frame_periodicity = rhs.frame_periodicity;
                        radar_initialized = rhs.radar_initialized;
                        debug = rhs.debug;
                    }
                    return *this;
                }

                /**
                 * @brief Destroy the RADAR object
                 * 
                 */
                ~RADAR() {}

                /**
             * @brief Check the json config file to make sure all necessary parameters are included
             * 
             * @return true - JSON is all good and has required elements
             * @return false - JSON is missing certain fields
             */
            bool check_config(){
                bool config_good = true;
                //check for tx file name
                if (config["RadarSettings"]["tx_file_folder_path"].is_null()){
                    std::cerr << "Radar::check_config: tx_file_folder_path not specified in JSON";
                    config_good = false;
                }
                
                //check for rx file name
                if (config["RadarSettings"]["rx_file_folder_path"].is_null()){
                    std::cerr << "Radar::check_config: rx_file_folder_path not specified in JSON";
                    config_good = false;
                }

                //check for number of chirps
                if (config["RadarSettings"]["num_chirps"].is_null()){
                    std::cerr << "RADAR::check_config num chirps not specified in JSON";
                    config_good = false;
                }

                //verify that a stream start time has been specified
                if (config["USRPSettings"]["Multi-USRP"]["stream_start_time"].is_null()){
                    std::cerr << "RADAR::check_config stream_start_time not specified in JSON";
                    config_good = false;
                }

                //check for the number of frames
                if (config["RadarSettings"]["num_frames"].is_null()){
                    std::cerr << "RADAR::check_config num_frames not specified in JSON";
                    config_good = false;
                }

                //check for the frame periodicity
                if (config["RadarSettings"]["frame_periodicity_ms"].is_null()){
                    std::cerr << "RADAR::check_config frame_periodicity_ms not specified in JSON";
                    config_good = false;
                }

                //check for the frame periodicity
                if (config["RadarSettings"]["debug"].is_null()){
                    std::cerr << "RADAR::check_config debug not specified in JSON";
                    config_good = false;
                }
                
                return config_good;
            }

            void init_debug_status(){
                debug = config["RadarSettings"]["debug"].get<bool>();
            }
                
                /**
                 * @brief get the tx chirp from its file, set the samples_per_chirp_variable, and
                 *  return the chirp as a vector
                 * 
                 * @param multiple_runs (default false) on true, denotes that there are multiple runs 
                 * being performed for the radar
                 * 
                 * @param run_number (default 0) when multiple runs is true, loads the tx chirp file
                 * corresponding to that number of run
                 * 
                 * @return std::vector<data_type> a single tx chirp as a vector
                 */
                std::vector<std::complex<data_type>> get_tx_chirp(bool multiple_runs = false,
                    size_t run_number = 0){
                    //initialize a vector to store the chirp data
                    Buffer_1D<std::complex<data_type>> tx_chirp_buffer(debug);

                    std::string tx_file_path = config["RadarSettings"]["tx_file_folder_path"].get<std::string>();

                    std::string file_name;
                    if (multiple_runs){
                        file_name = "MATLAB_chirp_full_" + std::to_string(run_number) + ".bin";
                    }else{
                        file_name = "MATLAB_chirp_full.bin";
                    }

                    //create a buffer to load the chirp data into it
                    tx_chirp_buffer.set_read_file(tx_file_path + file_name,true);
                    tx_chirp_buffer.import_from_file();

                    if(debug){
                        std::cout << "Radar::get_tx_chirp: detected samples per chirp: " << tx_chirp_buffer.num_samples << std::endl;
                        //tx_chirp_buffer.print_preview();
                    }

                    samples_per_chirp = tx_chirp_buffer.num_samples;

                    return tx_chirp_buffer.buffer;
                }

                /**
                 * @brief initialize the Tx Buffer for USRP operations.
                 * 
                 * @param desired_num_chirps desired number of chirps to load into the tx buffer
                 * @param desired_samples_per_buffer desired samples per buffer (defaults to max 
                 * samples per buffer for USRP tx device)
                 * @param multiple_runs (default false) on true, denotes that there are multiple runs 
                 * being performed for the radar
                 * 
                 * @param run_number (default 0) when multiple runs is true, loads the tx chirp file
                 * corresponding to that number of run
                 * 
                 */
                void init_tx_buffer(
                    size_t desired_num_chirps,
                    size_t desired_samples_per_buffer = 0,
                    bool multiple_runs = false,
                    size_t run_number = 0){

                    //get the tx chirp buffer
                    std::vector<std::complex<data_type>> tx_chirp = get_tx_chirp(multiple_runs,run_number);

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

                    if(debug){
                        std::cout << "Radar::init_tx_buffer: Num Rows: " << tx_buffer.num_rows << " Excess Samples: " << tx_buffer.excess_samples << std::endl;
                    }

                    //load tx chirp into the tx buffer
                    tx_buffer.load_chirp_into_buffer(tx_chirp);
                }
                
                /**
                 * @brief initializes the rx buffer for USRP operation
                 * 
                 * @param desired_num_chirps desired number of chirps
                 * @param desired_samples_per_buffer desired number of samples per buffer (defaults 
                 * to max for USRP device)
                 * @param multiple_runs (default false) on true, denotes that there are multiple runs 
                 * being performed for the radar
                 * 
                 * @param run_number (default 0) when multiple runs is true, set the rx chirp file
                 * corresponding to that number of run
                 */
                void init_rx_buffer(size_t desired_num_chirps,
                    size_t desired_samples_per_buffer = 0,
                    bool multiple_runs = false,
                    size_t run_number = 0){
                    
                    std::string rx_file_path = config["RadarSettings"]["rx_file_folder_path"].get<std::string>();

                    std::string file_name;
                    if (multiple_runs){
                        file_name = "cpp_rx_data_" + std::to_string(run_number) + ".bin";
                    }else{
                        file_name = "cpp_rx_data.bin";
                    }
                    
                    //create a buffer to load the chirp data into it
                    rx_buffer.set_write_file(rx_file_path + file_name,true);

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

                    if(debug){
                        std::cout << "Radar::init_rx_buffer: Num Rows: " << rx_buffer.num_rows 
                            << " Excess Samples: " << rx_buffer.excess_samples << std::endl;
                    }
                }
                
                /**
                 * @brief initialize the tx and rx buffers for RADAR radar operation
                 * 
                 * @param multiple_runs (default false) on true, denotes that there are multiple runs 
                 * being performed for the radar
                 * 
                 * @param run_number (default 0) when multiple runs is true, loads the tx chirp file
                 * corresponding to that number of run
                 */
                void init_buffers_for_radar(bool multiple_runs = false,
                    size_t run_number = 0){
                    
                    size_t num_chirps = config["RadarSettings"]["num_chirps"].get<size_t>();
                    
                    init_tx_buffer(num_chirps,0,multiple_runs,run_number);
                    init_rx_buffer(num_chirps,0,multiple_runs,run_number);
                }          
                

                /**
                 * @brief Computes the frame start times in advance, and applies an offset if one is necessary
                 * 
                 */
                void init_frame_start_times(void){
                    
                    //set stream start time
                    stream_start_time = config["USRPSettings"]["Multi-USRP"]["stream_start_time"].get<double>();
                    

                    //set num_frames
                    num_frames = config["RadarSettings"]["num_frames"].get<size_t>();

                    //set frame_periodicity
                    frame_periodicity = config["RadarSettings"]["frame_periodicity_ms"].get<double>() * 1e-3;

                    //initialize the frame start times vector
                    frame_start_times = std::vector<uhd::time_spec_t>(num_frames);

                    if(debug){
                        std::cout << "RADAR::init_frame_start_times: computed start times: " << std::endl;
                    }
                    for (size_t i = 0; i < num_frames; i++)
                    {
                        frame_start_times[i] = uhd::time_spec_t(stream_start_time + 
                                        (frame_periodicity * static_cast<double>(i)));
                        
                        if(debug){
                            std::cout << frame_start_times[i].get_real_secs() << ", ";
                        }
                    }
                    std::cout << std::endl;
                    
                }

                /**
                 * @brief 
                 * 
                 * @param multiple_runs (default false) on true, denotes that there are multiple runs 
                 * being performed for the radar
                 * 
                 * @param run_number (default 0) when multiple runs is true, loads the tx chirp file
                 * corresponding to that number of run
                 */
                void initialize_radar(bool multiple_runs = false,
                size_t run_number = 0){
                    //initialize the buffers
                    init_buffers_for_radar(multiple_runs,run_number);

                    //compute the frame start times
                    init_frame_start_times();

                    radar_initialized = true;
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
                        std::cerr << "RADAR::run_radar: radar is not initialized, but run called" << std::endl;
                    }
                    else
                    {
                        //stream the frames
                        usrp_handler.stream_frames(frame_start_times,& tx_buffer,& rx_buffer);
                        radar_initialized = false;
                    }
                     
                }
        };
    }

#endif