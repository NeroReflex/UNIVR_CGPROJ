cmake_minimum_required(VERSION 3.10)
project(dds_loader LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Source files
set(SOURCES
    dds_loader.cpp
    dds_loader.hpp

    dds_header.hpp
    dds_header.cpp
)

# Create a library (static by default; change to SHARED if you want a shared lib)
add_library(dds_loader ${SOURCES})

# Include current directory so headers are found
target_include_directories(dds_loader PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Example: set position-independent code for shared/static linking when needed
set_target_properties(dds_loader PROPERTIES POSITION_INDEPENDENT_CODE ON)