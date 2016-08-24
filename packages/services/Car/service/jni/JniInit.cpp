/*
 * Copyright (C) 2015 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "CAR.JNI"

#include <utils/Log.h>
#include <jni.h>

namespace android {
extern int register_com_android_car_CarCameraService(JNIEnv *env);
extern int register_com_android_car_CarInputService(JNIEnv *env);
};

extern "C" jint JNI_OnLoad(JavaVM* vm, void* /* reserved */) {
    JNIEnv* env = NULL;
    jint result = -1;

    if (vm->GetEnv((void**) &env, JNI_VERSION_1_4) != JNI_OK) {
        ALOGE("GetEnv failed!");
        return result;
    }
    ALOG_ASSERT(env, "Could not retrieve the env!");

    int r = android::register_com_android_car_CarCameraService(env);
    if (r != 0) {
        ALOGE("register_com_android_car_CarCameraService failed %d", r);
        return JNI_ERR;
    }
    r = android::register_com_android_car_CarInputService(env);
    if (r != 0) {
        ALOGE("register_com_android_car_CarInputService failed %d", r);
        return JNI_ERR;
    }
    return JNI_VERSION_1_6;
}
