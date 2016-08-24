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

#include "proxy_rpc_in_data_types.h"
#include "proxy_util.h"

namespace {
// Autotest Server test encodes the object type in this key.
static const char kXmlRpcStructTypeKey[] = "xmlrpc_struct_type_key";

void AssertStructTypeStringFromXmlRpcValue(
    XmlRpc::XmlRpcValue* xml_rpc_value_in,
    std::string expected_type) {
  if ((*xml_rpc_value_in)[kXmlRpcStructTypeKey] != expected_type) {
    LOG(FATAL) << "Unexpected object received. Expected: " << expected_type
               << "Recieved: " << (*xml_rpc_value_in)[kXmlRpcStructTypeKey];
  }
}

ProxyShillWifiClient::StationType ParseStationTypeFromXmlRpcValue(
    XmlRpc::XmlRpcValue* xml_rpc_value_in) {
  ProxyShillWifiClient::StationType station_type;
  std::string station_type_as_string;
  GetStringValueFromXmlRpcValueStructMember(
      xml_rpc_value_in, "station_type", "managed", &station_type_as_string);
  if (station_type_as_string == "managed") {
    station_type = ProxyShillWifiClient::kStationTypeManaged;
  } else if (station_type_as_string == "ibss") {
    station_type = ProxyShillWifiClient::kStationTypeIBSS;
  } else {
    station_type = ProxyShillWifiClient::kStationTypeUnknown;
  }
  return station_type;
}

ProxyShillWifiClient::AutoConnectType ParseAutoConnectTypeFromXmlRpcValue(
    XmlRpc::XmlRpcValue* xml_rpc_value_in) {
  ProxyShillWifiClient::AutoConnectType autoconnect_type;
  bool autoconnect_type_as_bool;
  if (GetBoolValueFromXmlRpcValueStructMember(
          xml_rpc_value_in, "autoconnect", false, &autoconnect_type_as_bool)) {
    if (autoconnect_type_as_bool) {
      autoconnect_type = ProxyShillWifiClient::kAutoConnectTypeEnabled;
    } else {
      autoconnect_type = ProxyShillWifiClient::kAutoConnectTypeDisabled;
    }
  } else {
    autoconnect_type = ProxyShillWifiClient::kAutoConnectTypeUnspecified;
  }
  return autoconnect_type;
}

} // namespace

const  int BgscanConfiguration::kDefaultShortIntervalSeconds = 30;
const int BgscanConfiguration::kDefaultLongIntervalSeconds = 180;
const int BgscanConfiguration::kDefaultSignalThreshold = -50;
const char BgscanConfiguration::kDefaultScanMethod[] = "default";
const int AssociationParameters::kDefaultDiscoveryTimeoutSeconds = 15;
const int AssociationParameters::kDefaultAssociationTimeoutSeconds = 15;
const int AssociationParameters::kDefaultConfigurationTimeoutSeconds = 15;

BgscanConfiguration::BgscanConfiguration(
    XmlRpc::XmlRpcValue* xml_rpc_value_in) {
  AssertStructTypeStringFromXmlRpcValue(
      xml_rpc_value_in, "BgscanConfiguration");
  GetStringValueFromXmlRpcValueStructMember(
      xml_rpc_value_in, "interface", std::string(), &interface_);
  GetIntValueFromXmlRpcValueStructMember(
      xml_rpc_value_in, "signal", kDefaultSignalThreshold, &signal_threshold_);
  GetIntValueFromXmlRpcValueStructMember(
      xml_rpc_value_in, "short_interval", kDefaultShortIntervalSeconds,
      &short_interval_);
  GetIntValueFromXmlRpcValueStructMember(
      xml_rpc_value_in, "short_interval", kDefaultLongIntervalSeconds,
      &long_interval_);
  GetStringValueFromXmlRpcValueStructMember(
      xml_rpc_value_in, "method", kDefaultScanMethod, &method_);
}

AssociationParameters::AssociationParameters(
    XmlRpc::XmlRpcValue* xml_rpc_value_in) {
  AssertStructTypeStringFromXmlRpcValue(
      xml_rpc_value_in, "AssociationParameters");
  GetStringValueFromXmlRpcValueStructMember(
      xml_rpc_value_in, "ssid", std::string(), &ssid_);
  GetIntValueFromXmlRpcValueStructMember(
      xml_rpc_value_in, "discovery_timeout",
      kDefaultDiscoveryTimeoutSeconds, &discovery_timeout_seconds_);
  GetIntValueFromXmlRpcValueStructMember(
      xml_rpc_value_in, "association_timeout",
      kDefaultAssociationTimeoutSeconds, &association_timeout_seconds_);
  GetIntValueFromXmlRpcValueStructMember(
      xml_rpc_value_in, "configuration_timeout",
      kDefaultConfigurationTimeoutSeconds, &configuration_timeout_seconds_);
  GetBoolValueFromXmlRpcValueStructMember(
      xml_rpc_value_in, "is_hidden", false, &is_hidden_);
  GetBoolValueFromXmlRpcValueStructMember(
      xml_rpc_value_in, "save_credentials", false, &save_credentials_);
  station_type_ = ParseStationTypeFromXmlRpcValue(xml_rpc_value_in);
  GetStringValueFromXmlRpcValueStructMember(
      xml_rpc_value_in, "guid", std::string(), &guid_);
  GetBoolValueFromXmlRpcValueStructMember(
      xml_rpc_value_in, "expect_failure", false, &expect_failure_);
  autoconnect_type_ = ParseAutoConnectTypeFromXmlRpcValue(xml_rpc_value_in);
  bgscan_config_ = std::unique_ptr<BgscanConfiguration>(
      new BgscanConfiguration(&(*xml_rpc_value_in)["bgscan_config"]));
  security_config_ = SecurityConfig::CreateSecurityConfigObject(
      &(*xml_rpc_value_in)["security_config"]);
}

ConfigureServiceParameters::ConfigureServiceParameters(
    XmlRpc::XmlRpcValue* xml_rpc_value_in) {
  AssertStructTypeStringFromXmlRpcValue(
      xml_rpc_value_in, "ConfigureServiceParameters");
  GetStringValueFromXmlRpcValueStructMember(
      xml_rpc_value_in, "guid", std::string(), &guid_);
  GetStringValueFromXmlRpcValueStructMember(
      xml_rpc_value_in, "passphrase", std::string(), &passphrase_);
  autoconnect_type_ = ParseAutoConnectTypeFromXmlRpcValue(xml_rpc_value_in);
}
