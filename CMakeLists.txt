#-----------------------------------------------------------------------------
#  CMake Config
#
#  https://cmake.org/cmake/help/v2.8.7/cmake.html
#-----------------------------------------------------------------------------

cmake_minimum_required(VERSION 2.8.7 FATAL_ERROR)

project(jeff-native-agent)

set(LIB_MAJOR_VERSION "0")
set(LIB_MINOR_VERSION "1")
set(LIB_PATCH_VERSION "1")
set(LIB_VERSION_STRING "${LIB_MAJOR_VERSION}.${LIB_MINOR_VERSION}.${LIB_PATCH_VERSION}")

#  Compiler and Linker flags

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

if(NOT MSVC)
    if(NOT USE_CPP_VERSION)
        set(USE_CPP_VERSION c++11)
    endif()
    message(STATUS "Use C++ version: ${USE_CPP_VERSION}")
    # following only available from cmake 2.8.12:
    #   add_compile_options(-std=${USE_CPP_VERSION})
    # so using this instead:
    add_definitions(-std=${USE_CPP_VERSION})
endif()

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

# Build

set(SOURCE_FILES 
    src/main.hpp
    src/main.cpp
)
add_library(jeff-native-agent SHARED ${SOURCE_FILES})

# Packaging

set(CPACK_PACKAGE_VERSION_MAJOR ${LIBOSMIUM_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${LIBOSMIUM_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${LIBOSMIUM_VERSION_PATCH})

set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "JVM TI native agent")
set(CPACK_PACKAGE_VENDOR "bluecatcode.com")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_NAME ${CMAKE_PROJECT_NAME})

if(WIN32)
    set(CPACK_GENERATOR ZIP)
else()
    set(CPACK_GENERATOR TGZ)
endif()

include(CPack)
