/*
 * Copyright 2016, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "wifi"

#include "jni.h"
#include <ScopedUtfChars.h>
#include <utils/misc.h>
#include <android_runtime/AndroidRuntime.h>
#include <utils/Log.h>
#include <utils/String16.h>

#include "wifi.h"
#include "wifi_hal.h"
#include "jni_helper.h"

namespace android {

/* JNI Helpers for wifi_hal implementation */

JNIHelper::JNIHelper(JavaVM *vm)
{
    vm->AttachCurrentThread(&mEnv, NULL);
    mVM = vm;
}

JNIHelper::JNIHelper(JNIEnv *env)
{
    mVM  = NULL;
    mEnv = env;
}

JNIHelper::~JNIHelper()
{
    if (mVM != NULL) {
        // mVM->DetachCurrentThread();  /* 'attempting to detach while still running code' */
        mVM = NULL;                     /* not really required; but may help debugging */
        mEnv = NULL;                    /* not really required; but may help debugging */
    }
}

jobject JNIHelper::newGlobalRef(jobject obj) {
    return mEnv->NewGlobalRef(obj);
}

void JNIHelper::deleteGlobalRef(jobject obj) {
    mEnv->DeleteGlobalRef(obj);
}

jobject JNIHelper::newLocalRef(jobject obj) {
    return mEnv->NewLocalRef(obj);
}

void JNIHelper::deleteLocalRef(jobject obj) {
    mEnv->DeleteLocalRef(obj);
}

void JNIHelper::throwException(const char *message, int line)
{
    ALOGE("error at line %d: %s", line, message);

    const char *className = "java/lang/Exception";

    jclass exClass = mEnv->FindClass(className );
    if ( exClass == NULL ) {
        ALOGE("Could not find exception class to throw error");
        ALOGE("error at line %d: %s", line, message);
        return;
    }

    mEnv->ThrowNew(exClass, message);
}

jboolean JNIHelper::getBoolField(jobject obj, const char *name)
{
    JNIObject<jclass> cls(*this, mEnv->GetObjectClass(obj));
    jfieldID field = mEnv->GetFieldID(cls, name, "Z");
    if (field == 0) {
        THROW(*this, "Error in accessing field");
        return 0;
    }

    return mEnv->GetBooleanField(obj, field);
}

jint JNIHelper::getIntField(jobject obj, const char *name)
{
    JNIObject<jclass> cls(*this, mEnv->GetObjectClass(obj));
    jfieldID field = mEnv->GetFieldID(cls, name, "I");
    if (field == 0) {
        THROW(*this, "Error in accessing field");
        return 0;
    }

    return mEnv->GetIntField(obj, field);
}

jbyte JNIHelper::getByteField(jobject obj, const char *name)
{
    JNIObject<jclass> cls(*this, mEnv->GetObjectClass(obj));
    jfieldID field = mEnv->GetFieldID(cls, name, "B");
    if (field == 0) {
        THROW(*this, "Error in accessing field");
        return 0;
    }

    return mEnv->GetByteField(obj, field);
}

jlong JNIHelper::getLongField(jobject obj, const char *name)
{
    JNIObject<jclass> cls(*this, mEnv->GetObjectClass(obj));
    jfieldID field = mEnv->GetFieldID(cls, name, "J");
    if (field == 0) {
        THROW(*this, "Error in accessing field");
        return 0;
    }

    return mEnv->GetLongField(obj, field);
}

JNIObject<jstring> JNIHelper::getStringField(jobject obj, const char *name)
{
    JNIObject<jobject> m = getObjectField(obj, name, "Ljava/lang/String;");
    if (m == NULL) {
        THROW(*this, "Error in accessing field");
        return JNIObject<jstring>(*this, NULL);
    }

    return JNIObject<jstring>(*this, (jstring)m.detach());
}

