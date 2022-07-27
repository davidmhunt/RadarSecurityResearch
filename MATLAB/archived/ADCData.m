classdef ADCData
    %UNTITLED Summary of this class goes here
    %   Detailed explanation goes here
    
    properties
        mmWave_device MMWaveDevice
        frames
        num_frames
    end
    
    methods 
        function obj = ADCData(adc_data_bin_file,MMWaveDevice)
            %UNTITLED Construct an instance of this class
            %   Detailed explanation goes here
            
            %initialize variables
            obj.mmWave_device = MMWaveDevice;
            obj.num_frames = obj.mmWave_device.num_frame;
            obj.frames = Frame.empty;
            
            %get adc data
            adc_data = obj.READ_ADC_DATA_BIN_FILE(adc_data_bin_file);
            
            %sort the adc data into frames
            for i = 1:obj.num_frames
                obj.frames(i) = Frame(obj.GET_FRAME(adc_data,i),obj.mmWave_device,i);
            end
        end
    end %end of public methods
    %% define class specific methods that are not used by other functions
    methods (Access = protected)
        function ret_adc_data = READ_ADC_DATA_BIN_FILE(obj,adc_data_bin_file)
            %METHOD1 Summary of this method goes here
            %   Detailed explanation goes here
            %% global variables
                % change based on sensor config
                numADCBits = obj.mmWave_device.adc_bits; % number of ADC bits per sample
                numLanes = 4; % do not change. number of lanes is always 4 even if only 1 lane is used. unused lanes
                isReal = 0; % set to 1 if real only data, 0 if complex data are populated with 0

            %% read file and convert to signed number
                % read .bin file
                fid = fopen(adc_data_bin_file,'r');
                % DCA1000 should read in two's complement data
                adc_data = fread(fid, 'int16');
                % if 12 or 14 bits ADC per sample compensate for sign extension
                if numADCBits ~= 16
                    l_max = 2^(numADCBits-1)-1;
                    adc_data(adc_data > l_max) = adc_data(adc_data > l_max) - 2^numADCBits;
                end
                fclose(fid);
            %% organize data by LVDS lane
                % for real only data
                if isReal
                    % reshape data based on one samples per LVDS lane
                    adc_data = reshape(adc_data, numLanes, []);
                    %for complex data
                else
                    % reshape and combine real and imaginary parts of complex number
                    adc_data = reshape(adc_data, numLanes*2, []);
                    adc_data = adc_data([1,2,3,4],:) + sqrt(-1)*adc_data([5,6,7,8],:);
                end
            %% return receiver data
            ret_adc_data = adc_data;
        end
        function [frame_adc_data] = GET_FRAME(obj,adc_data,frame)
            %{
                GET_FRAME gets the specified frame from the adc_data samples
                    Inputs:
                        adc_data: array with the adc_data stored in complex format with
                            a row for each LVDS lane
                        frame: the frame that data is desired from
                    Outputs
                        frame_adc_data: raw data for the desired frame stored in
                        complex format with a row for each LVDS lane
            %}
                assert(frame <= obj.mmWave_device.num_frame, "requested frame number is larger than number of frames recorded")

                samples_per_frame = obj.mmWave_device.num_sample_per_chirp * obj.mmWave_device.num_chirp_per_frame;
                stop_index = samples_per_frame * frame;
                start_index = stop_index - samples_per_frame + 1;
                frame_adc_data = adc_data(:,start_index:stop_index);
        end
    end
end

