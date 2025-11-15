cmake_minimum_required(VERSION 3.16)

project(nekoray_fixed_test VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Try to find Qt6 components
find_package(Qt6 QUIET COMPONENTS Core)

if(Qt6_FOUND)
    message(STATUS "Qt6 found, building Qt version")
    find_package(Qt6 REQUIRED COMPONENTS Core Network)
    
    # Create a test with fixed Qt implementation
    add_executable(nekoray_test_fixed_qt
        nekoray/core/NekoService_Fixed.cpp
        nekoray/core/CoreManager_Fixed.cpp  
        nekoray/core/TunManager_Fixed.cpp
        nekoray/core/ConfigManager.cpp
        test_qt_simple.cpp
    )
    
    target_link_libraries(nekoray_test_fixed_qt Qt6::Core Qt6::Network)
    target_include_directories(nekoray_test_fixed_qt PRIVATE nekoray)
    
    set_target_properties(nekoray_test_fixed_qt PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
        AUTOMOC ON
    )
    
    message(STATUS "Qt version will be built as: nekoray_test_fixed_qt")
else()
    message(STATUS "Qt6 not found, creating non-Qt test")
endif()

# Always create the basic C++ version
add_executable(nekoray_test_basic
    test_basic.cpp
)

set_target_properties(nekoray_test_basic PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)