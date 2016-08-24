//
// Copyright (C) 2014 The Android Open Source Project
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

#include "apmanager/config.h"

#include <base/strings/stringprintf.h>

#if !defined(__ANDROID__)
#include <chromeos/dbus/service_constants.h>
#else
#include <dbus/apmanager/dbus-constants.h>
#endif  // __ANDROID__

#include "apmanager/error.h"
#include "apmanager/daemon.h"
#include "apmanager/device.h"
#include "apmanager/manager.h"

using std::string;

namespace apmanager {

// static
const char Config::kHostapdConfigKeyBridgeInterface[] = "bridge";
const char Config::kHostapdConfigKeyChannel[] = "channel";
const char Config::kHostapdConfigKeyControlInterface[] = "ctrl_interface";
const char Config::kHostapdConfigKeyControlInterfaceGroup[] =
    "ctrl_interface_group";
const char Config::kHostapdConfigKeyDriver[] = "driver";
const char Config::kHostapdConfigKeyFragmThreshold[] = "fragm_threshold";
const char Config::kHostapdConfigKeyHTCapability[] = "ht_capab";
const char Config::kHostapdConfigKeyHwMode[] = "hw_mode";
const char Config::kHostapdConfigKeyIeee80211ac[] = "ieee80211ac";
const char Config::kHostapdConfigKeyIeee80211n[] = "ieee80211n";
const char Config::kHostapdConfigKeyIgnoreBroadcastSsid[] =
    "ignore_broadcast_ssid";
const char Config::kHostapdConfigKeyInterface[] = "interface";
const char Config::kHostapdConfigKeyRsnPairwise[] = "rsn_pairwise";
const char Config::kHostapdConfigKeyRtsThreshold[] = "rts_threshold";
const char Config::kHostapdConfigKeySsid[] = "ssid";
const char Config::kHostapdConfigKeyWepDefaultKey[] = "wep_default_key";
const char Config::kHostapdConfigKeyWepKey0[] = "wep_key0";
const char Config::kHostapdConfigKeyWpa[] = "wpa";
const char Config::kHostapdConfigKeyWpaKeyMgmt[] = "wpa_key_mgmt";
const char Config::kHostapdConfigKeyWpaPassphrase[] = "wpa_passphrase";

const char Config::kHostapdHwMode80211a[] = "a";
const char Config::kHostapdHwMode80211b[] = "b";
const char Config::kHostapdHwMode80211g[] = "g";

// static
const uint16_t Config::kPropertyDefaultChannel = 6;
const uint16_t Config::kPropertyDefaultServerAddressIndex = 0;
const bool Config::kPropertyDefaultHiddenNetwork = false;

// static
const char Config::kHostapdDefaultDriver[] = "nl80211";
const char Config::kHostapdDefaultRsnPairwise[] = "CCMP";
const char Config::kHostapdDefaultWpaKeyMgmt[] = "WPA-PSK";
// Fragmentation threshold: disabled.
const int Config::kHostapdDefaultFragmThreshold = 2346;
// RTS threshold: disabled.
const int Config::kHostapdDefaultRtsThreshold = 2347;

// static
const uint16_t Config::kBand24GHzChannelLow = 1;
const uint16_t Config::kBand24GHzChannelHigh = 13;
const uint32_t Config::kBand24GHzBaseFrequency = 2412;
const uint16_t Config::kBand5GHzChannelLow = 34;
const uint16_t Config::kBand5GHzChannelHigh = 165;
const uint16_t Config::kBand5GHzBaseFrequency = 5170;

// static
const int Config::kSsidMinLength = 1;
const int Config::kSsidMaxLength = 32;
const int Config::kPassphraseMinLength = 8;
const int Config::kPassphraseMaxLength = 63;

Config::Config(Manager* manager, int service_identifier)
    : manager_(manager),
      adaptor_(
          manager->control_interface()->CreateConfigAdaptor(
              this, service_identifier)) {
  // Initialize default configuration values.
  SetSecurityMode(kSecurityModeNone);
  SetHwMode(kHwMode80211g);
  SetOperationMode(kOperationModeServer);
  SetServerAddressIndex(kPropertyDefaultServerAddressIndex);
  SetChannel(kPropertyDefaultChannel);
  SetHiddenNetwork(kPropertyDefaultHiddenNetwork);
  SetFullDeviceControl(true);
}

Config::~Config() {}

// static.
bool Config::GetFrequencyFromChannel(uint16_t channel, uint32_t* freq) {
  bool ret_value = true;
  if (channel >= kBand24GHzChannelLow && channel <= kBand24GHzChannelHigh) {
    *freq = kBand24GHzBaseFrequency + (channel - kBand24GHzChannelLow) * 5;
  } else if (channel >= kBand5GHzChannelLow &&
             channel <= kBand5GHzChannelHigh) {
    *freq = kBand5GHzBaseFrequency + (channel - kBand5GHzChannelLow) * 5;
  } else {
    ret_value = false;
  }
  return ret_value;
}

bool Config::ValidateSsid(Error* error, const string& value) {
  if (value.length() < kSsidMinLength || value.length() > kSsidMaxLength) {
    Error::PopulateAndLog(
        error,
        Error::kInvalidArguments,
        base::StringPrintf("SSID must contain between %d and %d characters",
                           kSsidMinLength, kSsidMaxLength),
        FROM_HERE);
    return false;
  }
  return true;
}

bool Config::ValidateSecurityMode(Error* error, const string& value) {
  if (value != kSecurityModeNone && value != kSecurityModeRSN) {
    Error::PopulateAndLog(
        error,
        Error::kInvalidArguments,
        base::StringPrintf("Invalid/unsupported security mode [%s]",
                           value.c_str()),
        FROM_HERE);
    return false;
  }
  return true;
}

bool Config::ValidatePassphrase(Error* error, const string& value) {
  if (value.length() < kPassphraseMinLength ||
      value.length() > kPassphraseMaxLength) {
    Error::PopulateAndLog(
        error,
        Error::kInvalidArguments,
        base::StringPrintf("Passphrase must contain between %d and %d characters",
                           kPassphraseMinLength, kPassphraseMaxLength),
        FROM_HERE);

    return false;
  }
  return true;
}

bool Config::ValidateHwMode(Error* error, const string& value) {
  if (value != kHwMode80211a && value != kHwMode80211b &&
      value != kHwMode80211g && value != kHwMode80211n &&
      value != kHwMode80211ac) {
    Error::PopulateAndLog(
        error,
        Error::kInvalidArguments,
        base::StringPrintf("Invalid HW mode [%s]", value.c_str()),
        FROM_HERE);
    return false;
  }
  return true;
}

bool Config::ValidateOperationMode(Error* error, const string& value) {
  if (value != kOperationModeServer && value != kOperationModeBridge) {
    Error::PopulateAndLog(
        error,
        Error::kInvalidArguments,
        base::StringPrintf("Invalid operation mode [%s]", value.c_str()),
        FROM_HERE);
    return false;
  }
  return true;
}

bool Config::ValidateChannel(Error* error, const uint16_t& value) {
  if ((value >= kBand24GHzChannelLow && value <= kBand24GHzChannelHigh) ||
      (value >= kBand5GHzChannelLow && value <= kBand5GHzChannelHigh)) {
    return true;
  }
  Error::PopulateAndLog(error,
                        Error::kInvalidArguments,
                        base::StringPrintf("Invalid channel [%d]", value),
                        FROM_HERE);
  return false;
}

bool Config::GenerateConfigFile(Error* error, string* config_str) {
  // SSID.
  string ssid = GetSsid();
  if (ssid.empty()) {
    Error::PopulateAndLog(error,
                          Error::kInvalidConfiguration,
                          "SSID not specified",
                          FROM_HERE);
    return false;
  }
  base::StringAppendF(
      config_str, "%s=%s\n", kHostapdConfigKeySsid, ssid.c_str());

  // Bridge interface is required for bridge mode operation.
  if (GetOperationMode() == kOperationModeBridge) {
    if (GetBridgeInterface().empty()) {
      Error::PopulateAndLog(
          error,
          Error::kInvalidConfiguration,
          "Bridge interface not specified, required for bridge mode",
          FROM_HERE);
      return false;
    }
    base::StringAppendF(config_str,
                        "%s=%s\n",
                        kHostapdConfigKeyBridgeInterface,
                        GetBridgeInterface().c_str());
  }

  // Channel.
  base::StringAppendF(
      config_str, "%s=%d\n", kHostapdConfigKeyChannel, GetChannel());

  // Interface.
  if (!AppendInterface(error, config_str)) {
    return false;
  }

  // Hardware mode.
  if (!AppendHwMode(error, config_str)) {
    return false;
  }

  // Security mode configurations.
  if (!AppendSecurityMode(error, config_str)) {
    return false;
  }

  // Control interface.
  if (!control_interface_.empty()) {
    base::StringAppendF(config_str,
                        "%s=%s\n",
                        kHostapdConfigKeyControlInterface,
                        control_interface_.c_str());
    base::StringAppendF(config_str,
                        "%s=%s\n",
                        kHostapdConfigKeyControlInterfaceGroup,
                        Daemon::kAPManagerGroupName);
  }

  // Hostapd default configurations.
  if (!AppendHostapdDefaults(error, config_str)) {
    return false;
  }

  return true;
}

bool Config::ClaimDevice() {
  if (!device_) {
    LOG(ERROR) << "Failed to claim device: device doesn't exist.";
    return false;
  }
  return device_->ClaimDevice(GetFullDeviceControl());
}

bool Config::ReleaseDevice() {
  if (!device_) {
    LOG(ERROR) << "Failed to release device: device doesn't exist.";
    return false;
  }
  return device_->ReleaseDevice();
}

void Config::SetSsid(const string& ssid) {
  adaptor_->SetSsid(ssid);
}

string Config::GetSsid() const {
  return adaptor_->GetSsid();
}

void Config::SetInterfaceName(const std::string& interface_name) {
  adaptor_->SetInterfaceName(interface_name);
}

string Config::GetInterfaceName() const {
  return adaptor_->GetInterfaceName();
}

void Config::SetSecurityMode(const std::string& mode) {
  adaptor_->SetSecurityMode(mode);
}

string Config::GetSecurityMode() const {
  return adaptor_->GetSecurityMode();
}

void Config::SetPassphrase(const std::string& passphrase) {
  adaptor_->SetPassphrase(passphrase);
}

string Config::GetPassphrase() const {
  return adaptor_->GetPassphrase();
}

void Config::SetHwMode(const std::string& hw_mode) {
  adaptor_->SetHwMode(hw_mode);
}

string Config::GetHwMode() const {
  return adaptor_->GetHwMode();
}

void Config::SetOperationMode(const std::string& op_mode) {
  adaptor_->SetOperationMode(op_mode);
}

string Config::GetOperationMode() const {
  return adaptor_->GetOperationMode();
}

void Config::SetChannel(uint16_t channel) {
  adaptor_->SetChannel(channel);
}

uint16_t Config::GetChannel() const {
  return adaptor_->GetChannel();
}

void Config::SetHiddenNetwork(bool hidden_network) {
  adaptor_->SetHiddenNetwork(hidden_network);
}

bool Config::GetHiddenNetwork() const {
  return adaptor_->GetHiddenNetwork();
}

void Config::SetBridgeInterface(const std::string& interface_name) {
  adaptor_->SetBridgeInterface(interface_name);
}

string Config::GetBridgeInterface() const {
  return adaptor_->GetBridgeInterface();
}

void Config::SetServerAddressIndex(uint16_t index) {
  adaptor_->SetServerAddressIndex(index);
}

uint16_t Config::GetServerAddressIndex() const {
  return adaptor_->GetServerAddressIndex();
}

void Config::SetFullDeviceControl(bool full_control) {
  adaptor_->SetFullDeviceControl(full_control);
}

bool Config::GetFullDeviceControl() const {
  return adaptor_->GetFullDeviceControl();
}

bool Config::AppendHwMode(Error* error, string* config_str) {
  string hw_mode = GetHwMode();
  string hostapd_hw_mode;
  if (hw_mode == kHwMode80211a) {
    hostapd_hw_mode = kHostapdHwMode80211a;
  } else if (hw_mode == kHwMode80211b) {
    hostapd_hw_mode = kHostapdHwMode80211b;
  } else if (hw_mode == kHwMode80211g) {
    hostapd_hw_mode = kHostapdHwMode80211g;
  } else if (hw_mode == kHwMode80211n) {
    // Use 802.11a for 5GHz channel and 802.11g for 2.4GHz channel
    if (GetChannel() >= 34) {
      hostapd_hw_mode = kHostapdHwMode80211a;
    } else {
      hostapd_hw_mode = kHostapdHwMode80211g;
    }
    base::StringAppendF(config_str, "%s=1\n", kHostapdConfigKeyIeee80211n);

    // Get HT Capability.
    string ht_cap;
    if (!device_->GetHTCapability(GetChannel(), &ht_cap)) {
      Error::PopulateAndLog(error,
                            Error::kInvalidConfiguration,
                            "Failed to get HT Capability",
                            FROM_HERE);
      return false;
    }
    base::StringAppendF(config_str, "%s=%s\n",
                        kHostapdConfigKeyHTCapability,
                        ht_cap.c_str());
  } else if (hw_mode == kHwMode80211ac) {
    if (GetChannel() >= 34) {
      hostapd_hw_mode = kHostapdHwMode80211a;
    } else {
      hostapd_hw_mode = kHostapdHwMode80211g;
    }
    base::StringAppendF(config_str, "%s=1\n", kHostapdConfigKeyIeee80211ac);

    // TODO(zqiu): Determine VHT Capabilities based on the interface PHY's
    // capababilites.
  } else {
    Error::PopulateAndLog(
        error,
        Error::kInvalidConfiguration,
        base::StringPrintf("Invalid hardware mode: %s", hw_mode.c_str()),
        FROM_HERE);
    return false;
  }

  base::StringAppendF(
      config_str, "%s=%s\n", kHostapdConfigKeyHwMode, hostapd_hw_mode.c_str());
  return true;
}

bool Config::AppendHostapdDefaults(Error* error, string* config_str) {
  // Driver: NL80211.
  base::StringAppendF(
      config_str, "%s=%s\n", kHostapdConfigKeyDriver, kHostapdDefaultDriver);

  // Fragmentation threshold: disabled.
  base::StringAppendF(config_str,
                      "%s=%d\n",
                      kHostapdConfigKeyFragmThreshold,
                      kHostapdDefaultFragmThreshold);

  // RTS threshold: disabled.
  base::StringAppendF(config_str,
                      "%s=%d\n",
                      kHostapdConfigKeyRtsThreshold,
                      kHostapdDefaultRtsThreshold);

  return true;
}

bool Config::AppendInterface(Error* error, string* config_str) {
  string interface = GetInterfaceName();
  if (interface.empty()) {
    // Ask manager for unused ap capable device.
    device_ = manager_->GetAvailableDevice();
    if (!device_) {
      Error::PopulateAndLog(
          error, Error::kInternalError, "No device available", FROM_HERE);
      return false;
    }
  } else {
    device_ = manager_->GetDeviceFromInterfaceName(interface);
    if (!device_) {
      Error::PopulateAndLog(
          error,
          Error::kInvalidConfiguration,
          base::StringPrintf(
              "Unable to find device for the specified interface [%s]",
              interface.c_str()),
          FROM_HERE);
      return false;
    }
    if (device_->GetInUse()) {
      Error::PopulateAndLog(
          error,
          Error::kInvalidConfiguration,
          base::StringPrintf("Device [%s] for interface [%s] already in use",
                             device_->GetDeviceName().c_str(),
                             interface.c_str()),
          FROM_HERE);
      return false;
    }
  }

  // Use the preferred AP interface from the device.
  selected_interface_ = device_->GetPreferredApInterface();
  base::StringAppendF(config_str,
                      "%s=%s\n",
                      kHostapdConfigKeyInterface,
                      selected_interface_.c_str());
  return true;
}

bool Config::AppendSecurityMode(Error* error, string* config_str) {
  string security_mode = GetSecurityMode();
  if (security_mode == kSecurityModeNone) {
    // Nothing need to be done for open network.
    return true;
  }

  if (security_mode == kSecurityModeRSN) {
    string passphrase = GetPassphrase();
    if (passphrase.empty()) {
      Error::PopulateAndLog(
          error,
          Error::kInvalidConfiguration,
          base::StringPrintf("Passphrase not set for security mode: %s",
                             security_mode.c_str()),
          FROM_HERE);
      return false;
    }

    base::StringAppendF(config_str, "%s=2\n", kHostapdConfigKeyWpa);
    base::StringAppendF(config_str,
                        "%s=%s\n",
                        kHostapdConfigKeyRsnPairwise,
                        kHostapdDefaultRsnPairwise);
    base::StringAppendF(config_str,
                        "%s=%s\n",
                        kHostapdConfigKeyWpaKeyMgmt,
                        kHostapdDefaultWpaKeyMgmt);
    base::StringAppendF(config_str,
                        "%s=%s\n",
                        kHostapdConfigKeyWpaPassphrase,
                        passphrase.c_str());
    return true;
  }

  Error::PopulateAndLog(
      error,
      Error::kInvalidConfiguration,
      base::StringPrintf("Invalid security mode: %s", security_mode.c_str()),
      FROM_HERE);
  return false;
}

}  // namespace apmanager
