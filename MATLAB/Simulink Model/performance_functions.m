classdef performance_functions

    % is the target being detected 
    % error if it is
    % additional detection if not there 
    methods(Static)
        function performance_evaluation_functions(frames_to_compute, range_estimates)
            p_detection(frames_to_compute, range_estimates)

        end

        function [range_errors] = range_percent_error(range_actual, range_estimates)
           valid_range_column = range_estimates(:,1);
            range_errors = zeros(size(valid_range_column));
                
            for idx = 1: size(valid_range_column)
                if ~(isnan(valid_range_column(idx)))
                    error_position = abs(valid_range_column(idx) - range_actual(idx));
                    frame_error = error_position / range_actual(idx) * 100;
                    range_errors(idx) = frame_error;     
                end
   
            end
        end

        function [range_actual] = actual_ranges(frames_to_compute, frame_duration, target_velocity, victim_velocity, target_start, victim_start)
            range_actual = zeros(size(frames_to_compute));
            for idx = 1: frames_to_compute
                 range_actual(idx) = norm((victim_start-target_start) + frame_duration*(idx-1)*(victim_velocity(1)-target_velocity(1)));
            end
        end

        function [detected] = detection(frames_to_compute, range_estimates)
            valid_range_column = range_estimates(:,1);
            detected = zeros(size(frames_to_compute));
             for idx = 1: height(valid_range_column)
                 if ~(isnan(valid_range_column(idx)))
                    detected(idx) = 1;
                 end
             end
        end

        function [velocity_actual] = actual_velocities(frames_to_compute, target_velocity, victim_velocity)
            velocity_actual = zeros(size(frames_to_compute));
            for idx = 1: frames_to_compute
                 velocity_actual(idx) = victim_velocity(1) - target_velocity(1);
             end
        end


        function [velocity_errors] = velocity_percent_error(velocity_estimates, velocity_actual)
            valid_velocity_column = velocity_estimates(:,1);
            velocity_errors = zeros(size(valid_velocity_column));
                
            for idx = 1: size(valid_velocity_column)
                if ~(isnan(valid_velocity_column(idx)))
                    error_velocity = abs(valid_velocity_column(idx) - velocity_actual(idx));
                    frame_error =  error_velocity / abs(velocity_actual(idx)) * 100;
                    velocity_errors(idx) = frame_error;     
                end
   
            end
        end
    end
end
