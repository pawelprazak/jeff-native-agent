#include <iostream>
#include <string.h>
#include <jvmti.h>

#include "jni.h"
#include "jvmti.h"
#include "jvmticmlr.h"

using namespace std;

typedef struct {
    /* JVMTI Environment */
    jvmtiEnv *jvmti;
    jboolean vm_is_dead;
    jboolean vm_is_initialized;
    jboolean vm_is_started;
    /* Data access Lock */
    jrawMonitorID lock;
} GlobalAgentData;

static GlobalAgentData *gdata;

jint init(JavaVM *jvm, char *options);

void check_jvmti_error(jvmtiEnv *jvmti, jvmtiError errnum, const char *str);

void fatal_error(const char *format, ...);

void stdout_message(const char *format, ...);

static void enter_critical_section(jvmtiEnv *jvmti);

static void exit_critical_section(jvmtiEnv *jvmti);

/**
 * The VM will start the agent by calling this function.
 */
#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"
JNIEXPORT jint JNICALL
Agent_OnLoad(JavaVM *jvm, char *options, void *reserved) {
    return init(jvm, options);
};
#pragma clang diagnostic pop

/**
 * A VM may support a mechanism that allows agents to be started in the VM during the live phase.
 * Note that some capabilities may not be available in the live phase.
 */
#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"
JNIEXPORT jint JNICALL
Agent_OnAttach(JavaVM *jvm, char *options, void *reserved) {
    return init(jvm, options);
};
#pragma clang diagnostic pop

/**
 * This function will be called by the VM when the library is about to be unloaded.
 * This function can be used to clean-up resources allocated by the agent.
 */
#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"
JNIEXPORT void JNICALL
Agent_OnUnload(JavaVM *vm) {
    delete (gdata);
};
#pragma clang diagnostic pop

static void JNICALL VMStartCallback(jvmtiEnv *jvmti, JNIEnv *env);

static void JNICALL VMInitCallback(jvmtiEnv *jvmti, JNIEnv *env, jthread thread);

static void JNICALL VMDeathCallback(jvmtiEnv *jvmti, JNIEnv *env);

static void JNICALL MethodEntryCallback(jvmtiEnv *jvmti, JNIEnv *jni, jthread thread, jmethodID method);

static void JNICALL MethodExitCallback(jvmtiEnv *jvmti, JNIEnv *jni, jthread thread, jmethodID method,
                                       jboolean was_popped_by_exception, jvalue return_value);

static void JNICALL ExceptionCallback(jvmtiEnv *jvmti, JNIEnv *jni, jthread thread, jmethodID method,
                                      jlocation location, jobject exception, jmethodID catch_method,
                                      jlocation catch_location);

static void JNICALL ExceptionCatchCallback(jvmtiEnv *jvmti, JNIEnv *jni, jthread thread, jmethodID method,
                                           jlocation location, jobject exception);

