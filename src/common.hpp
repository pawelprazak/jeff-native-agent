#ifndef JEFF_COMMON_H
#define JEFF_COMMON_H

#include <string>
#include "jvmti.h"

std::string get_method_name(jvmtiEnv *jvmti, jmethodID method);

std::string get_thread_name(jvmtiEnv *jvmti, JNIEnv *env, jthread thread);

#endif // JEFF_COMMON_H