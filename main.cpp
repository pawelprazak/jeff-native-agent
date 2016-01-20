/**
 * see: https://docs.oracle.com/javase/8/docs/platform/jvmti/jvmti.html
 */

#include <iostream>
#include <string.h>
#include <jvmti.h>

using namespace std;

/* Some constant maximum sizes */
enum {
    MAX_TOKEN_LENGTH = 16,
    MAX_THREAD_NAME_LENGTH = 512,
    MAX_METHOD_NAME_LENGTH = 1024
};

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

void *allocate(jvmtiEnv *jvmti, jint len);

void deallocate(jvmtiEnv *jvmti, void *ptr);

static void get_thread_name(jvmtiEnv *jvmti, jthread thread, char *tname, int maxlen);

void check_jvmti_error(jvmtiEnv *jvmti, jvmtiError errnum, const char *str);

void fatal_error(const char *format, ...);

void stdout_message(const char *format, ...);

static void enter_critical_section(jvmtiEnv *jvmti);

static void exit_critical_section(jvmtiEnv *jvmti);

jint init(JavaVM *jvm, char *options);

jint live(jvmtiEnv *jvmti);

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
    /* Make sure all allocated space is freed */
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

static void JNICALL ThreadStartCallback(jvmtiEnv *jvmti,
                                        JNIEnv *env,
                                        jthread thread);

static void JNICALL ThreadEndCallback(jvmtiEnv *jvmti,
                                      JNIEnv *env,
                                      jthread thread);

static void JNICALL ResourceExhaustedCallback(jvmtiEnv *jvmti,
                                              JNIEnv *env,
                                              jint flags,
                                              const void *reserved,
                                              const char *description);

jint init(JavaVM *jvm, char *options) {
    jvmtiEnv *jvmti;
    jint result = jvm->GetEnv((void **) &jvmti, JVMTI_VERSION_1_2);
    if (result != JNI_OK) {
        /* This means that the VM was unable to obtain this version of the
         *   JVMTI interface, this is a fatal error.
         */
        fatal_error("ERROR: Unable to access JVMTI Version 1.2 (0x%x),"
                            " is your JDK a 6.0 or newer version?"
                            " JNIEnv's GetEnv() returned %d\n",
                    JVMTI_VERSION_1_2, result);
//        return JNI_ERR;
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

    /* Next we need to provide the pointers to the callback functions to this jvmti */
    error = jvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_VM_START, (jthread) NULL);
    check_jvmti_error(jvmti, error, "Cannot set event notification");

    error = jvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_VM_INIT, (jthread) NULL);
    check_jvmti_error(jvmti, error, "Cannot set event notification");

    error = jvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_VM_DEATH, (jthread) NULL);
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

    callbacks.ThreadStart = &ThreadStartCallback; /* JVMTI_EVENT_THREAD_START */
    callbacks.ThreadEnd = &ThreadEndCallback;     /* JVMTI_EVENT_THREAD_END */

    callbacks.ResourceExhausted = &ResourceExhaustedCallback; /* JVMTI_EVENT_RESOURCE_EXHAUSTED */

    error = jvmti->SetEventCallbacks(&callbacks, (jint) sizeof(callbacks));
    check_jvmti_error(jvmti, error, "Cannot set jvmti callbacks");

    /* Here we create a raw monitor for our use in this agent to protect critical sections of code.
     */
    error = jvmti->CreateRawMonitor("agent data", &(gdata->lock));
    check_jvmti_error(jvmti, error, "Cannot create raw monitor");

    return JNI_OK;
}

jint live(jvmtiEnv *jvmti) {
    jvmtiError error;

    error = jvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_METHOD_ENTRY, (jthread) NULL);
    check_jvmti_error(jvmti, error, "Cannot set event notification");

    error = jvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_METHOD_EXIT, (jthread) NULL);
    check_jvmti_error(jvmti, error, "Cannot set event notification");

    error = jvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_THREAD_START, (jthread) NULL);
    check_jvmti_error(jvmti, error, "Cannot set event notification");

    error = jvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_THREAD_END, (jthread) NULL);
    check_jvmti_error(jvmti, error, "Cannot set event notification");

    error = jvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_RESOURCE_EXHAUSTED, (jthread) NULL);
    check_jvmti_error(jvmti, error, "Cannot set event notification");
    
    error = jvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_EXCEPTION, (jthread) NULL);
    check_jvmti_error(jvmti, error, "Cannot set event notification");

    error = jvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_EXCEPTION_CATCH, (jthread) NULL);
    check_jvmti_error(jvmti, error, "Cannot set event notification");

    return JNI_OK;
}

