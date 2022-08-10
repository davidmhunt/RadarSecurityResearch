#include "BufferHandler.hpp"

using BufferHandler_namespace::BufferHandler;

BufferHandler::BufferHandler(
    json config,
    size_t rx_spb,
    size_t tx_spb){
        if (config["FMCWSettings"]["tx_file_name"].is_null() ||
            config["FMCWSettings"]["rx_file_name"].is_null() ||
            config["FMCWSettings"]["num_chirps"].is_null()){
                std::cerr << "BufferHandler: tx_file_name or rx_file_name not specified in JSON";
        }
        tx_file = config["FMCWSettings"]["tx_file_name"].get<std::string>();
        rx_file = config["FMCWSettings"]["rx_file_name"].get<std::string>();
        num_chirps = config["FMCWSettings"]["num_chirps"].get<size_t>();
        rx_samples_per_buff = rx_spb;
        tx_samples_per_buff = tx_spb;
        
        configure_debug(config);
        
        //run the initialization function
        init_BufferHandler();
}

BufferHandler::~BufferHandler(){
    if (rx_file_stream.is_open()){
        rx_file_stream.close();
    }
    if (tx_file_stream.is_open()){
        tx_file_stream.close();
    }
}

void BufferHandler::configure_debug(json & config){
    if (config["USRPSettings"]["AdditionalSettings"]["bufferHandler_debug"].is_null() == false){
        debug_status = config["USRPSettings"]["AdditionalSettings"]["bufferHandler_debug"].get<bool>();
        std::cout << "BufferHandler::configure_debug: bufferHandler_debug: " << debug_status << std::endl << std::endl;
    }
    else{
        std::cerr << "BufferHandler::configure_debug: couldn't find bufferHandler_debug in JSON" <<std::endl;
    }
}

void BufferHandler::load_tx_chirp(void){
        
    //initialize a vector to store the chirp from the file

    tx_chirp_buffer = load_data_from_file<std::complex<float>>(tx_file);
    samples_per_chirp = tx_chirp_buffer.size();
    std::cout << "BufferHandler::load_tx_chirp: detected samples per chirp: " << samples_per_chirp << std::endl;
    return;
}

void BufferHandler::init_tx_buffer(void){
    
    //compute the numer of buffers we need
    std::tuple<size_t,size_t> tx_buffer_settings = compute_usrp_buffer_settings(
            tx_samples_per_buff,
            samples_per_chirp,
            num_chirps);
    
    tx_num_rows = std::get<0>(tx_buffer_settings);
    tx_excess_samples = std::get<1>(tx_buffer_settings);

    std::cout << "BufferHandler::init_tx_buffer: Num Rows: " << tx_num_rows << " Excess Samples: " << tx_excess_samples << std::endl;
    
    //copy the chirp into the buffers
    tx_buffer = std::vector<std::vector<std::complex<float>>>(tx_num_rows,std::vector<std::complex<float>>(tx_samples_per_buff));
    
    //load the data into the usrp buffer
    load_data_into_usrp_buffer<std::complex<float>>(tx_chirp_buffer,tx_buffer, tx_excess_samples);
    
    if(debug_status){
        std::cout << "BufferHandler::init_tx_buffer: Tx Buffer Preview:" << std::endl;
        print_2d_buffer_preview(tx_buffer);
    }
    
    return;
}

void BufferHandler::init_rx_buffer(void){

    //open the write file
    rx_file_stream.open(rx_file.c_str(), std::ios::out | std::ios::binary);
    if(rx_file_stream.is_open()){

        //compute the numer of buffers we need
        std::tuple<size_t,size_t> rx_buffer_settings = compute_usrp_buffer_settings(
                rx_samples_per_buff,
                samples_per_chirp,
                num_chirps);
        
        rx_num_rows = std::get<0>(rx_buffer_settings);
        rx_excess_samples = std::get<1>(rx_buffer_settings);

        std::cout << "BufferHandler::init_rx_buffer: Num Rows: " << rx_num_rows << " Excess Samples: " << rx_excess_samples << std::endl;
        
        //initialize the rx_buffer
        rx_buffer = std::vector<std::vector<std::complex<float>>>(rx_num_rows,std::vector<std::complex<float>>(rx_samples_per_buff));
        
        if(debug_status){
            std::cout << "BufferHandler::init_rx_buffer: Rx Buffer Preview:" << std::endl;
            print_2d_buffer_preview(rx_buffer);
        }
    }
    else{
        std::cerr << "BufferHandler::failed to open rx file\n";
    }
}


