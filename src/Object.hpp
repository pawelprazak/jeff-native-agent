#ifndef JEFF_NATIVE_AGENT_OBJECT_HPP
#define JEFF_NATIVE_AGENT_OBJECT_HPP

#include <jni.h>
#include <jvmti.h>
#include "boost.hpp"

class Type;

class Object {

public:
    Object(jvmtiEnv &jvmti, JNIEnv &jni, jobject object);

    virtual ~Object();

private:
    jvmtiEnv &jvmti;
    JNIEnv &jni;
    jobject object;
    jclass type;

public:
    const Type getType() const;

};


#endif //JEFF_NATIVE_AGENT_OBJECT_HPP
