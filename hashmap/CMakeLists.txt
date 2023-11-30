cmake_minimum_required(VERSION 3.22)
project(hashmap)

set(CMAKE_CXX_STANDARD 17)

add_executable(hashmap main.cpp
        Google_tests/tests.cpp
        HashMap_lib/Bucket.cpp
        HashMap_lib/Bucket.h
        HashMap_lib/HashMap.cpp
        HashMap_lib/HashMap.h
)


add_subdirectory(Google_tests)