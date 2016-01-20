#! /bin/bash

echo "Building jeff-app project..."

(cd jeff-app && mvn clean install package >> /dev/null)

echo "Running jeff-app.jar with jeff libjeff-native-agent.so"

java -agentpath:build/libjeff-native-agent.so -jar jeff-app/target/jeff-app-1.0-SNAPSHOT.jar