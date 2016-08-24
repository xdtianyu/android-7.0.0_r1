/*
 * Copyright (C) 2012 The Android Open Source Project
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

#define LOG_TAG "BluetoothHealthServiceJni"

#define LOG_NDEBUG 0

#define CHECK_CALLBACK_ENV                                                      \
   if (!checkCallbackThread()) {                                                \
       ALOGE("Callback: '%s' is not called on the correct thread", __FUNCTION__);\
       return;                                                                  \
   }

#include "com_android_bluetooth.h"
#include "hardware/bt_hl.h"
#include "utils/Log.h"
#include "android_runtime/AndroidRuntime.h"

#include <string.h>

namespace android {

static jmethodID method_onAppRegistrationState;
static jmethodID method_onChannelStateChanged;

static const bthl_interface_t *sBluetoothHdpInterface = NULL;
static jobject mCallbacksObj = NULL;
static JNIEnv *sCallbackEnv = NULL;

static bool checkCallbackThread() {
    sCallbackEnv = getCallbackEnv();

    JNIEnv* env = AndroidRuntime::getJNIEnv();
    if (sCallbackEnv != env || sCallbackEnv == NULL) {
        ALOGE("Callback env check fail: env: %p, callback: %p", env, sCallbackEnv);
        return false;
    }
    return true;
}

// Define callback functions
static void app_registration_state_callback(int app_id, bthl_app_reg_state_t state) {
    CHECK_CALLBACK_ENV
    sCallbackEnv->CallVoidMethod(mCallbacksObj, method_onAppRegistrationState, app_id,
                                 (jint) state);
    checkAndClearExceptionFromCallback(sCallbackEnv, __FUNCTION__);
}

static void channel_state_callback(int app_id, bt_bdaddr_t *bd_addr, int mdep_cfg_index,
                                   int channel_id, bthl_channel_state_t state, int fd) {
    jbyteArray addr;
    jobject fileDescriptor = NULL;

    CHECK_CALLBACK_ENV
    addr = sCallbackEnv->NewByteArray(sizeof(bt_bdaddr_t));
    if (!addr) {
        ALOGE("Fail to new jbyteArray bd addr for channel state");
        checkAndClearExceptionFromCallback(sCallbackEnv, __FUNCTION__);
        return;
    }

    // TODO(BT) check if fd is only valid for BTHH_CONN_STATE_CONNECTED state
    if (state == BTHL_CONN_STATE_CONNECTED) {
        fileDescriptor = jniCreateFileDescriptor(sCallbackEnv, fd);
        if (!fileDescriptor) {
            ALOGE("Failed to convert file descriptor, fd: %d", fd);
            sCallbackEnv->DeleteLocalRef(addr);
            return;
        }
    }

    sCallbackEnv->SetByteArrayRegion(addr, 0, sizeof(bt_bdaddr_t), (jbyte*) bd_addr);
    sCallbackEnv->CallVoidMethod(mCallbacksObj, method_onChannelStateChanged, app_id, addr,
                                 mdep_cfg_index, channel_id, (jint) state, fileDescriptor);
    checkAndClearExceptionFromCallback(sCallbackEnv, __FUNCTION__);
    sCallbackEnv->DeleteLocalRef(addr);
}

static bthl_callbacks_t sBluetoothHdpCallbacks = {
    sizeof(sBluetoothHdpCallbacks),
    app_registration_state_callback,
    channel_state_callback
};

// Define native functions

static void classInitNative(JNIEnv* env, jclass clazz) {
    method_onAppRegistrationState = env->GetMethodID(clazz, "onAppRegistrationState", "(II)V");
    method_onChannelStateChanged = env->GetMethodID(clazz, "onChannelStateChanged",
            "(I[BIIILjava/io/FileDescriptor;)V");
    ALOGI("%s: succeeds", __FUNCTION__);
}

static void initializeNative(JNIEnv *env, jobject object) {
    const bt_interface_t* btInf;
    bt_status_t status;

    if ( (btInf = getBluetoothInterface()) == NULL) {
        ALOGE("Bluetooth module is not loaded");
        return;
    }

    if (sBluetoothHdpInterface !=NULL) {
         ALOGW("Cleaning up Bluetooth Health Interface before initializing...");
         sBluetoothHdpInterface->cleanup();
         sBluetoothHdpInterface = NULL;
    }

    if (mCallbacksObj != NULL) {
         ALOGW("Cleaning up Bluetooth Health callback object");
         env->DeleteGlobalRef(mCallbacksObj);
         mCallbacksObj = NULL;
    }

    if ( (sBluetoothHdpInterface = (bthl_interface_t *)
          btInf->get_profile_interface(BT_PROFILE_HEALTH_ID)) == NULL) {
        ALOGE("Failed to get Bluetooth Health Interface");
        return;
    }

    if ( (status = sBluetoothHdpInterface->init(&sBluetoothHdpCallbacks)) != BT_STATUS_SUCCESS) {
        ALOGE("Failed to initialize Bluetooth HDP, status: %d", status);
        sBluetoothHdpInterface = NULL;
        return;
    }

    mCallbacksObj = env->NewGlobalRef(object);
}

static void cleanupNative(JNIEnv *env, jobject object) {
    const bt_interface_t* btInf;

    if ( (btInf = getBluetoothInterface()) == NULL) {
        ALOGE("Bluetooth module is not loaded");
        return;
    }

    if (sBluetoothHdpInterface !=NULL) {
        ALOGW("Cleaning up Bluetooth Health Interface...");
        sBluetoothHdpInterface->cleanup();
        sBluetoothHdpInterface = NULL;
    }

    if (mCallbacksObj != NULL) {
        ALOGW("Cleaning up Bluetooth Health object");
        env->DeleteGlobalRef(mCallbacksObj);
        mCallbacksObj = NULL;
    }
}

static jint registerHealthAppNative(JNIEnv *env, jobject object, jint data_type,
                                       jint role, jstring name, jint channel_type) {
    bt_status_t status;
    bthl_mdep_cfg_t mdep_cfg;
    bthl_reg_param_t reg_param;
    int app_id;

    if (!sBluetoothHdpInterface) {
        ALOGE("Failed to register health app. No Bluetooth Health Interface available");
        return -1;
    }

    mdep_cfg.mdep_role = (bthl_mdep_role_t) role;
    mdep_cfg.data_type = data_type;
    mdep_cfg.channel_type = (bthl_channel_type_t) channel_type;
    // TODO(BT) pass all the followings in from java instead of reuse name
    mdep_cfg.mdep_description = env->GetStringUTFChars(name, NULL);
    reg_param.application_name = env->GetStringUTFChars(name, NULL);
    reg_param.provider_name = NULL;
    reg_param.srv_name = NULL;
    reg_param.srv_desp = NULL;
    reg_param.number_of_mdeps = 1;
    reg_param.mdep_cfg = &mdep_cfg;

    if ( (status = sBluetoothHdpInterface->register_application(&reg_param, &app_id)) !=
         BT_STATUS_SUCCESS) {
        ALOGE("Failed register health app, status: %d", status);
        return -1;
    }

    env->ReleaseStringUTFChars(name, mdep_cfg.mdep_description);
    env->ReleaseStringUTFChars(name, reg_param.application_name);
    return app_id;
}

static jboolean unregisterHealthAppNative(JNIEnv *env, jobject object, int app_id) {
    bt_status_t status;
    if (!sBluetoothHdpInterface) return JNI_FALSE;

    if ((status = sBluetoothHdpInterface->unregister_application(app_id)) != BT_STATUS_SUCCESS) {
        ALOGE("Failed to unregister app %d, status: %d", app_id, status);
    }
    return (status == BT_STATUS_SUCCESS) ? JNI_TRUE : JNI_FALSE;
}

static jint connectChannelNative(JNIEnv *env, jobject object,
                                 jbyteArray address, jint app_id) {
    bt_status_t status;
    jbyte *addr;
    jint chan_id;
    if (!sBluetoothHdpInterface) return -1;

    addr = env->GetByteArrayElements(address, NULL);
    if (!addr) {
        ALOGE("Bluetooth device address null");
        return -1;
    }

    if ( (status = sBluetoothHdpInterface->connect_channel(app_id, (bt_bdaddr_t *) addr,
                                                           0, &chan_id)) !=
         BT_STATUS_SUCCESS) {
        ALOGE("Failed HDP channel connection, status: %d", status);
        chan_id = -1;
    }
    env->ReleaseByteArrayElements(address, addr, 0);

    return chan_id;
}

static jboolean disconnectChannelNative(JNIEnv *env, jobject object, jint channel_id) {
    bt_status_t status;
    if (!sBluetoothHdpInterface) return JNI_FALSE;

    if ( (status = sBluetoothHdpInterface->destroy_channel(channel_id)) !=
         BT_STATUS_SUCCESS) {
        ALOGE("Failed disconnect health channel, status: %d", status);
        return JNI_FALSE;
    }
    return JNI_TRUE;
}

static JNINativeMethod sMethods[] = {
    {"classInitNative", "()V", (void *) classInitNative},
    {"initializeNative", "()V", (void *) initializeNative},
    {"cleanupNative", "()V", (void *) cleanupNative},
    {"registerHealthAppNative", "(IILjava/lang/String;I)I", (void *) registerHealthAppNative},
    {"unregisterHealthAppNative", "(I)Z", (void *) unregisterHealthAppNative},
    {"connectChannelNative", "([BI)I", (void *) connectChannelNative},
    {"disconnectChannelNative", "(I)Z", (void *) disconnectChannelNative},
};

int register_com_android_bluetooth_hdp(JNIEnv* env)
{
    return jniRegisterNativeMethods(env, "com/android/bluetooth/hdp/HealthService",
                                    sMethods, NELEM(sMethods));
}

}
