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

#include <base/logging.h>
#include <base/strings/stringprintf.h>
#include <service_constants.h>

#include "proxy_rpc_security_types.h"
#include "proxy_util.h"

// Autotest Server test encodes the object type in this key.
static const char kXmlRpcStructTypeKey[] = "xmlrpc_struct_type_key";
const char SecurityConfig::kDefaultSecurity[] = "none";
const int WPAConfig::kMaxPskSize = 64;
const char EAPConfig::kDefaultEapUsers[] = "* TLS";
const char EAPConfig::kDefaultEAPIdentity[] = "brillo";
int EAPConfig::last_tmp_id = 8800;
const int DynamicWEPConfig::kDefaultKeyPeriod = 20;
const char Tunneled1xConfig::kTTLSPrefix[] = "TTLS-";
const char Tunneled1xConfig::kLayer1TypePEAP[] = "PEAP";
const char Tunneled1xConfig::kLayer1TypeTTLS[] = "TTLS";
const char Tunneled1xConfig::kLayer2TypeGTC[] = "GTC";
const char Tunneled1xConfig::kLayer2TypeMSCHAPV2[] = "MSCHAPV2";
const char Tunneled1xConfig::kLayer2TypeMD5[] = "MD5";
const char Tunneled1xConfig::kLayer2TypeTTLSMSCHAPV2[] = "TTLS-MSCHAPV2";
const char Tunneled1xConfig::kLayer2TypeTTLSMSCHAP[] = "TTLS-MSCHAP";
const char Tunneled1xConfig::kLayer2TypeTTLSPAP[] = "TTLS-PAP";

std::unique_ptr<SecurityConfig> SecurityConfig::CreateSecurityConfigObject(
    XmlRpc::XmlRpcValue* xml_rpc_value_in) {
  const std::string& security_type = (*xml_rpc_value_in)[kXmlRpcStructTypeKey];
  if (security_type == "SecurityConfig") {
    return std::unique_ptr<SecurityConfig>(new SecurityConfig(xml_rpc_value_in));
  }
  if (security_type == "WEPConfig") {
    return std::unique_ptr<SecurityConfig>(new WEPConfig(xml_rpc_value_in));
  }
  if (security_type == "WPAConfig") {
    return std::unique_ptr<SecurityConfig>(new WPAConfig(xml_rpc_value_in));
  }
  LOG(FATAL) << "Unexpected object received. Received: " << security_type;
  return nullptr;
}

SecurityConfig::SecurityConfig(
    XmlRpc::XmlRpcValue* xml_rpc_value_in) {
  GetStringValueFromXmlRpcValueStructMember(
      xml_rpc_value_in, "security", kDefaultSecurity, &security_);
}

void SecurityConfig::GetServiceProperties(brillo::VariantDictionary* properties) {
  // The base class represents a connection with no security. So, no security
  // properties to be sent to Shill.
}

WEPConfig::WEPConfig(XmlRpc::XmlRpcValue* xml_rpc_value_in)
  : SecurityConfig::SecurityConfig(xml_rpc_value_in) {
  GetStringVectorFromXmlRpcValueStructMember(
      xml_rpc_value_in, "wep_keys", std::vector<std::string>(), &wep_keys_);
  GetIntValueFromXmlRpcValueStructMember(
        xml_rpc_value_in, "wep_default_key", 0, &wep_default_key_index_);
  GetIntValueFromXmlRpcValueStructMember(
        xml_rpc_value_in, "auth_algorithm", (int)kAuthAlgorithmTypeDefault,
        &auth_algorithm_);
  if (wep_default_key_index_ > static_cast<int>(wep_keys_.size())) {
    LOG(FATAL) << "Error in received wep_default_key: "
               << wep_default_key_index_;
  }
}

void WEPConfig::GetServiceProperties(brillo::VariantDictionary* properties) {
  std::string passphrase = base::StringPrintf(
      "%d:%s", wep_default_key_index_,
      wep_keys_[wep_default_key_index_].c_str());
  (*properties)[shill::kPassphraseProperty] = passphrase;
}

WPAConfig::WPAConfig(XmlRpc::XmlRpcValue* xml_rpc_value_in)
  : SecurityConfig::SecurityConfig(xml_rpc_value_in) {
  GetStringValueFromXmlRpcValueStructMember(
      xml_rpc_value_in, "psk", std::string(), &psk_);
  GetIntValueFromXmlRpcValueStructMember(
        xml_rpc_value_in, "wpa_mode", kWpaModeDefault, &wpa_mode_);
  GetStringVectorFromXmlRpcValueStructMember(
      xml_rpc_value_in, "wpa_ciphers", std::vector<std::string>(),
      &wpa_ciphers_);
  GetStringVectorFromXmlRpcValueStructMember(
      xml_rpc_value_in, "wpa2_ciphers", std::vector<std::string>(),
      &wpa2_ciphers_);
  GetIntValueFromXmlRpcValueStructMember(
      xml_rpc_value_in, "wpa_ptk_rekey_period", 0,
      &wpa_ptk_rekey_period_seconds_);
  GetIntValueFromXmlRpcValueStructMember(
      xml_rpc_value_in, "wpa_gtk_rekey_period", 0,
      &wpa_gtk_rekey_period_seconds_);
  GetIntValueFromXmlRpcValueStructMember(
      xml_rpc_value_in, "wpa_gmk_rekey_period", 0,
      &wpa_gmk_rekey_period_seconds_);
  GetBoolValueFromXmlRpcValueStructMember(
      xml_rpc_value_in, "use_strict_rekey", 0, &use_strict_rekey_);

  if (psk_.size() > kMaxPskSize) {
    LOG(FATAL) << "WPA passphrases can be no longer than 63 characters"
                  "(or 64 hex digits). PSK: " << psk_;
  }
  if ((psk_.size() == kMaxPskSize) &&
      (psk_.find_first_not_of("0123456789abcdef") != std::string::npos)) {
    LOG(FATAL) << "Invalid PSK: " << psk_;
  }
}

void WPAConfig::GetServiceProperties(brillo::VariantDictionary* properties) {
  (*properties)[shill::kPassphraseProperty] = psk_;
}
