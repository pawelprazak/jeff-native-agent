#include "main.hpp"

#include <boost/format.hpp>

#include "common.hpp"
#include "jni.hpp"
#include "jvmti.hpp"

#include "GlobalAgentData.hpp"
#include "Object.hpp"
#include "Type.hpp"

using namespace std;
using namespace jeff;

JNIEXPORT jint JNICALL
Agent_OnLoad(JavaVM *jvm, char *options, void *reserved) {
    return init(jvm, options);
}

JNIEXPORT jint JNICALL
Agent_OnAttach(JavaVM *jvm, char *options, void *reserved) {
    std::cerr << "Not supported" << std::endl;
    return JNI_ERR;
}

JNIEXPORT void JNICALL
Agent_OnUnload(JavaVM *jvm) {
    /* Make sure all allocated space is freed */
}

jint init(JavaVM *jvm, char *options) {
    jvmtiEnv *jvmti;
    jint result = jvm->GetEnv((void **) &jvmti, JVMTI_VERSION_1_2);
    if (result != JNI_OK) {
        /* This means that the VM was unable to obtain this version of the
         *   JVMTI interface, this is a fatal error.
         */
        std::cerr << boost::format("ERROR: Unable to access JVMTI Version 1.2 (0x%x),"
                                           " is your JDK a 6.0 or newer version?"
                                           " JNIEnv's GetEnv() returned %d\n") % JVMTI_VERSION_1_2 % result;
        return JNI_ERR;
    }

    /* Setup initial global agent data area */
    gdata.jvm = jvm;
    gdata.jvmti = jvmti;

    //print_possible_capabilities(*jvmti);

    /* Immediately after getting the jvmti* we need to ask for the
     *   capabilities this agent will need. In this case we need to make
     *   sure that we can get all class load hooks.
     */
    jvmtiCapabilities capabilities = {0};
    capabilities.can_get_owned_monitor_info = 1;
    capabilities.can_get_line_numbers = 1;
    capabilities.can_get_source_file_name = 1;
//    capabilities.can_get_source_debug_extension = 1;
    capabilities.can_tag_objects = 1;
    capabilities.can_access_local_variables = 1;
    /*
     * Enabling method entry or exit events will significantly degrade performance on many platforms
     * and is thus not advised for performance critical usage (such as profiling).
     * Bytecode instrumentation should be used in these cases.
     */
//    capabilities.can_generate_method_entry_events = 1;
//    capabilities.can_generate_method_exit_events = 1;
    capabilities.can_generate_exception_events = 1;
    capabilities.can_generate_resource_exhaustion_heap_events = 1;
    capabilities.can_generate_resource_exhaustion_threads_events = 1;

    jvmtiError error;

    error = jvmti->AddCapabilities(&capabilities);
    if (is_jvmti_error(*jvmti, error, "Unable to get necessary JVMTI capabilities")) return JNI_ERR;

    /* Next we need to provide the pointers to the callback functions to this jvmti */
    error = jvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_VM_START, (jthread) NULL);
    if (is_jvmti_error(*jvmti, error, "Cannot set event notification: JVMTI_EVENT_VM_START")) return JNI_ERR;

    error = jvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_VM_INIT, (jthread) NULL);
    if (is_jvmti_error(*jvmti, error, "Cannot set event notification: JVMTI_EVENT_VM_INIT")) return JNI_ERR;

    error = jvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_VM_DEATH, (jthread) NULL);
    if (is_jvmti_error(*jvmti, error, "Cannot set event notification: JVMTI_EVENT_VM_DEATH")) return JNI_ERR;

    jvmtiEventCallbacks callbacks = jvmtiEventCallbacks();

    callbacks.VMStart = &VMStartCallback;    /* JVMTI_EVENT_VM_START */
    callbacks.VMInit = &VMInitCallback;      /* JVMTI_EVENT_VM_INIT */
    callbacks.VMDeath = &VMDeathCallback;    /* JVMTI_EVENT_VM_DEATH */

    callbacks.MethodEntry = &MethodEntryCallback; /* JVMTI_EVENT_METHOD_ENTRY */
    callbacks.MethodExit = &MethodExitCallback;   /* JVMTI_EVENT_METHOD_EXIT */

    callbacks.Exception = &ExceptionCallback;           /* JVMTI_EVENT_EXCEPTION */
    callbacks.ExceptionCatch = &ExceptionCatchCallback; /* JVMTI_EVENT_EXCEPTION_CATCH */

    callbacks.ThreadStart = &ThreadStartCallback; /* JVMTI_EVENT_THREAD_START */
    callbacks.ThreadEnd = &ThreadEndCallback;     /* JVMTI_EVENT_THREAD_END */

    callbacks.ResourceExhausted = &ResourceExhaustedCallback; /* JVMTI_EVENT_RESOURCE_EXHAUSTED */

    error = jvmti->SetEventCallbacks(&callbacks, (jint) sizeof(callbacks));
    if (is_jvmti_error(*jvmti, error, "Cannot set jvmti callbacks")) return JNI_ERR;

    /* Here we create a raw monitor for our use in this agent to protect critical sections of code.
     */
    error = jvmti->CreateRawMonitor("agent data", &(gdata.lock));
    if (is_jvmti_error(*jvmti, error, "Cannot create raw monitor")) return JNI_ERR;

    std::cout << "The agent init phase successful\n";
    return JNI_OK;
}

