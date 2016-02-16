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

int get_stack_frame_count(jvmtiEnv &jvmti, jthread thread) {
    jint count_ptr;
    auto error = jvmti.GetFrameCount(thread, &count_ptr);
    check_jvmti_error(jvmti, error, "Unable to get stack frame count");
    return count_ptr;
}

list<string> get_stack_trace(jvmtiEnv &jvmti, jthread thread) {
    int depth = get_stack_frame_count(jvmti, thread);
    return get_stack_trace(jvmti, thread, depth);
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

void print_possible_capabilities(jvmtiEnv &jvmti) {
    jvmtiError error;

    jvmtiCapabilities potentialCapabilities = {0};
    error = jvmti.GetPotentialCapabilities(&potentialCapabilities);
    check_jvmti_error(jvmti, error, "Unable to get potential JVMTI capabilities.");

    cerr << boost::format("Potential JVMTI capabilities:\n"
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
