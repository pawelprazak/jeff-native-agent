#!/usr/bin/env bash

LOG_PATH=jeff.log
VALGRIND=NO
MAVEN=YES

usage () {
    echo "Usage:"
    echo "  ${0} [-l|-log=...] [--valgrind] [--skip-maven]"
    echo
    exit 1;
}

# https://stackoverflow.com/questions/192249/how-do-i-parse-command-line-arguments-in-bash

for i in "$@"
do
case $i in
    -l=*|--log=*)
    LOG_PATH="${i#*=}"
    shift # past argument=value
    ;;
    --valgrind)
    VALGRIND=YES
    shift # past argument with no value
    ;;
    --skip-maven)
    MAVEN=NO
    shift # past argument with no value
    ;;
    -h|--help)
    usage;
    ;;
    *)
    echo "unknown option $i"
    echo
    usage;
    ;;
esac
done

if [ ${MAVEN} == YES ]; then
    echo "Building jeff-app project..."
    (cd jeff-app && mvn clean install package >> /dev/null)
fi


echo "Running jeff-app.jar with jeff agent"

JEFF_PATH="build/libjeff-native-agent.so"

if [ ! -f ${JEFF_PATH}  ]; then
    echo "File ${JEFF_PATH} not found"
    exit 1
fi

JAR_PATH="jeff-app/target/jeff-app-1.0-SNAPSHOT.jar"

if [ ! -f ${JAR_PATH}  ]; then
    echo "File ${JAR_PATH} not found"
    exit 1
fi

if [ ${VALGRIND} == YES ]; then
    VALGRIND_COMMAND="valgrind -v --leak-check=yes --suppressions=valgrind.supp"
fi

# Run java with options:
# - neutralize penalty imposed by 64 bit JVM (HotSpot)
# - optimize the JVM for short-runnning applications
# - JNI debugging
# - JVM TI native agent
# - the program to run
${VALGRIND_COMMAND} java -showversion \
    -XX:+UseCompressedOops \
    -XX:+TieredCompilation -XX:TieredStopAtLevel=1 \
    -verbose:jni \
    -agentpath:${JEFF_PATH} \
    -jar ${JAR_PATH} > ${LOG_PATH}

echo
echo "Output logged into: ${LOG_PATH}"