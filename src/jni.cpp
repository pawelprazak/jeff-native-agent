#include "jni.hpp"
#include <boost/assert.hpp>
#include <boost/format.hpp>

using namespace std;
using namespace boost;

/**
 * Remember to use jni.DeleteLocalRef
 */
jclass get_object_class(JNIEnv &jni, jobject object) {
    jclass type = jni.GetObjectClass(object);
    if (jni.ExceptionCheck()) {
        jni.ExceptionOccurred();
    }
    BOOST_ASSERT_MSG(!jni.ExceptionCheck(), "Unable to get object class");
    return type;
}

jmethodID get_method_id(JNIEnv &jni, const jclass type, const string methodName, const string returnSignature) {
    jmethodID methodID = jni.GetMethodID(type, methodName.c_str(), returnSignature.c_str());
    BOOST_ASSERT_MSG(!jni.ExceptionCheck(), "Unable to get method ID");
    return methodID;
}

jobject call_method(JNIEnv &jni, const jclass type, jmethodID methodID, ...) {
    va_list args;
    jobject result;
    va_start(args, methodID);
    result = jni.CallObjectMethodV(type, methodID, args);
    BOOST_ASSERT_MSG(!jni.ExceptionCheck(), "Unable to call method");
    va_end(args);
    return result;
}

optional<jobject> call_method(JNIEnv &jni, const jobject object, const string methodName, const string returnSignature, ...) {
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

    return ret.str();
}

void delete_local_ref(JNIEnv &jni, const jclass type) {
    jni.DeleteLocalRef(type);
    BOOST_ASSERT_MSG(!jni.ExceptionCheck(), "Unable to delete local reference");
}