jint live(jvmtiEnv &jvmti) {
    jvmtiError error;

    error = jvmti.SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_EXCEPTION, (jthread) NULL);
    if (is_jvmti_error(jvmti, error, "Cannot set event notification: JVMTI_EVENT_EXCEPTION")) return error;

//    error = jvmti.SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_EXCEPTION_CATCH, (jthread) NULL);
//    if (is_jvmti_error(jvmti, error, "Cannot set event notification: JVMTI_EVENT_EXCEPTION_CATCH")) return error;

//    error = jvmti.SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_METHOD_ENTRY, (jthread) NULL);
//    if (is_jvmti_error(jvmti, error, "Cannot set event notification: JVMTI_EVENT_METHOD_ENTRY")) return error;

//    error = jvmti.SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_METHOD_EXIT, (jthread) NULL);
//    if (is_jvmti_error(jvmti, error, "Cannot set event notification: JVMTI_EVENT_METHOD_EXIT")) return error;

//    error = jvmti.SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_THREAD_START, (jthread) NULL);
//    if (is_jvmti_error(jvmti, error, "Cannot set event notification: JVMTI_EVENT_THREAD_START")) return error;

//    error = jvmti.SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_THREAD_END, (jthread) NULL);
//    if (is_jvmti_error(jvmti, error, "Cannot set event notification: JVMTI_EVENT_THREAD_END")) return error;

//    error = jvmti.SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_RESOURCE_EXHAUSTED, (jthread) NULL);
//    if (is_jvmti_error(jvmti, error, "Cannot set event notification: JVMTI_EVENT_RESOURCE_EXHAUSTED")) return JNI_ERR;

    std::cout << "The agent live phase successful\n";
    return JNI_OK;
}

/* Callback for JVMTI_EVENT_VM_START */
void JNICALL VMStartCallback(jvmtiEnv *jvmti, JNIEnv *env) {
    enter_critical_section(jvmti);
    {
        /* The VM has started. */
        std::string message = "VM Started (JVMTI_EVENT_VM_START)";
        std::cout << message << endl;

        /* Indicate VM has started */
        gdata.vm_is_started = JNI_TRUE;
        gdata.sender = Sender::create("localhost", "9999");
        gdata.sender->send(message);
    }
    exit_critical_section(jvmti);
}

/* Callback for JVMTI_EVENT_VM_INIT */
void JNICALL VMInitCallback(jvmtiEnv *jvmti, JNIEnv *env, jthread thread) {
    enter_critical_section(jvmti);
    {
        /* The VM has started. */
        string threadName = get_thread_name(*jvmti, *env, thread);
        auto message = boost::format("VMInit thread '%s' (JVMTI_EVENT_VM_INIT)\n") % threadName;
        std::cout << message;

        /* Indicate VM has initialized */
        gdata.vm_is_initialized = JNI_TRUE;

        /* The VM is now initialized, at this time we make our requests for additional events. */
        jint err = live(*jvmti);
        ASSERT_MSG(err == JVMTI_ERROR_NONE, (boost::format("live() returned an error '%s'") % err).str().c_str());

        gdata.sender->send(message.str());
    }
    exit_critical_section(jvmti);
}

/* Callback for JVMTI_EVENT_VM_DEATH */
void JNICALL VMDeathCallback(jvmtiEnv *jvmti, JNIEnv *env) {
    enter_critical_section(jvmti);
    {
        /* The VM has died. */
        std::string message = "VM Died (JVMTI_EVENT_VM_DEATH)\n";
        std::cout << message;

        /* The critical section here is important to hold back the VM death
         *    until all other callbacks have completed.
         */

        /* Since this critical section could be holding up other threads
         *   in other event callbacks, we need to indicate that the VM is
         *   dead so that the other callbacks can short circuit their work.
         *   We don't expect any further events after VmDeath but we do need
         *   to be careful that existing threads might be in our own agent
         *   callback code.
         */
        gdata.vm_is_dead = JNI_TRUE;

        if (gdata.sender != nullptr) {
            gdata.sender->send(message);
            gdata.sender->flush();
            gdata.sender->stop();
        }
    }
    exit_critical_section(jvmti);
}

void JNICALL MethodEntryCallback(jvmtiEnv *jvmti,
                                 JNIEnv *jni,
                                 jthread thread,
                                 jmethodID method) {
    string methodName = get_method_name(*jvmti, method);
    std::cout << boost::format("Enter Method: %s\n") % methodName;
}

void JNICALL MethodExitCallback(jvmtiEnv *jvmti,
                                JNIEnv *jni,
                                jthread thread,
                                jmethodID method,
                                jboolean was_popped_by_exception,
                                jvalue return_value) {
    string methodName = get_method_name(*jvmti, method);
    string wasException = was_popped_by_exception == JNI_TRUE ? " (popped_by_exception)" : "";
    std::cout << boost::format("Exit Method : %s %s\n") % methodName % wasException;
}