/* Callback for JVMTI_EVENT_VM_START */
static void JNICALL VMStartCallback(jvmtiEnv *jvmti, JNIEnv *env) {
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
static void JNICALL VMInitCallback(jvmtiEnv *jvmti, JNIEnv *env, jthread thread) {
    enter_critical_section(jvmti);
    {
        char tname[MAX_THREAD_NAME_LENGTH];

        /* The VM has started. */
        get_thread_name(jvmti, thread, tname, sizeof(tname));
        stdout_message("VMInit %s\n", tname);

        /* Indicate VM has initialized */
        gdata->vm_is_initialized = JNI_TRUE;

        /* The VM is now initialized, at this time we make our requests for additional events. */
        live(jvmti);
    }
    exit_critical_section(jvmti);
}

/* Callback for JVMTI_EVENT_VM_DEATH */
static void JNICALL VMDeathCallback(jvmtiEnv *jvmti, JNIEnv *env) {
    enter_critical_section(jvmti);
    {
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

    }
    exit_critical_section(jvmti);
}

static void JNICALL MethodEntryCallback(jvmtiEnv *jvmti,
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

static void JNICALL MethodExitCallback(jvmtiEnv *jvmti,
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

static void JNICALL ExceptionCallback(jvmtiEnv *jvmti,
                  JNIEnv *jni,
                  jthread thread,
                  jmethodID method,
                  jlocation location,
                  jobject exception,
                  jmethodID catch_method,
                  jlocation catch_location) {

    jvmtiError error;

    char *name;
    char *sig;
    char *gsig;
    error = jvmti->GetMethodName(method, &name, &sig, &gsig);
    check_jvmti_error(jvmti, error, "Unable to get method information.");

    printf("Exception in Method:%s%s\n", name, sig);

    // Get the exception class
    jclass exceptionClass = jni->GetObjectClass(exception);

    // Get the class object's class descriptor
    jclass classType = jni->GetObjectClass(exceptionClass);

    // Find the getSimpleName() method in the class object
    jmethodID methodId = jni->GetMethodID(classType, "getSimpleName", "()Ljava/lang/String;");
    jstring className = (jstring) jni->CallObjectMethod(exceptionClass, methodId);

    // Convert to native string
    const char *nativeString = jni->GetStringUTFChars(className, 0);

    // Use your class name string
    printf("Exception type:%s\n", nativeString);

    // And finally, release the JNI objects after usage
    jni->ReleaseStringUTFChars(className, nativeString);
    jni->DeleteLocalRef(classType);
    jni->DeleteLocalRef(exceptionClass);
};

static void JNICALL ExceptionCatchCallback(jvmtiEnv *jvmti,
                       JNIEnv *jni,
                       jthread thread,
                       jmethodID method,
                       jlocation location,
                       jobject exception) {
    jvmtiError error;

    char *name;
    char *sig;
    char *gsig;
    error = jvmti->GetMethodName(method, &name, &sig, &gsig);
    check_jvmti_error(jvmti, error, "Unable to get method information.");

    printf("Cought exception in Method:%s%s\n", name, sig);

    /* Exception Class name */
    // Get the exception class
    jclass exceptionClass = jni->GetObjectClass(exception);

    // Get the class object's class descriptor
    jclass classType = jni->GetObjectClass(exceptionClass);

    // Find the getSimpleName() method in the class object
    jmethodID methodId = jni->GetMethodID(classType, "getSimpleName", "()Ljava/lang/String;");
    jstring className = (jstring) jni->CallObjectMethod(exceptionClass, methodId);

    // Convert to native string
    const char *nativeString = jni->GetStringUTFChars(className, 0);

    // Use your class name string
    printf("Exception type:%s\n", nativeString);

    // And finally, release the JNI objects after usage
    jni->ReleaseStringUTFChars(className, nativeString);
    jni->DeleteLocalRef(classType);
    jni->DeleteLocalRef(exceptionClass);

    /* Stack trace */
    int depth = 5;
    jvmtiFrameInfo frames[depth];
    jint count;

    error = jvmti->GetStackTrace(thread, 0, depth, (jvmtiFrameInfo *) &frames, &count);
    check_jvmti_error(jvmti, error, "Unable to get stack trace.");

    printf("Exception Stack Trace\n");
    printf("=====================\n");
    printf("Stack Trace Depth: %d\n", count);

    char *methodName = "yet_to_call()";
    char *declaringClassName;
    jclass declaringClass;

    for (int i=0; i < count; i++) {
        error = jvmti->GetMethodName(frames[i].method, &methodName, NULL, NULL);

        if (error == JVMTI_ERROR_NONE) {
            error = jvmti->GetMethodDeclaringClass(frames[i].method, &declaringClass);
            error = jvmti->GetClassSignature(declaringClass, &declaringClassName, NULL);

            if (error == JVMTI_ERROR_NONE) {
                printf("at method %s() in class %s\n", methodName, declaringClassName);
            }
        }
    }
    printf("=====================\n");
}

static void JNICALL ThreadStartCallback(jvmtiEnv *jvmti,
                    JNIEnv *env,
                    jthread thread) {
    enter_critical_section(jvmti);
    {
        /* It's possible we get here right after VmDeath event, be careful */
        if (!gdata->vm_is_dead) {
            char tname[MAX_THREAD_NAME_LENGTH];

            get_thread_name(jvmti, thread, tname, sizeof(tname));
            stdout_message("ThreadStart %s\n", tname);
        }
    }
    exit_critical_section(jvmti);
}


static void JNICALL ThreadEndCallback(jvmtiEnv *jvmti,
                  JNIEnv *env,
                  jthread thread) {
    enter_critical_section(jvmti);
    {
        /* It's possible we get here right after VmDeath event, be careful */
        if (!gdata->vm_is_dead) {
            char tname[MAX_THREAD_NAME_LENGTH];

            get_thread_name(jvmti, thread, tname, sizeof(tname));
            stdout_message("ThreadEnd %s\n", tname);
        }
    }
    exit_critical_section(jvmti);
}


static void JNICALL ResourceExhaustedCallback(jvmtiEnv *jvmti,
                          JNIEnv *env,
                          jint flags,
                          const void *reserved,
                          const char *description) {
    enter_critical_section(jvmti);
    {
        /* It's possible we get here right after VmDeath event, be careful */
        if (!gdata->vm_is_dead) {
            switch (flags) {
                case JVMTI_RESOURCE_EXHAUSTED_OOM_ERROR:
                    stdout_message("Out Of Memory Error, %s\n", description);
                    break;
                case JVMTI_RESOURCE_EXHAUSTED_JAVA_HEAP:
                    stdout_message("Exhausted Java Heap, %s\n", description);
                    break;
                case JVMTI_RESOURCE_EXHAUSTED_THREADS:
                    stdout_message("Exhausted threads, %s\n", description);
                    break;
                default:
                    stdout_message("Unknown, %s\n", description);
                    break;
            }
        }
    }
    exit_critical_section(jvmti);
}

/* ------------------------------------------------------------------- */
/* Generic JVMTI utility functions */

/* Every JVMTI interface returns an error code, which should be checked
 *   to avoid any cascading errors down the line.
 *   The interface GetErrorName() returns the actual enumeration constant
 *   name, making the error messages much easier to understand.
 */
void check_jvmti_error(jvmtiEnv *jvmti, jvmtiError errnum, const char *str) {
    if (errnum != JVMTI_ERROR_NONE) {
        char *errnum_str = NULL;
        jvmti->GetErrorName(errnum, &errnum_str);

        fatal_error("ERROR: JVMTI: %d(%s): %s\n", errnum,
                    (errnum_str == NULL ? "Unknown" : errnum_str),
                    (str == NULL ? "" : str));
    }
}

/* All memory allocated by JVMTI must be freed by the JVMTI Deallocate
 *   interface.
 */
void deallocate(jvmtiEnv *jvmti, void *ptr) {
    jvmtiError error;

    error = jvmti->Deallocate((unsigned char *) ptr);
    check_jvmti_error(jvmti, error, "Cannot deallocate memory");
}

/* Allocation of JVMTI managed memory */
void * allocate(jvmtiEnv *jvmti, jint len) {
    jvmtiError error;
    void *ptr;

    error = jvmti->Allocate(len, (unsigned char **) &ptr);
    check_jvmti_error(jvmti, error, "Cannot allocate memory");
    return ptr;
}

/* Get a name for a jthread */
void static get_thread_name(jvmtiEnv *jvmti, jthread thread, char *tname, int maxlen) {
    jvmtiThreadInfo info;
    jvmtiError error;

    /* Make sure the stack variables are garbage free */
    (void) memset(&info, 0, sizeof(info));

    /* Assume the name is unknown for now */
    (void) strcpy(tname, "Unknown");

    /* Get the thread information, which includes the name */
    error = jvmti->GetThreadInfo(thread, &info);
    check_jvmti_error(jvmti, error, "Cannot get thread info");

    /* The thread might not have a name, be careful here. */
    if (info.name != NULL) {
        int len;

        /* Copy the thread name into tname if it will fit */
        len = (int) strlen(info.name);
        if (len < maxlen) {
            (void) strcpy(tname, info.name);
        }

        /* Every string allocated by JVMTI needs to be freed */
        deallocate(jvmti, (void *) info.name);
    }
}

/* Enter a critical section by doing a JVMTI Raw Monitor Enter */
static void enter_critical_section(jvmtiEnv *jvmti) {
    jvmtiError error;

    error = jvmti->RawMonitorEnter(gdata->lock);
    check_jvmti_error(jvmti, error, "Cannot enter with raw monitor");
}

/* Exit a critical section by doing a JVMTI Raw Monitor Exit */
static void exit_critical_section(jvmtiEnv *jvmti) {
    jvmtiError error;

    error = jvmti->RawMonitorExit(gdata->lock);
    check_jvmti_error(jvmti, error, "Cannot exit with raw monitor");
}

void stdout_message(const char *format, ...) {
    va_list ap;

    va_start(ap, format);
    (void) vfprintf(stdout, format, ap);
    va_end(ap);
}

void fatal_error(const char *format, ...) {
    va_list ap;

    va_start(ap, format);
    (void) vfprintf(stderr, format, ap);
    (void) fflush(stderr);
    va_end(ap);
    exit(3);
}