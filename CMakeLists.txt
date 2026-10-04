cmake_minimum_required(VERSION 3.28)
project(SoftWallSimulator LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# SFML is downloaded and built from source, so CMake and a C++17 compiler are the only prerequisites.
set(SFML_BUILD_AUDIO OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_NETWORK OFF CACHE BOOL "" FORCE)
include(FetchContent)
FetchContent_Declare(SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG 3.0.2
    GIT_SHALLOW ON
    EXCLUDE_FROM_ALL
    SYSTEM)
FetchContent_MakeAvailable(SFML)

# Simulation: the grid and everything that moves on it. No rendering, so the tests run headless.
add_library(SoftWallSimulation STATIC
    src/simulation/Grid.cpp
    src/simulation/Level.cpp
    src/simulation/Player.cpp
    src/simulation/World.cpp)
target_include_directories(SoftWallSimulation PUBLIC src)
target_link_libraries(SoftWallSimulation PUBLIC SFML::System)

add_executable(SoftWallTests tests/SimulationTests.cpp)
target_link_libraries(SoftWallTests PRIVATE SoftWallSimulation)

foreach(soft_wall_target SoftWallSimulation SoftWallTests)
    if(MSVC)
        target_compile_options(${soft_wall_target} PRIVATE /W4 /permissive-)
    else()
        target_compile_options(${soft_wall_target} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    set_target_properties(${soft_wall_target} PROPERTIES COMPILE_WARNING_AS_ERROR ON)
endforeach()
