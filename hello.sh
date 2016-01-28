#!/usr/bin/env bash

echo "Building jeff-app project..."

(cd jeff-app && mvn clean install package >> /dev/null)

echo "Running jeff-app.jar with jeff libjeff-native-agent.so"

LOG_FILE=jeff.log

# Run java with options:
# - neutralize penalty imposed by 64 bit JVM (HotSpot)
# - optimize the JVM for short-runnning applications
# - JNI debugging
# - JVM TI native agent
# - the program to run
java -showversion \
    -XX:+UseCompressedOops \
    -XX:+TieredCompilation -XX:TieredStopAtLevel=1 \
    -verbose:jni \
    -agentpath:build/libjeff-native-agent.so \
    -jar jeff-app/target/jeff-app-1.0-SNAPSHOT.jar > ${LOG_FILE}

echo
echo "Output logged into: ${LOG_FILE}"