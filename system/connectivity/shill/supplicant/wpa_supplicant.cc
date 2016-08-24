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

#include "shill/supplicant/wpa_supplicant.h"

#include <map>
#include <string>

#include "shill/logging.h"

using std::map;
using std::string;

namespace shill {

// static
const char WPASupplicant::kBSSPropertyBSSID[] = "BSSID";
const char WPASupplicant::kBSSPropertyFrequency[] = "Frequency";
const char WPASupplicant::kBSSPropertyIEs[] = "IEs";
const char WPASupplicant::kBSSPropertyMode[] = "Mode";
const char WPASupplicant::kBSSPropertyRates[] = "Rates";
const char WPASupplicant::kBSSPropertySSID[] = "SSID";
const char WPASupplicant::kBSSPropertySignal[] = "Signal";
// TODO(gauravsh): Make this path be a configurable option. crbug.com/208594
// Location of the system root CA certificates.
const char WPASupplicant::kCaPath[] = "/etc/ssl/certs";
const char WPASupplicant::kCurrentBSSNull[] = "/";
const char WPASupplicant::kDBusAddr[] = "fi.w1.wpa_supplicant1";
const char WPASupplicant::kDBusPath[] = "/fi/w1/wpa_supplicant1";
const char WPASupplicant::kDebugLevelDebug[] = "debug";
const char WPASupplicant::kDebugLevelError[] = "error";
const char WPASupplicant::kDebugLevelExcessive[] = "excessive";
const char WPASupplicant::kDebugLevelInfo[] = "info";
const char WPASupplicant::kDebugLevelMsgDump[] = "msgdump";
const char WPASupplicant::kDebugLevelWarning[] = "warning";
const char WPASupplicant::kDriverNL80211[] = "nl80211";
const char WPASupplicant::kDriverWired[] = "wired";
const char WPASupplicant::kEAPParameterAlertUnknownCA[] = "unknown CA";
const char WPASupplicant::kEAPParameterFailure[] = "failure";
const char WPASupplicant::kEAPParameterSuccess[] = "success";
const char WPASupplicant::kEAPRequestedParameterPIN[] = "PIN";
const char WPASupplicant::kEAPStatusAcceptProposedMethod[] =
    "accept proposed method";
const char WPASupplicant::kEAPStatusCompletion[] = "completion";
const char WPASupplicant::kEAPStatusLocalTLSAlert[] = "local TLS alert";
const char WPASupplicant::kEAPStatusParameterNeeded[] = "eap parameter needed";
const char WPASupplicant::kEAPStatusRemoteCertificateVerification[] =
    "remote certificate verification";
const char WPASupplicant::kEAPStatusRemoteTLSAlert[] = "remote TLS alert";
const char WPASupplicant::kEAPStatusStarted[] = "started";
const char WPASupplicant::kEnginePKCS11[] = "pkcs11";
const char WPASupplicant::kErrorNetworkUnknown[]
    = "fi.w1.wpa_supplicant1.NetworkUnknown";
const char WPASupplicant::kErrorInterfaceExists[]
    = "fi.w1.wpa_supplicant1.InterfaceExists";
const char WPASupplicant::kInterfacePropertyConfigFile[] = "ConfigFile";
const char WPASupplicant::kInterfacePropertyCurrentBSS[] = "CurrentBSS";
const char WPASupplicant::kInterfacePropertyDepth[] = "depth";
const char WPASupplicant::kInterfacePropertyDisconnectReason[]
    = "DisconnectReason";
const char WPASupplicant::kInterfacePropertyDriver[] = "Driver";
const char WPASupplicant::kInterfacePropertyName[] = "Ifname";
const char WPASupplicant::kInterfacePropertyState[] = "State";
const char WPASupplicant::kInterfacePropertySubject[] = "subject";
const char WPASupplicant::kInterfaceState4WayHandshake[] = "4way_handshake";
const char WPASupplicant::kInterfaceStateAssociated[] = "associated";
const char WPASupplicant::kInterfaceStateAssociating[] = "associating";
const char WPASupplicant::kInterfaceStateAuthenticating[] = "authenticating";
const char WPASupplicant::kInterfaceStateCompleted[] = "completed";
const char WPASupplicant::kInterfaceStateDisconnected[] = "disconnected";
const char WPASupplicant::kInterfaceStateGroupHandshake[] = "group_handshake";
const char WPASupplicant::kInterfaceStateInactive[] = "inactive";
const char WPASupplicant::kInterfaceStateScanning[] = "scanning";
const char WPASupplicant::kKeyManagementIeee8021X[] = "IEEE8021X";
const char WPASupplicant::kKeyManagementMethodSuffixEAP[] = "-eap";
const char WPASupplicant::kKeyManagementMethodSuffixPSK[] = "-psk";
const char WPASupplicant::kKeyModeNone[] = "NONE";
const char WPASupplicant::kNetworkBgscanMethodLearn[] = "learn";
// None is not a real method name, but we interpret 'none' as a request that
// no background scan parameter should be supplied to wpa_supplicant.
const char WPASupplicant::kNetworkBgscanMethodNone[] = "none";
const char WPASupplicant::kNetworkBgscanMethodSimple[] = "simple";
const char WPASupplicant::kNetworkModeInfrastructure[] = "infrastructure";
const char WPASupplicant::kNetworkModeAdHoc[] = "ad-hoc";
const char WPASupplicant::kNetworkModeAccessPoint[] = "ap";
const char WPASupplicant::kNetworkPropertyBgscan[] = "bgscan";
const char WPASupplicant::kNetworkPropertyCaPath[] = "ca_path";
const char WPASupplicant::kNetworkPropertyDisableVHT[] = "disable_vht";
const char WPASupplicant::kNetworkPropertyEapIdentity[] = "identity";
const char WPASupplicant::kNetworkPropertyEapKeyManagement[] = "key_mgmt";
const char WPASupplicant::kNetworkPropertyEapEap[] = "eap";
const char WPASupplicant::kNetworkPropertyEapInnerEap[] = "phase2";
const char WPASupplicant::kNetworkPropertyEapAnonymousIdentity[]
    = "anonymous_identity";
const char WPASupplicant::kNetworkPropertyEapClientCert[] = "client_cert";
const char WPASupplicant::kNetworkPropertyEapPrivateKey[] = "private_key";
const char WPASupplicant::kNetworkPropertyEapPrivateKeyPassword[]
    = "private_key_passwd";
const char WPASupplicant::kNetworkPropertyEapProactiveKeyCaching[]
    = "proactive_key_caching";
const char WPASupplicant::kNetworkPropertyEapCaCert[] = "ca_cert";
const char WPASupplicant::kNetworkPropertyEapCaPassword[] = "password";
const char WPASupplicant::kNetworkPropertyEapCertId[] = "cert_id";
const char WPASupplicant::kNetworkPropertyEapKeyId[] = "key_id";
const char WPASupplicant::kNetworkPropertyEapCaCertId[] = "ca_cert_id";
const char WPASupplicant::kNetworkPropertyEapPin[] = "pin";
const char WPASupplicant::kNetworkPropertyEapSubjectMatch[] = "subject_match";
const char WPASupplicant::kNetworkPropertyEapolFlags[] = "eapol_flags";
const char WPASupplicant::kNetworkPropertyEngine[] = "engine";
const char WPASupplicant::kNetworkPropertyEngineId[] = "engine_id";
const char WPASupplicant::kNetworkPropertyFrequency[] = "frequency";
const char WPASupplicant::kNetworkPropertyIeee80211w[] = "ieee80211w";
const char WPASupplicant::kNetworkPropertyMode[] = "mode";
const char WPASupplicant::kNetworkPropertyScanSSID[] = "scan_ssid";
const char WPASupplicant::kNetworkPropertySSID[] = "ssid";
const char WPASupplicant::kPropertyAuthAlg[] = "auth_alg";
const char WPASupplicant::kPropertyPreSharedKey[] = "psk";
const char WPASupplicant::kPropertyPrivacy[] = "Privacy";
const char WPASupplicant::kPropertyRSN[] = "RSN";
const char WPASupplicant::kPropertyScanSSIDs[] = "SSIDs";
const char WPASupplicant::kPropertyScanType[] = "Type";
const char WPASupplicant::kPropertySecurityProtocol[] = "proto";
const char WPASupplicant::kPropertyWEPKey[] = "wep_key";
const char WPASupplicant::kPropertyWEPTxKeyIndex[] = "wep_tx_keyidx";
const char WPASupplicant::kPropertyWPA[] = "WPA";
const char WPASupplicant::kScanTypeActive[] = "active";
const char WPASupplicant::kSecurityAuthAlg[] = "OPEN SHARED";
const char WPASupplicant::kSecurityMethodPropertyKeyManagement[] = "KeyMgmt";
const char WPASupplicant::kSecurityModeRSN[] = "RSN";
const char WPASupplicant::kSecurityModeWPA[] = "WPA";

const char WPASupplicant::kTDLSStateConnected[] = "connected";
const char WPASupplicant::kTDLSStateDisabled[] = "disabled";
const char WPASupplicant::kTDLSStatePeerDoesNotExist[] = "peer does not exist";
const char WPASupplicant::kTDLSStatePeerNotConnected[] = "peer not connected";

const uint32_t WPASupplicant::kDefaultEngine = 1;
const uint32_t WPASupplicant::kNetworkIeee80211wDisabled = 0;
const uint32_t WPASupplicant::kNetworkIeee80211wEnabled = 1;
const uint32_t WPASupplicant::kNetworkIeee80211wRequired = 2;
const uint32_t WPASupplicant::kNetworkModeInfrastructureInt = 0;
const uint32_t WPASupplicant::kNetworkModeAdHocInt = 1;
const uint32_t WPASupplicant::kNetworkModeAccessPointInt = 2;
const uint32_t WPASupplicant::kScanMaxSSIDsPerScan = 4;

const uint32_t WPASupplicant::kProactiveKeyCachingDisabled = 0;
const uint32_t WPASupplicant::kProactiveKeyCachingEnabled = 1;

const char WPASupplicant::kSupplicantConfPath[] =
    SHIMDIR "/wpa_supplicant.conf";

// static
bool WPASupplicant::ExtractRemoteCertification(const KeyValueStore& properties,
                                               string* subject,
                                               uint32_t* depth) {
  if (!properties.ContainsUint(WPASupplicant::kInterfacePropertyDepth)) {
    LOG(ERROR) << __func__ << " no depth parameter.";
    return false;
  }
  if (!properties.ContainsString(WPASupplicant::kInterfacePropertySubject)) {
    LOG(ERROR) << __func__ << " no subject parameter.";
    return false;
  }

  *depth = properties.GetUint(WPASupplicant::kInterfacePropertyDepth);
  *subject = properties.GetString(WPASupplicant::kInterfacePropertySubject);
  return true;
}

}  // namespace shill
