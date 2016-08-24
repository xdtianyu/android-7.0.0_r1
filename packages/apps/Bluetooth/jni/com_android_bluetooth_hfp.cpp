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

#define LOG_TAG "BluetoothHeadsetServiceJni"

#define LOG_NDEBUG 0

#define CHECK_CALLBACK_ENV                                                      \
   if (!checkCallbackThread()) {                                                \
       ALOGE("Callback: '%s' is not called on the correct thread", __FUNCTION__);\
       return;                                                                  \
   }

#include "com_android_bluetooth.h"
#include "hardware/bt_hf.h"
#include "utils/Log.h"
#include "android_runtime/AndroidRuntime.h"

#include <string.h>

namespace android {

static jmethodID method_onConnectionStateChanged;
static jmethodID method_onAudioStateChanged;
static jmethodID method_onVrStateChanged;
static jmethodID method_onAnswerCall;
static jmethodID method_onHangupCall;
static jmethodID method_onVolumeChanged;
static jmethodID method_onDialCall;
static jmethodID method_onSendDtmf;
static jmethodID method_onNoiceReductionEnable;
static jmethodID method_onWBS;
static jmethodID method_onAtChld;
static jmethodID method_onAtCnum;
static jmethodID method_onAtCind;
static jmethodID method_onAtCops;
static jmethodID method_onAtClcc;
static jmethodID method_onUnknownAt;
static jmethodID method_onKeyPressed;

static const bthf_interface_t *sBluetoothHfpInterface = NULL;
static jobject mCallbacksObj = NULL;
static JNIEnv *sCallbackEnv = NULL;

static bool checkCallbackThread() {
    // Always fetch the latest callbackEnv from AdapterService.
    // Caching this could cause this sCallbackEnv to go out-of-sync
    // with the AdapterService's ENV if an ASSOCIATE/DISASSOCIATE event
    // is received
    //if (sCallbackEnv == NULL) {
    sCallbackEnv = getCallbackEnv();
    //}
    JNIEnv* env = AndroidRuntime::getJNIEnv();
    if (sCallbackEnv != env || sCallbackEnv == NULL) return false;
    return true;
}

static jbyteArray marshall_bda(bt_bdaddr_t* bd_addr)
{
    jbyteArray addr;
    addr = sCallbackEnv->NewByteArray(sizeof(bt_bdaddr_t));
    if (!addr) {
        ALOGE("Fail to new jbyteArray bd addr");
        checkAndClearExceptionFromCallback(sCallbackEnv, __FUNCTION__);
        return NULL;
    }
    sCallbackEnv->SetByteArrayRegion(addr, 0, sizeof(bt_bdaddr_t), (jbyte *) bd_addr);
    return addr;
}

static void connection_state_callback(bthf_connection_state_t state, bt_bdaddr_t* bd_addr) {
    jbyteArray addr;

    ALOGI("%s", __FUNCTION__);

    CHECK_CALLBACK_ENV
    addr = sCallbackEnv->NewByteArray(sizeof(bt_bdaddr_t));
    if (!addr) {
        ALOGE("Fail to new jbyteArray bd addr for connection state");
        checkAndClearExceptionFromCallback(sCallbackEnv, __FUNCTION__);
        return;
    }

    sCallbackEnv->SetByteArrayRegion(addr, 0, sizeof(bt_bdaddr_t), (jbyte*) bd_addr);
    sCallbackEnv->CallVoidMethod(mCallbacksObj, method_onConnectionStateChanged,
                                 (jint) state, addr);
    checkAndClearExceptionFromCallback(sCallbackEnv, __FUNCTION__);
    sCallbackEnv->DeleteLocalRef(addr);
}

static void audio_state_callback(bthf_audio_state_t state, bt_bdaddr_t* bd_addr) {
    jbyteArray addr;

    CHECK_CALLBACK_ENV
    addr = sCallbackEnv->NewByteArray(sizeof(bt_bdaddr_t));
    if (!addr) {
        ALOGE("Fail to new jbyteArray bd addr for audio state");
        checkAndClearExceptionFromCallback(sCallbackEnv, __FUNCTION__);
        return;
    }

    sCallbackEnv->SetByteArrayRegion(addr, 0, sizeof(bt_bdaddr_t), (jbyte *) bd_addr);
    sCallbackEnv->CallVoidMethod(mCallbacksObj, method_onAudioStateChanged, (jint) state, addr);
    checkAndClearExceptionFromCallback(sCallbackEnv, __FUNCTION__);
    sCallbackEnv->DeleteLocalRef(addr);
}

static void voice_recognition_callback(bthf_vr_state_t state, bt_bdaddr_t* bd_addr) {
    jbyteArray addr;

    CHECK_CALLBACK_ENV
    addr = sCallbackEnv->NewByteArray(sizeof(bt_bdaddr_t));
    if (!addr) {
        ALOGE("Fail to new jbyteArray bd addr for audio state");
        checkAndClearExceptionFromCallback(sCallbackEnv, __FUNCTION__);
        return;
    }

    sCallbackEnv->SetByteArrayRegion(addr, 0, sizeof(bt_bdaddr_t), (jbyte *) bd_addr);
    sCallbackEnv->CallVoidMethod(mCallbacksObj, method_onVrStateChanged, (jint) state, addr);
    checkAndClearExceptionFromCallback(sCallbackEnv, __FUNCTION__);
    sCallbackEnv->DeleteLocalRef(addr);
}

static void answer_call_callback(bt_bdaddr_t* bd_addr) {
    jbyteArray addr;

    CHECK_CALLBACK_ENV
    addr = sCallbackEnv->NewByteArray(sizeof(bt_bdaddr_t));
    if (!addr) {
        ALOGE("Fail to new jbyteArray bd addr for audio state");
        checkAndClearExceptionFromCallback(sCallbackEnv, __FUNCTION__);
        return;
    }

    sCallbackEnv->SetByteArrayRegion(addr, 0, sizeof(bt_bdaddr_t), (jbyte *) bd_addr);
    sCallbackEnv->CallVoidMethod(mCallbacksObj, method_onAnswerCall, addr);
    checkAndClearExceptionFromCallback(sCallbackEnv, __FUNCTION__);
    sCallbackEnv->DeleteLocalRef(addr);
}

static void hangup_call_callback(bt_bdaddr_t* bd_addr) {
    jbyteArray addr;

    CHECK_CALLBACK_ENV
    addr = sCallbackEnv->NewByteArray(sizeof(bt_bdaddr_t));
    if (!addr) {
        ALOGE("Fail to new jbyteArray bd addr for audio state");
        checkAndClearExceptionFromCallback(sCallbackEnv, __FUNCTION__);
        return;
    }

    sCallbackEnv->SetByteArrayRegion(addr, 0, sizeof(bt_bdaddr_t), (jbyte *) bd_addr);
    sCallbackEnv->CallVoidMethod(mCallbacksObj, method_onHangupCall, addr);
    checkAndClearExceptionFromCallback(sCallbackEnv, __FUNCTION__);
    sCallbackEnv->DeleteLocalRef(addr);
}

static void volume_control_callback(bthf_volume_type_t type, int volume, bt_bdaddr_t* bd_addr) {
    jbyteArray addr;

    CHECK_CALLBACK_ENV
    addr = sCallbackEnv->NewByteArray(sizeof(bt_bdaddr_t));
    if (!addr) {
        ALOGE("Fail to new jbyteArray bd addr for audio state");
        checkAndClearExceptionFromCallback(sCallbackEnv, __FUNCTION__);
        return;
    }

    sCallbackEnv->SetByteArrayRegion(addr, 0, sizeof(bt_bdaddr_t), (jbyte *) bd_addr);
    sCallbackEnv->CallVoidMethod(mCallbacksObj, method_onVolumeChanged, (jint) type,
                                                  (jint) volume, addr);
    checkAndClearExceptionFromCallback(sCallbackEnv, __FUNCTION__);
    sCallbackEnv->DeleteLocalRef(addr);
}

static void dial_call_callback(char *number, bt_bdaddr_t* bd_addr) {
    jbyteArray addr;

    CHECK_CALLBACK_ENV
    addr = sCallbackEnv->NewByteArray(sizeof(bt_bdaddr_t));
    if (!addr) {
        ALOGE("Fail to new jbyteArray bd addr for audio state");
        checkAndClearExceptionFromCallback(sCallbackEnv, __FUNCTION__);
        return;
    }

    sCallbackEnv->SetByteArrayRegion(addr, 0, sizeof(bt_bdaddr_t), (jbyte *) bd_addr);
    jstring js_number = sCallbackEnv->NewStringUTF(number);
    sCallbackEnv->CallVoidMethod(mCallbacksObj, method_onDialCall,
                                 js_number, addr);
    checkAndClearExceptionFromCallback(sCallbackEnv, __FUNCTION__);
    sCallbackEnv->DeleteLocalRef(js_number);
    sCallbackEnv->DeleteLocalRef(addr);
}

static void dtmf_cmd_callback(char dtmf, bt_bdaddr_t* bd_addr) {
    jbyteArray addr;

    CHECK_CALLBACK_ENV
    addr = sCallbackEnv->NewByteArray(sizeof(bt_bdaddr_t));
    if (!addr) {
        ALOGE("Fail to new jbyteArray bd addr for audio state");
        checkAndClearExceptionFromCallback(sCallbackEnv, __FUNCTION__);
        return;
    }

    sCallbackEnv->SetByteArrayRegion(addr, 0, sizeof(bt_bdaddr_t), (jbyte *) bd_addr);
    // TBD dtmf has changed from int to char
    sCallbackEnv->CallVoidMethod(mCallbacksObj, method_onSendDtmf, dtmf, addr);
    checkAndClearExceptionFromCallback(sCallbackEnv, __FUNCTION__);
    sCallbackEnv->DeleteLocalRef(addr);
}

static void noice_reduction_callback(bthf_nrec_t nrec, bt_bdaddr_t* bd_addr) {
    jbyteArray addr;

    CHECK_CALLBACK_ENV
    addr = sCallbackEnv->NewByteArray(sizeof(bt_bdaddr_t));
    if (!addr) {
        ALOGE("Fail to new jbyteArray bd addr for audio state");
        checkAndClearExceptionFromCallback(sCallbackEnv, __FUNCTION__);
        return;
    }
    sCallbackEnv->SetByteArrayRegion(addr, 0, sizeof(bt_bdaddr_t), (jbyte *) bd_addr);
    sCallbackEnv->CallVoidMethod(mCallbacksObj, method_onNoiceReductionEnable,
                                 nrec == BTHF_NREC_START, addr);
    checkAndClearExceptionFromCallback(sCallbackEnv, __FUNCTION__);
    sCallbackEnv->DeleteLocalRef(addr);
}

static void wbs_callback(bthf_wbs_config_t wbs_config, bt_bdaddr_t* bd_addr) {
    jbyteArray addr;

    CHECK_CALLBACK_ENV

    if ((addr = marshall_bda(bd_addr)) == NULL)
        return;

    sCallbackEnv->CallVoidMethod(mCallbacksObj, method_onWBS, wbs_config, addr);
    checkAndClearExceptionFromCallback(sCallbackEnv, __FUNCTION__);
    sCallbackEnv->DeleteLocalRef(addr);
}

static void at_chld_callback(bthf_chld_type_t chld, bt_bdaddr_t* bd_addr) {
    jbyteArray addr;

    CHECK_CALLBACK_ENV
    addr = sCallbackEnv->NewByteArray(sizeof(bt_bdaddr_t));
    if (!addr) {
        ALOGE("Fail to new jbyteArray bd addr for audio state");
        checkAndClearExceptionFromCallback(sCallbackEnv, __FUNCTION__);
        return;
    }

    sCallbackEnv->SetByteArrayRegion(addr, 0, sizeof(bt_bdaddr_t), (jbyte *) bd_addr);
    sCallbackEnv->CallVoidMethod(mCallbacksObj, method_onAtChld, chld, addr);
    checkAndClearExceptionFromCallback(sCallbackEnv, __FUNCTION__);
    sCallbackEnv->DeleteLocalRef(addr);
}

static void at_cnum_callback(bt_bdaddr_t* bd_addr) {
    jbyteArray addr;

    CHECK_CALLBACK_ENV
    addr = sCallbackEnv->NewByteArray(sizeof(bt_bdaddr_t));
    if (!addr) {
        ALOGE("Fail to new jbyteArray bd addr for audio state");
        checkAndClearExceptionFromCallback(sCallbackEnv, __FUNCTION__);
        return;
    }

    sCallbackEnv->SetByteArrayRegion(addr, 0, sizeof(bt_bdaddr_t), (jbyte *) bd_addr);
    sCallbackEnv->CallVoidMethod(mCallbacksObj, method_onAtCnum, addr);
    checkAndClearExceptionFromCallback(sCallbackEnv, __FUNCTION__);
    sCallbackEnv->DeleteLocalRef(addr);
}

static void at_cind_callback(bt_bdaddr_t* bd_addr) {
    jbyteArray addr;

    CHECK_CALLBACK_ENV
    addr = sCallbackEnv->NewByteArray(sizeof(bt_bdaddr_t));
    if (!addr) {
        ALOGE("Fail to new jbyteArray bd addr for audio state");
        checkAndClearExceptionFromCallback(sCallbackEnv, __FUNCTION__);
        return;
    }

    sCallbackEnv->SetByteArrayRegion(addr, 0, sizeof(bt_bdaddr_t), (jbyte *) bd_addr);
    sCallbackEnv->CallVoidMethod(mCallbacksObj, method_onAtCind, addr);
    checkAndClearExceptionFromCallback(sCallbackEnv, __FUNCTION__);
    sCallbackEnv->DeleteLocalRef(addr);
}

static void at_cops_callback(bt_bdaddr_t* bd_addr) {
    jbyteArray addr;

    CHECK_CALLBACK_ENV
    addr = sCallbackEnv->NewByteArray(sizeof(bt_bdaddr_t));
    if (!addr) {
        ALOGE("Fail to new jbyteArray bd addr for audio state");
        checkAndClearExceptionFromCallback(sCallbackEnv, __FUNCTION__);
        return;
    }

    sCallbackEnv->SetByteArrayRegion(addr, 0, sizeof(bt_bdaddr_t), (jbyte *) bd_addr);
    sCallbackEnv->CallVoidMethod(mCallbacksObj, method_onAtCops, addr);
    checkAndClearExceptionFromCallback(sCallbackEnv, __FUNCTION__);
    sCallbackEnv->DeleteLocalRef(addr);
}

static void at_clcc_callback(bt_bdaddr_t* bd_addr) {
    jbyteArray addr;

    CHECK_CALLBACK_ENV
    addr = sCallbackEnv->NewByteArray(sizeof(bt_bdaddr_t));
    if (!addr) {
        ALOGE("Fail to new jbyteArray bd addr for audio state");
        checkAndClearExceptionFromCallback(sCallbackEnv, __FUNCTION__);
        return;
    }

    sCallbackEnv->SetByteArrayRegion(addr, 0, sizeof(bt_bdaddr_t), (jbyte *) bd_addr);
    sCallbackEnv->CallVoidMethod(mCallbacksObj, method_onAtClcc, addr);
    checkAndClearExceptionFromCallback(sCallbackEnv, __FUNCTION__);
    sCallbackEnv->DeleteLocalRef(addr);
}

static void unknown_at_callback(char *at_string, bt_bdaddr_t* bd_addr) {
    jbyteArray addr;

    CHECK_CALLBACK_ENV
    addr = sCallbackEnv->NewByteArray(sizeof(bt_bdaddr_t));
    if (!addr) {
        ALOGE("Fail to new jbyteArray bd addr for audio state");
        checkAndClearExceptionFromCallback(sCallbackEnv, __FUNCTION__);
        return;
    }

    sCallbackEnv->SetByteArrayRegion(addr, 0, sizeof(bt_bdaddr_t), (jbyte *) bd_addr);
    jstring js_at_string = sCallbackEnv->NewStringUTF(at_string);
    sCallbackEnv->CallVoidMethod(mCallbacksObj, method_onUnknownAt,
                                 js_at_string, addr);
    checkAndClearExceptionFromCallback(sCallbackEnv, __FUNCTION__);
    sCallbackEnv->DeleteLocalRef(js_at_string);
    sCallbackEnv->DeleteLocalRef(addr);
}

static void key_pressed_callback(bt_bdaddr_t* bd_addr) {
    jbyteArray addr;

    CHECK_CALLBACK_ENV
    addr = sCallbackEnv->NewByteArray(sizeof(bt_bdaddr_t));
    if (!addr) {
        ALOGE("Fail to new jbyteArray bd addr for audio state");
        checkAndClearExceptionFromCallback(sCallbackEnv, __FUNCTION__);
        return;
    }

    sCallbackEnv->SetByteArrayRegion(addr, 0, sizeof(bt_bdaddr_t), (jbyte *) bd_addr);
    sCallbackEnv->CallVoidMethod(mCallbacksObj, method_onKeyPressed, addr);
    checkAndClearExceptionFromCallback(sCallbackEnv, __FUNCTION__);
    sCallbackEnv->DeleteLocalRef(addr);
}

static bthf_callbacks_t sBluetoothHfpCallbacks = {
    sizeof(sBluetoothHfpCallbacks),
    connection_state_callback,
    audio_state_callback,
    voice_recognition_callback,
    answer_call_callback,
    hangup_call_callback,
    volume_control_callback,
    dial_call_callback,
    dtmf_cmd_callback,
    noice_reduction_callback,
    wbs_callback,
    at_chld_callback,
    at_cnum_callback,
    at_cind_callback,
    at_cops_callback,
    at_clcc_callback,
    unknown_at_callback,
    key_pressed_callback
};

static void classInitNative(JNIEnv* env, jclass clazz) {
    method_onConnectionStateChanged =
        env->GetMethodID(clazz, "onConnectionStateChanged", "(I[B)V");
    method_onAudioStateChanged = env->GetMethodID(clazz, "onAudioStateChanged", "(I[B)V");
    method_onVrStateChanged = env->GetMethodID(clazz, "onVrStateChanged", "(I[B)V");
    method_onAnswerCall = env->GetMethodID(clazz, "onAnswerCall", "([B)V");
    method_onHangupCall = env->GetMethodID(clazz, "onHangupCall", "([B)V");
    method_onVolumeChanged = env->GetMethodID(clazz, "onVolumeChanged", "(II[B)V");
    method_onDialCall = env->GetMethodID(clazz, "onDialCall", "(Ljava/lang/String;[B)V");
    method_onSendDtmf = env->GetMethodID(clazz, "onSendDtmf", "(I[B)V");
    method_onNoiceReductionEnable = env->GetMethodID(clazz, "onNoiceReductionEnable", "(Z[B)V");
    method_onWBS = env->GetMethodID(clazz,"onWBS","(I[B)V");
    method_onAtChld = env->GetMethodID(clazz, "onAtChld", "(I[B)V");
    method_onAtCnum = env->GetMethodID(clazz, "onAtCnum", "([B)V");
    method_onAtCind = env->GetMethodID(clazz, "onAtCind", "([B)V");
    method_onAtCops = env->GetMethodID(clazz, "onAtCops", "([B)V");
    method_onAtClcc = env->GetMethodID(clazz, "onAtClcc", "([B)V");
    method_onUnknownAt = env->GetMethodID(clazz, "onUnknownAt", "(Ljava/lang/String;[B)V");
    method_onKeyPressed = env->GetMethodID(clazz, "onKeyPressed", "([B)V");

    ALOGI("%s: succeeds", __FUNCTION__);
}

static void initializeNative(JNIEnv *env, jobject object, jint max_hf_clients) {
    const bt_interface_t* btInf;
    bt_status_t status;

    if ( (btInf = getBluetoothInterface()) == NULL) {
        ALOGE("Bluetooth module is not loaded");
        return;
    }

    if (sBluetoothHfpInterface !=NULL) {
        ALOGW("Cleaning up Bluetooth Handsfree Interface before initializing...");
        sBluetoothHfpInterface->cleanup();
        sBluetoothHfpInterface = NULL;
    }

    if (mCallbacksObj != NULL) {
        ALOGW("Cleaning up Bluetooth Handsfree callback object");
        env->DeleteGlobalRef(mCallbacksObj);
        mCallbacksObj = NULL;
    }

    if ( (sBluetoothHfpInterface = (bthf_interface_t *)
          btInf->get_profile_interface(BT_PROFILE_HANDSFREE_ID)) == NULL) {
        ALOGE("Failed to get Bluetooth Handsfree Interface");
        return;
    }

    if ( (status = sBluetoothHfpInterface->init(&sBluetoothHfpCallbacks,
          max_hf_clients)) != BT_STATUS_SUCCESS) {
        ALOGE("Failed to initialize Bluetooth HFP, status: %d", status);
        sBluetoothHfpInterface = NULL;
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

    if (sBluetoothHfpInterface !=NULL) {
        ALOGW("Cleaning up Bluetooth Handsfree Interface...");
        sBluetoothHfpInterface->cleanup();
        sBluetoothHfpInterface = NULL;
    }

    if (mCallbacksObj != NULL) {
        ALOGW("Cleaning up Bluetooth Handsfree callback object");
        env->DeleteGlobalRef(mCallbacksObj);
        mCallbacksObj = NULL;
    }
}

static jboolean connectHfpNative(JNIEnv *env, jobject object, jbyteArray address) {
    jbyte *addr;
    bt_status_t status;

    ALOGI("%s: sBluetoothHfpInterface: %p", __FUNCTION__, sBluetoothHfpInterface);
    if (!sBluetoothHfpInterface) return JNI_FALSE;

    addr = env->GetByteArrayElements(address, NULL);
    if (!addr) {
        jniThrowIOException(env, EINVAL);
        return JNI_FALSE;
    }

    if ((status = sBluetoothHfpInterface->connect((bt_bdaddr_t *)addr)) != BT_STATUS_SUCCESS) {
        ALOGE("Failed HF connection, status: %d", status);
    }
    env->ReleaseByteArrayElements(address, addr, 0);
    return (status == BT_STATUS_SUCCESS) ? JNI_TRUE : JNI_FALSE;
}

static jboolean disconnectHfpNative(JNIEnv *env, jobject object, jbyteArray address) {
    jbyte *addr;
    bt_status_t status;

    if (!sBluetoothHfpInterface) return JNI_FALSE;

    addr = env->GetByteArrayElements(address, NULL);
    if (!addr) {
        jniThrowIOException(env, EINVAL);
        return JNI_FALSE;
    }

    if ( (status = sBluetoothHfpInterface->disconnect((bt_bdaddr_t *)addr)) != BT_STATUS_SUCCESS) {
        ALOGE("Failed HF disconnection, status: %d", status);
    }
    env->ReleaseByteArrayElements(address, addr, 0);
    return (status == BT_STATUS_SUCCESS) ? JNI_TRUE : JNI_FALSE;
}

static jboolean connectAudioNative(JNIEnv *env, jobject object, jbyteArray address) {
    jbyte *addr;
    bt_status_t status;

    if (!sBluetoothHfpInterface) return JNI_FALSE;

    addr = env->GetByteArrayElements(address, NULL);
    if (!addr) {
        jniThrowIOException(env, EINVAL);
        return JNI_FALSE;
    }

    if ( (status = sBluetoothHfpInterface->connect_audio((bt_bdaddr_t *)addr)) !=
         BT_STATUS_SUCCESS) {
        ALOGE("Failed HF audio connection, status: %d", status);
    }
    env->ReleaseByteArrayElements(address, addr, 0);
    return (status == BT_STATUS_SUCCESS) ? JNI_TRUE : JNI_FALSE;
}

static jboolean disconnectAudioNative(JNIEnv *env, jobject object, jbyteArray address) {
    jbyte *addr;
    bt_status_t status;

    if (!sBluetoothHfpInterface) return JNI_FALSE;

    addr = env->GetByteArrayElements(address, NULL);
    if (!addr) {
        jniThrowIOException(env, EINVAL);
        return JNI_FALSE;
    }

    if ( (status = sBluetoothHfpInterface->disconnect_audio((bt_bdaddr_t *) addr)) !=
         BT_STATUS_SUCCESS) {
        ALOGE("Failed HF audio disconnection, status: %d", status);
    }
    env->ReleaseByteArrayElements(address, addr, 0);
    return (status == BT_STATUS_SUCCESS) ? JNI_TRUE : JNI_FALSE;
}

static jboolean startVoiceRecognitionNative(JNIEnv *env, jobject object, jbyteArray address) {
    jbyte *addr;
    bt_status_t status;
    if (!sBluetoothHfpInterface) return JNI_FALSE;

    addr = env->GetByteArrayElements(address, NULL);
    if (!addr) {
        jniThrowIOException(env, EINVAL);
        return JNI_FALSE;
    }

    if ( (status = sBluetoothHfpInterface->start_voice_recognition((bt_bdaddr_t *) addr))
                                          != BT_STATUS_SUCCESS) {
        ALOGE("Failed to start voice recognition, status: %d", status);
    }
    env->ReleaseByteArrayElements(address, addr, 0);
    return (status == BT_STATUS_SUCCESS) ? JNI_TRUE : JNI_FALSE;
}

static jboolean stopVoiceRecognitionNative(JNIEnv *env, jobject object, jbyteArray address) {
    jbyte *addr;
    bt_status_t status;
    if (!sBluetoothHfpInterface) return JNI_FALSE;

    addr = env->GetByteArrayElements(address, NULL);
    if (!addr) {
        jniThrowIOException(env, EINVAL);
        return JNI_FALSE;
    }

    if ( (status = sBluetoothHfpInterface->stop_voice_recognition((bt_bdaddr_t *) addr))
                                     != BT_STATUS_SUCCESS) {
        ALOGE("Failed to stop voice recognition, status: %d", status);
    }
    env->ReleaseByteArrayElements(address, addr, 0);
    return (status == BT_STATUS_SUCCESS) ? JNI_TRUE : JNI_FALSE;
}

static jboolean setVolumeNative(JNIEnv *env, jobject object, jint volume_type,
                                     jint volume, jbyteArray address) {
    jbyte *addr;
    bt_status_t status;
    if (!sBluetoothHfpInterface) return JNI_FALSE;

    addr = env->GetByteArrayElements(address, NULL);
    if (!addr) {
        jniThrowIOException(env, EINVAL);
        return JNI_FALSE;
    }

    if ( (status = sBluetoothHfpInterface->volume_control((bthf_volume_type_t) volume_type,
                                volume, (bt_bdaddr_t *) addr)) != BT_STATUS_SUCCESS) {
        ALOGE("FAILED to control volume, status: %d", status);
    }
    env->ReleaseByteArrayElements(address, addr, 0);
    return (status == BT_STATUS_SUCCESS) ? JNI_TRUE : JNI_FALSE;
}

static jboolean notifyDeviceStatusNative(JNIEnv *env, jobject object,
                                         jint network_state, jint service_type, jint signal,
                                         jint battery_charge) {
    bt_status_t status;
    if (!sBluetoothHfpInterface) return JNI_FALSE;

    if ( (status = sBluetoothHfpInterface->device_status_notification
          ((bthf_network_state_t) network_state, (bthf_service_type_t) service_type,
           signal, battery_charge)) != BT_STATUS_SUCCESS) {
        ALOGE("FAILED to notify device status, status: %d", status);
    }
    return (status == BT_STATUS_SUCCESS) ? JNI_TRUE : JNI_FALSE;
}

static jboolean copsResponseNative(JNIEnv *env, jobject object, jstring operator_str,
                                              jbyteArray address) {
    jbyte *addr;
    bt_status_t status;
    const char *operator_name;
    if (!sBluetoothHfpInterface) return JNI_FALSE;

    addr = env->GetByteArrayElements(address, NULL);
    if (!addr) {
        jniThrowIOException(env, EINVAL);
        return JNI_FALSE;
    }

    operator_name = env->GetStringUTFChars(operator_str, NULL);

    if ( (status = sBluetoothHfpInterface->cops_response(operator_name,(bt_bdaddr_t *) addr))
                                                  != BT_STATUS_SUCCESS) {
        ALOGE("Failed sending cops response, status: %d", status);
    }
    env->ReleaseByteArrayElements(address, addr, 0);
    env->ReleaseStringUTFChars(operator_str, operator_name);
    return (status == BT_STATUS_SUCCESS) ? JNI_TRUE : JNI_FALSE;
}

static jboolean cindResponseNative(JNIEnv *env, jobject object,
                                   jint service, jint num_active, jint num_held, jint call_state,
                                   jint signal, jint roam, jint battery_charge, jbyteArray address) {
    jbyte *addr;
    bt_status_t status;

    ALOGI("%s: sBluetoothHfpInterface: %p", __FUNCTION__, sBluetoothHfpInterface);
    if (!sBluetoothHfpInterface) return JNI_FALSE;

    addr = env->GetByteArrayElements(address, NULL);
    if (!addr) {
        jniThrowIOException(env, EINVAL);
        return JNI_FALSE;
    }

    if ( (status = sBluetoothHfpInterface->cind_response(service, num_active, num_held,
                       (bthf_call_state_t) call_state,
                       signal, roam, battery_charge, (bt_bdaddr_t *)addr)) != BT_STATUS_SUCCESS) {
        ALOGE("Failed cind_response, status: %d", status);
    }
    env->ReleaseByteArrayElements(address, addr, 0);
    return (status == BT_STATUS_SUCCESS) ? JNI_TRUE : JNI_FALSE;
}


static jboolean atResponseStringNative(JNIEnv *env, jobject object, jstring response_str,
                                                 jbyteArray address) {
    jbyte *addr;
    bt_status_t status;
    const char *response;
    if (!sBluetoothHfpInterface) return JNI_FALSE;

    addr = env->GetByteArrayElements(address, NULL);
    if (!addr) {
        jniThrowIOException(env, EINVAL);
        return JNI_FALSE;
    }

    response = env->GetStringUTFChars(response_str, NULL);

    if ( (status = sBluetoothHfpInterface->formatted_at_response(response,
                            (bt_bdaddr_t *)addr))!= BT_STATUS_SUCCESS) {
        ALOGE("Failed formatted AT response, status: %d", status);
    }
    env->ReleaseByteArrayElements(address, addr, 0);
    env->ReleaseStringUTFChars(response_str, response);
    return (status == BT_STATUS_SUCCESS) ? JNI_TRUE : JNI_FALSE;
}

static jboolean atResponseCodeNative(JNIEnv *env, jobject object, jint response_code,
                                             jint cmee_code, jbyteArray address) {
    jbyte *addr;
    bt_status_t status;
    if (!sBluetoothHfpInterface) return JNI_FALSE;

    addr = env->GetByteArrayElements(address, NULL);
    if (!addr) {
        jniThrowIOException(env, EINVAL);
        return JNI_FALSE;
    }

    if ( (status = sBluetoothHfpInterface->at_response((bthf_at_response_t) response_code,
              cmee_code, (bt_bdaddr_t *)addr)) != BT_STATUS_SUCCESS) {
        ALOGE("Failed AT response, status: %d", status);
    }
    env->ReleaseByteArrayElements(address, addr, 0);
    return (status == BT_STATUS_SUCCESS) ? JNI_TRUE : JNI_FALSE;
}

static jboolean clccResponseNative(JNIEnv *env, jobject object, jint index, jint dir,
                                   jint callStatus, jint mode, jboolean mpty, jstring number_str,
                                   jint type, jbyteArray address) {
    jbyte *addr;
    bt_status_t status;
    const char *number = NULL;
    if (!sBluetoothHfpInterface) return JNI_FALSE;

    addr = env->GetByteArrayElements(address, NULL);
    if (!addr) {
        jniThrowIOException(env, EINVAL);
        return JNI_FALSE;
    }

    if (number_str)
        number = env->GetStringUTFChars(number_str, NULL);

    if ( (status = sBluetoothHfpInterface->clcc_response(index, (bthf_call_direction_t) dir,
                (bthf_call_state_t) callStatus,  (bthf_call_mode_t) mode,
                mpty ? BTHF_CALL_MPTY_TYPE_MULTI : BTHF_CALL_MPTY_TYPE_SINGLE,
                number, (bthf_call_addrtype_t) type, (bt_bdaddr_t *)addr)) != BT_STATUS_SUCCESS) {
        ALOGE("Failed sending CLCC response, status: %d", status);
    }
    env->ReleaseByteArrayElements(address, addr, 0);
    if (number)
        env->ReleaseStringUTFChars(number_str, number);
    return (status == BT_STATUS_SUCCESS) ? JNI_TRUE : JNI_FALSE;
}

static jboolean phoneStateChangeNative(JNIEnv *env, jobject object, jint num_active, jint num_held,
                                       jint call_state, jstring number_str, jint type) {
    bt_status_t status;
    const char *number;
    if (!sBluetoothHfpInterface) return JNI_FALSE;

    number = env->GetStringUTFChars(number_str, NULL);

    if ( (status = sBluetoothHfpInterface->phone_state_change(num_active, num_held,
                       (bthf_call_state_t) call_state, number,
                       (bthf_call_addrtype_t) type)) != BT_STATUS_SUCCESS) {
        ALOGE("Failed report phone state change, status: %d", status);
    }
    env->ReleaseStringUTFChars(number_str, number);
    return (status == BT_STATUS_SUCCESS) ? JNI_TRUE : JNI_FALSE;
}


static jboolean configureWBSNative(JNIEnv *env, jobject object, jbyteArray address,
                                   jint codec_config) {
    jbyte *addr;
    bt_status_t status;

    if (!sBluetoothHfpInterface) return JNI_FALSE;

    addr = env->GetByteArrayElements(address, NULL);
    if (!addr) {
        jniThrowIOException(env, EINVAL);
        return JNI_FALSE;
    }

    if ((status = sBluetoothHfpInterface->configure_wbs((bt_bdaddr_t *)addr,
                   (bthf_wbs_config_t)codec_config)) != BT_STATUS_SUCCESS){
        ALOGE("Failed HF WBS codec config, status: %d", status);
    }
    env->ReleaseByteArrayElements(address, addr, 0);
    return (status == BT_STATUS_SUCCESS) ? JNI_TRUE : JNI_FALSE;
}


static JNINativeMethod sMethods[] = {
    {"classInitNative", "()V", (void *) classInitNative},
    {"initializeNative", "(I)V", (void *) initializeNative},
    {"cleanupNative", "()V", (void *) cleanupNative},
    {"connectHfpNative", "([B)Z", (void *) connectHfpNative},
    {"disconnectHfpNative", "([B)Z", (void *) disconnectHfpNative},
    {"connectAudioNative", "([B)Z", (void *) connectAudioNative},
    {"disconnectAudioNative", "([B)Z", (void *) disconnectAudioNative},
    {"startVoiceRecognitionNative", "([B)Z", (void *) startVoiceRecognitionNative},
    {"stopVoiceRecognitionNative", "([B)Z", (void *) stopVoiceRecognitionNative},
    {"setVolumeNative", "(II[B)Z", (void *) setVolumeNative},
    {"notifyDeviceStatusNative", "(IIII)Z", (void *) notifyDeviceStatusNative},
    {"copsResponseNative", "(Ljava/lang/String;[B)Z", (void *) copsResponseNative},
    {"cindResponseNative", "(IIIIIII[B)Z", (void *) cindResponseNative},
    {"atResponseStringNative", "(Ljava/lang/String;[B)Z", (void *) atResponseStringNative},
    {"atResponseCodeNative", "(II[B)Z", (void *)atResponseCodeNative},
    {"clccResponseNative", "(IIIIZLjava/lang/String;I[B)Z", (void *) clccResponseNative},
    {"phoneStateChangeNative", "(IIILjava/lang/String;I)Z", (void *) phoneStateChangeNative},
    {"configureWBSNative", "([BI)Z", (void *) configureWBSNative},
};

int register_com_android_bluetooth_hfp(JNIEnv* env)
{
    return jniRegisterNativeMethods(env, "com/android/bluetooth/hfp/HeadsetStateMachine",
                                    sMethods, NELEM(sMethods));
}

} /* namespace android */
