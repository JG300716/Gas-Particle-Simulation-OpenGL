cmake_minimum_required(VERSION 3.21)
project(PWAG LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(PWAG
    src/main.cpp
)

# Wymuś konsolę na Windows (DEBUG!)
if (WIN32)
    set_target_properties(PWAG PROPERTIES
        LINK_FLAGS "/SUBSYSTEM:CONSOLE"
    )
endif()