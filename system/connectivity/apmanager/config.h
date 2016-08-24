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

#ifndef APMANAGER_CONFIG_H_
#define APMANAGER_CONFIG_H_

#include <memory>
#include <string>

#include <base/macros.h>
#include <base/memory/ref_counted.h>
#include <brillo/errors/error.h>

#include "apmanager/config_adaptor_interface.h"

namespace apmanager {

class Error;
class Device;
class Manager;

class Config {
 public:
  Config(Manager* manager, int service_identifier);
  virtual ~Config();

  bool ValidateSsid(Error* error, const std::string& value);
  bool ValidateSecurityMode(Error* error, const std::string& value);
  bool ValidatePassphrase(Error* error, const std::string& value);
  bool ValidateHwMode(Error* error, const std::string& value);
  bool ValidateOperationMode(Error* error, const std::string& value);
  bool ValidateChannel(Error* error, const uint16_t& value);

  // Calculate the frequency based on the given |channel|. Return true and set
  // the output |frequency| if is valid channel, false otherwise.
  static bool GetFrequencyFromChannel(uint16_t channel, uint32_t* freq);

  // Generate a config file string for a hostapd instance. Populate
  // |error| when encounter invalid configuration. Return true if success,
  // false otherwise.
  virtual bool GenerateConfigFile(Error* error, std::string* config_str);

  // Claim and release the device needed for this configuration.
  virtual bool ClaimDevice();
  virtual bool ReleaseDevice();

  // Getter and setter for configuration properties.
  void SetSsid(const std::string& ssid);
  std::string GetSsid() const;
  void SetInterfaceName(const std::string& interface_name);
  std::string GetInterfaceName() const;
  void SetSecurityMode(const std::string& security_mode);
  std::string GetSecurityMode() const;
  void SetPassphrase(const std::string& passphrase);
  std::string GetPassphrase() const;
  void SetHwMode(const std::string& hw_mode);
  std::string GetHwMode() const;
  void SetOperationMode(const std::string& op_mode);
  std::string GetOperationMode() const;
  void SetChannel(uint16_t channel);
  uint16_t GetChannel() const;
  void SetHiddenNetwork(bool hidden);
  bool GetHiddenNetwork() const;
  void SetBridgeInterface(const std::string& interface_name);
  std::string GetBridgeInterface() const;
  void SetServerAddressIndex(uint16_t);
  uint16_t GetServerAddressIndex() const;
  void SetFullDeviceControl(bool full_control);
  bool GetFullDeviceControl() const;

  const std::string& control_interface() const { return control_interface_; }
  void set_control_interface(const std::string& control_interface) {
    control_interface_ = control_interface;
  }

  const std::string& selected_interface() const { return selected_interface_; }

  ConfigAdaptorInterface* adaptor() const { return adaptor_.get(); }

 private:
  // Keys used in hostapd config file.
  static const char kHostapdConfigKeyBridgeInterface[];
  static const char kHostapdConfigKeyChannel[];
  static const char kHostapdConfigKeyControlInterface[];
  static const char kHostapdConfigKeyControlInterfaceGroup[];
  static const char kHostapdConfigKeyDriver[];
  static const char kHostapdConfigKeyFragmThreshold[];
  static const char kHostapdConfigKeyHTCapability[];
  static const char kHostapdConfigKeyHwMode[];
  static const char kHostapdConfigKeyIeee80211ac[];
  static const char kHostapdConfigKeyIeee80211n[];
  static const char kHostapdConfigKeyIgnoreBroadcastSsid[];
  static const char kHostapdConfigKeyInterface[];
  static const char kHostapdConfigKeyRsnPairwise[];
  static const char kHostapdConfigKeyRtsThreshold[];
  static const char kHostapdConfigKeySsid[];
  static const char kHostapdConfigKeyWepDefaultKey[];
  static const char kHostapdConfigKeyWepKey0[];
  static const char kHostapdConfigKeyWpa[];
  static const char kHostapdConfigKeyWpaKeyMgmt[];
  static const char kHostapdConfigKeyWpaPassphrase[];

  // Hardware mode value for hostapd config file.
  static const char kHostapdHwMode80211a[];
  static const char kHostapdHwMode80211b[];
  static const char kHostapdHwMode80211g[];

  // Default hostapd configuration values. User will not be able to configure
  // these.
  static const char kHostapdDefaultDriver[];
  static const char kHostapdDefaultRsnPairwise[];
  static const char kHostapdDefaultWpaKeyMgmt[];
  static const int kHostapdDefaultFragmThreshold;
  static const int kHostapdDefaultRtsThreshold;

  // Default config property values.
  static const uint16_t kPropertyDefaultChannel;;
  static const bool kPropertyDefaultHiddenNetwork;
  static const uint16_t kPropertyDefaultServerAddressIndex;

  // Constants use for converting channel to frequency.
  static const uint16_t kBand24GHzChannelLow;
  static const uint16_t kBand24GHzChannelHigh;
  static const uint32_t kBand24GHzBaseFrequency;
  static const uint16_t kBand5GHzChannelLow;
  static const uint16_t kBand5GHzChannelHigh;
  static const uint16_t kBand5GHzBaseFrequency;

  static const int kSsidMinLength;
  static const int kSsidMaxLength;
  static const int kPassphraseMinLength;
  static const int kPassphraseMaxLength;

  // Append default hostapd configurations to the config file.
  bool AppendHostapdDefaults(Error* error, std::string* config_str);

  // Append hardware mode related configurations to the config file.
  bool AppendHwMode(Error* error, std::string* config_str);

  // Determine/append interface configuration to the config file.
  bool AppendInterface(Error* error, std::string* config_str);

  // Append security related configurations to the config file.
  bool AppendSecurityMode(Error* error, std::string* config_str);

  Manager* manager_;
  std::string control_interface_;
  // Interface selected for hostapd.
  std::string selected_interface_;
  scoped_refptr<Device> device_;

  std::unique_ptr<ConfigAdaptorInterface> adaptor_;

  DISALLOW_COPY_AND_ASSIGN(Config);
};

}  // namespace apmanager

#endif  // APMANAGER_CONFIG_H_
