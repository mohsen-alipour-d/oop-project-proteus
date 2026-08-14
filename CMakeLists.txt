cmake_minimum_required(VERSION 3.26)
project(oop_project_proteus)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(oop_project_proteus
        src/main.cpp
        src/core/pin.cpp
        src/core/component.cpp
        src/components/sources/ground.cpp
        src/components/sources/dc_source.cpp
        src/components/sources/battery.cpp
        src/components/sources/clock_gen.cpp
)