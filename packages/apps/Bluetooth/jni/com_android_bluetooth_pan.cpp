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

#define LOG_TAG "BluetoothPanServiceJni"

#define LOG_NDEBUG 0

#define CHECK_CALLBACK_ENV                                                      \
   if (!checkCallbackThread()) {                                                \
       error("Callback: '%s' is not called on the correct thread", __FUNCTION__);\
       return;                                                                  \
   }

#include "com_android_bluetooth.h"
#include "hardware/bt_pan.h"
#include "utils/Log.h"
#include "android_runtime/AndroidRuntime.h"

#include <string.h>

#include <cutils/log.h>
#define info(fmt, ...)  ALOGI ("%s(L%d): " fmt,__FUNCTION__, __LINE__,  ## __VA_ARGS__)
#define debug(fmt, ...) ALOGD ("%s(L%d): " fmt,__FUNCTION__, __LINE__,  ## __VA_ARGS__)
#define warn(fmt, ...) ALOGW ("## WARNING : %s(L%d): " fmt "##",__FUNCTION__, __LINE__, ## __VA_ARGS__)
#define error(fmt, ...) ALOGE ("## ERROR : %s(L%d): " fmt "##",__FUNCTION__, __LINE__, ## __VA_ARGS__)
#define asrt(s) if(!(s)) ALOGE ("## %s(L%d): ASSERT %s failed! ##",__FUNCTION__, __LINE__, #s)


