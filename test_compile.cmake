cmake_minimum_required(VERSION 3.16)

project(nekoray_test VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Create a simple test executable without external dependencies
add_executable(nekoray_test_basic
    test_basic.cpp
)

set_target_properties(nekoray_test_basic PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)