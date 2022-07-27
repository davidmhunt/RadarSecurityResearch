classdef Chirp
    %UNTITLED Summary of this class goes here
    %   Detailed explanation goes here
    
    properties
        mmWave_device MMWaveDevice
        chirp_data
        num_LVDS_lanes
        chirp_number
        frame_number
    end
    
    methods
        function obj = Chirp(chirp_adc_samples,MMWaveDevice,chirp_number,frame_number)
            %UNTITLED Construct an instance of this class
            %   Detailed explanation goes here
            obj.chirp_data = chirp_adc_samples;
            obj.mmWave_device = MMWaveDevice;
            obj.chirp_number = chirp_number;
            obj.frame_number = frame_number;
        end
        
        function plot(obj,LVDS_lane)
            %get adc data setup
            real_data = real(obj.chirp_data(LVDS_lane,:));
            imaginary_data = imag(obj.chirp_data(LVDS_lane,:));

            %get time setup
            t = (1:size(obj.chirp_data,2))/(obj.mmWave_device.adc_samp_rate * 10^6);

            %plot the data
            plot(t,[real_data;imaginary_data]);
            legend('real','imaginary');
            ylabel('codes')
            xlabel('time (seconds)')
            grid on
            title(sprintf('Frame %d, Chirp %d, LVDS Lane %d',obj.frame_number, obj.chirp_number,LVDS_lane))
        end
    end
end

