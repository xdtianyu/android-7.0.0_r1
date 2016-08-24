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

#include <stdint.h>
#include "JniConstants.h"
#include <ScopedUtfChars.h>
#include <ScopedBytes.h>
#include <utils/misc.h>
#include <android_runtime/AndroidRuntime.h>
#include <utils/Log.h>
#include <utils/String16.h>
#include <ctype.h>
#include <sys/socket.h>
#include <linux/if.h>
#include "wifi.h"
#include "wifi_hal.h"
#include "jni_helper.h"
#include "wifi_hal_mock.h"
#include <sstream>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

namespace android {

extern jobject mock_mObj; /* saved NanHalMock object */
extern JavaVM* mock_mVM; /* saved JVM pointer */

/* Variable and function declared and defined in:
 *  com_android_servier_wifi_nan_WifiNanNative.cpp
 */
extern wifi_hal_fn hal_fn;
extern "C" jint Java_com_android_server_wifi_nan_WifiNanNative_registerNanNatives(
    JNIEnv* env, jclass clazz);

static NanCallbackHandler mCallbackHandlers;

wifi_error wifi_nan_enable_request_mock(transaction_id id,
                                        wifi_interface_handle iface,
                                        NanEnableRequest* msg) {
  JNIHelper helper(mock_mVM);

  ALOGD("wifi_nan_enable_request_mock");
  HalMockJsonWriter jsonW;
  jsonW.put_int("master_pref", msg->master_pref);
  jsonW.put_int("cluster_low", msg->cluster_low);
  jsonW.put_int("cluster_high", msg->cluster_high);
  jsonW.put_int("config_support_5g", msg->config_support_5g);
  jsonW.put_int("support_5g_val", msg->support_5g_val);
  jsonW.put_int("config_sid_beacon", msg->config_sid_beacon);
  jsonW.put_int("sid_beacon_val", msg->sid_beacon_val);
  jsonW.put_int("config_2dot4g_rssi_close", msg->config_2dot4g_rssi_close);
  jsonW.put_int("rssi_close_2dot4g_val", msg->rssi_close_2dot4g_val);
  jsonW.put_int("config_2dot4g_rssi_middle", msg->config_2dot4g_rssi_middle);
  jsonW.put_int("rssi_middle_2dot4g_val", msg->rssi_middle_2dot4g_val);
  jsonW.put_int("config_2dot4g_rssi_proximity",
                msg->config_2dot4g_rssi_proximity);
  jsonW.put_int("rssi_proximity_2dot4g_val", msg->rssi_proximity_2dot4g_val);
  jsonW.put_int("config_hop_count_limit", msg->config_hop_count_limit);
  jsonW.put_int("hop_count_limit_val", msg->hop_count_limit_val);
  jsonW.put_int("config_2dot4g_support", msg->config_2dot4g_support);
  jsonW.put_int("support_2dot4g_val", msg->support_2dot4g_val);
  jsonW.put_int("config_2dot4g_beacons", msg->config_2dot4g_beacons);
  jsonW.put_int("beacon_2dot4g_val", msg->beacon_2dot4g_val);
  jsonW.put_int("config_2dot4g_sdf", msg->config_2dot4g_sdf);
  jsonW.put_int("sdf_2dot4g_val", msg->sdf_2dot4g_val);
  jsonW.put_int("config_5g_beacons", msg->config_5g_beacons);
  jsonW.put_int("beacon_5g_val", msg->beacon_5g_val);
  jsonW.put_int("config_5g_sdf", msg->config_5g_sdf);
  jsonW.put_int("sdf_5g_val", msg->sdf_5g_val);
  jsonW.put_int("config_5g_rssi_close", msg->config_5g_rssi_close);
  jsonW.put_int("rssi_close_5g_val", msg->rssi_close_5g_val);
  jsonW.put_int("config_5g_rssi_middle", msg->config_5g_rssi_middle);
  jsonW.put_int("rssi_middle_5g_val", msg->rssi_middle_5g_val);
  jsonW.put_int("config_5g_rssi_close_proximity",
                msg->config_5g_rssi_close_proximity);
  jsonW.put_int("rssi_close_proximity_5g_val",
                msg->rssi_close_proximity_5g_val);
  jsonW.put_int("config_rssi_window_size", msg->config_rssi_window_size);
  jsonW.put_int("rssi_window_size_val", msg->rssi_window_size_val);
  jsonW.put_int("config_oui", msg->config_oui);
  jsonW.put_int("oui_val", msg->oui_val);
  jsonW.put_int("config_intf_addr", msg->config_intf_addr);
  jsonW.put_byte_array("intf_addr_val", msg->intf_addr_val, 6);
  jsonW.put_int("config_cluster_attribute_val",
                msg->config_cluster_attribute_val);
  jsonW.put_int("config_scan_params", msg->config_scan_params);
  jsonW.put_int("scan_params_val.dwell_time.0",
                msg->scan_params_val.dwell_time[NAN_CHANNEL_24G_BAND]);
  jsonW.put_int("scan_params_val.dwell_time.1",
                msg->scan_params_val.dwell_time[NAN_CHANNEL_5G_BAND_LOW]);
  jsonW.put_int("scan_params_val.dwell_time.2",
                msg->scan_params_val.dwell_time[NAN_CHANNEL_5G_BAND_HIGH]);
  jsonW.put_int("scan_params_val.scan_period.0",
                msg->scan_params_val.scan_period[NAN_CHANNEL_24G_BAND]);
  jsonW.put_int("scan_params_val.scan_period.0",
                msg->scan_params_val.scan_period[NAN_CHANNEL_5G_BAND_LOW]);
  jsonW.put_int("scan_params_val.scan_period.0",
                msg->scan_params_val.scan_period[NAN_CHANNEL_5G_BAND_HIGH]);
  jsonW.put_int("config_random_factor_force", msg->config_random_factor_force);
  jsonW.put_int("random_factor_force_val", msg->random_factor_force_val);
  jsonW.put_int("config_hop_count_force", msg->config_hop_count_force);
  jsonW.put_int("hop_count_force_val", msg->hop_count_force_val);
  std::string str = jsonW.to_string();

  JNIObject < jstring > json_write_string = helper.newStringUTF(str.c_str());

  helper.callMethod(mock_mObj, "enableHalMockNative", "(SLjava/lang/String;)V",
                    (short) id, json_write_string.get());

  return WIFI_SUCCESS;
}

wifi_error wifi_nan_disable_request_mock(transaction_id id,
                                         wifi_interface_handle iface) {
  JNIHelper helper(mock_mVM);

  ALOGD("wifi_nan_disable_request_mock");
  helper.callMethod(mock_mObj, "disableHalMockNative", "(S)V", (short) id);

  return WIFI_SUCCESS;
}

wifi_error wifi_nan_publish_request_mock(transaction_id id,
                                         wifi_interface_handle iface,
                                         NanPublishRequest* msg) {
  JNIHelper helper(mock_mVM);

  ALOGD("wifi_nan_publish_request_mock");
  HalMockJsonWriter jsonW;
  jsonW.put_int("publish_id", msg->publish_id);
  jsonW.put_int("ttl", msg->ttl);
  jsonW.put_int("publish_type", msg->publish_type);
  jsonW.put_int("tx_type", msg->tx_type);
  jsonW.put_int("publish_count", msg->publish_count);
  jsonW.put_int("service_name_len", msg->service_name_len);
  jsonW.put_byte_array("service_name", msg->service_name,
                       msg->service_name_len);
  jsonW.put_int("publish_match_indicator", msg->publish_match_indicator);
  jsonW.put_int("service_specific_info_len", msg->service_specific_info_len);
  jsonW.put_byte_array("service_specific_info", msg->service_specific_info,
                       msg->service_specific_info_len);
  jsonW.put_int("rx_match_filter_len", msg->rx_match_filter_len);
  jsonW.put_byte_array("rx_match_filter", msg->rx_match_filter,
                       msg->rx_match_filter_len);
  jsonW.put_int("tx_match_filter_len", msg->tx_match_filter_len);
  jsonW.put_byte_array("tx_match_filter", msg->tx_match_filter,
                       msg->tx_match_filter_len);
  jsonW.put_int("rssi_threshold_flag", msg->rssi_threshold_flag);
  jsonW.put_int("connmap", msg->connmap);
  std::string str = jsonW.to_string();

  JNIObject < jstring > json_write_string = helper.newStringUTF(str.c_str());

  helper.callMethod(mock_mObj, "publishHalMockNative", "(SLjava/lang/String;)V",
                    (short) id, json_write_string.get());
  return WIFI_SUCCESS;
}

wifi_error wifi_nan_publish_cancel_request_mock(transaction_id id,
                                                wifi_interface_handle iface,
                                                NanPublishCancelRequest* msg) {
  JNIHelper helper(mock_mVM);

  ALOGD("wifi_nan_publish_cancel_request_mock");
  HalMockJsonWriter jsonW;
  jsonW.put_int("publish_id", msg->publish_id);
  std::string str = jsonW.to_string();

  JNIObject < jstring > json_write_string = helper.newStringUTF(str.c_str());

  helper.callMethod(mock_mObj, "publishCancelHalMockNative",
                    "(SLjava/lang/String;)V", (short) id,
                    json_write_string.get());
  return WIFI_SUCCESS;
}

wifi_error wifi_nan_subscribe_request_mock(transaction_id id,
                                           wifi_interface_handle iface,
                                           NanSubscribeRequest* msg) {
  JNIHelper helper(mock_mVM);

  ALOGD("wifi_nan_subscribe_request_mock");
  HalMockJsonWriter jsonW;
  jsonW.put_int("subscribe_id", msg->subscribe_id);
  jsonW.put_int("ttl", msg->ttl);
  jsonW.put_int("period", msg->period);
  jsonW.put_int("subscribe_type", msg->subscribe_type);
  jsonW.put_int("serviceResponseFilter", msg->serviceResponseFilter);
  jsonW.put_int("serviceResponseInclude", msg->serviceResponseInclude);
  jsonW.put_int("useServiceResponseFilter", msg->useServiceResponseFilter);
  jsonW.put_int("ssiRequiredForMatchIndication",
                msg->ssiRequiredForMatchIndication);
  jsonW.put_int("subscribe_match_indicator", msg->subscribe_match_indicator);
  jsonW.put_int("subscribe_count", msg->subscribe_count);
  jsonW.put_int("service_name_len", msg->service_name_len);
  jsonW.put_byte_array("service_name", msg->service_name,
                       msg->service_name_len);
  jsonW.put_int("service_specific_info_len", msg->service_name_len);
  jsonW.put_byte_array("service_specific_info", msg->service_specific_info,
                       msg->service_specific_info_len);
  jsonW.put_int("rx_match_filter_len", msg->rx_match_filter_len);
  jsonW.put_byte_array("rx_match_filter", msg->rx_match_filter,
                       msg->rx_match_filter_len);
  jsonW.put_int("tx_match_filter_len", msg->tx_match_filter_len);
  jsonW.put_byte_array("tx_match_filter", msg->tx_match_filter,
                       msg->tx_match_filter_len);
  jsonW.put_int("rssi_threshold_flag", msg->rssi_threshold_flag);
  jsonW.put_int("connmap", msg->connmap);
  jsonW.put_int("num_intf_addr_present", msg->num_intf_addr_present);
  // TODO: jsonW.put_byte_array("intf_addr", msg->intf_addr, NAN_MAX_SUBSCRIBE_MAX_ADDRESS * NAN_MAC_ADDR_LEN);
  std::string str = jsonW.to_string();

  JNIObject < jstring > json_write_string = helper.newStringUTF(str.c_str());

  helper.callMethod(mock_mObj, "subscribeHalMockNative",
                    "(SLjava/lang/String;)V", (short) id,
                    json_write_string.get());
  return WIFI_SUCCESS;
}

wifi_error wifi_nan_subscribe_cancel_request_mock(
    transaction_id id, wifi_interface_handle iface,
    NanSubscribeCancelRequest* msg) {
  JNIHelper helper(mock_mVM);

  ALOGD("wifi_nan_subscribe_cancel_request_mock");
  HalMockJsonWriter jsonW;
  jsonW.put_int("subscribe_id", msg->subscribe_id);
  std::string str = jsonW.to_string();

  JNIObject < jstring > json_write_string = helper.newStringUTF(str.c_str());

  helper.callMethod(mock_mObj, "subscribeCancelHalMockNative",
                    "(SLjava/lang/String;)V", (short) id,
                    json_write_string.get());
  return WIFI_SUCCESS;
}

wifi_error wifi_nan_transmit_followup_request_mock(
    transaction_id id, wifi_interface_handle iface,
    NanTransmitFollowupRequest* msg) {
  JNIHelper helper(mock_mVM);

  ALOGD("wifi_nan_transmit_followup_request_mock");
  HalMockJsonWriter jsonW;
  jsonW.put_int("publish_subscribe_id", msg->publish_subscribe_id);
  jsonW.put_int("requestor_instance_id", msg->requestor_instance_id);
  jsonW.put_byte_array("addr", msg->addr, 6);
  jsonW.put_int("priority", msg->priority);
  jsonW.put_int("dw_or_faw", msg->dw_or_faw);
  jsonW.put_int("service_specific_info_len", msg->service_specific_info_len);
  jsonW.put_byte_array("service_specific_info", msg->service_specific_info,
                       msg->service_specific_info_len);

  std::string str = jsonW.to_string();

  JNIObject < jstring > json_write_string = helper.newStringUTF(str.c_str());

  helper.callMethod(mock_mObj, "transmitFollowupHalMockNative",
                    "(SLjava/lang/String;)V", (short) id,
                    json_write_string.get());
  return WIFI_SUCCESS;
}

wifi_error wifi_nan_stats_request_mock(transaction_id id,
                                       wifi_interface_handle iface,
                                       NanStatsRequest* msg) {
  ALOGD("wifi_nan_stats_request_mock");
  return WIFI_ERROR_UNINITIALIZED;
}

wifi_error wifi_nan_config_request_mock(transaction_id id,
                                        wifi_interface_handle iface,
                                        NanConfigRequest* msg) {
  ALOGD("wifi_nan_config_request_mock");
  return WIFI_ERROR_UNINITIALIZED;
}

wifi_error wifi_nan_tca_request_mock(transaction_id id,
                                     wifi_interface_handle iface,
                                     NanTCARequest* msg) {
  ALOGD("wifi_nan_tca_request_mock");
  return WIFI_ERROR_UNINITIALIZED;
}

wifi_error wifi_nan_beacon_sdf_payload_request_mock(
    transaction_id id, wifi_interface_handle iface,
    NanBeaconSdfPayloadRequest* msg) {
  ALOGD("wifi_nan_beacon_sdf_payload_request_mock");
  return WIFI_ERROR_UNINITIALIZED;
}

wifi_error wifi_nan_register_handler_mock(wifi_interface_handle iface,
                                          NanCallbackHandler handlers) {
  ALOGD("wifi_nan_register_handler_mock");
  mCallbackHandlers = handlers;
  return WIFI_SUCCESS;
}

wifi_error wifi_nan_get_version_mock(wifi_handle handle, NanVersion* version) {
  ALOGD("wifi_nan_get_version_mock");
  return WIFI_ERROR_UNINITIALIZED;
}

wifi_error wifi_nan_get_capabilities_mock(transaction_id id,
                                          wifi_interface_handle iface) {
  JNIHelper helper(mock_mVM);

  ALOGD("wifi_nan_get_capabilities_mock");

  helper.callMethod(mock_mObj, "getCapabilitiesHalMockNative", "(S)V",
                    (short) id);
  return WIFI_SUCCESS;
}

// Callbacks

extern "C" void Java_com_android_server_wifi_nan_WifiNanHalMock_callNotifyResponse(
    JNIEnv* env, jclass clazz, jshort transaction_id,
    jstring json_args_jstring) {
  ScopedUtfChars chars(env, json_args_jstring);
  HalMockJsonReader jsonR(chars.c_str());
  bool error = false;

  ALOGD("Java_com_android_server_wifi_nan_WifiNanHalMock_callNotifyResponse: '%s'",
        chars.c_str());

  NanResponseMsg msg;
  msg.status = (NanStatusType) jsonR.get_int("status", &error);
  msg.value = jsonR.get_int("value", &error);
  msg.response_type = (NanResponseType) jsonR.get_int("response_type", &error);
  if (msg.response_type == NAN_RESPONSE_PUBLISH) {
    msg.body.publish_response.publish_id = jsonR.get_int(
        "body.publish_response.publish_id", &error);
  } else if (msg.response_type == NAN_RESPONSE_SUBSCRIBE) {
    msg.body.subscribe_response.subscribe_id = jsonR.get_int(
        "body.subscribe_response.subscribe_id", &error);
  } else if (msg.response_type == NAN_GET_CAPABILITIES) {
    msg.body.nan_capabilities.max_concurrent_nan_clusters = jsonR.get_int(
        "body.nan_capabilities.max_concurrent_nan_clusters", &error);
    msg.body.nan_capabilities.max_publishes = jsonR.get_int(
        "body.nan_capabilities.max_publishes", &error);
    msg.body.nan_capabilities.max_subscribes = jsonR.get_int(
        "body.nan_capabilities.max_subscribes", &error);
    msg.body.nan_capabilities.max_service_name_len = jsonR.get_int(
        "body.nan_capabilities.max_service_name_len", &error);
    msg.body.nan_capabilities.max_match_filter_len = jsonR.get_int(
        "body.nan_capabilities.max_match_filter_len", &error);
    msg.body.nan_capabilities.max_total_match_filter_len = jsonR.get_int(
        "body.nan_capabilities.max_total_match_filter_len", &error);
    msg.body.nan_capabilities.max_service_specific_info_len = jsonR.get_int(
        "body.nan_capabilities.max_service_specific_info_len", &error);
    msg.body.nan_capabilities.max_vsa_data_len = jsonR.get_int(
        "body.nan_capabilities.max_vsa_data_len", &error);
    msg.body.nan_capabilities.max_mesh_data_len = jsonR.get_int(
        "body.nan_capabilities.max_mesh_data_len", &error);
    msg.body.nan_capabilities.max_ndi_interfaces = jsonR.get_int(
        "body.nan_capabilities.max_ndi_interfaces", &error);
    msg.body.nan_capabilities.max_ndp_sessions = jsonR.get_int(
        "body.nan_capabilities.max_ndp_sessions", &error);
    msg.body.nan_capabilities.max_app_info_len = jsonR.get_int(
        "body.nan_capabilities.max_app_info_len", &error);
  }

  if (error) {
    ALOGE("Java_com_android_server_wifi_nan_WifiNanHalMock_callNotifyResponse: "
          "error parsing args");
    return;
  }

  mCallbackHandlers.NotifyResponse(transaction_id, &msg);
}

extern "C" void Java_com_android_server_wifi_nan_WifiNanHalMock_callPublishTerminated(
    JNIEnv* env, jclass clazz, jstring json_args_jstring) {
  ScopedUtfChars chars(env, json_args_jstring);
  HalMockJsonReader jsonR(chars.c_str());
  bool error = false;

  ALOGD(
      "Java_com_android_server_wifi_nan_WifiNanHalMock_callPublishTerminated: '%s'",
      chars.c_str());

  NanPublishTerminatedInd msg;
  msg.publish_id = jsonR.get_int("publish_id", &error);
  msg.reason = (NanStatusType) jsonR.get_int("reason", &error);

  if (error) {
    ALOGE("Java_com_android_server_wifi_nan_WifiNanHalMock_callPublishTerminated: "
          "error parsing args");
    return;
  }

  mCallbackHandlers.EventPublishTerminated(&msg);
}

extern "C" void Java_com_android_server_wifi_nan_WifiNanHalMock_callSubscribeTerminated(
    JNIEnv* env, jclass clazz, jstring json_args_jstring) {
  ScopedUtfChars chars(env, json_args_jstring);
  HalMockJsonReader jsonR(chars.c_str());
  bool error = false;

  ALOGD(
      "Java_com_android_server_wifi_nan_WifiNanHalMock_callSubscribeTerminated: '%s'",
      chars.c_str());

  NanSubscribeTerminatedInd msg;
  msg.subscribe_id = jsonR.get_int("subscribe_id", &error);
  msg.reason = (NanStatusType) jsonR.get_int("reason", &error);

  if (error) {
    ALOGE("Java_com_android_server_wifi_nan_WifiNanHalMock_callSubscribeTerminated:"
          " error parsing args");
    return;
  }

  mCallbackHandlers.EventSubscribeTerminated(&msg);
}

extern "C" void Java_com_android_server_wifi_nan_WifiNanHalMock_callFollowup(
    JNIEnv* env, jclass clazz, jstring json_args_jstring) {
  ScopedUtfChars chars(env, json_args_jstring);
  HalMockJsonReader jsonR(chars.c_str());
  bool error = false;

  ALOGD("Java_com_android_server_wifi_nan_WifiNanHalMock_callFollowup: '%s'",
        chars.c_str());

  NanFollowupInd msg;
  msg.publish_subscribe_id = jsonR.get_int("publish_subscribe_id", &error);
  msg.requestor_instance_id = jsonR.get_int("requestor_instance_id", &error);
  jsonR.get_byte_array("addr", &error, msg.addr, NAN_MAC_ADDR_LEN);
  msg.dw_or_faw = jsonR.get_int("dw_or_faw", &error);
  msg.service_specific_info_len = jsonR.get_int("service_specific_info_len",
                                                &error);
  jsonR.get_byte_array("service_specific_info", &error,
                       msg.service_specific_info,
                       NAN_MAX_SERVICE_SPECIFIC_INFO_LEN);

  if (error) {
    ALOGE("Java_com_android_server_wifi_nan_WifiNanHalMock_callFollowup: "
          "error parsing args");
    return;
  }

  mCallbackHandlers.EventFollowup(&msg);
}

extern "C" void Java_com_android_server_wifi_nan_WifiNanHalMock_callMatch(
    JNIEnv* env, jclass clazz, jstring json_args_jstring) {
  ScopedUtfChars chars(env, json_args_jstring);
  HalMockJsonReader jsonR(chars.c_str());
  bool error = false;

  ALOGD("Java_com_android_server_wifi_nan_WifiNanHalMock_callMatch: '%s'",
        chars.c_str());

  NanMatchInd msg;
  msg.publish_subscribe_id = jsonR.get_int("publish_subscribe_id", &error);
  msg.requestor_instance_id = jsonR.get_int("requestor_instance_id", &error);
  jsonR.get_byte_array("addr", &error, msg.addr, NAN_MAC_ADDR_LEN);
  msg.service_specific_info_len = jsonR.get_int("service_specific_info_len",
                                                &error);
  jsonR.get_byte_array("service_specific_info", &error,
                       msg.service_specific_info,
                       NAN_MAX_SERVICE_SPECIFIC_INFO_LEN);
  msg.sdf_match_filter_len = jsonR.get_int("sdf_match_filter_len", &error);
  jsonR.get_byte_array("sdf_match_filter", &error, msg.sdf_match_filter,
                       NAN_MAX_MATCH_FILTER_LEN);
  /* a few more fields here - but not used (yet/never?) */

  if (error) {
    ALOGE("Java_com_android_server_wifi_nan_WifiNanHalMock_callMatch: "
          "error parsing args");
    return;
  }

  mCallbackHandlers.EventMatch(&msg);
}

extern "C" void Java_com_android_server_wifi_nan_WifiNanHalMock_callDiscEngEvent(
    JNIEnv* env, jclass clazz, jstring json_args_jstring) {
  ScopedUtfChars chars(env, json_args_jstring);
  HalMockJsonReader jsonR(chars.c_str());
  bool error = false;

  ALOGD("Java_com_android_server_wifi_nan_WifiNanHalMock_callDiscEngEvent: '%s'",
        chars.c_str());

  NanDiscEngEventInd msg;
  msg.event_type = (NanDiscEngEventType) jsonR.get_int("event_type", &error);
  if (msg.event_type == NAN_EVENT_ID_DISC_MAC_ADDR) {
    jsonR.get_byte_array("data", &error, msg.data.mac_addr.addr,
                         NAN_MAC_ADDR_LEN);
  } else {
    jsonR.get_byte_array("data", &error, msg.data.cluster.addr,
                         NAN_MAC_ADDR_LEN);
  }

  if (error) {
    ALOGE("Java_com_android_server_wifi_nan_WifiNanHalMock_callDiscEngEvent: "
          "error parsing args");
    return;
  }

  mCallbackHandlers.EventDiscEngEvent(&msg);
}

extern "C" void Java_com_android_server_wifi_nan_WifiNanHalMock_callDisabled(
    JNIEnv* env, jclass clazz, jstring json_args_jstring) {
  ScopedUtfChars chars(env, json_args_jstring);
  HalMockJsonReader jsonR(chars.c_str());
  bool error = false;

  ALOGD("Java_com_android_server_wifi_nan_WifiNanHalMock_callDisabled: '%s'",
        chars.c_str());

  NanDisabledInd msg;
  msg.reason = (NanStatusType) jsonR.get_int("reason", &error);

  if (error) {
    ALOGE("Java_com_android_server_wifi_nan_WifiNanHalMock_callDisabled: "
          "error parsing args");
    return;
  }

  mCallbackHandlers.EventDisabled(&msg);
}

// TODO: Not currently used: add as needed
//void (*EventUnMatch) (NanUnmatchInd* event);
//void (*EventTca) (NanTCAInd* event);
//void (*EventBeaconSdfPayload) (NanBeaconSdfPayloadInd* event);

int init_wifi_nan_hal_func_table_mock(wifi_hal_fn *fn) {
  if (fn == NULL) {
    return -1;
  }

  fn->wifi_nan_enable_request = wifi_nan_enable_request_mock;
  fn->wifi_nan_disable_request = wifi_nan_disable_request_mock;
  fn->wifi_nan_publish_request = wifi_nan_publish_request_mock;
  fn->wifi_nan_publish_cancel_request =
      wifi_nan_publish_cancel_request_mock;
  fn->wifi_nan_subscribe_request = wifi_nan_subscribe_request_mock;
  fn->wifi_nan_subscribe_cancel_request =
      wifi_nan_subscribe_cancel_request_mock;
  fn->wifi_nan_transmit_followup_request =
      wifi_nan_transmit_followup_request_mock;
  fn->wifi_nan_stats_request = wifi_nan_stats_request_mock;
  fn->wifi_nan_config_request = wifi_nan_config_request_mock;
  fn->wifi_nan_tca_request = wifi_nan_tca_request_mock;
  fn->wifi_nan_beacon_sdf_payload_request =
      wifi_nan_beacon_sdf_payload_request_mock;
  fn->wifi_nan_register_handler = wifi_nan_register_handler_mock;
  fn->wifi_nan_get_version = wifi_nan_get_version_mock;
  fn->wifi_nan_get_capabilities = wifi_nan_get_capabilities_mock;

  return 0;
}

extern "C" jint Java_com_android_server_wifi_nan_WifiNanHalMock_initNanHalMock(
    JNIEnv* env, jclass clazz) {
  Java_com_android_server_wifi_nan_WifiNanNative_registerNanNatives(env, clazz);
  return init_wifi_nan_hal_func_table_mock(&hal_fn);
}

}// namespace android
