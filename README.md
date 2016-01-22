# jeff-native-agent

[![Build Status](https://travis-ci.org/pawelprazak/jeff-native-agent.svg?branch=master)](https://travis-ci.org/pawelprazak/jeff-native-agent)
[![Build status](https://ci.appveyor.com/api/projects/status/8sjmq6gs13c44lgp?svg=true)](https://ci.appveyor.com/project/pawelprazak/jeff-native-agent)

## Dependencies

    sudo apt-get install libboost-all-dev

## Building

Jeff uses cmake:

    mkdir build
    cd build
    cmake ..
    make

## Basic scripts

    ./build.sh && ./hello.sh > agent.log

## Dependecies

- JDK (mainly `jvmti.h`)

## JVM TI and JNI

[JVM Tool Interface Version 1.2](https://docs.oracle.com/javase/8/docs/platform/jvmti/jvmti.html)

[Java Native Interface Specification](https://docs.oracle.com/javase/8/docs/technotes/guides/jni/spec/jniTOC.html)

## License

Jeff is available under the Apache License Version 2.0. See LICENSE.txt.

## Authors

Jeff was mainly written and is maintained by Paweł Prażak and Bartłomiej Antoniak
See the git commit log for details.