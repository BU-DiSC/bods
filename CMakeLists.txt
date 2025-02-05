# =============================================================================
# Benchmark on data sortedness
# =============================================================================
cmake_minimum_required(VERSION 3.16)
project(bods VERSION 1.0
    DESCRIPTION "Benchmark on data sortedness"
    LANGUAGES CXX
)

message(STATUS "CXX : ${CMAKE_CXX_COMPILER}")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})
add_subdirectory(external)

# =============================================================================
# HEADER Options
# =============================================================================
option(DEBUG "Debug mode on or off" OFF)
if(${DEBUG})
    set(CMAKE_BUILD_TYPE "Debug")
else()
    set(CMAKE_BUILD_TYPE "Release")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# =============================================================================
# HEADER Dependencies
# =============================================================================
find_package(Boost REQUIRED)

# =============================================================================
# HEADER bods
# =============================================================================
set(BODS_SRCS
    src/sortedness_data_generator.cpp
)

add_executable(sortedness_data_generator ${BODS_SRCS})

target_include_directories(sortedness_data_generator PUBLIC
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/external
)

target_link_libraries(sortedness_data_generator PUBLIC
    Boost::boost
    spdlog::spdlog
    cxxopts
)
