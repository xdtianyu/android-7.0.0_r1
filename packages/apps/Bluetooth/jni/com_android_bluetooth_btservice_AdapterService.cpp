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

#define LOG_TAG "BluetoothServiceJni"
#include "com_android_bluetooth.h"
#include "hardware/bt_sock.h"
#include "utils/Log.h"
#include "utils/misc.h"
#include "cutils/properties.h"
#include "android_runtime/AndroidRuntime.h"
#include "android_runtime/Log.h"

#include <string.h>
#include <pthread.h>

#include <sys/stat.h>
#include <fcntl.h>

namespace android {

#define OOB_TK_SIZE 16

#define ADDITIONAL_NREFS 50
static jmethodID method_stateChangeCallback;
static jmethodID method_adapterPropertyChangedCallback;
static jmethodID method_devicePropertyChangedCallback;
static jmethodID method_deviceFoundCallback;
static jmethodID method_pinRequestCallback;
static jmethodID method_sspRequestCallback;
static jmethodID method_bondStateChangeCallback;
static jmethodID method_aclStateChangeCallback;
static jmethodID method_discoveryStateChangeCallback;
static jmethodID method_setWakeAlarm;
static jmethodID method_acquireWakeLock;
static jmethodID method_releaseWakeLock;
static jmethodID method_energyInfo;

static struct {
    jclass clazz;
    jmethodID constructor;
} android_bluetooth_UidTraffic;

static const bt_interface_t *sBluetoothInterface = NULL;
static const btsock_interface_t *sBluetoothSocketInterface = NULL;
static JNIEnv *callbackEnv = NULL;

static jobject sJniAdapterServiceObj;
static jobject sJniCallbacksObj;
static jfieldID sJniCallbacksField;


const bt_interface_t* getBluetoothInterface() {
    return sBluetoothInterface;
}

JNIEnv* getCallbackEnv() {
    return callbackEnv;
}

void checkAndClearExceptionFromCallback(JNIEnv* env,
                                               const char* methodName) {
    if (env->ExceptionCheck()) {
        ALOGE("An exception was thrown by callback '%s'.", methodName);
        LOGE_EX(env);
        env->ExceptionClear();
    }
}

static bool checkCallbackThread() {
    JNIEnv* env = AndroidRuntime::getJNIEnv();
    if (callbackEnv != env || callbackEnv == NULL) {
        ALOGE("Callback env check fail: env: %p, callback: %p", env, callbackEnv);
        return false;
    }
    return true;
}

static void adapter_state_change_callback(bt_state_t status) {
    if (!checkCallbackThread()) {
       ALOGE("Callback: '%s' is not called on the correct thread", __FUNCTION__);
       return;
    }
    ALOGV("%s: Status is: %d", __FUNCTION__, status);

    callbackEnv->CallVoidMethod(sJniCallbacksObj, method_stateChangeCallback, (jint)status);

    checkAndClearExceptionFromCallback(callbackEnv, __FUNCTION__);
}

static int get_properties(int num_properties, bt_property_t *properties, jintArray *types,
                        jobjectArray *props) {
    jbyteArray propVal;
    for (int i = 0; i < num_properties; i++) {
        propVal = callbackEnv->NewByteArray(properties[i].len);
        if (propVal == NULL) goto Fail;

        callbackEnv->SetByteArrayRegion(propVal, 0, properties[i].len,
                                             (jbyte*)properties[i].val);
        callbackEnv->SetObjectArrayElement(*props, i, propVal);
        // Delete reference to propVal
        callbackEnv->DeleteLocalRef(propVal);
        callbackEnv->SetIntArrayRegion(*types, i, 1, (jint *)&properties[i].type);
    }
    return 0;
Fail:
    if (propVal) callbackEnv->DeleteLocalRef(propVal);
    ALOGE("Error while allocation of array in %s", __FUNCTION__);
    return -1;
}

static void adapter_properties_callback(bt_status_t status, int num_properties,
                                        bt_property_t *properties) {
    jobjectArray props;
    jintArray types;
    jbyteArray val;
    jclass mclass;

    if (!checkCallbackThread()) {
       ALOGE("Callback: '%s' is not called on the correct thread", __FUNCTION__);
       return;
    }

    ALOGV("%s: Status is: %d, Properties: %d", __FUNCTION__, status, num_properties);

    if (status != BT_STATUS_SUCCESS) {
        ALOGE("%s: Status %d is incorrect", __FUNCTION__, status);
        return;
    }

    val = (jbyteArray) callbackEnv->NewByteArray(num_properties);
    if (val == NULL) {
        ALOGE("%s: Error allocating byteArray", __FUNCTION__);
        return;
    }

    mclass = callbackEnv->GetObjectClass(val);

    /* (BT) Initialize the jobjectArray and jintArray here itself and send the
     initialized array pointers alone to get_properties */

    props = callbackEnv->NewObjectArray(num_properties, mclass,
                                             NULL);
    if (props == NULL) {
        ALOGE("%s: Error allocating object Array for properties", __FUNCTION__);
        return;
    }

    types = (jintArray)callbackEnv->NewIntArray(num_properties);

    if (types == NULL) {
        ALOGE("%s: Error allocating int Array for values", __FUNCTION__);
        return;
    }
    // Delete the reference to val and mclass
    callbackEnv->DeleteLocalRef(mclass);
    callbackEnv->DeleteLocalRef(val);

    if (get_properties(num_properties, properties, &types, &props) < 0) {
        if (props) callbackEnv->DeleteLocalRef(props);
        if (types) callbackEnv->DeleteLocalRef(types);
        return;
    }

    callbackEnv->CallVoidMethod(sJniCallbacksObj, method_adapterPropertyChangedCallback, types,
                                props);
    checkAndClearExceptionFromCallback(callbackEnv, __FUNCTION__);
    callbackEnv->DeleteLocalRef(props);
    callbackEnv->DeleteLocalRef(types);
    return;

}

static void remote_device_properties_callback(bt_status_t status, bt_bdaddr_t *bd_addr,
                                              int num_properties, bt_property_t *properties) {
    if (!checkCallbackThread()) {
       ALOGE("Callback: '%s' is not called on the correct thread", __FUNCTION__);
       return;
    }

    ALOGV("%s: Status is: %d, Properties: %d", __FUNCTION__, status, num_properties);

    if (status != BT_STATUS_SUCCESS) {
        ALOGE("%s: Status %d is incorrect", __FUNCTION__, status);
        return;
    }

    callbackEnv->PushLocalFrame(ADDITIONAL_NREFS);

    jobjectArray props;
    jbyteArray addr;
    jintArray types;
    jbyteArray val;
    jclass mclass;

    val = (jbyteArray) callbackEnv->NewByteArray(num_properties);
    if (val == NULL) {
        ALOGE("%s: Error allocating byteArray", __FUNCTION__);
        return;
    }

    mclass = callbackEnv->GetObjectClass(val);

    /* Initialize the jobjectArray and jintArray here itself and send the
     initialized array pointers alone to get_properties */

    props = callbackEnv->NewObjectArray(num_properties, mclass,
                                             NULL);
    if (props == NULL) {
        ALOGE("%s: Error allocating object Array for properties", __FUNCTION__);
        return;
    }

    types = (jintArray)callbackEnv->NewIntArray(num_properties);

    if (types == NULL) {
        ALOGE("%s: Error allocating int Array for values", __FUNCTION__);
        return;
    }
    // Delete the reference to val and mclass
    callbackEnv->DeleteLocalRef(mclass);
    callbackEnv->DeleteLocalRef(val);

    addr = callbackEnv->NewByteArray(sizeof(bt_bdaddr_t));
    if (addr == NULL) goto Fail;
    if (addr) callbackEnv->SetByteArrayRegion(addr, 0, sizeof(bt_bdaddr_t), (jbyte*)bd_addr);

    if (get_properties(num_properties, properties, &types, &props) < 0) {
        if (props) callbackEnv->DeleteLocalRef(props);
        if (types) callbackEnv->DeleteLocalRef(types);
        callbackEnv->PopLocalFrame(NULL);
        return;
    }

    callbackEnv->CallVoidMethod(sJniCallbacksObj, method_devicePropertyChangedCallback, addr,
                                types, props);
    checkAndClearExceptionFromCallback(callbackEnv, __FUNCTION__);
    callbackEnv->DeleteLocalRef(props);
    callbackEnv->DeleteLocalRef(types);
    callbackEnv->DeleteLocalRef(addr);
    callbackEnv->PopLocalFrame(NULL);
    return;

Fail:
    ALOGE("Error while allocation byte array in %s", __FUNCTION__);
}


static void device_found_callback(int num_properties, bt_property_t *properties) {
    jbyteArray addr = NULL;
    int addr_index;

    for (int i = 0; i < num_properties; i++) {
        if (properties[i].type == BT_PROPERTY_BDADDR) {
            addr = callbackEnv->NewByteArray(properties[i].len);
            if (addr) {
                callbackEnv->SetByteArrayRegion(addr, 0, properties[i].len,
                                                (jbyte*)properties[i].val);
                addr_index = i;
            } else {
                ALOGE("Address is NULL (unable to allocate) in %s", __FUNCTION__);
                return;
            }
        }
    }
    if (addr == NULL) {
        ALOGE("Address is NULL in %s", __FUNCTION__);
        return;
    }

    ALOGV("%s: Properties: %d, Address: %s", __FUNCTION__, num_properties,
        (const char *)properties[addr_index].val);

    remote_device_properties_callback(BT_STATUS_SUCCESS, (bt_bdaddr_t *)properties[addr_index].val,
                                      num_properties, properties);

    callbackEnv->CallVoidMethod(sJniCallbacksObj, method_deviceFoundCallback, addr);
    checkAndClearExceptionFromCallback(callbackEnv, __FUNCTION__);
    callbackEnv->DeleteLocalRef(addr);
}

static void bond_state_changed_callback(bt_status_t status, bt_bdaddr_t *bd_addr,
                                        bt_bond_state_t state) {
    jbyteArray addr;
    if (!checkCallbackThread()) {
       ALOGE("Callback: '%s' is not called on the correct thread", __FUNCTION__);
       return;
    }
    if (!bd_addr) {
        ALOGE("Address is null in %s", __FUNCTION__);
        return;
    }
    addr = callbackEnv->NewByteArray(sizeof(bt_bdaddr_t));
    if (addr == NULL) {
       ALOGE("Address allocation failed in %s", __FUNCTION__);
       return;
    }
    callbackEnv->SetByteArrayRegion(addr, 0, sizeof(bt_bdaddr_t), (jbyte *)bd_addr);

    callbackEnv->CallVoidMethod(sJniCallbacksObj, method_bondStateChangeCallback, (jint) status,
                                addr, (jint)state);
    checkAndClearExceptionFromCallback(callbackEnv, __FUNCTION__);
    callbackEnv->DeleteLocalRef(addr);
}

static void acl_state_changed_callback(bt_status_t status, bt_bdaddr_t *bd_addr,
                                       bt_acl_state_t state)
{
    jbyteArray addr;
    if (!checkCallbackThread()) {
       ALOGE("Callback: '%s' is not called on the correct thread", __FUNCTION__);
       return;
    }
    if (!bd_addr) {
        ALOGE("Address is null in %s", __FUNCTION__);
        return;
    }
    addr = callbackEnv->NewByteArray(sizeof(bt_bdaddr_t));
    if (addr == NULL) {
       ALOGE("Address allocation failed in %s", __FUNCTION__);
       return;
    }
    callbackEnv->SetByteArrayRegion(addr, 0, sizeof(bt_bdaddr_t), (jbyte *)bd_addr);

    callbackEnv->CallVoidMethod(sJniCallbacksObj, method_aclStateChangeCallback, (jint) status,
                                addr, (jint)state);
    checkAndClearExceptionFromCallback(callbackEnv, __FUNCTION__);
    callbackEnv->DeleteLocalRef(addr);
}

static void discovery_state_changed_callback(bt_discovery_state_t state) {
    if (!checkCallbackThread()) {
       ALOGE("Callback: '%s' is not called on the correct thread", __FUNCTION__);
       return;
    }

    ALOGV("%s: DiscoveryState:%d ", __FUNCTION__, state);

    callbackEnv->CallVoidMethod(sJniCallbacksObj, method_discoveryStateChangeCallback,
                                (jint)state);

    checkAndClearExceptionFromCallback(callbackEnv, __FUNCTION__);
}

static void pin_request_callback(bt_bdaddr_t *bd_addr, bt_bdname_t *bdname, uint32_t cod,
        bool min_16_digits) {
    jbyteArray addr = NULL;
    jbyteArray devname = NULL;
    if (!checkCallbackThread()) {
       ALOGE("Callback: '%s' is not called on the correct thread", __FUNCTION__);
       return;
    }
    if (!bd_addr) {
        ALOGE("Address is null in %s", __FUNCTION__);
        return;
    }

    addr = callbackEnv->NewByteArray(sizeof(bt_bdaddr_t));
    if (addr == NULL) goto Fail;
    callbackEnv->SetByteArrayRegion(addr, 0, sizeof(bt_bdaddr_t), (jbyte*)bd_addr);

    devname = callbackEnv->NewByteArray(sizeof(bt_bdname_t));
    if (devname == NULL) goto Fail;

    callbackEnv->SetByteArrayRegion(devname, 0, sizeof(bt_bdname_t), (jbyte*)bdname);

    callbackEnv->CallVoidMethod(sJniCallbacksObj, method_pinRequestCallback, addr, devname, cod,
            min_16_digits);

    checkAndClearExceptionFromCallback(callbackEnv, __FUNCTION__);
    callbackEnv->DeleteLocalRef(addr);
    callbackEnv->DeleteLocalRef(devname);
    return;

Fail:
    if (addr) callbackEnv->DeleteLocalRef(addr);
    if (devname) callbackEnv->DeleteLocalRef(devname);
    ALOGE("Error while allocating in: %s", __FUNCTION__);
}

static void ssp_request_callback(bt_bdaddr_t *bd_addr, bt_bdname_t *bdname, uint32_t cod,
                                 bt_ssp_variant_t pairing_variant, uint32_t pass_key) {
    jbyteArray addr = NULL;
    jbyteArray devname = NULL;
    if (!checkCallbackThread()) {
       ALOGE("Callback: '%s' is not called on the correct thread", __FUNCTION__);
       return;
    }
    if (!bd_addr) {
        ALOGE("Address is null in %s", __FUNCTION__);
        return;
    }

    addr = callbackEnv->NewByteArray(sizeof(bt_bdaddr_t));
    if (addr == NULL) goto Fail;
    callbackEnv->SetByteArrayRegion(addr, 0, sizeof(bt_bdaddr_t), (jbyte *)bd_addr);

    devname = callbackEnv->NewByteArray(sizeof(bt_bdname_t));
    if (devname == NULL) goto Fail;
    callbackEnv->SetByteArrayRegion(devname, 0, sizeof(bt_bdname_t), (jbyte*)bdname);

    callbackEnv->CallVoidMethod(sJniCallbacksObj, method_sspRequestCallback, addr, devname, cod,
                                (jint) pairing_variant, pass_key);

    checkAndClearExceptionFromCallback(callbackEnv, __FUNCTION__);
    callbackEnv->DeleteLocalRef(addr);
    callbackEnv->DeleteLocalRef(devname);
    return;

Fail:
    if (addr) callbackEnv->DeleteLocalRef(addr);
    if (devname) callbackEnv->DeleteLocalRef(devname);

    ALOGE("Error while allocating in: %s", __FUNCTION__);
}

static void callback_thread_event(bt_cb_thread_evt event) {
    JavaVM* vm = AndroidRuntime::getJavaVM();
    if (event  == ASSOCIATE_JVM) {
        JavaVMAttachArgs args;
        char name[] = "BT Service Callback Thread";
        args.version = JNI_VERSION_1_6;
        args.name = name;
        args.group = NULL;
        vm->AttachCurrentThread(&callbackEnv, &args);
        ALOGV("Callback thread attached: %p", callbackEnv);
    } else if (event == DISASSOCIATE_JVM) {
        if (!checkCallbackThread()) {
            ALOGE("Callback: '%s' is not called on the correct thread", __FUNCTION__);
            return;
        }
        vm->DetachCurrentThread();
    }
}

static void dut_mode_recv_callback (uint16_t opcode, uint8_t *buf, uint8_t len) {

}
static void le_test_mode_recv_callback (bt_status_t status, uint16_t packet_count) {

    ALOGV("%s: status:%d packet_count:%d ", __FUNCTION__, status, packet_count);
}

static void energy_info_recv_callback(bt_activity_energy_info *p_energy_info,
                                      bt_uid_traffic_t* uid_data)
{
    if (!checkCallbackThread()) {
       ALOGE("Callback: '%s' is not called on the correct thread", __FUNCTION__);
       return;
    }

    jsize len = 0;
    for (bt_uid_traffic_t* data = uid_data; data->app_uid != -1; data++) {
        len++;
    }

    jobjectArray array = callbackEnv->NewObjectArray(len, android_bluetooth_UidTraffic.clazz, NULL);
    jsize i = 0;
    for (bt_uid_traffic_t* data = uid_data; data->app_uid != -1; data++) {
        jobject uidObj = callbackEnv->NewObject(android_bluetooth_UidTraffic.clazz,
                                                android_bluetooth_UidTraffic.constructor,
                                                (jint) data->app_uid, (jlong) data->rx_bytes,
                                                (jlong) data->tx_bytes);
        callbackEnv->SetObjectArrayElement(array, i++, uidObj);
        callbackEnv->DeleteLocalRef(uidObj);
    }

    callbackEnv->CallVoidMethod(sJniAdapterServiceObj, method_energyInfo, p_energy_info->status,
        p_energy_info->ctrl_state, p_energy_info->tx_time, p_energy_info->rx_time,
        p_energy_info->idle_time, p_energy_info->energy_used, array);

    checkAndClearExceptionFromCallback(callbackEnv, __FUNCTION__);
    callbackEnv->DeleteLocalRef(array);
}

static bt_callbacks_t sBluetoothCallbacks = {
    sizeof(sBluetoothCallbacks),
    adapter_state_change_callback,
    adapter_properties_callback,
    remote_device_properties_callback,
    device_found_callback,
    discovery_state_changed_callback,
    pin_request_callback,
    ssp_request_callback,
    bond_state_changed_callback,
    acl_state_changed_callback,
    callback_thread_event,
    dut_mode_recv_callback,
    le_test_mode_recv_callback,
    energy_info_recv_callback
};

// The callback to call when the wake alarm fires.
static alarm_cb sAlarmCallback;

// The data to pass to the wake alarm callback.
static void *sAlarmCallbackData;

static JavaVMAttachArgs sAttachArgs = {
  .version = JNI_VERSION_1_6,
  .name = "bluetooth wake",
  .group = NULL
};

static bool set_wake_alarm_callout(uint64_t delay_millis, bool should_wake,
        alarm_cb cb, void *data) {
    JNIEnv *env;
    JavaVM *vm = AndroidRuntime::getJavaVM();
    jint status = vm->GetEnv((void **)&env, JNI_VERSION_1_6);

    if (status != JNI_OK && status != JNI_EDETACHED) {
        ALOGE("%s unable to get environment for JNI call", __func__);
        return false;
    }

    if (status == JNI_EDETACHED && vm->AttachCurrentThread(&env, &sAttachArgs) != 0) {
        ALOGE("%s unable to attach thread to VM", __func__);
        return false;
    }

    sAlarmCallback = cb;
    sAlarmCallbackData = data;

    jboolean jshould_wake = should_wake ? JNI_TRUE : JNI_FALSE;
    jboolean ret = env->CallBooleanMethod(sJniAdapterServiceObj, method_setWakeAlarm,
            (jlong)delay_millis, jshould_wake);
    if (!ret) {
        sAlarmCallback = NULL;
        sAlarmCallbackData = NULL;
    }

    if (status == JNI_EDETACHED) {
        vm->DetachCurrentThread();
    }

    return !!ret;
}

static int acquire_wake_lock_callout(const char *lock_name) {
    JNIEnv *env;
    JavaVM *vm = AndroidRuntime::getJavaVM();
    jint status = vm->GetEnv((void **)&env, JNI_VERSION_1_6);
    if (status != JNI_OK && status != JNI_EDETACHED) {
        ALOGE("%s unable to get environment for JNI call", __func__);
        return BT_STATUS_JNI_ENVIRONMENT_ERROR;
    }
    if (status == JNI_EDETACHED && vm->AttachCurrentThread(&env, &sAttachArgs) != 0) {
        ALOGE("%s unable to attach thread to VM", __func__);
        return BT_STATUS_JNI_THREAD_ATTACH_ERROR;
    }

    jint ret = BT_STATUS_SUCCESS;
    jstring lock_name_jni = env->NewStringUTF(lock_name);
    if (lock_name_jni) {
        bool acquired = env->CallBooleanMethod(sJniAdapterServiceObj,
                            method_acquireWakeLock, lock_name_jni);
        if (!acquired) ret = BT_STATUS_WAKELOCK_ERROR;
        env->DeleteLocalRef(lock_name_jni);
    } else {
        ALOGE("%s unable to allocate string: %s", __func__, lock_name);
        ret = BT_STATUS_NOMEM;
    }

    if (status == JNI_EDETACHED) {
        vm->DetachCurrentThread();
    }

    return ret;
}

static int release_wake_lock_callout(const char *lock_name) {
    JNIEnv *env;
    JavaVM *vm = AndroidRuntime::getJavaVM();
    jint status = vm->GetEnv((void **)&env, JNI_VERSION_1_6);
    if (status != JNI_OK && status != JNI_EDETACHED) {
        ALOGE("%s unable to get environment for JNI call", __func__);
        return BT_STATUS_JNI_ENVIRONMENT_ERROR;
    }
    if (status == JNI_EDETACHED && vm->AttachCurrentThread(&env, &sAttachArgs) != 0) {
        ALOGE("%s unable to attach thread to VM", __func__);
        return BT_STATUS_JNI_THREAD_ATTACH_ERROR;
    }

    jint ret = BT_STATUS_SUCCESS;
    jstring lock_name_jni = env->NewStringUTF(lock_name);
    if (lock_name_jni) {
        bool released = env->CallBooleanMethod(sJniAdapterServiceObj,
                            method_releaseWakeLock, lock_name_jni);
        if (!released) ret = BT_STATUS_WAKELOCK_ERROR;
        env->DeleteLocalRef(lock_name_jni);
    } else {
        ALOGE("%s unable to allocate string: %s", __func__, lock_name);
        ret = BT_STATUS_NOMEM;
    }

    if (status == JNI_EDETACHED) {
        vm->DetachCurrentThread();
    }

    return ret;
}

// Called by Java code when alarm is fired. A wake lock is held by the caller
// over the duration of this callback.
static void alarmFiredNative(JNIEnv *env, jobject obj) {
    if (sAlarmCallback) {
        sAlarmCallback(sAlarmCallbackData);
    } else {
        ALOGE("%s() - Alarm fired with callback not set!", __FUNCTION__);
    }
}

static bt_os_callouts_t sBluetoothOsCallouts = {
    sizeof(sBluetoothOsCallouts),
    set_wake_alarm_callout,
    acquire_wake_lock_callout,
    release_wake_lock_callout,
};



static void classInitNative(JNIEnv* env, jclass clazz) {
    int err;
    hw_module_t* module;


    jclass jniUidTrafficClass = env->FindClass("android/bluetooth/UidTraffic");
    android_bluetooth_UidTraffic.constructor = env->GetMethodID(jniUidTrafficClass,
                                                                "<init>", "(IJJ)V");

    jclass jniCallbackClass =
        env->FindClass("com/android/bluetooth/btservice/JniCallbacks");
    sJniCallbacksField = env->GetFieldID(clazz, "mJniCallbacks",
        "Lcom/android/bluetooth/btservice/JniCallbacks;");

    method_stateChangeCallback = env->GetMethodID(jniCallbackClass, "stateChangeCallback", "(I)V");

    method_adapterPropertyChangedCallback = env->GetMethodID(jniCallbackClass,
                                                             "adapterPropertyChangedCallback",
                                                             "([I[[B)V");
    method_discoveryStateChangeCallback = env->GetMethodID(jniCallbackClass,
                                                           "discoveryStateChangeCallback", "(I)V");

    method_devicePropertyChangedCallback = env->GetMethodID(jniCallbackClass,
                                                            "devicePropertyChangedCallback",
                                                            "([B[I[[B)V");
    method_deviceFoundCallback = env->GetMethodID(jniCallbackClass, "deviceFoundCallback", "([B)V");
    method_pinRequestCallback = env->GetMethodID(jniCallbackClass, "pinRequestCallback",
                                                 "([B[BIZ)V");
    method_sspRequestCallback = env->GetMethodID(jniCallbackClass, "sspRequestCallback",
                                                 "([B[BIII)V");

    method_bondStateChangeCallback = env->GetMethodID(jniCallbackClass,
                                                     "bondStateChangeCallback", "(I[BI)V");

    method_aclStateChangeCallback = env->GetMethodID(jniCallbackClass,
                                                    "aclStateChangeCallback", "(I[BI)V");

    method_setWakeAlarm = env->GetMethodID(clazz, "setWakeAlarm", "(JZ)Z");
    method_acquireWakeLock = env->GetMethodID(clazz, "acquireWakeLock", "(Ljava/lang/String;)Z");
    method_releaseWakeLock = env->GetMethodID(clazz, "releaseWakeLock", "(Ljava/lang/String;)Z");
    method_energyInfo = env->GetMethodID(clazz, "energyInfoCallback", "(IIJJJJ[Landroid/bluetooth/UidTraffic;)V");

    char value[PROPERTY_VALUE_MAX];
    property_get("bluetooth.mock_stack", value, "");

    const char *id = (strcmp(value, "1")? BT_STACK_MODULE_ID : BT_STACK_TEST_MODULE_ID);

    err = hw_get_module(id, (hw_module_t const**)&module);

    if (err == 0) {
        hw_device_t* abstraction;
        err = module->methods->open(module, id, &abstraction);
        if (err == 0) {
            bluetooth_module_t* btStack = (bluetooth_module_t *)abstraction;
            sBluetoothInterface = btStack->get_bluetooth_interface();
        } else {
           ALOGE("Error while opening Bluetooth library");
        }
    } else {
        ALOGE("No Bluetooth Library found");
    }
}

static bool initNative(JNIEnv* env, jobject obj) {
    ALOGV("%s:",__FUNCTION__);

    android_bluetooth_UidTraffic.clazz = (jclass) env->NewGlobalRef(
            env->FindClass("android/bluetooth/UidTraffic"));

    sJniAdapterServiceObj = env->NewGlobalRef(obj);
    sJniCallbacksObj = env->NewGlobalRef(env->GetObjectField(obj, sJniCallbacksField));

    if (sBluetoothInterface) {
        int ret = sBluetoothInterface->init(&sBluetoothCallbacks);
        if (ret != BT_STATUS_SUCCESS) {
            ALOGE("Error while setting the callbacks: %d\n", ret);
            sBluetoothInterface = NULL;
            return JNI_FALSE;
        }
        ret = sBluetoothInterface->set_os_callouts(&sBluetoothOsCallouts);
        if (ret != BT_STATUS_SUCCESS) {
            ALOGE("Error while setting Bluetooth callouts: %d\n", ret);
            sBluetoothInterface->cleanup();
            sBluetoothInterface = NULL;
            return JNI_FALSE;
        }

        if ( (sBluetoothSocketInterface = (btsock_interface_t *)
                  sBluetoothInterface->get_profile_interface(BT_PROFILE_SOCKETS_ID)) == NULL) {
                ALOGE("Error getting socket interface");
        }

        return JNI_TRUE;
    }
    return JNI_FALSE;
}

static bool cleanupNative(JNIEnv *env, jobject obj) {
    ALOGV("%s:",__FUNCTION__);

    jboolean result = JNI_FALSE;
    if (!sBluetoothInterface) return result;

    sBluetoothInterface->cleanup();
    ALOGI("%s: return from cleanup",__FUNCTION__);

    env->DeleteGlobalRef(sJniCallbacksObj);
    env->DeleteGlobalRef(sJniAdapterServiceObj);
    env->DeleteGlobalRef(android_bluetooth_UidTraffic.clazz);
    android_bluetooth_UidTraffic.clazz = NULL;
    return JNI_TRUE;
}

static jboolean enableNative(JNIEnv* env, jobject obj, jboolean isGuest) {
    ALOGV("%s:",__FUNCTION__);

    jboolean result = JNI_FALSE;
    if (!sBluetoothInterface) return result;
    int ret = sBluetoothInterface->enable(isGuest == JNI_TRUE ? 1 : 0);
    result = (ret == BT_STATUS_SUCCESS || ret == BT_STATUS_DONE) ? JNI_TRUE : JNI_FALSE;
    return result;
}

static jboolean disableNative(JNIEnv* env, jobject obj) {
    ALOGV("%s:",__FUNCTION__);

    jboolean result = JNI_FALSE;
    if (!sBluetoothInterface) return result;

    int ret = sBluetoothInterface->disable();
    /* Retrun JNI_FALSE only when BTIF explicitly reports
       BT_STATUS_FAIL. It is fine for the BT_STATUS_NOT_READY
       case which indicates that stack had not been enabled.
    */
    result = (ret == BT_STATUS_FAIL) ? JNI_FALSE : JNI_TRUE;
    return result;
}

static jboolean startDiscoveryNative(JNIEnv* env, jobject obj) {
    ALOGV("%s:",__FUNCTION__);

    jboolean result = JNI_FALSE;
    if (!sBluetoothInterface) return result;

    int ret = sBluetoothInterface->start_discovery();
    result = (ret == BT_STATUS_SUCCESS) ? JNI_TRUE : JNI_FALSE;
    return result;
}

static jboolean cancelDiscoveryNative(JNIEnv* env, jobject obj) {
    ALOGV("%s:",__FUNCTION__);

    jboolean result = JNI_FALSE;
    if (!sBluetoothInterface) return result;

    int ret = sBluetoothInterface->cancel_discovery();
    result = (ret == BT_STATUS_SUCCESS) ? JNI_TRUE : JNI_FALSE;
    return result;
}

static jboolean createBondNative(JNIEnv* env, jobject obj, jbyteArray address, jint transport) {
    ALOGV("%s:",__FUNCTION__);

    jbyte *addr;
    jboolean result = JNI_FALSE;

    if (!sBluetoothInterface) return result;

    addr = env->GetByteArrayElements(address, NULL);
    if (addr == NULL) {
        jniThrowIOException(env, EINVAL);
        return result;
    }

    int ret = sBluetoothInterface->create_bond((bt_bdaddr_t *)addr, transport);
    env->ReleaseByteArrayElements(address, addr, 0);
    result = (ret == BT_STATUS_SUCCESS) ? JNI_TRUE : JNI_FALSE;

    return result;
}

static jbyteArray callByteArrayGetter(JNIEnv* env, jobject object,
                                      const char* className,
                                      const char* methodName) {
    jclass myClass = env->FindClass(className);
    jmethodID myMethod = env->GetMethodID(myClass, methodName, "()[B");
    return (jbyteArray) env->CallObjectMethod(object, myMethod);
}

static jboolean createBondOutOfBandNative(JNIEnv* env, jobject obj, jbyteArray address,
                jint transport, jobject oobData) {
    jbyte *addr;
    jboolean result = JNI_FALSE;
    bt_out_of_band_data_t oob_data;

    memset(&oob_data, 0, sizeof(oob_data));

    if (!sBluetoothInterface) return result;

    addr = env->GetByteArrayElements(address, NULL);
    if (addr == NULL) {
        jniThrowIOException(env, EINVAL);
        return result;
    }

    jbyte* smTKBytes = NULL;
    jbyteArray smTK = callByteArrayGetter(env, oobData, "android/bluetooth/OobData", "getSecurityManagerTk");
    if (smTK != NULL) {
        smTKBytes = env->GetByteArrayElements(smTK, NULL);
        int len = env->GetArrayLength(smTK);
        if (len != OOB_TK_SIZE) {
            ALOGI("%s: wrong length of smTK, should be empty or %d bytes.", __FUNCTION__, OOB_TK_SIZE);
            jniThrowIOException(env, EINVAL);
            goto done;
        }
        memcpy(oob_data.sm_tk, smTKBytes, len);
    }

    if (sBluetoothInterface->create_bond_out_of_band((bt_bdaddr_t *)addr, transport, &oob_data)
        == BT_STATUS_SUCCESS)
        result = JNI_TRUE;

done:
    env->ReleaseByteArrayElements(address, addr, 0);

    if (smTK != NULL)
        env->ReleaseByteArrayElements(smTK, smTKBytes, 0);

    return result;
}

static jboolean removeBondNative(JNIEnv* env, jobject obj, jbyteArray address) {
    ALOGV("%s:",__FUNCTION__);

    jbyte *addr;
    jboolean result;
    if (!sBluetoothInterface) return JNI_FALSE;

    addr = env->GetByteArrayElements(address, NULL);
    if (addr == NULL) {
        jniThrowIOException(env, EINVAL);
        return JNI_FALSE;
    }

    int ret = sBluetoothInterface->remove_bond((bt_bdaddr_t *)addr);
    env->ReleaseByteArrayElements(address, addr, 0);
    result = (ret == BT_STATUS_SUCCESS) ? JNI_TRUE : JNI_FALSE;

    return result;
}

static jboolean cancelBondNative(JNIEnv* env, jobject obj, jbyteArray address) {
    ALOGV("%s:",__FUNCTION__);

    jbyte *addr;
    jboolean result;
    if (!sBluetoothInterface) return JNI_FALSE;

    addr = env->GetByteArrayElements(address, NULL);
    if (addr == NULL) {
        jniThrowIOException(env, EINVAL);
        return JNI_FALSE;
    }

    int ret = sBluetoothInterface->cancel_bond((bt_bdaddr_t *)addr);
    env->ReleaseByteArrayElements(address, addr, 0);
    result = (ret == BT_STATUS_SUCCESS) ? JNI_TRUE : JNI_FALSE;

    return result;
}

static int getConnectionStateNative(JNIEnv* env, jobject obj, jbyteArray address) {
    ALOGV("%s:",__FUNCTION__);
    if (!sBluetoothInterface) return JNI_FALSE;

    jbyte *addr = env->GetByteArrayElements(address, NULL);
    if (addr == NULL) {
        jniThrowIOException(env, EINVAL);
        return JNI_FALSE;
    }

    int ret = sBluetoothInterface->get_connection_state((bt_bdaddr_t *)addr);
    env->ReleaseByteArrayElements(address, addr, 0);

    return ret;
}

static jboolean pinReplyNative(JNIEnv *env, jobject obj, jbyteArray address, jboolean accept,
                               jint len, jbyteArray pinArray) {
    ALOGV("%s:",__FUNCTION__);

    jbyte *addr, *pinPtr = NULL;
    jboolean result = JNI_FALSE;
    if (!sBluetoothInterface) return result;

    addr = env->GetByteArrayElements(address, NULL);
    if (addr == NULL) {
        jniThrowIOException(env, EINVAL);
        return result;
    }

    if (accept) {
        pinPtr = env->GetByteArrayElements(pinArray, NULL);
        if (pinPtr == NULL) {
           jniThrowIOException(env, EINVAL);
           env->ReleaseByteArrayElements(address, addr, 0);
           return result;
        }
    }

    int ret = sBluetoothInterface->pin_reply((bt_bdaddr_t*)addr, accept, len,
                                              (bt_pin_code_t *) pinPtr);
    env->ReleaseByteArrayElements(address, addr, 0);
    env->ReleaseByteArrayElements(pinArray, pinPtr, 0);
    result = (ret == BT_STATUS_SUCCESS) ? JNI_TRUE : JNI_FALSE;

    return result;
}

static jboolean sspReplyNative(JNIEnv *env, jobject obj, jbyteArray address,
                               jint type, jboolean accept, jint passkey) {
    ALOGV("%s:",__FUNCTION__);

    jbyte *addr;
    jboolean result = JNI_FALSE;
    if (!sBluetoothInterface) return result;

    addr = env->GetByteArrayElements(address, NULL);
    if (addr == NULL) {
        jniThrowIOException(env, EINVAL);
        return result;
    }

    int ret = sBluetoothInterface->ssp_reply((bt_bdaddr_t *)addr,
         (bt_ssp_variant_t) type, accept, passkey);
    env->ReleaseByteArrayElements(address, addr, 0);
    result = (ret == BT_STATUS_SUCCESS) ? JNI_TRUE : JNI_FALSE;

    return result;
}

static jboolean setAdapterPropertyNative(JNIEnv *env, jobject obj, jint type, jbyteArray value) {
    ALOGV("%s:",__FUNCTION__);

    jbyte *val;
    jboolean result = JNI_FALSE;
    if (!sBluetoothInterface) return result;

    val = env->GetByteArrayElements(value, NULL);
    bt_property_t prop;
    prop.type = (bt_property_type_t) type;
    prop.len = env->GetArrayLength(value);
    prop.val = val;

    int ret = sBluetoothInterface->set_adapter_property(&prop);
    env->ReleaseByteArrayElements(value, val, 0);
    result = (ret == BT_STATUS_SUCCESS) ? JNI_TRUE : JNI_FALSE;

    return result;
}

static jboolean getAdapterPropertiesNative(JNIEnv *env, jobject obj) {
    ALOGV("%s:",__FUNCTION__);

    jboolean result = JNI_FALSE;
    if (!sBluetoothInterface) return result;

    int ret = sBluetoothInterface->get_adapter_properties();
    result = (ret == BT_STATUS_SUCCESS) ? JNI_TRUE : JNI_FALSE;

    return result;
}

static jboolean getAdapterPropertyNative(JNIEnv *env, jobject obj, jint type) {
    ALOGV("%s:",__FUNCTION__);

    jboolean result = JNI_FALSE;
    if (!sBluetoothInterface) return result;

    int ret = sBluetoothInterface->get_adapter_property((bt_property_type_t) type);
    result = (ret == BT_STATUS_SUCCESS) ? JNI_TRUE : JNI_FALSE;

    return result;
}

static jboolean getDevicePropertyNative(JNIEnv *env, jobject obj, jbyteArray address, jint type) {
    ALOGV("%s:",__FUNCTION__);

    jbyte *addr = NULL;
    jboolean result = JNI_FALSE;
    if (!sBluetoothInterface) return result;

    addr = env->GetByteArrayElements(address, NULL);
    if (addr == NULL) {
        jniThrowIOException(env, EINVAL);
        return result;
    }

    int ret = sBluetoothInterface->get_remote_device_property((bt_bdaddr_t *)addr,
                                                              (bt_property_type_t) type);
    env->ReleaseByteArrayElements(address, addr, 0);
    result = (ret == BT_STATUS_SUCCESS) ? JNI_TRUE : JNI_FALSE;

    return result;
}

static jboolean setDevicePropertyNative(JNIEnv *env, jobject obj, jbyteArray address,
                                        jint type, jbyteArray value) {
    ALOGV("%s:",__FUNCTION__);

    jbyte *val, *addr;
    jboolean result = JNI_FALSE;
    if (!sBluetoothInterface) return result;

    val = env->GetByteArrayElements(value, NULL);
    if (val == NULL) {
        jniThrowIOException(env, EINVAL);
        return result;
    }

    addr = env->GetByteArrayElements(address, NULL);
    if (addr == NULL) {
        env->ReleaseByteArrayElements(value, val, 0);
        jniThrowIOException(env, EINVAL);
        return result;
    }


    bt_property_t prop;
    prop.type = (bt_property_type_t) type;
    prop.len = env->GetArrayLength(value);
    prop.val = val;

    int ret = sBluetoothInterface->set_remote_device_property((bt_bdaddr_t *)addr, &prop);
    env->ReleaseByteArrayElements(value, val, 0);
    env->ReleaseByteArrayElements(address, addr, 0);

    result = (ret == BT_STATUS_SUCCESS) ? JNI_TRUE : JNI_FALSE;

    return result;
}

static jboolean getRemoteServicesNative(JNIEnv *env, jobject obj, jbyteArray address) {
    ALOGV("%s:",__FUNCTION__);

    jbyte *addr = NULL;
    jboolean result = JNI_FALSE;
    if (!sBluetoothInterface) return result;

    addr = env->GetByteArrayElements(address, NULL);
    if (addr == NULL) {
        jniThrowIOException(env, EINVAL);
        return result;
    }

    int ret = sBluetoothInterface->get_remote_services((bt_bdaddr_t *)addr);
    env->ReleaseByteArrayElements(address, addr, 0);
    result = (ret == BT_STATUS_SUCCESS) ? JNI_TRUE : JNI_FALSE;
    return result;
}

static int connectSocketNative(JNIEnv *env, jobject object, jbyteArray address, jint type,
                                   jbyteArray uuidObj, jint channel, jint flag, jint callingUid) {
    jbyte *addr = NULL, *uuid = NULL;
    int socket_fd;
    bt_status_t status;

    if (!sBluetoothSocketInterface) return -1;

    addr = env->GetByteArrayElements(address, NULL);
    if (!addr) {
        ALOGE("failed to get Bluetooth device address");
        goto Fail;
    }

    if(uuidObj != NULL) {
        uuid = env->GetByteArrayElements(uuidObj, NULL);
        if (!uuid) {
            ALOGE("failed to get uuid");
            goto Fail;
        }
    }

    if ( (status = sBluetoothSocketInterface->connect((bt_bdaddr_t *) addr, (btsock_type_t) type,
                       (const uint8_t*) uuid, channel, &socket_fd, flag, callingUid))
            != BT_STATUS_SUCCESS) {
        ALOGE("Socket connection failed: %d", status);
        goto Fail;
    }


    if (socket_fd < 0) {
        ALOGE("Fail to create file descriptor on socket fd");
        goto Fail;
    }
    env->ReleaseByteArrayElements(address, addr, 0);
    env->ReleaseByteArrayElements(uuidObj, uuid, 0);
    return socket_fd;

Fail:
    if (addr) env->ReleaseByteArrayElements(address, addr, 0);
    if (uuid) env->ReleaseByteArrayElements(uuidObj, uuid, 0);

    return -1;
}

static int createSocketChannelNative(JNIEnv *env, jobject object, jint type,
                                     jstring name_str, jbyteArray uuidObj,
                                     jint channel, jint flag, jint callingUid) {
    const char *service_name = NULL;
    jbyte *uuid = NULL;
    int socket_fd;
    bt_status_t status;

    if (!sBluetoothSocketInterface) return -1;

    ALOGV("%s: SOCK FLAG = %x", __FUNCTION__, flag);

    if(name_str != NULL) {
        service_name = env->GetStringUTFChars(name_str, NULL);
    }

    if(uuidObj != NULL) {
        uuid = env->GetByteArrayElements(uuidObj, NULL);
        if (!uuid) {
            ALOGE("failed to get uuid");
            goto Fail;
        }
    }
    if ( (status = sBluetoothSocketInterface->listen((btsock_type_t) type, service_name,
                       (const uint8_t*) uuid, channel, &socket_fd, flag, callingUid))
            != BT_STATUS_SUCCESS) {
        ALOGE("Socket listen failed: %d", status);
        goto Fail;
    }

    if (socket_fd < 0) {
        ALOGE("Fail to creat file descriptor on socket fd");
        goto Fail;
    }
    if (service_name) env->ReleaseStringUTFChars(name_str, service_name);
    if (uuid) env->ReleaseByteArrayElements(uuidObj, uuid, 0);
    return socket_fd;

Fail:
    if (service_name) env->ReleaseStringUTFChars(name_str, service_name);
    if (uuid) env->ReleaseByteArrayElements(uuidObj, uuid, 0);
    return -1;
}

static jboolean configHciSnoopLogNative(JNIEnv* env, jobject obj, jboolean enable) {
    ALOGV("%s:",__FUNCTION__);

    jboolean result = JNI_FALSE;

    if (!sBluetoothInterface) return result;

    int ret = sBluetoothInterface->config_hci_snoop_log(enable);

    result = (ret == BT_STATUS_SUCCESS) ? JNI_TRUE : JNI_FALSE;

    return result;
}

static int readEnergyInfo()
{
    ALOGV("%s:",__FUNCTION__);
    jboolean result = JNI_FALSE;
    if (!sBluetoothInterface) return result;
    int ret = sBluetoothInterface->read_energy_info();
    result = (ret == BT_STATUS_SUCCESS) ? JNI_TRUE : JNI_FALSE;
    return result;
}

static void dumpNative(JNIEnv *env, jobject obj, jobject fdObj,
                       jobjectArray argArray)
{
    ALOGV("%s()", __FUNCTION__);
    if (!sBluetoothInterface) return;

    int fd = jniGetFDFromFileDescriptor(env, fdObj);
    if (fd < 0) return;

    int numArgs = env->GetArrayLength(argArray);

    jstring *argObjs = new jstring[numArgs];
    const char **args = nullptr;
    if (numArgs > 0)
      args = new const char*[numArgs];

    for (int i = 0; i < numArgs; i++) {
      argObjs[i] = (jstring) env->GetObjectArrayElement(argArray, i);
      args[i] = env->GetStringUTFChars(argObjs[i], NULL);
    }

    sBluetoothInterface->dump(fd, args);

    for (int i = 0; i < numArgs; i++) {
      env->ReleaseStringUTFChars(argObjs[i], args[i]);
    }

    delete[] args;
    delete[] argObjs;
}

static jboolean factoryResetNative(JNIEnv *env, jobject obj) {
    ALOGV("%s:", __FUNCTION__);
    if (!sBluetoothInterface) return JNI_FALSE;
    int ret = sBluetoothInterface->config_clear();
    return (ret == BT_STATUS_SUCCESS) ? JNI_TRUE : JNI_FALSE;
}

static void interopDatabaseClearNative(JNIEnv *env, jobject obj) {
    ALOGV("%s()", __FUNCTION__);
    if (!sBluetoothInterface) return;
    sBluetoothInterface->interop_database_clear();
}

static void interopDatabaseAddNative(JNIEnv *env, jobject obj, int feature,
                                      jbyteArray address, int length) {
    ALOGV("%s()", __FUNCTION__);
    if (!sBluetoothInterface) return;

    jbyte *addr = env->GetByteArrayElements(address, NULL);
    if (addr == NULL) {
        jniThrowIOException(env, EINVAL);
        return;
    }

    sBluetoothInterface->interop_database_add(feature, (bt_bdaddr_t *)addr, length);
    env->ReleaseByteArrayElements(address, addr, 0);
}

static JNINativeMethod sMethods[] = {
    /* name, signature, funcPtr */
    {"classInitNative", "()V", (void *) classInitNative},
    {"initNative", "()Z", (void *) initNative},
    {"cleanupNative", "()V", (void*) cleanupNative},
    {"enableNative", "(Z)Z",  (void*) enableNative},
    {"disableNative", "()Z",  (void*) disableNative},
    {"setAdapterPropertyNative", "(I[B)Z", (void*) setAdapterPropertyNative},
    {"getAdapterPropertiesNative", "()Z", (void*) getAdapterPropertiesNative},
    {"getAdapterPropertyNative", "(I)Z", (void*) getAdapterPropertyNative},
    {"getDevicePropertyNative", "([BI)Z", (void*) getDevicePropertyNative},
    {"setDevicePropertyNative", "([BI[B)Z", (void*) setDevicePropertyNative},
    {"startDiscoveryNative", "()Z", (void*) startDiscoveryNative},
    {"cancelDiscoveryNative", "()Z", (void*) cancelDiscoveryNative},
    {"createBondNative", "([BI)Z", (void*) createBondNative},
    {"createBondOutOfBandNative", "([BILandroid/bluetooth/OobData;)Z", (void*) createBondOutOfBandNative},
    {"removeBondNative", "([B)Z", (void*) removeBondNative},
    {"cancelBondNative", "([B)Z", (void*) cancelBondNative},
    {"getConnectionStateNative", "([B)I", (void*) getConnectionStateNative},
    {"pinReplyNative", "([BZI[B)Z", (void*) pinReplyNative},
    {"sspReplyNative", "([BIZI)Z", (void*) sspReplyNative},
    {"getRemoteServicesNative", "([B)Z", (void*) getRemoteServicesNative},
    {"connectSocketNative", "([BI[BIII)I", (void*) connectSocketNative},
    {"createSocketChannelNative", "(ILjava/lang/String;[BIII)I",
     (void*) createSocketChannelNative},
    {"configHciSnoopLogNative", "(Z)Z", (void*) configHciSnoopLogNative},
    {"alarmFiredNative", "()V", (void *) alarmFiredNative},
    {"readEnergyInfo", "()I", (void*) readEnergyInfo},
    {"dumpNative", "(Ljava/io/FileDescriptor;[Ljava/lang/String;)V", (void*) dumpNative},
    {"factoryResetNative", "()Z", (void*)factoryResetNative},
    {"interopDatabaseClearNative", "()V", (void*) interopDatabaseClearNative},
    {"interopDatabaseAddNative", "(I[BI)V", (void*) interopDatabaseAddNative}
};

int register_com_android_bluetooth_btservice_AdapterService(JNIEnv* env)
{
    return jniRegisterNativeMethods(env, "com/android/bluetooth/btservice/AdapterService",
                                    sMethods, NELEM(sMethods));
}

} /* namespace android */


