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

#ifndef PROXY_RPC_SECURITY_TYPES_H
#define PROXY_RPC_SECURITY_TYPES_H

#include <string>

#include <XmlRpcValue.h>

#include <brillo/variant_dictionary.h>

// Abstracts the security configuration for a WiFi network.
// This bundle of credentials can be passed to both HostapConfig and
// AssociationParameters so that both shill and hostapd can set up and connect
// to an encrypted WiFi network. By default, we'll assume we're connecting
// to an open network.
class SecurityConfig {
 public:
  enum WpaModeType {
    kWpaModePure = 1,
    kWpaModePure_2 = 2,
    kWpaModeMixed = kWpaModePure | kWpaModePure_2,
    kWpaModeDefault = kWpaModeMixed,
  };
  enum AuthAlgorithmType {
    kAuthAlgorithmTypeOpen = 1,
    kAuthAlgorithmTypeShared = 2,
    kAuthAlgorithmTypeDefault = kAuthAlgorithmTypeOpen
  };
  static const char kDefaultSecurity[];

  // This function creates the appropriate |SecurityConfig| subclass
  // object from the incoming RPC data.
  static std::unique_ptr<SecurityConfig> CreateSecurityConfigObject(
      XmlRpc::XmlRpcValue* xml_rpc_value_in);
  SecurityConfig(XmlRpc::XmlRpcValue* xml_rpc_value_in);
  virtual ~SecurityConfig() = default;
  virtual void GetServiceProperties(brillo::VariantDictionary* properties);

  std::string security_;
};

// Abstracts security configuration for a WiFi network using static WEP.
// Open system authentication means that we don"t do a 4 way AUTH handshake,
// and simply start using the WEP keys after association finishes.
class WEPConfig : public SecurityConfig {
 public:
  WEPConfig(XmlRpc::XmlRpcValue* xml_rpc_value_in);
  virtual void GetServiceProperties(brillo::VariantDictionary* properties) override;

 private:
  std::vector<std::string> wep_keys_;
  int wep_default_key_index_;
  int auth_algorithm_;
};

// Abstracts security configuration for a WPA encrypted WiFi network.
class WPAConfig : public SecurityConfig {
 public:
  WPAConfig(XmlRpc::XmlRpcValue* xml_rpc_value_in);
  void GetServiceProperties(brillo::VariantDictionary* properties) override;

  static const int kMaxPskSize;

 private:
  std::string psk_;
  int wpa_mode_;
  std::vector<std::string> wpa_ciphers_;
  std::vector<std::string> wpa2_ciphers_;
  int wpa_ptk_rekey_period_seconds_;
  int wpa_gtk_rekey_period_seconds_;
  int wpa_gmk_rekey_period_seconds_;
  bool use_strict_rekey_;
};

// Abstract superclass that implements certificate/key installation.
class EAPConfig : public SecurityConfig {
 public:
  static const char kDefaultEapUsers[];
  static const char kDefaultEAPIdentity[];
  static int last_tmp_id;

  EAPConfig(XmlRpc::XmlRpcValue* xml_rpc_value_in);
  void GetServiceProperties(brillo::VariantDictionary* properties) override;

 private:
  bool use_system_cas_;
  std::string server_ca_cert_;
  std::string server_cert_;
  std::string server_key_;
  std::string server_eap_users;
  std::string client_ca_cert_;
  std::string client_cert_;
  std::string client_key_;
  std::string server_ca_cert_file_path_;
  std::string server_cert_file_path_;
  std::string server_key_file_path_;
  std::string server_eap_user_file_path_;
  std::string file_path_suffix_;
  std::string client_cert_id_;
  std::string client_key_id_;
  std::string pin_;
  std::string client_cert_slot_id_;
  std::string client_key_slot_id_;
  std::string eap_identity_;
};

// Configuration settings bundle for dynamic WEP.
// This is a WEP encrypted connection where the keys are negotiated after the
// client authenticates via 802.1x.
class DynamicWEPConfig : public EAPConfig {
 public:
  static const int kDefaultKeyPeriod;

  DynamicWEPConfig(XmlRpc::XmlRpcValue* xml_rpc_value_in);
  void GetServiceProperties(brillo::VariantDictionary* properties) override;

 private:
  bool use_short_keys_;
  int wep_rekey_period_seconds_;
};

// Security type to set up a WPA connection via EAP-TLS negotiation.
class WPAEAPConfig : public EAPConfig {
 public:
  WPAEAPConfig(XmlRpc::XmlRpcValue* xml_rpc_value_in);
  void GetServiceProperties(brillo::VariantDictionary* properties) override;

 private:
  bool use_short_keys_;
  WpaModeType wpa_mode_;
};

// Security type to set up a TTLS/PEAP connection.
// Both PEAP and TTLS are tunneled protocols which use EAP inside of a TLS
// secured tunnel.  The secured tunnel is a symmetric key encryption scheme
// negotiated under the protection of a public key in the server certificate.
// Thus, we"ll see server credentials in the form of certificates, but client
// credentials in the form of passwords and a CA Cert to root the trust chain.
class Tunneled1xConfig : public WPAEAPConfig {
 public:
  static const char kTTLSPrefix[];
  static const char kLayer1TypePEAP[];
  static const char kLayer1TypeTTLS[];
  static const char kLayer2TypeGTC[];
  static const char kLayer2TypeMSCHAPV2[];
  static const char kLayer2TypeMD5[];
  static const char kLayer2TypeTTLSMSCHAPV2[];
  static const char kLayer2TypeTTLSMSCHAP[];
  static const char kLayer2TypeTTLSPAP[];

  Tunneled1xConfig(XmlRpc::XmlRpcValue* xml_rpc_value_in);
  void GetServiceProperties(brillo::VariantDictionary* properties) override;

 private:
  std::string password_;
  std::string inner_protocol_;
};

#endif // PROXY_RPC_SECURITY_TYPES_H
