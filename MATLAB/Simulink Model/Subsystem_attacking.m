classdef Subsystem_attacking  < handle
    %UNTITLED Summary of this class goes here
    %   Detailed explanation goes here

    properties

        %the target emulator will associate itself with a Victim and will
        %emulate radar targets based on this
        Attacker

        %variables to track estimated parameters from the sensing subsystem
        chirps_to_compute                   %for determining how many chirps to compute
        estimated_chirp_cycle_time_us
        estimated_frequency_slope_MHz_us
        estimated_frame_periodicity_ms

        %calculated victim parameters based on the estimated parameters
        sweep_time_s
        idle_time_s
        num_samples_per_chirp
        num_samples_sweep_time
        num_samples_idle_time

        %variables to aid in the computation of the emulated chirp
        phase_shift_per_chirp
        t                           %varialbe to hold the times at which to compute chirps

        FrequencySlope_MHz_us       %initializing a varialbe to hold the slope of the emulated chirps as some attacks adjust this
        
        %variables to be used for simulating random phase variation used in
        %the "similar velocity" mode
        sigma                               %the variance of the additional phase variation
        velocity_spoof_adjustment           %the amount to adjust the velocity by (affects phase shift per chirp)
        additional_phase_variation          %for executing velocity attacks
        additional_time_delay_us            %for executing range attacks

        %variable to hold all of the emulated chirps for a specific frame
        emulated_chirps

        
        
        %parameters for the desired position and velocity of the added
        %points
        desired_range_m
        desired_velocity_m_s

        %parameter to keep track of the current frame that the emulator is
        %on
        frame

        %specify the type of emulation ("target", 
        % "velocity spoof - noisy", 
        % "velocity spoof - similar velocity",
        % "range spoof - similar slope")
        attack_mode

    end

    methods
        function obj = Subsystem_attacking(Attacker)
            %{
                Purpose: the purpose of the class is to initialize an FMCW
                Radar target emulator that will replicate what a radar
                would see from a target
                Inputs:
                    Attacker: The Attacker class object that will hold the
                    chirp values
                    desired_target_range_m: the desired range of the
                        emulated target
                    desired_target_velocity_m_s: the desired velocity of
                        the emulated target
            %}
            obj.Attacker = Attacker;
            
            %set the first frame to zero
            obj.frame = 0;
            
        end

        function set_attacker_parameters(obj, desired_range_m, desired_velocity_m_s, attack_mode)
            %{
                Purpose: initialize the attacker parameters for desired
                    range, desired velocity, and the attack mode
                Inputs:
                    desired_target_range_m: the desired range of the
                        emulated target
                    desired_target_velocity_m_s: the desired velocity of
                        the emulated target
                    attack_mode: the mode of the attacker
            %}
            obj.desired_range_m = desired_range_m;
            obj.desired_velocity_m_s = desired_velocity_m_s;
            obj.attack_mode = attack_mode;
        end

        function compute_calculated_values(obj, chirp_cycle_time_us, frequency_slope_MHz_us,estimated_frame_periodicity_ms, num_chirps)
            %{
                Purpose: computes all of the calculated parameters for the
                    attacking subsystem
                Inputs:
                    chirp_cycle_time_us: the estimated chirp cycle time in
                        us
                    frequency_slope_MHz_us: the estimated chirp slope in
                        MHz/us
            %}
            
            %determine needed parameters
            fmcw_sampling_period_s = 1/(obj.Attacker.FMCW_sample_rate_Msps * 1e6);
            attacker_lambda_m = physconst("LightSpeed") * obj.Attacker.StartFrequency_GHz * 1e9;

            %save estimated parameter values
            obj.chirps_to_compute = num_chirps;
            obj.estimated_chirp_cycle_time_us = chirp_cycle_time_us;
            obj.estimated_frequency_slope_MHz_us = frequency_slope_MHz_us;
            obj.estimated_frame_periodicity_ms = estimated_frame_periodicity_ms;
            
            %compute the sweep time and idle time for each chirp based on
            %the estimated parameters
            obj.calculate_victim_parameters()
            
            obj.t = 0:fmcw_sampling_period_s:obj.sweep_time_s ...
                - fmcw_sampling_period_s;
            
            %compute range specific parameters
            if contains(obj.attack_mode,"range spoof")
                obj.initialize_range_spoof();
            else
                obj.FrequencySlope_MHz_us = obj.estimated_frequency_slope_MHz_us;
                obj.additional_time_delay_us = 0;
            end
            
            %compute velocity specific parameters
            obj.phase_shift_per_chirp = 4 * pi * obj.desired_velocity_m_s ...
                * obj.estimated_chirp_cycle_time_us * 1e-6 / attacker_lambda_m;

            if contains(obj.attack_mode,"velocity spoof")
                obj.initialize_velocity_spoof()
            else
                obj.additional_phase_variation = zeros(obj.chirps_to_compute,1);
            end
        end

        function calculate_victim_parameters(obj)
        %{
            Purpose: computes additional victim parameters like the sweep
            time, idle time, number of samples in the idle time, and
            samples per chirp
        %}
            %determine the number of samples in a chirp
            obj.num_samples_per_chirp = round(obj.estimated_chirp_cycle_time_us *...
                obj.Attacker.FMCW_sample_rate_Msps);

            %recompute the chirp cycle time so that it is a multiple of the
            %FMCW sampling rate
            obj.estimated_chirp_cycle_time_us = obj.num_samples_per_chirp /...
                (obj.Attacker.FMCW_sample_rate_Msps * 1e6) * 1e6;
            
            %compute the sweep and idle times
            obj.sweep_time_s = (obj.Attacker.Bandwidth_MHz / obj.estimated_frequency_slope_MHz_us) * 1e-6;
            obj.num_samples_sweep_time = round(obj.sweep_time_s *...
                obj.Attacker.FMCW_sample_rate_Msps * 1e6);
            obj.sweep_time_s = obj.num_samples_sweep_time /...
                (obj.Attacker.FMCW_sample_rate_Msps * 1e6);
            
            %compute the idle time, and correct the sweep time if the sweep
            %time is longer than the chirp cycle time
            if obj.sweep_time_s * 1e6 > obj.estimated_chirp_cycle_time_us
                obj.sweep_time_s = obj.estimated_chirp_cycle_time_us * 1e-6;
                obj.idle_time_s = 0;
                obj.num_samples_idle_time = 0;
            else
                obj.idle_time_s = obj.estimated_chirp_cycle_time_us * 1e-6 - obj.sweep_time_s;
                obj.num_samples_idle_time = obj.num_samples_per_chirp - obj.num_samples_sweep_time;
            end
        end
    
        function initialize_range_spoof(obj)
            %{
                Purpose: initializes unique range spoofing attacks
            %}
            if contains(obj.attack_mode,"similar slope")
                obj.FrequencySlope_MHz_us = obj.estimated_frequency_slope_MHz_us + obj.estimated_frequency_slope_MHz_us * 0.007; %use 0.015 for low BW attacks
                obj.additional_time_delay_us = ...
                    0.5 * obj.Attacker.Bandwidth_MHz *...
                    (1/obj.estimated_frequency_slope_MHz_us - 1/obj.FrequencySlope_MHz_us); %use 0.4 for low BW attacks
            end
        end

        function initialize_velocity_spoof(obj)

            attacker_lambda_m = physconst("LightSpeed") * obj.Attacker.StartFrequency_GHz * 1e9;

            if contains(obj.attack_mode,"noisy")
                %compute any additional phase variation (mu = 0)
                num_bins_to_target = max(obj.chirps_to_compute * 0.2,3);
                obj.sigma = num_bins_to_target * 2 * pi / obj.chirps_to_compute;
    
                %adjust the velocity by the desired amount

                %if there is also a range spoof, don't adjust the velocity
                if contains(obj.attack_mode,"range spoof")
                    obj.velocity_spoof_adjustment = 0;
                else
                    obj.velocity_spoof_adjustment = -3.0;
                end
                obj.phase_shift_per_chirp = 4 * pi * ...
                    (obj.desired_velocity_m_s + obj.velocity_spoof_adjustment) ...
                    * obj.estimated_chirp_cycle_time_us * 1e-6 / attacker_lambda_m;
            elseif contains(obj.attack_mode,"similar velocity")
                %adjust the velocity by the desired amount
                obj.velocity_spoof_adjustment = 0;
                obj.phase_shift_per_chirp = 4 * pi * ...
                    (obj.desired_velocity_m_s + obj.velocity_spoof_adjustment) ...
                    * obj.estimated_chirp_cycle_time_us * 1e-6 / attacker_lambda_m;

                %compute what the phase shift values would actually be with
                %no attack
                phase_shift_at_chirps = obj.phase_shift_per_chirp * (0:obj.chirps_to_compute - 1);
                

                %compute what the phase shift values should be with the
                %attack
                num_bins_to_target = max(obj.chirps_to_compute * 0.10,3);
                max_phase_shift = num_bins_to_target * 2 * pi / obj.chirps_to_compute;

                phase_shift_change_per_chirp = 2 * max_phase_shift / obj.chirps_to_compute;
                desired_phase_shift_deltas = (-1 * max_phase_shift : phase_shift_change_per_chirp : ...
                    max_phase_shift - phase_shift_change_per_chirp) ...
                    + obj.phase_shift_per_chirp;
                desired_phase_shift_at_chirps = zeros(1,obj.chirps_to_compute);

                for chirp = 1:obj.chirps_to_compute - 1
                    desired_phase_shift_at_chirps(chirp + 1) = ...
                        desired_phase_shift_at_chirps(chirp) + ...
                        desired_phase_shift_deltas(chirp);
                end
                obj.additional_phase_variation = desired_phase_shift_at_chirps - phase_shift_at_chirps;
            end
        end
    
        function compute_noisy_velocity_spoof_values(obj)
            %for normal distribution
            %obj.additional_phase_variation = randn(obj.chirps_to_compute,1) * obj.sigma;
    
            %for uniform distribution
            obj.additional_phase_variation = (rand(obj.chirps_to_compute,1) - 0.5) * 2 * obj.sigma;
        end

        function compute_next_emulated_chirps(obj)
        %{
            Purpose: updates the chirps_emulated array to be chirps for the
                current frame. Attacking chirps are also adjusted to account for
                the attacker position relative to the victim position.
                After generating the emulated chirps for the current frame,
                the frame number is incremented so that the next time the
                function is called the next frame's chirps is automatically
                computed
        %}
            %increment the frame counter to be the next frame
            obj.frame = obj.frame + 1;
            
            %initialize the array of chirps
            obj.emulated_chirps = zeros(obj.num_samples_per_chirp, obj.chirps_to_compute);
            
            %if velocity spoof, update the additional phase_variation
            %values
            if contains(obj.attack_mode,"velocity spoof - noisy")
                obj.compute_noisy_velocity_spoof_values()
            end
            
            %time delay for current emulation range
            current_emulation_range = obj.desired_range_m - ...
                obj.desired_velocity_m_s * ...
                obj.estimated_frame_periodicity_ms * 1e-3 * ...
                (double(obj.frame) - 1);
            desired_time_delay = range2time(current_emulation_range,physconst('lightspeed')) + ...
                obj.additional_time_delay_us * 1e-6;
            propagation_delay = range2time(obj.Attacker.current_victim_pos ,physconst('lightspeed'))/2;
            time_delay = desired_time_delay - propagation_delay;
            num_samples_delay = round(time_delay * (obj.Attacker.FMCW_sample_rate_Msps * 1e6));
            
            for chirp = 1:int32(obj.chirps_to_compute)
                %compute the phase shift
                phase_shift = (double(chirp) - 1) * obj.phase_shift_per_chirp;
            
                %compute the waveform
                waveform = cos(pi * obj.FrequencySlope_MHz_us * 1e12 ...
                    * obj.t.^(2) + phase_shift + obj.additional_phase_variation(chirp)) + ...
                    1i * sin(pi * obj.FrequencySlope_MHz_us * 1e12...
                    * obj.t.^(2) + phase_shift + obj.additional_phase_variation(chirp));
            
                %save it in the emulated_chirps array
                obj.emulated_chirps(1:obj.num_samples_sweep_time,chirp) = waveform.';
            
                if int32(num_samples_delay) > 0
                    obj.emulated_chirps(:,chirp) = ...
                        [zeros(int32(num_samples_delay),1);...
                        obj.emulated_chirps(1: end - int32(num_samples_delay),chirp)];
                elseif num_samples_delay < 0
                            obj.emulated_chirps(:,chirp) = ...
                            [obj.emulated_chirps(int32(-1 * num_samples_delay) + 1: end,chirp);...
                            zeros(int32(-1 * num_samples_delay),1)];
                end
            end
        end

        function Tx_sig_attacker = get_transmitted_attacker_chirp(obj,chirp)
            %{
                Purpose: retrieves the specific attacking chirp and
                    computes the transmitted signal
                Inputs:
                    chirp - the attacking chirp number
                Outputs:
                    Tx_sig_attacker - the transmitted attacking chirp
                        signal
            %}
            attacker_sig = obj.attacker_chirps(:,chirp);
            Tx_sig_attacker = obj.transmitter(attacker_sig);
        end
    end
end