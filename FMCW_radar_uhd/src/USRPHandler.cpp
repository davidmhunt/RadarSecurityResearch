#include "USRPHandler.hpp"

using USRPHandler_namespace::USRPHandler;

USRPHandler::USRPHandler(json & config_file){
    config = config_file;
    configure_debug();
    init_multi_usrp();
}

void USRPHandler::configure_debug(void){
    if (config["USRPSettings"]["AdditionalSettings"]["simplified_streamer_metadata"].is_null() == false){
        simplified_metadata = config["USRPSettings"]["AdditionalSettings"]["simplified_streamer_metadata"].get<bool>();
        std::cout << "USRPHandler::configure_debug: simplified metadata: " << simplified_metadata << std::endl << std::endl;
    }
    else{
        std::cerr << "USRPHandler::configure_debusg: couldn't find simplified_streamer_metadata in JSON" <<std::endl;
    }
}

uhd::device_addrs_t USRPHandler::find_devices(void){
    /*
    Purpose: finds all of the USRP devices connected to the host and returns a vector of them
    Outputs:
        uhd::device_addrs_t - a vector of uhd::device_addr_t objects for each detected device
    */
    uhd::device_addr_t hint;
    uhd::device_addrs_t dev_addrs = uhd::device::find(hint);
    if (dev_addrs.size() > 0){
        std::cout << "USRPHandler::find_devices: Number of devices found: " << dev_addrs.size() << "\n";
        std::cout << "USRPHandler::find_devices: Device Information: \n" << dev_addrs[0].to_pp_string() <<std::endl;
    }
    else{
        std::cerr << "USRPHandler::find_devices: No devices found\n";
    }
    return dev_addrs;
}

void USRPHandler::create_USRP_device(void){
    

    // create a usrp device
    if (config["USRPSettings"]["Multi-USRP"]["args"].is_null() == false &&
        config["USRPSettings"]["Multi-USRP"]["args"].get<std::string>().empty() == false)
    {
        
        std::string args = config["USRPSettings"]["Multi-USRP"]["args"].get<std::string>();
        std::cout << "USRPHandler::create_USRP_device:Creating the usrp device with: " <<  args << std::endl;
        usrp = uhd::usrp::multi_usrp::make(args);
    }
    else //if no device was specified, search for and use the first USRP device
    {
        std::cout << std::endl;
        std::cout << "USRPHandler::create_USRP_device: No usrp device arguments specified, searching for device...\n";
        uhd::device_addrs_t dev_addrs = find_devices();
        usrp = uhd::usrp::multi_usrp::make(dev_addrs[0]);
    }
}

void USRPHandler::wait_for_setup_time(void){
    if (config["USRPSettings"]["AdditionalSettings"]["setup_time"].is_null() == false){
        float setup_time = config["USRPSettings"]["AdditionalSettings"]["setup_time"].get<float>();
        std::cout << "USRPHandler::wait_for_setup_time: waiting for " << 
                    setup_time * 1000 << " ms for setup" <<std::endl << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(int64_t(1000 * setup_time)));
    }
    else{
        std::cerr << "USRPHandler::wait_for_setup_time: no setup time allotted in JSON";
    }
}

void USRPHandler::set_ref(void){
    //set clock reference
    if (config["USRPSettings"]["Multi-USRP"]["ref"].is_null() == false)
    {
        std::string ref = config["USRPSettings"]["Multi-USRP"]["ref"].get<std::string>();
        if (ref.empty() == false){
            usrp->set_clock_source(ref);
        }
        else{
            std::cout << "USRPHandler::set_ref: ref defined, but empty string\n\n";
        }
    }
    else {
        std::cout << "USRPHandler::set_ref: ref not defined in JSON\n\n";
    }
}

void USRPHandler::set_subdevices(void){
    //set Tx and Rx subdevices
    if (config["USRPSettings"]["Rx"]["subdev"].is_null() == false){
        std::string rx_subdev = config["USRPSettings"]["Rx"]["subdev"].get<std::string>();
        if (rx_subdev.empty() == false){
            usrp->set_rx_subdev_spec(rx_subdev);
        }
    }
    if (config["USRPSettings"]["Tx"]["subdev"].is_null() == false){
        std::string tx_subdev = config["USRPSettings"]["Tx"]["subdev"].get<std::string>();
        if (tx_subdev.empty() == false){
            usrp->set_tx_subdev_spec(tx_subdev);
        }
    }
    std::cout << boost::format("USRPHandler::set_subdevices: Using Device: %s") % usrp->get_pp_string() << std::endl;
}

