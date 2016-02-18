#include "jvmti.hpp"
#include "jni.hpp"
#include "Object.hpp"
#include "GlobalAgentData.hpp"
#include "common.hpp"

#include <sstream>

using namespace std;
using namespace boost;
using namespace jeff;

string jeff::get_class_status(jvmtiEnv &jvmti, jclass type) {
    jint status = 0;
    jvmtiError error = jvmti.GetClassStatus(type, &status);
    ASSERT_JVMTI_MSG(error, "Cannot get class status");

    list<string> flags = list<string>();
    if (status & JVMTI_CLASS_STATUS_VERIFIED) {
        flags.push_back("JVMTI_CLASS_STATUS_VERIFIED");
    }
    if (status & JVMTI_CLASS_STATUS_PREPARED) {
        flags.push_back("JVMTI_CLASS_STATUS_PREPARED");
    }
    if (status & JVMTI_CLASS_STATUS_PREPARED) {
        flags.push_back("JVMTI_CLASS_STATUS_INITIALIZED");
    }
    if (status & JVMTI_CLASS_STATUS_PREPARED) {
        flags.push_back("JVMTI_CLASS_STATUS_ERROR");
    }
    if (status & JVMTI_CLASS_STATUS_PREPARED) {
        flags.push_back("JVMTI_CLASS_STATUS_ARRAY");
    }
    if (status & JVMTI_CLASS_STATUS_PREPARED) {
        flags.push_back("JVMTI_CLASS_STATUS_PRIMITIVE");
    }

    auto start = format("%s:") % status;
    auto join_lines = [](string a, string b) { return a + "&" + b; };
    return join(flags, start.str(), join_lines);
}

string jeff::get_class_signature(jvmtiEnv &jvmti, jclass type) {
    char *signature;
    jvmtiError error = jvmti.GetClassSignature(type, &signature, nullptr);
    if (error == JVMTI_ERROR_INVALID_CLASS) {
        return "<invalid class>";
    }
    ASSERT_JVMTI_MSG(error, (format("Unable to get class signature, class status: '%s'") %
                             get_class_status(jvmti, type)).str().c_str());

    string ret = string(signature);
    deallocate(jvmti, signature);
    return ret;
}

/* Get a name for a jmethodID */
string jeff::get_method_name(jvmtiEnv &jvmti, jmethodID method) {
    jvmtiError error;

    jclass declaringType;
    error = jvmti.GetMethodDeclaringClass(method, &declaringType);
    check_jvmti_error(jvmti, error, "Unable to get method declaring class");

    string type = get_class_signature(jvmti, declaringType);

    char *name;
    char *sig;
    char *gsig;
    error = jvmti.GetMethodName(method, &name, &sig, &gsig);
    check_jvmti_error(jvmti, error, "Unable to get method name");

    std::stringstream stream;
    stream << type << "#" << name << ((gsig == NULL) ? sig : gsig);

    deallocate(jvmti, gsig);
    deallocate(jvmti, sig);
    deallocate(jvmti, name);

    return stream.str();
}

