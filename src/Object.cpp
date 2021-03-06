#include "Object.hpp"

#include "common.hpp"
#include "jni.hpp"
#include "jvmti.hpp"
#include "Type.hpp"

using namespace std;
using namespace boost;
using namespace jeff;

Object::Object() : type(nullptr), to_string(string()) {
    // Empty
}

Object::Object(const Type *const type, std::string to_string)
        : type(type), to_string(to_string) {
    // Empty
}

Object::~Object() {
    delete type;
}

const Type Object::getType() const {
    return *type;
}

const string Object::toString() const {
    return to_string;
}

unique_ptr<Object> Object::from(jvmtiEnv &jvmti, JNIEnv &jni, jbooleanArray array, string signature) {
    list<bool> values = to_list(jni, array);
    string as_string = join(values, [](string a, string b) -> string { return a + ", " + b; });

    Object *ret = new Object(Type::from(jvmti, jni, signature), as_string);
    return unique_ptr<Object>(ret);
}

unique_ptr<Object> Object::from(jvmtiEnv &jvmti, JNIEnv &jni, jarray array, string signature) {
    // TODO
    string as_string = "[not implemented yet]";

    Object *ret = new Object(Type::from(jvmti, jni, signature), as_string);
    return unique_ptr<Object>(ret);
}

unique_ptr<Object> Object::from(jvmtiEnv &jvmti, JNIEnv &jni, jobject object) {
    jclass type = get_object_class(jni, object);

    jobject result = call_method(jni, object, "toString", "()Ljava/lang/String;");
    string as_string = (result == nullptr) ? "" : jeff::to_string(jni, static_cast<jstring>(result));

    std::string signature = get_class_signature(jvmti, type);
    Object *ret = new Object(Type::from(jvmti, jni, signature), as_string);
    jni.DeleteLocalRef(type);
    return unique_ptr<Object>(ret);
}

unique_ptr<Object> Object::from(jvmtiEnv &jvmti, JNIEnv &jni, bool value) {
    return unique_ptr<Object>(new Object(Type::from(jvmti, jni, 'Z'), (value == 0) ? "false" : "true"));
}

unique_ptr<Object> Object::from(jvmtiEnv &jvmti, JNIEnv &jni, jchar value) {
    return unique_ptr<Object>(new Object(Type::from(jvmti, jni, 'C'), std::to_string(value)));
}

unique_ptr<Object> Object::from(jvmtiEnv &jvmti, JNIEnv &jni, jbyte value) {
    return unique_ptr<Object>(new Object(Type::from(jvmti, jni, 'B'), std::to_string(value)));
}

unique_ptr<Object> Object::from(jvmtiEnv &jvmti, JNIEnv &jni, jshort value) {
    return unique_ptr<Object>(new Object(Type::from(jvmti, jni, 'S'), std::to_string(value)));
}

unique_ptr<Object> Object::from(jvmtiEnv &jvmti, JNIEnv &jni, jint value) {
    return unique_ptr<Object>(new Object(Type::from(jvmti, jni, 'I'), std::to_string(value)));
}

unique_ptr<Object> Object::from(jvmtiEnv &jvmti, JNIEnv &jni, jlong value) {
    return unique_ptr<Object>(new Object(Type::from(jvmti, jni, 'J'), std::to_string(value)));
}

unique_ptr<Object> Object::from(jvmtiEnv &jvmti, JNIEnv &jni, jfloat value) {
    return unique_ptr<Object>(new Object(Type::from(jvmti, jni, 'F'), std::to_string(value)));
}

unique_ptr<Object> Object::from(jvmtiEnv &jvmti, JNIEnv &jni, jdouble value) {
    return unique_ptr<Object>(new Object(Type::from(jvmti, jni, 'D'), std::to_string(value)));
}