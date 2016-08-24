/*
 * Copyright (c) 2014 The Android Open Source Project
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

#define LOG_TAG "BluetoothHeadsetClientServiceJni"
#define LOG_NDEBUG 0

#include "com_android_bluetooth.h"
#include "hardware/bt_hf_client.h"
#include "utils/Log.h"
#include "android_runtime/AndroidRuntime.h"

#define CHECK_CALLBACK_ENV                                                      \
   if (!checkCallbackThread()) {                                                \
       ALOGE("Callback: '%s' is not called on the correct thread", __FUNCTION__);\
       return;                                                                  \
   }

namespace android {

static bthf_client_interface_t *sBluetoothHfpClientInterface = NULL;
static jobject mCallbacksObj = NULL;
static JNIEnv *sCallbackEnv = NULL;

static jmethodID method_onConnectionStateChanged;
static jmethodID method_onAudioStateChanged;
static jmethodID method_onVrStateChanged;
static jmethodID method_onNetworkState;
static jmethodID method_onNetworkRoaming;
static jmethodID method_onNetworkSignal;
static jmethodID method_onBatteryLevel;
static jmethodID method_onCurrentOperator;
static jmethodID method_onCall;
static jmethodID method_onCallSetup;
static jmethodID method_onCallHeld;
static jmethodID method_onRespAndHold;
static jmethodID method_onClip;
static jmethodID method_onCallWaiting;
static jmethodID method_onCurrentCalls;
static jmethodID method_onVolumeChange;
static jmethodID method_onCmdResult;
static jmethodID method_onSubscriberInfo;
static jmethodID method_onInBandRing;
static jmethodID method_onLastVoiceTagNumber;
static jmethodID method_onRingIndication;

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

static void connection_state_cb(bthf_client_connection_state_t state, unsigned int peer_feat, unsigned int chld_feat, bt_bdaddr_t *bd_addr) {
    jbyteArray addr;

    CHECK_CALLBACK_ENV

    addr = sCallbackEnv->NewByteArray(sizeof(bt_bdaddr_t));
    if (!addr) {
        ALOGE("Fail to new jbyteArray bd addr for connection state");
        checkAndClearExceptionFromCallback(sCallbackEnv, __FUNCTION__);
        return;
    }

    sCallbackEnv->SetByteArrayRegion(addr, 0, sizeof(bt_bdaddr_t), (jbyte*) bd_addr);
    sCallbackEnv->CallVoidMethod(mCallbacksObj, method_onConnectionStateChanged, (jint) state, (jint) peer_feat, (jint) chld_feat, addr);
    checkAndClearExceptionFromCallback(sCallbackEnv, __FUNCTION__);
    sCallbackEnv->DeleteLocalRef(addr);
}

static void audio_state_cb(bthf_client_audio_state_t state, bt_bdaddr_t *bd_addr) {
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

static void vr_cmd_cb(bthf_client_vr_state_t state) {
    CHECK_CALLBACK_ENV
    sCallbackEnv->CallVoidMethod(mCallbacksObj, method_onVrStateChanged, (jint) state);
    checkAndClearExceptionFromCallback(sCallbackEnv, __FUNCTION__);
}

static void network_state_cb (bthf_client_network_state_t state) {
    CHECK_CALLBACK_ENV
    sCallbackEnv->CallVoidMethod(mCallbacksObj, method_onNetworkState, (jint) state);
    checkAndClearExceptionFromCallback(sCallbackEnv, __FUNCTION__);
}

static void network_roaming_cb (bthf_client_service_type_t type) {
    CHECK_CALLBACK_ENV
    sCallbackEnv->CallVoidMethod(mCallbacksObj, method_onNetworkRoaming, (jint) type);
    checkAndClearExceptionFromCallback(sCallbackEnv, __FUNCTION__);
}

static void network_signal_cb (int signal) {
    CHECK_CALLBACK_ENV
    sCallbackEnv->CallVoidMethod(mCallbacksObj, method_onNetworkSignal, (jint) signal);
    checkAndClearExceptionFromCallback(sCallbackEnv, __FUNCTION__);
}

static void battery_level_cb (int level) {
    CHECK_CALLBACK_ENV
    sCallbackEnv->CallVoidMethod(mCallbacksObj, method_onBatteryLevel, (jint) level);
    checkAndClearExceptionFromCallback(sCallbackEnv, __FUNCTION__);
}

static void current_operator_cb (const char *name) {
    jstring js_name;

    CHECK_CALLBACK_ENV

    js_name = sCallbackEnv->NewStringUTF(name);
    sCallbackEnv->CallVoidMethod(mCallbacksObj, method_onCurrentOperator, js_name);
    checkAndClearExceptionFromCallback(sCallbackEnv, __FUNCTION__);
    sCallbackEnv->DeleteLocalRef(js_name);
}

static void call_cb (bthf_client_call_t call) {
    CHECK_CALLBACK_ENV
    sCallbackEnv->CallVoidMethod(mCallbacksObj, method_onCall, (jint) call);
    checkAndClearExceptionFromCallback(sCallbackEnv, __FUNCTION__);
}

static void callsetup_cb (bthf_client_callsetup_t callsetup) {
    CHECK_CALLBACK_ENV
    sCallbackEnv->CallVoidMethod(mCallbacksObj, method_onCallSetup, (jint) callsetup);
    checkAndClearExceptionFromCallback(sCallbackEnv, __FUNCTION__);
}

static void callheld_cb (bthf_client_callheld_t callheld) {
    CHECK_CALLBACK_ENV
    sCallbackEnv->CallVoidMethod(mCallbacksObj, method_onCallHeld, (jint) callheld);
    checkAndClearExceptionFromCallback(sCallbackEnv, __FUNCTION__);
}

static void resp_and_hold_cb (bthf_client_resp_and_hold_t resp_and_hold) {
    CHECK_CALLBACK_ENV
    sCallbackEnv->CallVoidMethod(mCallbacksObj, method_onRespAndHold, (jint) resp_and_hold);
    checkAndClearExceptionFromCallback(sCallbackEnv, __FUNCTION__);
}

static void clip_cb (const char *number) {
    jstring js_number;

    CHECK_CALLBACK_ENV

    js_number = sCallbackEnv->NewStringUTF(number);
    sCallbackEnv->CallVoidMethod(mCallbacksObj, method_onClip, js_number);
    checkAndClearExceptionFromCallback(sCallbackEnv, __FUNCTION__);
    sCallbackEnv->DeleteLocalRef(js_number);
}

static void call_waiting_cb (const char *number) {
    jstring js_number;

    CHECK_CALLBACK_ENV

    js_number = sCallbackEnv->NewStringUTF(number);
    sCallbackEnv->CallVoidMethod(mCallbacksObj, method_onCallWaiting, js_number);
    checkAndClearExceptionFromCallback(sCallbackEnv, __FUNCTION__);
    sCallbackEnv->DeleteLocalRef(js_number);
}

static void current_calls_cb (int index, bthf_client_call_direction_t dir,
                                            bthf_client_call_state_t state,
                                            bthf_client_call_mpty_type_t mpty,
                                            const char *number) {
    jstring js_number;

    CHECK_CALLBACK_ENV

    js_number = sCallbackEnv->NewStringUTF(number);
    sCallbackEnv->CallVoidMethod(mCallbacksObj, method_onCurrentCalls, index, dir, state, mpty, js_number);
    checkAndClearExceptionFromCallback(sCallbackEnv, __FUNCTION__);
    sCallbackEnv->DeleteLocalRef(js_number);
}

static void volume_change_cb (bthf_client_volume_type_t type, int volume) {
    CHECK_CALLBACK_ENV
    sCallbackEnv->CallVoidMethod(mCallbacksObj, method_onVolumeChange, (jint) type, (jint) volume);
    checkAndClearExceptionFromCallback(sCallbackEnv, __FUNCTION__);
}

static void cmd_complete_cb (bthf_client_cmd_complete_t type, int cme) {
    CHECK_CALLBACK_ENV
    sCallbackEnv->CallVoidMethod(mCallbacksObj, method_onCmdResult, (jint) type, (jint) cme);
    checkAndClearExceptionFromCallback(sCallbackEnv, __FUNCTION__);
}

static void subscriber_info_cb (const char *name, bthf_client_subscriber_service_type_t type) {
    jstring js_name;

    CHECK_CALLBACK_ENV

    js_name = sCallbackEnv->NewStringUTF(name);
    sCallbackEnv->CallVoidMethod(mCallbacksObj, method_onSubscriberInfo, js_name, (jint) type);
    checkAndClearExceptionFromCallback(sCallbackEnv, __FUNCTION__);
    sCallbackEnv->DeleteLocalRef(js_name);
}

static void in_band_ring_cb (bthf_client_in_band_ring_state_t in_band) {
    CHECK_CALLBACK_ENV
    sCallbackEnv->CallVoidMethod(mCallbacksObj, method_onInBandRing, (jint) in_band);
    checkAndClearExceptionFromCallback(sCallbackEnv, __FUNCTION__);
}

static void last_voice_tag_number_cb (const char *number) {
    jstring js_number;

    CHECK_CALLBACK_ENV

    js_number = sCallbackEnv->NewStringUTF(number);
    sCallbackEnv->CallVoidMethod(mCallbacksObj, method_onLastVoiceTagNumber, js_number);
    checkAndClearExceptionFromCallback(sCallbackEnv, __FUNCTION__);
    sCallbackEnv->DeleteLocalRef(js_number);
}

static void ring_indication_cb () {
    CHECK_CALLBACK_ENV
    sCallbackEnv->CallVoidMethod(mCallbacksObj, method_onRingIndication);
    checkAndClearExceptionFromCallback(sCallbackEnv, __FUNCTION__);
}

static bthf_client_callbacks_t sBluetoothHfpClientCallbacks = {
    sizeof(sBluetoothHfpClientCallbacks),
    connection_state_cb,
    audio_state_cb,
    vr_cmd_cb,
    network_state_cb,
    network_roaming_cb,
    network_signal_cb,
    battery_level_cb,
    current_operator_cb,
    call_cb,
    callsetup_cb,
    callheld_cb,
    resp_and_hold_cb,
    clip_cb,
    call_waiting_cb,
    current_calls_cb,
    volume_change_cb,
    cmd_complete_cb,
    subscriber_info_cb,
    in_band_ring_cb,
    last_voice_tag_number_cb,
    ring_indication_cb,
};

static void classInitNative(JNIEnv* env, jclass clazz) {
    method_onConnectionStateChanged = env->GetMethodID(clazz, "onConnectionStateChanged", "(III[B)V");
    method_onAudioStateChanged = env->GetMethodID(clazz, "onAudioStateChanged", "(I[B)V");
    method_onVrStateChanged = env->GetMethodID(clazz, "onVrStateChanged", "(I)V");
    method_onNetworkState = env->GetMethodID(clazz, "onNetworkState", "(I)V");
    method_onNetworkRoaming = env->GetMethodID(clazz, "onNetworkRoaming", "(I)V");
    method_onNetworkSignal = env->GetMethodID(clazz, "onNetworkSignal", "(I)V");
    method_onBatteryLevel = env->GetMethodID(clazz, "onBatteryLevel", "(I)V");
    method_onCurrentOperator = env->GetMethodID(clazz, "onCurrentOperator", "(Ljava/lang/String;)V");
    method_onCall = env->GetMethodID(clazz, "onCall", "(I)V");
    method_onCallSetup = env->GetMethodID(clazz, "onCallSetup", "(I)V");
    method_onCallHeld = env->GetMethodID(clazz, "onCallHeld", "(I)V");
    method_onRespAndHold = env->GetMethodID(clazz, "onRespAndHold", "(I)V");
    method_onClip = env->GetMethodID(clazz, "onClip", "(Ljava/lang/String;)V");
    method_onCallWaiting = env->GetMethodID(clazz, "onCallWaiting", "(Ljava/lang/String;)V");
    method_onCurrentCalls = env->GetMethodID(clazz, "onCurrentCalls", "(IIIILjava/lang/String;)V");
    method_onVolumeChange = env->GetMethodID(clazz, "onVolumeChange", "(II)V");
    method_onCmdResult = env->GetMethodID(clazz, "onCmdResult", "(II)V");
    method_onSubscriberInfo = env->GetMethodID(clazz, "onSubscriberInfo", "(Ljava/lang/String;I)V");
    method_onInBandRing = env->GetMethodID(clazz, "onInBandRing", "(I)V");
    method_onLastVoiceTagNumber = env->GetMethodID(clazz, "onLastVoiceTagNumber",
        "(Ljava/lang/String;)V");
    method_onRingIndication = env->GetMethodID(clazz, "onRingIndication","()V");

    ALOGI("%s succeeds", __FUNCTION__);
}

static void initializeNative(JNIEnv *env, jobject object) {
    const bt_interface_t* btInf;
    bt_status_t status;

    btInf = getBluetoothInterface();
    if (btInf == NULL) {
        ALOGE("Bluetooth module is not loaded");
        return;
    }

    if (sBluetoothHfpClientInterface != NULL) {
        ALOGW("Cleaning up Bluetooth HFP Client Interface before initializing");
        sBluetoothHfpClientInterface->cleanup();
        sBluetoothHfpClientInterface = NULL;
    }

    if (mCallbacksObj != NULL) {
        ALOGW("Cleaning up Bluetooth HFP Client callback object");
        env->DeleteGlobalRef(mCallbacksObj);
        mCallbacksObj = NULL;
    }

    sBluetoothHfpClientInterface = (bthf_client_interface_t *)
            btInf->get_profile_interface(BT_PROFILE_HANDSFREE_CLIENT_ID);
    if (sBluetoothHfpClientInterface  == NULL) {
        ALOGE("Failed to get Bluetooth HFP Client Interface");
        return;
    }

    status = sBluetoothHfpClientInterface->init(&sBluetoothHfpClientCallbacks);
    if (status != BT_STATUS_SUCCESS) {
        ALOGE("Failed to initialize Bluetooth HFP Client, status: %d", status);
        sBluetoothHfpClientInterface = NULL;
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

    if (sBluetoothHfpClientInterface != NULL) {
        ALOGW("Cleaning up Bluetooth HFP Client Interface...");
        sBluetoothHfpClientInterface->cleanup();
        sBluetoothHfpClientInterface = NULL;
    }

    if (mCallbacksObj != NULL) {
        ALOGW("Cleaning up Bluetooth HFP Client callback object");
        env->DeleteGlobalRef(mCallbacksObj);
        mCallbacksObj = NULL;
    }
}

static jboolean connectNative(JNIEnv *env, jobject object, jbyteArray address) {
    jbyte *addr;
    bt_status_t status;

    if (!sBluetoothHfpClientInterface) return JNI_FALSE;

    addr = env->GetByteArrayElements(address, NULL);
    if (!addr) {
        jniThrowIOException(env, EINVAL);
        return JNI_FALSE;
    }

    if ((status = sBluetoothHfpClientInterface->connect((bt_bdaddr_t *)addr)) != BT_STATUS_SUCCESS) {
        ALOGE("Failed AG connection, status: %d", status);
    }
    env->ReleaseByteArrayElements(address, addr, 0);
    return (status == BT_STATUS_SUCCESS) ? JNI_TRUE : JNI_FALSE;
}

static jboolean disconnectNative(JNIEnv *env, jobject object, jbyteArray address) {
    jbyte *addr;
    bt_status_t status;

    if (!sBluetoothHfpClientInterface) return JNI_FALSE;

    addr = env->GetByteArrayElements(address, NULL);
    if (!addr) {
        jniThrowIOException(env, EINVAL);
        return JNI_FALSE;
    }

    if ( (status = sBluetoothHfpClientInterface->disconnect((bt_bdaddr_t *)addr)) != BT_STATUS_SUCCESS) {
        ALOGE("Failed AG disconnection, status: %d", status);
    }
    env->ReleaseByteArrayElements(address, addr, 0);
    return (status == BT_STATUS_SUCCESS) ? JNI_TRUE : JNI_FALSE;
}

static jboolean connectAudioNative(JNIEnv *env, jobject object, jbyteArray address) {
    jbyte *addr;
    bt_status_t status;

    if (!sBluetoothHfpClientInterface) return JNI_FALSE;

    addr = env->GetByteArrayElements(address, NULL);
    if (!addr) {
        jniThrowIOException(env, EINVAL);
        return JNI_FALSE;
    }

    if ( (status = sBluetoothHfpClientInterface->connect_audio((bt_bdaddr_t *)addr)) !=
         BT_STATUS_SUCCESS) {
        ALOGE("Failed AG audio connection, status: %d", status);
    }
    env->ReleaseByteArrayElements(address, addr, 0);
    return (status == BT_STATUS_SUCCESS) ? JNI_TRUE : JNI_FALSE;
}

static jboolean disconnectAudioNative(JNIEnv *env, jobject object, jbyteArray address) {
    jbyte *addr;
    bt_status_t status;

    if (!sBluetoothHfpClientInterface) return JNI_FALSE;

    addr = env->GetByteArrayElements(address, NULL);
    if (!addr) {
        jniThrowIOException(env, EINVAL);
        return JNI_FALSE;
    }

    if ( (status = sBluetoothHfpClientInterface->disconnect_audio((bt_bdaddr_t *) addr)) !=
         BT_STATUS_SUCCESS) {
        ALOGE("Failed AG audio disconnection, status: %d", status);
    }
    env->ReleaseByteArrayElements(address, addr, 0);
    return (status == BT_STATUS_SUCCESS) ? JNI_TRUE : JNI_FALSE;
}

static jboolean startVoiceRecognitionNative(JNIEnv *env, jobject object) {
    bt_status_t status;
    if (!sBluetoothHfpClientInterface) return JNI_FALSE;

    if ( (status = sBluetoothHfpClientInterface->start_voice_recognition()) != BT_STATUS_SUCCESS) {
        ALOGE("Failed to start voice recognition, status: %d", status);
    }
    return (status == BT_STATUS_SUCCESS) ? JNI_TRUE : JNI_FALSE;
}

static jboolean stopVoiceRecognitionNative(JNIEnv *env, jobject object) {
    bt_status_t status;
    if (!sBluetoothHfpClientInterface) return JNI_FALSE;

    if ( (status = sBluetoothHfpClientInterface->stop_voice_recognition()) != BT_STATUS_SUCCESS) {
        ALOGE("Failed to stop voice recognition, status: %d", status);
    }
    return (status == BT_STATUS_SUCCESS) ? JNI_TRUE : JNI_FALSE;
}

static jboolean setVolumeNative(JNIEnv *env, jobject object, jint volume_type, jint volume) {
    bt_status_t status;
    if (!sBluetoothHfpClientInterface) return JNI_FALSE;

    if ( (status = sBluetoothHfpClientInterface->volume_control((bthf_client_volume_type_t) volume_type,
                                                          volume)) != BT_STATUS_SUCCESS) {
        ALOGE("FAILED to control volume, status: %d", status);
    }
    return (status == BT_STATUS_SUCCESS) ? JNI_TRUE : JNI_FALSE;
}

static jboolean dialNative(JNIEnv *env, jobject object, jstring number_str) {
    bt_status_t status;
    const char *number = NULL;
    if (!sBluetoothHfpClientInterface) return JNI_FALSE;

    if (number_str != NULL) {
        number = env->GetStringUTFChars(number_str, NULL);
    }

    if ( (status = sBluetoothHfpClientInterface->dial(number)) != BT_STATUS_SUCCESS) {
        ALOGE("Failed to dial, status: %d", status);
    }
    if (number != NULL) {
        env->ReleaseStringUTFChars(number_str, number);
    }
    return (status == BT_STATUS_SUCCESS) ? JNI_TRUE : JNI_FALSE;
}

static jboolean dialMemoryNative(JNIEnv *env, jobject object, jint location) {
    bt_status_t status;

    if (!sBluetoothHfpClientInterface) return JNI_FALSE;

    if ( (status = sBluetoothHfpClientInterface->dial_memory((int)location)) != BT_STATUS_SUCCESS) {
        ALOGE("Failed to dial from memory, status: %d", status);
    }
    return (status == BT_STATUS_SUCCESS) ? JNI_TRUE : JNI_FALSE;
}

static jboolean handleCallActionNative(JNIEnv *env, jobject object, jint action, jint index) {
    bt_status_t status;

    if (!sBluetoothHfpClientInterface) return JNI_FALSE;

    if ( (status = sBluetoothHfpClientInterface->handle_call_action((bthf_client_call_action_t)action, (int)index)) != BT_STATUS_SUCCESS) {
        ALOGE("Failed to enter private mode, status: %d", status);
    }
    return (status == BT_STATUS_SUCCESS) ? JNI_TRUE : JNI_FALSE;
}

static jboolean queryCurrentCallsNative(JNIEnv *env, jobject object) {
    bt_status_t status;

    if (!sBluetoothHfpClientInterface) return JNI_FALSE;

    if ( (status = sBluetoothHfpClientInterface->query_current_calls()) != BT_STATUS_SUCCESS) {
        ALOGE("Failed to query current calls, status: %d", status);
    }
    return (status == BT_STATUS_SUCCESS) ? JNI_TRUE : JNI_FALSE;
}

static jboolean queryCurrentOperatorNameNative(JNIEnv *env, jobject object) {
    bt_status_t status;

    if (!sBluetoothHfpClientInterface) return JNI_FALSE;

    if ( (status = sBluetoothHfpClientInterface->query_current_operator_name()) != BT_STATUS_SUCCESS) {
        ALOGE("Failed to query current operator name, status: %d", status);
    }
    return (status == BT_STATUS_SUCCESS) ? JNI_TRUE : JNI_FALSE;
}

static jboolean retrieveSubscriberInfoNative(JNIEnv *env, jobject object) {
    bt_status_t status;

    if (!sBluetoothHfpClientInterface) return JNI_FALSE;

    if ( (status = sBluetoothHfpClientInterface->retrieve_subscriber_info()) != BT_STATUS_SUCCESS) {
        ALOGE("Failed to retrieve subscriber info, status: %d", status);
    }
    return (status == BT_STATUS_SUCCESS) ? JNI_TRUE : JNI_FALSE;
}

static jboolean sendDtmfNative(JNIEnv *env, jobject object, jbyte code) {
    bt_status_t status;

    if (!sBluetoothHfpClientInterface) return JNI_FALSE;

    if ( (status = sBluetoothHfpClientInterface->send_dtmf((char)code)) != BT_STATUS_SUCCESS) {
        ALOGE("Failed to send DTMF, status: %d", status);
    }
    return (status == BT_STATUS_SUCCESS) ? JNI_TRUE : JNI_FALSE;
}

static jboolean requestLastVoiceTagNumberNative(JNIEnv *env, jobject object) {
    bt_status_t status;

    if (!sBluetoothHfpClientInterface) return JNI_FALSE;

    if ( (status = sBluetoothHfpClientInterface->request_last_voice_tag_number()) != BT_STATUS_SUCCESS) {
        ALOGE("Failed to request last Voice Tag number, status: %d", status);
    }
    return (status == BT_STATUS_SUCCESS) ? JNI_TRUE : JNI_FALSE;
}

static jboolean sendATCmdNative(JNIEnv *env, jobject object, jint cmd,
                                jint val1, jint val2, jstring arg_str) {
    bt_status_t status;
    const char *arg = NULL;

    if (!sBluetoothHfpClientInterface) return JNI_FALSE;

    if (arg_str != NULL) {
        arg = env->GetStringUTFChars(arg_str, NULL);
    }

    if ((status = sBluetoothHfpClientInterface->send_at_cmd(cmd,val1,val2,arg)) !=
            BT_STATUS_SUCCESS) {
        ALOGE("Failed to send cmd, status: %d", status);
    }

    if (arg != NULL) {
        env->ReleaseStringUTFChars(arg_str, arg);
    }
    return (status == BT_STATUS_SUCCESS) ? JNI_TRUE : JNI_FALSE;
}

static JNINativeMethod sMethods[] = {
    {"classInitNative", "()V", (void *) classInitNative},
    {"initializeNative", "()V", (void *) initializeNative},
    {"cleanupNative", "()V", (void *) cleanupNative},
    {"connectNative", "([B)Z", (void *) connectNative},
    {"disconnectNative", "([B)Z", (void *) disconnectNative},
    {"connectAudioNative", "([B)Z", (void *) connectAudioNative},
    {"disconnectAudioNative", "([B)Z", (void *) disconnectAudioNative},
    {"startVoiceRecognitionNative", "()Z", (void *) startVoiceRecognitionNative},
    {"stopVoiceRecognitionNative", "()Z", (void *) stopVoiceRecognitionNative},
    {"setVolumeNative", "(II)Z", (void *) setVolumeNative},
    {"dialNative", "(Ljava/lang/String;)Z", (void *) dialNative},
    {"dialMemoryNative", "(I)Z", (void *) dialMemoryNative},
    {"handleCallActionNative", "(II)Z", (void *) handleCallActionNative},
    {"queryCurrentCallsNative", "()Z", (void *) queryCurrentCallsNative},
    {"queryCurrentOperatorNameNative", "()Z", (void *) queryCurrentOperatorNameNative},
    {"retrieveSubscriberInfoNative", "()Z", (void *) retrieveSubscriberInfoNative},
    {"sendDtmfNative", "(B)Z", (void *) sendDtmfNative},
    {"requestLastVoiceTagNumberNative", "()Z",
        (void *) requestLastVoiceTagNumberNative},
    {"sendATCmdNative", "(IIILjava/lang/String;)Z", (void *) sendATCmdNative},
};

int register_com_android_bluetooth_hfpclient(JNIEnv* env)
{
    return jniRegisterNativeMethods(env, "com/android/bluetooth/hfpclient/HeadsetClientStateMachine",
                                    sMethods, NELEM(sMethods));
}

} /* namespace android */
