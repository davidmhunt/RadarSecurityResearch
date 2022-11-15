#enable threads
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

#declare libraries
add_library(SpectrogramHandler SpectrogramHandler.cpp)
add_library(EnergyDetector EnergyDetector.cpp)


#link libraries as needed
target_link_libraries(SpectrogramHandler    PRIVATE Threads::Threads
                                            PRIVATE BufferHandler
                                            PRIVATE nlohmann_json::nlohmann_json)
target_link_libraries(EnergyDetector        PRIVATE nlohmann_json::nlohmann_json)

#include directories
target_link_directories(SpectrogramHandler PRIVATE 
                        ${WORKING_DIRECTORY}/pocketfft
                        ${WORKING_DIRECTORY}/Eigen
                        ${PARENT_DIRECTORY}
                        )