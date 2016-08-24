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

#include <base/stl_util.h>
#if defined(__ANDROID__)
#include <dbus/service_constants.h>
#else
#include <chromeos/dbus/service_constants.h>
#endif  // __ANDROID__
#include <gtest/gtest.h>

#include "shill/key_value_store.h"
#include "shill/mock_certificate_file.h"
#include "shill/mock_event_dispatcher.h"
#include "shill/mock_log.h"
#include "shill/mock_metrics.h"
#include "shill/mock_property_store.h"
#include "shill/mock_store.h"
#include "shill/supplicant/wpa_supplicant.h"
#include "shill/technology.h"

using base::FilePath;
using std::map;
using std::string;
using std::vector;
using testing::_;
using testing::AnyNumber;
using testing::DoAll;
using testing::HasSubstr;
using testing::Mock;
using testing::Return;
using testing::SetArgumentPointee;

namespace shill {

class EapCredentialsTest : public testing::Test {
 public:
  EapCredentialsTest() {}
  virtual ~EapCredentialsTest() {}

 protected:
  void PopulateSupplicantProperties() {
    eap_.PopulateSupplicantProperties(&certificate_file_, &params_);
  }

  void SetAnonymousIdentity(const string& anonymous_identity) {
    eap_.anonymous_identity_ = anonymous_identity;
  }
  void SetCACertNSS(const string& ca_cert_nss) {
    eap_.ca_cert_nss_ = ca_cert_nss;
  }
  void SetCACertPEM(const vector<string>& ca_cert_pem) {
    eap_.ca_cert_pem_ = ca_cert_pem;
  }
  void SetClientCert(const string& client_cert) {
    eap_.client_cert_ = client_cert;
  }
  void SetCertId(const string& cert_id) {
    eap_.cert_id_ = cert_id;
  }
  void SetCACertId(const string& ca_cert_id) {
    eap_.ca_cert_id_ = ca_cert_id;
  }
  void SetEap(const string& eap) {
    eap_.eap_ = eap;
  }
  void SetIdentity(const string& identity) {
    eap_.identity_ = identity;
  }
  void SetInnerEap(const string& inner_eap) {
    eap_.inner_eap_ = inner_eap;
  }
  void SetKeyId(const string& key_id) {
    eap_.key_id_ = key_id;
  }
  const string& GetPassword() {
    return eap_.password_;
  }
  void SetPassword(const string& password) {
    eap_.password_ = password;
  }
  void SetPrivateKey(const string& private_key) {
    eap_.private_key_ = private_key;
  }
  void SetPin(const string& pin) {
    eap_.pin_ = pin;
  }
  void SetUseProactiveKeyCaching(bool use_proactive_key_caching) {
    eap_.use_proactive_key_caching_ = use_proactive_key_caching;
  }
  void SetUseSystemCAs(bool use_system_cas) {
    eap_.use_system_cas_ = use_system_cas;
  }
  bool IsReset() {
    return
        eap_.anonymous_identity_.empty() &&
        eap_.cert_id_.empty() &&
        eap_.client_cert_.empty() &&
        eap_.identity_.empty() &&
        eap_.key_id_.empty() &&
        eap_.password_.empty() &&
        eap_.pin_.empty() &&
        eap_.private_key_.empty() &&
        eap_.private_key_password_.empty() &&
        eap_.ca_cert_.empty() &&
        eap_.ca_cert_id_.empty() &&
        eap_.ca_cert_nss_.empty() &&
        eap_.ca_cert_pem_.empty() &&
        eap_.eap_.empty() &&
        eap_.inner_eap_.empty() &&
        eap_.subject_match_.empty() &&
        eap_.use_system_cas_ == true &&
        eap_.use_proactive_key_caching_ == false;
  }

  const string& GetKeyManagement() {
    return eap_.key_management_;
  }
  bool SetEapPassword(const string& password, Error* error) {
    return eap_.SetEapPassword(password, error);
  }
  bool SetEapPrivateKeyPassword(const string& password, Error* error) {
    return eap_.SetEapPrivateKeyPassword(password, error);
  }

