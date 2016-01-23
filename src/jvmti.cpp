#include "jvmti.hpp"
#include "jni.hpp"

#include <sstream>

using namespace std;
using namespace boost;

/* Get a name for a jmethodID */
string get_method_name(jvmtiEnv &jvmti, jmethodID method) {
    jvmtiError error;

    jclass declaringType;
    error = jvmti.GetMethodDeclaringClass(method, &declaringType);
    check_jvmti_error(jvmti, error, "Unable to get method information.");

    char *type;
    error = jvmti.GetClassSignature(declaringType, &type, NULL);
    check_jvmti_error(jvmti, error, "Unable to get class signature.");

    char *name;
    char *sig;
    char *gsig;
    error = jvmti.GetMethodName(method, &name, &sig, &gsig);
    check_jvmti_error(jvmti, error, "Unable to get method information.");

    std::stringstream stream;
    stream << type << "#" << name << ((gsig == NULL) ? sig : gsig);

    deallocate(jvmti, gsig);
    deallocate(jvmti, sig);
    deallocate(jvmti, name);
    deallocate(jvmti, type);

    return stream.str();
}

/* Get a name for a jthread */
string get_thread_name(jvmtiEnv &jvmti, JNIEnv &jni, jthread thread) {
    jvmtiThreadInfo info = {0};
    jvmtiError error;

    /* Get the thread information, which includes the name */
    error = jvmti.GetThreadInfo(thread, &info);
    check_jvmti_error(jvmti, error, "Cannot get thread info");

    std::stringstream stream;

    /* The thread might not have a name, be careful here. */
    if (info.name != NULL) {
        stream << info.name;

        /* Every string allocated by JVMTI needs to be freed */
        deallocate(jvmti, (void *) info.name);
    } else {
        stream << "Unknown";
    }

    /* Cleanup JNI references */
    jni.DeleteLocalRef(info.thread_group);
    jni.DeleteLocalRef(info.context_class_loader);

    return stream.str();
}

string get_error_name(jvmtiEnv &jvmti, jvmtiError error, const string message) {
    char *error_name = NULL;
    jvmtiError error_ = jvmti.GetErrorName(error, &error_name);
    ASSERT_MSG(error_ == JVMTI_ERROR_NONE, "JVMTI ERROR while getting an error name");

    const string messageSeparator = message.empty() ? "" : " ";
    auto name = format("%s%sJVMTI ERROR: %d(%s)\n") % message % messageSeparator % error %
                (error_name == NULL ? "Unknown" : error_name);
    deallocate(jvmti, error_name);
    return name.str();
}

/* Every JVMTI interface returns an error code, which should be checked
 *   to avoid any cascading errors down the line.
 *   The interface GetErrorName() returns the actual enumeration constant
 *   name, making the error messages much easier to understand.
 */
void check_jvmti_error(jvmtiEnv &jvmti, jvmtiError error, const string message) {
    ASSERT_MSG(error == JVMTI_ERROR_NONE, get_error_name(jvmti, error, message).c_str());
}

/* All memory allocated by JVMTI must be freed by the JVMTI Deallocate
 *   interface.
 */
void deallocate(jvmtiEnv &jvmti, void *ptr) {
    jvmtiError error;

    error = jvmti.Deallocate((unsigned char *) ptr);
    check_jvmti_error(jvmti, error, "Cannot deallocate memory");
}

/* Allocation of JVMTI managed memory */
void *allocate(jvmtiEnv &jvmti, jint len) {
    void *ptr;

    jvmtiError error = jvmti.Allocate(len, (unsigned char **) &ptr);
    check_jvmti_error(jvmti, error, "Cannot allocate memory");
    return ptr;
}