/*
 * JNI Initialization
 */
jint JNI_OnLoad(JavaVM *jvm, void *reserved)
{
    JNIEnv *e;
    int status;

    ALOGV("Bluetooth Adapter Service : loading JNI\n");

    // Check JNI version
    if (jvm->GetEnv((void **)&e, JNI_VERSION_1_6)) {
        ALOGE("JNI version mismatch error");
        return JNI_ERR;
    }

    if ((status = android::register_com_android_bluetooth_btservice_AdapterService(e)) < 0) {
        ALOGE("jni adapter service registration failure, status: %d", status);
        return JNI_ERR;
    }

    if ((status = android::register_com_android_bluetooth_hfp(e)) < 0) {
        ALOGE("jni hfp registration failure, status: %d", status);
        return JNI_ERR;
    }

    if ((status = android::register_com_android_bluetooth_hfpclient(e)) < 0) {
        ALOGE("jni hfp client registration failure, status: %d", status);
        return JNI_ERR;
    }

    if ((status = android::register_com_android_bluetooth_a2dp(e)) < 0) {
        ALOGE("jni a2dp source registration failure: %d", status);
        return JNI_ERR;
    }

    if ((status = android::register_com_android_bluetooth_a2dp_sink(e)) < 0) {
        ALOGE("jni a2dp sink registration failure: %d", status);
        return JNI_ERR;
    }

    if ((status = android::register_com_android_bluetooth_avrcp(e)) < 0) {
        ALOGE("jni avrcp target registration failure: %d", status);
        return JNI_ERR;
    }

    if ((status = android::register_com_android_bluetooth_avrcp_controller(e)) < 0) {
        ALOGE("jni avrcp controller registration failure: %d", status);
        return JNI_ERR;
    }

    if ((status = android::register_com_android_bluetooth_hid(e)) < 0) {
        ALOGE("jni hid registration failure: %d", status);
        return JNI_ERR;
    }

    if ((status = android::register_com_android_bluetooth_hdp(e)) < 0) {
        ALOGE("jni hdp registration failure: %d", status);
        return JNI_ERR;
    }

    if ((status = android::register_com_android_bluetooth_pan(e)) < 0) {
        ALOGE("jni pan registration failure: %d", status);
        return JNI_ERR;
    }

    if ((status = android::register_com_android_bluetooth_gatt(e)) < 0) {
        ALOGE("jni gatt registration failure: %d", status);
        return JNI_ERR;
    }

    if ((status = android::register_com_android_bluetooth_sdp(e)) < 0) {
        ALOGE("jni sdp registration failure: %d", status);
        return JNI_ERR;
    }

    return JNI_VERSION_1_6;
}
