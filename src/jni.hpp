#ifndef JEFF_NATIVE_AGENT_JNI_HPP
#define JEFF_NATIVE_AGENT_JNI_HPP

#include <jni.h>
#include <string>
#include <boost/optional.hpp>

#define THROW_JAVA_EXCEPTION (msg, exception_type) \
    __throw_exception(exception_type, msg, \
        BOOST_CURRENT_FUNCTION, __FILE__, __LINE__);

#define ASSERT_MSG(expr, msg) ((expr) \
    ? ((void)0) \
    : __throw_exception(#expr, msg, AssertionError, \
        BOOST_CURRENT_FUNCTION, __FILE__, __LINE__))

constexpr const char *const RuntimeException = "java/lang/RuntimeException";
constexpr const char *const AssertionError = "java/lang/AssertionError";

jclass get_object_class(JNIEnv &jni, jobject object);

jmethodID get_method_id(JNIEnv &jni, const jclass type, const std::string methodName,
                        const std::string returnSignature);

jobject call_method(JNIEnv &jni, const jclass type, jmethodID methodID, ...);

boost::optional<jobject> call_method(JNIEnv &jni, const jobject object, const std::string methodName,
                                     const std::string returnSignature, ...);

std::string to_string(JNIEnv &jni, jstring str);

void delete_local_ref(JNIEnv &jni, const jclass type);

void __throw_exception(const char *message, const char *exceptionType,
                       const char *function, const char *file, int line);

void __throw_exception(const char *message, const char *expression, const char *exceptionType,
                       const char *function, const char *file, int line);


#endif //JEFF_NATIVE_AGENT_JNI_HPP
