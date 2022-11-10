% Class to house performance functions, which evaluate the performance of
% the radar's estimations of range and velocity (in progress)

classdef performance_functions
    methods(Static)
        % function to compute the range error (by frame)
        function [range_errors] = range_error(range_actual, range_estimates, col_detection)
            range_errors = zeros(size(range_estimates(:,1)));
            for idx = 1: size(range_estimates(:,1))
                if (col_detection(idx) ~= 0)
                    valid_range_column = range_estimates(:,col_detection(idx));
                    if ~(isnan(valid_range_column(idx)))
                        error_position = abs(valid_range_column(idx) - range_actual(idx));
                        range_errors(idx) = error_position;
                    end
                end
            end
        end

        % function to compute the actual distance b/t actual target and 
        % victim
        function [range_actual] = actual_ranges(frames_to_compute, frame_duration, target_velocity, victim_velocity, target_start, victim_start)
            range_actual = zeros(size(frames_to_compute));
            for idx = 1: frames_to_compute
                 range_actual(idx) = norm((victim_start-target_start) + frame_duration*(idx-1)*(victim_velocity-target_velocity));
            end
        end

        % function to determine if an object has been detected in each
        % frame
        function [detected, col_detection, false_positives] = detection(frames_to_compute, range_estimates, actual_ranges, velocity_estimates, actual_velocities)
            detected = zeros(frames_to_compute);
            false_positives = zeros(frames_to_compute);
            col_detection = zeros(frames_to_compute);
            for col = 1:size(range_estimates, 2)
               valid_range_column = range_estimates(:,col);
               valid_velocity_column = velocity_estimates(:,col);
             for idx = 1: height(valid_range_column)
                 if (~isnan(valid_range_column(idx)))
                     if ((abs(valid_range_column(idx) - actual_ranges(idx))< 5) && (abs(valid_velocity_column(idx) - actual_velocities(idx))< 5))
                            detected(idx) = 1;
                            col_detection(idx) = col;
                    else
                     false_positives(idx) = 1;
                     end
                 end
             end
            end
        end
        
        % function to determine the actual relative velocity of the target 
        % for each frame
        function [velocity_actual] = actual_velocities(frames_to_compute, target_velocity, victim_velocity)
            velocity_actual = zeros(size(frames_to_compute));
            for idx = 1: frames_to_compute
                 velocity_actual(idx) = victim_velocity(1) - target_velocity(1);
             end
        end

        % function to determine the velocity error (per frame)
        function [velocity_errors] = velocity_error(velocity_estimates, velocity_actual, col_detection)
            velocity_errors = zeros(size(velocity_estimates(:,1)));
            for idx = 1: size(velocity_estimates(:,1))
                if (col_detection(idx) ~= 0)
                    valid_velocity_column = velocity_estimates(:,col_detection(idx));
                    if ~(isnan(valid_velocity_column(idx)))
                        error_position = abs(valid_velocity_column(idx) - velocity_actual(idx));
                        velocity_errors(idx) = error_position;
                    end
                end
            end
        end
    end
end
