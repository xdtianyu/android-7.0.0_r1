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

#define LOG_TAG "BluetoothHidServiceJni"

#define LOG_NDEBUG 1

#define CHECK_CALLBACK_ENV                                                      \
   if (!checkCallbackThread()) {                                                \
       ALOGE("Callback: '%s' is not called on the correct thread", __FUNCTION__);\
       return;                                                                  \
   }

#include "com_android_bluetooth.h"
#include "hardware/bt_hh.h"
#include "utils/Log.h"
#include "android_runtime/AndroidRuntime.h"

#include <string.h>

namespace android {

static jmethodID method_onConnectStateChanged;
static jmethodID method_onGetProtocolMode;
static jmethodID method_onGetReport;
static jmethodID method_onHandshake;
static jmethodID method_onVirtualUnplug;

static const bthh_interface_t *sBluetoothHidInterface = NULL;
static jobject mCallbacksObj = NULL;
static JNIEnv *sCallbackEnv = NULL;

static bool checkCallbackThread() {

    // Always fetch the latest callbackEnv from AdapterService.
    // Caching this could cause this sCallbackEnv to go out-of-sync
    // with the AdapterService's ENV if an ASSOCIATE/DISASSOCIATE event
    // is received

    sCallbackEnv = getCallbackEnv();

    JNIEnv* env = AndroidRuntime::getJNIEnv();
    if (sCallbackEnv != env || sCallbackEnv == NULL) return false;
    return true;
}

static void connection_state_callback(bt_bdaddr_t *bd_addr, bthh_connection_state_t state) {
    jbyteArray addr;

    CHECK_CALLBACK_ENV
    addr = sCallbackEnv->NewByteArray(sizeof(bt_bdaddr_t));
    if (!addr) {
        ALOGE("Fail to new jbyteArray bd addr for HID channel state");
        checkAndClearExceptionFromCallback(sCallbackEnv, __FUNCTION__);
        return;
    }
    sCallbackEnv->SetByteArrayRegion(addr, 0, sizeof(bt_bdaddr_t), (jbyte *) bd_addr);

    sCallbackEnv->CallVoidMethod(mCallbacksObj, method_onConnectStateChanged, addr, (jint) state);
    checkAndClearExceptionFromCallback(sCallbackEnv, __FUNCTION__);
    sCallbackEnv->DeleteLocalRef(addr);
}

static void get_protocol_mode_callback(bt_bdaddr_t *bd_addr, bthh_status_t hh_status,bthh_protocol_mode_t mode) {
    jbyteArray addr;

    CHECK_CALLBACK_ENV
    if (hh_status != BTHH_OK) {
        ALOGE("BTHH Status is not OK!");
        checkAndClearExceptionFromCallback(sCallbackEnv, __FUNCTION__);
        return;
    }

    addr = sCallbackEnv->NewByteArray(sizeof(bt_bdaddr_t));
    if (!addr) {
        ALOGE("Fail to new jbyteArray bd addr for get protocal mode callback");
        checkAndClearExceptionFromCallback(sCallbackEnv, __FUNCTION__);
        return;
    }
    sCallbackEnv->SetByteArrayRegion(addr, 0, sizeof(bt_bdaddr_t), (jbyte *) bd_addr);

    sCallbackEnv->CallVoidMethod(mCallbacksObj, method_onGetProtocolMode, addr, (jint) mode);
    checkAndClearExceptionFromCallback(sCallbackEnv, __FUNCTION__);
    sCallbackEnv->DeleteLocalRef(addr);
}

static void get_report_callback(bt_bdaddr_t *bd_addr, bthh_status_t hh_status, uint8_t *rpt_data, int rpt_size) {
    jbyteArray addr;
    jbyteArray data;

    CHECK_CALLBACK_ENV
    if (hh_status != BTHH_OK) {
        ALOGE("BTHH Status is not OK!");
        checkAndClearExceptionFromCallback(sCallbackEnv, __FUNCTION__);
        return;
    }

    addr = sCallbackEnv->NewByteArray(sizeof(bt_bdaddr_t));
    if (!addr) {
        ALOGE("Fail to new jbyteArray bd addr for get report callback");
        checkAndClearExceptionFromCallback(sCallbackEnv, __FUNCTION__);
        return;
    }
    data = sCallbackEnv->NewByteArray(rpt_size);
    if (!data) {
        ALOGE("Fail to new jbyteArray data for get report callback");
        checkAndClearExceptionFromCallback(sCallbackEnv, __FUNCTION__);
        sCallbackEnv->DeleteLocalRef(addr);
        return;
    }

    sCallbackEnv->SetByteArrayRegion(addr, 0, sizeof(bt_bdaddr_t), (jbyte *) bd_addr);
    sCallbackEnv->SetByteArrayRegion(data, 0, rpt_size, (jbyte *) rpt_data);

    sCallbackEnv->CallVoidMethod(mCallbacksObj, method_onGetReport, addr, data, (jint) rpt_size);
    checkAndClearExceptionFromCallback(sCallbackEnv, __FUNCTION__);
    sCallbackEnv->DeleteLocalRef(addr);
    sCallbackEnv->DeleteLocalRef(data);
}

static void virtual_unplug_callback(bt_bdaddr_t *bd_addr, bthh_status_t hh_status) {
    ALOGV("call to virtual_unplug_callback");
    jbyteArray addr;

    CHECK_CALLBACK_ENV
    addr = sCallbackEnv->NewByteArray(sizeof(bt_bdaddr_t));
    if (!addr) {
        ALOGE("Fail to new jbyteArray bd addr for HID channel state");
        checkAndClearExceptionFromCallback(sCallbackEnv, __FUNCTION__);
        return;
    }
    sCallbackEnv->SetByteArrayRegion(addr, 0, sizeof(bt_bdaddr_t), (jbyte *) bd_addr);

    sCallbackEnv->CallVoidMethod(mCallbacksObj, method_onVirtualUnplug, addr, (jint) hh_status);
    checkAndClearExceptionFromCallback(sCallbackEnv, __FUNCTION__);
    sCallbackEnv->DeleteLocalRef(addr);

    /*jbyteArray addr;
    jint status = hh_status;
    CHECK_CALLBACK_ENV
    addr = sCallbackEnv->NewByteArray(sizeof(bt_bdaddr_t));
    if (!addr) {
        ALOGE("Fail to new jbyteArray bd addr for HID report");
        checkAndClearExceptionFromCallback(sCallbackEnv, __FUNCTION__);
        return;
    }
    sCallbackEnv->SetByteArrayRegion(addr, 0, sizeof(bt_bdaddr_t), (jbyte *) bd_addr);

    sCallbackEnv->CallVoidMethod(mCallbacksObj, method_onVirtualUnplug, addr, status);
    checkAndClearExceptionFromCallback(sCallbackEnv, __FUNCTION__);
    sCallbackEnv->DeleteLocalRef(addr);*/
}

static void handshake_callback(bt_bdaddr_t *bd_addr, bthh_status_t hh_status)
{
    jbyteArray addr;

    CHECK_CALLBACK_ENV

    addr = sCallbackEnv->NewByteArray(sizeof(bt_bdaddr_t));
    if (!addr) {
        ALOGE("Fail to new jbyteArray bd addr for handshake callback");
        checkAndClearExceptionFromCallback(sCallbackEnv, __FUNCTION__);
        return;
    }
    sCallbackEnv->SetByteArrayRegion(addr, 0, sizeof(bt_bdaddr_t), (jbyte *) bd_addr);
    sCallbackEnv->CallVoidMethod(mCallbacksObj, method_onHandshake, addr, (jint) hh_status);
    checkAndClearExceptionFromCallback(sCallbackEnv, __FUNCTION__);
    sCallbackEnv->DeleteLocalRef(addr);
}

static bthh_callbacks_t sBluetoothHidCallbacks = {
    sizeof(sBluetoothHidCallbacks),
    connection_state_callback,
    NULL,
    get_protocol_mode_callback,
    NULL,
    get_report_callback,
    virtual_unplug_callback,
    handshake_callback
};

// Define native functions

static void classInitNative(JNIEnv* env, jclass clazz) {
    method_onConnectStateChanged = env->GetMethodID(clazz, "onConnectStateChanged", "([BI)V");
    method_onGetProtocolMode = env->GetMethodID(clazz, "onGetProtocolMode", "([BI)V");
    method_onGetReport = env->GetMethodID(clazz, "onGetReport", "([B[BI)V");
    method_onHandshake = env->GetMethodID(clazz, "onHandshake", "([BI)V");
    method_onVirtualUnplug = env->GetMethodID(clazz, "onVirtualUnplug", "([BI)V");

    ALOGI("%s: succeeds", __FUNCTION__);
}

static void initializeNative(JNIEnv *env, jobject object) {
    const bt_interface_t* btInf;
    bt_status_t status;

    if ( (btInf = getBluetoothInterface()) == NULL) {
        ALOGE("Bluetooth module is not loaded");
        return;
    }

    if (sBluetoothHidInterface !=NULL) {
        ALOGW("Cleaning up Bluetooth HID Interface before initializing...");
        sBluetoothHidInterface->cleanup();
        sBluetoothHidInterface = NULL;
    }

    if (mCallbacksObj != NULL) {
        ALOGW("Cleaning up Bluetooth GID callback object");
        env->DeleteGlobalRef(mCallbacksObj);
        mCallbacksObj = NULL;
    }


    if ( (sBluetoothHidInterface = (bthh_interface_t *)
          btInf->get_profile_interface(BT_PROFILE_HIDHOST_ID)) == NULL) {
        ALOGE("Failed to get Bluetooth HID Interface");
        return;
    }

    if ( (status = sBluetoothHidInterface->init(&sBluetoothHidCallbacks)) != BT_STATUS_SUCCESS) {
        ALOGE("Failed to initialize Bluetooth HID, status: %d", status);
        sBluetoothHidInterface = NULL;
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

    if (sBluetoothHidInterface !=NULL) {
        ALOGW("Cleaning up Bluetooth HID Interface...");
        sBluetoothHidInterface->cleanup();
        sBluetoothHidInterface = NULL;
    }

    if (mCallbacksObj != NULL) {
        ALOGW("Cleaning up Bluetooth GID callback object");
        env->DeleteGlobalRef(mCallbacksObj);
        mCallbacksObj = NULL;
    }

    env->DeleteGlobalRef(mCallbacksObj);
}

static jboolean connectHidNative(JNIEnv *env, jobject object, jbyteArray address) {
    bt_status_t status;
    jbyte *addr;
    jboolean ret = JNI_TRUE;
    if (!sBluetoothHidInterface) return JNI_FALSE;

    addr = env->GetByteArrayElements(address, NULL);
    if (!addr) {
        ALOGE("Bluetooth device address null");
        return JNI_FALSE;
    }

    if ((status = sBluetoothHidInterface->connect((bt_bdaddr_t *) addr)) !=
         BT_STATUS_SUCCESS) {
        ALOGE("Failed HID channel connection, status: %d", status);
        ret = JNI_FALSE;
    }
    env->ReleaseByteArrayElements(address, addr, 0);

    return ret;
}

static jboolean disconnectHidNative(JNIEnv *env, jobject object, jbyteArray address) {
    bt_status_t status;
    jbyte *addr;
    jboolean ret = JNI_TRUE;
    if (!sBluetoothHidInterface) return JNI_FALSE;

    addr = env->GetByteArrayElements(address, NULL);
    if (!addr) {
        ALOGE("Bluetooth device address null");
        return JNI_FALSE;
    }

    if ( (status = sBluetoothHidInterface->disconnect((bt_bdaddr_t *) addr)) !=
         BT_STATUS_SUCCESS) {
        ALOGE("Failed disconnect hid channel, status: %d", status);
        ret = JNI_FALSE;
    }
    env->ReleaseByteArrayElements(address, addr, 0);

    return ret;
}

static jboolean getProtocolModeNative(JNIEnv *env, jobject object, jbyteArray address) {
    bt_status_t status;
    jbyte *addr;
    jboolean ret = JNI_TRUE;
    bthh_protocol_mode_t protocolMode;
    if (!sBluetoothHidInterface) return JNI_FALSE;

    addr = env->GetByteArrayElements(address, NULL);
    if (!addr) {
        ALOGE("Bluetooth device address null");
        return JNI_FALSE;
    }

    if ( (status = sBluetoothHidInterface->get_protocol((bt_bdaddr_t *) addr, (bthh_protocol_mode_t) protocolMode)) !=
         BT_STATUS_SUCCESS) {
        ALOGE("Failed get protocol mode, status: %d", status);
        ret = JNI_FALSE;
    }
    env->ReleaseByteArrayElements(address, addr, 0);

    return ret;
}

static jboolean virtualUnPlugNative(JNIEnv *env, jobject object, jbyteArray address) {
    bt_status_t status;
    jbyte *addr;
    jboolean ret = JNI_TRUE;
    if (!sBluetoothHidInterface) return JNI_FALSE;

    addr = env->GetByteArrayElements(address, NULL);
        if (!addr) {
            ALOGE("Bluetooth device address null");
            return JNI_FALSE;
        }
    if ( (status = sBluetoothHidInterface->virtual_unplug((bt_bdaddr_t *) addr)) !=
             BT_STATUS_SUCCESS) {
        ALOGE("Failed virual unplug, status: %d", status);
        ret = JNI_FALSE;
    }
    env->ReleaseByteArrayElements(address, addr, 0);
    return ret;

}


static jboolean setProtocolModeNative(JNIEnv *env, jobject object, jbyteArray address, jint protocolMode) {
    bt_status_t status;
    jbyte *addr;
    jboolean ret = JNI_TRUE;
    if (!sBluetoothHidInterface) return JNI_FALSE;

    ALOGD("%s: protocolMode = %d", __FUNCTION__, protocolMode);

    addr = env->GetByteArrayElements(address, NULL);
    if (!addr) {
        ALOGE("Bluetooth device address null");
        return JNI_FALSE;
    }

    bthh_protocol_mode_t mode;
    switch(protocolMode){
        case 0:
            mode = BTHH_REPORT_MODE;
            break;
        case 1:
            mode = BTHH_BOOT_MODE;
            break;
        default:
            ALOGE("Unknown HID protocol mode");
            return JNI_FALSE;
    }
    if ( (status = sBluetoothHidInterface->set_protocol((bt_bdaddr_t *) addr, mode)) !=
             BT_STATUS_SUCCESS) {
        ALOGE("Failed set protocol mode, status: %d", status);
        ret = JNI_FALSE;
    }
    env->ReleaseByteArrayElements(address, addr, 0);

    return JNI_TRUE;
}

static jboolean getReportNative(JNIEnv *env, jobject object, jbyteArray address, jbyte reportType, jbyte reportId, jint bufferSize) {
    ALOGV("%s: reportType = %d, reportId = %d, bufferSize = %d", __FUNCTION__, reportType, reportId, bufferSize);

    bt_status_t status;
    jbyte *addr;
    jboolean ret = JNI_TRUE;
    if (!sBluetoothHidInterface) return JNI_FALSE;

    addr = env->GetByteArrayElements(address, NULL);
    if (!addr) {
        ALOGE("Bluetooth device address null");
        return JNI_FALSE;
    }

    jint rType = reportType;
    jint rId = reportId;

    if ( (status = sBluetoothHidInterface->get_report((bt_bdaddr_t *) addr, (bthh_report_type_t) rType, (uint8_t) rId, bufferSize)) !=
             BT_STATUS_SUCCESS) {
        ALOGE("Failed get report, status: %d", status);
        ret = JNI_FALSE;
    }
    env->ReleaseByteArrayElements(address, addr, 0);

    return ret;
}


static jboolean setReportNative(JNIEnv *env, jobject object, jbyteArray address, jbyte reportType, jstring report) {
    ALOGV("%s: reportType = %d", __FUNCTION__, reportType);
    bt_status_t status;
    jbyte *addr;
    jboolean ret = JNI_TRUE;
    if (!sBluetoothHidInterface) return JNI_FALSE;

    addr = env->GetByteArrayElements(address, NULL);
    if (!addr) {
        ALOGE("Bluetooth device address null");
        return JNI_FALSE;
    }
    jint rType = reportType;
    const char *c_report = env->GetStringUTFChars(report, NULL);

    if ( (status = sBluetoothHidInterface->set_report((bt_bdaddr_t *) addr, (bthh_report_type_t)rType, (char*) c_report)) !=
             BT_STATUS_SUCCESS) {
        ALOGE("Failed set report, status: %d", status);
        ret = JNI_FALSE;
    }
    env->ReleaseStringUTFChars(report, c_report);
    env->ReleaseByteArrayElements(address, addr, 0);

    return ret;
}

static jboolean sendDataNative(JNIEnv *env, jobject object, jbyteArray address, jstring report) {
    ALOGV("%s", __FUNCTION__);
    bt_status_t status;
    jbyte *addr;
    jboolean ret = JNI_TRUE;
    if (!sBluetoothHidInterface) return JNI_FALSE;

    addr = env->GetByteArrayElements(address, NULL);
    if (!addr) {
        ALOGE("Bluetooth device address null");
        return JNI_FALSE;
    }
    const char *c_report = env->GetStringUTFChars(report, NULL);
    if ( (status = sBluetoothHidInterface->send_data((bt_bdaddr_t *) addr, (char*) c_report)) !=
             BT_STATUS_SUCCESS) {
        ALOGE("Failed set report, status: %d", status);
        ret = JNI_FALSE;
    }
    env->ReleaseStringUTFChars(report, c_report);
    env->ReleaseByteArrayElements(address, addr, 0);

    return ret;

}

static JNINativeMethod sMethods[] = {
    {"classInitNative", "()V", (void *) classInitNative},
    {"initializeNative", "()V", (void *) initializeNative},
    {"cleanupNative", "()V", (void *) cleanupNative},
    {"connectHidNative", "([B)Z", (void *) connectHidNative},
    {"disconnectHidNative", "([B)Z", (void *) disconnectHidNative},
    {"getProtocolModeNative", "([B)Z", (void *) getProtocolModeNative},
    {"virtualUnPlugNative", "([B)Z", (void *) virtualUnPlugNative},
    {"setProtocolModeNative", "([BB)Z", (void *) setProtocolModeNative},
    {"getReportNative", "([BBBI)Z", (void *) getReportNative},
    {"setReportNative", "([BBLjava/lang/String;)Z", (void *) setReportNative},
    {"sendDataNative", "([BLjava/lang/String;)Z", (void *) sendDataNative},
};

int register_com_android_bluetooth_hid(JNIEnv* env)
{
    return jniRegisterNativeMethods(env, "com/android/bluetooth/hid/HidService",
                                    sMethods, NELEM(sMethods));
}

}
