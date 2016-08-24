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

#include <linux/netlink.h>
#include <linux/rtnetlink.h>

#include <map>
#include <string>
#include <vector>

#include <base/files/file_util.h>
#include <base/files/scoped_temp_dir.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <shill/net/byte_string.h>
#include <shill/net/mock_netlink_manager.h>
#include "shill/net/netlink_message_matchers.h"
#include "shill/net/nl80211_attribute.h"
#include "shill/net/nl80211_message.h"
#include <shill/net/rtnl_message.h>

#include "apmanager/fake_device_adaptor.h"
#include "apmanager/mock_control.h"
#include "apmanager/mock_device.h"
#include "apmanager/mock_manager.h"

using shill::ByteString;
using shill::Nl80211Message;
using shill::RTNLMessage;
using std::map;
using std::string;
using std::vector;
using ::testing::_;
using ::testing::Mock;
using ::testing::ReturnNew;

namespace apmanager {

namespace {

const char kTestDeviceName[] = "test-phy";
const char kTestInterface0Name[] = "test-interface0";
const char kTestInterface1Name[] = "test-interface1";
const uint32_t kTestInterface0Index = 1000;
const uint32_t kTestInterface1Index = 1001;

}  // namespace

class DeviceInfoTest : public testing::Test {
 public:
  DeviceInfoTest()
      : manager_(&control_interface_),
        device_info_(&manager_) {}
  virtual ~DeviceInfoTest() {}

  virtual void SetUp() {
    // Setup temporary directory for device info files.
    CHECK(temp_dir_.CreateUniqueTempDir());
    device_info_root_ = temp_dir_.path().Append("sys/class/net");
    device_info_.device_info_root_ = device_info_root_;

    // Setup mock pointers;
    device_info_.netlink_manager_ = &netlink_manager_;

    ON_CALL(control_interface_, CreateDeviceAdaptorRaw())
      .WillByDefault(ReturnNew<FakeDeviceAdaptor>());
  }

  bool IsWifiInterface(const string& interface_name) {
    return device_info_.IsWifiInterface(interface_name);
  }

  void CreateDeviceInfoFile(const string& interface_name,
                            const string& file_name,
                            const string& contents) {
    base::FilePath info_path =
        device_info_root_.Append(interface_name).Append(file_name);
    EXPECT_TRUE(base::CreateDirectory(info_path.DirName()));
    EXPECT_TRUE(base::WriteFile(info_path, contents.c_str(), contents.size()));
  }

  void SendLinkMsg(RTNLMessage::Mode mode,
                   uint32_t interface_index,
                   const string& interface_name) {
    RTNLMessage message(RTNLMessage::kTypeLink,
                        mode,
                        0,
                        0,
                        0,
                        interface_index,
                        shill::IPAddress::kFamilyIPv4);
    message.SetAttribute(static_cast<uint16_t>(IFLA_IFNAME),
                         ByteString(interface_name, true));
    device_info_.LinkMsgHandler(message);
  }

  void VerifyInterfaceList(const vector<Device::WiFiInterface>& interfaces) {
    // Verify number of elements in the interface infos map and interface index
    // of the elements in the map.
    EXPECT_EQ(interfaces.size(), device_info_.interface_infos_.size());
    for (const auto& interface : interfaces) {
      map<uint32_t, Device::WiFiInterface>::iterator it =
          device_info_.interface_infos_.find(interface.iface_index);
      EXPECT_NE(device_info_.interface_infos_.end(), it);
      EXPECT_TRUE(interface.Equals(it->second));
    }
  }

  void VerifyDeviceList(const vector<scoped_refptr<Device>>& devices) {
    // Verify number of elements in the device map and the elements in the map.
    EXPECT_EQ(devices.size(), device_info_.devices_.size());
    for (const auto& device : devices) {
      map<string, scoped_refptr<Device>>::iterator it =
          device_info_.devices_.find(device->GetDeviceName());
      EXPECT_NE(device_info_.devices_.end(), it);
      EXPECT_EQ(device, it->second);
    }
  }
  void AddInterface(const Device::WiFiInterface& interface) {
    device_info_.interface_infos_[interface.iface_index] = interface;
  }