jint init(JavaVM *jvm, char *options) {
    jvmtiEnv *jvmti;
    jint result = jvm->GetEnv((void **) &jvmti, JVMTI_VERSION_1_2);
    if (result != JNI_OK) {
        /* This means that the VM was unable to obtain this version of the
         *   JVMTI interface, this is a fatal error.
         */
        fatal_error("ERROR: Unable to create jvmtiEnv, GetEnv failed, error=%d\n", result);
        return JNI_ERR;
    }

    /* Setup initial global agent data area */
    gdata = new GlobalAgentData();
    gdata->jvmti = jvmti;

    /* Immediately after getting the jvmti* we need to ask for the
     *   capabilities this agent will need. In this case we need to make
     *   sure that we can get all class load hooks.
     */
    jvmtiCapabilities capabilities = jvmtiCapabilities();
    capabilities.can_get_owned_monitor_info = 1;
    capabilities.can_generate_method_entry_events = 1;
    capabilities.can_generate_method_exit_events = 1;
    capabilities.can_generate_exception_events = 1;
    capabilities.can_generate_resource_exhaustion_heap_events = 1;
    capabilities.can_generate_resource_exhaustion_threads_events = 1;
    capabilities.can_tag_objects = 1;

    jvmtiError error;

    error = jvmti->AddCapabilities(&capabilities);
    check_jvmti_error(jvmti, error, "Unable to get necessary JVMTI capabilities.");

    /* Next we need to provide the pointers to the callback functions to this jvmti
     */
    error = jvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_VM_START, (jthread) NULL);
    check_jvmti_error(jvmti, error, "Cannot set event notification");

    error = jvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_VM_INIT, (jthread) NULL);
    check_jvmti_error(jvmti, error, "Cannot set event notification");

    error = jvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_VM_DEATH, (jthread) NULL);
    check_jvmti_error(jvmti, error, "Cannot set event notification");

    error = jvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_METHOD_ENTRY, (jthread) NULL);
    check_jvmti_error(jvmti, error, "Cannot set event notification");

    error = jvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_METHOD_EXIT, (jthread) NULL);
    check_jvmti_error(jvmti, error, "Cannot set event notification");

    error = jvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_EXCEPTION, (jthread) NULL);
    check_jvmti_error(jvmti, error, "Cannot set event notification");

    error = jvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_EXCEPTION_CATCH, (jthread) NULL);
    check_jvmti_error(jvmti, error, "Cannot set event notification");

    error = jvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_THREAD_START, (jthread) NULL);
    check_jvmti_error(jvmti, error, "Cannot set event notification");

    error = jvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_THREAD_END, (jthread) NULL);
    check_jvmti_error(jvmti, error, "Cannot set event notification");

    error = jvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_RESOURCE_EXHAUSTED, (jthread) NULL);
    check_jvmti_error(jvmti, error, "Cannot set event notification");

    jvmtiEventCallbacks callbacks = jvmtiEventCallbacks();

    callbacks.VMStart = &VMStartCallback;    /* JVMTI_EVENT_VM_START */
    callbacks.VMInit = &VMInitCallback;      /* JVMTI_EVENT_VM_INIT */
    callbacks.VMDeath = &VMDeathCallback;    /* JVMTI_EVENT_VM_DEATH */

    /* Enabling method entry or exit events will significantly degrade performance on many platforms
     * and is thus not advised for performance critical usage (such as profiling).
     * Bytecode instrumentation should be used in these cases. */
    callbacks.MethodEntry = &MethodEntryCallback; /* JVMTI_EVENT_METHOD_ENTRY */
    callbacks.MethodExit = &MethodExitCallback;   /* JVMTI_EVENT_METHOD_EXIT */

    callbacks.Exception = &ExceptionCallback;           /* JVMTI_EVENT_EXCEPTION */
    callbacks.ExceptionCatch = &ExceptionCatchCallback; /* JVMTI_EVENT_EXCEPTION_CATCH */

//    callbacks.ThreadStart = &ThreadStartCallback; /* JVMTI_EVENT_THREAD_START */
//    callbacks.ThreadEnd = &ThreadEndCallback;     /* JVMTI_EVENT_THREAD_END */

//    callbacks.ResourceExhausted = &ResourceExhaustedCallback; /* JVMTI_EVENT_RESOURCE_EXHAUSTED */

    error = jvmti->SetEventCallbacks(&callbacks, (jint) sizeof(callbacks));
    check_jvmti_error(jvmti, error, "Cannot set jvmti callbacks");

    /* Here we create a raw monitor for our use in this agent to protect critical sections of code.
     */
    error = jvmti->CreateRawMonitor("agent data", &(gdata->lock));
    check_jvmti_error(jvmti, error, "Cannot create raw monitor");

    return JNI_OK;
}

/* Callback for JVMTI_EVENT_VM_START */
static void JNICALL
VMStartCallback(jvmtiEnv *jvmti, JNIEnv *env) {
    enter_critical_section(jvmti);
    {
        /* The VM has started. */
        stdout_message("VMStart\n");

        /* Indicate VM has started */
        gdata->vm_is_started = JNI_TRUE;
    }
    exit_critical_section(jvmti);
}

/* Callback for JVMTI_EVENT_VM_INIT */
static void JNICALL
VMInitCallback(jvmtiEnv *jvmti, JNIEnv *env, jthread thread) {
    enter_critical_section(jvmti);
    {
        /* The VM has initialized. */
        stdout_message("VMInit\n");

        /* Indicate VM has initialized */
        gdata->vm_is_initialized = JNI_TRUE;
    }
    exit_critical_section(jvmti);
}

