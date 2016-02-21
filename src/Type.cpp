#include <sstream>

#include "jvmti.hpp"

#include "Type.hpp"

using namespace jeff;

Type::Type() {
    // Empty
}

Type::Type(const std::string signature)
        : signature(signature) {
    // Empty
}

Type::~Type() {
    // Empty
}

const std::string Type::getSignature() const {
    return signature;
}

const Type *const Type::from(jvmtiEnv &jvmti, JNIEnv &jni, std::string signature) {
    return new Type(signature);
}

const Type *const Type::from(jvmtiEnv &jvmti, JNIEnv &jni, char primitive_signature) {
    return new Type(std::string(1, primitive_signature));
}
