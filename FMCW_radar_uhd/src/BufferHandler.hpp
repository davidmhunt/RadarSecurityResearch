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

            public:
                //buffers
                std::vector<std::vector<std::complex<float>>> tx_buffer;
                std::vector<std::vector<std::complex<float>>> rx_buffer;

                //buffer settings
                size_t samples_per_buff;
                size_t num_rows;
                size_t excess_samples;

                BufferHandler(std::string & tx_file_name, std::string & rx_file_name, json config);
                ~BufferHandler();
                std::vector<std::complex<float>> load_tx_chirp(void);
                void init_tx_buffer(void);
                void init_rx_buffer(void);
                void save_rx_buffer_to_file(void);
        };
    }
#endif