bool JNIHelper::getStringFieldValue(jobject obj, const char *name, char *buf, int size)
{
    JNIObject<jclass> cls(*this, mEnv->GetObjectClass(obj));
    jfieldID field = mEnv->GetFieldID(cls, name, "Ljava/lang/String;");
    if (field == 0) {
        THROW(*this, "Error in accessing field");
        return 0;
    }

    JNIObject<jobject> value(*this, mEnv->GetObjectField(obj, field));
    JNIObject<jstring> string(*this, (jstring)value.clone());
    ScopedUtfChars chars(mEnv, string);

    const char *utf = chars.c_str();
    if (utf == NULL) {
        THROW(*this, "Error in accessing value");
        return false;
    }

    if (*utf != 0 && size < 1) {
        return false;
    }

    strncpy(buf, utf, size);
    if (size > 0) {
        buf[size - 1] = 0;
    }

    return true;
}

jlong JNIHelper::getStaticLongField(jobject obj, const char *name)
{
    JNIObject<jclass> cls(*this, mEnv->GetObjectClass(obj));
    return getStaticLongField(cls, name);
}

jlong JNIHelper::getStaticLongField(jclass cls, const char *name)
{
    jfieldID field = mEnv->GetStaticFieldID(cls, name, "J");
    if (field == 0) {
        THROW(*this, "Error in accessing field");
        return 0;
    }
    //ALOGE("getStaticLongField %s %p", name, cls);
    return mEnv->GetStaticLongField(cls, field);
}

JNIObject<jobject> JNIHelper::getObjectField(jobject obj, const char *name, const char *type)
{
    JNIObject<jclass> cls(*this, mEnv->GetObjectClass(obj));
    jfieldID field = mEnv->GetFieldID(cls, name, type);
    if (field == 0) {
        THROW(*this, "Error in accessing field");
        return JNIObject<jobject>(*this, NULL);
    }

    return JNIObject<jobject>(*this, mEnv->GetObjectField(obj, field));
}

JNIObject<jobjectArray> JNIHelper::getArrayField(jobject obj, const char *name, const char *type)
{
    JNIObject<jclass> cls(*this, mEnv->GetObjectClass(obj));
    jfieldID field = mEnv->GetFieldID(cls, name, type);
    if (field == 0) {
        THROW(*this, "Error in accessing field");
        return JNIObject<jobjectArray>(*this, NULL);
    }

    return JNIObject<jobjectArray>(*this, (jobjectArray)mEnv->GetObjectField(obj, field));
}

jlong JNIHelper::getLongArrayField(jobject obj, const char *name, int index)
{
    JNIObject<jclass> cls(*this, mEnv->GetObjectClass(obj));
    jfieldID field = mEnv->GetFieldID(cls, name, "[J");
    if (field == 0) {
        THROW(*this, "Error in accessing field definition");
        return 0;
    }

    JNIObject<jlongArray> array(*this, (jlongArray)mEnv->GetObjectField(obj, field));
    if (array == NULL) {
        THROW(*this, "Error in accessing array");
        return 0;
    }

    jlong *elem = mEnv->GetLongArrayElements(array, 0);
    if (elem == NULL) {
        THROW(*this, "Error in accessing index element");
        return 0;
    }

    jlong value = elem[index];
    mEnv->ReleaseLongArrayElements(array, elem, 0);
    return value;
}

void JNIHelper::getByteArrayField(jobject obj, const char *name, byte* buf, int size) {
    JNIObject<jclass> cls(*this, mEnv->GetObjectClass(obj));
    jfieldID field = mEnv->GetFieldID(cls, name, "[B");
    if (field == 0) {
        THROW(*this, "Error in accessing field definition");
        return;
    }

    JNIObject<jbyteArray> array(*this, (jbyteArray)mEnv->GetObjectField(obj, field));
    if (array == NULL) {
        THROW(*this, "Error in accessing array");
        return;
    }

    jbyte *elem = mEnv->GetByteArrayElements(array, 0);
    if (elem == NULL) {
        THROW(*this, "Error in accessing index element");
        return;
    }

    memcpy(buf, elem, size);
    mEnv->ReleaseByteArrayElements(array, elem, 0);
}

jlong JNIHelper::getStaticLongArrayField(jobject obj, const char *name, int index)
{
    JNIObject<jclass> cls(*this, mEnv->GetObjectClass(obj));
    return getStaticLongArrayField(cls, name, index);
}

