#include "BufferHandler.hpp"

using BufferHandler_namespace::BufferHandler;

BufferHandler::BufferHandler(
    json config,
    size_t rx_spb,
    size_t tx_spb,
    bool debug){
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
        debug_status = debug;
        if (debug_status){
            std::cout << "BufferHandler: Debug Status = " << debug_status << std::endl;
        }
        
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

void BufferHandler::load_tx_chirp(void){
        
        //initialize a vector to store the chirp from the file
        
        //open the specified file
        tx_file_stream.open(tx_file.c_str(), std::ios::in | std::ios::binary);

        if (tx_file_stream.is_open()){

            //get the size of the file to be read
            std::streampos size;
            tx_file_stream.seekg (0,std::ios::end);
            size = tx_file_stream.tellg();

            //determine the number of samples in the file
            samples_per_chirp = size / sizeof(std::complex<float>);
            std::cout << "BufferHandler::read_tx_file: detected samples per chirp: " << samples_per_chirp << std::endl;

            //define the complex vector
            tx_chirp_buffer = std::vector<std::complex<float>>(samples_per_chirp);

            //read the file
            tx_file_stream.seekg(0,std::ios::beg);
            tx_file_stream.read((char*) &tx_chirp_buffer.front(), tx_chirp_buffer.size() * sizeof(std::complex<float>));
            tx_file_stream.close();

            if(debug_status){
                std::cout << "BufferHandler::load_tx_chirp: Chirp Vector Preview:" << std::endl;
                print_1d_buffer_preview(tx_chirp_buffer);
            }   
        }
        else{
            std::cerr << "Could not open file\n";
        }
    return;
}

void BufferHandler::init_tx_buffer(void){
    
    //compute the numer of buffers we need

    if (tx_samples_per_buff == samples_per_chirp){
        tx_num_rows = num_chirps;
        tx_excess_samples = 0;
    }
    else if (((num_chirps * samples_per_chirp) % tx_samples_per_buff) == 0){
        tx_num_rows = (num_chirps * samples_per_chirp) / tx_samples_per_buff;
        tx_excess_samples = 0;   
    }
    else
    {
        tx_num_rows = ((num_chirps * samples_per_chirp) / tx_samples_per_buff) + 1;
        tx_excess_samples = (tx_num_rows * tx_samples_per_buff) - (num_chirps * samples_per_chirp);
    }
    std::cout << "BufferHandler::init_tx_buffer: Num Rows: " << tx_num_rows << " Excess Samples: " << tx_excess_samples << std::endl;
    
    //copy the chirp into the buffers
    tx_buffer = std::vector<std::vector<std::complex<float>>>(tx_num_rows,std::vector<std::complex<float>>(tx_samples_per_buff));
    std::vector<std::complex<float>>::iterator chirp_iterator = tx_chirp_buffer.begin();
    size_t row = 0;
    std::vector<std::complex<float>>::iterator buffer_iterator = tx_buffer[0].begin();
    while (buffer_iterator != (tx_buffer[tx_num_rows - 1].end() - tx_excess_samples))
    {
        *buffer_iterator = *chirp_iterator;

        //increment chirp iterator
        if(chirp_iterator == tx_chirp_buffer.end() - 1){
            chirp_iterator = tx_chirp_buffer.begin();
        }
        else{
            ++chirp_iterator;
        }

        //increment buffer iterator
        if(buffer_iterator == tx_buffer[row].end() - 1){
            if(row == (tx_num_rows - 1) && tx_excess_samples == 0){
                buffer_iterator = tx_buffer[row].end();
            }
            else{
                row = row + 1;
                buffer_iterator = tx_buffer[row].begin();
            }
        }
        else{
            ++buffer_iterator;
        }
    }
    
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
        if (rx_samples_per_buff == samples_per_chirp){
            rx_num_rows = num_chirps;
            rx_excess_samples = 0;
        }
        else if (((num_chirps * samples_per_chirp) % rx_samples_per_buff) == 0){
            rx_num_rows = (num_chirps * samples_per_chirp) / rx_samples_per_buff;
            rx_excess_samples = 0;   
        }
        else
        {
            rx_num_rows = ((num_chirps * samples_per_chirp) / rx_samples_per_buff) + 1;
            rx_excess_samples = (rx_num_rows * rx_samples_per_buff) - (num_chirps * samples_per_chirp);
        }
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