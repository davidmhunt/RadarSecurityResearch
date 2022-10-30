function [detected, actual_ranges, estimated_ranges, estimated_velocities, actual_velocities, percent_error_ranges, percent_error_velocities, false_positives] = sim_wrap_attack(user, json_config_file, frames_per_sim, target_start, target_velocity)

simulator = Simulator_revB();

david_file_path = "/home/david/Documents/RadarSecurityResearch/MATLAB/Simulink Model/config_files/";
kristen_file_path = "C:\Users\krist\OneDrive\Documents\2022-2023 School Year\Radar Security Project\RadarSecurityResearch\MATLAB\Simulink Model\config_files\";

% file_path = "B210_params.json";
% file_path = "B210_params_highBW.json";
% file_path = "B210_params_highvres.json";
% file_path = "B210_params_lowBW.json";
% file_path = "B210_params_sensing_system.json";
% file_path = "X310_params_100MHzBW.json";
% file_path = "realistic_params.json";


if (user == "kristen")
    user_path = kristen_file_path;
else 
    user_path = david_file_path;
end

simulator.load_params_from_JSON(user_path + json_config_file);


%apply timing offsets as desired
simulator.Victim.timing_offset_us = 0;
simulator.Attacker.Subsystem_tracking.timing_offset_us = 0;

%configure the FMCW parameters
simulator.configure_FMCW_Radar_parameters();


%load default attacker, and victim positions and velocities
simulator.load_realistic_attacker_and_victim_position_and_velocity();

%print out key parameters
simulator.Victim.print_chirp_parameters;

simulator.Victim.print_frame_parameters;
%log detection first, then log all the data about it
simulator.Victim.print_performance_specs;
simulator.Victim.print_FMCW_specs;

simulator.load_target_realistic(target_start, target_velocity);

%specify the number of frames and chirps to compute

frames_to_compute = frames_per_sim;

%pre-compute the victim's chirps
simulator.Victim.precompute_radar_chirps();

%initialize the attacker parameters
simulator.Attacker.initialize_attacker(...
                        simulator.Victim.FMCW_sampling_rate_Hz * 1e-6,...
                        simulator.Victim.StartFrequency_GHz,...
                        simulator.Victim.Chirp_Tx_Bandwidth_MHz);

%initialize the sensing subsystem's debugger
simulator.Attacker.Subsystem_spectrum_sensing.initialize_debugger(1,simulator.Victim,frames_to_compute);

%specify the type of emulation ("target", 
        % "velocity spoof - noisy", 
        % "velocity spoof - similar velocity",
        % "range spoof - similar slope")

attack_type = "velocity spoof - similar velocity, range spoof - similar slope";
%initialize the attacker
simulator.Attacker.Subsystem_attacking.set_attack_mode(attack_type);

%if it is desired to specify a specific attack location
%simulator.Attacker.Subsystem_attacking.set_desired_attack_location(75,7);

%run the simulation (without an attacker for now)
simulator.run_simulation_with_attack(frames_to_compute,false);

% report the range estimates
estimated_ranges = simulator.Victim.Radar_Signal_Processor.range_estimates
% compute the actual range per frame
actual_ranges = performance_functions.actual_ranges(frames_to_compute, simulator.Victim.FramePeriodicity_ms*.001, simulator.SimulatedTarget.velocity_meters_per_s, simulator.Victim.velocity_m_per_s, simulator.SimulatedTarget.position_m, simulator.Victim.position_m)
% compute the percent error of the range estimate for each frame
percent_error_ranges = performance_functions.range_percent_error(actual_ranges, estimated_ranges)
% report the velocity estimates
estimated_velocities = simulator.Victim.Radar_Signal_Processor.velocity_estimates
% compute the actual velocity per frame
actual_velocities = performance_functions.actual_velocities(frames_to_compute, simulator.SimulatedTarget.velocity_meters_per_s, simulator.Victim.velocity_m_per_s)
% compute the velocity percent error per frame
percent_error_velocities = performance_functions.velocity_percent_error(estimated_velocities, actual_velocities)

% determine if an object has been detected in each frame, and determine
% false positives
[detected, false_positives] = performance_functions.detection(frames_to_compute, estimated_ranges, actual_ranges, estimated_velocities, actual_velocities)

end
