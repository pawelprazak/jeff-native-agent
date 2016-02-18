#include "Object.hpp"
#include "Type.hpp"
#include "jni.hpp"
#include "jvmti.hpp"

using namespace std;
using namespace boost;
using namespace jeff;

Object::Object() : type(nullptr), to_string(wstring()) {
    // Empty
}

Object::Object(const Type *const type, std::wstring to_string)
        : type(type), to_string(to_string) {
    // Empty
}

Object::~Object() {
    delete type;
}

const Type Object::getType() const {
    return *type;
}

const std::wstring Object::toString() const {
    return to_string;
}

unique_ptr<Object> Object::from(jvmtiEnv &jvmti, JNIEnv &jni, jarray array) {
    jclass type = get_object_class(jni, array);
    std::string signature = get_class_signature(jvmti, type);

    // TODO
    wstring as_string = L"[not implemented yet]";

    Object *ret = new Object(Type::from(jvmti, jni, signature), as_string);
    jni.DeleteLocalRef(type);
    return unique_ptr<Object>(ret);
}

unique_ptr<Object> Object::from(jvmtiEnv &jvmti, JNIEnv &jni, jobject object) {
    jclass type = get_object_class(jni, object);

//    std::function<wstring(jobject)> wstring_transformer = [jni](jobject result) mutable {
//        return (result == nullptr) ? L"" : jeff::to_wstring(jni, static_cast<jstring>(result));
//    };
//    wstring as_string = call_method(jni, object, "toString", "()Ljava/lang/String;", wstring_transformer);
//    wstring as_string = call_method(jni, object, "toString", "()Ljava/lang/String;", wstring_transformer(jni));

    jobject result = call_method(jni, object, "toString", "()Ljava/lang/String;");
    wstring as_string = (result == nullptr) ? L"" : jeff::to_wstring(jni, static_cast<jstring>(result));

    std::string signature = get_class_signature(jvmti, type);
    auto theType = Type::from(jvmti, jni, signature);
    Object *ret = new Object(theType, as_string);
    jni.DeleteLocalRef(type);
    return unique_ptr<Object>(ret);
}

unique_ptr<Object> Object::from(jvmtiEnv &jvmti, JNIEnv &jni, bool value) {
    return unique_ptr<Object>(new Object(Type::from(jvmti, jni, 'Z'), (value == 0) ? L"false" : L"true"));
}

unique_ptr<Object> Object::from(jvmtiEnv &jvmti, JNIEnv &jni, jchar value) {
    return unique_ptr<Object>(new Object(Type::from(jvmti, jni, 'C'), to_wstring(value)));
}

unique_ptr<Object> Object::from(jvmtiEnv &jvmti, JNIEnv &jni, jbyte value) {
    return unique_ptr<Object>(new Object(Type::from(jvmti, jni, 'B'), to_wstring(value)));
}

unique_ptr<Object> Object::from(jvmtiEnv &jvmti, JNIEnv &jni, jshort value) {
    return unique_ptr<Object>(new Object(Type::from(jvmti, jni, 'S'), to_wstring(value)));
}

unique_ptr<Object> Object::from(jvmtiEnv &jvmti, JNIEnv &jni, jint value) {
    return unique_ptr<Object>(new Object(Type::from(jvmti, jni, 'I'), to_wstring(value)));
}

unique_ptr<Object> Object::from(jvmtiEnv &jvmti, JNIEnv &jni, jlong value) {
    return unique_ptr<Object>(new Object(Type::from(jvmti, jni, 'J'), to_wstring(value)));
}

unique_ptr<Object> Object::from(jvmtiEnv &jvmti, JNIEnv &jni, jfloat value) {
    return unique_ptr<Object>(new Object(Type::from(jvmti, jni, 'F'), to_wstring(value)));
}

unique_ptr<Object> Object::from(jvmtiEnv &jvmti, JNIEnv &jni, jdouble value) {
    return unique_ptr<Object>(new Object(Type::from(jvmti, jni, 'D'), to_wstring(value)));
}
