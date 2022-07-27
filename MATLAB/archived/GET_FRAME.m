function [frame_adc_data] = GET_FRAME(adc_data,mmwave_device_params,frame)
%{
    GET_FRAME gets the specified frame from the adc_data samples
        Inputs:
            adc_data: array with the adc_data stored in complex format with
                a row for each LVDS lane
            mmwave_device_params: device parameters for the mmwave device
            frame: the frame that data is desired from
        Outputs
            frame_adc_data: raw data for the desired frame stored in
            complex format with a row for each LVDS lane
%}
    assert(frame <= mmwave_device_params.num_frame, "requested frame number is larger than number of frames recorded")
    
    samples_per_frame = mmwave_device_params.num_sample_per_chirp * mmwave_device_params.num_chirp_per_frame;
    stop_index = samples_per_frame * frame;
    start_index = stop_index - samples_per_frame + 1;
    frame_adc_data = adc_data(:,start_index:stop_index);
end