void USRPHandler::set_sample_rate(void){

    //rx rate
    if (config["USRPSettings"]["Multi-USRP"]["sampling_rate"].is_null() == false &&
    config["USRPSettings"]["RX"]["channel"].is_null() == false){
        float rx_rate = config["USRPSettings"]["Multi-USRP"]["sampling_rate"].get<float>();
        rx_channel = config["USRPSettings"]["RX"]["channel"].get<size_t>();
        if (rx_rate <= 0.0) {
            std::cerr << "USRPHandler::set_sample_rate: Please specify a valid RX sample rate" << std::endl;
        }
        std::cout << "USRPHandler::set_sample_rate: Setting RX Rate: " << 
                    rx_rate/1e6 << " Msps..." << std::endl;
        usrp->set_rx_rate(rx_rate, rx_channel);
        std::cout << "USRPHandler::set_sample_rate: Actual RX Rate: " << 
                    usrp->get_rx_rate(rx_channel) / 1e6 << " Msps..." << std::endl;
    }
    else {
        std::cerr << "USRPHandler::set_sample_rate: No Rx sample rate in JSON";
    }
    //tx rate
    if (config["USRPSettings"]["Multi-USRP"]["sampling_rate"].is_null() == false &&
    config["USRPSettings"]["TX"]["channel"].is_null() == false){
        float tx_rate = config["USRPSettings"]["Multi-USRP"]["sampling_rate"].get<float>();
        tx_channel = config["USRPSettings"]["TX"]["channel"].get<size_t>();
        if (tx_rate <= 0.0) {
            std::cerr << "USRPHandler::set_sample_rate: Please specify a valid TX sample rate" << std::endl;
        }
        std::cout << "USRPHandler::set_sample_rate: Setting TX Rate: " << 
                    tx_rate/1e6 << " Msps..." << std::endl;
        usrp->set_tx_rate(tx_rate, tx_channel);
        std::cout << "USRPHandler::set_sample_rate: Actual TX Rate: " << 
                    usrp->get_tx_rate(tx_channel) / 1e6 << " Msps..." << std::endl <<std::endl;
    }
    else {
        std::cerr << "USRPHandler::set_sample_rate: No Tx sample rate in JSON";
    }
}

void USRPHandler::set_center_frequency(void){
    if (config["USRPSettings"]["Multi-USRP"]["center_freq"].is_null() == false &&
        config["USRPSettings"]["Multi-USRP"]["lo-offset"].is_null() == false)
    {
        //get desired center frequency and LO offset
        float center_freq = config["USRPSettings"]["Multi-USRP"]["center_freq"].get<float>();
        float lo_offset = config["USRPSettings"]["Multi-USRP"]["lo-offset"].get<float>();

        //setting Rx
        std::cout << "Setting Rx Freq: " << center_freq/1e6 << " MHz" << std::endl;
        std::cout << "Setting Rx Lo-offset: : " << lo_offset/1e6 << " MHz" << std::endl;
        uhd::tune_request_t rx_tune_request;
        rx_tune_request = uhd::tune_request_t(center_freq,lo_offset);
        if (config["USRPSettings"]["AdditionalSettings"]["int-n"].get<bool>() == true){
            rx_tune_request.args = uhd::device_addr_t("mode_n=integer");
        }
        usrp->set_rx_freq(rx_tune_request);
        std::cout << "Actual Rx Freq: " << usrp->get_rx_freq()/1e6 << " MHz" <<std::endl <<std::endl;


        //setting Tx
        std::cout << "Setting Tx Freq: " << center_freq/1e6 << " MHz" << std::endl;
        std::cout << "Setting Tx Lo-offset: : " << lo_offset/1e6 << " MHz" << std::endl;
        uhd::tune_request_t tx_tune_request;
        tx_tune_request = uhd::tune_request_t(center_freq,lo_offset);
        if (config["USRPSettings"]["AdditionalSettings"]["int-n"].get<bool>() == true){
            tx_tune_request.args = uhd::device_addr_t("mode_n=integer");
        }
        usrp->set_tx_freq(tx_tune_request);
        std::cout << "Actual Tx Freq: " << usrp->get_tx_freq()/1e6 << " MHz" <<std::endl <<std::endl;
    }
    else{
        std::cerr << "USRPHandler::set_center_frequency: center frequency and lo-offset not defined in JSON config file\n";
    }
    
}

