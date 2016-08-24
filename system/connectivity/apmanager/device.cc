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

#include "apmanager/device.h"

#include <base/strings/stringprintf.h>
#include <brillo/strings/string_utils.h>
#include <shill/net/attribute_list.h>
#include <shill/net/ieee80211.h>

#include "apmanager/config.h"
#include "apmanager/control_interface.h"
#include "apmanager/manager.h"

using shill::ByteString;
using std::string;

namespace apmanager {

Device::Device(Manager* manager,
               const string& device_name,
               int identifier)
    : manager_(manager),
      supports_ap_mode_(false),
      identifier_(identifier),
      adaptor_(manager->control_interface()->CreateDeviceAdaptor(this)) {
  SetDeviceName(device_name);
  SetInUse(false);
}

Device::~Device() {}

void Device::RegisterInterface(const WiFiInterface& new_interface) {
  LOG(INFO) << "RegisteringInterface " << new_interface.iface_name
            << " on device " << GetDeviceName();
  for (const auto& interface : interface_list_) {
    // Done if interface already in the list.
    if (interface.iface_index == new_interface.iface_index) {
      LOG(INFO) << "Interface " << new_interface.iface_name
                << " already registered.";
      return;
    }
  }
  interface_list_.push_back(new_interface);
  UpdatePreferredAPInterface();
}

void Device::DeregisterInterface(const WiFiInterface& interface) {
  LOG(INFO) << "DeregisteringInterface " << interface.iface_name
            << " on device " << GetDeviceName();
  for (auto it = interface_list_.begin(); it != interface_list_.end(); ++it) {
    if (it->iface_index == interface.iface_index) {
      interface_list_.erase(it);
      UpdatePreferredAPInterface();
      return;
    }
  }
}

void Device::ParseWiphyCapability(const shill::Nl80211Message& msg) {
  // Parse NL80211_ATTR_SUPPORTED_IFTYPES for AP mode interface support.
  shill::AttributeListConstRefPtr supported_iftypes;
  if (!msg.const_attributes()->ConstGetNestedAttributeList(
      NL80211_ATTR_SUPPORTED_IFTYPES, &supported_iftypes)) {
    LOG(ERROR) << "NL80211_CMD_NEW_WIPHY had no NL80211_ATTR_SUPPORTED_IFTYPES";
    return;
  }
  supported_iftypes->GetFlagAttributeValue(NL80211_IFTYPE_AP,
                                           &supports_ap_mode_);

  // Parse WiFi band capabilities.
  shill::AttributeListConstRefPtr wiphy_bands;
  if (!msg.const_attributes()->ConstGetNestedAttributeList(
      NL80211_ATTR_WIPHY_BANDS, &wiphy_bands)) {
    LOG(ERROR) << "NL80211_CMD_NEW_WIPHY had no NL80211_ATTR_WIPHY_BANDS";
    return;
  }

  shill::AttributeIdIterator band_iter(*wiphy_bands);
  for (; !band_iter.AtEnd(); band_iter.Advance()) {
    BandCapability band_cap;

    shill::AttributeListConstRefPtr wiphy_band;
    if (!wiphy_bands->ConstGetNestedAttributeList(band_iter.GetId(),
                                                  &wiphy_band)) {
      LOG(WARNING) << "WiFi band " << band_iter.GetId() << " not found";
      continue;
    }

    // ...Each band has a FREQS attribute...
    shill::AttributeListConstRefPtr frequencies;
    if (!wiphy_band->ConstGetNestedAttributeList(NL80211_BAND_ATTR_FREQS,
                                                 &frequencies)) {
      LOG(ERROR) << "BAND " << band_iter.GetId()
                 << " had no 'frequencies' attribute";
      continue;
    }

    // ...And each FREQS attribute contains an array of information about the
    // frequency...
    shill::AttributeIdIterator freq_iter(*frequencies);
    for (; !freq_iter.AtEnd(); freq_iter.Advance()) {
      shill::AttributeListConstRefPtr frequency;
      if (frequencies->ConstGetNestedAttributeList(freq_iter.GetId(),
                                                   &frequency)) {
        // ...Including the frequency, itself (the part we want).
        uint32_t frequency_value = 0;
        if (frequency->GetU32AttributeValue(NL80211_FREQUENCY_ATTR_FREQ,
                                            &frequency_value)) {
          band_cap.frequencies.push_back(frequency_value);
        }
      }
    }

    wiphy_band->GetU16AttributeValue(NL80211_BAND_ATTR_HT_CAPA,
                                     &band_cap.ht_capability_mask);
    wiphy_band->GetU16AttributeValue(NL80211_BAND_ATTR_VHT_CAPA,
                                     &band_cap.vht_capability_mask);
    band_capability_.push_back(band_cap);
  }
}

bool Device::ClaimDevice(bool full_control) {
  if (GetInUse()) {
    LOG(ERROR) << "Failed to claim device [" << GetDeviceName()
               << "]: already in used.";
    return false;
  }

  if (full_control) {
    for (const auto& interface : interface_list_) {
      manager_->ClaimInterface(interface.iface_name);
      claimed_interfaces_.insert(interface.iface_name);
    }
  } else {
    manager_->ClaimInterface(GetPreferredApInterface());
    claimed_interfaces_.insert(GetPreferredApInterface());
  }
  SetInUse(true);
  return true;
}

bool Device::ReleaseDevice() {
  if (!GetInUse()) {
    LOG(ERROR) << "Failed to release device [" << GetDeviceName()
               << "]: not currently in-used.";
    return false;
  }

  for (const auto& interface : claimed_interfaces_) {
    manager_->ReleaseInterface(interface);
  }
  claimed_interfaces_.clear();
  SetInUse(false);
  return true;
}

bool Device::InterfaceExists(const string& interface_name) {
  for (const auto& interface : interface_list_) {
    if (interface.iface_name == interface_name) {
      return true;
    }
  }
  return false;
}

bool Device::GetHTCapability(uint16_t channel, string* ht_cap) {
  // Get the band capability based on the channel.
  BandCapability band_cap;
  if (!GetBandCapability(channel, &band_cap)) {
    LOG(ERROR) << "No band capability found for channel " << channel;
    return false;
  }

  std::vector<string> ht_capability;
  // LDPC coding capability.
  if (band_cap.ht_capability_mask & shill::IEEE_80211::kHTCapMaskLdpcCoding) {
    ht_capability.push_back("LDPC");
  }

  // Supported channel width set.
  if (band_cap.ht_capability_mask &
      shill::IEEE_80211::kHTCapMaskSupWidth2040) {
    // Determine secondary channel is below or above the primary.
    bool above = false;
    if (!GetHTSecondaryChannelLocation(channel, &above)) {
      LOG(ERROR) << "Unable to determine secondary channel location for "
                 << "channel " << channel;
      return false;
    }
    if (above) {
      ht_capability.push_back("HT40+");
    } else {
      ht_capability.push_back("HT40-");
    }
  }

  // Spatial Multiplexing (SM) Power Save.
  uint16_t power_save_mask =
      (band_cap.ht_capability_mask >>
          shill::IEEE_80211::kHTCapMaskSmPsShift) & 0x3;
  if (power_save_mask == 0) {
    ht_capability.push_back("SMPS-STATIC");
  } else if (power_save_mask == 1) {
    ht_capability.push_back("SMPS-DYNAMIC");
  }

  // HT-greenfield.
  if (band_cap.ht_capability_mask & shill::IEEE_80211::kHTCapMaskGrnFld) {
    ht_capability.push_back("GF");
  }

  // Short GI for 20 MHz.
  if (band_cap.ht_capability_mask & shill::IEEE_80211::kHTCapMaskSgi20) {
    ht_capability.push_back("SHORT-GI-20");
  }

  // Short GI for 40 MHz.
  if (band_cap.ht_capability_mask & shill::IEEE_80211::kHTCapMaskSgi40) {
    ht_capability.push_back("SHORT-GI-40");
  }

  // Tx STBC.
  if (band_cap.ht_capability_mask & shill::IEEE_80211::kHTCapMaskTxStbc) {
    ht_capability.push_back("TX-STBC");
  }

  // Rx STBC.
  uint16_t rx_stbc =
      (band_cap.ht_capability_mask >>
          shill::IEEE_80211::kHTCapMaskRxStbcShift) & 0x3;
  if (rx_stbc == 1) {
    ht_capability.push_back("RX-STBC1");
  } else if (rx_stbc == 2) {
    ht_capability.push_back("RX-STBC12");
  } else if (rx_stbc == 3) {
    ht_capability.push_back("RX-STBC123");
  }

  // HT-delayed Block Ack.
  if (band_cap.ht_capability_mask & shill::IEEE_80211::kHTCapMaskDelayBA) {
    ht_capability.push_back("DELAYED-BA");
  }

  // Maximum A-MSDU length.
  if (band_cap.ht_capability_mask & shill::IEEE_80211::kHTCapMaskMaxAmsdu) {
    ht_capability.push_back("MAX-AMSDU-7935");
  }

  // DSSS/CCK Mode in 40 MHz.
  if (band_cap.ht_capability_mask & shill::IEEE_80211::kHTCapMaskDsssCck40) {
    ht_capability.push_back("DSSS_CCK-40");
  }

  // 40 MHz intolerant.
  if (band_cap.ht_capability_mask &
      shill::IEEE_80211::kHTCapMask40MHzIntolerant) {
    ht_capability.push_back("40-INTOLERANT");
  }

  *ht_cap = base::StringPrintf("[%s]",
      brillo::string_utils::Join(" ", ht_capability).c_str());
  return true;
}

bool Device::GetVHTCapability(uint16_t channel, string* vht_cap) {
  // TODO(zqiu): to be implemented.
  return false;
}

void Device::SetDeviceName(const std::string& device_name) {
  adaptor_->SetDeviceName(device_name);
}

string Device::GetDeviceName() const {
  return adaptor_->GetDeviceName();
}

void Device::SetPreferredApInterface(const std::string& interface_name) {
  adaptor_->SetPreferredApInterface(interface_name);
}

string Device::GetPreferredApInterface() const {
  return adaptor_->GetPreferredApInterface();
}

void Device::SetInUse(bool in_use) {
  return adaptor_->SetInUse(in_use);
}

bool Device::GetInUse() const {
  return adaptor_->GetInUse();
}

// static
bool Device::GetHTSecondaryChannelLocation(uint16_t channel, bool* above) {
  bool ret_val = true;

  // Determine secondary channel location base on the channel. Refer to
  // ht_cap section in hostapd.conf documentation.
  switch (channel) {
    case 7:
    case 8:
    case 9:
    case 10:
    case 11:
    case 12:
    case 13:
    case 40:
    case 48:
    case 56:
    case 64:
      *above = false;
      break;

    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 36:
    case 44:
    case 52:
    case 60:
      *above = true;
      break;

    default:
      ret_val = false;
      break;
  }

  return ret_val;
}

bool Device::GetBandCapability(uint16_t channel, BandCapability* capability) {
  uint32_t frequency;
  if (!Config::GetFrequencyFromChannel(channel, &frequency)) {
    LOG(ERROR) << "Invalid channel " << channel;
    return false;
  }

  for (const auto& band : band_capability_) {
    if (std::find(band.frequencies.begin(),
                  band.frequencies.end(),
                  frequency) != band.frequencies.end()) {
      *capability = band;
      return true;
    }
  }
  return false;
}

void Device::UpdatePreferredAPInterface() {
  // Return if device doesn't support AP interface mode.
  if (!supports_ap_mode_) {
    return;
  }

  // Use the first registered AP mode interface if there is one, otherwise use
  // the first registered managed mode interface. If none are available, then
  // no interface can be used for AP operation on this device.
  WiFiInterface preferred_interface;
  for (const auto& interface : interface_list_) {
    if (interface.iface_type == NL80211_IFTYPE_AP) {
      preferred_interface = interface;
      break;
    } else if (interface.iface_type == NL80211_IFTYPE_STATION &&
               preferred_interface.iface_name.empty()) {
      preferred_interface = interface;
    }
    // Ignore all other interface types.
  }
  // Update preferred AP interface property.
  SetPreferredApInterface(preferred_interface.iface_name);
}

}  // namespace apmanager
