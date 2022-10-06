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
            public:
                std::string read_file;
                std::string write_file;
            //file streams
                std::ifstream read_file_stream;
                std::ofstream write_file_stream;
            //debug status
                bool debug_status;

            //functions
            public:
                /**
                 * @brief Default constructor a new Buffer object
                 * 
                 */
                Buffer(): debug_status(false) {}

                /**
                 * @brief Construct a new Buffer object, alternative constructor to specify debug status
                 * 
                 * @param debug the desired debug status
                 */
                Buffer(bool debug) : debug_status(debug) {}

                /**
                 * @brief Destroy the Buffer object
                 * 
                 */
                virtual ~Buffer(){
                    if (read_file_stream.is_open()){
                        read_file_stream.close();
                    }
                    if (write_file_stream.is_open()){
                        write_file_stream.close();
                    }
                }

                //managing and initializing file streams

                /**
                 * @brief set the read file path
                 * 
                 * @param file_path the file name to be read
                 * @param init_stream (on true) also initializes the read file stream,
                 * (on false) only sets the read file name
                 */
                void set_read_file(std::string file_path, bool init_stream = true){
                    read_file = file_path;
                    if (init_stream){
                        init_read_file_stream();
                    }
                }

                /**
                 * @brief Set the write file path
                 * 
                 * @param file_path the file name to be written to
                 * @param init_stream (on true) also initializes the write file stream,
                 * (on false) only sets the write file
                 */
                void set_write_file(std::string file_path, bool init_stream = true){
                    write_file = file_path;
                    if (init_stream){
                        init_write_file_stream();
                    }
                }
                
                /**
                 * @brief open the read file in preparation for streaming
                 * 
                 */
                void init_read_file_stream(void){
                    if(read_file.empty()){
                        std::cerr << "Buffer::init_read_file_stream: no read_file name" <<std::endl;
                    }
                    else{
                        read_file_stream.open(read_file.c_str(), std::ios::in | std::ios::binary);
                        if(read_file_stream.is_open()){
                            std::cout << "Buffer::init_read_file_stream: read file opened successfully" <<std::endl;
                        }
                        else{
                            std::cerr << "BufferHandler::failed to open read file\n";
                        }
                    }
                }

                /**
                 * @brief open the write file in preparation for streaming
                 * 
                 */
                void init_write_file_stream(void){
                    //open the write file
                    if(write_file.empty()){
                        std::cerr << "Buffer::init_write_file_stream: no write_file name" <<std::endl;
                    }
                    else{
                        write_file_stream.open(write_file.c_str(), std::ios::out | std::ios::binary);
                        if(write_file_stream.is_open()){
                            std::cout << "Buffer::init_write_file_stream: write file opened successfully" <<std::endl;
                        }
                        else{
                            std::cerr << "BufferHandler::failed to open write file\n";
                        }
                    }
                }

                /**
                 * @brief close the read file stream
                 * 
                 */
                void close_read_file_stream(void){
                    if(read_file_stream.is_open()){
                        read_file_stream.close();
                        std::cout << "Buffer::close_read_file_stream: read file stream closed" << std::endl;
                    }
                    else{
                        std::cerr << "Buffer::close_read_file_stream: close_read_file_stream"  << 
                            "called but read file already closed" << std::endl;
                    }
                }

                /**
                 * @brief close the write file stream
                 * 
                 */
                void close_write_file_stream(void){
                    if(write_file_stream.is_open()){
                        write_file_stream.close();
                        std::cout << "Buffer::close_write_file_stream: write file stream closed" << std::endl;
                    }
                    else{
                        std::cerr << "Buffer::close_write_file_stream: close_write_file_stream"  << 
                            "called but write file already closed" << std::endl;
                    }
                }

                /**
                 * @brief Read the data stored in the read_file_stream (must already be opened), 
                 * and return a vector with the data. 
                 * NOTE: The read file stream is closed after data has been loaded
                 * 
                 * @return std::vector<data_type>: a vector with the data stored in the read file
                 */
                std::vector<data_type> load_data_from_read_file(void){
                    //initialize the return vector
                    std::vector<data_type> data_vector;
                    
                    //if the read file is open, read it into the data_vector
                    if (read_file_stream.is_open()){

                        //get the size of the file to be read
                        std::streampos size;
                        read_file_stream.seekg (0,std::ios::end);
                        size = read_file_stream.tellg();

                        //determine the number of samples in the file
                        size_t detected_samples = size / sizeof(data_type);
                        std::cout << "Buffer::load_data_from_read_file: detected samples: " << detected_samples << std::endl;

                        //define the vector
                        data_vector = std::vector<data_type>(detected_samples);

                        //read the file
                        read_file_stream.seekg(0,std::ios::beg);
                        read_file_stream.read((char*) &data_vector.front(), data_vector.size() * sizeof(data_type));
                        read_file_stream.close();  
                    }
                    else{
                        std::cerr << "Buffer::load_data_from_read_file: read_file_stream is not open\n";
                    }
                    return data_vector;
                }

            //abstract functions
                virtual void print_preview(void) = 0;
                virtual void import_from_file(void) = 0;
                virtual void save_to_file() = 0;
        }; // end of Buffer class

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

                /**
                 * @brief Construct a new Buffer_2D object
                 * 
                 */
                Buffer_2D(): Buffer<data_type>(){}

                /**
                 * @brief Construct a new Buffer_2D object
                 * 
                 * @param debug the specified debug setting
                 */
                Buffer_2D(bool debug): Buffer<data_type>(debug){}

                /**
                 * @brief Construct a new Buffer_2D object
                 * 
                 * @param rows the number of rows to be in the buffer
                 * @param cols the number of columns to be in the buffer
                 * @param excess the number of excess/unused samples in the buffer
                 * @param debug the specified debug setting
                 */
                Buffer_2D(size_t rows, size_t cols,size_t excess = 0, bool debug = false)
                        : Buffer<data_type>(debug),buffer(rows,std::vector<data_type>(cols)),
                                        num_rows(rows),num_cols(cols){}

                /**
                 * @brief Destructor for Buffer_2D object
                 * 
                 */
                virtual ~Buffer_2D() {}

                /**
                 * @brief prints out a 1d buffer
                 * 
                 * @param buffer_to_print a reference to the buffer(vector) to be printed
                 */
                void print_1d_buffer_preview(std::vector<data_type> & buffer_to_print){
                    //declare variable to keep track of how many samples to print out (limited to the first 5 and the last sample)
                    size_t samples_to_print;
                    if (buffer_to_print.size() > 5){
                        samples_to_print = 5;
                    }
                    else if (buffer_to_print.size() > 0)
                    {
                        samples_to_print = buffer_to_print.size();
                    }
                    
                    else{
                        std::cerr << "Buffer_2D::print_1d_buffer_preview: buffer to print is empty" << std::endl;
                        return;
                    }

                    //print out the first five samples
                    for (size_t i = 0; i < samples_to_print; i++)
                    {
                        std::cout << buffer_to_print[i].real() << " + " << buffer_to_print[i].imag() << "j, ";
                    }

                    //if there are more than 5 samples, print out the last sample from the vector as well
                    if(buffer_to_print.size() > 5){
                        std::cout << "\t...\t" << buffer_to_print.back().real() << " + " <<
                                buffer_to_print.back().imag() << "j" <<std::endl;
                    }
                    else{
                        std::cout << std::endl;
                    }
                    return;
                }

                /**
                 * @brief load data from a vector into the buffer
                 * 
                 * @param data_to_load the data to load into the buffer
                 * @param copy_until_buffer_full (on true) continuously copies data from the vector into the buffer until
                 * the buffer is full or until the excess samples is reached, even if multiple copies of the data are made
                 * (on false) inserts up to only 1 copy of the data into the buffer
                 */
                void load_data_into_buffer(std::vector<data_type> & data_to_load, bool copy_until_buffer_full = true){
                    //setup bool to stop copying if copy_until_buffer_full is false
                    bool stop_signal = false;
                    //setup iterators
                    typename std::vector<data_type>::iterator data_iterator = data_to_load.begin();
                    size_t row = 0;
                    typename std::vector<data_type>::iterator buffer_iterator = buffer[0].begin();
                    while (buffer_iterator != (buffer[num_rows - 1].end() - excess_samples) && stop_signal == false)
                    {
                        *buffer_iterator = *data_iterator;

                        //increment data iterator
                        if(data_iterator == data_to_load.end() - 1){
                            if(copy_until_buffer_full){
                                data_iterator = data_to_load.begin();
                            }
                            else{
                                stop_signal = true;
                            }
                            
                        }
                        else{
                            ++data_iterator;
                        }

                        //increment buffer iterator
                        if(buffer_iterator == buffer[row].end() - 1){
                            if(row == (num_rows - 1) && excess_samples == 0){
                                buffer_iterator = buffer[row].end();
                            }
                            else{
                                row = row + 1;
                                buffer_iterator = buffer[row].begin();
                            }
                        }
                        else{
                            ++buffer_iterator;
                        }
                    }
                }
                
                /**
                 * @brief print a prevew of the buffer (1st 3 rows, 1st 5 columns, last row, last column)
                 * 
                 */
                virtual void print_preview(){
                    size_t rows_to_print;
                    if (buffer.size() > 3){
                        rows_to_print = 3;
                    }
                    else if (buffer.size() > 0)
                    {
                        rows_to_print = buffer.size();
                    }
                    
                    else{
                        std::cerr << "Buffer_2D::print_preview: buffer to print is empty" << std::endl;
                    }

                    //print out the first three samples
                    for (size_t i = 0; i < rows_to_print; i++)
                    {
                        print_1d_buffer_preview(buffer[i]);
                    }

                    //if there are more than 5 samples, print out the last sample from the vector as well
                    if(buffer.size() > 5){
                        std::cout << "\t...\t" << std::endl;
                        print_1d_buffer_preview(buffer.back());
                    }
                    else{
                        std::cout << std::endl;
                    }
                    return;
                }

                /**
                 * @brief Function to load data from a file into the current buffer
                 * (assumes that the current buffer is already initialized and that the read file stream is 
                 * already open)
                 * 
                 */
                virtual void import_from_file(){
                    //save the data from the file into a vector
                    std::vector<data_type> data = Buffer<data_type>::load_data_from_read_file();

                    load_data_into_buffer(data,false);
                }

                /**
                 * @brief Save the current buffer to the file (assumes that the write file stream is already open)
                 * 
                 */
                virtual void save_to_file(){
                    //the out file stream must already be open when the function is called
                    if(Buffer<data_type>::write_file_stream.is_open()){
                        //save all of the rows except for the last one
                        for (size_t i = 0; i < num_rows - 1; i++)
                        {
                            Buffer<data_type>::write_file_stream.write((char*) &buffer[i].front(), buffer[i].size() * sizeof(data_type));
                        }
                        //for the last row, since there may be excess samples in the buffer, only save those pertaining to a chirp
                        Buffer<data_type>::write_file_stream.write((char*) &buffer.back().front(), (num_cols - excess_samples) * sizeof(data_type));

                    }
                    else{
                        std::cerr << "Buffer_2D::save_to_file: write_file_stream not open" << std::endl;
                    }
                }

        }; // end of Buffer_2D class

        /**
         * @brief Buffer_1D a 1D buffer that stores data in a buffer and allows easy read/write integration witha file
         * 
         * @tparam data_type: the type of data that the buffer stores 
         */
        template<typename data_type>
        class Buffer_1D : public Buffer<data_type>{
            public:
            //variables
                //a vector to store things in
                std::vector<data_type> buffer;

                //keep track of the size of the buffer
                size_t num_samples;

                //keep track of how many excess samples are unused in the buffer
            //funcions

                /**
                 * @brief Construct a new Buffer_1D object
                 * 
                 */
                Buffer_1D() : Buffer<data_type>() {}

                /**
                 * @brief Construct a new Buffer_1D object
                 * 
                 * @param debug the desired debug setting
                 */
                Buffer_1D(bool debug) : Buffer<data_type>(debug) {}

                /**
                 * @brief Construct a new Buffer_1D object
                 * 
                 * @param samples the number of samples that the buffer stores
                 * @param debug the desired debug setting
                 */
                Buffer_1D(size_t samples, bool debug = false) 
                    : Buffer<data_type>(debug),buffer(samples),
                    num_samples(samples) {}

                /**
                 * @brief Destroy the Buffer_1D object
                 * 
                 */
                virtual ~Buffer_1D() {}

                /**
                 * @brief print a preview of the buffer (assumes buffer already has samples in it)
                 * 
                 */
                virtual void print_preview() {
                    //determine whether or not to print the full buffer or a preview of the buffer
                    size_t samples_to_print;
                    if (buffer.size() > 5){
                        samples_to_print = 5;
                    }
                    else if (buffer.size() > 0)
                    {
                        samples_to_print = buffer.size();
                    }
                    
                    else{
                        std::cerr << "Buffer_1D::print_preview: buffer to print is empty" << std::endl;
                        return;
                    }

                    //print out the first five samples
                    for (size_t i = 0; i < samples_to_print; i++)
                    {
                        std::cout << buffer[i].real() << " + " << buffer[i].imag() << "j, ";
                    }

                    //if there are more than 5 samples, print out the last sample from the buffer as well
                    if(buffer.size() > 5){
                        std::cout << "\t...\t" << buffer.back().real() << " + " <<
                                buffer.back().imag() << "j" <<std::endl;
                    }
                    else{
                        std::cout << std::endl;
                    }
                    return;
                }

                /**
                 * @brief imports data from an already opened read_file_stream and loads it into the buffer,
                 * sets the buffer to be equal to the data in the buffer and sets num_samples to the number
                 * of samples imported from the file, Note: read file stream is closed after data is imported
                 * 
                 */
                virtual void import_from_file(){
                    //load the data from the read file
                    buffer = Buffer<data_type>::load_data_from_read_file();
                    num_samples = buffer.size();
                }

                /**
                 * @brief save the current buffer to the write_file_sream (assumes write file stream
                 * is already open)
                 * 
                 */
                virtual void save_to_file(){
                    if(Buffer<data_type>::write_file_stream.is_open()){
                        //save all of the rows except for the last one
                        Buffer<data_type>::write_file_stream.write((char*) &buffer.front(), buffer.size() * sizeof(data_type));
                    }
                    else{
                        std::cerr << "Buffer_1D::save_to_file: write_file_stream not open" << std::endl;
                    }
                }

                /**
                 * @brief Set size of the buffer and initialize a buffer of that size
                 * 
                 * @param num_samps the number of samples for the buffer to store
                 */
                void set_buffer_size(size_t num_samps){
                    buffer = std::vector<data_type>(num_samps);
                    num_samples = num_samps;
                }
        }; // end Buffer_1D class
    
        template<typename data_type>
        class RADAR_Buffer : public Buffer_2D<data_type>{
            public:
                //variables
                size_t num_chirps;
                size_t samples_per_chirp;

                //functions
                    
                    
                    /**
                     * @brief Construct a new fmcw buffer object
                     * 
                     */
                    RADAR_Buffer(): Buffer_2D<data_type>(){}
                    
                    /**
                     * @brief Construct a new fmcw buffer object
                     * 
                     * @param debug the desired debug setting
                     */
                    RADAR_Buffer(bool debug): Buffer_2D<data_type>(debug){}
                    
                    /**
                     * @brief Construct a new RADAR buffer object (initialized)
                     * 
                     * @param desired_samples_per_buff the number of samples per buffer (for the USRP)
                     * @param required_samples_per_chirp the number of samples in a single chirp
                     * @param desired_num_chirps the number of chirps the buffer should contain
                     * @param debug the desired debug setting (optional)
                     */
                    RADAR_Buffer(
                        size_t desired_samples_per_buff,
                        size_t required_samples_per_chirp,
                        size_t desired_num_chirps,
                        bool debug = false) 
                        : Buffer_2D<data_type>(debug){
                            configure_fmcw_buffer(
                                desired_samples_per_buff,
                                required_samples_per_chirp,
                                desired_num_chirps);
                        }
                    
                    /**
                     * @brief Destroy the fmcw buffer object
                     * 
                     */
                    ~RADAR_Buffer() {}

                    /**
                     * @brief configures a Buffer_2D to be able to operate as a buffer used by the RADAR radar,
                     * and initializes a buffer (vector) of the correct dimensions
                     * 
                     * @param desired_samples_per_buff the number of samples in a buffer
                     * @param required_samples_per_chirp the number of samples per chirp
                     * @param desired_num_chirps the number of chirps
                     */
                    void configure_fmcw_buffer(
                        size_t desired_samples_per_buff,
                        size_t required_samples_per_chirp,
                        size_t desired_num_chirps)
                    {
                        //start of code for function
                        num_chirps = desired_num_chirps;
                        samples_per_chirp = required_samples_per_chirp;
                        Buffer_2D<data_type>::num_cols = desired_samples_per_buff;

                        if (desired_samples_per_buff == required_samples_per_chirp){
                            Buffer_2D<data_type>::num_rows = desired_num_chirps;
                            Buffer_2D<data_type>::excess_samples = 0;
                        }
                        else if (((desired_num_chirps * required_samples_per_chirp) % desired_samples_per_buff) == 0){
                            Buffer_2D<data_type>::num_rows = (desired_num_chirps * required_samples_per_chirp) / desired_samples_per_buff;
                            Buffer_2D<data_type>::excess_samples = 0;   
                        }
                        else
                        {
                            Buffer_2D<data_type>::num_rows = ((desired_num_chirps * required_samples_per_chirp) / desired_samples_per_buff) + 1;
                            Buffer_2D<data_type>::excess_samples = (Buffer_2D<data_type>::num_rows * desired_samples_per_buff) - (desired_num_chirps * required_samples_per_chirp);
                        }

                        Buffer_2D<data_type>::buffer = std::vector<std::vector<data_type>>(Buffer_2D<data_type>::num_rows,std::vector<data_type>(Buffer_2D<data_type>::num_cols));
                    }

                    /**
                     * @brief loads data from a vector into the initizlized RADAR buffer, making copies of the 
                     * vector as necessary until the buffer is full or the number of excess samples has been
                     * reached
                     * 
                     * @param chirp a vector containing the samples for a signle chirp
                     */
                    void load_chirp_into_buffer(std::vector<data_type> & chirp){
                        Buffer_2D<data_type>::load_data_into_buffer(chirp,true);
                    }
        };

    }
#endif