classdef Frame
    %UNTITLED2 Summary of this class goes here
    %   Detailed explanation goes here
    
    properties
        mmWave_device MMWaveDevice
        frame_data
        frame_number
        num_chirps
        chirps
    end
    
    methods
        function obj = Frame(frame_adc_samples,MMWaveDevice,frame_number)
            %Frame Construct an instance of this class
            %   Detailed explanation goes here
            
            %initialize variables
            obj.mmWave_device = MMWaveDevice;
            obj.frame_data = frame_adc_samples;
            obj.num_chirps = obj.mmWave_device.num_chirp_per_frame;
            obj.chirps = Chirp.empty;
            obj.frame_number = frame_number;
            
            %get the chrips corresponding to that frame
            for i = 1:obj.num_chirps
                obj.chirps(i) = Chirp(obj.GET_CHIRP(i),obj.mmWave_device,i,frame_number);
            end
        end
    end
    
    methods (Access = protected)
        function [chirp_adc_data] = GET_CHIRP(obj,chirp)
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
            assert(chirp <= obj.mmWave_device.num_chirp_per_frame, "requested chirp number is larger than number of chirps in a frame")

            stop_index = obj.mmWave_device.num_sample_per_chirp * chirp;
            start_index = stop_index - obj.mmWave_device.num_sample_per_chirp + 1;
            chirp_adc_data = obj.frame_data(:,start_index:stop_index);
        end
    end
end

