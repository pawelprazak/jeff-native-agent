#ifndef JEFF_NATIVE_AGENT_JVMTI_H
#define JEFF_NATIVE_AGENT_JVMTI_H

#include "jvmti.h"
#include <string>
#include <list>

#include "boost.hpp"

std::string get_method_name(jvmtiEnv &jvmti, jmethodID method);

std::string get_thread_name(jvmtiEnv &jvmti, JNIEnv &jni, jthread thread);

std::string get_location(jvmtiEnv &jvmti, jmethodID method, jlocation location);

std::string get_bytecode_location(jvmtiEnv &jvmti, jmethodID method, jlocation location);

std::list<std::string> get_stack_trace(jvmtiEnv &jvmti, jthread thread, int depth = 5);

std::string get_error_name(jvmtiEnv &jvmti, jvmtiError error, const std::string message = "");

void check_jvmti_error(jvmtiEnv &jvmti, jvmtiError error, const std::string message = "");

void deallocate(jvmtiEnv &jvmti, void *ptr);

void *allocate(jvmtiEnv &jvmti, jint len);

void print_possible_capabilities(jvmtiEnv &jvmti);

#endif // JEFF_NATIVE_AGENT_JVMTI_H