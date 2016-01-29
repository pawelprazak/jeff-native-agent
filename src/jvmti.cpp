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

    /* Get the thread information, which includes the name */
    auto error = jvmti.GetThreadInfo(thread, &info);
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

string get_location(jvmtiEnv &jvmti, jmethodID method, jlocation location) {
    jvmtiJlocationFormat format;

    auto error = jvmti.GetJLocationFormat(&format);
    check_jvmti_error(jvmti, error, "Cannot get location format");

    string ret;
    switch (format) {
        case JVMTI_JLOCATION_JVMBCI: ret = get_bytecode_location(jvmti, method, location); break;
        case JVMTI_JLOCATION_MACHINEPC: ret = (boost::format("native: %s") % (jlong) location).str(); break;
        case JVMTI_JLOCATION_OTHER: ret = "unknown"; break;
    }

    return ret;
}

string get_bytecode_location(jvmtiEnv &jvmti, jmethodID method, jlocation location) {
    jint entryCount;
    jvmtiLineNumberEntry *entries;

    auto error = jvmti.GetLineNumberTable(method, &entryCount, &entries);
    check_jvmti_error(jvmti, error, "Cannot get line number table");

    string ret;

    bool found = false;
    auto entry = entries;
    for (int i = 0; i < entryCount; ++i, entry++) {
        if (entry->start_location == location) {
            ret = (format("line: %s") % entry->line_number).str();
            found = true;
        }
    }
    if (!found) {
        ret = (format("line: %s (~%s)") % entries->line_number % location).str();
    }
    deallocate(jvmti, entries);
    return ret;
}

list<string> get_stack_trace(jvmtiEnv &jvmti, jthread thread, int depth) {
    unique_ptr<jvmtiFrameInfo[]> frames(new jvmtiFrameInfo[depth]);
    jint count;

    auto error = jvmti.GetStackTrace(thread, 0, depth, frames.get(), &count);
    check_jvmti_error(jvmti, error, "Unable to get stack trace frames");

    list<string> lines;
    for (size_t i = 0; i < count; i++) {
        lines.push_back(get_method_name(jvmti, frames[i].method));
    }

    return lines;
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
