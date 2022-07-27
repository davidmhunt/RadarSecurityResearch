function PLOT_CHIRP_DATA(chirp_adc_data,mmwave_device_params)
%UNTITLED2 Summary of this function goes here
%   Detailed explanation goes here

   %get adc data setup
    real_data = real(chirp_adc_data);
    imaginary_data = imag(chirp_adc_data);
    
    %get time setup
    t = (1:size(chirp_adc_data,2))/(mmwave_device_params.adc_samp_rate * 10^6);
    
    %plot the data
    plot(t,[real_data;imaginary_data]);
    legend('real','imaginary');
    ylabel('codes')
    xlabel('time (seconds)')
    grid on
    title('Frame 1, Chirp 1, LVDS Lane 2')
end

