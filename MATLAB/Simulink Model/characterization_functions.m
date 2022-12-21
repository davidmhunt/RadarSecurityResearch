classdef characterization_functions
    %characterization_functions - class to house functions for system
    %characterizations including characterizing the sensing subsystem and
    %attacking subsystem

    methods (Static)
        
        
        %{
            Purpose: compute the objects sensed by a victim given a
            spoofing attack
            Inputs:
                config_path - path to the .json configuration file
                actual_range - actual range of the object to be added
                actual_velocity - actual velocity of the object ot be
                    spoofed
                frames_to_compute - number of frames to simulate before
                    recording a result
                attack_start_frame - the frame that the attack starts at
            Outputs:
                estimated_ranges - the estimated ranges for each frame that
                    the victim was under attack
                estimated_velocities - the estimated velocities for each
                    frame that the victim was under attack
        %}
        function [estimated_ranges,estimated_velocities] = ...
            compute_sensed_targets(config_path,actual_range,actual_velocity,frames_to_compute,attack_start_frame)
            
            simulator = Simulator_revB();
            
            simulator.load_params_from_JSON(config_path);
            
            %apply timing offsets as desired
            simulator.Victim.timing_offset_us = 0;
            simulator.Attacker.Subsystem_tracking.timing_offset_us = 0;
            
            %configure the FMCW parameters
            simulator.configure_FMCW_Radar_parameters();
            
            %load default attacker, and victim positions and velocities
            simulator.load_realistic_attacker_and_victim_position_and_velocity();
            
            %load the target
            simulator.load_target_realistic(50,15);
        
            %initialize victim and simulation parameters
        
            
            %specify whether or not to record a movie of the range-doppler plot
            record_movie = false;
            simulator.Victim.Radar_Signal_Processor.configure_movie_capture(frames_to_compute,record_movie);
            
            %pre-compute the victim's chirps
            simulator.Victim.precompute_radar_chirps();
        
            %initialize the attacker parameters
            simulator.Attacker.initialize_attacker(...
                                    simulator.Victim.FMCW_sampling_rate_Hz * 1e-6,...
                                    simulator.Victim.StartFrequency_GHz,...
                                    simulator.Victim.Chirp_Tx_Bandwidth_MHz);
            
            %initialize the sensing subsystem's debugger
            simulator.Attacker.Subsystem_spectrum_sensing.initialize_debugger(0,simulator.Victim,frames_to_compute);
            %set to change 1 to zero to disable
            
            %specify the type of emulation ("target", 
                    % "velocity spoof - noisy", 
                    % "velocity spoof - similar velocity",
                    % "range spoof - similar slope")
            
            %attack_type = "range spoof - similar slope";
            attack_type = "target";
            %attack_type = "target";
            %initialize the attacker
            simulator.Attacker.Subsystem_attacking.set_attack_mode(attack_type);
            
            %if it is desired to specify a specific attack location
            simulator.Attacker.Subsystem_attacking.set_desired_attack_location(actual_range,actual_velocity);
        
            simulator.run_simulation_attack_no_target(frames_to_compute,false);
            
            %get the return values
        
            estimated_ranges = simulator.Victim.Radar_Signal_Processor.range_estimates(attack_start_frame:frames_to_compute,1);
            estimated_velocities = simulator.Victim.Radar_Signal_Processor.velocity_estimates(attack_start_frame:frames_to_compute,1);
        end
        
        %{
            Purpose: Initialize a series of random victim configurations to
                be used to test the sensing subsystem performance
            Inputs:
                num_cases - the number of test cases to generate
                valid_slopes - the range of valid slopes expressed as
                    [min,max]
                valid_chirp_periods - the range of valid chirp periods
                    expressed as [min,max]
                valid_BWs_MHz - the range of valid bandwidths for a victim
                    expressed as [min,max]
                num_adc_samples - the number of ADC samples
            Outputs:
                slopes - the slope for each victim test
                chirp_periods - the chirp period for each victim test
                adc_sampling_rates - the adc sampling rate for each victim
                    test
        %}
        function [slopes,chirp_periods,adc_sampling_rates] = initialize_sensing_subsystem_test_cases_general( ...
                num_cases, ...
                valid_slopes, ...
                valid_chirp_periods, ...
                valid_BWs_MHz, ...
                num_adc_samples)


            %initialize the output arrays
            slopes = zeros(num_cases,1);
            chirp_periods = zeros(num_cases,1);
            adc_sampling_rates = zeros(num_cases,1);

            %using a for loop initialize all of the test cases
            for i = 1:num_cases
                
                %% select parameters for the test case
                %select a random slope
                slope_MHz_us = (rand * (valid_slopes(2) - valid_slopes(1)))...
                    + valid_slopes(1);
                
                %determine the range of chirp cycle periods that work with the selected
                %slope
                chirp_cycle_time_range_us = [0,0];
                
                %compute the min chirp cycle time
                chirp_cycle_time_range_us(1) = ...
                    max(valid_chirp_periods(1),valid_BWs_MHz(1)/slope_MHz_us);
                
                %compute the max chirp cycle time
                chirp_cycle_time_range_us(2) = ...
                    min(valid_chirp_periods(2),valid_BWs_MHz(2)/slope_MHz_us);
                
                %randomly select a chirp cycle time
                chirp_cycle_time_us = (rand * (chirp_cycle_time_range_us(2) - chirp_cycle_time_range_us(1)))...
                    + chirp_cycle_time_range_us(1);
                
                %compute the required ADC sample rate to achieve the maximum BW
                sampling_period = chirp_cycle_time_us - 7.3;
                ADC_SampleRate_MSps = num_adc_samples / sampling_period;
                
                
                slopes(i) = slope_MHz_us;
                chirp_periods(i) = chirp_cycle_time_us;
                adc_sampling_rates(i) = ADC_SampleRate_MSps;
            end
        end


        %{
            Purpose: Initialize a series of random victim configurations to
                be used to test the sensing subsystem performance
            Inputs:
                slopes - the slope for each victim test
                chirp_periods - the chirp period for each victim test
                adc_sampling_rates - the adc sampling rate for each victim
                    test
                num_adc_samples - the number of ADC samples
                config_path - path to a config file for initializing other
                    simulation variables
            Outputs:
                configurations_passed: a bool specifying that all
                    configurations are usable
        %}
        function configurations_passed = check_test_configurations( ...
                slopes, ...
                chirp_periods, ...
                adc_sampling_rates, ...
                num_adc_samples,...
                config_path)

            num_cases = size(slopes,1);
            configurations_passed = true;
            for i = 1:num_cases

                %initialize the simulator
                simulator = Simulator_revB();
                simulator.load_params_from_JSON(config_path);

                %initialize the victim parameters
                simulator.Victim.FrequencySlope_MHz_us = slopes(i);
                simulator.Victim.ChirpCycleTime_us = chirp_periods(i);
                simulator.Victim.ADC_Samples = num_adc_samples;
                simulator.Victim.ADC_SampleRate_MSps = adc_sampling_rates(i);
                simulator.Victim.compute_calculated_vals();
                
                %set attacker parameters
                simulator.Attacker.Subsystem_tracking.FrequencySlope_MHz_us = slopes(i);
                simulator.Attacker.Subsystem_tracking.ChirpCycleTime_us = chirp_periods(i);
                simulator.Attacker.Subsystem_tracking.ADC_Samples = num_adc_samples;
                simulator.Attacker.Subsystem_tracking.ADC_SampleRate_MSps = adc_sampling_rates(i);
                simulator.Attacker.Subsystem_tracking.compute_calculated_vals();
                
                %apply timing offsets as desired
                simulator.Victim.timing_offset_us = 0;
                simulator.Attacker.Subsystem_tracking.timing_offset_us = 0;
                
                %configure the FMCW parameters
                simulator.configure_FMCW_Radar_parameters();

                if simulator.Victim.IdleTime_us <= 0
                    configurations_passed = false;
                end
            end
            
            if configurations_passed
                fprintf("All Configurations passed \n");
            else
                fprintf("A configuration has failed \n");
            end
        end
        
        %{
            Purpose: generate a plot for the test configuration slopes and
                chirp cycle times
            Inputs: 
                read_file_path - csv file containing the simulation
                    results, actual chirp slopes, and actual chirp cycle times
        %}
        function plot_test_configurations(read_file_path)
            table_data_results = readtable(read_file_path);
            slopes_MHz_us = table_data_results{:,1};
            chirp_cycle_times_MHz_us = table_data_results{:,2};
            figure;
            scatter(chirp_cycle_times_MHz_us, slopes_MHz_us);
            ylabel("Chirp Slope (MHz/us)")
            xlabel("Chirp Cycle Time (us)")
            title("Victim Configurations to Test")
            saveas(gcf, "generated_plots/victim_test_configurations.png")
        end

        %{
            Purpose: generate a plot for CDF of the test configuration slopes and
                chirp cycle times
            Inputs: 
                read_file_path - csv file containing the simulation
                    results, actual chirp slopes, and actual chirp cycle times
        %}
        function plot_test_configuration_cdfs(read_file_path)
            table_data_results = readtable(read_file_path);
            slopes_MHz_us = table_data_results{:,1};
            chirp_cycle_times_MHz_us = table_data_results{:,2};
            figure;
            %plot cdf for slopes
            subplot(1,2,1);
            [h,stats] = cdfplot(slopes_MHz_us);
            xlabel("Chirp Slope (MHz/us)")
            title("CDF of Test Slopes")

            %plot cdf for chirp cycle times
            subplot(1,2,2);
            [h,stats] = cdfplot(chirp_cycle_times_MHz_us);
            xlabel("Chirp Cycle Time (us)")
            title("CDF of Test Chirp Cycle Times")
            saveas(gcf, "generated_plots/victim_test_configuration_cdfs.png")
        end

        %{
            Purpose: generate a table of the key statistics for the
                absolute error of the slope estimation and plot the cdf of the
                absolute error
        %}
        function summary_table = generate_slope_estimation_summary(read_file_path)
            
            %plot the cdf of the chirp slope errors
            table_data_results = readtable(read_file_path);
            actual_values = table_data_results{:,1};
            estimated_values = table_data_results{:,5};
            abs_errors = table_data_results{:,8};
            figure;
            [h,stats] = cdfplot(abs_errors);
            xlabel("Chirp Slope Error (MHz/us)")
            title("CDF of Chirp Slope Errors")
            
            %statistics based on absolute errors
            mean_error = mean(abs_errors);
            variance_error = var(abs_errors);
            MSE_error = mse(actual_values,estimated_values);
            tail_95th_percentile = prctile(abs_errors,95);
            
            variable_names = ["Mean (MHz/us)", "Variance (MHz/us)^2", "MSE (MHz/us)^2", "95th Percentile (MHz/us)"];
            summary_table = array2table( ...
                [mean_error,...
                variance_error,...
                MSE_error,...
                tail_95th_percentile], ...
                "VariableNames",variable_names);
            
            saveas(gcf, "generated_plots/slope_error_estimation_cdf.png")

        end

        %{
            Purpose: generate a table of the key statistics for the
                absolute error of the chirp cycle period estimation and plot the cdf of the
                absolute error
        %}
        function summary_table = generate_chirp_period_estimation_summary(read_file_path)
            
            %plot the cdf of the chirp slope errors
            table_data_results = readtable(read_file_path);
            actual_values = table_data_results{:,2};
            estimated_values = table_data_results{:,6};
            abs_errors = table_data_results{:,9};
            figure;
            [h,stats] = cdfplot(abs_errors);
            xlabel("Chirp Cycle Period Estimation Error (us)")
            title("CDF of Chirp Cycle Period Estimation Errors")
            
            %statistics based on absolute errors
            mean_error = mean(abs_errors);
            variance_error = var(abs_errors);
            MSE_error = mse(actual_values,estimated_values);
            tail_95th_percentile = prctile(abs_errors,95);
            
            variable_names = ["Mean (us)", "Variance (us)^2", "MSE (us)^2", "95th Percentile (us)"];
            summary_table = array2table( ...
                [mean_error,...
                variance_error,...
                MSE_error,...
                tail_95th_percentile], ...
                "VariableNames",variable_names);

            saveas(gcf, "generated_plots/chirp_cycle_period_error_estimation_cdf.png")
        end

        %{
            Purpose: generate a table of the key statistics for the
                absolute error of the frame period estimation and plot the cdf of the
                absolute error
        %}
        function summary_table = generate_frame_period_estimation_summary(read_file_path)
            
            %plot the cdf of the chirp slope errors
            table_data_results = readtable(read_file_path);
            actual_values = table_data_results{:,3};
            estimated_values = table_data_results{:,7};
            abs_errors = table_data_results{:,10};
            figure;
            [h,stats] = cdfplot(abs_errors);
            xlabel("Frame Period Estimation Error (us)")
            title("CDF of Frame Period Estimation Errors")
            
            %statistics based on absolute errors
            mean_error = mean(abs_errors);
            variance_error = var(abs_errors);
            MSE_error = mse(actual_values,estimated_values);
            tail_95th_percentile = prctile(abs_errors,95);
            
            variable_names = ["Mean (us)", "Variance (us)^2", "MSE (us)^2", "95th Percentile (us)"];
            summary_table = array2table( ...
                [mean_error,...
                variance_error,...
                MSE_error,...
                tail_95th_percentile], ...
                "VariableNames",variable_names);

            saveas(gcf, "generated_plots/frame_period_error_estimation_cdf.png")
        end
        
        function plot_predicted_start_time_errors(read_file_path)
            %access the saved data for the predicted start time errors
            table_data_results = readtable(read_file_path);
            prediction_errors = abs(table_data_results{:,:});

            %create arrays for mean absolute error, variance, and 95th
            %percentile
            mean_absolute_errors = zeros(size(prediction_errors,2),1);
            variances = zeros(size(prediction_errors,2),1);
            tail_95th_percentiles = zeros(size(prediction_errors,2),1);


            %for each frame
            for i = 1:size(prediction_errors,2)
                mean_absolute_errors(i) = mean(prediction_errors(:,i));
                variances(i) = var(prediction_errors(:,i));
                tail_95th_percentiles(i) = prctile(prediction_errors(:,i),95);
            end            
            
            %compute range errors
            errors_in_range = mean_absolute_errors * 1e-6 * physconst("LightSpeed");
            
            %plot a figure with error bars
            error_bars = tail_95th_percentiles;
            figure;
            clf;
            errorbar(mean_absolute_errors,error_bars);
            title("Timing Estimation error vs Number of Frames")
            xlabel("Number of Frames Sensed")
            ylabel("error (us)")
            y_lim = 3 *max(mean_absolute_errors);
            ylim([0, y_lim])
            xlim([0,size(prediction_errors,2)+ 1])
            
            %add in the axis for range error
            yyaxis right
            plot(errors_in_range)
            ylabel("corresponding range error (m)")
            set(gca,"YColor", 'black')
            y_lim = 3 * max(errors_in_range);
            ylim([0, y_lim])

            saveas(gcf, "generated_plots/predicted_start_time_errors.png")
            
        end
