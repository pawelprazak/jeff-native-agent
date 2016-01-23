#include "jni.hpp"
#include <boost/assert.hpp>
#include <boost/format.hpp>
#include "GlobalAgentData.hpp"

using namespace std;
using namespace boost;

/**
 * Remember to use jni.DeleteLocalRef
 */
jclass get_object_class(JNIEnv &jni, jobject object) {
    jclass type = jni.GetObjectClass(object);
    ASSERT_MSG(!jni.ExceptionCheck(), "Unable to get object class");
    return type;
}

jmethodID get_method_id(JNIEnv &jni, const jclass type, const string methodName, const string returnSignature) {
    jmethodID methodID = jni.GetMethodID(type, methodName.c_str(), returnSignature.c_str());
    ASSERT_MSG(!jni.ExceptionCheck(), "Unable to get method ID");
    return methodID;
}

jobject call_method(JNIEnv &jni, const jclass type, jmethodID methodID, ...) {
    va_list args;
    jobject result;
    va_start(args, methodID);
    result = jni.CallObjectMethodV(type, methodID, args);
    ASSERT_MSG(!jni.ExceptionCheck(), "Unable to call method");
    va_end(args);
    return result;
}

optional<jobject> call_method(JNIEnv &jni, const jobject object, const string methodName,
                              const string returnSignature, ...) {
    // Get the object's class
    jclass type = get_object_class(jni, object);

    va_list args;
    jobject result;
    va_start(args, returnSignature);
    jmethodID methodID = get_method_id(jni, type, methodName, returnSignature);
    result = call_method(jni, type, methodID, args);
    va_end(args);

    delete_local_ref(jni, type);

    if (result == NULL) {
        return boost::none;
    } else {
        return result;
    }
}

string to_string(JNIEnv &jni, jstring str) {
    // Convert to native char array
    const char *chars = jni.GetStringUTFChars(str, 0);

    // Convert to string
    auto ret = format("%s") % chars;

    // And finally, release the JNI objects after usage
    jni.ReleaseStringUTFChars(str, chars);
    ASSERT_MSG(!jni.ExceptionCheck(), "Unable to release string");

    return ret.str();
}

void delete_local_ref(JNIEnv &jni, const jclass type) {
    jni.DeleteLocalRef(type);
    ASSERT_MSG(!jni.ExceptionCheck(), "Unable to delete local reference");
}

/**
 * Use THROW_JAVA_EXCEPTION macro.
 *
 * JNI specification 6.1.1:
 * A pending exception raised through the JNI (by calling ThrowNew, for example) does not immediately disrupt
 * the native method execution. JNI programmers must explicitly implement the control flow after an exception has occurred.
 */
void __throw_exception(const char *message, const char *expression, const char *exceptionType,
                       const char *function, const char *file, int line) {
    BOOST_ASSERT_MSG(exceptionType != NULL, "Expected non-null exceptionType");

    auto exceptionMessage = format("JNIException (%s): '%s' %s\n\t%s (%s:%s)")
                            % exceptionType % message
                            % ((expression == NULL) ? "" : (format(" assertion (%s) failed in:") % expression).str())
                            % (function == NULL ? "unknown" : function)
                            % (file == NULL ? "unknown" : file)
                            % line;

    JNIEnv *jni;
    gdata->jvm->AttachCurrentThread((void **) &jni, NULL);  // Get the JNIEnv by attaching to the current thread.
    BOOST_ASSERT_MSG(jni != NULL, "Unable to attach to current thread to get JNIEnv");

    jclass type = jni->FindClass(exceptionType);
    /* if type is NULL, an exception has already been thrown by JVM */
    if (type != NULL) {
        jni->ThrowNew(type, exceptionMessage.str().c_str());
    }
    jni->DeleteLocalRef(type);
}


void __throw_exception(const char *message, const char *exceptionType,
                       const char *function, const char *file, int line) {
    __throw_exception(message, NULL, exceptionType, function, file, line);
}