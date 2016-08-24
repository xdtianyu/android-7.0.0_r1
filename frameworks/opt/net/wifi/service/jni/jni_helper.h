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

namespace android {

/* JNI Helpers for wifi_hal to WifiNative bridge implementation */

class JNIHelper;

template<typename T>
class JNIObject {
protected:
    JNIHelper &mHelper;
    T mObj;
public:
    JNIObject(JNIHelper &helper, T obj);
    JNIObject(const JNIObject<T>& rhs);
    virtual ~JNIObject();
    JNIHelper& getHelper() const {
        return mHelper;
    }
    T get() const {
        return mObj;
    }
    operator T() const {
        return mObj;
    }
    bool isNull() const {
        return mObj == NULL;
    }
    void release();
    T detach() {
        T tObj = mObj;
        mObj = NULL;
        return tObj;
    }
    T clone();
    JNIObject<T>& operator = (const JNIObject<T>& rhs) {
        release();
        mHelper = rhs.mHelper;
        mObj = rhs.mObj;
        return *this;
    }
    void print() {
        ALOGD("holding %p", mObj);
    }

private:
    template<typename T2>
    JNIObject(const JNIObject<T2>& rhs);
};

class JNIHelper {
    JavaVM *mVM;
    JNIEnv *mEnv;

public :
    JNIHelper(JavaVM *vm);
    JNIHelper(JNIEnv *env);
    ~JNIHelper();

    void throwException(const char *message, int line);

    /* helpers to deal with members */
    jboolean getBoolField(jobject obj, const char *name);
    jint getIntField(jobject obj, const char *name);
    jlong getLongField(jobject obj, const char *name);
    JNIObject<jstring> getStringField(jobject obj, const char *name);
    bool getStringFieldValue(jobject obj, const char *name, char *buf, int size);
    JNIObject<jobject> getObjectField(jobject obj, const char *name, const char *type);
    JNIObject<jobjectArray> getArrayField(jobject obj, const char *name, const char *type);
    void getByteArrayField(jobject obj, const char *name, byte* buf, int size);
    jlong getLongArrayField(jobject obj, const char *name, int index);
    JNIObject<jobject> getObjectArrayField(
            jobject obj, const char *name, const char *type, int index);
    void setIntField(jobject obj, const char *name, jint value);
    void setByteField(jobject obj, const char *name, jbyte value);
    jbyte getByteField(jobject obj, const char *name);
    void setBooleanField(jobject obj, const char *name, jboolean value);
    void setLongField(jobject obj, const char *name, jlong value);
    void setLongArrayField(jobject obj, const char *name, jlongArray value);
    void setLongArrayElement(jobject obj, const char *name, int index, jlong value);
    jboolean setStringField(jobject obj, const char *name, const char *value);
    void reportEvent(jclass cls, const char *method, const char *signature, ...);
    JNIObject<jobject> createObject(const char *className);
    JNIObject<jobject> createObjectWithArgs(const char *className, const char *signature, ...);
    JNIObject<jobjectArray> createObjectArray(const char *className, int size);
    void setObjectField(jobject obj, const char *name, const char *type, jobject value);
    void callMethod(jobject obj, const char *method, const char *signature, ...);

    /* helpers to deal with static members */
    jlong getStaticLongField(jobject obj, const char *name);
    jlong getStaticLongField(jclass cls, const char *name);
    void setStaticLongField(jobject obj, const char *name, jlong value);
    void setStaticLongField(jclass cls, const char *name, jlong value);
    jlong getStaticLongArrayField(jobject obj, const char *name, int index);
    jlong getStaticLongArrayField(jclass cls, const char *name, int index);
    void setStaticLongArrayField(jobject obj, const char *name, jlongArray value);
    void setStaticLongArrayField(jclass obj, const char *name, jlongArray value);
    jboolean callStaticMethod(jclass cls, const char *method, const char *signature, ...);

    JNIObject<jobject> getObjectArrayElement(jobjectArray array, int index);
    JNIObject<jobject> getObjectArrayElement(jobject array, int index);
    int getArrayLength(jarray array);
    JNIObject<jobjectArray> newObjectArray(int num, const char *className, jobject val);
    JNIObject<jbyteArray> newByteArray(int num);
    JNIObject<jintArray> newIntArray(int num);
    JNIObject<jlongArray> newLongArray(int num);
    JNIObject<jstring> newStringUTF(const char *utf);
    void setObjectArrayElement(jobjectArray array, int index, jobject obj);
    void setByteArrayRegion(jbyteArray array, int from, int to, const jbyte *bytes);
    void setIntArrayRegion(jintArray array, int from, int to, const jint *ints);
    void setLongArrayRegion(jlongArray array, int from, int to, const jlong *longs);

    jobject newGlobalRef(jobject obj);
    void deleteGlobalRef(jobject obj);

private:
    /* Jni wrappers */
    friend class JNIObject<jobject>;
    friend class JNIObject<jstring>;
    friend class JNIObject<jobjectArray>;
    friend class JNIObject<jclass>;
    friend class JNIObject<jlongArray>;
    friend class JNIObject<jbyteArray>;
    friend class JNIObject<jintArray>;
    jobject newLocalRef(jobject obj);
    void deleteLocalRef(jobject obj);
};

template<typename T>
JNIObject<T>::JNIObject(JNIHelper &helper, T obj)
    : mHelper(helper), mObj(obj)
{ }

template<typename T>
JNIObject<T>::JNIObject(const JNIObject<T>& rhs)
    : mHelper(rhs.mHelper), mObj(NULL)
{
    mObj = (T)mHelper.newLocalRef(rhs.mObj);
}

template<typename T>
JNIObject<T>::~JNIObject() {
    release();
}

template<typename T>
void JNIObject<T>::release()
{
    if (mObj != NULL) {
        mHelper.deleteLocalRef(mObj);
        mObj = NULL;
    }
}

template<typename T>
T JNIObject<T>::clone()
{
    return mHelper.newLocalRef(mObj);
}

}

#define THROW(env, message)      (env).throwException(message, __LINE__)