void BufferHandler::init_BufferHandler(void){
    //load the tx chirp
    load_tx_chirp();

    //initialize the tx buffer
    init_tx_buffer();

    //initialize the rx buffer
    init_rx_buffer();
}

void BufferHandler::save_rx_buffer_to_file(void){
    //once the rx buffer has been filled up with all of its chirps, call this function to save the buffer to a file
    save_usrp_buffer_to_file(rx_buffer,rx_file_stream,rx_excess_samples);
    /*
    if(rx_file_stream.is_open()){
        //save all of the rows except for the last one
        for (size_t i = 0; i < rx_num_rows - 1; i++)
        {
            rx_file_stream.write((char*) &rx_buffer[i].front(), rx_buffer[i].size() * sizeof(std::complex<float>));
        }
        //for the last row, since there may be excess samples in the buffer, only save those pertaining to a chirp
        rx_file_stream.write((char*) &rx_buffer.back().front(), (rx_samples_per_buff - rx_excess_samples) * sizeof(std::complex<float>));

    }
    else{
        std::cerr << "BufferHandler::save_rx_buffer_to_file: rx_file not open" << std::endl;
    }
    */
}

void BufferHandler::print_1d_buffer_preview(std::vector<std::complex<float>>& buffer_to_print){
    //declare variable to keep track of how many samples to print out (limited to the first 5 and the last sample)
    size_t samples_to_print;
    if (buffer_to_print.size() > 5){
        samples_to_print = 5;
    }
    else{
        samples_to_print = buffer_to_print.size();
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

void BufferHandler::print_2d_buffer_preview(std::vector<std::vector<std::complex<float>>> & buffer_to_print){
    size_t rows_to_print;
    if (buffer_to_print.size() > 3){
        rows_to_print = 3;
    }
    else{
        rows_to_print = buffer_to_print.size();
    }

    //print out the first three samples
    for (size_t i = 0; i < rows_to_print; i++)
    {
        print_1d_buffer_preview(buffer_to_print[i]);
    }

    //if there are more than 5 samples, print out the last sample from the vector as well
    if(buffer_to_print.size() > 5){
        std::cout << "\t...\t" << std::endl;
        print_1d_buffer_preview(buffer_to_print.back());
    }
    else{
        std::cout << std::endl;
    }
    return;
}

/**
 * @brief function loads in data from a file and returns the data stored in a vector
 * 
 * @tparam data_type the data type of the data being read
 * @param file the path to the file being read
 * @return std::vector<data_type> a vector containing the read data
 */
template<typename data_type>
std::vector<data_type> BufferHandler::load_data_from_file(std::string file){
    //initialize a file stream to read the data from
    std::ifstream file_stream;
    std::vector<data_type> data_vector;
    
    //open the specified file
    file_stream.open(file.c_str(), std::ios::in | std::ios::binary);

    if (file_stream.is_open()){

        //get the size of the file to be read
        std::streampos size;
        file_stream.seekg (0,std::ios::end);
        size = file_stream.tellg();

        //determine the number of samples in the file
        size_t detected_samples = size / sizeof(data_type);
        std::cout << "BufferHandler::load_data_from_file: detected samples: " << detected_samples << std::endl;

        //define the vector
        data_vector = std::vector<data_type>(detected_samples);

        //read the file
        file_stream.seekg(0,std::ios::beg);
        file_stream.read((char*) &data_vector.front(), data_vector.size() * sizeof(data_type));
        file_stream.close();

        if(debug_status){
            std::cout << "BufferHandler::load_data_from_file: Data Vector Preview:" << std::endl;
            print_1d_buffer_preview(data_vector);
        }   
    }
    else{
        std::cerr << "Could not open file\n";
    }
    return data_vector;
}

/**
 * @brief computes the number of buffer rows and number of excess samples 
 *          for a tx or rx buffer to be used while streaming on the usrp
 * 
 * @param desired_samples_per_buff  the number of samples in a single usrp buffer (1 row of the buffer)
 * @param required_samples_per_chirp  the number of samples in a precomputed chirp
 * @param desired_num_chirps the number of precomputed chirps the buffer will contain
 * @return std::tuple<size_t,size_t> computed settings for a usrp tx or rx buffer
 *                                  stored as (num_rows, excess_samples)
 */
std::tuple<size_t,size_t> BufferHandler::compute_usrp_buffer_settings(
                    size_t desired_samples_per_buff,
                    size_t required_samples_per_chirp,
                    size_t desired_num_chirps){
    
    size_t num_rows;
    size_t excess_samples;

    if (desired_samples_per_buff == required_samples_per_chirp){
        num_rows = desired_num_chirps;
        excess_samples = 0;
    }
    else if (((desired_num_chirps * required_samples_per_chirp) % desired_samples_per_buff) == 0){
        num_rows = (desired_num_chirps * required_samples_per_chirp) / desired_samples_per_buff;
        excess_samples = 0;   
    }
    else
    {
        num_rows = ((desired_num_chirps * required_samples_per_chirp) / desired_samples_per_buff) + 1;
        excess_samples = (num_rows * desired_samples_per_buff) - (desired_num_chirps * required_samples_per_chirp);
    }
    return std::make_tuple(num_rows,excess_samples);
}

/**
 * @brief copies data loaded from a file using load_data_from_file into a usrp buffer
 * 
 * @tparam data_type the type of the data that was read from a file
 * @param data the data loaded from the file stored in a 1d vector
 * @param usrp_buffer  a pre-configured usrp_buffer (2d vector) that the data will be loaded into
 * @param excess_samples the pre-computed number of excess samples that will be unused in the buffer
 */
template<typename data_type>
void BufferHandler::load_data_into_usrp_buffer(
    std::vector<data_type> & data,
    std::vector<std::vector<data_type>> & usrp_buffer,
    size_t excess_samples){

        //get information about the usrp_buffer
        size_t num_rows = usrp_buffer.size();
        size_t samples_per_buffer = usrp_buffer[0].size();

        //setup iterators
        typename std::vector<data_type>::iterator data_iterator = data.begin();
        size_t row = 0;
        typename std::vector<data_type>::iterator buffer_iterator = usrp_buffer[0].begin();
        while (buffer_iterator != (usrp_buffer[num_rows - 1].end() - excess_samples))
        {
            *buffer_iterator = *data_iterator;

            //increment data iterator
            if(data_iterator == data.end() - 1){
                data_iterator = data.begin();
            }
            else{
                ++data_iterator;
            }

            //increment buffer iterator
            if(buffer_iterator == usrp_buffer[row].end() - 1){
                if(row == (num_rows - 1) && excess_samples == 0){
                    buffer_iterator = usrp_buffer[row].end();
                }
                else{
                    row = row + 1;
                    buffer_iterator = usrp_buffer[row].begin();
                }
            }
            else{
                ++buffer_iterator;
            }
        }
}


/**
 * @brief saves a usrp_buffer to a file
 * 
 * @tparam data_type the data type of the samples to be saved to a file
 * @param usrp_buffer the usrp buffer to be saved (2d vector)
 * @param out_file_stream the file stream to save the buffer to (Note: must already be open)
 * @param excess_samples the number of unused samples in the buffer (if any)
 */
template<typename data_type>
void BufferHandler::save_usrp_buffer_to_file(
    std::vector<std::vector<data_type>>  & usrp_buffer,
    std::ofstream & out_file_stream,
    size_t & excess_samples){
        
        //determine characteristics about the buffer
        size_t num_rows = usrp_buffer.size();
        size_t samples_per_buff = usrp_buffer[0].size();

        //the out file stream must already be open when the function is called
        if(out_file_stream.is_open()){
            //save all of the rows except for the last one
            for (size_t i = 0; i < num_rows - 1; i++)
            {
                out_file_stream.write((char*) &usrp_buffer[i].front(), usrp_buffer[i].size() * sizeof(data_type));
            }
            //for the last row, since there may be excess samples in the buffer, only save those pertaining to a chirp
            out_file_stream.write((char*) &usrp_buffer.back().front(), (samples_per_buff - excess_samples) * sizeof(data_type));

        }
        else{
            std::cerr << "BufferHandler::save_usrp_buffer_to_file: out_file_stream not open" << std::endl;
        }
    }