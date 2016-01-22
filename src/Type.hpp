#ifndef JEFF_NATIVE_AGENT_TYPE_HPP
#define JEFF_NATIVE_AGENT_TYPE_HPP

#include <jni.h>
#include <jvmti.h>
#include "Object.hpp"

class Type : Object {

public:
    Type(jvmtiEnv &jvmti, JNIEnv &jni, jclass type);

    virtual ~Type();

private:
    jvmtiEnv &jvmti;
    JNIEnv &jni;
    jclass type;

public:
    const std::string getSignature() const;

};

#endif //JEFF_NATIVE_AGENT_TYPE_HPP
