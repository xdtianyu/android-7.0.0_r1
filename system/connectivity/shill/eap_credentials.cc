//
// Copyright (C) 2013 The Android Open Source Project
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

#include "shill/eap_credentials.h"

#include <map>
#include <string>
#include <utility>
#include <vector>

#if defined(__ANDROID__)
#include <dbus/service_constants.h>
#else
#include <chromeos/dbus/service_constants.h>
#endif  // __ANDROID__

#include "shill/certificate_file.h"
#include "shill/key_value_store.h"
#include "shill/logging.h"
#include "shill/metrics.h"
#include "shill/property_accessor.h"
#include "shill/property_store.h"
#include "shill/service.h"
#include "shill/store_interface.h"
#include "shill/supplicant/wpa_supplicant.h"

using base::FilePath;
using std::map;
using std::string;
using std::vector;

using std::string;

namespace shill {

namespace Logging {
static auto kModuleLogScope = ScopeLogger::kService;
static string ObjectID(const EapCredentials* e) { return "(eap_credentials)"; }
}

const char EapCredentials::kStorageEapAnonymousIdentity[] =
    "EAP.AnonymousIdentity";
const char EapCredentials::kStorageEapCACert[] = "EAP.CACert";
const char EapCredentials::kStorageEapCACertID[] = "EAP.CACertID";
const char EapCredentials::kStorageEapCACertNSS[] = "EAP.CACertNSS";
const char EapCredentials::kStorageEapCACertPEM[] = "EAP.CACertPEM";
const char EapCredentials::kStorageEapCertID[] = "EAP.CertID";
const char EapCredentials::kStorageEapClientCert[] = "EAP.ClientCert";
const char EapCredentials::kStorageEapEap[] = "EAP.EAP";
const char EapCredentials::kStorageEapIdentity[] = "EAP.Identity";
const char EapCredentials::kStorageEapInnerEap[] = "EAP.InnerEAP";
const char EapCredentials::kStorageEapKeyID[] = "EAP.KeyID";
const char EapCredentials::kStorageEapKeyManagement[] = "EAP.KeyMgmt";
const char EapCredentials::kStorageEapPIN[] = "EAP.PIN";
const char EapCredentials::kStorageEapPassword[] = "EAP.Password";
const char EapCredentials::kStorageEapPrivateKey[] = "EAP.PrivateKey";
const char EapCredentials::kStorageEapPrivateKeyPassword[] =
    "EAP.PrivateKeyPassword";
const char EapCredentials::kStorageEapSubjectMatch[] =
    "EAP.SubjectMatch";
const char EapCredentials::kStorageEapUseProactiveKeyCaching[] =
    "EAP.UseProactiveKeyCaching";
const char EapCredentials::kStorageEapUseSystemCAs[] = "EAP.UseSystemCAs";

EapCredentials::EapCredentials() : use_system_cas_(true),
                                   use_proactive_key_caching_(false) {}

EapCredentials::~EapCredentials() {}

// static
void EapCredentials::PopulateSupplicantProperties(
    CertificateFile* certificate_file, KeyValueStore* params) const {
  string ca_cert = ca_cert_;
  if (!ca_cert_pem_.empty()) {
    FilePath certfile =
        certificate_file->CreatePEMFromStrings(ca_cert_pem_);
    if (certfile.empty()) {
      LOG(ERROR) << "Unable to extract PEM certificate.";
    } else {
      ca_cert = certfile.value();
    }
  }


  typedef std::pair<const char*, const char*> KeyVal;
  KeyVal init_propertyvals[] = {
    // Authentication properties.
    KeyVal(WPASupplicant::kNetworkPropertyEapAnonymousIdentity,
           anonymous_identity_.c_str()),
    KeyVal(WPASupplicant::kNetworkPropertyEapClientCert,
           client_cert_.c_str()),
    KeyVal(WPASupplicant::kNetworkPropertyEapIdentity, identity_.c_str()),
    KeyVal(WPASupplicant::kNetworkPropertyEapCaPassword,
           password_.c_str()),
    KeyVal(WPASupplicant::kNetworkPropertyEapPrivateKey,
           private_key_.c_str()),
    KeyVal(WPASupplicant::kNetworkPropertyEapPrivateKeyPassword,
           private_key_password_.c_str()),

    // Non-authentication properties.
    KeyVal(WPASupplicant::kNetworkPropertyEapCaCert, ca_cert.c_str()),
    KeyVal(WPASupplicant::kNetworkPropertyEapCaCertId,
           ca_cert_id_.c_str()),
    KeyVal(WPASupplicant::kNetworkPropertyEapEap, eap_.c_str()),
    KeyVal(WPASupplicant::kNetworkPropertyEapInnerEap,
           inner_eap_.c_str()),
    KeyVal(WPASupplicant::kNetworkPropertyEapSubjectMatch,
           subject_match_.c_str())
  };

  vector<KeyVal> propertyvals(init_propertyvals,
                              init_propertyvals + arraysize(init_propertyvals));
  if (use_system_cas_) {
    propertyvals.push_back(KeyVal(
        WPASupplicant::kNetworkPropertyCaPath, WPASupplicant::kCaPath));
  } else if (ca_cert.empty()) {
    LOG(WARNING) << __func__
                 << ": No certificate authorities are configured."
                 << " Server certificates will be accepted"
                 << " unconditionally.";
  }

  if (ClientAuthenticationUsesCryptoToken()) {
    propertyvals.push_back(KeyVal(
        WPASupplicant::kNetworkPropertyEapCertId, cert_id_.c_str()));
    propertyvals.push_back(KeyVal(
        WPASupplicant::kNetworkPropertyEapKeyId, key_id_.c_str()));
  }

  if (ClientAuthenticationUsesCryptoToken() || !ca_cert_id_.empty()) {
    propertyvals.push_back(KeyVal(
        WPASupplicant::kNetworkPropertyEapPin, pin_.c_str()));
    propertyvals.push_back(KeyVal(
        WPASupplicant::kNetworkPropertyEngineId,
        WPASupplicant::kEnginePKCS11));
    // We can't use the propertyvals vector for this since this argument
    // is a uint32_t, not a string.
    params->SetUint(WPASupplicant::kNetworkPropertyEngine,
                   WPASupplicant::kDefaultEngine);
  }

  if (use_proactive_key_caching_) {
    params->SetUint(WPASupplicant::kNetworkPropertyEapProactiveKeyCaching,
                   WPASupplicant::kProactiveKeyCachingEnabled);
  } else {
    params->SetUint(WPASupplicant::kNetworkPropertyEapProactiveKeyCaching,
                   WPASupplicant::kProactiveKeyCachingDisabled);
  }

  for (const auto& keyval : propertyvals) {
    if (strlen(keyval.second) > 0) {
      params->SetString(keyval.first, keyval.second);
    }
  }
}

// static
void EapCredentials::PopulateWiMaxProperties(KeyValueStore* params) const {
  if (!anonymous_identity_.empty()) {
    params->SetString(wimax_manager::kEAPAnonymousIdentity,
                      anonymous_identity_);
  }
  if (!identity_.empty()) {
    params->SetString(wimax_manager::kEAPUserIdentity, identity_);
  }
  if (!password_.empty()) {
    params->SetString(wimax_manager::kEAPUserPassword, password_);
  }
}

void EapCredentials::InitPropertyStore(PropertyStore* store) {
  // Authentication properties.
  store->RegisterString(kEapAnonymousIdentityProperty, &anonymous_identity_);
  store->RegisterString(kEapCertIdProperty, &cert_id_);
  store->RegisterString(kEapClientCertProperty, &client_cert_);
  store->RegisterString(kEapIdentityProperty, &identity_);
  store->RegisterString(kEapKeyIdProperty, &key_id_);
  HelpRegisterDerivedString(store,
                            kEapKeyMgmtProperty,
                            &EapCredentials::GetKeyManagement,
                            &EapCredentials::SetKeyManagement);
  HelpRegisterWriteOnlyDerivedString(store,
                                     kEapPasswordProperty,
                                     &EapCredentials::SetEapPassword,
                                     nullptr,
                                     &password_);
  store->RegisterString(kEapPinProperty, &pin_);
  store->RegisterString(kEapPrivateKeyProperty, &private_key_);
  HelpRegisterWriteOnlyDerivedString(store,
                                     kEapPrivateKeyPasswordProperty,
                                     &EapCredentials::SetEapPrivateKeyPassword,
                                     nullptr,
                                     &private_key_password_);

  // Non-authentication properties.
  store->RegisterStrings(kEapCaCertPemProperty, &ca_cert_pem_);
  store->RegisterString(kEapCaCertIdProperty, &ca_cert_id_);
  store->RegisterString(kEapCaCertNssProperty, &ca_cert_nss_);
  store->RegisterString(kEapCaCertProperty, &ca_cert_);
  store->RegisterString(kEapMethodProperty, &eap_);
  store->RegisterString(kEapPhase2AuthProperty, &inner_eap_);
  store->RegisterString(kEapSubjectMatchProperty, &subject_match_);
  store->RegisterBool(kEapUseProactiveKeyCachingProperty,
                      &use_proactive_key_caching_);
  store->RegisterBool(kEapUseSystemCasProperty, &use_system_cas_);
}

// static
bool EapCredentials::IsEapAuthenticationProperty(const string property) {
  return
      property == kEapAnonymousIdentityProperty ||
      property == kEapCertIdProperty ||
      property == kEapClientCertProperty ||
      property == kEapIdentityProperty ||
      property == kEapKeyIdProperty ||
      property == kEapKeyMgmtProperty ||
      property == kEapPasswordProperty ||
      property == kEapPinProperty ||
      property == kEapPrivateKeyProperty ||
      property == kEapPrivateKeyPasswordProperty;
}

bool EapCredentials::IsConnectable() const {
  // Identity is required.
  if (identity_.empty()) {
    SLOG(this, 2) << "Not connectable: Identity is empty.";
    return false;
  }

  if (!client_cert_.empty() || !cert_id_.empty()) {
    // If a client certificate is being used, we must have a private key.
    if (private_key_.empty() && key_id_.empty()) {
      SLOG(this, 2)
          << "Not connectable: Client certificate but no private key.";
      return false;
    }
  }
  if (!cert_id_.empty() || !key_id_.empty() ||
      !ca_cert_id_.empty()) {
    // If PKCS#11 data is needed, a PIN is required.
    if (pin_.empty()) {
      SLOG(this, 2) << "Not connectable: PKCS#11 data but no PIN.";
      return false;
    }
  }

  // For EAP-TLS, a client certificate is required.
  if (eap_.empty() || eap_ == kEapMethodTLS) {
    if ((!client_cert_.empty() || !cert_id_.empty()) &&
        (!private_key_.empty() || !key_id_.empty())) {
      SLOG(this, 2) << "Connectable: EAP-TLS with a client cert and key.";
      return true;
    }
  }

  // For EAP types other than TLS (e.g. EAP-TTLS or EAP-PEAP, password is the
  // minimum requirement), at least an identity + password is required.
  if (eap_.empty() || eap_ != kEapMethodTLS) {
    if (!password_.empty()) {
      SLOG(this, 2) << "Connectable. !EAP-TLS and has a password.";
      return true;
    }
  }

  SLOG(this, 2) << "Not connectable: No suitable EAP configuration was found.";
  return false;
}

bool EapCredentials::IsConnectableUsingPassphrase() const {
  return !identity_.empty() && !password_.empty();
}

void EapCredentials::Load(StoreInterface* storage, const string& id) {
  // Authentication properties.
  storage->GetCryptedString(id,
                            kStorageEapAnonymousIdentity,
                            &anonymous_identity_);
  storage->GetString(id, kStorageEapCertID, &cert_id_);
  storage->GetString(id, kStorageEapClientCert, &client_cert_);
  storage->GetCryptedString(id, kStorageEapIdentity, &identity_);
  storage->GetString(id, kStorageEapKeyID, &key_id_);
  string key_management;
  storage->GetString(id, kStorageEapKeyManagement, &key_management);
  SetKeyManagement(key_management, nullptr);
  storage->GetCryptedString(id, kStorageEapPassword, &password_);
  storage->GetString(id, kStorageEapPIN, &pin_);
  storage->GetString(id, kStorageEapPrivateKey, &private_key_);
  storage->GetCryptedString(id,
                            kStorageEapPrivateKeyPassword,
                            &private_key_password_);

  // Non-authentication properties.
  storage->GetString(id, kStorageEapCACert, &ca_cert_);
  storage->GetString(id, kStorageEapCACertID, &ca_cert_id_);
  storage->GetString(id, kStorageEapCACertNSS, &ca_cert_nss_);
  storage->GetStringList(id, kStorageEapCACertPEM, &ca_cert_pem_);
  storage->GetString(id, kStorageEapEap, &eap_);
  storage->GetString(id, kStorageEapInnerEap, &inner_eap_);
  storage->GetString(id, kStorageEapSubjectMatch, &subject_match_);
  storage->GetBool(id, kStorageEapUseProactiveKeyCaching,
                   &use_proactive_key_caching_);
  storage->GetBool(id, kStorageEapUseSystemCAs, &use_system_cas_);
}

void EapCredentials::OutputConnectionMetrics(
    Metrics* metrics, Technology::Identifier technology) const {
  Metrics::EapOuterProtocol outer_protocol =
      Metrics::EapOuterProtocolStringToEnum(eap_);
  metrics->SendEnumToUMA(
      metrics->GetFullMetricName(Metrics::kMetricNetworkEapOuterProtocolSuffix,
                                 technology),
      outer_protocol,
      Metrics::kMetricNetworkEapOuterProtocolMax);

  Metrics::EapInnerProtocol inner_protocol =
      Metrics::EapInnerProtocolStringToEnum(inner_eap_);
  metrics->SendEnumToUMA(
      metrics->GetFullMetricName(Metrics::kMetricNetworkEapInnerProtocolSuffix,
                                 technology),
      inner_protocol,
      Metrics::kMetricNetworkEapInnerProtocolMax);
}

void EapCredentials::Save(StoreInterface* storage, const string& id,
                          bool save_credentials) const {
  // Authentication properties.
  Service::SaveString(storage,
                      id,
                      kStorageEapAnonymousIdentity,
                      anonymous_identity_,
                      true,
                      save_credentials);
  Service::SaveString(storage,
                      id,
                      kStorageEapCertID,
                      cert_id_,
                      false,
                      save_credentials);
  Service::SaveString(storage,
                      id,
                      kStorageEapClientCert,
                      client_cert_,
                      false,
                      save_credentials);
  Service::SaveString(storage,
                      id,
                      kStorageEapIdentity,
                      identity_,
                      true,
                      save_credentials);
  Service::SaveString(storage,
                      id,
                      kStorageEapKeyID,
                      key_id_,
                      false,
                      save_credentials);
  Service::SaveString(storage,
                      id,
                      kStorageEapKeyManagement,
                      key_management_,
                      false,
                      true);
  Service::SaveString(storage,
                      id,
                      kStorageEapPassword,
                      password_,
                      true,
                      save_credentials);
  Service::SaveString(storage,
                      id,
                      kStorageEapPIN,
                      pin_,
                      false,
                      save_credentials);
  Service::SaveString(storage,
                      id,
                      kStorageEapPrivateKey,
                      private_key_,
                      false,
                      save_credentials);
  Service::SaveString(storage,
                      id,
                      kStorageEapPrivateKeyPassword,
                      private_key_password_,
                      true,
                      save_credentials);

  // Non-authentication properties.
  Service::SaveString(storage, id, kStorageEapCACert, ca_cert_, false, true);
  Service::SaveString(storage,
                      id,
                      kStorageEapCACertID,
                      ca_cert_id_,
                      false,
                      true);
  Service::SaveString(storage,
                      id,
                      kStorageEapCACertNSS,
                      ca_cert_nss_,
                      false,
                      true);
  if (ca_cert_pem_.empty()) {
      storage->DeleteKey(id, kStorageEapCACertPEM);
  } else {
      storage->SetStringList(id, kStorageEapCACertPEM, ca_cert_pem_);
  }
  Service::SaveString(storage, id, kStorageEapEap, eap_, false, true);
  Service::SaveString(storage,
                      id,
                      kStorageEapInnerEap,
                      inner_eap_,
                      false,
                      true);
  Service::SaveString(storage,
                      id,
                      kStorageEapSubjectMatch,
                      subject_match_,
                      false,
                      true);
  storage->SetBool(id, kStorageEapUseProactiveKeyCaching,
                   use_proactive_key_caching_);
  storage->SetBool(id, kStorageEapUseSystemCAs, use_system_cas_);
}

void EapCredentials::Reset() {
  // Authentication properties.
  anonymous_identity_ = "";
  cert_id_ = "";
  client_cert_ = "";
  identity_ = "";
  key_id_ = "";
  // Do not reset key_management_, since it should never be emptied.
  password_ = "";
  pin_ = "";
  private_key_ = "";
  private_key_password_ = "";

  // Non-authentication properties.
  ca_cert_ = "";
  ca_cert_id_ = "";
  ca_cert_nss_ = "";
  ca_cert_pem_.clear();
  eap_ = "";
  inner_eap_ = "";
  subject_match_ = "";
  use_system_cas_ = true;
  use_proactive_key_caching_ = false;
}

bool EapCredentials::SetEapPassword(const string& password, Error* /*error*/) {
  if (password_ == password) {
    return false;
  }
  password_ = password;
  return true;
}

bool EapCredentials::SetEapPrivateKeyPassword(const string& password,
                                              Error* /*error*/) {
  if (private_key_password_ == password) {
    return false;
  }
  private_key_password_ = password;
  return true;
}

string EapCredentials::GetKeyManagement(Error* /*error*/) {
  return key_management_;
}

bool EapCredentials::SetKeyManagement(const std::string& key_management,
                                      Error* /*error*/) {
  if (key_management.empty()) {
    return false;
  }
  if (key_management_ == key_management) {
    return false;
  }
  key_management_ = key_management;
  return true;
}

bool EapCredentials::ClientAuthenticationUsesCryptoToken() const {
  return (eap_.empty() || eap_ == kEapMethodTLS ||
          inner_eap_ == kEapMethodTLS) &&
         (!cert_id_.empty() || !key_id_.empty());
}

void EapCredentials::HelpRegisterDerivedString(
    PropertyStore* store,
    const string& name,
    string(EapCredentials::*get)(Error* error),
    bool(EapCredentials::*set)(const string&, Error*)) {
  store->RegisterDerivedString(
      name,
      StringAccessor(new CustomAccessor<EapCredentials, string>(
          this, get, set)));
}

void EapCredentials::HelpRegisterWriteOnlyDerivedString(
    PropertyStore* store,
    const string& name,
    bool(EapCredentials::*set)(const string&, Error*),
    void(EapCredentials::*clear)(Error* error),
    const string* default_value) {
  store->RegisterDerivedString(
      name,
      StringAccessor(
          new CustomWriteOnlyAccessor<EapCredentials, string>(
              this, set, clear, default_value)));
}

}  // namespace shill
