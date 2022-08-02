#ifndef BUFFERHANDLER
#define BUFFERHANDLER
    //c standard library
    #include <cstdlib>
    #include <iostream>
    #include <fstream>
    #include <complex>
    #include <string>
    #include <vector>

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

                BufferHandler(
                    json config,
                    size_t rx_spb,
                    size_t tx_spb,
                    bool debug = false);
                ~BufferHandler();
                void load_tx_chirp(void);
                void init_tx_buffer(void);
                void init_rx_buffer(void);
                void init_BufferHandler(void);
                void save_rx_buffer_to_file(void);
                void print_1d_buffer_preview(
                    std::vector<std::complex<float>> & buffer_to_print);
                void print_2d_buffer_preview(
                    std::vector<std::vector<std::complex<float>>> & buffer_to_print);
        };
    }
#endif