void USRPHandler::set_rf_gain(void){
    
    //set Rx Gain
    if (config["USRPSettings"]["RX"]["gain"].is_null() == false)
    {
        double rx_gain = config["USRPSettings"]["RX"]["gain"].get<double>();
        std::cout << "USRPHandler::set_rf_gain: Setting Rx Gain: " << rx_gain << 
                    " dB" <<std::endl;
        usrp -> set_rx_gain(rx_gain,rx_channel);
        std::cout << "USRPHandler::set_rf_gain: Actual Rx Gain: " << usrp -> get_rx_gain(rx_channel) << 
                    " dB" <<std::endl;
    }
    
    //set Tx Gain
    if (config["USRPSettings"]["TX"]["gain"].is_null() == false)
    {
        double tx_gain = config["USRPSettings"]["TX"]["gain"].get<double>();
        std::cout << "USRPHandler::set_rf_gain: Setting Tx Gain: " << tx_gain << 
                    " dB" <<std::endl;
        usrp -> set_tx_gain(tx_gain,tx_channel);
        std::cout << "USRPHandler::set_rf_gain: Actual Tx Gain: " << usrp -> get_tx_gain(tx_channel) << 
                    " dB" <<std::endl <<std::endl;
    }
}

void USRPHandler::set_if_filter_bw(void){
    
    if(config["USRPSettings"]["Multi-USRP"]["IF_filter_bw"].is_null() == false){
        double if_filter_bw = config["USRPSettings"]["Multi-USRP"]["IF_filter_bw"].get<double>();

        //set Rx IF filter BW
        std::cout << "USRPHandler::set_if_filter_bw: Setting Rx Bandwidth: " <<
                        if_filter_bw/1e6 << " MHz" <<std::endl;
        usrp->set_rx_bandwidth(if_filter_bw);
        std::cout << "USRPHandler::set_if_filter_bw: Actual Rx Bandwidth: " <<
                        usrp->get_rx_bandwidth()/1e6 << " MHz" <<std::endl;
        
        //set Tx IF filter BW
        std::cout << "USRPHandler::set_if_filter_bw: Setting Tx Bandwidth: " <<
                        if_filter_bw/1e6 << " MHz" <<std::endl;
        usrp->set_tx_bandwidth(if_filter_bw);
        std::cout << "USRPHandler::set_if_filter_bw: Actual Tx Bandwidth: " <<
                        usrp->get_tx_bandwidth()/1e6 << " MHz" <<std::endl <<std::endl;
    }
}

void USRPHandler::set_antennas(void){
    //set Rx Antenna
    if(config["USRPSettings"]["RX"]["ant"].is_null() == false){
        std::string rx_ant = config["USRPSettings"]["RX"]["ant"].get<std::string>();
        std::cout << "USRPHandler::set_antennas: Setting Rx Antenna: " <<
                    rx_ant << std::endl;
        usrp->set_rx_antenna(rx_ant,rx_channel);
        std::cout << "USRPHandler::set_antennas: Actual Rx Antenna: " <<
                    usrp->get_rx_antenna(rx_channel) << std::endl;
    }

    //set Tx Antenna
    if(config["USRPSettings"]["TX"]["ant"].is_null() == false){
        std::string tx_ant = config["USRPSettings"]["TX"]["ant"].get<std::string>();
        std::cout << "USRPHandler::set_antennas: Setting Tx Antenna: " <<
                    tx_ant << std::endl;
        usrp->set_tx_antenna(tx_ant,tx_channel);
        std::cout << "USRPHandler::set_antennas: Actual Tx Antenna: " <<
                    usrp->get_tx_antenna(tx_channel) << std::endl <<std::endl;
    }
}

