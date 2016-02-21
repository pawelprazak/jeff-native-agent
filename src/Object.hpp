#ifndef JEFF_NATIVE_AGENT_OBJECT_HPP
#define JEFF_NATIVE_AGENT_OBJECT_HPP

#include <jni.h>
#include <jvmti.h>

#include <array>
#include <memory>

#include <boost/noncopyable.hpp>

class Type;

class Object {

public:
    Object();

    Object(const Type *const type, const std::string to_string);

    virtual ~Object();

private:
    const Type *const type;

    const std::string to_string;

public:
    const Type getType() const;

    const std::string toString() const;

    static std::unique_ptr<Object> from(jvmtiEnv &jvmti, JNIEnv &jni, jbooleanArray array, std::string signature);

    static std::unique_ptr<Object> from(jvmtiEnv &jvmti, JNIEnv &jni, jarray array, std::string signature);

    static std::unique_ptr<Object> from(jvmtiEnv &jvmti, JNIEnv &jni, jobject object);

    static std::unique_ptr<Object> from(jvmtiEnv &jvmti, JNIEnv &jni, bool value);

    static std::unique_ptr<Object> from(jvmtiEnv &jvmti, JNIEnv &jni, jchar value);

    static std::unique_ptr<Object> from(jvmtiEnv &jvmti, JNIEnv &jni, jbyte value);

    static std::unique_ptr<Object> from(jvmtiEnv &jvmti, JNIEnv &jni, jshort value);

    static std::unique_ptr<Object> from(jvmtiEnv &jvmti, JNIEnv &jni, jint value);

    static std::unique_ptr<Object> from(jvmtiEnv &jvmti, JNIEnv &jni, jlong value);

    static std::unique_ptr<Object> from(jvmtiEnv &jvmti, JNIEnv &jni, jfloat value);

    static std::unique_ptr<Object> from(jvmtiEnv &jvmti, JNIEnv &jni, jdouble value);
};

#endif //JEFF_NATIVE_AGENT_OBJECT_HPP