namespace android {

static jmethodID method_onConnectStateChanged;
static jmethodID method_onControlStateChanged;

static const btpan_interface_t *sPanIf = NULL;
static jobject mCallbacksObj = NULL;
static JNIEnv *sCallbackEnv = NULL;

static bool checkCallbackThread() {
    sCallbackEnv = getCallbackEnv();

    JNIEnv* env = AndroidRuntime::getJNIEnv();
    if (sCallbackEnv != env || sCallbackEnv == NULL) return false;
    return true;
}

static void control_state_callback(btpan_control_state_t state, int local_role, bt_status_t error,
                const char* ifname) {
    debug("state:%d, local_role:%d, ifname:%s", state, local_role, ifname);
    CHECK_CALLBACK_ENV
    jstring js_ifname = sCallbackEnv->NewStringUTF(ifname);
    sCallbackEnv->CallVoidMethod(mCallbacksObj, method_onControlStateChanged, (jint)local_role, (jint)state,
                                (jint)error, js_ifname);
    sCallbackEnv->DeleteLocalRef(js_ifname);
}

static void connection_state_callback(btpan_connection_state_t state, bt_status_t error, const bt_bdaddr_t *bd_addr,
                                      int local_role, int remote_role) {
    jbyteArray addr;
    debug("state:%d, local_role:%d, remote_role:%d", state, local_role, remote_role);
    CHECK_CALLBACK_ENV
    addr = sCallbackEnv->NewByteArray(sizeof(bt_bdaddr_t));
    if (!addr) {
        error("Fail to new jbyteArray bd addr for PAN channel state");
        checkAndClearExceptionFromCallback(sCallbackEnv, __FUNCTION__);
        return;
    }
    sCallbackEnv->SetByteArrayRegion(addr, 0, sizeof(bt_bdaddr_t), (jbyte *) bd_addr);

    sCallbackEnv->CallVoidMethod(mCallbacksObj, method_onConnectStateChanged, addr, (jint) state,
                                    (jint)error, (jint)local_role, (jint)remote_role);
    checkAndClearExceptionFromCallback(sCallbackEnv, __FUNCTION__);
    sCallbackEnv->DeleteLocalRef(addr);
}

static btpan_callbacks_t sBluetoothPanCallbacks = {
    sizeof(sBluetoothPanCallbacks),
    control_state_callback,
    connection_state_callback
};

// Define native functions

static void classInitNative(JNIEnv* env, jclass clazz) {
    method_onConnectStateChanged = env->GetMethodID(clazz, "onConnectStateChanged",
                                                    "([BIIII)V");
    method_onControlStateChanged = env->GetMethodID(clazz, "onControlStateChanged",
                                                    "(IIILjava/lang/String;)V");

    info("succeeds");
}
static const bt_interface_t* btIf;

static void initializeNative(JNIEnv *env, jobject object) {
    debug("pan");
    if(btIf)
        return;

    if ( (btIf = getBluetoothInterface()) == NULL) {
        error("Bluetooth module is not loaded");
        return;
    }

    if (sPanIf !=NULL) {
         ALOGW("Cleaning up Bluetooth PAN Interface before initializing...");
         sPanIf->cleanup();
         sPanIf = NULL;
    }

    if (mCallbacksObj != NULL) {
         ALOGW("Cleaning up Bluetooth PAN callback object");
         env->DeleteGlobalRef(mCallbacksObj);
         mCallbacksObj = NULL;
    }

    if ( (sPanIf = (btpan_interface_t *)
          btIf->get_profile_interface(BT_PROFILE_PAN_ID)) == NULL) {
        error("Failed to get Bluetooth PAN Interface");
        return;
    }

    mCallbacksObj = env->NewGlobalRef(object);

    bt_status_t status;
    if ( (status = sPanIf->init(&sBluetoothPanCallbacks)) != BT_STATUS_SUCCESS) {
        error("Failed to initialize Bluetooth PAN, status: %d", status);
        sPanIf = NULL;
        if (mCallbacksObj != NULL) {
            ALOGW("initialization failed: Cleaning up Bluetooth PAN callback object");
            env->DeleteGlobalRef(mCallbacksObj);
            mCallbacksObj = NULL;
        }
        return;
    }
}

static void cleanupNative(JNIEnv *env, jobject object) {
    if (!btIf) return;

    if (sPanIf !=NULL) {
        ALOGW("Cleaning up Bluetooth PAN Interface...");
        sPanIf->cleanup();
        sPanIf = NULL;
    }

    if (mCallbacksObj != NULL) {
        ALOGW("Cleaning up Bluetooth PAN callback object");
        env->DeleteGlobalRef(mCallbacksObj);
        mCallbacksObj = NULL;
    }
    btIf = NULL;
}

static jboolean enablePanNative(JNIEnv *env, jobject object, jint local_role) {
    bt_status_t status = BT_STATUS_FAIL;
    debug("in");
    if (sPanIf)
        status = sPanIf->enable(local_role);
    debug("out");
    return status == BT_STATUS_SUCCESS ? JNI_TRUE : JNI_FALSE;
}
static jint getPanLocalRoleNative(JNIEnv *env, jobject object) {
    debug("in");
    int local_role = 0;
    if (sPanIf)
        local_role  = sPanIf->get_local_role();
    debug("out");
    return (jint)local_role;
}



static jboolean connectPanNative(JNIEnv *env, jobject object, jbyteArray address,
                                 jint src_role, jint dest_role) {
    debug("in");
    bt_status_t status;
    jbyte *addr;
    jboolean ret = JNI_TRUE;
    if (!sPanIf) return JNI_FALSE;

    addr = env->GetByteArrayElements(address, NULL);
    if (!addr) {
        error("Bluetooth device address null");
        return JNI_FALSE;
    }

    if ((status = sPanIf->connect((bt_bdaddr_t *) addr, src_role, dest_role)) !=
         BT_STATUS_SUCCESS) {
        error("Failed PAN channel connection, status: %d", status);
        ret = JNI_FALSE;
    }
    env->ReleaseByteArrayElements(address, addr, 0);

    return ret;
}

static jboolean disconnectPanNative(JNIEnv *env, jobject object, jbyteArray address) {
    bt_status_t status;
    jbyte *addr;
    jboolean ret = JNI_TRUE;
    if (!sPanIf) return JNI_FALSE;

    addr = env->GetByteArrayElements(address, NULL);
    if (!addr) {
        error("Bluetooth device address null");
        return JNI_FALSE;
    }

    if ( (status = sPanIf->disconnect((bt_bdaddr_t *) addr)) !=
         BT_STATUS_SUCCESS) {
        error("Failed disconnect pan channel, status: %d", status);
        ret = JNI_FALSE;
    }
    env->ReleaseByteArrayElements(address, addr, 0);

    return ret;
}

static JNINativeMethod sMethods[] = {
    {"classInitNative", "()V", (void *) classInitNative},
    {"initializeNative", "()V", (void *) initializeNative},
    {"cleanupNative", "()V", (void *) cleanupNative},
    {"connectPanNative", "([BII)Z", (void *) connectPanNative},
    {"enablePanNative", "(I)Z", (void *) enablePanNative},
    {"getPanLocalRoleNative", "()I", (void *) getPanLocalRoleNative},
    {"disconnectPanNative", "([B)Z", (void *) disconnectPanNative},
    // TBD cleanup
};

int register_com_android_bluetooth_pan(JNIEnv* env)
{
    return jniRegisterNativeMethods(env, "com/android/bluetooth/pan/PanService",
                                    sMethods, NELEM(sMethods));
}

}
