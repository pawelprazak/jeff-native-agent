#ifndef JEFF_JVMTI_H
#define JEFF_JVMTI_H

#include "jvmti.h"

void *allocate(jvmtiEnv *jvmti, jint len);

void deallocate(jvmtiEnv *jvmti, void *ptr);

void check_jvmti_error(jvmtiEnv *jvmti, jvmtiError errnum, const char *str);

#endif // JEFF_JVMTI_H