void USRPHandler::check_lo_locked(void){
    if(config["USRPSettings"]["AdditionalSettings"]["skip-lo"].get<bool>() == false){
        // Check Ref and LO Lock detect
        if(config["USRPSettings"]["Multi-USRP"]["ref"].is_null() == false){
            std::string ref = config["USRPSettings"]["Multi-USRP"]["ref"].get<std::string>();
            
            //check Tx sensor
            std::vector<std::string> sensor_names;
            sensor_names = usrp->get_tx_sensor_names(0);
            if (std::find(sensor_names.begin(), sensor_names.end(), "lo_locked")
                != sensor_names.end()) {
                uhd::sensor_value_t lo_locked = usrp->get_tx_sensor("lo_locked", 0);
                std::cout << "USRPHandler::check_lo_locked: Checking TX: " << lo_locked.to_pp_string()
                        << std::endl;
                UHD_ASSERT_THROW(lo_locked.to_bool());
            }

            //checking Rx sensor
            sensor_names = usrp->get_rx_sensor_names(0);
            if (std::find(sensor_names.begin(), sensor_names.end(), "lo_locked")
                != sensor_names.end()) {
                uhd::sensor_value_t lo_locked = usrp->get_rx_sensor("lo_locked", 0);
                std::cout << "USRPHandler::check_lo_locked: Checking RX: " << lo_locked.to_pp_string()
                        << std::endl;
                UHD_ASSERT_THROW(lo_locked.to_bool());
            }

            //checking for mimo or external references
            sensor_names = usrp->get_mboard_sensor_names(0);
            if ((ref == "mimo")
                and (std::find(sensor_names.begin(), sensor_names.end(), "mimo_locked")
                    != sensor_names.end())) {
                uhd::sensor_value_t mimo_locked = usrp->get_mboard_sensor("mimo_locked", 0);
                std::cout << boost::format("USRPHandler::check_lo_locked:Checking TX: %s ...") % mimo_locked.to_pp_string()
                        << std::endl;
                UHD_ASSERT_THROW(mimo_locked.to_bool());
            }
            if ((ref == "external")
                and (std::find(sensor_names.begin(), sensor_names.end(), "ref_locked")
                    != sensor_names.end())) {
                uhd::sensor_value_t ref_locked = usrp->get_mboard_sensor("ref_locked", 0);
                std::cout << boost::format("USRPHandler::check_lo_locked:Checking TX: %s ...") % ref_locked.to_pp_string()
                        << std::endl;
                UHD_ASSERT_THROW(ref_locked.to_bool());
            }
        }
        else{
            std::cout << "USRPHandler::check_lo_locked: ref is not defined in JSON\n\n";
        }
    }
    else{
        std::cout << "USRPHandler::check_lo_locked: skipping lo-locked check\n\n";
    }
}

void USRPHandler::init_multi_usrp(void){
    /*
    Purpose: initializes the USRP device
    */

    //create USRP device
    create_USRP_device();
    
    //set clock reference
    set_ref();

    //set subdevices
    set_subdevices();

    //set sampling rate
    set_sample_rate();

    //set the center frequency
    set_center_frequency();

    //set the RF gain
    set_rf_gain();

    //set the IF filter bandwidth
    set_if_filter_bw();
    
    //set the Tx and Rx antennas
    set_antennas();

    //wait for the specified setup time
    wait_for_setup_time();

    //check to confirm that the LO is locked
    check_lo_locked();

    //compute the frame start times
    init_frame_start_times();

    //initialize the stream arguments
    init_stream_args();
}

