classdef Processor
    %CLASS NOT CONFIGURED YET

    properties
        radar           %a radar object
        adc_data_cube   %the raw data (simulated or otherwise) from the radar
    end

    methods
        function obj = Processor(radar)
            %Processor Construct an instance of this class
            %   Detailed explanation goes here
            obj.radar = radar;
        end

        function [fft_out,ranges] = compute_range_fft(obj,desired_chirp, desired_frame)
            %{
                Purpose: computes the range fft for a given chirp and frame
                Inputs:
                    desired_chirp: the desired chirp to generate the
                        range_fft of
                    desired_frame: the desired frame to generate the
                        range_fft of
            %}
            outputArg = obj.radar + inputArg;
        end
    end
end