cmake_minimum_required(VERSION 3.26)
project(ping_detector)

set(CMAKE_CXX_STANDARD 17)


# Find SDL
find_package(SDL2 REQUIRED)

# popen()/pclose() are not C++ standards
set(CMAKE_CXX_EXTENSIONS ON)

add_executable("${PROJECT_NAME}" main.cpp
        g711.cpp)

set(PcapPlusPlus_DIR "~/CLionProjects/ping_detector/pcapplusplus-23.09-ubuntu-22.04-gcc-11.2.0-x86_64/lib/cmake/pcapplusplus")

message("Compiling Tutorial out of project")
find_package(PcapPlusPlus REQUIRED)
include_directories(${PcapPlusPlus_INCLUDE_DIRS})
# We want to have the binary compiled in the same folder as the .cpp to be near the pcap file
set_target_properties("${PROJECT_NAME}" PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries("${PROJECT_NAME}" PUBLIC PcapPlusPlus::Pcap++)


# Include SDL headers
target_include_directories("${PROJECT_NAME}" PRIVATE ${SDL2_INCLUDE_DIRS})

# Link with SDL library
target_link_libraries("${PROJECT_NAME}" PRIVATE ${SDL2_LIBRARIES})

target_link_libraries("${PROJECT_NAME}" PRIVATE SDL2_mixer)