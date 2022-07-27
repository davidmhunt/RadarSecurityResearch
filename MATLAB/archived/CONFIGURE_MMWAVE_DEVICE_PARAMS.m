function [mmwave_device_params] = CONFIGURE_MMWAVE_DEVICE_PARAMS(adc_data_bin_file,mmwave_setup_json_file)
%{
CONFIGURE_MMWAVE_DEVICE_PARAMS reads the mmwave_setup json file containing
useful information on the mmwave device configuration
   Inputs
        adc_data_bin_file: path to the the bin file of the raw ADC data generated from
            the DCA1000 board
        mmwave_setup_json_file: path to the mmwave.json file generated in mmwave studio 
    Outputs
%}
    mmwave_device_params.freq = 77.5999e9; %get this information from the actual radar
    mmwave_device_params.lambda = physconst('LightSpeed') / mmwave_device_params.freq;

    mmwave_device_params.aoa_angle = [0:1:180];%not using yet
    mmwave_device_params.num_angle = length(mmwave_device_params.aoa_angle); %not using yet

    % read json config format
    sys_param_json = jsondecode(fileread(mmwave_setup_json_file));
    mmwave_device_params.num_byte_per_sample = 4;
    mmwave_device_params.num_rx_chnl = 4;

    mmwave_device_params.num_sample_per_chirp = sys_param_json.mmWaveDevices.rfConfig.rlProfiles.rlProfileCfg_t.numAdcSamples;
    mmwave_device_params.num_chirp_per_frame = sys_param_json.mmWaveDevices.rfConfig.rlFrameCfg_t.numLoops;

    % coefficient for the Hanning window
    mmwave_device_params.win_hann = hanning(mmwave_device_params.num_sample_per_chirp);

    % size of one frame in Byte
    mmwave_device_params.size_per_frame = mmwave_device_params.num_byte_per_sample * mmwave_device_params.num_rx_chnl...
                               * mmwave_device_params.num_sample_per_chirp * mmwave_device_params.num_chirp_per_frame;

    % get # of frames and samples per frame
    try
        bin_file = dir(adc_data_bin_file);
        bin_file_size = bin_file.bytes;
    catch
        error('Reading Bin file failed');
    end
    mmwave_device_params.num_frame = floor( bin_file_size/mmwave_device_params.size_per_frame );
    if mmwave_device_params.num_frame ~= sys_param_json.mmWaveDevices.rfConfig.rlFrameCfg_t.numFrames
        error('Computed # of frames not equal to that in the JSON file');
    end
    mmwave_device_params.num_sample_per_frame = mmwave_device_params.num_rx_chnl * mmwave_device_params.num_chirp_per_frame * mmwave_device_params.num_sample_per_chirp;

    % ADC specs
    mmwave_device_params.adc_samp_rate = sys_param_json.mmWaveDevices.rfConfig.rlProfiles.rlProfileCfg_t.digOutSampleRate/1000;
    if 2 == sys_param_json.mmWaveDevices.rfConfig.rlAdcOutCfg_t.fmt.b2AdcBits
        mmwave_device_params.adc_bits = 16;
    end
    mmwave_device_params.dbfs_coeff = - (20*log10(2.^(mmwave_device_params.adc_bits-1)) + 20*log10(sum(mmwave_device_params.win_hann)) - 20*log10(sqrt(2)));

    % chirp specs (MHz/usec)
    mmwave_device_params.chirp_slope = sys_param_json.mmWaveDevices.rfConfig.rlProfiles.rlProfileCfg_t.freqSlopeConst_MHz_usec;
    mmwave_device_params.bw = mmwave_device_params.chirp_slope * mmwave_device_params.num_sample_per_chirp / mmwave_device_params.adc_samp_rate;
    mmwave_device_params.chirp_ramp_time = sys_param_json.mmWaveDevices.rfConfig.rlProfiles.rlProfileCfg_t.rampEndTime_usec;
    mmwave_device_params.chirp_idle_time = sys_param_json.mmWaveDevices.rfConfig.rlProfiles.rlProfileCfg_t.idleTimeConst_usec;
    mmwave_device_params.chirp_adc_start_time = sys_param_json.mmWaveDevices.rfConfig.rlProfiles.rlProfileCfg_t.adcStartTimeConst_usec;
    mmwave_device_params.frame_periodicity = sys_param_json.mmWaveDevices.rfConfig.rlFrameCfg_t.framePeriodicity_msec;

    % range resolution
    mmwave_device_params.range_max = physconst('LightSpeed') * mmwave_device_params.adc_samp_rate*1e6 / (2*mmwave_device_params.chirp_slope*1e6*1e6);
    mmwave_device_params.range_res = physconst('LightSpeed') / (2*mmwave_device_params.bw*1e6);

    % velocity resolution
    mmwave_device_params.v_max = mmwave_device_params.lambda / (4*mmwave_device_params.chirp_ramp_time/1e6);
    mmwave_device_params.v_res = mmwave_device_params.lambda / (2*mmwave_device_params.num_chirp_per_frame*(mmwave_device_params.chirp_idle_time+mmwave_device_params.chirp_ramp_time)/1e6);

    % checking iqSwap setting
    mmwave_device_params.is_iq_swap = sys_param_json.mmWaveDevices.rawDataCaptureConfig.rlDevDataFmtCfg_t.iqSwapSel;
    % % Change Interleave data to non-interleave
    mmwave_device_params.is_interleave = sys_param_json.mmWaveDevices.rawDataCaptureConfig.rlDevDataFmtCfg_t.chInterleave;

    fprintf('# of sample/chirp: %d\n', mmwave_device_params.num_sample_per_chirp);
    fprintf('# of chirp/frame: %d\n', mmwave_device_params.num_chirp_per_frame);
    fprintf('# of sample/frame: %d\n', mmwave_device_params.num_sample_per_frame);
    fprintf('Size of one frame: %d Bytes\n', mmwave_device_params.size_per_frame);
    fprintf('# of frames in the raw ADC data file: %d\n\n', mmwave_device_params.num_frame);

    fprintf('Radar bandwidth: %.2f (GHz)\n\n', mmwave_device_params.bw/1000);

    fprintf('ADC sampling rate: %.2f (MSa/s)\n', mmwave_device_params.adc_samp_rate);
    fprintf('ADC bits: %d bit\n', mmwave_device_params.adc_bits);
    fprintf('dBFS scaling factor: %.2f (dB)\n\n', mmwave_device_params.dbfs_coeff);

    fprintf('Chirp duration: %.2f (usec)\n', mmwave_device_params.chirp_idle_time+mmwave_device_params.chirp_ramp_time);
    fprintf('Chirp slope: %.2f (MHz/usec)\n', mmwave_device_params.chirp_slope);
    fprintf('Chirp bandwidth: %.2f (MHz)\n', mmwave_device_params.bw);
    fprintf('Chirp "valid" duration: %.2f (usec)\n\n', mmwave_device_params.num_sample_per_chirp/mmwave_device_params.adc_samp_rate);

    fprintf('Frame length: %.2f (msec)\n', mmwave_device_params.num_chirp_per_frame*(mmwave_device_params.chirp_idle_time+mmwave_device_params.chirp_ramp_time)/1000);
    fprintf('Frame periodicity: %.2f (msec)\n', mmwave_device_params.frame_periodicity);
    fprintf('Frame duty-cycle: %.2f \n\n', mmwave_device_params.num_chirp_per_frame*(mmwave_device_params.chirp_idle_time+mmwave_device_params.chirp_ramp_time)/1000/mmwave_device_params.frame_periodicity);

    fprintf('Range limit: %.4f (m)\n', mmwave_device_params.range_max);
    fprintf('Range resolution: %.4f (m)\n', mmwave_device_params.range_res);
    fprintf('Velocity limit: %.4f (m/s)\n', mmwave_device_params.v_max);
    fprintf('Velocity resolution: %.4f (m/s)\n\n', mmwave_device_params.v_res);

    fprintf('IQ swap?: %d\n', mmwave_device_params.is_iq_swap);
    fprintf('Interleaved data?: %d\n', mmwave_device_params.is_interleave);
    fprintf('...System configuration imported!\n\n')
end