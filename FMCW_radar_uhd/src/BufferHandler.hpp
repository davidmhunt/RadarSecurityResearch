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

    namespace Buffers{

        /**
         * @brief Buffer Class - a parent class or a buffer
         * 
         * @tparam data_type - type of data that the buffer stores
         */
        template<typename data_type>
        class Buffer {
            //variables
                //read and write file paths
                private:
                    std::string read_file;
                    std::string write_file;
                //file streams
                    std::ifstream read_file_stream;
                    std::ofstream write_file_stream;
                //debug status
                    bool debug_status;
            //functions
                //file functions
                public:
                    //constructors and destructors
                    Buffer();
                    Buffer(bool debug);
                    virtual ~Buffer();

                    //managing and initializing file streams
                    void set_read_file(std::string file_path);
                    void set_write_file(std::string file_path);
                    void init_read_file_stream(void);
                    void init_write_file_stream(void);
                    void close_read_file_stream(void);
                    void close_write_file_stream(void);

                    //load data from a file
                    std::vector<data_type> load_data_from_read_file();

                //abstract functions
                    virtual void print_preview() = 0;
                    virtual void import_from_file() = 0;
                    virtual void save_to_file() = 0;
        }; // end of Buer class

        /**
         * @brief Buffer_2D - a 2D buffer that stores data in a buffer and allows easy read/write integration witha file
         * 
         * @tparam data_type: the type of data that the buffer stores
         */
        template<typename data_type>
        class Buffer_2D : public Buffer<data_type>{
            public:
            //variables
                //a vector to store things in
                std::vector<std::vector<data_type>> buffer;

                //keep track of the size of the buffer
                size_t num_rows;
                size_t num_cols;

                //keep track of how many excess samples are unused in the buffer
                size_t excess_samples; 
            //funcions
                Buffer_2D();
                Buffer_2D(bool debug);
                Buffer_2D(size_t num_rows, size_t num_cols,size_t excess_samples = 0, bool debug = false);
                virtual ~Buffer_2D();

                //functions for printing
                void print_1d_buffer_preview(std::vector<data_type> & buffer_to_print);

                //function to load data from a vector into a buffer
                void load_data_into_buffer(std::vector<data_type> & data_to_load, bool copy_until_buffer_full = true);
                
                //abstract functions
                    virtual void print_preview();
                    virtual void import_from_file();
                    virtual void save_to_file();

        };
    }
#endif