  EapCredentials eap_;
  MockCertificateFile certificate_file_;
  KeyValueStore params_;
};

TEST_F(EapCredentialsTest, PropertyStore) {
  PropertyStore store;
  eap_.InitPropertyStore(&store);
  const string kIdentity("Cross-Eyed Mary");
  Error error;
  EXPECT_TRUE(store.SetStringProperty(kEapIdentityProperty, kIdentity, &error));
  EXPECT_EQ(kIdentity, eap_.identity());
}

TEST_F(EapCredentialsTest, Connectable) {
  // Empty EAP credentials should not make a 802.1x network connectable.
  EXPECT_FALSE(eap_.IsConnectable());

  // Identity alone is not enough.
  SetIdentity("Steel Monkey");
  EXPECT_FALSE(eap_.IsConnectable());

  // Set a password.
  SetPassword("Angry Tapir");

  // Empty "EAP" parameter is treated like "not EAP-TLS", and connectable.
  EXPECT_TRUE(eap_.IsConnectable());

  // Some other non-TLS EAP type.
  SetEap("DodgeBall");
  EXPECT_TRUE(eap_.IsConnectable());

  // EAP-TLS requires certificate parameters, and cares not for passwords.
  SetEap("TLS");
  EXPECT_FALSE(eap_.IsConnectable());

  // Clearing the password won't help.
  SetPassword("");
  EXPECT_FALSE(eap_.IsConnectable());

  // A client cert by itself doesn't help.
  SetClientCert("client-cert");
  EXPECT_FALSE(eap_.IsConnectable());

  // A client cert and key will, however.
  SetPrivateKey("client-cert");
  EXPECT_TRUE(eap_.IsConnectable());

  // A key-id (and cert) doesn't work.
  SetKeyId("client-key-id");
  EXPECT_FALSE(eap_.IsConnectable());

  // We need a PIN for the key id in addition.
  SetPin("pin");
  EXPECT_TRUE(eap_.IsConnectable());

  // If we clear the "EAP" property, we just assume these valid certificate
  // credentials are the ones to be used.
  SetEap("");
  EXPECT_TRUE(eap_.IsConnectable());

  // Check that clearing the certificate parameter breaks us again.
  SetClientCert("");
  EXPECT_FALSE(eap_.IsConnectable());

  // Setting the cert-id will fix things.
  SetCertId("client-cert-id");
  EXPECT_TRUE(eap_.IsConnectable());
}

TEST_F(EapCredentialsTest, ConnectableUsingPassphrase) {
  EXPECT_FALSE(eap_.IsConnectableUsingPassphrase());

  // No password.
  SetIdentity("TestIdentity");
  EXPECT_FALSE(eap_.IsConnectableUsingPassphrase());

  // Success.
  SetPassword("TestPassword");
  EXPECT_TRUE(eap_.IsConnectableUsingPassphrase());

  // Clear identity.
  SetIdentity("");
  EXPECT_FALSE(eap_.IsConnectableUsingPassphrase());
}

TEST_F(EapCredentialsTest, IsEapAuthenticationProperty) {
  EXPECT_TRUE(EapCredentials::IsEapAuthenticationProperty(
      kEapAnonymousIdentityProperty));
  EXPECT_TRUE(EapCredentials::IsEapAuthenticationProperty(kEapCertIdProperty));
  EXPECT_TRUE(EapCredentials::IsEapAuthenticationProperty(
      kEapClientCertProperty));
  EXPECT_TRUE(EapCredentials::IsEapAuthenticationProperty(
      kEapIdentityProperty));
  EXPECT_TRUE(EapCredentials::IsEapAuthenticationProperty(kEapKeyIdProperty));
  EXPECT_TRUE(EapCredentials::IsEapAuthenticationProperty(kEapKeyMgmtProperty));
  EXPECT_TRUE(EapCredentials::IsEapAuthenticationProperty(
      kEapPasswordProperty));
  EXPECT_TRUE(EapCredentials::IsEapAuthenticationProperty(kEapPinProperty));
  EXPECT_TRUE(EapCredentials::IsEapAuthenticationProperty(
      kEapPrivateKeyProperty));
  EXPECT_TRUE(EapCredentials::IsEapAuthenticationProperty(
      kEapPrivateKeyPasswordProperty));

  // It's easier to test that this function returns TRUE in every situation
  // that it should, than to test all the cases it should return FALSE in.
  EXPECT_FALSE(EapCredentials::IsEapAuthenticationProperty(kEapCaCertProperty));
  EXPECT_FALSE(EapCredentials::IsEapAuthenticationProperty(
      kEapCaCertIdProperty));
  EXPECT_FALSE(EapCredentials::IsEapAuthenticationProperty(
      kEapCaCertNssProperty));
  EXPECT_FALSE(EapCredentials::IsEapAuthenticationProperty(
      kEapCaCertPemProperty));
  EXPECT_FALSE(EapCredentials::IsEapAuthenticationProperty(kEapMethodProperty));
  EXPECT_FALSE(EapCredentials::IsEapAuthenticationProperty(
      kEapPhase2AuthProperty));
  EXPECT_FALSE(EapCredentials::IsEapAuthenticationProperty(
      kEapRemoteCertificationProperty));
  EXPECT_FALSE(EapCredentials::IsEapAuthenticationProperty(
      kEapSubjectMatchProperty));
  EXPECT_FALSE(EapCredentials::IsEapAuthenticationProperty(
      kEapUseProactiveKeyCachingProperty));
  EXPECT_FALSE(EapCredentials::IsEapAuthenticationProperty(
      kEapUseSystemCasProperty));
}

TEST_F(EapCredentialsTest, LoadAndSave) {
  MockStore store;
  // For the values we're not testing...
  EXPECT_CALL(store, GetCryptedString(_, _, _)).WillRepeatedly(Return(false));
  EXPECT_CALL(store, GetString(_, _, _)).WillRepeatedly(Return(false));

  const string kId("storage-id");
  const string kIdentity("Purple Onion");
  EXPECT_CALL(store, GetCryptedString(
      kId, EapCredentials::kStorageEapIdentity, _))
      .WillOnce(DoAll(SetArgumentPointee<2>(kIdentity), Return(true)));
  const string kManagement("Shave and a Haircut");
  EXPECT_CALL(store, GetString(
      kId, EapCredentials::kStorageEapKeyManagement, _))
      .WillOnce(DoAll(SetArgumentPointee<2>(kManagement), Return(true)));
  const string kPassword("Two Bits");
  EXPECT_CALL(store, GetCryptedString(
      kId, EapCredentials::kStorageEapPassword, _))
      .WillOnce(DoAll(SetArgumentPointee<2>(kPassword), Return(true)));

  eap_.Load(&store, kId);
  Mock::VerifyAndClearExpectations(&store);

  EXPECT_EQ(kIdentity, eap_.identity());
  EXPECT_EQ(kManagement, eap_.key_management());
  EXPECT_EQ(kPassword, GetPassword());

  // Authentication properties are deleted from the store if they are empty,
  // so we expect the fields that we haven't set to be deleted.
  EXPECT_CALL(store, DeleteKey(_, _)).Times(AnyNumber());
  EXPECT_CALL(store, SetCryptedString(_, _, _)).Times(0);
  EXPECT_CALL(store, DeleteKey(kId, EapCredentials::kStorageEapIdentity));
  EXPECT_CALL(store, SetString(
      kId, EapCredentials::kStorageEapKeyManagement, kManagement));
  EXPECT_CALL(store, DeleteKey(kId, EapCredentials::kStorageEapPassword));
  eap_.Save(&store, kId, false);
  Mock::VerifyAndClearExpectations(&store);

  // Authentication properties are deleted from the store if they are empty,
  // so we expect the fields that we haven't set to be deleted.
  EXPECT_CALL(store, DeleteKey(_, _)).Times(AnyNumber());
  EXPECT_CALL(store, SetCryptedString(
      kId, EapCredentials::kStorageEapIdentity, kIdentity));
  EXPECT_CALL(store, SetString(
      kId, EapCredentials::kStorageEapKeyManagement, kManagement));
  EXPECT_CALL(store, SetCryptedString(
      kId, EapCredentials::kStorageEapPassword, kPassword));
  eap_.Save(&store, kId, true);
}

TEST_F(EapCredentialsTest, OutputConnectionMetrics) {
  Error unused_error;
  SetEap(kEapMethodPEAP);
  SetInnerEap(kEapPhase2AuthPEAPMSCHAPV2);

  MockEventDispatcher dispatcher;
  MockMetrics metrics(&dispatcher);
  EXPECT_CALL(metrics, SendEnumToUMA("Network.Shill.Wifi.EapOuterProtocol",
                                     Metrics::kEapOuterProtocolPeap,
                                     Metrics::kEapOuterProtocolMax));
  EXPECT_CALL(metrics, SendEnumToUMA("Network.Shill.Wifi.EapInnerProtocol",
                                     Metrics::kEapInnerProtocolPeapMschapv2,
                                     Metrics::kEapInnerProtocolMax));
  eap_.OutputConnectionMetrics(&metrics, Technology::kWifi);
}

TEST_F(EapCredentialsTest, PopulateSupplicantProperties) {
  SetIdentity("testidentity");
  SetPin("xxxx");
  PopulateSupplicantProperties();
  // Test that only non-empty 802.1x properties are populated.
  EXPECT_TRUE(
      params_.ContainsString(WPASupplicant::kNetworkPropertyEapIdentity));
  EXPECT_FALSE(params_.ContainsString(WPASupplicant::kNetworkPropertyEapKeyId));
  EXPECT_FALSE(
      params_.ContainsString(WPASupplicant::kNetworkPropertyEapCaCert));

  // Test that CA path is set by default.
  EXPECT_TRUE(params_.ContainsString(WPASupplicant::kNetworkPropertyCaPath));

  // Test that hardware-backed security arguments are not set, since
  // neither key-id nor cert-id were set.
  EXPECT_FALSE(params_.ContainsString(WPASupplicant::kNetworkPropertyEapPin));
  EXPECT_FALSE(params_.ContainsUint(WPASupplicant::kNetworkPropertyEngine));
  EXPECT_FALSE(params_.ContainsString(WPASupplicant::kNetworkPropertyEngineId));
}

TEST_F(EapCredentialsTest, PopulateSupplicantPropertiesNoSystemCAs) {
  SetIdentity("testidentity");
  SetUseSystemCAs(false);
  PopulateSupplicantProperties();
  // Test that CA path is not set if use_system_cas is explicitly false.
  EXPECT_FALSE(params_.ContainsString(WPASupplicant::kNetworkPropertyCaPath));
}

TEST_F(EapCredentialsTest,
       PopulateSupplicantPropertiesProactiveKeyCachingDisabledByDefault) {
  SetIdentity("testidentity");
  PopulateSupplicantProperties();

  ASSERT_TRUE(params_.ContainsUint(
      WPASupplicant::kNetworkPropertyEapProactiveKeyCaching));

  const uint32_t kProactiveKeyCachingDisabled(0);

  EXPECT_EQ(kProactiveKeyCachingDisabled,
            params_.GetUint(
                WPASupplicant::kNetworkPropertyEapProactiveKeyCaching));
}

TEST_F(EapCredentialsTest,
       PopulateSupplicantPropertiesEnableProactiveKeyCaching) {
  SetIdentity("testidentity");
  SetUseProactiveKeyCaching(true);
  PopulateSupplicantProperties();

  ASSERT_TRUE(params_.ContainsUint(
      WPASupplicant::kNetworkPropertyEapProactiveKeyCaching));

  const uint32_t kProactiveKeyCachingEnabled(1);

  EXPECT_EQ(kProactiveKeyCachingEnabled,
            params_.GetUint(
                WPASupplicant::kNetworkPropertyEapProactiveKeyCaching));
}

TEST_F(EapCredentialsTest,
       PopulateSupplicantPropertiesDisableProactiveKeyCaching) {
  SetIdentity("testidentity");
  SetUseProactiveKeyCaching(false);
  PopulateSupplicantProperties();

  ASSERT_TRUE(params_.ContainsUint(
      WPASupplicant::kNetworkPropertyEapProactiveKeyCaching));

  const uint32_t kProactiveKeyCachingDisabled(0);

  EXPECT_EQ(kProactiveKeyCachingDisabled,
            params_.GetUint(
                WPASupplicant::kNetworkPropertyEapProactiveKeyCaching));
}

TEST_F(EapCredentialsTest, PopulateSupplicantPropertiesUsingHardwareAuth) {
  SetIdentity("testidentity");
  SetKeyId("key_id");
  SetPin("xxxx");
  SetEap("PEAP");
  PopulateSupplicantProperties();
  // Test that EAP engine parameters are not set if the authentication type
  // is not one that accepts a client certificate.
  EXPECT_FALSE(params_.ContainsString(WPASupplicant::kNetworkPropertyEapPin));
  EXPECT_FALSE(params_.ContainsString(WPASupplicant::kNetworkPropertyEapKeyId));
  EXPECT_FALSE(params_.ContainsUint(WPASupplicant::kNetworkPropertyEngine));
  EXPECT_FALSE(params_.ContainsString(WPASupplicant::kNetworkPropertyEngineId));

  // Test that EAP engine parameters are set if key_id is set and the
  // authentication type accepts a client certificate.
  params_.Clear();
  SetEap("TLS");
  PopulateSupplicantProperties();
  EXPECT_TRUE(params_.ContainsString(WPASupplicant::kNetworkPropertyEapPin));
  EXPECT_TRUE(params_.ContainsString(WPASupplicant::kNetworkPropertyEapKeyId));
  EXPECT_TRUE(params_.ContainsUint(WPASupplicant::kNetworkPropertyEngine));
  EXPECT_TRUE(params_.ContainsString(WPASupplicant::kNetworkPropertyEngineId));

  // An empty EAP parameter should be considered to be possibly "TLS".
  params_.Clear();
  SetEap("");
  PopulateSupplicantProperties();
  EXPECT_TRUE(params_.ContainsString(WPASupplicant::kNetworkPropertyEapPin));
  EXPECT_TRUE(params_.ContainsString(WPASupplicant::kNetworkPropertyEapKeyId));
  EXPECT_TRUE(params_.ContainsUint(WPASupplicant::kNetworkPropertyEngine));
  EXPECT_TRUE(params_.ContainsString(WPASupplicant::kNetworkPropertyEngineId));

  // Test that EAP engine parameters are set if ca_cert_id is set even if the
  // authentication type does not accept a client certificate.  However,
  // the client key id should not be provided.
  params_.Clear();
  SetEap("PEAP");
  SetCACertId("certid");
  PopulateSupplicantProperties();
  EXPECT_TRUE(params_.ContainsString(WPASupplicant::kNetworkPropertyEapPin));
  EXPECT_FALSE(params_.ContainsString(WPASupplicant::kNetworkPropertyEapKeyId));
  EXPECT_TRUE(params_.ContainsUint(WPASupplicant::kNetworkPropertyEngine));
  EXPECT_TRUE(params_.ContainsString(WPASupplicant::kNetworkPropertyEngineId));
  EXPECT_TRUE(
      params_.ContainsString(WPASupplicant::kNetworkPropertyEapCaCertId));
}

TEST_F(EapCredentialsTest, PopulateSupplicantPropertiesPEM) {
  const vector<string> kPemCert{ "-pem-certificate-here-" };
  SetCACertPEM(kPemCert);
  const string kPEMCertfile("/tmp/pem-cert");
  FilePath pem_cert(kPEMCertfile);
  EXPECT_CALL(certificate_file_, CreatePEMFromStrings(kPemCert))
      .WillOnce(Return(pem_cert));

  PopulateSupplicantProperties();
  EXPECT_TRUE(params_.ContainsString(WPASupplicant::kNetworkPropertyEapCaCert));
  if (params_.ContainsString(WPASupplicant::kNetworkPropertyEapCaCert)) {
    EXPECT_EQ(kPEMCertfile,
              params_.GetString(WPASupplicant::kNetworkPropertyEapCaCert));
  }
}

TEST_F(EapCredentialsTest, PopulateWiMaxProperties) {
  {
    KeyValueStore parameters;
    eap_.PopulateWiMaxProperties(&parameters);

    EXPECT_FALSE(parameters.ContainsString(
        wimax_manager::kEAPAnonymousIdentity));
    EXPECT_FALSE(parameters.ContainsString(
        wimax_manager::kEAPUserIdentity));
    EXPECT_FALSE(parameters.ContainsString(
        wimax_manager::kEAPUserPassword));
  }

  const string kAnonymousIdentity("TestAnonymousIdentity");
  SetAnonymousIdentity(kAnonymousIdentity);
  const string kIdentity("TestUserIdentity");
  SetIdentity(kIdentity);
  const string kPassword("TestPassword");
  SetPassword(kPassword);

  {
    KeyValueStore parameters;
    eap_.PopulateWiMaxProperties(&parameters);
    EXPECT_EQ(kAnonymousIdentity, parameters.LookupString(
        wimax_manager::kEAPAnonymousIdentity, ""));
    EXPECT_EQ(kIdentity, parameters.LookupString(
        wimax_manager::kEAPUserIdentity, ""));
    EXPECT_EQ(kPassword, parameters.LookupString(
        wimax_manager::kEAPUserPassword, ""));
  }
}

TEST_F(EapCredentialsTest, Reset) {
  EXPECT_TRUE(IsReset());
  EXPECT_TRUE(GetKeyManagement().empty());
  SetAnonymousIdentity("foo");
  SetCACertId("foo");
  SetCACertNSS("foo");
  SetCACertPEM(vector<string>{ "foo" });
  SetClientCert("foo");
  SetCertId("foo");
  SetEap("foo");
  SetIdentity("foo");
  SetInnerEap("foo");
  SetKeyId("foo");
  SetPassword("foo");
  SetPrivateKey("foo");
  SetPin("foo");
  SetUseSystemCAs(false);
  SetUseProactiveKeyCaching(true);
  eap_.SetKeyManagement("foo", nullptr);
  EXPECT_FALSE(IsReset());
  EXPECT_FALSE(GetKeyManagement().empty());
  eap_.Reset();
  EXPECT_TRUE(IsReset());
  EXPECT_FALSE(GetKeyManagement().empty());
}

TEST_F(EapCredentialsTest, SetKeyManagement) {
  const string kKeyManagement0("foo");
  eap_.SetKeyManagement(kKeyManagement0, nullptr);
  EXPECT_EQ(kKeyManagement0, GetKeyManagement());

  const string kKeyManagement1("bar");
  eap_.SetKeyManagement(kKeyManagement1, nullptr);
  EXPECT_EQ(kKeyManagement1, GetKeyManagement());

  // We should not be able to set the key management to an empty string.
  eap_.SetKeyManagement("", nullptr);
  EXPECT_EQ(kKeyManagement1, GetKeyManagement());
}

// Custom property setters should return false, and make no changes, if
// the new value is the same as the old value.
TEST_F(EapCredentialsTest, CustomSetterNoopChange) {
  // SetEapKeyManagement
  {
    const string kKeyManagement("foo");
    Error error;
    // Set to known value.
    EXPECT_TRUE(eap_.SetKeyManagement(kKeyManagement, &error));
    EXPECT_TRUE(error.IsSuccess());
    // Set to same value.
    EXPECT_FALSE(eap_.SetKeyManagement(kKeyManagement, &error));
    EXPECT_TRUE(error.IsSuccess());
  }

  // SetEapPassword
  {
    const string kPassword("foo");
    Error error;
    // Set to known value.
    EXPECT_TRUE(SetEapPassword(kPassword, &error));
    EXPECT_TRUE(error.IsSuccess());
    // Set to same value.
    EXPECT_FALSE(SetEapPassword(kPassword, &error));
    EXPECT_TRUE(error.IsSuccess());
  }

  // SetEapPrivateKeyPassword
  {
    const string kPrivateKeyPassword("foo");
    Error error;
    // Set to known value.
    EXPECT_TRUE(SetEapPrivateKeyPassword(kPrivateKeyPassword, &error));
    EXPECT_TRUE(error.IsSuccess());
    // Set to same value.
    EXPECT_FALSE(SetEapPrivateKeyPassword(kPrivateKeyPassword, &error));
    EXPECT_TRUE(error.IsSuccess());
  }
}

}  // namespace shill