unique_ptr<Object> jeff::get_local_value(jvmtiEnv &jvmti, JNIEnv &jni, jthread thread, int depth, int slot,
                                         string signature) {

    std::cout << "jeff::get_local_value: " << get_thread_name(jvmti, jni, thread) << " " << depth << " " << slot <<
    " " << signature << endl;
    jvmtiError error;
    switch (signature.c_str()[0]) {
        case 'Z':   /* boolean */
            jint bool_value;
            error = jvmti.GetLocalInt(thread, depth, slot, &bool_value);
            ASSERT_JVMTI_MSG(error, "Unable to get local value");
            return Object::from(jvmti, jni, bool_value > 0);
        case 'C':   /* char */
            jint char_value;
            error = jvmti.GetLocalInt(thread, depth, slot, &char_value);
            ASSERT_JVMTI_MSG(error, "Unable to get local value");
            return Object::from(jvmti, jni, (jchar) char_value);
        case 'B':   /* byte */
            jint byte_value;
            error = jvmti.GetLocalInt(thread, depth, slot, &byte_value);
            ASSERT_JVMTI_MSG(error, "Unable to get local value");
            return Object::from(jvmti, jni, (jbyte) byte_value);
        case 'S':   /* short */
            jint short_value;
            error = jvmti.GetLocalInt(thread, depth, slot, &short_value);
            ASSERT_JVMTI_MSG(error, "Unable to get local value");
            return Object::from(jvmti, jni, (jshort) short_value);
        case 'I':   /* int */
            jint int_value;
            error = jvmti.GetLocalInt(thread, depth, slot, &int_value);
            ASSERT_JVMTI_MSG(error, "Unable to get local value");
            return Object::from(jvmti, jni, int_value);
        case 'J':   /* long */
            jlong long_value;
            error = jvmti.GetLocalLong(thread, depth, slot, &long_value);
            ASSERT_JVMTI_MSG(error, "Unable to get local value");
            return Object::from(jvmti, jni, long_value);
        case 'F':   /* float */
            jfloat float_value;
            error = jvmti.GetLocalFloat(thread, depth, slot, &float_value);
            ASSERT_JVMTI_MSG(error, "Unable to get local value");
            ASSERT_MSG(sizeof(float) == 32, "size of float is: " + sizeof(float));
            return Object::from(jvmti, jni, float_value);
        case 'D':   /* double */
            jdouble double_value;
            error = jvmti.GetLocalDouble(thread, depth, slot, &double_value);
            ASSERT_JVMTI_MSG(error, "Unable to get local value");
            ASSERT_MSG(sizeof(double) == 64, "size of double is: " + sizeof(double));
            return Object::from(jvmti, jni, double_value);
        case 'L': {  /* Object */
            jobject object_value;
            error = jvmti.GetLocalObject(thread, depth, slot, &object_value);
            ASSERT_JVMTI_MSG(error, "Unable to get local value");
            auto ret = Object::from(jvmti, jni, object_value);
            jni.DeleteLocalRef(object_value);
            return ret;
        }
        case '[': {   /* Array */
            jobject array_value;
            error = jvmti.GetLocalObject(thread, depth, slot, &array_value);
            ASSERT_JVMTI_MSG(error, "Unable to get local value");
            auto ret = Object::from(jvmti, jni, (jarray) array_value);
            jni.DeleteLocalRef(array_value);
            return ret;
        }
        default: {
            format msg = format("Not expected signature '%s'") % signature;
            THROW_JAVA_EXCEPTION(msg.str().c_str(), RuntimeException);
            return Object::from(jvmti, jni, (jobject) nullptr);
        }
    }
}

list<string> jeff::get_method_local_variables(jvmtiEnv &jvmti, JNIEnv &jni, jthread thread, jmethodID method,
                                              int limit, int depth) {
    jint size;
    jvmtiLocalVariableEntry *entries;

    auto error = jvmti.GetLocalVariableTable(method, &size, &entries);
    if (error == JVMTI_ERROR_ABSENT_INFORMATION) {
        return list<string>(0);
    }
    check_jvmti_error(jvmti, error, "Unable to get local varable table");

    list<string> arguments;
    auto entry = entries;
    for (int i = 0; i < min(size, limit); ++i, entry++) {
        string name = string(entry->name);
        string signature = entry->signature;
        int slot = entry->slot;

        unique_ptr<Object> value = get_local_value(jvmti, jni, thread, depth, slot, signature);

        wstring to_string = value->toString();
        arguments.push_back((format("%s [%s] '%s'") % name % slot % S(to_string)).str());

        deallocate(jvmti, entry->generic_signature);
        deallocate(jvmti, entry->signature);
        deallocate(jvmti, entry->name);
    }

    deallocate(jvmti, entries);

    return arguments;
}

int jeff::get_method_arguments_size(jvmtiEnv &jvmti, jmethodID method) {
    jint size;
    auto error = jvmti.GetArgumentsSize(method, &size);
    check_jvmti_error(jvmti, error, "Unable to get arguments size");

    return size;
}

list<string> jeff::get_method_arguments(jvmtiEnv &jvmti, JNIEnv &jni, jthread thread, jmethodID method, int depth) {
    list<string> lines;
    int size = get_method_arguments_size(jvmti, method);
    list<string> variables = get_method_local_variables(jvmti, jni, thread, method, size, depth);
    size = min(size, (int) variables.size());
    for (size_t i = 0; i < size; i++) {
        lines.push_back(variables.front());
        variables.pop_front();
    }
    return lines;
}

