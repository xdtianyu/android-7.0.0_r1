//
// Copyright (C) 2012 The Android Open Source Project
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

#ifndef SHILL_SUPPLICANT_WPA_SUPPLICANT_H_
#define SHILL_SUPPLICANT_WPA_SUPPLICANT_H_

#include <map>
#include <string>
#include <vector>

#include "shill/key_value_store.h"

namespace shill {

class WPASupplicant {
 public:
  static const char kBSSPropertyBSSID[];
  static const char kBSSPropertyFrequency[];
  static const char kBSSPropertyIEs[];
  static const char kBSSPropertyMode[];
  static const char kBSSPropertyRates[];
  static const char kBSSPropertySSID[];
  static const char kBSSPropertySignal[];
  static const char kCaPath[];
  static const char kCurrentBSSNull[];
  static const char kDBusAddr[];
  static const char kDBusPath[];
  static const char kDebugLevelDebug[];
  static const char kDebugLevelError[];
  static const char kDebugLevelExcessive[];
  static const char kDebugLevelInfo[];
  static const char kDebugLevelMsgDump[];
  static const char kDebugLevelWarning[];
  static const char kDriverNL80211[];
  static const char kDriverWired[];
  static const char kEAPParameterAlertUnknownCA[];
  static const char kEAPParameterFailure[];
  static const char kEAPParameterSuccess[];
  static const char kEAPRequestedParameterPIN[];
  static const char kEAPStatusAcceptProposedMethod[];
  static const char kEAPStatusCompletion[];
  static const char kEAPStatusLocalTLSAlert[];
  static const char kEAPStatusParameterNeeded[];
  static const char kEAPStatusRemoteCertificateVerification[];
  static const char kEAPStatusRemoteTLSAlert[];
  static const char kEAPStatusStarted[];
  static const char kEnginePKCS11[];
  static const char kErrorNetworkUnknown[];
  static const char kErrorInterfaceExists[];
  static const char kInterfacePropertyConfigFile[];
  static const char kInterfacePropertyCurrentBSS[];
  static const char kInterfacePropertyDepth[];
  static const char kInterfacePropertyDisconnectReason[];
  static const char kInterfacePropertyDriver[];
  static const char kInterfacePropertyName[];
  static const char kInterfacePropertyState[];
  static const char kInterfacePropertySubject[];
  static const char kInterfaceState4WayHandshake[];
  static const char kInterfaceStateAssociated[];
  static const char kInterfaceStateAssociating[];
  static const char kInterfaceStateAuthenticating[];
  static const char kInterfaceStateCompleted[];
  static const char kInterfaceStateDisconnected[];
  static const char kInterfaceStateGroupHandshake[];
  static const char kInterfaceStateInactive[];
  static const char kInterfaceStateScanning[];
  static const char kKeyManagementIeee8021X[];
  static const char kKeyManagementMethodSuffixEAP[];
  static const char kKeyManagementMethodSuffixPSK[];
  static const char kKeyModeNone[];
  static const char kNetworkBgscanMethodLearn[];
// None is not a real method name, but we interpret 'none' as a request that
// no background scan parameter should be supplied to wpa_supplicant.
  static const char kNetworkBgscanMethodNone[];
  static const char kNetworkBgscanMethodSimple[];
  static const char kNetworkModeInfrastructure[];
  static const char kNetworkModeAdHoc[];
  static const char kNetworkModeAccessPoint[];
  static const char kNetworkPropertyBgscan[];
  static const char kNetworkPropertyCaPath[];
  static const char kNetworkPropertyDisableVHT[];
  static const char kNetworkPropertyEapKeyManagement[];
  static const char kNetworkPropertyEapIdentity[];
  static const char kNetworkPropertyEapEap[];
  static const char kNetworkPropertyEapInnerEap[];
  static const char kNetworkPropertyEapAnonymousIdentity[];
  static const char kNetworkPropertyEapClientCert[];
  static const char kNetworkPropertyEapPrivateKey[];
  static const char kNetworkPropertyEapPrivateKeyPassword[];
  static const char kNetworkPropertyEapCaCert[];
  static const char kNetworkPropertyEapCaPassword[];
  static const char kNetworkPropertyEapCertId[];
  static const char kNetworkPropertyEapKeyId[];
  static const char kNetworkPropertyEapCaCertId[];
  static const char kNetworkPropertyEapPin[];
  static const char kNetworkPropertyEapProactiveKeyCaching[];
  static const char kNetworkPropertyEapSubjectMatch[];
  static const char kNetworkPropertyEapolFlags[];
  static const char kNetworkPropertyEngine[];
  static const char kNetworkPropertyEngineId[];
  static const char kNetworkPropertyFrequency[];
  static const char kNetworkPropertyIeee80211w[];
  static const char kNetworkPropertyMode[];
  static const char kNetworkPropertySSID[];
  static const char kNetworkPropertyScanSSID[];
// TODO(quiche): Make the naming scheme more consistent, by adding the
// object type to the property names below. (crbug.com/206642)
  static const char kPropertyAuthAlg[];
  static const char kPropertyBSSID[];
  static const char kPropertyMode[];
  static const char kPropertyPreSharedKey[];
  static const char kPropertyPrivacy[];
  static const char kPropertyRSN[];
  static const char kPropertyScanSSIDs[];
  static const char kPropertyScanType[];
  static const char kPropertySecurityProtocol[];
  static const char kPropertySignal[];
  static const char kPropertyWEPKey[];
  static const char kPropertyWEPTxKeyIndex[];
  static const char kPropertyWPA[];
  static const char kScanTypeActive[];
  static const char kSecurityAuthAlg[];
  static const char kSecurityMethodPropertyKeyManagement[];
  static const char kSecurityModeRSN[];
  static const char kSecurityModeWPA[];
  static const char kTDLSStateConnected[];
  static const char kTDLSStateDisabled[];
  static const char kTDLSStatePeerDoesNotExist[];
  static const char kTDLSStatePeerNotConnected[];

  static const uint32_t kDefaultEngine;
  static const uint32_t kNetworkIeee80211wDisabled;
  static const uint32_t kNetworkIeee80211wEnabled;
  static const uint32_t kNetworkIeee80211wRequired;
  static const uint32_t kNetworkModeInfrastructureInt;
  static const uint32_t kNetworkModeAdHocInt;
  static const uint32_t kNetworkModeAccessPointInt;
  static const uint32_t kScanMaxSSIDsPerScan;

  static const uint32_t kProactiveKeyCachingDisabled;
  static const uint32_t kProactiveKeyCachingEnabled;

  static const char kSupplicantConfPath[];

  // Retrieve the |subject| and |depth| of an a remote certifying entity,
  // as contained the the |properties| to a Certification event from
  // wpa_supplicant.  Returns true if an |subject| and |depth| were
  // extracted successfully, false otherwise.
  static bool ExtractRemoteCertification(const KeyValueStore& properties,
                                         std::string* subject,
                                         uint32_t* depth);
};

}  // namespace shill

#endif  // SHILL_SUPPLICANT_WPA_SUPPLICANT_H_
