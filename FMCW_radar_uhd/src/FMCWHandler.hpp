#ifndef FMCWHANDLER
#define FMCWHANDLER

    //c standard library
    #include <iostream>
    #include <cstdlib>
    #include <string>
    #include <complex>
    #include <csignal>

    //uhd specific libraries
    #include <uhd/usrp/multi_usrp.hpp>

    //JSON class
    #include <nlohmann/json.hpp>

    //source libraries
    #include "JSONHandler.hpp"
    #include "USRPHandler.hpp"
    #include "BufferHandler.hpp"

    using json = nlohmann::json;
    using USRPHandler_namespace::USRPHandler;
    using Buffers::FMCW_Buffer;
    using Buffers::Buffer_1D;

    namespace FMCW_namespace{

        /**
         * @brief FMCW Class - class to run all FMCW experiments out of
         * 
         * @tparam data_type 
         */
        template<typename data_type>
        class FMCW {
            //variables
            private:
                json config;
                USRPHandler usrp_handler;
                FMCW_Buffer<data_type> tx_buffer;
                FMCW_Buffer<data_type> rx_buffer;
            //functions
                //constructors and destructors
            public:
                FMCW(json & config_data);

                //functions to aid in buffer initializations
                std::vector<data_type> get_tx_chirp(void);
                
                //initialization functions
                void init_buffers_for_radar(void);
                void init_buffers_for_synchronization(void);                
                
                //functions to run streaming
                void run_FMCW(void);
        };
    }

#endif