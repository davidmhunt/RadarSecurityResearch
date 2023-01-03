#ifndef ATTACKINGSUBSYSTEM
#define ATTACKINGSUBSYSTEM

    //c standard library
    #include <iostream>
    #include <cstdlib>
    #include <string>
    #include <complex>
    #include <csignal>
    #include <mutex>
    #include <thread>

    //JSON class
    #include <nlohmann/json.hpp>

    //source libraries
    #include "../JSONHandler.hpp"
    #include "../USRPHandler.hpp"
    #include "../BufferHandler.hpp"

    // add in namespaces as needed
    using json = nlohmann::json;
    using USRPHandler_namespace::USRPHandler;
    using Buffers::Buffer_2D;
    using Buffers::Buffer_1D;

    namespace AttackingSubsystem_namespace{

        template<typename data_type>
        class AttackingSubsystem {
            public:
                //enable flag
                bool enabled;

                //attack status
                bool attacking; //true when the system is attacking

                //attack complete (for sensing system to tell attacking subsystem that the attack is complete)
                bool sensing_complete;
            private:
                //pointer to usrp device
                USRPHandler<data_type> * attacker_usrp_handler;

                //configuration
                json config;

                //timing arguments
                double attack_start_time_ms;
                double stream_start_offset_us;
                std::vector<uhd::time_spec_t> frame_start_times;

                //for tracking how many frames have been loaded and the current attacking frame
                size_t current_attack_frame; //for tracking the current attack frame
                size_t next_frame_to_load; //for tracking the next frame to load
                bool new_frame_start_time_available;
                
                //FMCW arguments
                size_t num_attack_frames;

                //mutex for frame start times to ensure thread safety
                std::mutex frame_start_times_mutex;
                std::mutex sensing_complete_mutex;

            public:
                size_t attack_start_frame;
            private:
                double frame_periodicity_ms;

                //attack signal buffer
                double samples_per_buffer;
                std::string attack_signal_file;
                Buffer_2D<std::complex<data_type>> attack_signal_buffer;

            public:
                /**
                 * @brief Construct a new Attacking Subsystem object
                 * 
                 * @param config_data JSON configuration object for the attacker
                 * @param usrp_handler pointer to a USRP handler for the attacking subsystem to use
                 */
                AttackingSubsystem(json config_data, USRPHandler<data_type> * usrp_handler):
                    config(config_data),
                    attacker_usrp_handler(usrp_handler)
                {
                    if (check_config())
                    {
                        initialize_attack_subsystem_parameters();
                        if (enabled)
                        {
                            init_attack_signal_buffer();
                            init_frame_start_times_buffer();
                        }
                        
                    }                    
                }

                ~AttackingSubsystem(){}

                /**
             * @brief Check the json config file to make sure all necessary parameters are included
             * 
             * @return true - JSON is all good and has required elements
             * @return false - JSON is missing certain fields
             */
            bool check_config(){
                bool config_good = true;

                //tx file name
                if(config["AttackSubsystemSettings"]["tx_file_name"].is_null()){
                    std::cerr << "AttackSubsystem::check_config: no tx_file_name in JSON" <<std::endl;
                    config_good = false;
                }

                //enable status
                if(config["AttackSubsystemSettings"]["enabled"].is_null()){
                    std::cerr << "AttackSubsystem::check_config: no enabled in JSON" <<std::endl;
                    config_good = false;
                }

                //number of attack frames
                if(config["AttackSubsystemSettings"]["num_attack_frames"].is_null()){
                    std::cerr << "AttackSubsystem::check_config: no num_attack_frames in JSON" <<std::endl;
                    config_good = false;
                }
                
                //attack start frame - the frame that the attack starts on
                if(config["AttackSubsystemSettings"]["attack_start_frame"].is_null()){
                    std::cerr << "AttackSubsystem::check_config: no attack_start_frame in JSON" <<std::endl;
                    config_good = false;
                }

                //estimated frame periodicity
                if(config["AttackSubsystemSettings"]["estimated_frame_periodicity_ms"].is_null()){
                    std::cerr << "AttackSubsystem::check_config: no estimated_frame_periodicity_ms in JSON" <<std::endl;
                    config_good = false;
                }
                
                //samples per buffer
                if(config["USRPSettings"]["TX"]["spb"].is_null()){
                    std::cerr << "AttackSubsystem::check_config: no samples per buffer for Tx in JSON" <<std::endl;
                    config_good = false;
                }

                //stream start offset
                if(config["USRPSettings"]["RX"]["offset_us"].is_null()){
                    std::cerr << "AttackSubsystem::check_config: no samples per buffer for Tx in JSON" <<std::endl;
                    config_good = false;
                }

                return config_good;
            }

            /**
             * @brief Initialize attack subsystem parameters
             * 
             */
            void initialize_attack_subsystem_parameters(){
                
                //attack signal file path
                attack_signal_file = config["AttackSubsystemSettings"]["tx_file_name"].get<std::string>();
                
                //enabled status
                enabled = config["AttackSubsystemSettings"]["enabled"].get<bool>();

                //FMCW arguments
                num_attack_frames = config["AttackSubsystemSettings"]["num_attack_frames"].get<size_t>();
                attack_start_frame = config["AttackSubsystemSettings"]["attack_start_frame"].get<size_t>();
                frame_periodicity_ms = config["AttackSubsystemSettings"]["estimated_frame_periodicity_ms"].get<double>();
                stream_start_offset_us = config["USRPSettings"]["RX"]["offset_us"].get<double>();

                //attack signal buffer
                samples_per_buffer = config["USRPSettings"]["TX"]["spb"].get<double>();

                //set attacking flags
                attacking = false;
                sensing_complete = false;
            }

            /**
             * @brief Initialize the buffer that tracks the frame start times
             * 
             */
            void init_frame_start_times_buffer(void){
                frame_start_times = std::vector<uhd::time_spec_t>(num_attack_frames);
                current_attack_frame = 0;
                next_frame_to_load = 0;
                new_frame_start_time_available = false;
            }


            /**
             * @brief load the pre-computed attack signal into the attack signal buffer
             * 
             */
            void init_attack_signal_buffer(){

                //set the buffer path
                attack_signal_buffer.set_read_file(attack_signal_file,true);

                //load the samples into the buffer
                attack_signal_buffer.import_from_file(samples_per_buffer);
            }

            /**
             * @brief Load a new frame start time into the frame_start_time_buffer
             * 
             * @param desired_attack_start_time_ms the desired attack start time in ms
             */
            void load_new_frame_start_time(double desired_attack_start_time_ms){

                std::unique_lock<std::mutex> frame_start_times_lock(frame_start_times_mutex, std::defer_lock);


                if (next_frame_to_load < num_attack_frames)
                {

                    //set stream start time
                    double attack_start_time_s = (desired_attack_start_time_ms - (stream_start_offset_us * 1e-3)) * 1e-3;
                    
                    //check to make sure that the attack_start_time is valid
                    double current_time = attacker_usrp_handler -> usrp -> get_time_now().get_real_secs();
                    
                    if((attack_start_time_s - current_time) <= 1e-3)
                    {
                        std::cerr << "AttackingSubsystem::load_new_frame_start_time: frame start time occurs before current USRP time" <<std::endl;
                    }
                    else
                    {
                        if(next_frame_to_load == 0)
                        {
                            attacking = true;
                        }

                        frame_start_times_lock.lock();
                        frame_start_times[next_frame_to_load] = uhd::time_spec_t(attack_start_time_s);
                        next_frame_to_load += 1;
                        new_frame_start_time_available = true;
                        frame_start_times_lock.unlock();
                    }
                }
            }

            void run(){
                while (current_attack_frame < num_attack_frames && not get_sensing_complete()) //make sure that the sensing subsystem is still active
                {
                    std::unique_lock<std::mutex> frame_start_times_lock(frame_start_times_mutex, std::defer_lock);
                    frame_start_times_lock.lock();
                    if (new_frame_start_time_available)
                    {
                        std::vector<uhd::time_spec_t> frame_start_time(1);
                        frame_start_time[0] = frame_start_times[current_attack_frame];
                        current_attack_frame += 1;
                        if (current_attack_frame == next_frame_to_load)
                        {
                            new_frame_start_time_available = false;
                        }
                        frame_start_times_lock.unlock();

                        //transmit the attack
                        attacker_usrp_handler -> stream_frames_tx_only(frame_start_time, & attack_signal_buffer);
                        
                    }
                    else{
                        frame_start_times_lock.unlock();
                        //std::this_thread::sleep_for(std::chrono::milliseconds(int64_t(1)));
                    }
                    
                }
            }

            /**
             * @brief Set the sensing complete object's value
             * 
             * @param new_value - set to true if the sensing subsystem is no longer performing sensing
             */
            void set_sensing_complete(bool new_value){
                std::unique_lock<std::mutex> sensing_complete_lock(sensing_complete_mutex, std::defer_lock);
                
                sensing_complete_lock.lock();
                sensing_complete = new_value;
                sensing_complete_lock.unlock();
            }

            /**
             * @brief Get the attack complete object's value
             * 
             * @return true - sensing subsystem is no longer performing sensing
             * @return false - sensing subsystem is still sensing
             */
            bool get_sensing_complete(){
                std::unique_lock<std::mutex> sensing_complete_lock(sensing_complete_mutex, std::defer_lock);
                
                sensing_complete_lock.lock();
                bool sensing_complete_status = sensing_complete;
                sensing_complete_lock.unlock();

                return sensing_complete_status;
            }
            
            /**
             * @brief Computes the frame start times in advance, and applies an offset if one is necessary
             * 
             */
            void compute_frame_start_times(double desired_attack_start_time_ms){
                
                //get the offset between the Tx and Rx streams
                
                //set stream start time
                attack_start_time_ms = desired_attack_start_time_ms - (stream_start_offset_us * 1e-3);

                //initialize the frame start times vector
                frame_start_times = std::vector<uhd::time_spec_t>(num_attack_frames);
                //std::cout << "AttackSubsystem::compute_frame_start_times: computed start times: " << std::endl;
                for (size_t i = 0; i < num_attack_frames; i++)
                {
                    frame_start_times[i] = uhd::time_spec_t((attack_start_time_ms * 1e-3) + 
                                    ((frame_periodicity_ms * 1e-3) * static_cast<double>(i)));
                    //std::cout << frame_start_times[i].get_real_secs() << ", ";
                }
                //std::cout << std::endl;
            }


            /**
             * @brief run the attack subsystem (assumes frame start times already computed and attack
             * signal buffer already initialized)
             * 
             */
            void run_attack_subsystem(){
                attacker_usrp_handler -> stream_frames_tx_only(frame_start_times, & attack_signal_buffer);
            }

            /**
             * @brief Resets the attacking subsystem (useful if performing multiple runs)
             * 
             */
            void reset(){
                initialize_attack_subsystem_parameters();
                if (enabled)
                {
                    init_attack_signal_buffer();
                }
            }

            
        };
    }
#endif