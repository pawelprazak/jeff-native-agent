#ifndef JEFF_NATIVE_AGENT_JNI_HPP
#define JEFF_NATIVE_AGENT_JNI_HPP

#include <jni.h>
#include <string>
#include <functional>
#include <boost/optional.hpp>

#define THROW_JAVA_EXCEPTION(msg, exception_type) \
    jeff::__throw_exception(exception_type, msg, \
        BOOST_CURRENT_FUNCTION, __FILE__, __LINE__);

#define ASSERT_MSG(expr, msg) ((expr) \
    ? ((void)0) \
    : jeff::__throw_exception(#expr, msg, AssertionError, \
        BOOST_CURRENT_FUNCTION, __FILE__, __LINE__))

namespace jeff {

    constexpr const char *const RuntimeException = "java/lang/RuntimeException";
    constexpr const char *const AssertionError = "java/lang/AssertionError";

    jclass find_class(JNIEnv &jni, std::string name);

    jclass get_object_class(JNIEnv &jni, jobject object);

    jmethodID get_method_id(JNIEnv &jni, jclass type, const std::string methodName,
                            const std::string returnSignature);
//    template<typename R>
    std::wstring call_method(JNIEnv &jni, jobject &object, const std::string methodName,
                  const std::string methodSignature, std::function<std::wstring(jobject)> transformer, ...);

    jobject call_method(JNIEnv &jni, jobject &object, const std::string methodName,
                              const std::string methodSignature, ...);

    jobject call_method(JNIEnv &jni, jobject &object, jmethodID methodID, ...);

    std::wstring to_wstring(JNIEnv &jni, jstring str);

//    std::function<std::wstring(jobject)> wstring_transformer(JNIEnv &jni);

    void delete_local_ref(JNIEnv &jni, jclass type);

    void __throw_exception(const char *message, const char *exceptionType,
                           const char *function, const char *file, int line);

    void __throw_exception(const char *message, const char *expression, const char *exceptionType,
                           const char *function, const char *file, int line);

}

#endif //JEFF_NATIVE_AGENT_JNI_HPP
