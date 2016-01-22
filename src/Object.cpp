#include "Object.hpp"
#include "Type.hpp"
#include "jni.hpp"

using namespace boost;

Object::Object(jvmtiEnv &jvmti, JNIEnv &jni, jobject object)
        : jvmti(jvmti), jni(jni), object(object) {
    type = get_object_class(jni, object);
}

Object::~Object() {
    jni.DeleteLocalRef(type);
    jni.DeleteLocalRef(object);
}

const Type Object::getType() const {
    return Type(jvmti, jni, type);
}
