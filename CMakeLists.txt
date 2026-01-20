cmake_minimum_required(VERSION 3.20)
project(com_meas LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(n88util CONFIG REQUIRED)
find_package(Boost CONFIG REQUIRED COMPONENTS filesystem system)
find_package(AimIO CONFIG REQUIRED)

add_executable(com_meas src/main.cpp)
target_link_libraries(com_meas PRIVATE AimIO::AimIO)

# Conda layout on Windows
target_include_directories(com_meas PRIVATE
    $ENV{CONDA_PREFIX}/include
    $ENV{CONDA_PREFIX}/Library/include
)
