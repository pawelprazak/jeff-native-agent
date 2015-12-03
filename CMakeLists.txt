cmake_minimum_required(VERSION 3.3)
project(jeff-native-agent)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

include_directories(
        "/usr/lib/jvm/java-8-oracle/include/"
        "/usr/lib/jvm/java-8-oracle/include/linux"
)

set(SOURCE_FILES main.cpp)
add_library(jeff-native-agent SHARED ${SOURCE_FILES})