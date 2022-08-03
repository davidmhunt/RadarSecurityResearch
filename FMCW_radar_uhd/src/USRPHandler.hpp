#ifndef USRPHANDLER
#define USRPHANDLER
    //C standard libraries
    #include <iostream>
    #include <cstdlib>
    #include <string>
    #include <sstream>
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

    //user generated header files
    #include "BufferHandler.hpp"

    using json = nlohmann::json;

    namespace USRPHandler_namespace {
        class USRPHandler {
            
            private:
                bool overflow_message; //true if no overflow message sent
                bool rx_first_buffer;
                std::atomic<bool> tx_stream_complete;
                size_t rx_channel;
                size_t tx_channel;
            public:
                //class variables
                BufferHandler_namespace::BufferHandler * buffer_handler;
                //usrp device
                uhd::usrp::multi_usrp::sptr usrp;
                
                //FMCW arguments
                size_t num_frames;
                double frame_periodicity;
                
                //timing arguments
                double stream_start_time;
                std::vector<uhd::time_spec_t> frame_start_times;
                uhd::time_spec_t rx_stream_start_offset;
                
                //stream arguments
                uhd::stream_args_t tx_stream_args;
                uhd::tx_streamer::sptr tx_stream;
                uhd::tx_metadata_t tx_md;
                uhd::async_metadata_t tx_async_md;
                size_t tx_samples_per_buffer;
                uhd::stream_args_t rx_stream_args;
                uhd::rx_streamer::sptr rx_stream;
                uhd::rx_metadata_t rx_md;
                size_t rx_samples_per_buffer;

                
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
                void init_frame_start_times(void);
                void init_stream_args(void);
                void reset_usrp_clock(void);
                void load_BufferHandler(
                    BufferHandler_namespace::BufferHandler * new_buffer_handler);
                void stream_rx_frame(void);
                void check_rx_metadata(uhd::rx_metadata_t & rx_md);
                void stream_tx_frame(void);
                void check_tx_async_messages(void);
                void stream_frame(void);
        };
    }
#endif