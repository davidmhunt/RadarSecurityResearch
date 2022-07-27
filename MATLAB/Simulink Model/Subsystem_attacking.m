classdef Subsystem_attacking  < handle
    %UNTITLED Summary of this class goes here
    %   Detailed explanation goes here

    properties

        %imported parameters
        StartFrequency_GHz
        FrequencySlope_MHz_us
        IdleTime_us                     %calculated value, not setable in UI
        RampEndTime_us                  %calculated value, not setable in UI                  
        Chirp_Tx_Bandwidth_MHz          %calculated value, not setable in UI
        ChirpCycleTime_us
        Lambda_m                        %calculated value, not setable in UI

        %frame parameters
        NumChirps
        FramePeriodicity_ms
        ActiveFrameTime_ms              %calculated value, not setable in UI

        %parameters for the FMCW waveform, not setable parameters in UI
        FMCW_sampling_rate_Hz
        FMCW_sampling_period_s
        chirp_BW_Hz
        sweep_time_s
        num_samples_idle_time
        num_samples_per_frame

        %parameters for the transmitter and receiver
        transmitter         %phased.Transmitter object

        %parameters for the Rx and Tx power when dealing with the FMCW
        %waveform, not setable parameters in the UI
        ant_aperture_m2
        ant_gain_dB

        tx_power_W
        tx_gain_dB
        

        %variables to aid in the computation of the attacking chirp
        phase_shift_per_chirp
        t                           %varialbe to hold the times at which to compute chirps

        %variable to hold all of the attacker chirps for a specific frame
        attacker_chirps
        
        %parameters for the desired position and velocity of the added
        %points
        desired_attack_range_m
        desired_attack_velocity_m_s
    end

    methods
        function obj = Subsystem_attacking()
            
        end

        function configure_transmitter(obj)
            %{
                Purpose: configures the transmitter object
                Note: all other parameters must be set before configuring
                the transmitter and receiver
            %}
            lambda = freq2wavelen(obj.StartFrequency_GHz * 1e9);


            obj.ant_aperture_m2 = 6.06e-4;                                  % in square meter
            obj.ant_gain_dB = aperture2gain(obj.ant_aperture_m2,lambda);    % in dB
            
            obj.tx_power_W = db2pow(5)*1e-3;                                % in watts
            obj.tx_gain_dB = 9+ obj.ant_gain_dB;                            % in dB

            obj.transmitter = phased.Transmitter( ...
                'PeakPower',obj.tx_power_W, ...
                'Gain',obj.tx_gain_dB);
        end
        
        function compute_calculated_values(obj)
            %{
                Purpose: computes all of the calculated parameters for the
                attacking subsystem
            %}
            %FMCW parameters
            obj.FMCW_sampling_period_s = 1/obj.FMCW_sampling_rate_Hz;
            obj.num_samples_idle_time = ...
                int32(obj.IdleTime_us ...
                * 1e-6 * obj.FMCW_sampling_rate_Hz);
            obj.chirp_BW_Hz = obj.Chirp_Tx_Bandwidth_MHz * 1e6;
            
            %other parameters
            obj.t = 0:obj.FMCW_sampling_period_s:obj.sweep_time_s - obj.FMCW_sampling_period_s;
            obj.phase_shift_per_chirp = 4 * pi * obj.desired_attack_velocity_m_s * obj.ChirpCycleTime_us * 1e-6 / obj.Lambda_m;

        end
    
        function update_attacker_chirps(obj,frame,attacker_pos,victim_pos)
        %{
            Purpose: updates the attacker_chirps array to be chirps for the
                given frame. Attacking chirps are also adjusted to account for
                the attacker position relative to the victim position
            Inputs:
                frame - the number of the current frame
                attacker_pos - attacker position in meters specified as
                    [x,y,z]
                victim_pos - victim position in meters specified as [x,y,z]
        %}
            %initialize the array of chirps
            obj.attacker_chirps = zeros(size(obj.t,2) + obj.num_samples_idle_time, obj.NumChirps);
        
            %generate the attacker chirps for the current frame
            %already existing time delay for propogation to the victim
            current_range = norm(pdist([attacker_pos.';victim_pos.']));
            propogation_delay = range2time(current_range,physconst('lightspeed'))/2;
            
            
            %time delay for range
            current_attack_range = obj.desired_attack_range_m - ...
                obj.desired_attack_velocity_m_s * ...
                obj.FramePeriodicity_ms * 1e-3 * ...
                (double(frame) - 1);
            desired_time_delay = range2time(current_attack_range,physconst('lightspeed'));
            additional_time_delay_needed = desired_time_delay - propogation_delay;
            num_samples_delay = additional_time_delay_needed / obj.FMCW_sampling_period_s;
            
            for chirp = 1:obj.NumChirps
                %compute the phase shift
                phase_shift = (chirp - 1) * obj.phase_shift_per_chirp;
            
                %compute the waveform
                waveform = cos(pi * obj.chirp_BW_Hz  ...
                / obj.sweep_time_s * obj.t.^(2) + phase_shift) + ...
                1i * sin(pi * obj.chirp_BW_Hz...
                / obj.sweep_time_s * obj.t.^(2) + phase_shift);
            
                %save it in the attacker_chirps array
                obj.attacker_chirps(:,chirp) = [zeros(obj.num_samples_idle_time,1); waveform.'];
            
                if int32(num_samples_delay) > 0
                    obj.attacker_chirps(:,chirp) = ...
                        [zeros(int32(num_samples_delay),1);...
                        obj.attacker_chirps(1: end - int32(num_samples_delay),chirp)];
                elseif num_samples_delay < 0
                            obj.attacker_chirps(:,chirp) = ...
                            [obj.attacker_chirps(int32(-1 * num_samples_delay) + 1: end,chirp);...
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