jlong JNIHelper::getStaticLongArrayField(jclass cls, const char *name, int index)
{
    jfieldID field = mEnv->GetStaticFieldID(cls, name, "[J");
    if (field == 0) {
        THROW(*this, "Error in accessing field definition");
        return 0;
    }

    JNIObject<jlongArray> array(*this, (jlongArray)mEnv->GetStaticObjectField(cls, field));
    jlong *elem = mEnv->GetLongArrayElements(array, 0);
    if (elem == NULL) {
        THROW(*this, "Error in accessing index element");
        return 0;
    }

    jlong value = elem[index];
    mEnv->ReleaseLongArrayElements(array, elem, 0);
    return value;
}

JNIObject<jobject> JNIHelper::getObjectArrayField(jobject obj, const char *name, const char *type,
int index)
{
    JNIObject<jclass> cls(*this, mEnv->GetObjectClass(obj));
    jfieldID field = mEnv->GetFieldID(cls, name, type);
    if (field == 0) {
        THROW(*this, "Error in accessing field definition");
        return JNIObject<jobject>(*this, NULL);
    }

    JNIObject<jobjectArray> array(*this, (jobjectArray)mEnv->GetObjectField(obj, field));
    JNIObject<jobject> elem(*this, mEnv->GetObjectArrayElement(array, index));
    if (elem.isNull()) {
        THROW(*this, "Error in accessing index element");
        return JNIObject<jobject>(*this, NULL);
    }
    return elem;
}

void JNIHelper::setIntField(jobject obj, const char *name, jint value)
{
    JNIObject<jclass> cls(*this, mEnv->GetObjectClass(obj));
    if (cls == NULL) {
        THROW(*this, "Error in accessing class");
        return;
    }

    jfieldID field = mEnv->GetFieldID(cls, name, "I");
    if (field == NULL) {
        THROW(*this, "Error in accessing field");
        return;
    }

    mEnv->SetIntField(obj, field, value);
}

void JNIHelper::setByteField(jobject obj, const char *name, jbyte value)
{
    JNIObject<jclass> cls(*this, mEnv->GetObjectClass(obj));
    if (cls == NULL) {
        THROW(*this, "Error in accessing class");
        return;
    }

    jfieldID field = mEnv->GetFieldID(cls, name, "B");
    if (field == NULL) {
        THROW(*this, "Error in accessing field");
        return;
    }

    mEnv->SetByteField(obj, field, value);
}

void JNIHelper::setBooleanField(jobject obj, const char *name, jboolean value)
{
    JNIObject<jclass> cls(*this, mEnv->GetObjectClass(obj));
    if (cls == NULL) {
        THROW(*this, "Error in accessing class");
        return;
    }

    jfieldID field = mEnv->GetFieldID(cls, name, "Z");
    if (field == NULL) {
        THROW(*this, "Error in accessing field");
        return;
    }

    mEnv->SetBooleanField(obj, field, value);
}

void JNIHelper::setLongField(jobject obj, const char *name, jlong value)
{
    JNIObject<jclass> cls(*this, mEnv->GetObjectClass(obj));
    if (cls == NULL) {
        THROW(*this, "Error in accessing class");
        return;
    }

    jfieldID field = mEnv->GetFieldID(cls, name, "J");
    if (field == NULL) {
        THROW(*this, "Error in accessing field");
        return;
    }

    mEnv->SetLongField(obj, field, value);
}

void JNIHelper::setStaticLongField(jobject obj, const char *name, jlong value)
{
    JNIObject<jclass> cls(*this, mEnv->GetObjectClass(obj));
    if (cls == NULL) {
        THROW(*this, "Error in accessing class");
        return;
    }

    setStaticLongField(cls, name, value);
}

void JNIHelper::setStaticLongField(jclass cls, const char *name, jlong value)
{
    jfieldID field = mEnv->GetStaticFieldID(cls, name, "J");
    if (field == NULL) {
        THROW(*this, "Error in accessing field");
        return;
    }

    mEnv->SetStaticLongField(cls, field, value);
}

void JNIHelper::setLongArrayField(jobject obj, const char *name, jlongArray value)
{
    JNIObject<jclass> cls(*this, mEnv->GetObjectClass(obj));
    if (cls == NULL) {
        THROW(*this, "Error in accessing field");
        return;
    }

    jfieldID field = mEnv->GetFieldID(cls, name, "[J");
    if (field == NULL) {
        THROW(*this, "Error in accessing field");
        return;
    }

    mEnv->SetObjectField(obj, field, value);
}

