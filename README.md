# jeff-native-agent

[![Build Status](https://travis-ci.org/pawelprazak/jeff-native-agent.svg?branch=master)](https://travis-ci.org/pawelprazak/jeff-native-agent)
[![Build status](https://ci.appveyor.com/api/projects/status/8sjmq6gs13c44lgp?svg=true)](https://ci.appveyor.com/project/pawelprazak/jeff-native-agent)

## Dependencies
Debian/Ubuntu:

    sudo apt-get install libboost-all-dev

Windows:

    choco install boost-msvc-12

## Building

Jeff uses cmake:

    mkdir build
    cd build
    cmake ..
    make

## Basic scripts

    ./build.sh && ./hello.sh && less jeff.log

## Dependecies

- JDK (mainly `jvmti.h`)

## JVM TI and JNI

[Oracle - JVM Tool Interface Version 1.2](https://docs.oracle.com/javase/8/docs/platform/jvmti/jvmti.html)

[Oracle - Java Native Interface Specification](https://docs.oracle.com/javase/8/docs/technotes/guides/jni/spec/jniTOC.html)

[Wikipedia - Java Native Interface](https://en.wikipedia.org/wiki/Java_Native_Interface)

[IBM - Best practices for using the Java Native Interface](http://www.ibm.com/developerworks/library/j-jni/)

[IBM - Java programming with JNI](http://www.ibm.com/developerworks/java/tutorials/j-jni/j-jni.html)

[IBM - The JNI and the Garbage Collector](http://www-01.ibm.com/support/knowledgecenter/SSYKE2_5.0.0/com.ibm.java.doc.diagnostics.50/diag/understanding/jni_gc.html)

[Android - JNI Tips](https://developer.android.com/training/articles/perf-jni.html)

[CERT - Coding Standard for Java - JNI](https://www.securecoding.cert.org/confluence/pages/viewpage.action?pageId=121930001)

[AMD - JVMTI Event Piggybacking For Precise Source Mapping](https://web.archive.org/web/20120607024712/http://developer.amd.com/documentation/articles/pages/JVMTIEventPiggybacking.aspx)

## C++

[Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html)

## License

Jeff is available under the Apache License Version 2.0. See LICENSE.

## Authors

Jeff was mainly written and is maintained by Paweł Prażak and Bartłomiej Antoniak
See the git commit log for details.
