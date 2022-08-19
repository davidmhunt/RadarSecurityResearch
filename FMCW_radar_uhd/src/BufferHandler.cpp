#include "BufferHandler.hpp"

using Buffers::Buffer;
using Buffers::Buffer_2D;

/*
    CODE FOR BUFER CLASS
*/


/**
 * @brief Default Constructor for Buffer class
 * 
 * @tparam data_type: the type of data that the buffer holds
 */
template <typename data_type>
Buffer<data_type>::Buffer(): debug_status(false){};

/**
 * @brief Construct a new Buffer<data_type>::Buffer object, alternative constructor to specify debug status
 * 
 * @tparam data_type: the type of data that the buffer holds 
 * @param debug the desired debug status
 */
template <typename data_type>
Buffer<data_type>::Buffer(bool debug) : debug_status(debug) {};

template <typename data_type>
Buffer<data_type>::~Buffer(){
    if (read_file_stream.is_open()){
        read_file_stream.close();
    }
    if (write_file_stream.is_open()){
        write_file_stream.close();
    }
}

/**
 * @brief set the read file path
 * 
 * @tparam data_type: type of data that the buffer holds
 * @param file_name the file name to be read
 */
template <typename data_type>
void Buffer<data_type>::set_read_file(std::string file_name){
    read_file = file_name;
}

/**
 * @brief set the write file path
 * 
 * @tparam data_type: type of data that the buffer holds
 * @param file_name the file name to be read
 */
template <typename data_type>
void Buffer<data_type>::set_write_file(std::string file_name){
    write_file = file_name;
}

/**
 * @brief open the read file in preparation for streaming
 * 
 * @tparam data_type: the type of data that the buffer holds
 */
template <typename data_type>
void Buffer<data_type>::init_read_file_stream(void){
        //open the write file
    if(read_file.empty()){
        std::cerr << "Buffer::init_read_file_stream: no read_file name" <<std::endl;
    }
    else{
        read_file_stream.open(read_file.c_str(), std::ios::out | std::ios::binary);
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
 * @tparam data_type: the type of data that the buffer holds
 */
template <typename data_type>
void Buffer<data_type>::init_write_file_stream(void){
        //open the write file
    if(write_file.empty()){
        std::cerr << "Buffer::init_write_file_stream: no write_file name" <<std::endl;
    }
    else{
        write_file_stream.open(write_file.c_str(), std::ios::in | std::ios::binary);
        if(write_file_stream.is_open()){
            std::cout << "Buffer::init_write_file_stream: write file opened successfully" <<std::endl;
        }
        else{
            std::cerr << "BufferHandler::failed to open write file\n";
        }
    }
}

/**
 * @brief close the read_file_stream
 * 
 * @tparam data_type: the type of data in the buffer
 */
template <typename data_type>
void Buffer<data_type>::close_read_file_stream(void){
    if(read_file_stream.open){
        read_file_stream.close();
        std::cout << "Buffer::close_read_file_stream: read file stream closed" << std::endl;
    }
    else{
        std::cerr << "Buffer::close_read_file_stream: close_read_file_stream"  << 
            "called but read file already closed" << std::endl;
    }
}

/**
 * @brief close the write_file_stream
 * 
 * @tparam data_type: the type of data in the buffer
 */
template <typename data_type>
void Buffer<data_type>::close_write_file_stream(void){
    if(write_file_stream.open){
        write_file_stream.close();
        std::cout << "Buffer::close_write_file_stream: write file stream closed" << std::endl;
    }
    else{
        std::cerr << "Buffer::close_write_file_stream: close_write_file_stream"  << 
            "called but write file already closed" << std::endl;
    }
}

/**
 * @brief Read the data stored in the read_file_stream (must already be opened), and return a vector with the data
 * 
 * @tparam data_type: the type of data that the buffer holds
 * @return std::vector<data_type>: a vector with the data stored in the read file
 */
template <typename data_type>
std::vector<data_type> Buffer<data_type>::load_data_from_read_file(){
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


/*
    CODE FOR BUFFER_2D Class
*/

/**
 * @brief Construct a new Buffer_2D<data_type>::Buffer_2D object
 * 
 * @tparam data_type: the type of data that the buffer stores
 */
template<typename data_type>
Buffer_2D<data_type>::Buffer_2D() : Buffer<data_type>(){}

/**
 * @brief Construct a new Buffer_2D<data_type>::Buffer_2D object
 * 
 * @tparam data_type: the typeof data that the buffer stores
 * @param debug the specified debug setting
 */
template<typename data_type>
Buffer_2D<data_type>::Buffer_2D(bool debug) : Buffer<data_type>(debug){}

/**
 * @brief Construct a new Buffer_2D<data_type>::Buffer_2D object
 * 
 * @tparam data_type: the type of data that the buffer stores
 * @param rows the number of rows to be in the buffer
 * @param cols the number of columns to be in the buffer
 * @param exces the number of excess/unused samples in the buffer
 * @param debug the specified debug setting
 */
template<typename data_type>
Buffer_2D<data_type>::Buffer_2D(size_t rows, size_t cols,size_t exces,bool debug)
                                 : Buffer<data_type>(debug),buffer(rows,std::vector<data_type>(cols)),
                                 num_rows(rows),num_cols(cols){}

/**
 * @brief Destroy the Buffer_2D<data_type>::Buffer_2D object
 * 
 * @tparam data_type: the type of data that the buffer stores
 */
template<typename data_type>
Buffer_2D<data_type>::~Buffer_2D(){}

/**
 * @brief prints out a 1d buffer
 * 
 * @tparam data_type: the type of data that the buffer stores
 * @param buffer_to_print the buffer to be printed
 */
template<typename data_type>
void Buffer_2D<data_type>::print_1d_buffer_preview(std::vector<data_type> & buffer_to_print){
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
 * @brief print a prevew of the buffer (1st 3 rows, 1st 5 columns, last row, last column)
 * 
 * @tparam data_type: the type of data that the buffer stores
 */
template<typename data_type>
void Buffer_2D<data_type>::print_preview(void){
    
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
 * @brief load data from a vector into the buffer
 * 
 * @tparam data_type: the ype of data that the buffer stores 
 * @param data_to_load the data to load into the bufer
 * @param copy_until_buffer_full (on true) continuously copies data from the vector into the buffer until
 * the buffer is full or until the excess samples is reached, even if multiple copies of the data are made
 * (on false) inserts up to only 1 copy of the data into the buffer
 */
template<typename data_type>
void Buffer_2D<data_type>::load_data_into_buffer(std::vector<data_type> & data_to_load, bool copy_until_buffer_full){
        
        //setup bool to stop copying if copy_until_buffer_full is false
        bool stop_signal = false;
        //setup iterators
        typename std::vector<data_type>::iterator data_iterator = data.begin();
        size_t row = 0;
        typename std::vector<data_type>::iterator buffer_iterator = buffer[0].begin();
        while (buffer_iterator != (buffer[num_rows - 1].end() - excess_samples) && stop_signal == false)
        {
            *buffer_iterator = *data_iterator;

            //increment data iterator
            if(data_iterator == data.end() - 1){
                if(copy_until_buffer_full){
                    data_iterator = data.begin();
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
 * @brief Load data into a pre-initialized Buffer_2D using the already opened read_file_stream,
 * only one copy of the data from the file is loadd though
 * 
 * @tparam data_type: the type of data that the buffer stores
 */
template<typename data_type>
void Buffer_2D<data_type>::import_from_file(void){

        //save the data from the file into a vector
        std::vector<data_type> data = load_data_from_read_file();

        load_data_into_buffer(data,false);
}