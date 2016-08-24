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

#ifndef APMANAGER_DEVICE_INFO_H_
#define APMANAGER_DEVICE_INFO_H_

#include <map>
#include <string>

#include <base/callback.h>
#include <base/files/file_path.h>
#include <base/memory/ref_counted.h>
#include <base/memory/weak_ptr.h>
#include <gtest/gtest_prod.h>  // for FRIEND_TEST

#include "apmanager/device.h"

namespace shill {

class NetlinkManager;
class Nl80211Message;
class RTNLHandler;
class RTNLMessage;
class RTNLListener;

}  // namespace shill

namespace apmanager {

class ControlInterface;
class Manager;

// DeviceInfo will enumerate WiFi devices (PHYs) during startup and on-demand
// (when new interface is detected but the corresponding device is not
// enumerated). And use RTNL to monitor creation/deletion of WiFi interfaces.
class DeviceInfo : public base::SupportsWeakPtr<DeviceInfo> {
 public:
  explicit DeviceInfo(Manager* manager);
  virtual ~DeviceInfo();

  // Start and stop device detection monitoring.
  void Start();
  void Stop();

 private:
  friend class DeviceInfoTest;

  static const char kDeviceInfoRoot[];
  static const char kInterfaceUevent[];
  static const char kInterfaceUeventWifiSignature[];

  // Use nl80211 to enumerate available WiFi PHYs.
  void EnumerateDevices();
  void OnWiFiPhyInfoReceived(const shill::Nl80211Message& msg);

  // Handler for RTNL link event.
  void LinkMsgHandler(const shill::RTNLMessage& msg);
  void AddLinkMsgHandler(const std::string& iface_name, int iface_index);
  void DelLinkMsgHandler(const std::string& iface_name, int iface_index);

  // Return true if the specify |iface_name| is a wifi interface, false
  // otherwise.
  bool IsWifiInterface(const std::string& iface_name);

  // Return the contents of the device info file |path_name| for interface
  // |iface_name| in output parameter |contents_out|. Return true if file
  // read succeed, fales otherwise.
  bool GetDeviceInfoContents(const std::string& iface_name,
                             const std::string& path_name,
                             std::string* contents_out);

  // Use nl80211 to get WiFi interface information for interface on
  // |iface_index|.
  void GetWiFiInterfaceInfo(int iface_index);
  void OnWiFiInterfaceInfoReceived(const shill::Nl80211Message& msg);

  // Use nl80211 to get PHY info for interface on |iface_index|.
  void GetWiFiInterfacePhyInfo(uint32_t iface_index);
  void OnWiFiInterfacePhyInfoReceived(
      uint32_t iface_index, const shill::Nl80211Message& msg);

  scoped_refptr<Device> GetDevice(const std::string& phy_name);
  void RegisterDevice(scoped_refptr<Device> device);

  // Maps interface index to interface info
  std::map<uint32_t, Device::WiFiInterface> interface_infos_;
  // Maps device name to device object. Each device object represents a PHY.
  std::map<std::string, scoped_refptr<Device>> devices_;

  // RTNL link event callback and listener.
  base::Callback<void(const shill::RTNLMessage&)> link_callback_;
  std::unique_ptr<shill::RTNLListener> link_listener_;

  base::FilePath device_info_root_;
  Manager* manager_;

  // Cache copy of singleton pointers.
  shill::NetlinkManager* netlink_manager_;
  shill::RTNLHandler* rtnl_handler_;

  int device_identifier_;

  DISALLOW_COPY_AND_ASSIGN(DeviceInfo);
};

}  // namespace apmanager

#endif  // APMANAGER_DEVICE_INFO_H_
