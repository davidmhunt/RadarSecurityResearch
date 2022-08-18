#include "BufferHandler.hpp"

using Buffers::Buffer;

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
