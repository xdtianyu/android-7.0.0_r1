//
// Copyright (C) 2015 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#ifndef PROXY_RPC_IN_DATA_TYPES_H
#define PROXY_RPC_IN_DATA_TYPES_H

#include <string>

#include <XmlRpcValue.h>

#include "proxy_shill_wifi_client.h"
#include "proxy_rpc_security_types.h"

// Describes how to configure wpa_supplicant on a DUT.
class BgscanConfiguration {
 public:
  static const int kDefaultShortIntervalSeconds;
  static const int kDefaultLongIntervalSeconds;
  static const int kDefaultSignalThreshold;
  static const char kDefaultScanMethod[];

  BgscanConfiguration(XmlRpc::XmlRpcValue* xml_rpc_value_in);

  std::string interface_;
  int signal_threshold_;
  int short_interval_;
  int long_interval_;
  std::string method_;
};

// Describes parameters used in WiFi connection attempts.
class AssociationParameters {
 public:
  static const int kDefaultDiscoveryTimeoutSeconds;
  static const int kDefaultAssociationTimeoutSeconds;
  static const int kDefaultConfigurationTimeoutSeconds;

  AssociationParameters(XmlRpc::XmlRpcValue* xml_rpc_value_in);

  std::string ssid_;
  int discovery_timeout_seconds_;
  int association_timeout_seconds_;
  int configuration_timeout_seconds_;
  bool is_hidden_;
  bool save_credentials_;
  ProxyShillWifiClient::StationType station_type_;
  std::string guid_;
  bool expect_failure_;
  ProxyShillWifiClient::AutoConnectType autoconnect_type_;
  std::unique_ptr<BgscanConfiguration> bgscan_config_;
  std::unique_ptr<SecurityConfig> security_config_;
};

// Describes a group of optional settings for use with ConfigureService.
// The Manager in shill has a method ConfigureService which takes a dictionary
// of parameters, and uses some of them to look up a service, and sets the
// remainder of the properties on the service.  This struct represents
// some of the optional parameters that can be set in this way.  Current
// consumers of this interface look up the service by GUID.
class ConfigureServiceParameters {
 public:
  ConfigureServiceParameters(XmlRpc::XmlRpcValue* xml_rpc_value_in);

  std::string guid_;
  std::string passphrase_;
  ProxyShillWifiClient::AutoConnectType autoconnect_type_;
};

#endif // PROXY_RPC_IN_DATA_TYPES_H