void JNICALL ExceptionCallback(jvmtiEnv *jvmti,
                               JNIEnv *jni,
                               jthread thread,
                               jmethodID method,
                               jlocation location,
                               jobject exception,
                               jmethodID catch_method,
                               jlocation catch_location) {

    string methodName = get_method_name(*jvmti, method);

    unique_ptr<Object> object = Object::from(*jvmti, *jni, exception);
    string exceptionSignature = object->getType().getSignature();

    std::function<string(jobject)> string_transformer = [jni](jobject result) mutable {
        return (result == nullptr) ? "" : jeff::to_string(*jni, static_cast<jstring>(result));
    };
    string message = call_method(*jni, exception, "getMessage", "()Ljava/lang/String;", string_transformer);

    string line = get_location(*jvmti, method, location);

    auto join_lines = [](string a, string b) { return "\t" + a + "\n\t" + b; };
    string stack_trace = join(get_stack_trace(*jvmti, *jni, thread), join_lines);

    auto the_message =
            boost::format("Uncought exception: %s, message: '%s'\n\tin method: %s [%s]\nStack trace:%s\n\n")
            % exceptionSignature % message % methodName % line % stack_trace;
    std::cout << the_message;

    gdata.sender->send(the_message.str());
}

void JNICALL ExceptionCatchCallback(jvmtiEnv *jvmti,
                                    JNIEnv *jni,
                                    jthread thread,
                                    jmethodID method,
                                    jlocation location,
                                    jobject exception) {

    string methodName = get_method_name(*jvmti, method);

    unique_ptr<Object> object = Object::from(*jvmti, *jni, exception);
    string exceptionSignature = object->getType().getSignature();

    std::function<string(jobject)> string_transformer = [jni](jobject result) mutable {
        return (result == nullptr) ? "" : jeff::to_string(*jni, static_cast<jstring>(result));
    };
    string message = call_method(*jni, exception, "getMessage", "()Ljava/lang/String;", string_transformer);
    string line = get_location(*jvmti, method, location);

    auto the_message = boost::format("Cought exception: %s, message: '%s'\n\tin method: %s [%s]\n")
                       % exceptionSignature % message % methodName % line;
    std::cout << the_message;

    gdata.sender->send(the_message.str());
}

void JNICALL ThreadStartCallback(jvmtiEnv *jvmti,
                                 JNIEnv *env,
                                 jthread thread) {
    enter_critical_section(jvmti);
    {
        /* It's possible we get here right after VmDeath event, be careful */
        if (!gdata.vm_is_dead) {
            string threadName = get_thread_name(*jvmti, *env, thread);
            std::cout << boost::format("ThreadStart: %s\n") % threadName;
        }
    }
    exit_critical_section(jvmti);
}

void JNICALL ThreadEndCallback(jvmtiEnv *jvmti,
                               JNIEnv *jni,
                               jthread thread) {
    enter_critical_section(jvmti);
    {
        /* It's possible we get here right after VmDeath event, be careful */
        if (!gdata.vm_is_dead) {
            string threadName = get_thread_name(*jvmti, *jni, thread);
            std::cout << boost::format("ThreadEnd: %s\n") % threadName;
        }
    }
    exit_critical_section(jvmti);
}

void JNICALL ResourceExhaustedCallback(jvmtiEnv *jvmti,
                                       JNIEnv *jni,
                                       jint flags,
                                       const void *reserved,
                                       const char *description) {
    enter_critical_section(jvmti);
    {
        /* It's possible we get here right after VmDeath event, be careful */
        if (!gdata.vm_is_dead) {
            std::string message;
            switch (flags) {
                case JVMTI_RESOURCE_EXHAUSTED_OOM_ERROR: {
                    message = (boost::format("VM died: Out Of Memory Error, %s\n") % description).str();
                    break;
                }
                case JVMTI_RESOURCE_EXHAUSTED_JAVA_HEAP: {
                    message = (boost::format("VM died: Exhausted Java Heap, %s\n") % description).str();
                    break;
                }
                case JVMTI_RESOURCE_EXHAUSTED_THREADS: {
                    message = (boost::format("VM died: Exhausted threads, %s\n") % description).str();
                    break;
                }
                default: {
                    message = (boost::format("VM died: Unknown, %s\n") % description).str();
                    break;
                }
            }
            std::cout << message;
            gdata.sender->send(message);
        }
    }
    exit_critical_section(jvmti);
}

/* ------------------------------------------------------------------- */
/* Generic JVMTI utility functions */

/* Enter a critical section by doing a JVMTI Raw Monitor Enter */
void enter_critical_section(jvmtiEnv *jvmti) {
    jvmtiError error = jvmti->RawMonitorEnter(gdata.lock);
    check_jvmti_error(*jvmti, error, "Cannot enter with raw monitor");
}

/* Exit a critical section by doing a JVMTI Raw Monitor Exit */
void exit_critical_section(jvmtiEnv *jvmti) {
    jvmtiError error = jvmti->RawMonitorExit(gdata.lock);
    check_jvmti_error(*jvmti, error, "Cannot exit with raw monitor");
}                                              
