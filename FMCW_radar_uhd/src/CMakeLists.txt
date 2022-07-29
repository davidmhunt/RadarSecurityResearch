#add the subdirectory/dependencies
find_package(nlohmann_json 3.10.5 REQUIRED)

#declare the library
add_library(JSONHandler JSONHandler.cpp)

#link the library
target_link_libraries(JSONHandler PRIVATE nlohmann_json::nlohmann_json)