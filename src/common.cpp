#include "common.hpp"
#include "message.hpp"
#include "jvmti.hpp"

#include <sstream>

using namespace std;

/* Get a name for a jmethodID */
string get_method_name(jvmtiEnv *jvmti, jmethodID method) {
    jvmtiError error;

    jclass declaringType;
    error = jvmti->GetMethodDeclaringClass(method, &declaringType);
    check_jvmti_error(jvmti, error, "Unable to get method information.");

    char *type;
    error = jvmti->GetClassSignature(declaringType, &type, NULL);
    check_jvmti_error(jvmti, error, "Unable to get class signature.");

    char *name;
    char *sig;
    char *gsig;
    error = jvmti->GetMethodName(method, &name, &sig, &gsig);
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
string get_thread_name(jvmtiEnv *jvmti, JNIEnv *jni, jthread thread) {
    jvmtiThreadInfo info = {0};
    jvmtiError error;

    /* Make sure the stack variable is garbage free */
    // (void) memset(&info, 0, sizeof(info));

    /* Get the thread information, which includes the name */
    error = jvmti->GetThreadInfo(thread, &info);
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
    jni->DeleteLocalRef(info.thread_group);
    jni->DeleteLocalRef(info.context_class_loader);
    
    return stream.str();
}