void USRPHandler::init_frame_start_times(void){
    
    //set stream start time
    if(config["USRPSettings"]["Multi-USRP"]["stream_start_time"].is_null() == false){
        stream_start_time = config["USRPSettings"]["Multi-USRP"]["stream_start_time"].get<double>();
    }
    else{
        std::cerr << "USRPHandler::init_frame_start_times: couldn't find stream start time in JSON" <<std::endl;
    }

    //set num_frames
    if(config["FMCWSettings"]["num_frames"].is_null() == false){
        num_frames = config["FMCWSettings"]["num_frames"].get<size_t>();
    }
    else{
        std::cerr << "USRPHandler::init_frame_start_times: couldn't find num_frames in JSON" <<std::endl;
    }

    //set frame_periodicity
    if(config["FMCWSettings"]["frame_periodicity_ms"].is_null() == false){
        frame_periodicity = config["FMCWSettings"]["frame_periodicity_ms"].get<double>() * 1e-3;
    }
    else{
        std::cerr << "USRPHandler::init_frame_start_times: couldn't find frame_periodicity_ms in JSON" <<std::endl;
    }

    //initialize the frame start times vector
    frame_start_times = std::vector<uhd::time_spec_t>(num_frames);
    std::cout << "USRPHandler::init_frame_start_times: computed start times: " << std::endl;
    for (size_t i = 0; i < num_frames; i++)
    {
        frame_start_times[i] = uhd::time_spec_t(stream_start_time + 
                        (frame_periodicity * static_cast<double>(i)));
        std::cout << frame_start_times[i].get_real_secs() << ", ";
    }
    std::cout << std::endl;
    
    //set the rx stream start offset
        /*NOTE: this was added because on some USRP devices, there appears to
        be a difference between when the receiver starts and the transmitter starts
        even when both devices are given the same start time
        */
    if(config["USRPSettings"]["RX"]["offset_us"].is_null() == false){
        rx_stream_start_offset = uhd::time_spec_t(
            config["USRPSettings"]["RX"]["offset_us"].get<double>() * 1e-6);
    }
    else{
        std::cerr << "USRPHandler::init_frame_start_times: couldn't find rx stream start time offset in JSON" <<std::endl;
    }
}

void USRPHandler::init_stream_args(void){

    std::string cpu_format = config["USRPSettings"]["Multi-USRP"]["cpufmt"].get<std::string>();
    std::string wirefmt = config["USRPSettings"]["Multi-USRP"]["wirefmt"].get<std::string>();
    
    //configure tx stream args
    std::vector<size_t> tx_channels(1,tx_channel);
    tx_stream_args = uhd::stream_args_t(cpu_format,wirefmt);
    tx_stream_args.channels = tx_channels;
    tx_stream = usrp -> get_tx_stream(tx_stream_args);
    tx_samples_per_buffer = tx_stream -> get_max_num_samps();
    std::cout << "USRPHandler::init_stream_args: tx_spb: " << tx_samples_per_buffer <<std::endl;
    
    //configure rx stream args
    std::vector<size_t> rx_channels(1,rx_channel);
    rx_stream_args = uhd::stream_args_t(cpu_format,wirefmt);
    rx_stream_args.channels = rx_channels;
    rx_stream = usrp -> get_rx_stream(rx_stream_args);
    rx_samples_per_buffer = rx_stream -> get_max_num_samps();
    std::cout << "USRPHandler::init_stream_args: tx_spb: " << tx_samples_per_buffer << 
        " rx_spb: " << rx_samples_per_buffer <<std::endl;
    std::cout << "USRPHandler::init_stream_args: initialized stream arguments" << std::endl << std::endl;
}

void USRPHandler::reset_usrp_clock(void){
    std::cout << "USRPHandler::reset_usrp_clock: setting device timestamp to 0" << 
                std::endl << std::endl;
    usrp -> set_time_now(uhd::time_spec_t(0.0));
    return;
}

void USRPHandler::load_Buffers(
    FMCW_Buffer<std::complex<float>> * new_tx_buffer,
    FMCW_Buffer<std::complex<float>> * new_rx_buffer){
        tx_buffer = new_tx_buffer;
        rx_buffer = new_rx_buffer;
    return;
}