void JNIHelper::setStaticLongArrayField(jobject obj, const char *name, jlongArray value)
{
    JNIObject<jclass> cls(*this, mEnv->GetObjectClass(obj));
    if (cls == NULL) {
        THROW(*this, "Error in accessing field");
        return;
    }

    setStaticLongArrayField(cls, name, value);
}

void JNIHelper::setStaticLongArrayField(jclass cls, const char *name, jlongArray value)
{
    jfieldID field = mEnv->GetStaticFieldID(cls, name, "[J");
    if (field == NULL) {
        THROW(*this, "Error in accessing field");
        return;
    }

    mEnv->SetStaticObjectField(cls, field, value);
    ALOGD("array field set");
}

void JNIHelper::setLongArrayElement(jobject obj, const char *name, int index, jlong value)
{
    JNIObject<jclass> cls(*this, mEnv->GetObjectClass(obj));
    if (cls == NULL) {
        THROW(*this, "Error in accessing field");
        return;
    }

    jfieldID field = mEnv->GetFieldID(cls, name, "[J");
    if (field == NULL) {
        THROW(*this, "Error in accessing field");
        return;
    }

    JNIObject<jlongArray> array(*this, (jlongArray)mEnv->GetObjectField(obj, field));
    if (array == NULL) {
        THROW(*this, "Error in accessing array");
        return;
    }

    jlong *elem = mEnv->GetLongArrayElements(array, NULL);
    if (elem == NULL) {
        THROW(*this, "Error in accessing index element");
        return;
    }

    elem[index] = value;
    mEnv->ReleaseLongArrayElements(array, elem, 0);
}

void JNIHelper::setObjectField(jobject obj, const char *name, const char *type, jobject value)
{
    JNIObject<jclass> cls(*this, mEnv->GetObjectClass(obj));
    if (cls == NULL) {
        THROW(*this, "Error in accessing class");
        return;
    }

    jfieldID field = mEnv->GetFieldID(cls, name, type);
    if (field == NULL) {
        THROW(*this, "Error in accessing field");
        return;
    }

    mEnv->SetObjectField(obj, field, value);
}

jboolean JNIHelper::setStringField(jobject obj, const char *name, const char *value)
{
    JNIObject<jstring> str(*this, mEnv->NewStringUTF(value));

    if (mEnv->ExceptionCheck()) {
        mEnv->ExceptionDescribe();
        mEnv->ExceptionClear();
        return false;
    }

    if (str == NULL) {
        THROW(*this, "Error creating string");
        return false;
    }

    setObjectField(obj, name, "Ljava/lang/String;", str);
    return true;
}

void JNIHelper::reportEvent(jclass cls, const char *method, const char *signature, ...)
{
    va_list params;
    va_start(params, signature);

    jmethodID methodID = mEnv->GetStaticMethodID(cls, method, signature);
    if (methodID == 0) {
        ALOGE("Error in getting method ID");
        return;
    }

    mEnv->CallStaticVoidMethodV(cls, methodID, params);
    if (mEnv->ExceptionCheck()) {
        mEnv->ExceptionDescribe();
        mEnv->ExceptionClear();
    }

    va_end(params);
}

void JNIHelper::callMethod(jobject obj, const char *method, const char *signature, ...)
{
    va_list params;
    va_start(params, signature);

    jclass cls = mEnv->GetObjectClass(obj);
    jmethodID methodID = mEnv->GetMethodID(cls, method, signature);
    if (methodID == 0) {
        ALOGE("Error in getting method ID");
        return;
    }

    mEnv->CallVoidMethodV(obj, methodID, params);
    if (mEnv->ExceptionCheck()) {
        mEnv->ExceptionDescribe();
        mEnv->ExceptionClear();
    }

    va_end(params);
}

jboolean JNIHelper::callStaticMethod(jclass cls, const char *method, const char *signature, ...)
{
    va_list params;
    va_start(params, signature);

    jmethodID methodID = mEnv->GetStaticMethodID(cls, method, signature);
    if (methodID == 0) {
        ALOGE("Error in getting method ID");
        return false;
    }

    jboolean result = mEnv->CallStaticBooleanMethodV(cls, methodID, params);
    if (mEnv->ExceptionCheck()) {
        mEnv->ExceptionDescribe();
        mEnv->ExceptionClear();
        return false;
    }

    va_end(params);
    return result;
}