%% Functions to Run Sensing Subsystem Simulations

        %{
            Purpose: compute the estimated chirp slope, chirp duration, and
            frame duration for a given sensing subsystem
            Inputs:
                config_path - path to the .json configuration file
                slope_MHz_us - the chirp slope to use for testing
                chirp_cycle_period_us - the chirp cycle period
                adc_sampling_rates - the adc sampling rate for each victim
                    test
                num_adc_samples - the number of ADC samples
                frames_to_compute - number of frames to simulate before
                    recording a result
                attack_start_frame - the frame that the attack starts at
            Outputs:
                actual_slope
                estimated_slope
                actual_chirp_duration
                estimated_chirp_duration
                actual_frame_duration
                estimated_chirp_duration
                ADC_SampleRate_MSps
                predicted_start_time_errors
        %}


        function [actual_slope,estimated_slope,...
            actual_chirp_duration,estimated_chirp_duration,...
            actual_frame_duration, estimated_frame_duration,...
            ADC_SampleRate_MSps, predicted_start_time_errors] = ...
            compute_sensed_values( ...
                config_path, ...
                slope_MHz_us, ...
                chirp_cycle_period_us, ...
                ADC_SampleRate_MSps, ...
                num_adc_samples, ...
                frames_to_compute)
            
            %initialize the simulator
            simulator = Simulator_revB();
            simulator.load_params_from_JSON(config_path);

            %initialize the victim parameters
            simulator.Victim.FrequencySlope_MHz_us = slope_MHz_us;
            simulator.Victim.ChirpCycleTime_us = chirp_cycle_period_us;
            simulator.Victim.ADC_Samples = num_adc_samples;
            simulator.Victim.ADC_SampleRate_MSps = ADC_SampleRate_MSps;
            simulator.Victim.compute_calculated_vals();
            
            %set attacker parameters
            simulator.Attacker.Subsystem_tracking.FrequencySlope_MHz_us = slope_MHz_us;
            simulator.Attacker.Subsystem_tracking.ChirpCycleTime_us = chirp_cycle_period_us;
            simulator.Attacker.Subsystem_tracking.ADC_Samples = num_adc_samples;
            simulator.Attacker.Subsystem_tracking.ADC_SampleRate_MSps = ADC_SampleRate_MSps;
            simulator.Attacker.Subsystem_tracking.compute_calculated_vals();
            
            %apply timing offsets as desired
            simulator.Victim.timing_offset_us = 0;
            simulator.Attacker.Subsystem_tracking.timing_offset_us = 0;
            
            %configure the FMCW parameters
            simulator.configure_FMCW_Radar_parameters();
            
            %load default attacker, and victim positions and velocities
            simulator.load_realistic_attacker_and_victim_position_and_velocity();
            
            %load the target
            simulator.load_target_realistic(50,15);
        
            %initialize victim and simulation parameters
        
            
            %specify whether or not to record a movie of the range-doppler plot
            record_movie = false;
            simulator.Victim.Radar_Signal_Processor.configure_movie_capture(frames_to_compute, ...
                record_movie,50,15,40);
            
            %pre-compute the victim's chirps
            simulator.Victim.precompute_radar_chirps();
        
            %initialize the attacker parameters
            simulator.Attacker.initialize_attacker(...
                                    simulator.Victim.FMCW_sampling_rate_Hz * 1e-6,...
                                    simulator.Victim.StartFrequency_GHz,...
                                    simulator.Victim.Chirp_Tx_Bandwidth_MHz);
            
            %initialize the sensing subsystem's debugger
            simulator.Attacker.Subsystem_spectrum_sensing.initialize_debugger(0,simulator.Victim,frames_to_compute);
            %set to change 1 to zero to disable
            
            %specify the type of emulation ("target", 
                    % "velocity spoof - noisy", 
                    % "velocity spoof - similar velocity",
                    % "range spoof - similar slope")
            
            %attack_type = "range spoof - similar slope";
            attack_type = "range spoof - similar slope,velocity spoof - noisy";
            %attack_type = "target";
            %initialize the attacker
            simulator.Attacker.Subsystem_attacking.set_attack_mode(attack_type);
            
            %if it is desired to specify a specific attack location
            %simulator.Attacker.Subsystem_attacking.set_desired_attack_location(100,7);
        
            simulator.run_simulation_attack_no_target(frames_to_compute,false);
            
            %get the return values
        
            %frame duration
            estimated_frame_duration = simulator.Attacker.Subsystem_spectrum_sensing.frame_tracking.average_frame_duration * 1e-3;
            actual_frame_duration = simulator.Victim.FramePeriodicity_ms;
            %chirp duration
            estimated_chirp_duration = simulator.Attacker.Subsystem_spectrum_sensing.frame_tracking.average_chirp_duration;
            actual_chirp_duration = simulator.Victim.ChirpCycleTime_us;
            %slope
            estimated_slope = simulator.Attacker.Subsystem_spectrum_sensing.frame_tracking.average_slope;
            actual_slope = simulator.Victim.FrequencySlope_MHz_us;

            %adc sample rate
            ADC_SampleRate_MSps = simulator.Victim.ADC_SampleRate_MSps;

            %frame start time prediction errors
            %compare the predicted frame start times with the actual frame start times
            actual_frame_start_times = (3:frames_to_compute).' * simulator.Victim.FramePeriodicity_ms * 1e3 ... 
                                            + simulator.Victim.IdleTime_us;
            predicted_start_time_errors = simulator.Attacker.Subsystem_spectrum_sensing.frame_tracking.captured_frames(2:...
                simulator.Attacker.Subsystem_spectrum_sensing.frame_tracking.num_captured_frames,7) ...
                                            - actual_frame_start_times;
        end

        %{
            Purpose: run the given sensing subsystem performance test cases
            and saves the results to a csv file
            Inputs:
                num_cases - number of test cases to run
                frames_to_compute - number of frames to simulate for each
                    run
                slopes_MHz_us - array of slopes to test
                chirp_cycle_periods_us - array of chirp cycle periods
                adc_sampling_rates - array of adc sampling rates
                num_adc_samples - the number of ADC samples
                config_path - path to the json config
                save_file_name - path to save the results file to (without
                    the .csv extension
            Outputs:
                testing_data_parameter_estimations - key parameter
                    estimations for each test case
                testing_data_frame_start_prediction_errors - the error in
                    the frame start time prediction for each frame in each test
                    case
        %}
        function [testing_data_parameter_estimations, ...
                testing_data_frame_start_prediction_errors] = ...
                run_sensing_performance_test_cases( ...
                    num_cases, ...
                    frames_to_compute, ...
                    slopes_MHz_us, ...
                    chirp_cycle_periods_us, ...
                    adc_sampling_rates, ...
                    num_adc_samples,...
                    config_path, ...
                    save_file_name)

            save_file_headers = ["Actual Chirp Slope (Mhz/us)",...
                "Actual Chirp Cycle Period (us)",...
                "Actual Frame Duration (ms)",...
                "ADC Sample Rate (MSPS)",...
                "Estimated Chirp Slope (Mhz/us)",...
                "Estimated Chirp Cycle Period (us)",...
                "Estimated Frame Duration (ms)",...
                "Absolute Slope Error (MHz/us)",...
                "Absolute Chirp Cycle Period Error (us)",...
                "Absolute Frame Duration Error (ms)"];

            testing_data_parameter_estimations = zeros(num_cases,size(save_file_headers,2));
            testing_data_frame_start_prediction_errors = zeros(num_cases,frames_to_compute-2);
            
            status = sprintf("Running Test: %d of %d",1, num_cases);
            progress_bar = waitbar(0,status,"Name","Testing Sensing Subsystem");
            for i = 1:num_cases
                %update the waitbar
                status = sprintf("Running Test: %d of %d",i, num_cases);
                waitbar(i/num_cases,progress_bar,status);
            
                %run the test case
            
                            [actual_slope,estimated_slope,...
                        actual_chirp_duration,estimated_chirp_duration,...
                        actual_frame_duration, estimated_frame_duration,...
                        ADC_SampleRate_MSps, predicted_start_time_errors] = ...
                        characterization_functions.compute_sensed_values(config_path, ...
                            slopes_MHz_us(i), ...
                            chirp_cycle_periods_us(i), ...
                            adc_sampling_rates(i), ...
                            num_adc_samples, ...
                            frames_to_compute);
            
                %save values - resulting averages from simulation runs
                testing_data_parameter_estimations(i,1) = actual_slope;
                testing_data_parameter_estimations(i,2) = actual_chirp_duration;
                testing_data_parameter_estimations(i,3) = actual_frame_duration;
                testing_data_parameter_estimations(i,4) = ADC_SampleRate_MSps;
                testing_data_parameter_estimations(i,5) = estimated_slope;
                testing_data_parameter_estimations(i,6) = estimated_chirp_duration;
                testing_data_parameter_estimations(i,7) = estimated_frame_duration;
                testing_data_parameter_estimations(i,8) = abs(actual_slope - estimated_slope);
                testing_data_parameter_estimations(i,9) = abs(actual_chirp_duration - estimated_chirp_duration);
                testing_data_parameter_estimations(i,10) = abs(actual_frame_duration - estimated_frame_duration);

                %save values for frame_start_time prediction errors
                testing_data_frame_start_prediction_errors(i,:) = predicted_start_time_errors;
                test_data_results = array2table(testing_data_frame_start_prediction_errors);
                writetable(test_data_results,save_file_name + "_frame_start_time_prediction_errors.csv",...
                    'WriteRowNames',true); 
                
                %save the results to a file - continuously saving the data
                %allows for obtaining some data even in the event of a error or
                %crash

                %save the testing data for results
                test_data_results = array2table(testing_data_parameter_estimations,"VariableNames",save_file_headers);
                writetable(test_data_results,save_file_name + "_parameter_estimations.csv",'WriteRowNames',true); 
            end

            
        end
    end
end