/* Get a name for a jthread */
string jeff::get_thread_name(jvmtiEnv &jvmti, JNIEnv &jni, jthread thread) {
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

string jeff::get_location(jvmtiEnv &jvmti, jmethodID method, jlocation location) {
    jvmtiJlocationFormat format;

    auto error = jvmti.GetJLocationFormat(&format);
    check_jvmti_error(jvmti, error, "Cannot get location format");

    switch (format) {
        case JVMTI_JLOCATION_JVMBCI:
            return get_bytecode_location(jvmti, method, location);
        case JVMTI_JLOCATION_MACHINEPC:
            return (boost::format("native: %s") % (jlong) location).str();
        case JVMTI_JLOCATION_OTHER:
            return "unknown";
    }
    return "";
}

string jeff::get_bytecode_location(jvmtiEnv &jvmti, jmethodID method, jlocation location) {
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

int jeff::get_stack_frame_count(jvmtiEnv &jvmti, jthread thread) {
    jint count_ptr;
    auto error = jvmti.GetFrameCount(thread, &count_ptr);
    check_jvmti_error(jvmti, error, "Unable to get stack frame count");
    return count_ptr;
}

list<string> jeff::get_stack_trace(jvmtiEnv &jvmti, JNIEnv &jni, jthread thread) {
    int depth = get_stack_frame_count(jvmti, thread);
    return get_stack_trace(jvmti, jni, thread, depth);
}

list<string> jeff::get_stack_trace(jvmtiEnv &jvmti, JNIEnv &jni, jthread thread, int depth) {
    unique_ptr<jvmtiFrameInfo[]> frames(new jvmtiFrameInfo[depth]);
    jint count;

    auto error = jvmti.GetStackTrace(thread, 0, depth, frames.get(), &count);
    check_jvmti_error(jvmti, error, "Unable to get stack trace frames");

    list<string> lines;
    for (size_t i = 0; i < count; i++) {
        auto frame = frames[i];
        string method_name = get_method_name(jvmti, frame.method);

        list<string> args = get_method_arguments(jvmti, jni, thread, frame.method, i);
        auto join_lines = [](string a, string b) { return a + ", " + b; };
        string arguments_values = join(args, join_lines);

        auto line = boost::format("%s%s") % method_name % arguments_values;
        lines.push_back(line.str());
    }

    return lines;
}

string jeff::get_error_name(jvmtiEnv &jvmti, jvmtiError error, const string message) {
    char *error_name = NULL;
    jvmtiError error_ = jvmti.GetErrorName(error, &error_name);
    ASSERT_MSG(error_ == JVMTI_ERROR_NONE, "Another JVMTI ERROR while getting an error name");

    const string messageSeparator = message.empty() ? "" : "; ";
    auto name = format("%s%sJVMTI ERROR: '%d' (%s)") % message % messageSeparator % error %
                (error_name == NULL ? "Unknown" : error_name);
    deallocate(jvmti, error_name);
    return name.str();
}

/* All memory allocated by JVMTI must be freed by the JVMTI Deallocate
 *   interface.
 */
void jeff::deallocate(jvmtiEnv &jvmti, void *ptr) {
    jvmtiError error;

    error = jvmti.Deallocate((unsigned char *) ptr);
    check_jvmti_error(jvmti, error, "Cannot deallocate memory");
}

/* Allocation of JVMTI managed memory */
void *jeff::allocate(jvmtiEnv &jvmti, jint len) {
    void *ptr;

    jvmtiError error = jvmti.Allocate(len, (unsigned char **) &ptr);
    check_jvmti_error(jvmti, error, "Cannot allocate memory");
    return ptr;
}

void jeff::print_possible_capabilities(jvmtiEnv &jvmti) {
    jvmtiError error;

    jvmtiCapabilities potentialCapabilities = {0};
    error = jvmti.GetPotentialCapabilities(&potentialCapabilities);
    check_jvmti_error(jvmti, error, "Unable to get potential JVMTI capabilities.");

    cerr << boost::format("\nPotential JVMTI capabilities:\n"
                                  "can_tag_objects: '%s'\n"
                                  "can_generate_field_modification_events: '%s'\n"
                                  "can_generate_field_access_events: '%s'\n"
                                  "can_get_bytecodes: '%s'\n"
                                  "can_get_synthetic_attribute: '%s'\n"
                                  "can_get_owned_monitor_info: '%s'\n"
                                  "can_get_current_contended_monitor: '%s'\n"
                                  "can_get_monitor_info: '%s'\n"
                                  "can_pop_frame: '%s'\n"
                                  "can_redefine_classes: '%s'\n"
                                  "can_signal_thread: '%s'\n"
                                  "can_get_source_file_name: '%s'\n"
                                  "can_get_line_numbers: '%s'\n"
                                  "can_get_source_debug_extension: '%s'\n"
                                  "can_access_local_variables: '%s'\n"
                                  "can_maintain_original_method_order: '%s'\n"
                                  "can_generate_single_step_events: '%s'\n"
                                  "can_generate_exception_events: '%s'\n"
                                  "can_generate_frame_pop_events: '%s'\n"
                                  "can_generate_breakpoint_events: '%s'\n"
                                  "can_suspend: '%s'\n"
                                  "can_redefine_any_class: '%s'\n"
                                  "can_get_current_thread_cpu_time: '%s'\n"
                                  "can_get_thread_cpu_time: '%s'\n"
                                  "can_generate_method_entry_events: '%s'\n"
                                  "can_generate_method_exit_events: '%s'\n"
                                  "can_generate_all_class_hook_events: '%s'\n"
                                  "can_generate_compiled_method_load_events: '%s'\n"
                                  "can_generate_monitor_events: '%s'\n"
                                  "can_generate_vm_object_alloc_events: '%s'\n"
                                  "can_generate_native_method_bind_events: '%s'\n"
                                  "can_generate_garbage_collection_events: '%s'\n"
                                  "can_generate_object_free_events: '%s'\n"
                                  "can_force_early_return: '%s'\n"
                                  "can_get_owned_monitor_stack_depth_info: '%s'\n"
                                  "can_get_constant_pool: '%s'\n"
                                  "can_set_native_method_prefix: '%s'\n"
                                  "can_retransform_classes: '%s'\n"
                                  "can_retransform_any_class: '%s'\n"
                                  "can_generate_resource_exhaustion_heap_events: '%s'\n"
                                  "can_generate_resource_exhaustion_threads_events: '%s'\n"
    )
            % (potentialCapabilities.can_tag_objects == 1)
            % (potentialCapabilities.can_generate_field_modification_events == 1)
            % (potentialCapabilities.can_generate_field_access_events == 1)
            % (potentialCapabilities.can_get_bytecodes == 1)
            % (potentialCapabilities.can_get_synthetic_attribute == 1)
            % (potentialCapabilities.can_get_owned_monitor_info == 1)
            % (potentialCapabilities.can_get_current_contended_monitor == 1)
            % (potentialCapabilities.can_get_monitor_info == 1)
            % (potentialCapabilities.can_pop_frame == 1)
            % (potentialCapabilities.can_redefine_classes == 1)
            % (potentialCapabilities.can_signal_thread == 1)
            % (potentialCapabilities.can_get_source_file_name == 1)
            % (potentialCapabilities.can_get_line_numbers == 1)
            % (potentialCapabilities.can_get_source_debug_extension == 1)
            % (potentialCapabilities.can_access_local_variables == 1)
            % (potentialCapabilities.can_maintain_original_method_order == 1)
            % (potentialCapabilities.can_generate_single_step_events == 1)
            % (potentialCapabilities.can_generate_exception_events == 1)
            % (potentialCapabilities.can_generate_frame_pop_events == 1)
            % (potentialCapabilities.can_generate_breakpoint_events == 1)
            % (potentialCapabilities.can_suspend == 1)
            % (potentialCapabilities.can_redefine_any_class == 1)
            % (potentialCapabilities.can_get_current_thread_cpu_time == 1)
            % (potentialCapabilities.can_get_thread_cpu_time == 1)
            % (potentialCapabilities.can_generate_method_entry_events == 1)
            % (potentialCapabilities.can_generate_method_exit_events == 1)
            % (potentialCapabilities.can_generate_all_class_hook_events == 1)
            % (potentialCapabilities.can_generate_compiled_method_load_events == 1)
            % (potentialCapabilities.can_generate_monitor_events == 1)
            % (potentialCapabilities.can_generate_vm_object_alloc_events == 1)
            % (potentialCapabilities.can_generate_native_method_bind_events == 1)
            % (potentialCapabilities.can_generate_garbage_collection_events == 1)
            % (potentialCapabilities.can_generate_object_free_events == 1)
            % (potentialCapabilities.can_force_early_return == 1)
            % (potentialCapabilities.can_get_owned_monitor_stack_depth_info == 1)
            % (potentialCapabilities.can_get_constant_pool == 1)
            % (potentialCapabilities.can_set_native_method_prefix == 1)
            % (potentialCapabilities.can_retransform_classes == 1)
            % (potentialCapabilities.can_retransform_any_class == 1)
            % (potentialCapabilities.can_generate_resource_exhaustion_heap_events == 1)
            % (potentialCapabilities.can_generate_resource_exhaustion_threads_events == 1)

    << endl;
}

/* Every JVMTI interface returns an error code, which should be checked
 *   to avoid any cascading errors down the line.
 *   The interface GetErrorName() returns the actual enumeration constant
 *   name, making the error messages much easier to understand.
 */
void jeff::check_jvmti_error(jvmtiEnv &jvmti, jvmtiError error, const string message) {
    ASSERT_MSG(error == JVMTI_ERROR_NONE, get_error_name(jvmti, error, message).c_str());
}

bool jeff::is_jvmti_error(jvmtiEnv &jvmti, jvmtiError error, const string message) {
    if (error == JVMTI_ERROR_NONE) {
        return false;
    } else {
        std::cerr << "Error: " << get_error_name(jvmti, error, message).c_str() << std::endl;
        return true;
    }
}

/**
 * Use ASSERT_JVMTI_MSG macro.
 */
void ::jeff::__throw_jvmti_exception(jvmtiError error, const char *message, const char *exceptionType,
                                     const char *function, const char *file, int line) {
    jvmtiEnv &jvmti = *jeff::gdata.jvmti;
    __throw_exception(get_error_name(jvmti, error, message).c_str(), exceptionType, function, file, line);
}