#ifndef JEFF_NATIVE_AGENT_JVMTI_H
#define JEFF_NATIVE_AGENT_JVMTI_H

#include "jvmti.h"
#include <string>
#include <list>
#include <functional>
#include <memory>
#include <boost/assert.hpp>

#define ASSERT_JVMTI_MSG(error, msg) ((error == JVMTI_ERROR_NONE) \
    ? ((void)0) \
    : jeff::__throw_jvmti_exception(error, msg, AssertionError, \
        BOOST_CURRENT_FUNCTION, __FILE__, __LINE__))

class Object;

namespace jeff {

    std::string get_class_status(jvmtiEnv &jvmti, jclass type);

    std::string get_class_signature(jvmtiEnv &jvmti, jclass type);

    std::string get_method_name(jvmtiEnv &jvmti, jmethodID method);

    int get_method_arguments_size(jvmtiEnv &jvmti, jmethodID method);

    std::list<std::string> get_method_arguments(jvmtiEnv &jvmti, JNIEnv &jni, jthread thread, jmethodID method, int depth);

    std::unique_ptr<Object> get_local_value(jvmtiEnv &jvmti, JNIEnv &jni, jthread thread, int depth, int slot,
                                            std::string signature);

    std::list<std::string> get_method_local_variables(jvmtiEnv &jvmti, JNIEnv &jni, jthread thread,
                                                      jmethodID method, int limit, int depth);

    std::string get_thread_name(jvmtiEnv &jvmti, JNIEnv &jni, jthread thread);

    std::string get_location(jvmtiEnv &jvmti, jmethodID method, jlocation location);

    std::string get_bytecode_location(jvmtiEnv &jvmti, jmethodID method, jlocation location);

    int get_stack_frame_count(jvmtiEnv &jvmti, jthread thread);

    std::list<std::string> get_stack_trace(jvmtiEnv &jvmti, JNIEnv &jni, jthread thread);

    std::list<std::string> get_stack_trace(jvmtiEnv &jvmti, JNIEnv &jni, jthread thread, int depth);

    std::string get_error_name(jvmtiEnv &jvmti, jvmtiError error, const std::string message = "");

    void deallocate(jvmtiEnv &jvmti, void *ptr);

    void *allocate(jvmtiEnv &jvmti, jint len);

    void print_possible_capabilities(jvmtiEnv &jvmti);

    void check_jvmti_error(jvmtiEnv &jvmti, jvmtiError error, const std::string message = "");

    bool is_jvmti_error(jvmtiEnv &jvmti, jvmtiError error, const std::string message = "");

    void __throw_jvmti_exception(jvmtiError error, const char *message,
                                 const char *exceptionType, const char *function, const char *file, int line);
}
#endif // JEFF_NATIVE_AGENT_JVMTI_H
