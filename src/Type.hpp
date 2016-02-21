#ifndef JEFF_NATIVE_AGENT_TYPE_HPP
#define JEFF_NATIVE_AGENT_TYPE_HPP

#include <jni.h>
#include <jvmti.h>
#include "Object.hpp"

class Type : Object {

public:
    Type();

    Type(const std::string signature);

    virtual ~Type();

private:
    const std::string signature;

public:
    static const Type *const from(jvmtiEnv &jvmti, JNIEnv &jni, std::string signature);

    static const Type *const from(jvmtiEnv &jvmti, JNIEnv &jni, char primitive_signature);

    const std::string getSignature() const;
};

#endif //JEFF_NATIVE_AGENT_TYPE_HPP
