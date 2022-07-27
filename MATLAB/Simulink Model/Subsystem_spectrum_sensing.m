classdef Subsystem_spectrum_sensing < handle
    %UNTITLED Summary of this class goes here
    %   Detailed explanation goes here

    properties
        
        FMCW_sample_rate_Msps

        %struct to hold all of the spectogram parameters
        spectogram_params

        %structs to hold parameters for chirp and frame tracking
        chirp_tracking
        frame_tracking

        %structs for peak detection when evaluating the spectograms
        peak_detection_params

        %structs for clustering and linear model detection
        clustering_params
        
        %struct for adding a debugging capability
        Debugger

        %parameters used for plotting if desired
        plot_params
        
        %other support variables used by the sensing subsystem
        spectogram_points
        previous_spectogram_points
        sampling_window_count

        %other variables used to log progress and validate the simulink
        %model
        received_signal
        reshaped_signal
        reshaped_signal_for_fft
        windowed_signal
        generated_spectogram
        combined_spectogram
        detected_times
        detected_frequencies
        detected_chirps
        
        %variables used by the clustering algorithm
        idx
        corepts




    end

    methods
        function obj = Subsystem_spectrum_sensing()

        end

        function initialize_spectrum_sensing_parameters(obj,FMCW_sample_rate_Msps)
        %{
            Purpose: configures all of the spectrum_sensing parameters
                except for the debugger (done separately as this is a separate
                functionality)
            Inputs: 
                FMCW_sample_rate_Msps: the sampling rate of the spectrum
                    sensing subsystem
        %}
            obj.initialize_spectogram_params(FMCW_sample_rate_Msps);
            obj.initialize_chirp_and_frame_tracking();
            obj.initialize_plot_params(FMCW_sample_rate_Msps);
            obj.initialize_peak_detection_params();
            obj.initialize_clustering_params();

            %initialize the other miscellaneous parameters
            %initialize array to hold the current and previously measured spectrum
            obj.spectogram_points = [];
            obj.previous_spectogram_points = [];
            obj.combined_spectogram = [];
            obj.detected_times = [];
            obj.detected_times = [];
            obj.detected_chirps = zeros(obj.clustering_params.max_num_clusters,2);
            
            %initialize counter to keep track of how many full iterations of the while loop
            %have occurred
            obj.sampling_window_count = 0;
        end

        function  initialize_spectogram_params(obj,FMCW_sample_rate_Msps)
            %{
                Purpose: initializes the default settings for the
                    spectogram_params property of the class
                Inputs: 
                    FMCW_sample_rate_Msps: the sampling rate of the
                        spectrum sensing module in MSPS
            %}

            obj.spectogram_params.fft_size = 64;
            
            obj.spectogram_params.freq_sampling_period_us = 2; %sample frequency every x us 
            obj.spectogram_params.num_samples_per_sampling_window = ...
                ceil(obj.spectogram_params.freq_sampling_period_us * FMCW_sample_rate_Msps);

            %since the frequency sampling period may not have been
            %perfectly divisible by the FMCW sampling rate, adjust it so
            %that it is now divisible
            obj.spectogram_params.freq_sampling_period_us = ... 
                obj.spectogram_params.num_samples_per_sampling_window / FMCW_sample_rate_Msps;

            %continue setting the parameters
            obj.spectogram_params.num_freq_spectrum_samples_per_spectogram = 25;
            obj.spectogram_params.num_ADC_samples_per_spectogram = ...
                obj.spectogram_params.num_freq_spectrum_samples_per_spectogram * ...
                obj.spectogram_params.num_samples_per_sampling_window;
            
            %added a computation for the timing offset when computing the point values
            obj.spectogram_params.detected_time_offset = ...
                obj.spectogram_params.freq_sampling_period_us * ...
                obj.spectogram_params.fft_size / 2 /...
                obj.spectogram_params.num_samples_per_sampling_window;
            
            
            %compute the window to be used when generating the spectogram
            obj.spectogram_params.window = hann(obj.spectogram_params.fft_size);
        end
    
        function initialize_debugger(obj,debugger_enabled,Victim,frames_to_compute)
            %{
                Purpose: initializes the debugger if desired
                Inputs:
                    debugger_enabled: logical value determining if the
                        debugger is enabled (1 is enabled)
                    Victim: the vitim used in the simulation in order to
                        obtain "ground truth" information
                    frames_to_compute: how many frames will be computed in
                        the simulation
            %}
            
            obj.Debugger.enabled = debugger_enabled;

            %variables to keep track of the detected times and frequencies from the
            %individual points taken from the spectogram
            obj.Debugger.actual_num_chirps_per_frame = Victim.NumChirps;
            obj.Debugger.actual_ChirpCycleTime_us = Victim.ChirpCycleTime_us;
            obj.Debugger.detected_times = zeros(frames_to_compute * obj.Debugger.actual_num_chirps_per_frame,...
                    2 + 5 * ceil(obj.Debugger.actual_ChirpCycleTime_us / obj.spectogram_params.freq_sampling_period_us));
            obj.Debugger.detected_frequencies = zeros(frames_to_compute * obj.Debugger.actual_num_chirps_per_frame,...
                    2 + 5 * ceil(obj.Debugger.actual_ChirpCycleTime_us / obj.spectogram_params.freq_sampling_period_us));
            obj.Debugger.detected_chirp_slopes = zeros(frames_to_compute * obj.Debugger.actual_num_chirps_per_frame, 3);
            obj.Debugger.detected_chirp_intercepts = zeros(frames_to_compute * Victim.NumChirps, 3);
            
            %variables to keep track of the errors for the detected times, detected
            %frequencies, computed slope, and computed intercepts
            obj.Debugger.detected_times_errors = zeros(frames_to_compute * obj.Debugger.actual_num_chirps_per_frame,...
                    2 + 5 * ceil(obj.Debugger.actual_ChirpCycleTime_us / obj.spectogram_params.freq_sampling_period_us));
            obj.Debugger.detected_frequencies_errors = zeros(frames_to_compute * obj.Debugger.actual_num_chirps_per_frame,...
                    2 + 5 * ceil(obj.Debugger.actual_ChirpCycleTime_us / obj.spectogram_params.freq_sampling_period_us));
            obj.Debugger.detected_chirp_slopes_errors = zeros(frames_to_compute * obj.Debugger.actual_num_chirps_per_frame, 3);
            obj.Debugger.detected_chirp_intercepts_errors = zeros(frames_to_compute * obj.Debugger.actual_num_chirps_per_frame, 3);
            
            %variables to capture a summary of the errors - containing mean, variance,
            %and MSE
            obj.Debugger.detected_times_errors_summary = zeros(1,3);
            obj.Debugger.detected_frequencies_errors_summary = zeros(1,3);
            obj.Debugger.detected_chirp_slopes_errors_summary = zeros(1,3);
            obj.Debugger.detected_chirp_intercepts_errors_summary = zeros(1,3);
        end
    
        function initialize_chirp_and_frame_tracking(obj)
            %{
                Purpose: initializes the properties of the sensing
                subsystem to track specific chirp and frame parameters
            %}
            
            %initialize array to track captured chirps
            obj.chirp_tracking.captured_chirps_buffer_size = 128;        %this will be the maximum number of chirps captured when computing the average chirp duration
            obj.chirp_tracking.num_captured_chirps = 0;
            obj.chirp_tracking.captured_chirps = zeros(obj.chirp_tracking.captured_chirps_buffer_size, 2);
            obj.chirp_tracking.average_chirp_duration = 0;
            obj.chirp_tracking.average_slope = 0;
            obj.chirp_tracking.difference_threshold_us = 5;             %this is used as the threshold for detecting if a chirp has previously been detected
            obj.chirp_tracking.new_chirp_threshold_us = 5;              %this parameter is used to determine if a chirp has been seen before.

            %initialize array to track captured frames
            obj.frame_tracking.captured_frames_buffer_size = 128;
            obj.frame_tracking.num_captured_frames = 0;
            obj.frame_tracking.captured_frames = zeros(obj.frame_tracking.captured_frames_buffer_size,7); %duration, number of chirps, average slope, average chirp duration, start time, sampling window count
            obj.frame_tracking.average_frame_duration = 0;
            obj.frame_tracking.average_chirp_duration = 0;
            obj.frame_tracking.average_slope = 0;
            obj.frame_tracking.new_frame_threshold_us = 10;             
            %the above variable is the threshold used for detecting if a chirp is part 
            % of the current frame or the next frame. Basically, if a new
            %chirp has a duration that is different by this amount when 
            %compared with the average, it is designated a new chirp
        end
    
        function initialize_plot_params(obj,FMCW_sample_rate_Msps)
            %{
                Purpose: initialize the plot_params struct property of the
                spectrum sensing part of the attacker. This property holds
                useful values that will be used to plot different
                structures
            %}
            %if assuming complex sampling
            obj.plot_params.max_freq = FMCW_sample_rate_Msps; %assuming complex sampling
            obj.plot_params.freq_resolution = FMCW_sample_rate_Msps/obj.spectogram_params.fft_size;

            %if not assuming complex sampling