void USRPHandler::stream_rx_frames(void){
    
    //determine the number of samples to be streamed in the frame
    size_t num_samps_per_buff = rx_buffer -> num_cols;
    size_t num_rows = rx_buffer -> num_rows;
    size_t total_samps = num_samps_per_buff * num_rows;
    size_t num_samps_received;

    //reset the overflow message
    overflow_detected = false;
    rx_first_buffer = true;

    //initialize the stream command
    uhd::stream_cmd_t rx_stream_cmd(uhd::stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_DONE);
    rx_stream_cmd.num_samps = total_samps;
    rx_stream_cmd.stream_now = false;

    for (size_t i = 0; i < num_frames; i++)
    {
        //set the time spec for the frame start
        rx_stream_cmd.time_spec = frame_start_times[i] + rx_stream_start_offset;

        //send the stream command
        rx_stream -> issue_stream_cmd(rx_stream_cmd);

        for (size_t j = 0; j < num_rows; j++)
        {
            //receive the data
            num_samps_received = rx_stream -> recv(
                            &(rx_buffer->buffer[j].front()),
                            num_samps_per_buff,rx_md,0.5,true);
            
            //check the metadata to confirm good receive
            if (num_samps_received != num_samps_per_buff){
                std::cerr << "USRPHandler::stream_rx_frame: Tried receiving " << num_samps_per_buff <<
                            " samples, but only received " << num_samps_received << std::endl;
            }
            check_rx_metadata(rx_md);

            //if an overflow was detected, the frame is bad, save what we had and start a new frame
            if (overflow_detected){
                std::cout << "USRPHandler::stream_rx_frames: Overflow detected on frame " <<
                            i + 1 << " cancelling frame and starting again" <<std::endl;
                //reset the overflow tag
                overflow_detected = false;
                break;
            }
        }
        rx_buffer -> save_to_file();   
    }
    return;
}

void USRPHandler::check_rx_metadata(uhd::rx_metadata_t & rx_md){

    //create a unique lock for managing outputs using std::cout
    std::unique_lock<std::mutex> cout_unique_lock(cout_mutex, std::defer_lock);

    if(rx_first_buffer){
        cout_unique_lock.lock();
        std::cout << "USRPHandler::check_rx_metadata: start of burst metadata: " << std::endl <<
                    rx_md.to_pp_string(simplified_metadata) << std::endl <<std::endl;
        rx_first_buffer = false;
        cout_unique_lock.unlock();
    }
    if(rx_md.end_of_burst && rx_md.has_time_spec && not simplified_metadata) {
        cout_unique_lock.lock();
        std::cout << "USRPHandler::check_rx_metadata: end of burst occurred at " <<
                    rx_md.time_spec.get_real_secs() << " s" <<std::endl;
        cout_unique_lock.unlock();
    }
    if (rx_md.error_code == uhd::rx_metadata_t::ERROR_CODE_TIMEOUT) {
        std::cout << "USRPHandler::stream_rx_frame: Timeout while streaming" << std::endl;
    }
    if (rx_md.error_code == uhd::rx_metadata_t::ERROR_CODE_OVERFLOW) {
        if (not overflow_detected) {
            overflow_detected = true;
            std::cerr <<    
                        "Got an overflow indication. Please consider the following:\n" <<
                        "  Your write medium must sustain a rate of " << 
                        (usrp->get_rx_rate(rx_channel) * sizeof(std::complex<float>) / 1e6) <<"MB/s.\n" <<
                        "  Dropped samples will not be written to the file.\n" <<
                        "  Please modify this example for your purposes.\n" <<
                        "  This message will not appear again.\n"; 
        }
    }
    if (rx_md.error_code != uhd::rx_metadata_t::ERROR_CODE_NONE) {
        std::stringstream error;
        error << "USRPHandler::stream_rx_frame: Receiver error: " << rx_md.strerror();
        std::cerr << error.str() << std::endl;
    }
    return;
}