  void OnWiFiPhyInfoReceived(const Nl80211Message& message) {
    device_info_.OnWiFiPhyInfoReceived(message);
  }

  void OnWiFiInterfaceInfoReceived(const Nl80211Message& message) {
    device_info_.OnWiFiInterfaceInfoReceived(message);
  }

  void OnWiFiInterfacePhyInfoReceived(uint32_t interface_index,
                                      const Nl80211Message& message) {
    device_info_.OnWiFiInterfacePhyInfoReceived(interface_index, message);
  }

  void RegisterDevice(scoped_refptr<Device> device) {
    device_info_.RegisterDevice(device);
  }

 protected:
  MockControl control_interface_;
  MockManager manager_;

  shill::MockNetlinkManager netlink_manager_;
  base::ScopedTempDir temp_dir_;
  base::FilePath device_info_root_;
  DeviceInfo device_info_;
};

MATCHER_P2(IsGetInfoMessage, command, index, "") {
  if (arg->message_type() != Nl80211Message::GetMessageType()) {
    return false;
  }
  const Nl80211Message *msg = reinterpret_cast<const Nl80211Message *>(arg);
  if (msg->command() != command) {
    return false;
  }
  uint32_t interface_index;
  if (!msg->const_attributes()->GetU32AttributeValue(NL80211_ATTR_IFINDEX,
                                                     &interface_index)) {
    return false;
  }
  // kInterfaceIndex is signed, but the attribute as handed from the kernel
  // is unsigned.  We're silently casting it away with this assignment.
  uint32_t test_interface_index = index;
  return interface_index == test_interface_index;
}

MATCHER_P(IsInterface, interface, "") {
  return arg.Equals(interface);
}

MATCHER_P(IsDevice, device_name, "") {
  return arg->GetDeviceName() == device_name;
}

TEST_F(DeviceInfoTest, EnumerateDevices) {
  shill::NewWiphyMessage message;

  // No device name in the message, failed to create device.
  EXPECT_CALL(manager_, RegisterDevice(_)).Times(0);
  OnWiFiPhyInfoReceived(message);

  // Device name in the message, device should be created/register to manager.
  message.attributes()->CreateNl80211Attribute(
      NL80211_ATTR_WIPHY_NAME, shill::NetlinkMessage::MessageContext());
  message.attributes()->SetStringAttributeValue(NL80211_ATTR_WIPHY_NAME,
                                                kTestDeviceName);
  EXPECT_CALL(manager_, RegisterDevice(IsDevice(kTestDeviceName))).Times(1);
  OnWiFiPhyInfoReceived(message);
  Mock::VerifyAndClearExpectations(&manager_);

  // Receive a message for a device already created, should not create/register
  // device again.
  EXPECT_CALL(manager_, RegisterDevice(_)).Times(0);
  OnWiFiPhyInfoReceived(message);
}

TEST_F(DeviceInfoTest, IsWiFiInterface) {
  // No device info file exist, not a wifi interface.
  EXPECT_FALSE(IsWifiInterface(kTestInterface0Name));

  // Device info for an ethernet device, not a wifi interface
  CreateDeviceInfoFile(kTestInterface0Name, "uevent", "INTERFACE=eth0\n");
  EXPECT_FALSE(IsWifiInterface(kTestInterface0Name));

  // Device info for a wifi interface.
  CreateDeviceInfoFile(kTestInterface1Name, "uevent", "DEVTYPE=wlan\n");
  EXPECT_TRUE(IsWifiInterface(kTestInterface1Name));
}

TEST_F(DeviceInfoTest, InterfaceDetection) {
  vector<Device::WiFiInterface> interface_list;
  // Ignore non-wifi interface.
  SendLinkMsg(RTNLMessage::kModeAdd,
              kTestInterface0Index,
              kTestInterface0Name);
  VerifyInterfaceList(interface_list);

  // AddLink event for wifi interface.
  CreateDeviceInfoFile(kTestInterface0Name, "uevent", "DEVTYPE=wlan\n");
  EXPECT_CALL(netlink_manager_, SendNl80211Message(
      IsGetInfoMessage(NL80211_CMD_GET_INTERFACE, kTestInterface0Index),
      _, _, _)).Times(1);
  SendLinkMsg(RTNLMessage::kModeAdd,
              kTestInterface0Index,
              kTestInterface0Name);
  interface_list.push_back(Device::WiFiInterface(
      kTestInterface0Name, "", kTestInterface0Index, 0));
  VerifyInterfaceList(interface_list);
  Mock::VerifyAndClearExpectations(&netlink_manager_);

  // AddLink event for another wifi interface.
  CreateDeviceInfoFile(kTestInterface1Name, "uevent", "DEVTYPE=wlan\n");
  EXPECT_CALL(netlink_manager_, SendNl80211Message(
      IsGetInfoMessage(NL80211_CMD_GET_INTERFACE, kTestInterface1Index),
      _, _, _)).Times(1);
  SendLinkMsg(RTNLMessage::kModeAdd,
              kTestInterface1Index,
              kTestInterface1Name);
  interface_list.push_back(Device::WiFiInterface(
      kTestInterface1Name, "", kTestInterface1Index, 0));
  VerifyInterfaceList(interface_list);
  Mock::VerifyAndClearExpectations(&netlink_manager_);

  // AddLink event for an interface that's already added, no change to interface
  // list.
  EXPECT_CALL(netlink_manager_, SendNl80211Message(_, _, _, _)).Times(0);
  SendLinkMsg(RTNLMessage::kModeAdd,
              kTestInterface0Index,
              kTestInterface0Name);
  VerifyInterfaceList(interface_list);
  Mock::VerifyAndClearExpectations(&netlink_manager_);

  // Remove the first wifi interface.
  SendLinkMsg(RTNLMessage::kModeDelete,
              kTestInterface0Index,
              kTestInterface0Name);
  interface_list.clear();
  interface_list.push_back(Device::WiFiInterface(
      kTestInterface1Name, "", kTestInterface1Index, 0));
  VerifyInterfaceList(interface_list);

  // Remove the non-exist interface, no change to the list.
  SendLinkMsg(RTNLMessage::kModeDelete,
              kTestInterface0Index,
              kTestInterface0Name);
  VerifyInterfaceList(interface_list);

  // Remove the last interface, list should be empty now.
  SendLinkMsg(RTNLMessage::kModeDelete,
              kTestInterface1Index,
              kTestInterface1Name);
  interface_list.clear();
  VerifyInterfaceList(interface_list);
}

TEST_F(DeviceInfoTest, ParseWifiInterfaceInfo) {
  // Add an interface without interface type info.
  Device::WiFiInterface interface(
      kTestInterface0Name, "", kTestInterface0Index, 0);
  AddInterface(interface);
  vector<Device::WiFiInterface> interface_list;
  interface_list.push_back(interface);

  // Message contain no interface index, no change to the interface info.
  shill::NewInterfaceMessage message;
  OnWiFiInterfaceInfoReceived(message);
  VerifyInterfaceList(interface_list);

  // Message contain no interface type, no change to the interface info.
  message.attributes()->CreateNl80211Attribute(
      NL80211_ATTR_IFINDEX, shill::NetlinkMessage::MessageContext());
  message.attributes()->SetU32AttributeValue(NL80211_ATTR_IFINDEX,
                                             kTestInterface0Index);
  OnWiFiInterfaceInfoReceived(message);

  // Message contain interface type, interface info should be updated with
  // the interface type, and a new Nl80211 message should be send to query for
  // the PHY info.
  EXPECT_CALL(netlink_manager_, SendNl80211Message(
      IsGetInfoMessage(NL80211_CMD_GET_WIPHY, kTestInterface0Index),
      _, _, _)).Times(1);
  message.attributes()->CreateNl80211Attribute(
      NL80211_ATTR_IFTYPE, shill::NetlinkMessage::MessageContext());
  message.attributes()->SetU32AttributeValue(NL80211_ATTR_IFTYPE,
                                             NL80211_IFTYPE_AP);
  OnWiFiInterfaceInfoReceived(message);
  interface_list[0].iface_type = NL80211_IFTYPE_AP;
  VerifyInterfaceList(interface_list);
}

TEST_F(DeviceInfoTest, ParsePhyInfoForWifiInterface) {
  // Register a mock device.
  scoped_refptr<MockDevice> device = new MockDevice(&manager_);
  device->SetDeviceName(kTestDeviceName);
  EXPECT_CALL(manager_, RegisterDevice(_)).Times(1);
  RegisterDevice(device);

  // PHY info message.
  shill::NewWiphyMessage message;
  message.attributes()->CreateNl80211Attribute(
      NL80211_ATTR_WIPHY_NAME, shill::NetlinkMessage::MessageContext());
  message.attributes()->SetStringAttributeValue(NL80211_ATTR_WIPHY_NAME,
                                                kTestDeviceName);

  // Receive PHY info message for an interface that have not been detected yet.
  EXPECT_CALL(*device.get(), RegisterInterface(_)).Times(0);
  OnWiFiInterfacePhyInfoReceived(kTestInterface0Index, message);

  // Pretend interface is detected through AddLink with interface info already
  // received (interface type), and still missing PHY info for that interface.
  Device::WiFiInterface interface(
      kTestInterface0Name, "", kTestInterface0Index, NL80211_IFTYPE_AP);
  AddInterface(interface);

  // PHY info is received for a detected interface, should register that
  // interface to the corresponding Device.
  interface.device_name = kTestDeviceName;
  EXPECT_CALL(*device.get(),
              RegisterInterface(IsInterface(interface))).Times(1);
  OnWiFiInterfacePhyInfoReceived(kTestInterface0Index, message);
}

TEST_F(DeviceInfoTest, ReceivePhyInfoBeforePhyIsEnumerated) {
  // New interface is detected.
  Device::WiFiInterface interface(
      kTestInterface0Name, "", kTestInterface0Index, NL80211_IFTYPE_AP);
  AddInterface(interface);
  vector<Device::WiFiInterface> interface_list;
  interface_list.push_back(interface);

  // Received PHY info for the interface when the corresponding PHY is not
  // enumerated yet, new device should be created and register to manager.
  shill::NewWiphyMessage message;
  message.attributes()->CreateNl80211Attribute(
      NL80211_ATTR_WIPHY_NAME, shill::NetlinkMessage::MessageContext());
  message.attributes()->SetStringAttributeValue(NL80211_ATTR_WIPHY_NAME,
                                                kTestDeviceName);
  EXPECT_CALL(manager_, RegisterDevice(IsDevice(kTestDeviceName))).Times(1);
  OnWiFiInterfacePhyInfoReceived(kTestInterface0Index, message);
  interface_list[0].device_name = kTestDeviceName;
  VerifyInterfaceList(interface_list);
}

TEST_F(DeviceInfoTest, RegisterDevice) {
  vector<scoped_refptr<Device>> device_list;

  // Register a nullptr.
  RegisterDevice(nullptr);
  VerifyDeviceList(device_list);

  // Register a device.
  device_list.push_back(new Device(&manager_, kTestDeviceName, 0));
  EXPECT_CALL(manager_, RegisterDevice(device_list[0]));
  RegisterDevice(device_list[0]);
  VerifyDeviceList(device_list);
}

}  // namespace apmanager
