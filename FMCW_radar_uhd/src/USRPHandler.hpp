#ifndef USRPHANDLER
#define USRPHANDLER
    //C standard libraries
    #include <iostream>
    #include <cstdlib>
    #include <string>
    #include <chrono>
    #include <complex>
    #include <csignal>
    #include <thread>

    //uhd specific libraries
    #include <uhd/exception.hpp>
    #include <uhd/types/tune_request.hpp>
    #include <uhd/usrp/multi_usrp.hpp>
    #include <uhd/utils/safe_main.hpp>
    #include <uhd/utils/thread.hpp>
    #include <boost/format.hpp>
    #include <boost/lexical_cast.hpp>
    #include <boost/program_options.hpp>

    //includes for JSON editing
    #include <nlohmann/json.hpp>

    //includes for uhd specific coding
    //including uhd specific libraries
    #include "uhd/types/device_addr.hpp"
    #include "uhd/device.hpp"

    using json = nlohmann::json;

    namespace USRPHandler_namespace {
        class USRPHandler {
            
            public:
                //class variables
                //usrp device
                uhd::usrp::multi_usrp::sptr usrp;
                
                //stream arguments
                uhd::stream_args_t stream_args;
                uhd::tx_streamer::sptr tx_stream;
                uhd::tx_metadata_t tx_md;
                uhd::rx_streamer::sptr rx_stream;
                uhd::rx_metadata_t rx_md;
                
                //json config file
                json config;

                //class functions
                USRPHandler(json & config_file);
                uhd::device_addrs_t find_devices (void);
                void create_USRP_device(void);
                void wait_for_setup_time(void);
                void set_ref(void);
                void set_subdevices(void);
                void set_sample_rate(void);
                void set_center_frequency(void);
                void set_rf_gain(void);
                void set_if_filter_bw(void);
                void set_antennas(void);
                void check_lo_locked(void);
                void init_multi_usrp(void);
                void reset_usrp_clock(void);
        };
    }
#endif