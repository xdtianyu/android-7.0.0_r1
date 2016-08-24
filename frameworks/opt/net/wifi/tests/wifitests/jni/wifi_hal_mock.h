/*
 * Copyright (C) 2016 The Android Open Source Project
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

#ifndef __WIFI_HAL_MOCK_H__
#define __WIFI_HAL_MOCK_H__

#include "wifi_hal.h"

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

namespace android {

class HalMockJsonWriter {
 public:
  HalMockJsonWriter();

  void put_int(const char* name, int x);

  void put_byte_array(const char* name, u8* byte_array, int array_length);

  std::string to_string();

 private:
  rapidjson::Document doc;
  rapidjson::Document::AllocatorType& allocator;
};

class HalMockJsonReader {
 public:
  HalMockJsonReader(const char* str);

  int get_int(const char* key, bool* error);

  void get_byte_array(const char* key, bool* error, u8* array,
                      unsigned int max_array_size);
 private:
  rapidjson::Document doc;
};

/* declare all HAL mock APIs here*/
wifi_error wifi_nan_enable_request_mock(transaction_id id,
                                        wifi_interface_handle iface,
                                        NanEnableRequest* msg);
wifi_error wifi_nan_disable_request_mock(transaction_id id,
                                         wifi_interface_handle iface);
wifi_error wifi_nan_publish_request_mock(transaction_id id,
                                         wifi_interface_handle iface,
                                         NanPublishRequest* msg);
wifi_error wifi_nan_publish_cancel_request_mock(transaction_id id,
                                                wifi_interface_handle iface,
                                                NanPublishCancelRequest* msg);
wifi_error wifi_nan_subscribe_request_mock(transaction_id id,
                                           wifi_interface_handle iface,
                                           NanSubscribeRequest* msg);
wifi_error wifi_nan_subscribe_cancel_request_mock(
    transaction_id id, wifi_interface_handle iface,
    NanSubscribeCancelRequest* msg);
wifi_error wifi_nan_transmit_followup_request_mock(
    transaction_id id, wifi_interface_handle iface,
    NanTransmitFollowupRequest* msg);
wifi_error wifi_nan_stats_request_mock(transaction_id id,
                                       wifi_interface_handle iface,
                                       NanStatsRequest* msg);
wifi_error wifi_nan_config_request_mock(transaction_id id,
                                        wifi_interface_handle iface,
                                        NanConfigRequest* msg);
wifi_error wifi_nan_tca_request_mock(transaction_id id,
                                     wifi_interface_handle iface,
                                     NanTCARequest* msg);
wifi_error wifi_nan_beacon_sdf_payload_request_mock(
    transaction_id id, wifi_interface_handle iface,
    NanBeaconSdfPayloadRequest* msg);
wifi_error wifi_nan_register_handler_mock(wifi_interface_handle iface,
                                          NanCallbackHandler handlers);
wifi_error wifi_nan_get_version_mock(wifi_handle handle, NanVersion* version);
wifi_error wifi_nan_get_capabilities_mock(transaction_id id,
                                wifi_interface_handle iface);

}  // namespace android

#endif //__WIFI_HAL_MOCK_H__
