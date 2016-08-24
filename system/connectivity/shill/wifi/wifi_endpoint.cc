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

#include "shill/wifi/wifi_endpoint.h"

#include <algorithm>

#include <base/stl_util.h>
#include <base/strings/stringprintf.h>
#include <base/strings/string_number_conversions.h>
#include <base/strings/string_util.h>
#if defined(__ANDROID__)
#include <dbus/service_constants.h>
#else
#include <chromeos/dbus/service_constants.h>
#endif  // __ANDROID__

#include "shill/control_interface.h"
#include "shill/logging.h"
#include "shill/metrics.h"
#include "shill/net/ieee80211.h"
#include "shill/supplicant/supplicant_bss_proxy_interface.h"
#include "shill/supplicant/wpa_supplicant.h"
#include "shill/tethering.h"
#include "shill/wifi/wifi.h"

using base::StringPrintf;
using std::map;
using std::set;
using std::string;
using std::vector;

namespace shill {

namespace Logging {
static auto kModuleLogScope = ScopeLogger::kWiFi;
static string ObjectID(WiFiEndpoint* w) { return "(wifi_endpoint)"; }
}

WiFiEndpoint::WiFiEndpoint(ControlInterface* control_interface,
                           const WiFiRefPtr& device,
                           const string& rpc_id,
                           const KeyValueStore& properties)
    : frequency_(0),
      physical_mode_(Metrics::kWiFiNetworkPhyModeUndef),
      ieee80211w_required_(false),
      control_interface_(control_interface),
      device_(device),
      rpc_id_(rpc_id) {
  ssid_ = properties.GetUint8s(WPASupplicant::kBSSPropertySSID);
  bssid_ = properties.GetUint8s(WPASupplicant::kBSSPropertyBSSID);
  signal_strength_ = properties.GetInt16(WPASupplicant::kBSSPropertySignal);
  if (properties.ContainsUint16(WPASupplicant::kBSSPropertyFrequency)) {
    frequency_ = properties.GetUint16(WPASupplicant::kBSSPropertyFrequency);
  }

  Metrics::WiFiNetworkPhyMode phy_mode = Metrics::kWiFiNetworkPhyModeUndef;
  if (!ParseIEs(properties, &phy_mode, &vendor_information_,
                &ieee80211w_required_, &country_code_)) {
    phy_mode = DeterminePhyModeFromFrequency(properties, frequency_);
  }
  physical_mode_ = phy_mode;

  network_mode_ = ParseMode(
      properties.GetString(WPASupplicant::kBSSPropertyMode));
  set_security_mode(ParseSecurity(properties, &security_flags_));
  has_rsn_property_ =
      properties.ContainsKeyValueStore(WPASupplicant::kPropertyRSN);
  has_wpa_property_ =
      properties.ContainsKeyValueStore(WPASupplicant::kPropertyWPA);

  if (network_mode_.empty()) {
    // XXX log error?
  }

  ssid_string_ = string(ssid_.begin(), ssid_.end());
  WiFi::SanitizeSSID(&ssid_string_);
  ssid_hex_ = base::HexEncode(&(*ssid_.begin()), ssid_.size());
  bssid_string_ = Device::MakeStringFromHardwareAddress(bssid_);
  bssid_hex_ = base::HexEncode(&(*bssid_.begin()), bssid_.size());

  CheckForTetheringSignature();
}

WiFiEndpoint::~WiFiEndpoint() {}

void WiFiEndpoint::Start() {
  supplicant_bss_proxy_.reset(
      control_interface_->CreateSupplicantBSSProxy(this, rpc_id_));
}

void WiFiEndpoint::PropertiesChanged(const KeyValueStore& properties) {
  SLOG(this, 2) << __func__;
  bool should_notify = false;
  if (properties.ContainsInt16(WPASupplicant::kBSSPropertySignal)) {
    signal_strength_ = properties.GetInt16(WPASupplicant::kBSSPropertySignal);
    should_notify = true;
  }

  if (properties.ContainsString(WPASupplicant::kBSSPropertyMode)) {
    string new_mode =
        ParseMode(properties.GetString(WPASupplicant::kBSSPropertyMode));
    if (new_mode != network_mode_) {
      network_mode_ = new_mode;
      SLOG(this, 2) << "WiFiEndpoint " << bssid_string_ << " mode is now "
                    << network_mode_;
      should_notify = true;
    }
  }

  const char* new_security_mode = ParseSecurity(properties, &security_flags_);
  if (new_security_mode != security_mode()) {
    set_security_mode(new_security_mode);
    SLOG(this, 2) << "WiFiEndpoint " << bssid_string_ << " security is now "
                  << security_mode();
    should_notify = true;
  }

  if (should_notify) {
    device_->NotifyEndpointChanged(this);
  }
}

void WiFiEndpoint::UpdateSignalStrength(int16_t strength) {
  if (signal_strength_ == strength) {
    return;
  }

  SLOG(this, 2) << __func__ << ": signal strength "
                << signal_strength_ << " -> " << strength;
  signal_strength_ = strength;
  device_->NotifyEndpointChanged(this);
}

map<string, string> WiFiEndpoint::GetVendorInformation() const {
  map<string, string> vendor_information;
  if (!vendor_information_.wps_manufacturer.empty()) {
    vendor_information[kVendorWPSManufacturerProperty] =
        vendor_information_.wps_manufacturer;
  }
  if (!vendor_information_.wps_model_name.empty()) {
    vendor_information[kVendorWPSModelNameProperty] =
        vendor_information_.wps_model_name;
  }
  if (!vendor_information_.wps_model_number.empty()) {
    vendor_information[kVendorWPSModelNumberProperty] =
        vendor_information_.wps_model_number;
  }
  if (!vendor_information_.wps_device_name.empty()) {
    vendor_information[kVendorWPSDeviceNameProperty] =
        vendor_information_.wps_device_name;
  }
  if (!vendor_information_.oui_set.empty()) {
    vector<string> oui_vector;
    for (auto oui : vendor_information_.oui_set) {
      oui_vector.push_back(
          StringPrintf("%02x-%02x-%02x",
              oui >> 16, (oui >> 8) & 0xff, oui & 0xff));
    }
    vendor_information[kVendorOUIListProperty] =
        base::JoinString(oui_vector, " ");
  }
  return vendor_information;
}

// static
uint32_t WiFiEndpoint::ModeStringToUint(const string& mode_string) {
  if (mode_string == kModeManaged)
    return WPASupplicant::kNetworkModeInfrastructureInt;
  else if (mode_string == kModeAdhoc)
    return WPASupplicant::kNetworkModeAdHocInt;
  else
    NOTIMPLEMENTED() << "Shill dos not support " << mode_string
                     << " mode at this time.";
  return 0;
}

const vector<uint8_t>& WiFiEndpoint::ssid() const {
  return ssid_;
}

const string& WiFiEndpoint::ssid_string() const {
  return ssid_string_;
}

const string& WiFiEndpoint::ssid_hex() const {
  return ssid_hex_;
}

const string& WiFiEndpoint::bssid_string() const {
  return bssid_string_;
}

const string& WiFiEndpoint::bssid_hex() const {
  return bssid_hex_;
}

const string& WiFiEndpoint::country_code() const {
  return country_code_;
}

const WiFiRefPtr& WiFiEndpoint::device() const {
  return device_;
}

int16_t WiFiEndpoint::signal_strength() const {
  return signal_strength_;
}

uint16_t WiFiEndpoint::frequency() const {
  return frequency_;
}

uint16_t WiFiEndpoint::physical_mode() const {
  return physical_mode_;
}

const string& WiFiEndpoint::network_mode() const {
  return network_mode_;
}

const string& WiFiEndpoint::security_mode() const {
  return security_mode_;
}

bool WiFiEndpoint::ieee80211w_required() const {
  return ieee80211w_required_;
}

bool WiFiEndpoint::has_rsn_property() const {
  return has_rsn_property_;
}

bool WiFiEndpoint::has_wpa_property() const {
  return has_wpa_property_;
}

bool WiFiEndpoint::has_tethering_signature() const {
  return has_tethering_signature_;
}

// static
WiFiEndpoint* WiFiEndpoint::MakeOpenEndpoint(
    ControlInterface* control_interface,
    const WiFiRefPtr& wifi,
    const string& ssid,
    const string& bssid,
    const string& network_mode,
    uint16_t frequency,
    int16_t signal_dbm) {
  return MakeEndpoint(control_interface, wifi, ssid, bssid, network_mode,
                      frequency, signal_dbm, false, false);
}


// static
WiFiEndpoint* WiFiEndpoint::MakeEndpoint(ControlInterface* control_interface,
                                         const WiFiRefPtr& wifi,
                                         const string& ssid,
                                         const string& bssid,
                                         const string& network_mode,
                                         uint16_t frequency,
                                         int16_t signal_dbm,
                                         bool has_wpa_property,
                                         bool has_rsn_property) {
  KeyValueStore args;

  args.SetUint8s(WPASupplicant::kBSSPropertySSID,
                 vector<uint8_t>(ssid.begin(), ssid.end()));

  vector<uint8_t> bssid_bytes =
      Device::MakeHardwareAddressFromString(bssid);
  args.SetUint8s(WPASupplicant::kBSSPropertyBSSID, bssid_bytes);

  args.SetInt16(WPASupplicant::kBSSPropertySignal, signal_dbm);
  args.SetUint16(WPASupplicant::kBSSPropertyFrequency, frequency);
  args.SetString(WPASupplicant::kBSSPropertyMode, network_mode);

  if (has_wpa_property) {
    KeyValueStore empty_args;
    args.SetKeyValueStore(WPASupplicant::kPropertyWPA, empty_args);
  }
  if (has_rsn_property) {
    KeyValueStore empty_args;
    args.SetKeyValueStore(WPASupplicant::kPropertyRSN, empty_args);
  }

  return new WiFiEndpoint(
      control_interface, wifi, bssid, args);  // |bssid| fakes an RPC ID
}

// static
const char* WiFiEndpoint::ParseMode(const string& mode_string) {
  if (mode_string == WPASupplicant::kNetworkModeInfrastructure) {
    return kModeManaged;
  } else if (mode_string == WPASupplicant::kNetworkModeAdHoc) {
    return kModeAdhoc;
  } else if (mode_string == WPASupplicant::kNetworkModeAccessPoint) {
    NOTREACHED() << "Shill does not support AP mode at this time.";
    return nullptr;
  } else {
    NOTREACHED() << "Unknown WiFi endpoint mode!";
    return nullptr;
  }
}

// static
const char* WiFiEndpoint::ParseSecurity(
    const KeyValueStore& properties, SecurityFlags* flags) {
  if (properties.ContainsKeyValueStore(WPASupplicant::kPropertyRSN)) {
    KeyValueStore rsn_properties =
        properties.GetKeyValueStore(WPASupplicant::kPropertyRSN);
    set<KeyManagement> key_management;
    ParseKeyManagementMethods(rsn_properties, &key_management);
    flags->rsn_8021x = ContainsKey(key_management, kKeyManagement802_1x);
    flags->rsn_psk = ContainsKey(key_management, kKeyManagementPSK);
  }

  if (properties.ContainsKeyValueStore(WPASupplicant::kPropertyWPA)) {
    KeyValueStore rsn_properties =
        properties.GetKeyValueStore(WPASupplicant::kPropertyWPA);
    set<KeyManagement> key_management;
    ParseKeyManagementMethods(rsn_properties, &key_management);
    flags->wpa_8021x = ContainsKey(key_management, kKeyManagement802_1x);
    flags->wpa_psk = ContainsKey(key_management, kKeyManagementPSK);
  }

  if (properties.ContainsBool(WPASupplicant::kPropertyPrivacy)) {
    flags->privacy = properties.GetBool(WPASupplicant::kPropertyPrivacy);
  }

  if (flags->rsn_8021x || flags->wpa_8021x) {
    return kSecurity8021x;
  } else if (flags->rsn_psk) {
    return kSecurityRsn;
  } else if (flags->wpa_psk) {
    return kSecurityWpa;
  } else if (flags->privacy) {
    return kSecurityWep;
  } else {
    return kSecurityNone;
  }
}

// static
void WiFiEndpoint::ParseKeyManagementMethods(
    const KeyValueStore& security_method_properties,
    set<KeyManagement>* key_management_methods) {
  if (!security_method_properties.ContainsStrings(
      WPASupplicant::kSecurityMethodPropertyKeyManagement)) {
    return;
  }

  const vector<string> key_management_vec =
      security_method_properties.GetStrings(
          WPASupplicant::kSecurityMethodPropertyKeyManagement);

  for (const auto& method : key_management_vec) {
    if (base::EndsWith(method, WPASupplicant::kKeyManagementMethodSuffixEAP,
                       base::CompareCase::SENSITIVE)) {
      key_management_methods->insert(kKeyManagement802_1x);
    } else if (base::EndsWith(method,
                              WPASupplicant::kKeyManagementMethodSuffixPSK,
                              base::CompareCase::SENSITIVE)) {
      key_management_methods->insert(kKeyManagementPSK);
    }
  }
}

// static
Metrics::WiFiNetworkPhyMode WiFiEndpoint::DeterminePhyModeFromFrequency(
    const KeyValueStore& properties, uint16_t frequency) {
  uint32_t max_rate = 0;
  if (properties.ContainsUint32s(WPASupplicant::kBSSPropertyRates)) {
    vector<uint32_t> rates =
        properties.GetUint32s(WPASupplicant::kBSSPropertyRates);
    if (rates.size() > 0) {
      max_rate = rates[0];  // Rates are sorted in descending order
    }
  }

  Metrics::WiFiNetworkPhyMode phy_mode = Metrics::kWiFiNetworkPhyModeUndef;
  if (frequency < 3000) {
    // 2.4GHz legacy, check for tx rate for 11b-only
    // (note 22M is valid)
    if (max_rate < 24000000)
      phy_mode = Metrics::kWiFiNetworkPhyMode11b;
    else
      phy_mode = Metrics::kWiFiNetworkPhyMode11g;
  } else {
    phy_mode = Metrics::kWiFiNetworkPhyMode11a;
  }

  return phy_mode;
}

// static
bool WiFiEndpoint::ParseIEs(
    const KeyValueStore& properties,
    Metrics::WiFiNetworkPhyMode* phy_mode,
    VendorInformation* vendor_information,
    bool* ieee80211w_required, string* country_code) {

  if (!properties.ContainsUint8s(WPASupplicant::kBSSPropertyIEs)) {
    SLOG(nullptr, 2) << __func__ << ": No IE property in BSS.";
    return false;
  }
  vector<uint8_t> ies = properties.GetUint8s(WPASupplicant::kBSSPropertyIEs);

  // Format of an information element:
  //    1       1          1 - 252
  // +------+--------+----------------+
  // | Type | Length | Data           |
  // +------+--------+----------------+
  *phy_mode = Metrics::kWiFiNetworkPhyModeUndef;
  bool found_ht = false;
  bool found_vht = false;
  bool found_erp = false;
  int ie_len = 0;
  vector<uint8_t>::iterator it;
  for (it = ies.begin();
       std::distance(it, ies.end()) > 1;  // Ensure Length field is within PDU.
       it += ie_len) {
    ie_len = 2 + *(it + 1);
    if (std::distance(it, ies.end()) < ie_len) {
      LOG(ERROR) << __func__ << ": IE extends past containing PDU.";
      break;
    }
    switch (*it) {
      case IEEE_80211::kElemIdCountry:
        // Retrieve 2-character country code from the beginning of the element.
        if (ie_len >= 4) {
          *country_code = string(it + 2, it + 4);
        }
      case IEEE_80211::kElemIdErp:
        found_erp = true;
        break;
      case IEEE_80211::kElemIdHTCap:
      case IEEE_80211::kElemIdHTInfo:
        found_ht = true;
        break;
      case IEEE_80211::kElemIdVHTCap:
      case IEEE_80211::kElemIdVHTOperation:
        found_vht = true;
        break;
      case IEEE_80211::kElemIdRSN:
        ParseWPACapabilities(it + 2, it + ie_len, ieee80211w_required);
        break;
      case IEEE_80211::kElemIdVendor:
        ParseVendorIE(it + 2, it + ie_len, vendor_information,
                      ieee80211w_required);
        break;
    }
  }
  if (found_vht) {
    *phy_mode = Metrics::kWiFiNetworkPhyMode11ac;
  } else if (found_ht) {
    *phy_mode = Metrics::kWiFiNetworkPhyMode11n;
  } else if (found_erp) {
    *phy_mode = Metrics::kWiFiNetworkPhyMode11g;
  } else {
    return false;
  }
  return true;
}

// static
void WiFiEndpoint::ParseWPACapabilities(
    vector<uint8_t>::const_iterator ie,
    vector<uint8_t>::const_iterator end,
    bool* ieee80211w_required) {
  // Format of an RSN Information Element:
  //    2             4
  // +------+--------------------+
  // | Type | Group Cipher Suite |
  // +------+--------------------+
  //             2             4 * pairwise count
  // +-----------------------+---------------------+
  // | Pairwise Cipher Count | Pairwise Ciphers... |
  // +-----------------------+---------------------+
  //             2             4 * authkey count
  // +-----------------------+---------------------+
  // | AuthKey Suite Count   | AuthKey Suites...   |
  // +-----------------------+---------------------+
  //          2
  // +------------------+
  // | RSN Capabilities |
  // +------------------+
  //          2            16 * pmkid count
  // +------------------+-------------------+
  // |   PMKID Count    |      PMKIDs...    |
  // +------------------+-------------------+
  //          4
  // +-------------------------------+
  // | Group Management Cipher Suite |
  // +-------------------------------+
  if (std::distance(ie, end) < IEEE_80211::kRSNIECipherCountOffset) {
    return;
  }
  ie += IEEE_80211::kRSNIECipherCountOffset;

  // Advance past the pairwise and authkey ciphers.  Each is a little-endian
  // cipher count followed by n * cipher_selector.
  for (int i = 0; i < IEEE_80211::kRSNIENumCiphers; ++i) {
    // Retrieve a little-endian cipher count.
    if (std::distance(ie, end) < IEEE_80211::kRSNIECipherCountLen) {
      return;
    }
    uint16_t cipher_count = *ie | (*(ie + 1) << 8);

    // Skip over the cipher selectors.
    int skip_length = IEEE_80211::kRSNIECipherCountLen +
      cipher_count * IEEE_80211::kRSNIESelectorLen;
    if (std::distance(ie, end) < skip_length) {
      return;
    }
    ie += skip_length;
  }

  if (std::distance(ie, end) < IEEE_80211::kRSNIECapabilitiesLen) {
    return;
  }

  // Retrieve a little-endian capabilities bitfield.
  uint16_t capabilities = *ie | (*(ie + 1) << 8);

  if (capabilities & IEEE_80211::kRSNCapabilityFrameProtectionRequired &&
      ieee80211w_required) {
    // Never set this value to false, since there may be multiple RSN
    // information elements.
    *ieee80211w_required = true;
  }
}


// static
void WiFiEndpoint::ParseVendorIE(vector<uint8_t>::const_iterator ie,
                                 vector<uint8_t>::const_iterator end,
                                 VendorInformation* vendor_information,
                                 bool* ieee80211w_required) {
  // Format of an vendor-specific information element (with type
  // and length field for the IE removed by the caller):
  //        3           1       1 - 248
  // +------------+----------+----------------+
  // | OUI        | OUI Type | Data           |
  // +------------+----------+----------------+

  if (std::distance(ie, end) < 4) {
    LOG(ERROR) << __func__ << ": no room in IE for OUI and type field.";
    return;
  }
  uint32_t oui = (*ie << 16) | (*(ie + 1) << 8) | *(ie + 2);
  uint8_t oui_type = *(ie + 3);
  ie += 4;

  if (oui == IEEE_80211::kOUIVendorMicrosoft &&
      oui_type == IEEE_80211::kOUIMicrosoftWPS) {
    // Format of a WPS data element:
    //    2       2
    // +------+--------+----------------+
    // | Type | Length | Data           |
    // +------+--------+----------------+
    while (std::distance(ie, end) >= 4) {
      int element_type = (*ie << 8) | *(ie + 1);
      int element_length = (*(ie + 2) << 8) | *(ie + 3);
      ie += 4;
      if (std::distance(ie, end) < element_length) {
        LOG(ERROR) << __func__ << ": WPS element extends past containing PDU.";
        break;
      }
      string s(ie, ie + element_length);
      if (base::IsStringASCII(s)) {
        switch (element_type) {
          case IEEE_80211::kWPSElementManufacturer:
            vendor_information->wps_manufacturer = s;
            break;
          case IEEE_80211::kWPSElementModelName:
            vendor_information->wps_model_name = s;
            break;
          case IEEE_80211::kWPSElementModelNumber:
            vendor_information->wps_model_number = s;
            break;
          case IEEE_80211::kWPSElementDeviceName:
            vendor_information->wps_device_name = s;
            break;
        }
      }
      ie += element_length;
    }
  } else if (oui == IEEE_80211::kOUIVendorMicrosoft &&
             oui_type == IEEE_80211::kOUIMicrosoftWPA) {
    ParseWPACapabilities(ie, end, ieee80211w_required);
  } else if (oui != IEEE_80211::kOUIVendorEpigram &&
             oui != IEEE_80211::kOUIVendorMicrosoft) {
    vendor_information->oui_set.insert(oui);
  }
}

void WiFiEndpoint::CheckForTetheringSignature() {
  has_tethering_signature_ =
      Tethering::IsAndroidBSSID(bssid_) ||
      (Tethering::IsLocallyAdministeredBSSID(bssid_) &&
       Tethering::HasIosOui(vendor_information_.oui_set));
}

}  // namespace shill
