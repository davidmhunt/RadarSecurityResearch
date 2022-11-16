#ifndef SPECTROGRAMHANDLER
#define SPECTROGRAMHANDLER

    //include header files
    #include "pocketfft/pocketfft_hdronly.h"

    //C standard libraries
    #include <iostream>
    #include <cstdlib>
    #include <string>
    #include <complex.h>
    #include <vector>
    #include <csignal>
    #include <thread>

    #define _USE_MATH_DEFINES
    #include <cmath>

    //including buffer handler
    #include "../BufferHandler.hpp"

    //include the JSON handling capability
    #include <nlohmann/json.hpp>

    //include the required headers from EIGEN
    #include "Eigen/Dense"


    using namespace Buffers;
    using namespace pocketfft;
    using json = nlohmann::json;
    using namespace Eigen;

    namespace SpectrogramHandler_namespace {

        template<typename data_type>
        class SpectrogramHandler
        {
        private:

            json config;
            
            //size parameters
            size_t samples_per_sampling_window;
            size_t fft_size;
            size_t num_rows_rx_signal; //for the received signal
            size_t samples_per_buffer_rx_signal; //the spb for the received signal
            size_t num_rows_spectrogram; //for the reshaped array (in preparation for spectogram)
            size_t num_samples_rx_signal;
            size_t num_samples_per_spectrogram; //for the reshaped spectrogram

            //fft parameters
            shape_t shape;
            stride_t stride;
            shape_t axes;

            //peak_detection_parameters
            data_type peak_detection_threshold;

            //frequency and timing variables
            data_type FMCW_sampling_rate;
            data_type frequency_resolution;
            data_type frequency_sampling_period;
            data_type detected_time_offset;
            std::vector<data_type> frequencies;
            std::vector<data_type> times;

            //clustering parameters
            size_t min_points_per_chirp;
            int max_cluster_index;

            //timing parameters
            data_type detection_start_time_us;
            const data_type c = 2.99792458e8;
            size_t chirp_tracking_num_captured_chirps;
            data_type chirp_tracking_average_slope; //in MHz/us
            data_type chirp_tracking_average_chirp_duration; //in us
            size_t frame_tracking_num_captured_frames;
            data_type frame_tracking_average_frame_duration;
            data_type frame_tracking_average_chirp_duration;
            data_type frame_tracking_average_chirp_slope;


        
        public:

            //buffers used

                //rx signal buffer
                Buffer_2D<std::complex<data_type>> rx_buffer;

                //reshaped and window buffer (ready for FFT)
                Buffer_2D<std::complex<data_type>> reshaped__and_windowed_signal_for_fft;

                //hanning window
                Buffer_1D<std::complex<data_type>> hanning_window;

                //computed FFT vector
                Buffer_2D<std::complex<data_type>> computed_fft;

                //generated spectrogram
                Buffer_2D<data_type> generated_spectrogram;

                //spectrogram points
                Buffer_1D<data_type> spectrogram_points_values;
                Buffer_1D<size_t> spectrogram_points_indicies;

                //frequency and timing bins
                Buffer_1D<data_type> detected_times;
                Buffer_1D<data_type> detected_frequencies;

                //cluster indicies
                Buffer_1D<int> cluster_indicies;

                //buffers for detected chirps
                Buffer_1D<data_type> detected_slopes;
                Buffer_1D<data_type> detected_intercepts;

                //buffer for tracking victim frames
                Buffer_2D<data_type> captured_frames; //colums as follows: duration, number of chirps, average slope, average chirp duration, start time, next predicted frame start time
            
        public:

            /**
             * @brief Construct a new Spectrogram Handler object using a config file
             * 
             * @param json_config a json object with configuration information 
             */
            SpectrogramHandler(json json_config): config(json_config){
                if (check_config())
                {
                    initialize_spectrogram_params();
                    initialize_fft_params();
                    initialize_buffers();
                    initialize_hanning_window();
                    initialize_freq_and_timing_bins();
                    initialize_clustering_params();
                    initialize_chirp_and_frame_tracking();
                }
                
                
            }

            ~SpectrogramHandler() {};

            /**
             * @brief Check the json config file to make sure all necessary parameters are included
             * 
             * @return true - JSON is all good and has required elements
             * @return false - JSON is missing certain fields
             */
            bool check_config(){
                bool config_good = true;
                //check sampling rate
                if(config["USRPSettings"]["Multi-USRP"]["sampling_rate"].is_null()){
                    std::cerr << "SpectrogramHandler::check_config: no sampling_rate in JSON" <<std::endl;
                    config_good = false;
                }
                
                //check the samples per buffer
                if(config["USRPSettings"]["RX"]["spb"].is_null()){
                    std::cerr << "SpectrogramHandler::check_config: Rx spb not specified" <<std::endl;
                    config_good = false;
                }

                //check the minimum recording time
                if(config["SensingSubsystemSettings"]["min_recording_time_ms"].is_null()){
                    std::cerr << "SpectrogramHandler::check_config: min recording time not specified" <<std::endl;
                    config_good = false;
                }

                if(config["SensingSubsystemSettings"]["spectogram_peak_detection_threshold_dB"].is_null()){
                    std::cerr << "SpectrogramHandler::check_config: spectogram peak detection threshold not specified" <<std::endl;
                    config_good = false;
                }

                if(config["SensingSubsystemSettings"]["min_points_per_chirp"].is_null()){
                    std::cerr << "SpectrogramHandler::check_config: min number of points per chirp not specified" <<std::endl;
                    config_good = false;
                }

                if(config["SensingSubsystemSettings"]["num_victim_frames_to_capture"].is_null()){
                    std::cerr << "SpectrogramHandler::check_config: num_victim_frames_to_capture not specified" <<std::endl;
                    config_good = false;
                }

                return config_good;
            }
            
            /**
             * @brief Initialize spectrogram parameters based on the configuration file
             * 
             */
            void initialize_spectrogram_params(){

                //specify the sampling rate
                FMCW_sampling_rate = config["USRPSettings"]["Multi-USRP"]["sampling_rate"].get<data_type>();
                samples_per_buffer_rx_signal = config["USRPSettings"]["RX"]["spb"].get<size_t>();

                //determine the frequency sampling period based on the sampling rate
                data_type freq_sampling_period;
                if (FMCW_sampling_rate > 500e6)
                {
                    freq_sampling_period = 0.5e-6;
                }
                else{
                    freq_sampling_period = 2e-6;
                }
                
                //determine the number of samples per sampling window
                samples_per_sampling_window = static_cast<size_t>(std::ceil(FMCW_sampling_rate * freq_sampling_period));

                //determine the fft size
                fft_size = static_cast<size_t>(
                                std::pow(2,std::floor(
                                    std::log2(static_cast<data_type>(samples_per_sampling_window)))));

                //recompute the actual frequency sampling window using the number of samples 
                // per sampling window
                freq_sampling_period = static_cast<data_type>(samples_per_sampling_window) /
                                            FMCW_sampling_rate;
                
                //determine the number of rows in the rx signal buffer
                data_type row_period = static_cast<data_type>(samples_per_buffer_rx_signal)/FMCW_sampling_rate;
                data_type min_recording_time_ms = config["SensingSubsystemSettings"]["min_recording_time_ms"].get<data_type>();
                num_rows_rx_signal = static_cast<size_t>(std::ceil((min_recording_time_ms * 1e-3)/row_period));

                //determine the number of samples per rx signal
                num_samples_rx_signal = num_rows_rx_signal * samples_per_buffer_rx_signal;

                //determine the number of rows in the spectrogram
                num_rows_spectrogram = (num_samples_rx_signal / samples_per_sampling_window);
                num_samples_per_spectrogram = num_rows_spectrogram * samples_per_sampling_window;

                //set the peak detection threshold for the spectogram
                peak_detection_threshold = config["SensingSubsystemSettings"]["spectogram_peak_detection_threshold_dB"].get<data_type>();
            }


            /**
             * @brief initialize the parameters needed by the fft computation
             * 
             */
            void initialize_fft_params(){
                
                shape = {fft_size};
                stride = {sizeof(std::complex<data_type>)};
                axes = {0};
            }

            /**
             * @brief initialize all buffers used by the spectrogram handler
             * 
             */
            void initialize_buffers(){
                
                //rx_buffer
                rx_buffer = Buffer_2D<std::complex<data_type>>(num_rows_rx_signal,samples_per_buffer_rx_signal);

                //reshaped and sampled signal
                reshaped__and_windowed_signal_for_fft = Buffer_2D<std::complex<data_type>>(num_rows_spectrogram,fft_size);
                
                //window to apply
                hanning_window = Buffer_1D<std::complex<data_type>>(fft_size);
                
                //fft/spectrogram generation
                computed_fft = Buffer_2D<std::complex<data_type>>(num_rows_spectrogram,fft_size);
                generated_spectrogram = Buffer_2D<data_type>(num_rows_spectrogram,fft_size);

                //getting the points from the spectrogram
                spectrogram_points_values = Buffer_1D<data_type>(num_rows_spectrogram);
                spectrogram_points_indicies = Buffer_1D<size_t>(num_rows_spectrogram);

                //tracking detected times
                detected_times = Buffer_1D<data_type>(num_rows_spectrogram);
                detected_frequencies = Buffer_1D<data_type>(num_rows_spectrogram);

                //clustering indicies
                cluster_indicies = Buffer_1D<int>(num_rows_spectrogram);

                //detected slopes and intercepts
                detected_slopes = Buffer_1D<data_type>(num_rows_spectrogram);
                detected_intercepts = Buffer_1D<data_type>(num_rows_spectrogram);

                //captured frames
                size_t max_frames_to_capture = 
                    config["SensingSubsystemSettings"]["num_victim_frames_to_capture"].get<size_t>();
                captured_frames = Buffer_2D<data_type>(max_frames_to_capture,6);
            }

            /**
             * @brief Initialize the frequency and timing bins
             * 
             */
            void initialize_freq_and_timing_bins(){
                //initialize the frequency parameters and buffers
                frequency_resolution = FMCW_sampling_rate * 1e-6 /
                            static_cast<data_type>(fft_size);

                frequencies = std::vector<data_type>(fft_size,0);

                for (size_t i = 0; i < fft_size; i++)
                {
                    frequencies[i] = frequency_resolution * static_cast<data_type>(i);
                }

                //initialize the timing parameters and buffers
                    //compute the timing offset
                    frequency_sampling_period = 
                            static_cast<data_type>(samples_per_sampling_window)/
                                (FMCW_sampling_rate * 1e-6);
                    
                    detected_time_offset = frequency_sampling_period * 
                                static_cast<data_type>(fft_size) / 2 /
                                static_cast<data_type>(samples_per_sampling_window);
                    
                    //create the times buffer
                    times = std::vector<data_type>(num_rows_spectrogram,0);
                
                    for (size_t i = 0; i < num_rows_spectrogram; i++)
                    {
                        times[i] = (frequency_sampling_period *
                                    static_cast<data_type>(i)) + detected_time_offset;
                    }
            }

            /**
             * @brief Initialize the clustering parameters
             * 
             */
            void initialize_clustering_params(){

                // get the minimum number of points per chirp from the JSON file
                min_points_per_chirp = config["SensingSubsystemSettings"]["min_points_per_chirp"].get<size_t>();

                //initialize the maximum cluster index to be zero
                max_cluster_index = 0;
            }


            /**
             * @brief Computes a hanning window of a given size for use in the spectogram generation
             * 
             */
            void initialize_hanning_window() {
                data_type M = static_cast<data_type>(fft_size);
                for (size_t i = 0; i < fft_size; i++)
                {
                    data_type n = static_cast<data_type>(i);
                    //data_type x = 2 * M_PI * n / (M - 1);
                    //data_type cos_x = cos(x);

                    data_type hann = 0.5 * (1 - cos(2 * M_PI * n / (M - 1)));

                    hanning_window.buffer[i] = std::complex<data_type>(hann);
                } 
            }


            void initialize_chirp_and_frame_tracking(){
                //chirp tracking
                chirp_tracking_num_captured_chirps = 0;
                chirp_tracking_average_slope = 0;
                chirp_tracking_average_chirp_duration = 0;

                //frame tracking
                frame_tracking_num_captured_frames = 0;
                frame_tracking_average_frame_duration = 0;
                frame_tracking_average_chirp_duration = 0;
                frame_tracking_average_chirp_slope = 0;
            }

            /**
             * @brief Set the detection start time us object
             * 
             * @param start_time_us the time that the first sample in the rx_buffer occured at (us)
             * @param victim_distance_m the range of the victim (m)
             */
            void set_detection_start_time_us(data_type start_time_us, data_type victim_distance_m = 0){
                data_type distance_delay_us = (victim_distance_m / c) * 1e6;
                detection_start_time_us = start_time_us - distance_delay_us;
            }

            /**
             * @brief Process the received signal
             * 
             */
            void process_received_signal(){
                load_and_prepare_for_fft();
                compute_ffts();
                detect_peaks_in_spectrogram();
                compute_clusters();
                compute_linear_model();
                compute_victim_parameters();
            }

            /**
             * @brief Loads a received signal, reshapes, and prepares it 
             * for fft processing. Signal is saved in 
             * reshaped__and_windowed_signal_for_fft buffer
             * 
             */
            void load_and_prepare_for_fft(){
                
                //get dimmensions of rx_buffer array
                size_t m = rx_buffer.buffer.size(); //rows
                size_t n = rx_buffer.buffer[0].size(); //cols

                //initialize variables for reshaping
                size_t from_r;
                size_t from_c;

                //initialize variable to determine the coordinate in the received signal for a row/col index in reshpaed signal
                size_t k;

                for (size_t i = 0; i < num_rows_spectrogram; i++)
                {
                    for (size_t j = 0; j < fft_size; j++)
                    {
                        //for a given row,col index in the reshaped signal, determine the coordinate in the rx_buffer
                        size_t k = i * samples_per_sampling_window + j;

                        //indicies in rx_buffer
                        from_r = k/n;
                        from_c = k % n;

                        if (from_r >= m)
                        {
                            reshaped__and_windowed_signal_for_fft.buffer[i][j] = 0;
                        }
                        else{
                            reshaped__and_windowed_signal_for_fft.buffer[i][j] = rx_buffer.buffer[from_r][from_c] * hanning_window.buffer[j];
                        }
                    } 
                }
                return;
            }

            /**
             * @brief Compute the fft for the desired rows in the reshaped and windowed
             * signal buffer
             * 
             * @param start_idx the index of the row in the reshaped_and_windowed_signal buffer
             * to start computing ffts for
             * @param end_idx the index of the row in the reshaped_and_windowed_signal buffer to
             * end computing ffts for
             */
            void compute_ffts(size_t start_idx = 0, size_t end_idx = 0){
              
              //if the end_idx is zero (default condition), set it to be the num_rows_spectrogram
              if (end_idx == 0)
              {
                end_idx = num_rows_spectrogram;
              }
              
              
              //compute the fft and generate the spectrogram for the given rows
              for (size_t i = start_idx; i < end_idx; i++)
                {
                    c2c(shape, stride, stride, axes, FORWARD,
                        reshaped__and_windowed_signal_for_fft.buffer[i].data(),
                        computed_fft.buffer[i].data(), (data_type) 1.);

                    //convert to dB
                    for (size_t j = 0; j < fft_size; j++)
                    {
                        generated_spectrogram.buffer[i][j] = 10 * std::log10(std::abs(
                            computed_fft.buffer[i][j]
                        ));
                    }  
                }
            }

            /**
             * @brief CURRENTLY BROKEN compute ffts using multiple threads
             * (calls compute_ffts() from multiple threads)
             * 
             * @param num_threads the number of threads to use for FFT computations
             */
            void compute_ffts_multi_threaded(size_t num_threads = 1){
                //initialize a vector of threads
                std::vector<std::thread> threads;
                size_t rows_per_thread = num_rows_spectrogram / num_threads;
                size_t start_row;
                size_t end_row;

                //spawn multiple threads
                for (size_t thread_num = 0; thread_num < num_threads; thread_num++)
                {   
                    //determine the start and end row for each thread
                    start_row = thread_num * rows_per_thread;

                    if (thread_num == (num_threads - 1)){
                        end_row = num_rows_spectrogram;
                    }
                    else{
                        end_row = (thread_num + 1) * rows_per_thread;
                    }

                    //spawn the thread
                    threads.push_back(std::thread([&] () {
                        compute_ffts(start_row,end_row);
                    }));
                }

                //join the threads
                for (auto& t : threads){
                    t.join();
                }
            }
        
            /**
             * @brief Compute the maximum value and its index in the given signal
             * 
             * @param signal the signal to determine the maximum value of
             * @return std::tuple<data_type,size_t> the maximum value and index of the maximum value in the signal
             */
            std::tuple<data_type,size_t> compute_max_val(std::vector<data_type> & signal){
                //set asside a variable for the value and index of the max value
                data_type max = signal[0];
                size_t idx = 0;

                for (size_t i = 0; i < signal.size(); i++)
                {
                    if (signal[i] > max)
                    {
                        max = signal[i];
                        idx = i;
                    }
                }
                return std::make_tuple(max,idx);
            }

            /**
             * @brief Detect the peaks in the computed spectrogram 
             * and saves the results in the detected_times and detected_frequenies array
             * 
             */
            void detect_peaks_in_spectrogram(){
                //initialize variable to store results from compute_max_val
                std::tuple<data_type,size_t> max_val_and_idx;
                data_type max_val;
                size_t idx;

                //variable to track the absolute maximum value detected in the spectrogram
                data_type absolute_max_val = generated_spectrogram.buffer[0][0];

                //clear the detected times and frequencies buffers
                detected_times.clear();
                detected_frequencies.clear();

                //get the maximum_value from each computed_spectrogram
                for (size_t i = 0; i < num_rows_spectrogram; i++)
                {
                    max_val_and_idx = compute_max_val(generated_spectrogram.buffer[i]);
                    max_val = std::get<0>(max_val_and_idx);
                    idx = std::get<1>(max_val_and_idx);
                    spectrogram_points_values.buffer[i] = max_val;
                    spectrogram_points_indicies.buffer[i] = idx;

                    //update the max value
                    if (max_val > absolute_max_val)
                    {
                        absolute_max_val = max_val;
                    }
                }
                
                data_type threshold = absolute_max_val - peak_detection_threshold;
                //go through the spectrogram_points and zero out the points below the threshold
                for (size_t i = 0; i < num_rows_spectrogram; i ++){
                    if (spectrogram_points_values.buffer[i] > threshold)
                    {
                        detected_times.push_back(times[i]);
                        detected_frequencies.push_back(
                            frequencies[
                                spectrogram_points_indicies.buffer[i]]);
                    }
                }
                return;
            }

            /**
             * @brief identify the clusters from the detected times and frequencies
             * 
             */
            void compute_clusters(){

                //declare support variables
                int chirp = 1;
                size_t chirp_start_idx = 0;
                size_t num_points_in_chirp = 1;
                size_t set_val; // use

                //variable to track the total number of detected points
                size_t num_detected_points = detected_frequencies.num_samples;

                //go through the points and determine the clusters
                for (size_t i = 1; i < num_detected_points; i++)
                {
                    if ((detected_frequencies.buffer[i] - detected_frequencies.buffer[i-1]) > 0)
                    {
                        num_points_in_chirp += 1;
                    }
                    else{
                        if(num_points_in_chirp >= min_points_per_chirp){
                            cluster_indicies.set_val_at_indicies(chirp,chirp_start_idx,i);
                            
                            //start tracking the next chirp
                            chirp += 1;
                        }
                        else{
                            cluster_indicies.set_val_at_indicies(-1,chirp_start_idx,i);
                        }

                        //reset support variables for tracking new chirp
                        chirp_start_idx = i;
                        num_points_in_chirp = 1;
                    }
                }

                //check the last point
                if (num_points_in_chirp >= min_points_per_chirp)
                {
                    cluster_indicies.set_val_at_indicies(chirp,chirp_start_idx,num_detected_points);
                }
                else{
                    cluster_indicies.set_val_at_indicies(-1,chirp_start_idx,num_detected_points);
                }

                //set the maximum cluster index
                max_cluster_index = chirp;
                

                //set the remaining samples in the cluster array to zero
                for (size_t i = num_detected_points; i < num_rows_spectrogram; i++)
                {
                    cluster_indicies.buffer[i] = 0;
                }
            }

            /**
             * @brief Compute the linear model from the clustered times and frequencies
             * 
             */
            void compute_linear_model(){

                //clear the detected slopes and intercepts arrays
                detected_slopes.clear();
                detected_intercepts.clear();
                
                //initialize the b vector
                Eigen::Vector<data_type,2> b;
                size_t n = 0;

                //initialize variables to find indicies for each cluster index
                std::vector<size_t> indicies;
                size_t search_start = 0;


                for (int i = 1; i <= max_cluster_index; i++)
                {
                    indicies = cluster_indicies.find_indicies_with_value(i,search_start,true);
                    n = indicies.size();

                    //initialize Y and X matricies
                    Eigen::Matrix<data_type,Dynamic,2> X(n,2);
                    Eigen::Vector<data_type,Dynamic> Y(n);

                    for (size_t j = 0; j < n; j++)
                    {
                        Y(j) = detected_frequencies.buffer[indicies[j]];
                        X(j,0) = 1;
                        X(j,1) = detected_times.buffer[indicies[j]];
                    }

                    //solve the linear equation
                    b = (X.transpose() * X).ldlt().solve(X.transpose() * Y);
                    detected_slopes.push_back(b(1));
                    detected_intercepts.push_back(-b(0)/b(1) + detection_start_time_us);
                }
            }

            /**
             * @brief Compute the victim parameters from the detected signal
             * 
             */
            void compute_victim_parameters(){
                
                //determine number of chirps detected
                chirp_tracking_num_captured_chirps = detected_slopes.num_samples;
                
                //compute average chirp slope
                data_type sum = 0;
                for (size_t i = 0; i < chirp_tracking_num_captured_chirps; i++)
                {
                    sum += detected_slopes.buffer[i];
                }
                chirp_tracking_average_slope = sum/
                        static_cast<data_type>(chirp_tracking_num_captured_chirps);

                //compute average chirp intercept
                chirp_tracking_average_chirp_duration = 
                        (detected_intercepts.buffer[chirp_tracking_num_captured_chirps - 1]
                        - detected_intercepts.buffer[0])
                        / static_cast<data_type>(chirp_tracking_num_captured_chirps - 1);
                
                //increment frame counter
                frame_tracking_num_captured_frames += 1;

                //save num captured chirps, average slope, average chirp duration, and start time
                captured_frames.buffer[frame_tracking_num_captured_frames - 1][1] = static_cast<data_type>(chirp_tracking_num_captured_chirps);
                captured_frames.buffer[frame_tracking_num_captured_frames - 1][2] = chirp_tracking_average_slope;
                captured_frames.buffer[frame_tracking_num_captured_frames - 1][3] = chirp_tracking_average_chirp_duration;
                captured_frames.buffer[frame_tracking_num_captured_frames - 1][4] = detected_intercepts.buffer[0]; //time of first chirp

                //TODO: compute precise frame start time

                //compute frame duration, average frame duration, and predict next frame
                if(frame_tracking_num_captured_frames > 1){
                    //compute and save frame duration
                    captured_frames.buffer[frame_tracking_num_captured_frames - 1][0] =
                        captured_frames.buffer[frame_tracking_num_captured_frames - 1][4]
                        - captured_frames.buffer[frame_tracking_num_captured_frames - 2][4];
                    
                    // compute average frame duration
                    frame_tracking_average_frame_duration = 
                        (captured_frames.buffer[frame_tracking_num_captured_frames - 1][4] -
                        captured_frames.buffer[0][4])
                        / static_cast<data_type>(frame_tracking_num_captured_frames - 1);
                    
                    //predict next frame
                    captured_frames.buffer[frame_tracking_num_captured_frames - 1][5] = 
                        captured_frames.buffer[frame_tracking_num_captured_frames - 1][4]
                        + frame_tracking_average_frame_duration;
                }
                else{
                    captured_frames.buffer[frame_tracking_num_captured_frames - 1][0] = 0;
                    captured_frames.buffer[frame_tracking_num_captured_frames - 1][5] = 0;
                }

                //compute average chirp slope across all frames
                data_type sum_slopes = 0; //sum of all average chirp slopes
                data_type sum_count = 0; //sum of total number of chirps detected
                for (size_t i = 0; i < frame_tracking_num_captured_frames; i++)
                {
                    sum_slopes += captured_frames.buffer[i][2] * (captured_frames.buffer[i][1] - 1);
                    sum_count += captured_frames.buffer[i][1] - 1;
                }
                frame_tracking_average_chirp_slope = sum_slopes/sum_count;
                
                //compute average chirp curation across all frames
                data_type sum_durations = 0; //sum of all average chirp durations
                sum_count = 0; //sum of total number of chirps detected
                for (size_t i = 0; i < frame_tracking_num_captured_frames; i++)
                {
                    sum_durations += captured_frames.buffer[i][3] * (captured_frames.buffer[i][1] - 1);
                    sum_count += captured_frames.buffer[i][1] - 1;
                }
                frame_tracking_average_chirp_duration = sum_durations/sum_count;
            }
        };
    }


#endif