%             obj.plot_params.max_freq = 2 * FMCW_sample_rate_Msps; %assuming complex sampling
%             obj.plot_params.freq_resolution = FMCW_sample_rate_Msps/obj.spectogram_params.fft_size;

            obj.plot_params.frequencies = 0:obj.plot_params.freq_resolution:obj.plot_params.max_freq - obj.plot_params.freq_resolution;
            obj.plot_params.times = (0: obj.spectogram_params.freq_sampling_period_us : ...
                obj.spectogram_params.num_freq_spectrum_samples_per_spectogram * ...
                obj.spectogram_params.freq_sampling_period_us - obj.spectogram_params.freq_sampling_period_us)...
                + obj.spectogram_params.detected_time_offset;
            
            %compute times for the combined_spectogram
            obj.plot_params.combined_spectogram_times = (0: obj.spectogram_params.freq_sampling_period_us : ...
                2 * obj.spectogram_params.num_freq_spectrum_samples_per_spectogram * ...
                obj.spectogram_params.freq_sampling_period_us - obj.spectogram_params.freq_sampling_period_us) + ...
                obj.spectogram_params.detected_time_offset;
        end
    
        function initialize_peak_detection_params(obj)
            obj.peak_detection_params.threshold = 5;    %threshold height for a peak to be detected in the spectogram
            obj.peak_detection_params.numPeaks = 2;     %maximum number of peaks for each spectogram window   
        end
        
        function initialize_clustering_params(obj)
            %{
                Purpose: initializes parameters used for the clustering
                algorithm
            %}
            obj.clustering_params.max_num_clusters = 10;
            obj.clustering_params.min_num_points_per_cluster = 7;
        end

        %functions for processing the received signal
        function process_received_signal(obj,received_signal)
            %{
                Purpose: takes in a received signal, generates a spectogram
                Inputs:
                    received_signal: the signal received the by sensing
                    system
            %}
            obj.received_signal = received_signal;
            obj.generate_spectogram(obj.received_signal)
            obj.spectogram_points = obj.detect_peaks_in_spectogram(obj.generated_spectogram);

            %combine the detected points with the previously detected points
                if ~isempty(obj.spectogram_points)
                    obj.combined_spectogram = [obj.previous_spectogram_points,obj.spectogram_points...
                        + [0;obj.spectogram_params.num_freq_spectrum_samples_per_spectogram]];
                else
                    obj.combined_spectogram = obj.previous_spectogram_points;
                end
                obj.previous_spectogram_points = obj.spectogram_points;
            
            %provided there were detected points in the combined
            %spectogram, continue with the rest of the program
            if ~isempty(obj.combined_spectogram)
                obj.compute_detected_times_and_frequencies(obj.combined_spectogram);
                obj.compute_clusters();
                obj.fit_linear_model();
                obj.compute_victim_parameters()
            end
            obj.sampling_window_count = obj.sampling_window_count + 1;
        end
    
        function generate_spectogram(obj,received_signal)
            %{
                Purpose: takes in the received signal (not reshaped yet),
                    reshapes it for fft processing, performs windowing, and
                    generates the spectogram. The updated spectogram is stored
                    in the generated_spectogram property of the
                    spectrum_sensing class
                Inputs:
                    received_signal: the signal received the by sensing
                    system
            %}
            %first reshape the received signal
            obj.reshaped_signal = reshape(received_signal,...
                obj.spectogram_params.num_ADC_samples_per_spectogram / ...
                obj.spectogram_params.num_freq_spectrum_samples_per_spectogram,...
                obj.spectogram_params.num_freq_spectrum_samples_per_spectogram);
            
            %next, shave off the last few samples so that we get the desired fft size
            obj.reshaped_signal_for_fft = obj.reshaped_signal(1:obj.spectogram_params.fft_size,:);
            
            %window the signal
            obj.windowed_signal = obj.reshaped_signal_for_fft .* obj.spectogram_params.window;
            
            %perform an fft
            obj.generated_spectogram = fft(obj.windowed_signal);
            
            %clip off the negative frequencies - only is not using complex
            %sampling
