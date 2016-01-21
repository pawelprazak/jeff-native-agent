#include "jvmti.hpp"
#include "message.hpp"

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
void *allocate(jvmtiEnv *jvmti, jint len) {
    void *ptr;

    jvmtiError error = jvmti->Allocate(len, (unsigned char **) &ptr);
    check_jvmti_error(jvmti, error, "Cannot allocate memory");
    return ptr;
}
