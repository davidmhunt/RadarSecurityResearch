  clear;
  values = zeros(0,0);
  NUM_RUNS = 3;
  FRAMES_PER_RUN = 15;
  MAX_BIN = 20;
  TOTAL_FRAMES = NUM_RUNS * FRAMES_PER_RUN;

 for i = 1: NUM_RUNS
     % first plot -> range vs. P(detection)
     % range b/t 10-90 m (10 m bins)
     % -15 to 15 
     % one single frame
     % attack doesn't become operational until the fifth frame -> 10 frames
     % when we test attack -> last five frames are under attack
     % look at the attack
 
     [detected, actual_ranges, estimated_ranges, estimated_velocities, actual_velocities, percent_error_ranges, percent_error_velocities] = sim_wrap(randi(88)+12,randi(20));
    
     values = [values; actual_ranges', actual_velocities', detected(:,1), estimated_ranges(:,1), percent_error_ranges, estimated_velocities(:,1), percent_error_velocities]
 end

 bin_classification = cast(values(:,1)/10, "int16")';
 table_data = array2table(values,"VariableNames",["Range (m)","Velocity (m/s)","Detected","Estimated Range (m)", "Error Range (m)", "Estimated Velocity (m/s)", "Error Velocity (m)"]);

 %% P(detection) vs. Range
 histogram_vals = zeros(MAX_BIN,2);

for bin_num = 1:MAX_BIN
    histogram_vals(bin_num, 1) = bin_num;
    count = 0;
    detected = 0;
  for frame = 1: TOTAL_FRAMES
    if (bin_classification(frame) == bin_num)
        count = count + 1;
        if (values(frame, 3) == 1)
            detected = detected + 1;
        end
    end
  end
   if (count ~= 0)
        histogram_vals(bin_num, 2) = detected/count;
    else 
        histogram_vals(bin_num, 2) = 0;
    end
end
figure
bar(histogram_vals(:,1)*10, histogram_vals(:,2))
ylabel("P(detection)")
xlabel("Range Bin (m)")
title("P(detection) at Various Target Ranges")

%% Error vs Range
range_error_plot = zeros(0,0);
  for frame = 1: TOTAL_FRAMES
    if (values(frame, 3) ~= 0)
        range_error_plot = [range_error_plot; [values(frame, 1), values(frame, 5)]];
    end
  end
figure
scatter(range_error_plot(:,1), range_error_plot(:,2))
ylabel("Range Error (m)")
xlabel("Range (m)")
title("Range Error at Various Distances")