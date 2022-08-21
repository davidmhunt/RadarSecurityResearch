#include "FMCWHandler.hpp"

using FMCW_namespace::FMCW;

template<typename data_type>
FMCW<data_type>::FMCW() {};

/**
 * @brief Construct a new fmcw<data type>::fmcw object,
 * initializes the usrp_handler, initializes the buffers, and runs the experiment
 * 
 * @tparam data_type 
 * @param config_data 
 */
template<typename data_type>
FMCW<data_type>::FMCW(json config_data) : 
    config(config_data), 
    usrp_handler(config_data){
        init_buffers_for_radar();
}

/**
 * @brief Destroy the fmcw<data type>::fmcw object
 * 
 * @tparam data_type: the type of data that the FMCW handler works with
 */
template<typename data_type>
FMCW<data_type>::~FMCW() {}


template<typename data_type>
std::vector<data_type> FMCW<data_type>::get_tx_chirp(void){

    //initialize a vector to store the chirp data
    Buffer_1D<data_type> tx_chirp_buffer;

    if (config["FMCWSettings"]["tx_file_name"].is_null()){
        std::cerr << "FMCW: tx_file_name not specified in JSON";
        return tx_chirp_buffer.buffer;
    }
    std::string tx_file = config["FMCWSettings"]["tx_file_name"].get<std::string>();

    //create a buffer to load the chirp data into it
    tx_chirp_buffer.set_read_file(tx_file,true);
    tx_chirp_buffer.import_from_file();
    tx_chirp_buffer.close_read_file_stream();

    tx_chirp_buffer.print_preview();
    return tx_chirp_buffer.buffer;

}

template<typename data_type>
void FMCW<data_type>::init_buffers_for_radar(void){
    std::vector<data_type> tx_chirp = get_tx_chirp();
}

