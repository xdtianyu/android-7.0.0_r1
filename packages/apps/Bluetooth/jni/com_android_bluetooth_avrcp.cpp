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

#define LOG_TAG "BluetoothAvrcpServiceJni"

#define LOG_NDEBUG 0

#include "com_android_bluetooth.h"
#include "hardware/bt_rc.h"
#include "utils/Log.h"
#include "android_runtime/AndroidRuntime.h"

#include <string.h>

namespace android {
static jmethodID method_getRcFeatures;
static jmethodID method_getPlayStatus;
static jmethodID method_getElementAttr;
static jmethodID method_registerNotification;
static jmethodID method_volumeChangeCallback;
static jmethodID method_handlePassthroughCmd;

static const btrc_interface_t *sBluetoothAvrcpInterface = NULL;
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

static void btavrcp_remote_features_callback(bt_bdaddr_t* bd_addr, btrc_remote_features_t features) {
    ALOGI("%s", __FUNCTION__);
    jbyteArray addr;

    if (!checkCallbackThread()) {
        ALOGE("Callback: '%s' is not called on the correct thread", __FUNCTION__);
        return;
    }
    addr = sCallbackEnv->NewByteArray(sizeof(bt_bdaddr_t));
    if (!addr) {
        ALOGE("Unable to allocate byte array for bd_addr");
        checkAndClearExceptionFromCallback(sCallbackEnv, __FUNCTION__);
        return;
    }

    if (mCallbacksObj) {
        sCallbackEnv->SetByteArrayRegion(addr, 0, sizeof(bt_bdaddr_t), (jbyte*) bd_addr);
        sCallbackEnv->CallVoidMethod(mCallbacksObj, method_getRcFeatures, addr, (jint)features);
    } else {
        ALOGE("%s: mCallbacksObj is null", __FUNCTION__);
    }

    checkAndClearExceptionFromCallback(sCallbackEnv, __FUNCTION__);
    sCallbackEnv->DeleteLocalRef(addr);
}

static void btavrcp_get_play_status_callback() {
    ALOGI("%s", __FUNCTION__);

    if (!checkCallbackThread()) {
        ALOGE("Callback: '%s' is not called on the correct thread", __FUNCTION__);
        return;
    }

    if (mCallbacksObj) {
        sCallbackEnv->CallVoidMethod(mCallbacksObj, method_getPlayStatus);
    } else {
        ALOGE("%s: mCallbacksObj is null", __FUNCTION__);
    }
    checkAndClearExceptionFromCallback(sCallbackEnv, __FUNCTION__);
}

static void btavrcp_get_element_attr_callback(uint8_t num_attr, btrc_media_attr_t *p_attrs) {
    jintArray attrs;

    ALOGI("%s", __FUNCTION__);

    if (!checkCallbackThread()) {
        ALOGE("Callback: '%s' is not called on the correct thread", __FUNCTION__);
        return;
    }
    attrs = (jintArray)sCallbackEnv->NewIntArray(num_attr);
    if (!attrs) {
        ALOGE("Fail to new jintArray for attrs");
        checkAndClearExceptionFromCallback(sCallbackEnv, __FUNCTION__);
        return;
    }
    sCallbackEnv->SetIntArrayRegion(attrs, 0, num_attr, (jint *)p_attrs);
    if (mCallbacksObj) {
        sCallbackEnv->CallVoidMethod(mCallbacksObj, method_getElementAttr, (jbyte)num_attr, attrs);
    } else {
        ALOGE("%s: mCallbacksObj is null", __FUNCTION__);
    }
    checkAndClearExceptionFromCallback(sCallbackEnv, __FUNCTION__);
    sCallbackEnv->DeleteLocalRef(attrs);
}

static void btavrcp_register_notification_callback(btrc_event_id_t event_id, uint32_t param) {
    if (!checkCallbackThread()) {
        ALOGE("Callback: '%s' is not called on the correct thread", __FUNCTION__);
        return;
    }
    if (mCallbacksObj) {
        sCallbackEnv->CallVoidMethod(mCallbacksObj, method_registerNotification,
                                 (jint)event_id, (jint)param);
    } else {
        ALOGE("%s: mCallbacksObj is null", __FUNCTION__);
    }
    checkAndClearExceptionFromCallback(sCallbackEnv, __FUNCTION__);
}

static void btavrcp_volume_change_callback(uint8_t volume, uint8_t ctype) {
    ALOGI("%s", __FUNCTION__);

    if (!checkCallbackThread()) {
        ALOGE("Callback: '%s' is not called on the correct thread", __FUNCTION__);
        return;
    }
    if (mCallbacksObj) {
        sCallbackEnv->CallVoidMethod(mCallbacksObj, method_volumeChangeCallback, (jint)volume,
                                                     (jint)ctype);
    } else {
        ALOGE("%s: mCallbacksObj is null", __FUNCTION__);
    }

    checkAndClearExceptionFromCallback(sCallbackEnv, __FUNCTION__);
}

static void btavrcp_passthrough_command_callback(int id, int pressed) {
    ALOGI("%s", __FUNCTION__);

    if (!checkCallbackThread()) {
        ALOGE("Callback: '%s' is not called on the correct thread", __FUNCTION__);
        return;
    }
    if (mCallbacksObj) {
        sCallbackEnv->CallVoidMethod(mCallbacksObj, method_handlePassthroughCmd,
                      (jint)id, (jint)pressed);
    } else {
        ALOGE("%s: mCallbacksObj is null", __FUNCTION__);
    }
    checkAndClearExceptionFromCallback(sCallbackEnv, __FUNCTION__);
}

static btrc_callbacks_t sBluetoothAvrcpCallbacks = {
    sizeof(sBluetoothAvrcpCallbacks),
    btavrcp_remote_features_callback,
    btavrcp_get_play_status_callback,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    btavrcp_get_element_attr_callback,
    btavrcp_register_notification_callback,
    btavrcp_volume_change_callback,
    btavrcp_passthrough_command_callback,
};

static void classInitNative(JNIEnv* env, jclass clazz) {
    method_getRcFeatures =
        env->GetMethodID(clazz, "getRcFeatures", "([BI)V");
    method_getPlayStatus =
        env->GetMethodID(clazz, "getPlayStatus", "()V");

    method_getElementAttr =
        env->GetMethodID(clazz, "getElementAttr", "(B[I)V");

    method_registerNotification =
        env->GetMethodID(clazz, "registerNotification", "(II)V");

    method_volumeChangeCallback =
        env->GetMethodID(clazz, "volumeChangeCallback", "(II)V");

    method_handlePassthroughCmd =
        env->GetMethodID(clazz, "handlePassthroughCmd", "(II)V");

    ALOGI("%s: succeeds", __FUNCTION__);
}

static void initNative(JNIEnv *env, jobject object) {
    const bt_interface_t* btInf;
    bt_status_t status;

    if ( (btInf = getBluetoothInterface()) == NULL) {
        ALOGE("Bluetooth module is not loaded");
        return;
    }

    if (sBluetoothAvrcpInterface !=NULL) {
         ALOGW("Cleaning up Avrcp Interface before initializing...");
         sBluetoothAvrcpInterface->cleanup();
         sBluetoothAvrcpInterface = NULL;
    }

    if (mCallbacksObj != NULL) {
         ALOGW("Cleaning up Avrcp callback object");
         env->DeleteGlobalRef(mCallbacksObj);
         mCallbacksObj = NULL;
    }

    if ( (sBluetoothAvrcpInterface = (btrc_interface_t *)
          btInf->get_profile_interface(BT_PROFILE_AV_RC_ID)) == NULL) {
        ALOGE("Failed to get Bluetooth Avrcp Interface");
        return;
    }

    if ( (status = sBluetoothAvrcpInterface->init(&sBluetoothAvrcpCallbacks)) !=
         BT_STATUS_SUCCESS) {
        ALOGE("Failed to initialize Bluetooth Avrcp, status: %d", status);
        sBluetoothAvrcpInterface = NULL;
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

    if (sBluetoothAvrcpInterface !=NULL) {
        sBluetoothAvrcpInterface->cleanup();
        sBluetoothAvrcpInterface = NULL;
    }

    if (mCallbacksObj != NULL) {
        env->DeleteGlobalRef(mCallbacksObj);
        mCallbacksObj = NULL;
    }
}

static jboolean getPlayStatusRspNative(JNIEnv *env, jobject object, jint playStatus,
                                       jint songLen, jint songPos) {
    bt_status_t status;

    ALOGI("%s: sBluetoothAvrcpInterface: %p", __FUNCTION__, sBluetoothAvrcpInterface);
    if (!sBluetoothAvrcpInterface) return JNI_FALSE;

    if ((status = sBluetoothAvrcpInterface->get_play_status_rsp((btrc_play_status_t)playStatus,
                                            songLen, songPos)) != BT_STATUS_SUCCESS) {
        ALOGE("Failed get_play_status_rsp, status: %d", status);
    }

    return (status == BT_STATUS_SUCCESS) ? JNI_TRUE : JNI_FALSE;
}

  static jboolean getElementAttrRspNative(JNIEnv *env, jobject object, jbyte numAttr,
                                          jintArray attrIds, jobjectArray textArray) {
    jint *attr;
    bt_status_t status;
    jstring text;
    int i;
    btrc_element_attr_val_t *pAttrs = NULL;
    const char* textStr;

    if (!sBluetoothAvrcpInterface) return JNI_FALSE;

    if (numAttr > BTRC_MAX_ELEM_ATTR_SIZE) {
        ALOGE("get_element_attr_rsp: number of attributes exceed maximum");
        return JNI_FALSE;
    }

    pAttrs = new btrc_element_attr_val_t[numAttr];
    if (!pAttrs) {
        ALOGE("get_element_attr_rsp: not have enough memeory");
        return JNI_FALSE;
    }

    attr = env->GetIntArrayElements(attrIds, NULL);
    if (!attr) {
        delete[] pAttrs;
        jniThrowIOException(env, EINVAL);
        return JNI_FALSE;
    }

    for (i = 0; i < numAttr; ++i) {
        text = (jstring) env->GetObjectArrayElement(textArray, i);
        textStr = env->GetStringUTFChars(text, NULL);
        if (!textStr) {
            ALOGE("get_element_attr_rsp: GetStringUTFChars return NULL");
            env->DeleteLocalRef(text);
            break;
        }

        pAttrs[i].attr_id = attr[i];
        if (strlen(textStr) >= BTRC_MAX_ATTR_STR_LEN) {
            ALOGE("get_element_attr_rsp: string length exceed maximum");
            strncpy((char *)pAttrs[i].text, textStr, BTRC_MAX_ATTR_STR_LEN-1);
            pAttrs[i].text[BTRC_MAX_ATTR_STR_LEN-1] = 0;
        } else {
            strcpy((char *)pAttrs[i].text, textStr);
        }
        env->ReleaseStringUTFChars(text, textStr);
        env->DeleteLocalRef(text);
    }

    if (i < numAttr) {
        delete[] pAttrs;
        env->ReleaseIntArrayElements(attrIds, attr, 0);
        return JNI_FALSE;
    }

    if ((status = sBluetoothAvrcpInterface->get_element_attr_rsp(numAttr, pAttrs)) !=
        BT_STATUS_SUCCESS) {
        ALOGE("Failed get_element_attr_rsp, status: %d", status);
    }

    delete[] pAttrs;
    env->ReleaseIntArrayElements(attrIds, attr, 0);
    return (status == BT_STATUS_SUCCESS) ? JNI_TRUE : JNI_FALSE;
}

static jboolean registerNotificationRspPlayStatusNative(JNIEnv *env, jobject object,
                                                        jint type, jint playStatus) {
    bt_status_t status;
    btrc_register_notification_t param;

    ALOGI("%s: sBluetoothAvrcpInterface: %p", __FUNCTION__, sBluetoothAvrcpInterface);
    if (!sBluetoothAvrcpInterface) return JNI_FALSE;

    param.play_status = (btrc_play_status_t)playStatus;
    if ((status = sBluetoothAvrcpInterface->register_notification_rsp(BTRC_EVT_PLAY_STATUS_CHANGED,
                  (btrc_notification_type_t)type, &param)) != BT_STATUS_SUCCESS) {
        ALOGE("Failed register_notification_rsp play status, status: %d", status);
    }

    return (status == BT_STATUS_SUCCESS) ? JNI_TRUE : JNI_FALSE;
}

static jboolean registerNotificationRspTrackChangeNative(JNIEnv *env, jobject object,
                                                         jint type, jbyteArray track) {
    bt_status_t status;
    btrc_register_notification_t param;
    jbyte *trk;
    int i;

    ALOGI("%s: sBluetoothAvrcpInterface: %p", __FUNCTION__, sBluetoothAvrcpInterface);
    if (!sBluetoothAvrcpInterface) return JNI_FALSE;

    trk = env->GetByteArrayElements(track, NULL);
    if (!trk) {
        jniThrowIOException(env, EINVAL);
        return JNI_FALSE;
    }

    for (i = 0; i < BTRC_UID_SIZE; ++i) {
      param.track[i] = trk[i];
    }

    if ((status = sBluetoothAvrcpInterface->register_notification_rsp(BTRC_EVT_TRACK_CHANGE,
                  (btrc_notification_type_t)type, &param)) != BT_STATUS_SUCCESS) {
        ALOGE("Failed register_notification_rsp track change, status: %d", status);
    }

    env->ReleaseByteArrayElements(track, trk, 0);
    return (status == BT_STATUS_SUCCESS) ? JNI_TRUE : JNI_FALSE;
}

static jboolean registerNotificationRspPlayPosNative(JNIEnv *env, jobject object,
                                                        jint type, jint playPos) {
    bt_status_t status;
    btrc_register_notification_t param;

    if (!sBluetoothAvrcpInterface) return JNI_FALSE;

    param.song_pos = (uint32_t)playPos;
    if ((status = sBluetoothAvrcpInterface->register_notification_rsp(BTRC_EVT_PLAY_POS_CHANGED,
                  (btrc_notification_type_t)type, &param)) != BT_STATUS_SUCCESS) {
        ALOGE("Failed register_notification_rsp play position, status: %d", status);
    }

    return (status == BT_STATUS_SUCCESS) ? JNI_TRUE : JNI_FALSE;
}

static jboolean setVolumeNative(JNIEnv *env, jobject object, jint volume) {
    bt_status_t status;

    //TODO: delete test code
    ALOGI("%s: jint: %d, uint8_t: %u", __FUNCTION__, volume, (uint8_t) volume);

    ALOGI("%s: sBluetoothAvrcpInterface: %p", __FUNCTION__, sBluetoothAvrcpInterface);
    if (!sBluetoothAvrcpInterface) return JNI_FALSE;

    if ((status = sBluetoothAvrcpInterface->set_volume((uint8_t)volume)) != BT_STATUS_SUCCESS) {
        ALOGE("Failed set_volume, status: %d", status);
    }

    return (status == BT_STATUS_SUCCESS) ? JNI_TRUE : JNI_FALSE;
}

static JNINativeMethod sMethods[] = {
    {"classInitNative", "()V", (void *) classInitNative},
    {"initNative", "()V", (void *) initNative},
    {"cleanupNative", "()V", (void *) cleanupNative},
    {"getPlayStatusRspNative", "(III)Z", (void *) getPlayStatusRspNative},
    {"getElementAttrRspNative", "(B[I[Ljava/lang/String;)Z", (void *) getElementAttrRspNative},
    {"registerNotificationRspPlayStatusNative", "(II)Z",
     (void *) registerNotificationRspPlayStatusNative},
    {"registerNotificationRspTrackChangeNative", "(I[B)Z",
     (void *) registerNotificationRspTrackChangeNative},
    {"registerNotificationRspPlayPosNative", "(II)Z",
     (void *) registerNotificationRspPlayPosNative},
    {"setVolumeNative", "(I)Z",
     (void *) setVolumeNative},
};

int register_com_android_bluetooth_avrcp(JNIEnv* env)
{
    return jniRegisterNativeMethods(env, "com/android/bluetooth/avrcp/Avrcp",
                                    sMethods, NELEM(sMethods));
}

}
