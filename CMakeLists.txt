#-----------------------------------------------------------------------------
#  CMake Config
#-----------------------------------------------------------------------------

cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

project(jeff-native-agent)

if(NOT MSVC)
    if(NOT USE_CPP_VERSION)
        set(USE_CPP_VERSION c++11)
    endif()
    message(STATUS "Use C++ version: ${USE_CPP_VERSION}")
    # following only available from cmake 2.8.12:
    #   add_compile_options(-std=${USE_CPP_VERSION})
    # so using this instead:
    add_definitions(-std=c++11)
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

if(WIN32)
    include_directories(
        "C:/Program Files/Java/jdk1.8.0_51/include"
        "C:/Program Files/Java/jdk1.8.0_51/include/win32"
        "C:/Program Files/Java/jdk1.8.0/include"
        "C:/Program Files/Java/jdk1.8.0/include/win32"
    )
else()
    include_directories(
        "/usr/lib/jvm/java-8-oracle/include"
        "/usr/lib/jvm/java-8-oracle/include/linux"
        "/usr/lib/jvm/java-7-openjdk-amd64/include"
        "/usr/lib/jvm/java-7-openjdk-amd64/include/linux"
    )
endif()

set(SOURCE_FILES main.cpp)
add_library(jeff-native-agent SHARED ${SOURCE_FILES})