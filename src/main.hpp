#ifndef JEFF_MAIN_H
#define JEFF_MAIN_H

#include "jvmti.h"

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

/**
 * The VM will start the agent by calling this function.
 */
JNIEXPORT jint JNICALL
Agent_OnLoad(JavaVM *jvm, char *options, void *reserved);

/**
 * A VM may support a mechanism that allows agents to be started in the VM during the live phase.
 * Note that some capabilities may not be available in the live phase.
 */
JNIEXPORT jint JNICALL
Agent_OnAttach(JavaVM *jvm, char *options, void *reserved);

/**
 * This function will be called by the VM when the library is about to be unloaded.
 * This function can be used to clean-up resources allocated by the agent.
 */
JNIEXPORT void JNICALL
Agent_OnUnload(JavaVM *jvm);

/**
 * Main entry points
 */
 
static jint init(JavaVM *jvm, char *options);

static jint live(jvmtiEnv *jvmti);

/**
 * JVMTI calback functions
 */
 
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

static void JNICALL ThreadStartCallback(jvmtiEnv *jvmti, JNIEnv *env, jthread thread);

static void JNICALL ThreadEndCallback(jvmtiEnv *jvmti, JNIEnv *env, jthread thread);

static void JNICALL ResourceExhaustedCallback(jvmtiEnv *jvmti, JNIEnv *env, jint flags,
                                              const void *reserved, const char *description);
                                              
/* Generic JVMTI utility functions */

static void enter_critical_section(jvmtiEnv *jvmti);

static void exit_critical_section(jvmtiEnv *jvmti);

#endif // JEFF_MAIN_H