#include "jni.hpp"

#include <boost/assert.hpp>
#include <boost/format.hpp>

#include "GlobalAgentData.hpp"

using namespace std;
using namespace boost;

/**
 * Remember to use jni.DeleteLocalRef
 */
jclass jeff::find_class(JNIEnv &jni, string name) {
    jclass type = jni.FindClass(name.c_str());
    ASSERT_MSG(!jni.ExceptionCheck(), "Unable to find class");
    return type;
}

/**
 * Remember to use jni.DeleteLocalRef
 */
jclass jeff::get_object_class(JNIEnv &jni, jobject object) {
    jclass type = jni.GetObjectClass(object);
    ASSERT_MSG(!jni.ExceptionCheck(), "Unable to get object class");
    return type;
}

jmethodID jeff::get_method_id(JNIEnv &jni, jclass type, const string methodName, const string returnSignature) {
    jmethodID methodID = jni.GetMethodID(type, methodName.c_str(), returnSignature.c_str());
    ASSERT_MSG(!jni.ExceptionCheck(), "Unable to get method ID");
    ASSERT_MSG(methodID != 0, "Expected non-zero method ID");
    return methodID;
}

string jeff::call_method(JNIEnv &jni, jobject &object, const string methodName,
                          const string methodSignature, function<string(jobject)> transformer, ...) {
    jclass type = get_object_class(jni, object);
    jmethodID methodID = get_method_id(jni, type, methodName, methodSignature);
    delete_local_ref(jni, type);

    va_list args;
    jobject result;
    va_start(args, transformer);
    result = call_method(jni, object, methodID, args);
    va_end(args);

    return transformer(result);
}

jobject jeff::call_method(JNIEnv &jni, jobject &object, const string methodName,
                          const string methodSignature, ...) {
    jclass type = get_object_class(jni, object);
    jmethodID methodID = get_method_id(jni, type, methodName, methodSignature);
    delete_local_ref(jni, type);

    va_list args;
    jobject result;
    va_start(args, methodSignature);
    result = call_method(jni, object, methodID, args);
    va_end(args);

    return result;
}

jobject jeff::call_method(JNIEnv &jni, jobject &object, jmethodID methodID, ...) {
    va_list args;
    jobject result;
    va_start(args, methodID);
    result = jni.CallObjectMethodV(object, methodID, args);

    ASSERT_MSG(!jni.ExceptionCheck(), "Unable to call method");
    va_end(args);
    return result;
}

string jeff::to_string(JNIEnv &jni, jstring str) {
    // Convert to native char array
    const char *chars = jni.GetStringUTFChars(str, JNI_FALSE);
    ASSERT_MSG(!jni.ExceptionCheck(), "Unable to get string");
    ASSERT_MSG(chars != nullptr, "Expected non-null pointer");

    jsize len = jni.GetStringLength(str);
    ASSERT_MSG(!jni.ExceptionCheck(), "Unable to get string length");

    string ret;
    ret.assign(chars, chars + len);

    jni.ReleaseStringUTFChars(str, chars);
    ASSERT_MSG(!jni.ExceptionCheck(), "Unable to release string");
    return ret;
}

wstring jeff::to_wstring(JNIEnv &jni, jstring str) {
    // Convert to native char array
    const jchar *chars = jni.GetStringChars(str, JNI_FALSE);
    ASSERT_MSG(!jni.ExceptionCheck(), "Unable to get string");
    ASSERT_MSG(chars != nullptr, "Expected non-null pointer");

    jsize len = jni.GetStringLength(str);
    ASSERT_MSG(!jni.ExceptionCheck(), "Unable to get string length");

    wstring ret;
    ret.assign(chars, chars + len);

    jni.ReleaseStringChars(str, chars);
    ASSERT_MSG(!jni.ExceptionCheck(), "Unable to release string");
    return ret;
}

list<bool> jeff::to_list(JNIEnv &jni, jbooleanArray array) {
    jsize length = jni.GetArrayLength(array);
    ASSERT_MSG(!jni.ExceptionCheck(), "Unable to get array length");

    jboolean *values = jni.GetBooleanArrayElements(static_cast<jbooleanArray>(array), JNI_FALSE);
    ASSERT_MSG(!jni.ExceptionCheck(), "Unable to get array elements");

    list<bool> ret;
    for (jboolean *v = values; v < values + length; v++) {
        bool value = *v > 0;
        ret.push_back(value);
    }
    jni.ReleaseBooleanArrayElements(static_cast<jbooleanArray>(array), values, JNI_ABORT);
    ASSERT_MSG(!jni.ExceptionCheck(), "Unable to release array elements");

    return ret;
}

/*
std::function<std::wstring(jobject)> jeff::wstring_transformer(JNIEnv &jni) {
    return [jni](jobject result) mutable {
        return (result == nullptr) ? L"" : jeff::to_wstring(jni, static_cast<jstring>(result));
    };
}

std::function<wstring(jobject)> wstring_transformer = [jni](jobject result) mutable {
    return (result == nullptr) ? L"" : jeff::to_wstring(*jni, static_cast<jstring>(result));
};
*/

void jeff::delete_local_ref(JNIEnv &jni, jclass type) {
    jni.DeleteLocalRef(type);
    ASSERT_MSG(!jni.ExceptionCheck(), "Unable to delete local reference");
}

void jeff::throw_by_name(JNIEnv &jni, const string exceptionType, const string exceptionMessage) {
    jclass type = find_class(jni, exceptionType);
    /* if type is NULL, an exception has already been thrown by JVM */
    if (type != NULL) {
        jni.ThrowNew(type, exceptionMessage.c_str());
    }
    jni.DeleteLocalRef(type);
}

/**
 * Use ASSERT_MSG macro.
 *
 * JNI specification 6.1.1:
 * A pending exception raised through the JNI (by calling ThrowNew, for example) does not immediately disrupt
 * the native method execution. JNI programmers must explicitly implement the control flow after an exception has occurred.
 */
void jeff::__throw_exception(const char *message, const char *expression, const char *exceptionType,
                             const char *function, const char *file, int line) {
    BOOST_ASSERT_MSG(exceptionType != NULL, "Expected non-null exceptionType");

    auto exceptionMessage = format("JNIException (%s): '%s' %s\n\t%s (%s:%s)")
                            % exceptionType % message
                            % ((expression == NULL) ? "" : (format(" assertion (%s) failed in:") % expression).str())
                            % (function == NULL ? "unknown" : function)
                            % (file == NULL ? "unknown" : file)
                            % line;

    std::cerr << exceptionMessage << std::endl << std::endl;

    JNIEnv *jni = get_current_jni();
    throw_by_name(*jni, exceptionType, exceptionMessage.str());
}

JNIEnv *jeff::get_current_jni() {
    JNIEnv *jni;
    gdata.jvm->AttachCurrentThread((void **) &jni, nullptr);  // Get the JNIEnv by attaching to the current thread.
    BOOST_ASSERT_MSG(jni != nullptr, "Unable to attach to current thread to get JNIEnv");
    return jni;
}

/**
 * Use THROW_JAVA_EXCEPTION macro.
 */
void jeff::__throw_exception(const char *message, const char *exceptionType,
                             const char *function, const char *file, int line) {
    __throw_exception(message, NULL, exceptionType, function, file, line);
}