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

#define LOG_TAG "wifinan"

#include "jni.h"
#include "JniConstants.h"
#include <ScopedUtfChars.h>
#include <ScopedBytes.h>
#include <utils/misc.h>
#include <utils/Log.h>
#include <utils/String16.h>
#include <ctype.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <linux/if.h>
#include "wifi.h"
#include "wifi_hal.h"
#include "jni_helper.h"

namespace android {

static jclass mCls;                             /* saved WifiNanNative object */
static JavaVM *mVM = NULL;                      /* saved JVM pointer */

wifi_handle getWifiHandle(JNIHelper &helper, jclass cls);
wifi_interface_handle getIfaceHandle(JNIHelper &helper, jclass cls, jint index);

extern wifi_hal_fn hal_fn;

// Start NAN functions

static void OnNanNotifyResponse(transaction_id id, NanResponseMsg* msg) {
  ALOGD(
      "OnNanNotifyResponse: transaction_id=%d, status=%d, value=%d, response_type=%d",
      id, msg->status, msg->value, msg->response_type);

  JNIHelper helper(mVM);
  switch (msg->response_type) {
    case NAN_RESPONSE_PUBLISH:
      helper.reportEvent(mCls, "onNanNotifyResponsePublishSubscribe",
                         "(SIIII)V", (short) id, (int) msg->response_type,
                         (int) msg->status, (int) msg->value,
                         msg->body.publish_response.publish_id);
      break;
    case NAN_RESPONSE_SUBSCRIBE:
      helper.reportEvent(mCls, "onNanNotifyResponsePublishSubscribe",
                         "(SIIII)V", (short) id, (int) msg->response_type,
                         (int) msg->status, (int) msg->value,
                         msg->body.subscribe_response.subscribe_id);
      break;
    case NAN_GET_CAPABILITIES: {
      JNIObject<jobject> data = helper.createObject(
          "com/android/server/wifi/nan/WifiNanNative$Capabilities");
      if (data == NULL) {
        ALOGE(
            "Error in allocating WifiNanNative.Capabilities OnNanNotifyResponse");
        return;
      }

      helper.setIntField(
          data, "maxConcurrentNanClusters",
          (int) msg->body.nan_capabilities.max_concurrent_nan_clusters);
      helper.setIntField(data, "maxPublishes",
                         (int) msg->body.nan_capabilities.max_publishes);
      helper.setIntField(data, "maxSubscribes",
                         (int) msg->body.nan_capabilities.max_subscribes);
      helper.setIntField(data, "maxServiceNameLen",
                         (int) msg->body.nan_capabilities.max_service_name_len);
      helper.setIntField(data, "maxMatchFilterLen",
                         (int) msg->body.nan_capabilities.max_match_filter_len);
      helper.setIntField(
          data, "maxTotalMatchFilterLen",
          (int) msg->body.nan_capabilities.max_total_match_filter_len);
      helper.setIntField(
          data, "maxServiceSpecificInfoLen",
          (int) msg->body.nan_capabilities.max_service_specific_info_len);
      helper.setIntField(data, "maxVsaDataLen",
                         (int) msg->body.nan_capabilities.max_vsa_data_len);
      helper.setIntField(data, "maxMeshDataLen",
                         (int) msg->body.nan_capabilities.max_mesh_data_len);
      helper.setIntField(data, "maxNdiInterfaces",
                         (int) msg->body.nan_capabilities.max_ndi_interfaces);
      helper.setIntField(data, "maxNdpSessions",
                         (int) msg->body.nan_capabilities.max_ndp_sessions);
      helper.setIntField(data, "maxAppInfoLen",
                         (int) msg->body.nan_capabilities.max_app_info_len);

      helper.reportEvent(
          mCls, "onNanNotifyResponseCapabilities",
          "(SIILcom/android/server/wifi/nan/WifiNanNative$Capabilities;)V",
          (short) id, (int) msg->status, (int) msg->value, data.get());
      break;
    }
    default:
      helper.reportEvent(mCls, "onNanNotifyResponse", "(SIII)V", (short) id,
                         (int) msg->response_type, (int) msg->status,
                         (int) msg->value);
      break;
  }
}

static void OnNanEventPublishTerminated(NanPublishTerminatedInd* event) {
    ALOGD("OnNanEventPublishTerminated");

    JNIHelper helper(mVM);
    helper.reportEvent(mCls, "onPublishTerminated", "(II)V",
                       event->publish_id, event->reason);
}

static void OnNanEventMatch(NanMatchInd* event) {
    ALOGD("OnNanEventMatch");

    JNIHelper helper(mVM);

    JNIObject<jbyteArray> macBytes = helper.newByteArray(6);
    helper.setByteArrayRegion(macBytes, 0, 6, (jbyte *) event->addr);

    JNIObject<jbyteArray> ssiBytes = helper.newByteArray(event->service_specific_info_len);
    helper.setByteArrayRegion(ssiBytes, 0, event->service_specific_info_len,
                              (jbyte *) event->service_specific_info);

    JNIObject<jbyteArray> mfBytes = helper.newByteArray(event->sdf_match_filter_len);
    helper.setByteArrayRegion(mfBytes, 0, event->sdf_match_filter_len,
                              (jbyte *) event->sdf_match_filter);

    helper.reportEvent(mCls, "onMatchEvent", "(II[B[BI[BI)V",
                       (int) event->publish_subscribe_id,
                       (int) event->requestor_instance_id,
                       macBytes.get(),
                       ssiBytes.get(), event->service_specific_info_len,
                       mfBytes.get(), event->sdf_match_filter_len);
}

static void OnNanEventMatchExpired(NanMatchExpiredInd* event) {
    ALOGD("OnNanEventMatchExpired");
}

static void OnNanEventSubscribeTerminated(NanSubscribeTerminatedInd* event) {
    ALOGD("OnNanEventSubscribeTerminated");

    JNIHelper helper(mVM);
    helper.reportEvent(mCls, "onSubscribeTerminated", "(II)V",
                       event->subscribe_id, event->reason);
}

static void OnNanEventFollowup(NanFollowupInd* event) {
    ALOGD("OnNanEventFollowup");

    JNIHelper helper(mVM);

    JNIObject<jbyteArray> macBytes = helper.newByteArray(6);
    helper.setByteArrayRegion(macBytes, 0, 6, (jbyte *) event->addr);

    JNIObject<jbyteArray> msgBytes = helper.newByteArray(event->service_specific_info_len);
    helper.setByteArrayRegion(msgBytes, 0, event->service_specific_info_len, (jbyte *) event->service_specific_info);

    helper.reportEvent(mCls, "onFollowupEvent", "(II[B[BI)V",
                       (int) event->publish_subscribe_id,
                       (int) event->requestor_instance_id,
                       macBytes.get(),
                       msgBytes.get(),
                       (int) event->service_specific_info_len);
}

static void OnNanEventDiscEngEvent(NanDiscEngEventInd* event) {
    ALOGD("OnNanEventDiscEngEvent called: event_type=%d", event->event_type);

    JNIHelper helper(mVM);

    JNIObject<jbyteArray> macBytes = helper.newByteArray(6);
    if (event->event_type == NAN_EVENT_ID_DISC_MAC_ADDR) {
        helper.setByteArrayRegion(macBytes, 0, 6, (jbyte *) event->data.mac_addr.addr);
    } else {
        helper.setByteArrayRegion(macBytes, 0, 6, (jbyte *) event->data.cluster.addr);
    }

    helper.reportEvent(mCls, "onDiscoveryEngineEvent", "(I[B)V",
                       (int) event->event_type, macBytes.get());
}

static void OnNanEventDisabled(NanDisabledInd* event) {
    ALOGD("OnNanEventDisabled called: reason=%d", event->reason);

    JNIHelper helper(mVM);

    helper.reportEvent(mCls, "onDisabledEvent", "(I)V", (int) event->reason);
}

static void OnNanEventTca(NanTCAInd* event) {
    ALOGD("OnNanEventTca");
}

static void OnNanEventBeaconSdfPayload(NanBeaconSdfPayloadInd* event) {
    ALOGD("OnNanEventSdfPayload");
}

static jint android_net_wifi_nan_register_handler(JNIEnv *env, jclass cls,
                                                  jclass wifi_native_cls,
                                                  jint iface) {
    JNIHelper helper(env);
    wifi_interface_handle handle = getIfaceHandle(helper, wifi_native_cls, iface);

    ALOGD("android_net_wifi_nan_register_handler handle=%p", handle);

    NanCallbackHandler handlers;
    handlers.NotifyResponse = OnNanNotifyResponse;
    handlers.EventPublishTerminated = OnNanEventPublishTerminated;
    handlers.EventMatch = OnNanEventMatch;
    handlers.EventMatchExpired = OnNanEventMatchExpired;
    handlers.EventSubscribeTerminated = OnNanEventSubscribeTerminated;
    handlers.EventFollowup = OnNanEventFollowup;
    handlers.EventDiscEngEvent = OnNanEventDiscEngEvent;
    handlers.EventDisabled = OnNanEventDisabled;
    handlers.EventTca = OnNanEventTca;
    handlers.EventBeaconSdfPayload = OnNanEventBeaconSdfPayload;

    if (mVM == NULL) {
        env->GetJavaVM(&mVM);
        mCls = (jclass) env->NewGlobalRef(cls);
    }

    return hal_fn.wifi_nan_register_handler(handle, handlers);
}

static jint android_net_wifi_nan_enable_request(JNIEnv *env, jclass cls,
                                                jshort transaction_id,
                                                jclass wifi_native_cls,
                                                jint iface,
                                                jobject config_request) {
    JNIHelper helper(env);
    wifi_interface_handle handle = getIfaceHandle(helper, wifi_native_cls, iface);

    ALOGD("android_net_wifi_nan_enable_request handle=%p, id=%d",
          handle, transaction_id);

    NanEnableRequest msg;
    memset(&msg, 0, sizeof(NanEnableRequest));

    /* configurable settings */
    msg.config_support_5g = 1;
    msg.support_5g_val = helper.getBoolField(config_request, "mSupport5gBand");
    msg.master_pref = helper.getIntField(config_request, "mMasterPreference");
    msg.cluster_low = helper.getIntField(config_request, "mClusterLow");
    msg.cluster_high = helper.getIntField(config_request, "mClusterHigh");

    return hal_fn.wifi_nan_enable_request(transaction_id, handle, &msg);
}

static jint android_net_wifi_nan_get_capabilities(JNIEnv *env, jclass cls,
                                                  jshort transaction_id,
                                                  jclass wifi_native_cls,
                                                  jint iface) {
  JNIHelper helper(env);
  wifi_interface_handle handle = getIfaceHandle(helper, wifi_native_cls, iface);

  ALOGD("android_net_wifi_nan_get_capabilities handle=%p, id=%d", handle,
        transaction_id);

  return hal_fn.wifi_nan_get_capabilities(transaction_id, handle);
}

static jint android_net_wifi_nan_disable_request(JNIEnv *env, jclass cls,
                                                 jshort transaction_id,
                                                 jclass wifi_native_cls,
                                                 jint iface) {
    JNIHelper helper(env);
    wifi_interface_handle handle = getIfaceHandle(helper, wifi_native_cls, iface);

    ALOGD("android_net_wifi_nan_disable_request handle=%p, id=%d",
          handle, transaction_id);

    return hal_fn.wifi_nan_disable_request(transaction_id, handle);
}

static jint android_net_wifi_nan_publish(JNIEnv *env, jclass cls,
                                         jshort transaction_id,
                                         jint publish_id,
                                         jclass wifi_native_cls,
                                         jint iface,
                                         jobject publish_data,
                                         jobject publish_settings) {
    JNIHelper helper(env);
    wifi_interface_handle handle = getIfaceHandle(helper, wifi_native_cls, iface);

    ALOGD("android_net_wifi_nan_publish handle=%p, id=%d", handle, transaction_id);

    NanPublishRequest msg;
    memset(&msg, 0, sizeof(NanPublishRequest));

    /* hard-coded settings - TBD: move to configurable */
    msg.period = 500;
    msg.publish_match_indicator = NAN_MATCH_ALG_MATCH_ONCE;
    msg.rssi_threshold_flag = 0;
    msg.connmap = 0;

    /* configurable settings */
    msg.publish_id = publish_id;

    JNIObject<jstring> objStr1 = helper.getStringField(publish_data, "mServiceName");
    if (objStr1 == NULL) {
        ALOGE("Error accessing mServiceName field");
        return 0;
    }
    ScopedUtfChars chars1(env, objStr1);
    const char *serviceName = chars1.c_str();
    if (serviceName == NULL) {
        ALOGE("Error getting mServiceName");
        return 0;
    }
    msg.service_name_len = strlen(serviceName);
    strcpy((char*)msg.service_name, serviceName);

    msg.service_specific_info_len = helper.getIntField(publish_data, "mServiceSpecificInfoLength");
    if (msg.service_specific_info_len != 0) {
        helper.getByteArrayField(publish_data, "mServiceSpecificInfo",
                             msg.service_specific_info, msg.service_specific_info_len);
    }


    msg.tx_match_filter_len = helper.getIntField(publish_data, "mTxFilterLength");
    if (msg.tx_match_filter_len != 0) {
        helper.getByteArrayField(publish_data, "mTxFilter",
                             msg.tx_match_filter, msg.tx_match_filter_len);
    }

    msg.rx_match_filter_len = helper.getIntField(publish_data, "mRxFilterLength");
    if (msg.rx_match_filter_len != 0) {
        helper.getByteArrayField(publish_data, "mRxFilter",
                             msg.rx_match_filter, msg.rx_match_filter_len);
    }

    msg.publish_type = (NanPublishType)helper.getIntField(publish_settings, "mPublishType");
    msg.publish_count = helper.getIntField(publish_settings, "mPublishCount");
    msg.ttl = helper.getIntField(publish_settings, "mTtlSec");

    msg.tx_type = NAN_TX_TYPE_BROADCAST;
    if (msg.publish_type != NAN_PUBLISH_TYPE_UNSOLICITED)
      msg.tx_type = NAN_TX_TYPE_UNICAST;

    return hal_fn.wifi_nan_publish_request(transaction_id, handle, &msg);
}

static jint android_net_wifi_nan_subscribe(JNIEnv *env, jclass cls,
                                           jshort transaction_id,
                                           jint subscribe_id,
                                           jclass wifi_native_cls,
                                           jint iface,
                                           jobject subscribe_data,
                                           jobject subscribe_settings) {
    JNIHelper helper(env);
    wifi_interface_handle handle = getIfaceHandle(helper, wifi_native_cls, iface);

    ALOGD("android_net_wifi_nan_subscribe handle=%p, id=%d", handle, transaction_id);

    NanSubscribeRequest msg;
    memset(&msg, 0, sizeof(NanSubscribeRequest));

    /* hard-coded settings - TBD: move to configurable */
    msg.period = 500;
    msg.serviceResponseFilter = NAN_SRF_ATTR_PARTIAL_MAC_ADDR;
    msg.serviceResponseInclude = NAN_SRF_INCLUDE_RESPOND;
    msg.useServiceResponseFilter = NAN_DO_NOT_USE_SRF;
    msg.ssiRequiredForMatchIndication = NAN_SSI_NOT_REQUIRED_IN_MATCH_IND;
    msg.subscribe_match_indicator = NAN_MATCH_ALG_MATCH_ONCE;
    msg.rssi_threshold_flag = 0;
    msg.connmap = 0;
    msg.num_intf_addr_present = 0;

    /* configurable settings */
    msg.subscribe_id = subscribe_id;

    JNIObject<jstring> objStr1 = helper.getStringField(subscribe_data, "mServiceName");
    if (objStr1 == NULL) {
        ALOGE("Error accessing mServiceName field");
        return 0;
    }
    ScopedUtfChars chars1(env, objStr1);
    const char *serviceName = chars1.c_str();
    if (serviceName == NULL) {
        ALOGE("Error getting mServiceName");
        return 0;
    }
    msg.service_name_len = strlen(serviceName);
    strcpy((char*)msg.service_name, serviceName);

    msg.service_specific_info_len = helper.getIntField(subscribe_data, "mServiceSpecificInfoLength");
    if (msg.service_specific_info_len != 0) {
        helper.getByteArrayField(subscribe_data, "mServiceSpecificInfo",
                             msg.service_specific_info, msg.service_specific_info_len);
    }

    msg.tx_match_filter_len = helper.getIntField(subscribe_data, "mTxFilterLength");
    if (msg.tx_match_filter_len != 0) {
        helper.getByteArrayField(subscribe_data, "mTxFilter",
                             msg.tx_match_filter, msg.tx_match_filter_len);
    }

    msg.rx_match_filter_len = helper.getIntField(subscribe_data, "mRxFilterLength");
    if (msg.rx_match_filter_len != 0) {
        helper.getByteArrayField(subscribe_data, "mRxFilter",
                             msg.rx_match_filter, msg.rx_match_filter_len);
    }

    msg.subscribe_type = (NanSubscribeType)helper.getIntField(subscribe_settings, "mSubscribeType");
    msg.subscribe_count = helper.getIntField(subscribe_settings, "mSubscribeCount");
    msg.ttl = helper.getIntField(subscribe_settings, "mTtlSec");

    return hal_fn.wifi_nan_subscribe_request(transaction_id, handle, &msg);
}

static jint android_net_wifi_nan_send_message(JNIEnv *env, jclass cls,
                                              jshort transaction_id,
                                              jclass wifi_native_cls,
                                              jint iface,
                                              jint pub_sub_id,
                                              jint req_instance_id,
                                              jbyteArray dest,
                                              jbyteArray message,
                                              jint message_length) {
    JNIHelper helper(env);
    wifi_interface_handle handle = getIfaceHandle(helper, wifi_native_cls, iface);

    ALOGD("android_net_wifi_nan_send_message handle=%p, id=%d", handle, transaction_id);

    NanTransmitFollowupRequest msg;
    memset(&msg, 0, sizeof(NanTransmitFollowupRequest));

    /* hard-coded settings - TBD: move to configurable */
    msg.publish_subscribe_id = pub_sub_id;
    msg.requestor_instance_id = req_instance_id;
    msg.priority = NAN_TX_PRIORITY_NORMAL;
    msg.dw_or_faw = NAN_TRANSMIT_IN_DW;

    /* configurable settings */
    msg.service_specific_info_len = message_length;

    ScopedBytesRO messageBytes(env, message);
    memcpy(msg.service_specific_info, (byte*) messageBytes.get(), message_length);

    ScopedBytesRO destBytes(env, dest);
    memcpy(msg.addr, (byte*) destBytes.get(), 6);

    return hal_fn.wifi_nan_transmit_followup_request(transaction_id, handle, &msg);
}

static jint android_net_wifi_nan_stop_publish(JNIEnv *env, jclass cls,
                                              jshort transaction_id,
                                              jclass wifi_native_cls,
                                              jint iface,
                                              jint pub_sub_id) {
    JNIHelper helper(env);
    wifi_interface_handle handle = getIfaceHandle(helper, wifi_native_cls, iface);

    ALOGD("android_net_wifi_nan_stop_publish handle=%p, id=%d", handle, transaction_id);

    NanPublishCancelRequest msg;
    memset(&msg, 0, sizeof(NanPublishCancelRequest));

    msg.publish_id = pub_sub_id;

    return hal_fn.wifi_nan_publish_cancel_request(transaction_id, handle, &msg);
}

static jint android_net_wifi_nan_stop_subscribe(JNIEnv *env, jclass cls,
                                              jshort transaction_id,
                                              jclass wifi_native_cls,
                                              jint iface,
                                              jint pub_sub_id) {
    JNIHelper helper(env);
    wifi_interface_handle handle = getIfaceHandle(helper, wifi_native_cls, iface);

    ALOGD("android_net_wifi_nan_stop_subscribe handle=%p, id=%d", handle, transaction_id);

    NanSubscribeCancelRequest msg;
    memset(&msg, 0, sizeof(NanSubscribeCancelRequest));

    msg.subscribe_id = pub_sub_id;

    return hal_fn.wifi_nan_subscribe_cancel_request(transaction_id, handle, &msg);
}

// ----------------------------------------------------------------------------

/*
 * JNI registration.
 */

static JNINativeMethod gWifiNanMethods[] = {
    /* name, signature, funcPtr */

    {"initNanHandlersNative", "(Ljava/lang/Object;I)I", (void*)android_net_wifi_nan_register_handler },
    {"getCapabilitiesNative", "(SLjava/lang/Object;I)I", (void*)android_net_wifi_nan_get_capabilities },
    {"enableAndConfigureNative", "(SLjava/lang/Object;ILandroid/net/wifi/nan/ConfigRequest;)I", (void*)android_net_wifi_nan_enable_request },
    {"disableNative", "(SLjava/lang/Object;I)I", (void*)android_net_wifi_nan_disable_request },
    {"publishNative", "(SILjava/lang/Object;ILandroid/net/wifi/nan/PublishData;Landroid/net/wifi/nan/PublishSettings;)I", (void*)android_net_wifi_nan_publish },
    {"subscribeNative", "(SILjava/lang/Object;ILandroid/net/wifi/nan/SubscribeData;Landroid/net/wifi/nan/SubscribeSettings;)I", (void*)android_net_wifi_nan_subscribe },
    {"sendMessageNative", "(SLjava/lang/Object;III[B[BI)I", (void*)android_net_wifi_nan_send_message },
    {"stopPublishNative", "(SLjava/lang/Object;II)I", (void*)android_net_wifi_nan_stop_publish },
    {"stopSubscribeNative", "(SLjava/lang/Object;II)I", (void*)android_net_wifi_nan_stop_subscribe },
};

/* User to register native functions */
extern "C"
jint Java_com_android_server_wifi_nan_WifiNanNative_registerNanNatives(JNIEnv* env, jclass clazz) {
    return jniRegisterNativeMethods(env,
            "com/android/server/wifi/nan/WifiNanNative", gWifiNanMethods, NELEM(gWifiNanMethods));
}

}; // namespace android
