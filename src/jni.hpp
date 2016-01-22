#ifndef JEFF_NATIVE_AGENT_JNI_HPP
#define JEFF_NATIVE_AGENT_JNI_HPP

#include <jni.h>
#include <string>
#include <boost/optional.hpp>

jclass get_object_class(JNIEnv &jni, jobject object);

jmethodID get_method_id(JNIEnv &jni, const jclass type, const std::string methodName, const std::string returnSignature);

jobject call_method(JNIEnv &jni, const jclass type, jmethodID methodID, ...);

boost::optional<jobject> call_method(JNIEnv &jni, const jobject object, const std::string methodName, const std::string returnSignature, ...);

std::string to_string(JNIEnv &jni, jstring str);

void delete_local_ref(JNIEnv &jni, const jclass type);

#endif //JEFF_NATIVE_AGENT_JNI_HPP
