cmake_minimum_required(VERSION 3.0.0)
project(FMCWRadar VERSION 0.1.0)

include(CTest)
enable_testing()

#add libraries and subdirectories
add_subdirectory(src)
find_package(nlohmann_json 3.10.5 REQUIRED)

#addexecurable
add_executable(FMCWRadar main.cpp)

#perform linking
target_link_libraries(FMCWRadar PRIVATE JSONHandler)
target_link_libraries(FMCWRadar PRIVATE nlohmann_json::nlohmann_json)

#include directories
target_link_directories(FMCWRadar PRIVATE 
                        ${PROJECT_BINARY_DIR}
                        ${PROJECT_BINARY_DIR}/src)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
