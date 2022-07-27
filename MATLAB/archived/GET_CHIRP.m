function [chirp_adc_data] = GET_CHIRP(frame_adc_data,mmwave_device_params,chirp)
%{
    GET_FRAME gets the specified frame from the adc_data samples
        Inputs:
            frame_adc_data: array with the adc_data stored in complex format with
                a row for each LVDS lane
            mmwave_device_params: device parameters for the mmwave device
            chirp: the frame that data is desired from
        Outputs
            chirp_adc_data: raw data for the desired frame stored in
            complex format with a row for each LVDS lane
%}
    assert(chirp <= mmwave_device_params.num_chirp_per_frame, "requested chirp number is larger than number of chirps in a frame")
    
    stop_index = mmwave_device_params.num_sample_per_chirp * chirp;
    start_index = stop_index - mmwave_device_params.num_sample_per_chirp + 1;
    chirp_adc_data = frame_adc_data(:,start_index:stop_index);
end