%            obj.generated_spectogram = obj.generated_spectogram(1:obj.spectogram_params.fft_size/2,:);
        end

        function spectogram_points = detect_peaks_in_spectogram(obj,generated_spectogram)
            %{
                Purpose: for a given generated spectogram, detect the peaks
                    in the spectogram and output an array of indicies for the
                    detected peaks in the spectogram
                Inputs:
                    generated_spectogram: the generated spectogram to
                        identify peaks in
                Outputs:
                    spectogram_points: the indicies of the detected peaks
                        in the spectogram array, first row is the location
                        corresponding to frequency, second row is the index
                        corresponding to time
            %}
            spectogram_points = [];  %array to hold the time, freq locations of valid spectogram points
            
            for i = 1:size(generated_spectogram,2)
                [peaks,locations] = findpeaks(abs(generated_spectogram(:,i)),"NPeaks",obj.peak_detection_params.numPeaks);
                locations = locations(peaks > obj.peak_detection_params.threshold);
                if ~isempty(locations)
                    locations = [locations.'; i * ones(1,size(locations,1))];
                    spectogram_points = [spectogram_points,locations];
                end
            end
        end

        function compute_detected_times_and_frequencies(obj,combined_spectogram)
            %{
                Purpose: takes the combined spectogram of array indicies in
                    the generated spectogram and converts the indicies to times
                    and frequencies
                Inputs: 
                    combined_spectogram: the array of indicies of peaks in
                    the generated spectogram stored in the combined
                    spectogram object
            %}
            obj.detected_frequencies = obj.plot_params.frequencies(combined_spectogram(1,:));
            obj.detected_times = obj.plot_params.combined_spectogram_times(combined_spectogram(2,:));
        end
        
        function compute_clusters(obj)
            %{
                Purpose: performs the DBSCAN algorithm to identify each
                individual chirp
            %}

            [obj.idx,obj.corepts] = dbscan([obj.detected_times.',obj.detected_frequencies.'],10,2);
        end
        
        function fit_linear_model(obj)
            %{
                Purpose: fits a linear model for each valid cluster
                identified as a chirp
            %}
            obj.detected_chirps = zeros(max(obj.idx),2);
            for cluster_id = 1:max(obj.idx)
                %solve for linear model using linear algrebra
                %y is frequencies, x is time
                if sum(obj.idx == cluster_id) > obj.clustering_params.min_num_points_per_cluster
                    X = [ones(size(obj.detected_times(obj.idx == cluster_id),2),1),obj.detected_times(obj.idx == cluster_id).'];
                    Y = obj.detected_frequencies(obj.idx == cluster_id).';
                    B = inv(X.' * X) * X.' * Y;
                    obj.detected_chirps(cluster_id,1:2) = [-B(1)/B(2),B(2)];
                    slope = obj.detected_chirps(cluster_id,2);
                    intercept = obj.detected_chirps(cluster_id,1);
                end
            end

%             %option for a simplified linear model
%                     %Now, we experiment with only taking 15 points to determine the slope and
%                     %intercept
%                     num_points_for_linearizing = 15;
%                     detected_chirps_simplified = zeros(max(idx),2);
%                     for cluster_id = 1:max(idx)
%                         if sum(idx == cluster_id) >= num_points_for_linearizing
%                             cluster_times = detected_times(idx == cluster_id);
%                             cluster_freqs = detected_frequencies(idx == cluster_id);
%                             X_simplified = [ones(num_points_for_linearizing,1),cluster_times(1:num_points_for_linearizing).'];
%                             Y_simplified = cluster_freqs(1:num_points_for_linearizing).';
%                             B_simplified = inv(X_simplified.' * X_simplified) * X_simplified.' * Y_simplified;
%                             detected_chirps_simplified(cluster_id,1:2) = [-B_simplified(1)/B_simplified(2),B_simplified(2)];
%                             slope_simplified = detected_chirps_simplified(cluster_id,2);
%                             intercept_simplified = detected_chirps_simplified(cluster_id,1);
%                         end
%                     end
%                     %detected_chirps_simplified - detected_chirps;
%                     %shows that decreasing the number of points affected the computed intercept
%                     %point
        end
        
        function compute_victim_parameters(obj)

            %compute the actual time that the chirp intercept occured at
            obj.detected_chirps = obj.detected_chirps + [obj.spectogram_params.num_freq_spectrum_samples_per_spectogram * ...
                    obj.spectogram_params.freq_sampling_period_us * (obj.sampling_window_count - 1), 0];


            for i = 1:size(obj.detected_chirps,1)
                if obj.detected_chirps(i,2) ~= 0 && obj.chirp_tracking.num_captured_chirps < obj.chirp_tracking.captured_chirps_buffer_size
                    if obj.chirp_tracking.num_captured_chirps == 0 %if the captured chirps buffer is empty (i.e: first chirp detected or first chirp in a frame)
                        obj.chirp_tracking.captured_chirps(1,:) = obj.detected_chirps(i,:);
                        obj.chirp_tracking.num_captured_chirps = obj.chirp_tracking.num_captured_chirps + 1;
                        obj.debugger_save_detection_points(i);                
                    elseif all(abs(obj.chirp_tracking.captured_chirps - obj.detected_chirps(i,1)) > obj.chirp_tracking.new_chirp_threshold_us,'all')
                        if obj.chirp_tracking.num_captured_chirps >= 2
                            duration_difference_from_avg = abs(obj.detected_chirps(i,1) - ...
                                obj.chirp_tracking.captured_chirps(obj.chirp_tracking.num_captured_chirps,1) - obj.chirp_tracking.average_chirp_duration);
                            if duration_difference_from_avg >= obj.frame_tracking.new_frame_threshold_us %declare as part of a new frame
                                %increment the frame counter
                                obj.frame_tracking.num_captured_frames = obj.frame_tracking.num_captured_frames + 1;
                                %save the duration, number of chirps, average slope, average chirp duration, start time, and sampling window count
                                obj.frame_tracking.captured_frames(obj.frame_tracking.num_captured_frames,1) = obj.detected_chirps(i,1) - obj.chirp_tracking.captured_chirps(1,1);  % duration
                                obj.frame_tracking.captured_frames(obj.frame_tracking.num_captured_frames,2) = obj.chirp_tracking.num_captured_chirps;                          % number of chirps
                                obj.frame_tracking.captured_frames(obj.frame_tracking.num_captured_frames,3) = obj.chirp_tracking.average_slope;                                % average slope
                                obj.frame_tracking.captured_frames(obj.frame_tracking.num_captured_frames,4) = obj.chirp_tracking.average_chirp_duration;                       % average_chirp_duration
                                obj.frame_tracking.captured_frames(obj.frame_tracking.num_captured_frames,5) = obj.chirp_tracking.captured_chirps(1,1);
                                obj.frame_tracking.captured_frames(obj.frame_tracking.num_captured_frames,6) = obj.sampling_window_count;
    
                                %reset the chirp tracking
                                obj.chirp_tracking.num_captured_chirps = 0;
                                obj.chirp_tracking.captured_chirps = zeros(obj.chirp_tracking.captured_chirps_buffer_size, 2);
                                obj.chirp_tracking.average_chirp_duration = 0;
                                obj.chirp_tracking.average_slope = 0;
                                
                                %reset sampling window - not implemented yet
                                obj.frame_tracking.average_frame_duration = sum(obj.frame_tracking.captured_frames(:,1)) / obj.frame_tracking.num_captured_frames;
                                
                                %compute the overall averages for chirp slope
                                %and duration
                                obj.frame_tracking.average_chirp_duration =  ...
                                    sum(obj.frame_tracking.captured_frames(:,4) .* (obj.frame_tracking.captured_frames(:,2) - 1))/ ...
                                    sum((obj.frame_tracking.captured_frames(obj.frame_tracking.captured_frames(:,2) ~=0,2) - 1));
                                obj.frame_tracking.average_slope = ...
                                    sum(obj.frame_tracking.captured_frames(:,3) .* (obj.frame_tracking.captured_frames(:,2) - 1))/ ...
                                    sum((obj.frame_tracking.captured_frames(obj.frame_tracking.captured_frames(:,2) ~=0,2) - 1));
    
                                %compute the predicted time for the next chirp
                                %to occur on.
                                obj.frame_tracking.captured_frames(obj.frame_tracking.num_captured_frames,7) = ...
                                            obj.detected_chirps(i,1) + obj.frame_tracking.average_frame_duration;
                            end
                        end
                        obj.chirp_tracking.captured_chirps(obj.chirp_tracking.num_captured_chirps + 1,:) = obj.detected_chirps(i,:);
                        obj.chirp_tracking.num_captured_chirps = obj.chirp_tracking.num_captured_chirps + 1;
    
                        obj.debugger_save_detection_points(i);
                    end
                    obj.chirp_tracking.average_slope = sum(obj.chirp_tracking.captured_chirps(1:obj.chirp_tracking.num_captured_chirps,2))/...
                        double(obj.chirp_tracking.num_captured_chirps);
                    if obj.chirp_tracking.num_captured_chirps >= 2
                        obj.chirp_tracking.average_chirp_duration = (obj.chirp_tracking.captured_chirps(obj.chirp_tracking.num_captured_chirps,1)...
                            - obj.chirp_tracking.captured_chirps(1,1))/double(obj.chirp_tracking.num_captured_chirps - 1);
                    end
                end
            end
        end
        
        %debugger functions - These should eventually get updated, but for
        %now they are sufficient as is
        function debugger_save_detection_points(obj,detected_chirp_index)
        %{
            Purpose: updates the debugger object (if enabled)
            Inputs:
                detected_chirp_index - the cluster I.D (in the idx) for the whose chirp whose values are being stored
        %}
            if obj.Debugger.enabled
                 
                debugger_index = (obj.frame_tracking.num_captured_frames) * obj.Debugger.actual_num_chirps_per_frame + obj.chirp_tracking.num_captured_chirps;
                timing_offset = obj.spectogram_params.num_freq_spectrum_samples_per_spectogram * ...
                            obj.spectogram_params.freq_sampling_period_us * (obj.sampling_window_count - 1);
        
                %save the detected times
                obj.Debugger.detected_times(debugger_index,1: 2 + size(obj.idx(obj.idx == detected_chirp_index),1)) = ...
                    [obj.frame_tracking.num_captured_frames + 1,obj.chirp_tracking.num_captured_chirps, ...
                    timing_offset + obj.detected_times(obj.idx == detected_chirp_index)];
                %save the detected frequencies
                obj.Debugger.detected_frequencies(debugger_index,1:2 + size(obj.idx(obj.idx == detected_chirp_index),1)) = ...
                    [obj.frame_tracking.num_captured_frames + 1,obj.chirp_tracking.num_captured_chirps, obj.detected_frequencies(obj.idx == detected_chirp_index)];
        
                obj.Debugger.detected_chirp_slopes(debugger_index,:) = ...
                    [obj.frame_tracking.num_captured_frames + 1,obj.chirp_tracking.num_captured_chirps,obj.detected_chirps(detected_chirp_index,2)];
                obj.Debugger.detected_chirp_intercepts(debugger_index,:) = ...
                    [obj.frame_tracking.num_captured_frames + 1,obj.chirp_tracking.num_captured_chirps,obj.detected_chirps(detected_chirp_index,1)];
            end
        end
        
        function debugger_compute_errors(obj,victim)
        %{
            Purpose: compares the measured values with what the actual values
            should be
            Inputs:
                Debugger - the previous debugger object
                victim - the victim object containing relevant frame and chirp
                    parameters
            Outputs: 
                Debugger_updated - the updated debugger object
        %}
            if obj.Debugger.enabled
                %update the sizes of the error trackers as the size may have
                %changed slightly when computing:
                obj.Debugger.detected_times_errors = zeros(size(obj.Debugger.detected_times));
                obj.Debugger.detected_frequencies_errors = zeros(size(obj.Debugger.detected_frequencies));
                obj.Debugger.detected_chirp_slopes_errors = zeros(size(obj.Debugger.detected_chirp_slopes));
                obj.Debugger.detected_chirp_intercepts_errors = zeros(size(obj.Debugger.detected_chirp_intercepts));
                
            %compute the errors
                true_chirp_intercepts = ...
                    (obj.Debugger.detected_chirp_intercepts(:,1) - 1) * victim.FramePeriodicity_ms * 1e3 +...
                    (obj.Debugger.detected_chirp_intercepts(:,2) - 1) * victim.ChirpCycleTime_us + ...
                    victim.IdleTime_us;
        
                %intercept errors
                obj.Debugger.detected_chirp_intercepts_errors(:,1:2) = obj.Debugger.detected_chirp_intercepts(:,1:2);
                obj.Debugger.detected_chirp_intercepts_errors(:,3) = obj.Debugger.detected_chirp_intercepts(:,3) - true_chirp_intercepts; 
        
                %slope errors
                obj.Debugger.detected_chirp_slopes_errors(:,1:2) = obj.Debugger.detected_chirp_slopes(:,1:2);
                obj.Debugger.detected_chirp_slopes_errors(:,3) = obj.Debugger.detected_chirp_slopes(:,3) - victim.FrequencySlope_MHz_us;
        
                %error for time points
                obj.Debugger.detected_times_errors(:,1:2) = obj.Debugger.detected_times(:,1:2);
                for i = 1:size(obj.Debugger.detected_times,1)
                    num_detected_times = sum(obj.Debugger.detected_times(i,3:end) ~= 0);
                    true_times = (obj.Debugger.detected_frequencies(i,3:num_detected_times + 2) / ...
                        victim.FrequencySlope_MHz_us) + true_chirp_intercepts(i);
                    obj.Debugger.detected_times_errors(i,3:num_detected_times + 2) = ...
                        obj.Debugger.detected_times(i,3:num_detected_times + 2) - true_times;
                end
                
                %error for frequency points
                obj.Debugger.detected_frequencies_errors(:,1:2) = obj.Debugger.detected_frequencies(:,1:2);
                for i = 1:size(obj.Debugger.detected_frequencies,1)
                    num_detected_frequencies = sum(obj.Debugger.detected_frequencies(i,3:end) ~= 0);
                    true_frequencies = victim.FrequencySlope_MHz_us * ...
                        (obj.Debugger.detected_times(i,3:num_detected_frequencies + 2) - true_chirp_intercepts(i));
                    obj.Debugger.detected_frequencies_errors(i,3:num_detected_frequencies + 2) = ...
                        obj.Debugger.detected_frequencies(i,3:num_detected_frequencies + 2) - true_frequencies;
                end
        
            end
        end
        
        function [summary_table] = debugger_summarize_errors(obj)
            %{
                Purpose: computes the sample mean, sample variance, and mean
                squared error for the slope, intercept, timing, and frequency
                errors
            %}
            %chirp intercepts
            [mean,variance,MSE] = obj.compute_summary_statistics(...
                obj.Debugger.detected_chirp_intercepts_errors, obj.Debugger.detected_chirp_intercepts);
            obj.Debugger.detected_chirp_intercepts_errors_summary(1:3) = [mean,variance,MSE];
            %slope
            [mean,variance,MSE] = obj.compute_summary_statistics(...
                obj.Debugger.detected_chirp_slopes_errors, obj.Debugger.detected_chirp_slopes);
            obj.Debugger.detected_chirp_slopes_errors_summary(1:3) = [mean,variance,MSE];
             %frequency measurement
            [mean,variance,MSE] = obj.compute_summary_statistics(...
                obj.Debugger.detected_frequencies_errors, obj.Debugger.detected_frequencies);
            obj.Debugger.detected_frequencies_errors_summary(1:3) = [mean,variance,MSE];
             %time measurement
            [mean,variance,MSE] = obj.compute_summary_statistics(...
                obj.Debugger.detected_times_errors, obj.Debugger.detected_times);
            obj.Debugger.detected_times_errors_summary(1:3) = [mean,variance,MSE];
        
            %output the table as well
            summary_table = array2table( ...
                [obj.Debugger.detected_times_errors_summary;...
                obj.Debugger.detected_frequencies_errors_summary;...
                obj.Debugger.detected_chirp_slopes_errors_summary;...
                obj.Debugger.detected_chirp_intercepts_errors_summary], ...
                "VariableNames",[ "Mean","Variance","MSE"], ...
                "RowNames",["Detected Times","Detected Frequencies","Computed Slope","Computed Intercept"]);
        end
        
        function [sample_mean,sample_variance,MSE] = compute_summary_statistics(obj,errors_array, debugger_array)
            %{
                Purpose: computes the sample mean, sample variance, and mean
                    squared error for the various errors captured by the debugger
                Inputs: 
                    Errors_array: an array containing the errors computed from the
                        debugger
                    Debugger_array: the array containing the measured values from
                        the debugger
            %}
            valid_intercepts = debugger_array ~= 0;
            valid_intercepts(:,1:2) = 0;
            sample_mean = ...
                sum(errors_array(valid_intercepts),'all') /...
                sum(valid_intercepts,'all');
            sample_variance = ...
                sum((errors_array(valid_intercepts) - sample_mean).^2,'all') /...
                (sum(valid_intercepts,'all')-1);
            MSE = ...
                sum(errors_array(valid_intercepts).^2,'all') /...
                (sum(valid_intercepts,'all'));
        end

        
        %functions to aid in plotting
        function plot_received_spectogram(obj)
            %{
                Purpose: Plot the last spectogram computed by the sensing
                subsystem
            %}
            surf(obj.plot_params.times,obj.plot_params.frequencies, abs(obj.generated_spectogram));
            title_str = sprintf('Spectogram');
            title(title_str);
            xlabel('Time(us)')
            ylabel('Frequency (MHz)')
            view([0,90.0])
        end
    
        function plot_clusters(obj)
                    %{
                        Purpose: generates a plot of the detected chirps
                    %}
                    gscatter(obj.detected_times,obj.detected_frequencies,obj.idx);
                end
    end
end