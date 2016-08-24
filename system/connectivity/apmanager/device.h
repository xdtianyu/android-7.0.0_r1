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

#ifndef APMANAGER_DEVICE_H_
#define APMANAGER_DEVICE_H_

#include <set>
#include <string>
#include <vector>

#include <base/macros.h>
#include <base/memory/ref_counted.h>
#include <shill/net/byte_string.h>
#include <shill/net/nl80211_message.h>

#include "apmanager/device_adaptor_interface.h"

namespace apmanager {

class ControlInterface;
class Manager;

// Abstraction for WiFi Device (PHY). Each device can have one or more
// interfaces defined on it.
class Device : public base::RefCounted<Device> {
 public:
  struct WiFiInterface {
    WiFiInterface() : iface_index(0), iface_type(0) {}
    WiFiInterface(const std::string& in_iface_name,
                  const std::string& in_device_name,
                  uint32_t in_iface_index,
                  uint32_t in_iface_type)
        : iface_name(in_iface_name),
          device_name(in_device_name),
          iface_index(in_iface_index),
          iface_type(in_iface_type) {}
    std::string iface_name;
    std::string device_name;
    uint32_t iface_index;
    uint32_t iface_type;
    bool Equals(const WiFiInterface& other) const {
      return this->iface_name == other.iface_name &&
             this->device_name == other.device_name &&
             this->iface_index == other.iface_index &&
             this->iface_type == other.iface_type;
    }
  };

  struct BandCapability {
    std::vector<uint32_t> frequencies;
    uint16_t ht_capability_mask;
    uint16_t vht_capability_mask;
  };

  Device(Manager* manager,
         const std::string& device_name,
         int identifier);
  virtual ~Device();

  // Register/deregister WiFi interface on this device.
  virtual void RegisterInterface(const WiFiInterface& interface);
  virtual void DeregisterInterface(const WiFiInterface& interface);

  // Parse device capability from NL80211 message.
  void ParseWiphyCapability(const shill::Nl80211Message& msg);

  // Claim ownership of this device for AP operation. When |full_control| is
  // set to true, this will claim all interfaces reside on this device.
  // When it is set to false, this will only claim the interface used for AP
  // operation.
  virtual bool ClaimDevice(bool full_control);
  // Release any claimed interfaces.
  virtual bool ReleaseDevice();

  // Return true if interface with |interface_name| resides on this device,
  // false otherwise.
  virtual bool InterfaceExists(const std::string& interface_name);

  // Get HT and VHT capability string based on the operating channel.
  // Return true and set the output capability string if such capability
  // exist for the band the given |channel| is in, false otherwise.
  virtual bool GetHTCapability(uint16_t channel, std::string* ht_cap);
  virtual bool GetVHTCapability(uint16_t channel, std::string* vht_cap);

  void SetDeviceName(const std::string& device_name);
  std::string GetDeviceName() const;
  void SetPreferredApInterface(const std::string& interface_name);
  std::string GetPreferredApInterface() const;
  void SetInUse(bool in_use);
  bool GetInUse() const;

  int identifier() const { return identifier_; }

 private:
  friend class DeviceTest;

  // Get the HT secondary channel location base on the primary channel.
  // Return true and set the output |above| flag if channel is valid,
  // otherwise return false.
  static bool GetHTSecondaryChannelLocation(uint16_t channel, bool* above);

  // Determine preferred interface to used for AP operation based on the list
  // of interfaces reside on this device
  void UpdatePreferredAPInterface();

  // Get the capability for the band the given |channel| is in. Return true
  // and set the output |capability| pointer if such capability exist for the
  // band the given |channel| is in, false otherwise.
  bool GetBandCapability(uint16_t channel, BandCapability* capability);

  Manager* manager_;

  // List of WiFi interfaces live on this device (PHY).
  std::vector<WiFiInterface> interface_list_;

  // Flag indicating if this device supports AP mode interface or not.
  bool supports_ap_mode_;

  // Wiphy band capabilities.
  std::vector<BandCapability> band_capability_;

  // List of claimed interfaces.
  std::set<std::string> claimed_interfaces_;

  // Unique device identifier.
  int identifier_;

  // Adaptor for communicating with remote clients.
  std::unique_ptr<DeviceAdaptorInterface> adaptor_;

  DISALLOW_COPY_AND_ASSIGN(Device);
};

}  // namespace apmanager

#endif  // APMANAGER_DEVICE_H_
