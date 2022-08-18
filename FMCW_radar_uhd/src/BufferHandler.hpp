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
        //Buffer - parent class
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
        };
    }
#endif