void USRPHandler::stream_tx_frames(void){

    //create a unique lock for managing outputs using std::cout
    std::unique_lock<std::mutex> cout_unique_lock(cout_mutex, std::defer_lock);
    
    //determine the number of samples to be streamed in the frame
    size_t num_samps_per_buff = tx_buffer -> num_cols;
    size_t num_rows = tx_buffer -> num_rows;
    size_t total_samps = num_samps_per_buff * num_rows;
    size_t num_samps_sent;

    cout_unique_lock.lock();
    std::cout << "USRPHandler::stream_tx_frame: streaming frame starting at : " <<
                frame_start_times[0].get_real_secs() << " s" << std::endl;
    cout_unique_lock.unlock();

    //stream desired number of frames
    for (size_t i = 0; i < num_frames; i++)
    {
        //initialize the metadata
        tx_md.has_time_spec = true;
        tx_md.time_spec = frame_start_times[i];
        tx_md.start_of_burst = false;
        tx_md.end_of_burst = false;

        if (not simplified_metadata && i > 0)
        {
            cout_unique_lock.lock();
            std::cout << "USRPHandler::stream_tx_frame: streaming frame starting at : " <<
                        tx_md.time_spec.get_real_secs() << " s" << std::endl;
            cout_unique_lock.unlock();
        }
        
        //stream the desired number of chirps
        for (size_t j = 0; j < num_rows; j++)
        {
            num_samps_sent = tx_stream -> send(
                        &(tx_buffer -> buffer[j].front()),
                        num_samps_per_buff,
                        tx_md,0.5);
            
            tx_md.start_of_burst = false;
            tx_md.has_time_spec = false;

            //confirm that sent correct amount of samples
            if (num_samps_sent != num_samps_per_buff){
                std::cerr << "USRPHandler::stream_tx_frame: Tried sending " << num_samps_per_buff <<
                            " samples, but only sent " << num_samps_sent << std::endl;
            }
        }

        // send a mini EOB packet
        tx_md.end_of_burst = true;
        tx_stream->send("", 0, tx_md);

    }
    tx_stream_complete = true;
}

void USRPHandler::check_tx_async_messages(void){
    
    //create a unique lock for managing outputs using std::cout
    std::unique_lock<std::mutex> cout_unique_lock(cout_mutex, std::defer_lock);
    
    bool exit_flag = false;
    while (true) {
        if (tx_stream_complete) {
            exit_flag = true;
        }

        if (not tx_stream->recv_async_msg(tx_async_md)) {
            if (exit_flag == true){
                cout_unique_lock.lock();
                std::cout << "USRPHandler::check_tx_async_messages: exiting async handler due to exit flag" <<std::endl;
                cout_unique_lock.unlock();
                return;
            }
            continue;
        }

        // handle the error codes
        switch (tx_async_md.event_code) {
            case uhd::async_metadata_t::EVENT_CODE_BURST_ACK:
                //std::cout << "USRPHandler::check_tx_async_messages: exiting async handler due to end of burst" <<std::endl;
                if (tx_async_md.has_time_spec && not simplified_metadata){
                    cout_unique_lock.lock();
                    std::cout << "USRPHandler::check_tx_async_messages: end of burst occurred at " <<
                            tx_async_md.time_spec.get_real_secs() << " s" << std::endl;
                    cout_unique_lock.unlock();
                }
                continue;
                //return;

            case uhd::async_metadata_t::EVENT_CODE_UNDERFLOW:
            case uhd::async_metadata_t::EVENT_CODE_UNDERFLOW_IN_PACKET:
                std::cerr << "USRPHandler::check_tx_async_messages: Underflow Detected" << std::endl;
                break;

            case uhd::async_metadata_t::EVENT_CODE_SEQ_ERROR:
            case uhd::async_metadata_t::EVENT_CODE_SEQ_ERROR_IN_BURST:
                std::cerr << "USRPHandler::check_tx_async_messages: Packet Loss Detected" << std::endl;
                break;

            default:
                std::cerr <<  "USRPHandler::check_tx_async_messages: Event code: " << tx_async_md.event_code
                          << std::endl;
                std::cerr << "Unexpected event on async recv, continuing..." << std::endl;
                break;
        }
    }
}

void USRPHandler::stream_frames(void){
    //set the start time
    reset_usrp_clock();

    //create transmit thread
    tx_stream_complete = false;
    std::thread transmit_thread([&]() {
        stream_tx_frames();
    });

    //create transmit async handler
    std::thread transmit_async_thread([&]() {
        check_tx_async_messages();
    });

    //process receive frame
    stream_rx_frames();
    

    //wait for transmit thread to finish
    transmit_thread.join();
    transmit_async_thread.join();
    std::cout << "USRPHandler::stream_frame: Complete" << std::endl << std::endl;
}