JNIObject<jobject> JNIHelper::createObject(const char *className) {
    return createObjectWithArgs(className, "()V");
}

JNIObject<jobject> JNIHelper::createObjectWithArgs(
    const char *className, const char *signature, ...)
{
    va_list params;
    va_start(params, signature);

    JNIObject<jclass> cls(*this, mEnv->FindClass(className));
    if (cls == NULL) {
        ALOGE("Error in finding class %s", className);
        return JNIObject<jobject>(*this, NULL);
    }

    jmethodID constructor = mEnv->GetMethodID(cls, "<init>", signature);
    if (constructor == 0) {
        ALOGE("Error in constructor ID for %s", className);
        return JNIObject<jobject>(*this, NULL);
    }

    JNIObject<jobject> obj(*this, mEnv->NewObjectV(cls, constructor, params));
    if (obj == NULL) {
        ALOGE("Could not create new object of %s", className);
        return JNIObject<jobject>(*this, NULL);
    }

    va_end(params);
    return obj;
}

JNIObject<jobjectArray> JNIHelper::createObjectArray(const char *className, int num)
{
    JNIObject<jclass> cls(*this, mEnv->FindClass(className));
    if (cls == NULL) {
        ALOGE("Error in finding class %s", className);
        return JNIObject<jobjectArray>(*this, NULL);
    }

    JNIObject<jobject> array(*this, mEnv->NewObjectArray(num, cls.get(), NULL));
    if (array.get() == NULL) {
        ALOGE("Error in creating array of class %s", className);
        return JNIObject<jobjectArray>(*this, NULL);
    }

    return JNIObject<jobjectArray>(*this, (jobjectArray)array.detach());
}

JNIObject<jobject> JNIHelper::getObjectArrayElement(jobjectArray array, int index)
{
    return JNIObject<jobject>(*this, mEnv->GetObjectArrayElement(array, index));
}

JNIObject<jobject> JNIHelper::getObjectArrayElement(jobject array, int index)
{
    return getObjectArrayElement((jobjectArray)array, index);
}

int JNIHelper::getArrayLength(jarray array) {
    return mEnv->GetArrayLength(array);
}

JNIObject<jobjectArray> JNIHelper::newObjectArray(int num, const char *className, jobject val) {
    JNIObject<jclass> cls(*this, mEnv->FindClass(className));
    if (cls == NULL) {
        ALOGE("Error in finding class %s", className);
        return JNIObject<jobjectArray>(*this, NULL);
    }

    return JNIObject<jobjectArray>(*this, mEnv->NewObjectArray(num, cls, val));
}

JNIObject<jbyteArray> JNIHelper::newByteArray(int num) {
    return JNIObject<jbyteArray>(*this, mEnv->NewByteArray(num));
}

JNIObject<jintArray> JNIHelper::newIntArray(int num) {
    return JNIObject<jintArray>(*this, mEnv->NewIntArray(num));
}

JNIObject<jlongArray> JNIHelper::newLongArray(int num) {
    return JNIObject<jlongArray>(*this, mEnv->NewLongArray(num));
}

JNIObject<jstring> JNIHelper::newStringUTF(const char *utf) {
    return JNIObject<jstring>(*this, mEnv->NewStringUTF(utf));
}

void JNIHelper::setObjectArrayElement(jobjectArray array, int index, jobject obj) {
    mEnv->SetObjectArrayElement(array, index, obj);
}

void JNIHelper::setByteArrayRegion(jbyteArray array, int from, int to, const jbyte *bytes) {
    mEnv->SetByteArrayRegion(array, from, to, bytes);
}

void JNIHelper::setIntArrayRegion(jintArray array, int from, int to, const jint *ints) {
    mEnv->SetIntArrayRegion(array, from, to, ints);
}

void JNIHelper::setLongArrayRegion(jlongArray array, int from, int to, const jlong *longs) {
    mEnv->SetLongArrayRegion(array, from, to, longs);
}

}; // namespace android


