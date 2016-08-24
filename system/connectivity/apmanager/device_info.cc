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

#include "apmanager/device_info.h"

#include <linux/rtnetlink.h>

#include <string>

#include <base/bind.h>
#include <base/files/file_util.h>
#include <base/logging.h>
#include <shill/net/ndisc.h>
#include <shill/net/netlink_attribute.h>
#include <shill/net/netlink_manager.h>
#include <shill/net/nl80211_message.h>
#include <shill/net/rtnl_handler.h>
#include <shill/net/rtnl_listener.h>
#include <shill/net/rtnl_message.h>

#include "apmanager/control_interface.h"
#include "apmanager/manager.h"

using base::Bind;
using shill::ByteString;
using shill::NetlinkManager;
using shill::NetlinkMessage;
using shill::Nl80211Message;
using shill::RTNLHandler;
using shill::RTNLMessage;
using shill::RTNLListener;
using std::map;
using std::string;

namespace apmanager {

const char DeviceInfo::kDeviceInfoRoot[] = "/sys/class/net";
const char DeviceInfo::kInterfaceUevent[] = "uevent";
const char DeviceInfo::kInterfaceUeventWifiSignature[] = "DEVTYPE=wlan\n";

DeviceInfo::DeviceInfo(Manager* manager)
    : link_callback_(Bind(&DeviceInfo::LinkMsgHandler, Unretained(this))),
      device_info_root_(kDeviceInfoRoot),
      manager_(manager),
      netlink_manager_(NetlinkManager::GetInstance()),
      rtnl_handler_(RTNLHandler::GetInstance()),
      device_identifier_(0) {
}

DeviceInfo::~DeviceInfo() {}

void DeviceInfo::Start() {
  // Start netlink manager.
  netlink_manager_->Init();
  uint16_t nl80211_family_id = netlink_manager_->GetFamily(
      Nl80211Message::kMessageTypeString,
      Bind(&Nl80211Message::CreateMessage));
  if (nl80211_family_id == NetlinkMessage::kIllegalMessageType) {
    LOG(FATAL) << "Didn't get a legal message type for 'nl80211' messages.";
  }
  Nl80211Message::SetMessageType(nl80211_family_id);
  netlink_manager_->Start();

  // Start enumerating WiFi devices (PHYs).
  EnumerateDevices();

  // Start RTNL for monitoring network interfaces.
  rtnl_handler_->Start(RTMGRP_LINK | RTMGRP_IPV4_IFADDR | RTMGRP_IPV4_ROUTE |
                       RTMGRP_IPV6_IFADDR | RTMGRP_IPV6_ROUTE |
                       RTMGRP_ND_USEROPT);
  link_listener_.reset(
      new RTNLListener(RTNLHandler::kRequestLink, link_callback_));
  // Request link infos.
  rtnl_handler_->RequestDump(RTNLHandler::kRequestLink);
}

void DeviceInfo::Stop() {
  link_listener_.reset();
}

void DeviceInfo::EnumerateDevices() {
  shill::GetWiphyMessage get_wiphy;
  get_wiphy.attributes()->SetFlagAttributeValue(NL80211_ATTR_SPLIT_WIPHY_DUMP,
                                                true);
  get_wiphy.AddFlag(NLM_F_DUMP);
  netlink_manager_->SendNl80211Message(
      &get_wiphy,
      Bind(&DeviceInfo::OnWiFiPhyInfoReceived, AsWeakPtr()),
      Bind(&NetlinkManager::OnAckDoNothing),
      Bind(&NetlinkManager::OnNetlinkMessageError));
}

void DeviceInfo::OnWiFiPhyInfoReceived(const shill::Nl80211Message& msg) {
  // Verify NL80211_CMD_NEW_WIPHY.
  if (msg.command() != shill::NewWiphyMessage::kCommand) {
    LOG(ERROR) << "Received unexpected command:"
               << msg.command();
    return;
  }

  string device_name;
  if (!msg.const_attributes()->GetStringAttributeValue(NL80211_ATTR_WIPHY_NAME,
                                                       &device_name)) {
    LOG(ERROR) << "NL80211_CMD_NEW_WIPHY had no NL80211_ATTR_WIPHY_NAME";
    return;
  }

  if (GetDevice(device_name)) {
    LOG(INFO) << "Device " << device_name << " already enumerated.";
    return;
  }

  scoped_refptr<Device> device =
      new Device(manager_, device_name, device_identifier_++);
  device->ParseWiphyCapability(msg);

  // Register device
  RegisterDevice(device);
}

void DeviceInfo::LinkMsgHandler(const RTNLMessage& msg) {
  DCHECK(msg.type() == RTNLMessage::kTypeLink);

  // Get interface name.
  if (!msg.HasAttribute(IFLA_IFNAME)) {
    LOG(ERROR) << "Link event message does not have IFLA_IFNAME!";
    return;
  }
  ByteString b(msg.GetAttribute(IFLA_IFNAME));
  string iface_name(reinterpret_cast<const char*>(b.GetConstData()));

  int dev_index = msg.interface_index();
  if (msg.mode() == RTNLMessage::kModeAdd) {
    AddLinkMsgHandler(iface_name, dev_index);
  } else if (msg.mode() == RTNLMessage::kModeDelete) {
    DelLinkMsgHandler(iface_name, dev_index);
  } else {
    NOTREACHED();
  }
}

void DeviceInfo::AddLinkMsgHandler(const string& iface_name, int iface_index) {
  // Ignore non-wifi interfaces.
  if (!IsWifiInterface(iface_name)) {
    LOG(INFO) << "Ignore link event for non-wifi interface: " << iface_name;
    return;
  }

  // Return if interface already existed. Could receive multiple add link event
  // for a single interface.
  if (interface_infos_.find(iface_index) != interface_infos_.end()) {
    LOG(INFO) << "AddLinkMsgHandler: interface " << iface_name
              << " is already added";
    return;
  }

  // Add interface.
  Device::WiFiInterface wifi_interface;
  wifi_interface.iface_name = iface_name;
  wifi_interface.iface_index = iface_index;
  interface_infos_[iface_index] = wifi_interface;

  // Get interface info.
  GetWiFiInterfaceInfo(iface_index);
}

void DeviceInfo::DelLinkMsgHandler(const string& iface_name, int iface_index) {
  LOG(INFO) << "DelLinkMsgHandler iface_name: " << iface_name
            << "iface_index: " << iface_index;
  map<uint32_t, Device::WiFiInterface>::iterator iter =
      interface_infos_.find(iface_index);
  if (iter != interface_infos_.end()) {
    // Deregister interface from the Device.
    scoped_refptr<Device> device = GetDevice(iter->second.device_name);
    if (device) {
      device->DeregisterInterface(iter->second);
    }
    interface_infos_.erase(iter);
  }
}

bool DeviceInfo::IsWifiInterface(const string& iface_name) {
  string contents;
  if (!GetDeviceInfoContents(iface_name, kInterfaceUevent, &contents)) {
    LOG(INFO) << "Interface " << iface_name << " has no uevent file";
    return false;
  }

  if (contents.find(kInterfaceUeventWifiSignature) == string::npos) {
    LOG(INFO) << "Interface " << iface_name << " is not a WiFi interface";
    return false;
  }

  return true;
}

bool DeviceInfo::GetDeviceInfoContents(const string& iface_name,
                                       const string& path_name,
                                       string* contents_out) {
  return base::ReadFileToString(
      device_info_root_.Append(iface_name).Append(path_name),
      contents_out);
}

void DeviceInfo::GetWiFiInterfaceInfo(int interface_index) {
  shill::GetInterfaceMessage msg;
  if (!msg.attributes()->SetU32AttributeValue(NL80211_ATTR_IFINDEX,
                                              interface_index)) {
    LOG(ERROR) << "Unable to set interface index attribute for "
                  "GetInterface message.  Interface type cannot be "
                  "determined!";
    return;
  }

  netlink_manager_->SendNl80211Message(
      &msg,
      Bind(&DeviceInfo::OnWiFiInterfaceInfoReceived, AsWeakPtr()),
      Bind(&NetlinkManager::OnAckDoNothing),
      Bind(&NetlinkManager::OnNetlinkMessageError));
}

void DeviceInfo::OnWiFiInterfaceInfoReceived(const shill::Nl80211Message& msg) {
  if (msg.command() != NL80211_CMD_NEW_INTERFACE) {
    LOG(ERROR) << "Message is not a new interface response";
    return;
  }

  uint32_t interface_index;
  if (!msg.const_attributes()->GetU32AttributeValue(NL80211_ATTR_IFINDEX,
                                                    &interface_index)) {
    LOG(ERROR) << "Message contains no interface index";
    return;
  }
  uint32_t interface_type;
  if (!msg.const_attributes()->GetU32AttributeValue(NL80211_ATTR_IFTYPE,
                                                    &interface_type)) {
    LOG(ERROR) << "Message contains no interface type";
    return;
  }

  map<uint32_t, Device::WiFiInterface>::iterator iter =
      interface_infos_.find(interface_index);
  if (iter == interface_infos_.end()) {
    LOG(ERROR) << "Receive WiFi interface info for non-exist interface: "
               << interface_index;
    return;
  }
  iter->second.iface_type = interface_type;

  // Request PHY info, to know which Device to register this interface to.
  GetWiFiInterfacePhyInfo(interface_index);
}

void DeviceInfo::GetWiFiInterfacePhyInfo(uint32_t iface_index) {
  shill::GetWiphyMessage get_wiphy;
  get_wiphy.attributes()->SetU32AttributeValue(NL80211_ATTR_IFINDEX,
                                               iface_index);
  netlink_manager_->SendNl80211Message(
      &get_wiphy,
      Bind(&DeviceInfo::OnWiFiInterfacePhyInfoReceived,
           AsWeakPtr(),
           iface_index),
      Bind(&NetlinkManager::OnAckDoNothing),
      Bind(&NetlinkManager::OnNetlinkMessageError));
}

void DeviceInfo::OnWiFiInterfacePhyInfoReceived(
    uint32_t iface_index, const shill::Nl80211Message& msg) {
  // Verify NL80211_CMD_NEW_WIPHY.
  if (msg.command() != shill::NewWiphyMessage::kCommand) {
    LOG(ERROR) << "Received unexpected command:"
               << msg.command();
    return;
  }

  map<uint32_t, Device::WiFiInterface>::iterator iter =
      interface_infos_.find(iface_index);
  if (iter == interface_infos_.end()) {
    // Interface is gone by the time we received its PHY info.
    LOG(ERROR) << "Interface [" << iface_index
               << "] is deleted when PHY info is received";
    return;
  }

  string device_name;
  if (!msg.const_attributes()->GetStringAttributeValue(NL80211_ATTR_WIPHY_NAME,
                                                       &device_name)) {
    LOG(ERROR) << "NL80211_CMD_NEW_WIPHY had no NL80211_ATTR_WIPHY_NAME";
    return;
  }

  scoped_refptr<Device> device = GetDevice(device_name);
  // Create device if it is not enumerated yet.
  if (!device) {
    device =
        new Device(manager_, device_name, device_identifier_++);
    device->ParseWiphyCapability(msg);

    // Register device
    RegisterDevice(device);
  }
  iter->second.device_name = device_name;

  device->RegisterInterface(iter->second);
}

void DeviceInfo::RegisterDevice(scoped_refptr<Device> device) {
  if (!device) {
    return;
  }
  devices_[device->GetDeviceName()] = device;
  // Register device with manager.
  manager_->RegisterDevice(device);
}

scoped_refptr<Device> DeviceInfo::GetDevice(const string& device_name) {
  map<string, scoped_refptr<Device>>::iterator iter =
      devices_.find(device_name);
  if (iter == devices_.end()) {
    return nullptr;
  }
  return iter->second;
}

}  // namespace apmanager
