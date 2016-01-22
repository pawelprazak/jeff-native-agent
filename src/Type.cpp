#include <sstream>
#include "jvmti.hpp"
#include "Type.hpp"

Type::Type(jvmtiEnv &jvmti, JNIEnv &jni, jclass type)
        : Object(jvmti, jni, type), jvmti(jvmti), jni(jni), type(type) {

}

Type::~Type() {
    jni.DeleteLocalRef(type);
}

const std::string Type::getSignature() const {
    char *signature;
    jvmtiError error = jvmti.GetClassSignature(type, &signature, NULL);
    check_jvmti_error(jvmti, error, "Unable to get class signature.");

    std::stringstream stream;
    stream << signature;
    deallocate(jvmti, signature);

    return stream.str();
}
