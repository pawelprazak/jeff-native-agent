cmake_minimum_required(VERSION 2.8)

project(jeff-native-agent)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

if(WIN32)
    include_directories(
        "C:\Program Files\Java\jdk1.8.0_51\include"
        "C:\Program Files\Java\jdk1.8.0_51\include\win32"
        "C:\Program Files\Java\jdk1.8.0\include"
        "C:\Program Files\Java\jdk1.8.0\include\win32"
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