/* Callback for JVMTI_EVENT_VM_DEATH */
static void JNICALL
VMDeathCallback(jvmtiEnv *jvmti, JNIEnv *env) {
//    enter_critical_section(jvmti);
//    {
    /* The VM has died. */
    stdout_message("VMDeath\n");

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
    gdata->vm_is_dead = JNI_TRUE;

//    }
//    exit_critical_section(jvmti);
}

static void JNICALL
MethodEntryCallback(jvmtiEnv *jvmti,
                    JNIEnv *jni,
                    jthread thread,
                    jmethodID method) {
    char *name;
    char *sig;
    char *gsig;
    jvmtiError error = jvmti->GetMethodName(method, &name, &sig, &gsig);
    check_jvmti_error(jvmti, error, "Unable to get method information.");

    printf("Enter Method:%s%s\n", name, sig);
}

static void JNICALL
MethodExitCallback(jvmtiEnv *jvmti,
                   JNIEnv *jni,
                   jthread thread,
                   jmethodID method,
                   jboolean was_popped_by_exception,
                   jvalue return_value) {
    char *name;
    char *sig;
    char *gsig;
    jvmtiError error = jvmti->GetMethodName(method, &name, &sig, &gsig);
    check_jvmti_error(jvmti, error, "Unable to get method information.");

    printf("Exit Method:%s%s\n", name, sig);
}

static void JNICALL
ExceptionCallback(jvmtiEnv *jvmti,
                  JNIEnv *jni,
                  jthread thread,
                  jmethodID method,
                  jlocation location,
                  jobject exception,
                  jmethodID catch_method,
                  jlocation catch_location) {

    char *name;
    char *sig;
    char *gsig;
    jvmtiError error = jvmti->GetMethodName(method, &name, &sig, &gsig);
    check_jvmti_error(jvmti, error, "Unable to get method information.");

    printf("Exception in Method:%s%s\n", name, sig);
};

static void JNICALL
ExceptionCatchCallback(jvmtiEnv *jvmti,
                       JNIEnv *jni,
                       jthread thread,
                       jmethodID method,
                       jlocation location,
                       jobject exception) {
    char *name;
    char *sig;
    char *gsig;
    jvmtiError error = jvmti->GetMethodName(method, &name, &sig, &gsig);
    check_jvmti_error(jvmti, error, "Unable to get method information.");

    printf("Cought exception in Method:%s%s\n", name, sig);

}


/* ------------------------------------------------------------------- */
/* Generic JVMTI utility functions */

/* Every JVMTI interface returns an error code, which should be checked
 *   to avoid any cascading errors down the line.
 *   The interface GetErrorName() returns the actual enumeration constant
 *   name, making the error messages much easier to understand.
 */
void
check_jvmti_error(jvmtiEnv *jvmti, jvmtiError errnum, const char *str) {
    if (errnum != JVMTI_ERROR_NONE) {
        char *errnum_str = NULL;
        jvmti->GetErrorName(errnum, &errnum_str);

        fatal_error("ERROR: JVMTI: %d(%s): %s\n", errnum,
                    (errnum_str == NULL ? "Unknown" : errnum_str),
                    (str == NULL ? "" : str));
    }
}

/* Enter a critical section by doing a JVMTI Raw Monitor Enter */
static void
enter_critical_section(jvmtiEnv *jvmti) {
    jvmtiError error;

    error = jvmti->RawMonitorEnter(gdata->lock);
    check_jvmti_error(jvmti, error, "Cannot enter with raw monitor");
}

/* Exit a critical section by doing a JVMTI Raw Monitor Exit */
static void
exit_critical_section(jvmtiEnv *jvmti) {
    jvmtiError error;

    error = jvmti->RawMonitorExit(gdata->lock);
    check_jvmti_error(jvmti, error, "Cannot exit with raw monitor");
}

void
stdout_message(const char *format, ...) {
    va_list ap;

    va_start(ap, format);
    (void) vfprintf(stdout, format, ap);
    va_end(ap);
}

void
fatal_error(const char *format, ...) {
    va_list ap;

    va_start(ap, format);
    (void) vfprintf(stderr, format, ap);
    (void) fflush(stderr);
    va_end(ap);
    exit(3);
}