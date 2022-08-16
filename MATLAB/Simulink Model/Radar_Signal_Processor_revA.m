%% For use in SIMULINK Model

%% Notes
%{
    - If the start frequency is higher than 77 GHz, there is a little bit
    of extra work that has to be done. When creating this simulation, I
    removed this functionality to simplify things. I did implement the
    functionality in previous versions though which can be found in the
    archived functions folder. See the parameters additional_BW_MHz and
    additional_sweep_time_us in the archived Radar class file and the
    FMCW_generate_chirp_sig_vals function

    - reconfigured the chrip construction. Chirps still have an idle period
    and a sampling period, but the end of the sampling period now marks the
    end of the chirp instead of there being a little bit of extra time
    after the end of the sampling period
%}

classdef Radar_Signal_Processor_revA < handle
    %UNTITLED3 Summary of this class goes here
    %   Detailed explanation goes here

    properties (Access = public)
        %Radar object associated with this Radar Signal Processing Object
        Radar

        %parameters for the lowpass filter design
        Fp                      %start frequency of pass band
        Fst                     %start of stop pand 
        Ap                      %ripple to allow in the pass band
        Ast                     %attenuation in the stop band
        Fs                      %sampling frequency
        low_pass_filter         %low pass filter object

        %instead of a lowpass filter, using a decimator (used int he
        %simulink model
        FIRDecimator        %dsp.FIRDecimator object
        decimation_factor   %the decimation factor to use

        %parameters used to select the samples corresponding to the sample
        %period
        num_samples_prior_to_sample_period
        num_samples_in_sampling_period

        %parameters for the Range-Doppler Response
        RangeDopplerResponse        %phased.RangeDopplerResponse object
        
        %parameters for the CA - CFAR detector
        CFARDetector2D              %phased.CFARDetector2D object
        guard_region
        training_region
        PFAR
        CUT_indicies
        distance_detection_range
        velocity_detection_range

        %parameters for the DBScan algorithm
        Epsilon
        minpts

        %parameters for the phased.RangeEstimator object
        RangeEstimator                  %phased.RangeEstimator object
        NumEstimatesSource_rng
        NumEstimates_rng
        ClusterInputPort_rng

        %parameters for the phased.DopplerEstimator object
        DopplerEstimator                %phased.DopplerEstimator object
        NumEstimatesSource_dplr
        NumEstimates_dplr
        ClusterInputPort_dplr

    end

    methods (Access = public)
        function obj = Radar_Signal_Processor_revA(Radar)
            %{
                Purpose: Initialize a new instance of the Radar Signal
                Processing Class that will perform all processing for the
                Radar Class
                Inputs:
                    Radar: a Radar class object that will be associated
                    with this particular radar signal processor
            %}
            obj.Radar = Radar;
        end


        %% [2] Functions to compute calculated radar parameters

        function configure_radar_signal_processor(obj)
            %{
                Purpose: this object configures all of the Radar Signal
                Processing components by calling the other configuration
                functions
            %}
            obj.configure_lowpass_filter();
            obj.configure_decimator();
            obj.configure_select_sampled_IF_sig();
            obj.configure_RangeDopplerResponse();
            obj.configure_Range_and_Doppler_Estimators();
            obj.configure_CFAR_detector();
            obj.configure_DB_scan();
        end
        
        function configure_lowpass_filter(obj)
            %{
                Purpose: function configures the lowpass filter that will
                be used to simulate how the ADC and mixer attenuate higher
                frequency components in a signal
            %}
            
            obj.Fp = 7 * 1e6;                                  %start frequency of pass band
            obj.Fst = 13 * 1e6;                                 %start of stop band 
            obj.Ap = 0.5;                                       %ripple to allow in the pass band
            obj.Ast = 40;                                       %attenuation in the stop band
            obj.Fs = obj.Radar.FMCW_sampling_rate_Hz;           %sampling frequency of the Radar object
            
            d = fdesign.lowpass('Fp,Fst,Ap,Ast',obj.Fp,obj.Fst,obj.Ap,obj.Ast,obj.Fs);
            obj.low_pass_filter = design(d,'butter','MatchExactly','passband');
%             %Hd = design(d,'equiripple');
%             %fvtool(Hd)
        end

        function configure_decimator(obj)
            %{
                Purpose: function configures the decimator that will
                be used to simulate how the ADC and mixer attenuate higher
                frequency components in a signal
            %}
            
            obj.FIRDecimator = dsp.FIRDecimator(obj.decimation_factor,designMultirateFIR(1,ceil(obj.decimation_factor/2)));
            %obj.FIRDecimator = dsp.FIRDecimator(obj.decimation_factor,'Auto');
        end

        function configure_select_sampled_IF_sig(obj)
            %{
                Purpose: function configures the variables needed to select
                the sampled IF signal from the fully dechirped signal
            %}
            obj.num_samples_prior_to_sample_period = ...
                int32((obj.Radar.ADC_ValidStartTime_us + obj.Radar.IdleTime_us) *...
                obj.Radar.ADC_SampleRate_MSps);
            obj.num_samples_in_sampling_period = obj.Radar.ADC_Samples;
        end
    
        function configure_RangeDopplerResponse(obj)
            %{
                Purpose: configures the phased.RangeDopplerResponse object
                that is used to compute the range-doppler response
            %}
            obj.RangeDopplerResponse = phased.RangeDopplerResponse(...
                "RangeMethod","FFT",...
                "PropagationSpeed",physconst('Lightspeed'),...
                'SampleRate',obj.Radar.ADC_SampleRate_MSps * 1e6, ...
                'SweepSlope',obj.Radar.FrequencySlope_MHz_us * 1e12,...
                'DechirpInput',false,...
                'RangeFFTLengthSource','Auto',...
                'RangeWindow',"Hann",...
                'ReferenceRangeCentered',false,...
                'ReferenceRange',0.0,...
                'PRFSource','Property',...
                'PRF',1/(obj.Radar.ChirpCycleTime_us * 1e-6),...
                'DopplerFFTLengthSource','Property',...
                'DopplerFFTLength',obj.Radar.NumChirps,...
                'DopplerWindow','Hann',...
                'DopplerOutput','Speed',...
                'OperatingFrequency',obj.Radar.StartFrequency_GHz * 1e9);
        end

        function configure_Range_and_Doppler_Estimators(obj)
            %{
                Purpose: Configures the Phased.RangeEstimator and
                Phased.DopplerEstimator objects that will be used to
                convert cluster ID's into range estimates
            %}

            %set key parameters for the range estimator
            obj.NumEstimatesSource_rng = 'Property';
            obj.NumEstimates_rng = 5;
            obj.ClusterInputPort_rng = true;

            %initialize the range estimator
            obj.RangeEstimator = phased.RangeEstimator(...
                "NumEstimatesSource",obj.NumEstimatesSource_rng,...
                "NumEstimates", obj.NumEstimates_rng,...
                "ClusterInputPort",obj.ClusterInputPort_rng);

            %set key parameters for the range estimator
            obj.NumEstimatesSource_dplr = 'Property';
            obj.NumEstimates_dplr = 5;
            obj.ClusterInputPort_dplr = true;

            %initialize the range estimator
            obj.DopplerEstimator = phased.DopplerEstimator(...
                "NumEstimatesSource",obj.NumEstimatesSource_dplr,...
                "NumEstimates", obj.NumEstimates_dplr,...
                "ClusterInputPort",obj.ClusterInputPort_dplr);
        end
        
        function configure_CFAR_detector(obj)
            %set PFAR to be 1e-8 for now
            obj.PFAR = 1e-8;
            
            %calculate training region size
            range_training_size = min(8,ceil(obj.Radar.ADC_Samples * 0.05));
            velocity_training_size = max(3,ceil(obj.Radar.NumChirps * 0.05));
            
            %put guard and training region sizes into arrays for the CFAR detector
            obj.guard_region = [2,1];
            obj.training_region = [range_training_size,velocity_training_size];
            
            %compute the max and min indicies for the cells under test
            max_detected_range_index = obj.Radar.ADC_Samples - obj.guard_region(1) - obj.training_region(1);
            min_detected_range_index = 1 + obj.guard_region(1) + obj.training_region(1);
            max_detected_velocity_index = obj.Radar.NumChirps - obj.guard_region(2) - obj.training_region(2);
            min_detected_velocity_index = 1 + obj.guard_region(2) + obj.training_region(2);
            
            %generate an array of all the indicies to perform a CFAR operation on
            range_indicies = (min_detected_range_index:max_detected_range_index).';
            velocity_indicies = min_detected_velocity_index:max_detected_velocity_index;
            
            CUT_range_indicies = repmat(range_indicies,1, size(velocity_indicies, 2));
            CUT_range_indicies = reshape(CUT_range_indicies,[],1);
            
            CUT_velocity_indicies = repmat(velocity_indicies,size(range_indicies,1),1);
            CUT_velocity_indicies = reshape(CUT_velocity_indicies,[],1);
            
            obj.CUT_indicies = [CUT_range_indicies CUT_velocity_indicies].';      
            
            %compute the observable range for the detector
            max_detected_range = obj.Radar.Ranges(max_detected_range_index);
            min_detected_range = obj.Radar.Ranges(min_detected_range_index);
            max_detected_velocity = obj.Radar.Velocities(max_detected_velocity_index);
            min_detected_velocity = obj.Radar.Velocities(min_detected_velocity_index);
            
            obj.distance_detection_range = [min_detected_range max_detected_range];
            obj.velocity_detection_range = [min_detected_velocity max_detected_velocity];

            obj.CFARDetector2D = phased.CFARDetector2D(...
                'Method','CA',...
                'GuardBandSize',obj.guard_region,...
                'TrainingBandSize',obj.training_region,...
                'ThresholdFactor','Auto',...
                'ProbabilityFalseAlarm',obj.PFAR,...
                'OutputFormat','Detection index',...
                'NumDetectionsSource','Auto');
        end
        
        function configure_DB_scan(obj)
        %{
            Purpose: Set's the values for Epsilon and the minpts for the
            DBScan algorithm used to identify radar clusters
        %}

            obj.Epsilon = 2;
            obj.minpts = 3;
        end       
        
        %% [3] Functions for processing the signals
        
        function sampled_IF_sig = FMCW_dechirp_and_decimate(obj,Tx_sig,Rx_sig)
            %{
                Purpose: simulates a received signal going through a mixer
                    and then getting sampled by the defender's ADC at its ADC
                    sampling frequency. 
                Inputs:
                    Tx_sig: the waveform of the signal transmitted by
                        the defender
                    Rx_sig: the signal that's received by the
                        defender after reflecting off of any targets. Includes
                        any attacker interference as well
                Outputs:
                    sampled_IF_sig: the sampled IF signal that would be
                        recorded by the DCA1000. Represents all of the samples
                        recorded for the given defender chirp
            %}
            %dechirp the received signal
            sampled_IF_sig = dechirp(Rx_sig,Tx_sig);

            %run the sampled IF signal through a decimator
            sampled_IF_sig = obj.FIRDecimator(sampled_IF_sig);

            %select only the portion of the dechirped signal that was in
            %the sampling period
            sampled_IF_sig = obj.select_sampled_IF_sig(sampled_IF_sig);
            
        end

        function sampled_IF_sig = select_sampled_IF_sig(obj,IF_sig)
            %{
                Purpose: this function selects the portion of the IF signal
                that occurs during the sampling period
            %}
            sampled_IF_sig = IF_sig(...
                obj.num_samples_prior_to_sample_period + 1:...
                obj.num_samples_prior_to_sample_period + obj.num_samples_in_sampling_period);
        end
     end
end