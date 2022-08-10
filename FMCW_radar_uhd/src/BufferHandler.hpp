#ifndef BUFFERHANDLER
#define BUFFERHANDLER
    //c standard library
    #include <cstdlib>
    #include <iostream>
    #include <fstream>
    #include <complex>
    #include <string>
    #include <vector>
    #include <tuple>

    //includes for JSON editing
    #include <nlohmann/json.hpp>

    using json = nlohmann::json;

    namespace BufferHandler_namespace {
        class BufferHandler{

            private:
                //file streams
                std::string tx_file;
                std::string rx_file;
                std::ofstream rx_file_stream;
                std::ifstream tx_file_stream;

                //FMCW radar settings
                size_t samples_per_chirp;
                size_t num_chirps;

                //debugging variable
                bool debug_status;

            public:
                //buffers
                std::vector<std::complex<float>> tx_chirp_buffer;
                std::vector<std::vector<std::complex<float>>> tx_buffer;
                std::vector<std::vector<std::complex<float>>> rx_buffer;

                //buffer settings
                size_t rx_samples_per_buff;
                size_t rx_num_rows;
                size_t rx_excess_samples;

                size_t tx_samples_per_buff;
                size_t tx_num_rows;
                size_t tx_excess_samples;

                //CLASS FUNCTIONS
                    //configuration
                BufferHandler(
                    json config,
                    size_t rx_spb,
                    size_t tx_spb);
                ~BufferHandler();
                void configure_debug(json &config);
                void load_tx_chirp(void);

                    //wrapper functions to make FMCW operation simplar
                void init_tx_buffer(void);
                void init_rx_buffer(void);
                void init_BufferHandler(void);
                void save_rx_buffer_to_file(void);

                    //for printing buffers
                void print_1d_buffer_preview(
                    std::vector<std::complex<float>> & buffer_to_print);
                void print_2d_buffer_preview(
                    std::vector<std::vector<std::complex<float>>> & buffer_to_print);

                    //core functions
                template<typename data_type>
                std::vector<data_type> load_data_from_file(std::string file);
                
                std::tuple<size_t,size_t> compute_usrp_buffer_settings(
                    size_t desired_samples_per_buff,
                    size_t required_samples_per_chirp,
                    size_t desired_num_chirps);
                
                template<typename data_type>
                void load_data_into_usrp_buffer(
                    std::vector<data_type> & data,
                    std::vector<std::vector<data_type>> & usrp_buffer,
                    size_t excess_samples
                );

                template<typename data_type>
                void save_usrp_buffer_to_file(
                    std::vector<std::vector<data_type>>  & usrp_buffer,
                    std::ofstream & out_file_stream,
                    size_t & excess_samples
                );
                
                /*
                    //DEBUGGING: read/write buffer from/to a file
                void load_rx_buffer_from_file(std::string existing_rx_buffer_file);

                template<typename T>
                void write_1d_buffer_to_file(std::string write_file, std::vector<T> &vector_to_write);
                */
        };  
    }
#endif