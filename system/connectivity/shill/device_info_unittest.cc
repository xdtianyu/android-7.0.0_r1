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

#include "shill/device_info.h"

#include <memory>

#include <linux/if.h>
#include <linux/if_tun.h>
#include <linux/netlink.h>  // Needs typedefs from sys/socket.h.
#include <linux/rtnetlink.h>
#include <linux/sockios.h>
#include <net/if_arp.h>
#include <sys/socket.h>

#include <base/bind.h>
#include <base/files/file_util.h>
#include <base/files/scoped_temp_dir.h>
#include <base/memory/ref_counted.h>
#include <base/message_loop/message_loop.h>
#include <base/stl_util.h>
#include <base/strings/string_number_conversions.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "shill/cellular/mock_modem_info.h"
#include "shill/logging.h"
#include "shill/manager.h"
#include "shill/mock_control.h"
#include "shill/mock_device.h"
#include "shill/mock_log.h"
#include "shill/mock_manager.h"
#include "shill/mock_metrics.h"
#include "shill/mock_routing_table.h"
#include "shill/net/ip_address.h"
#include "shill/net/mock_rtnl_handler.h"
#include "shill/net/mock_sockets.h"
#include "shill/net/mock_time.h"
#include "shill/net/rtnl_message.h"
#include "shill/vpn/mock_vpn_provider.h"
#include "shill/wimax/mock_wimax_provider.h"
#include "shill/wimax/wimax.h"

#if !defined(DISABLE_WIFI)
#include "shill/net/mock_netlink_manager.h"
#include "shill/net/netlink_attribute.h"
#include "shill/net/nl80211_message.h"
#endif  // DISABLE_WIFI

using base::Callback;
using base::FilePath;
using std::map;
using std::set;
using std::string;
using std::unique_ptr;
using std::vector;
using testing::_;
using testing::AnyNumber;
using testing::ContainerEq;
using testing::DoAll;
using testing::ElementsAreArray;
using testing::HasSubstr;
using testing::Mock;
using testing::NotNull;
using testing::Return;
using testing::SetArgPointee;
using testing::StrictMock;
using testing::Test;

namespace shill {

class TestEventDispatcherForDeviceInfo : public EventDispatcher {
 public:
  virtual IOHandler* CreateInputHandler(
      int /*fd*/,
      const IOHandler::InputCallback& /*input_callback*/,
      const IOHandler::ErrorCallback& /*error_callback*/) {
    return nullptr;
  }
  MOCK_METHOD2(PostDelayedTask, void(const base::Closure& task,
                                     int64_t delay_ms));
};

class DeviceInfoTest : public Test {
 public:
  DeviceInfoTest()
      : metrics_(&dispatcher_),
        manager_(&control_interface_, &dispatcher_, &metrics_),
        device_info_(&control_interface_, &dispatcher_, &metrics_, &manager_) {
  }
  virtual ~DeviceInfoTest() {}

  virtual void SetUp() {
    device_info_.rtnl_handler_ = &rtnl_handler_;
    device_info_.routing_table_ = &routing_table_;
#if !defined(DISABLE_WIFI)
    device_info_.netlink_manager_ = &netlink_manager_;
#endif  // DISABLE_WIFI
    device_info_.time_ = &time_;
    manager_.set_mock_device_info(&device_info_);
    EXPECT_CALL(manager_, FilterPrependDNSServersByFamily(_))
      .WillRepeatedly(Return(vector<string>()));
  }

  IPAddress CreateInterfaceAddress() {
    // Create an IP address entry (as if left-over from a previous connection
    // manager).
    IPAddress address(IPAddress::kFamilyIPv4);
    EXPECT_TRUE(address.SetAddressFromString(kTestIPAddress0));
    address.set_prefix(kTestIPAddressPrefix0);
    vector<DeviceInfo::AddressData>& addresses =
        device_info_.infos_[kTestDeviceIndex].ip_addresses;
    addresses.push_back(DeviceInfo::AddressData(address, 0, RT_SCOPE_UNIVERSE));
    EXPECT_EQ(1, addresses.size());
    return address;
  }

  DeviceRefPtr CreateDevice(const std::string& link_name,
                            const std::string& address,
                            int interface_index,
                            Technology::Identifier technology) {
    return device_info_.CreateDevice(link_name, address, interface_index,
                                     technology);
  }

  virtual std::set<int>& GetDelayedDevices() {
    return device_info_.delayed_devices_;
  }

  int GetDelayedDeviceCreationMilliseconds() {
    return DeviceInfo::kDelayedDeviceCreationSeconds * 1000;
  }

  void SetSockets() {
    mock_sockets_ = new MockSockets();
    device_info_.set_sockets(mock_sockets_);
  }

  // Takes ownership of |provider|.
  void SetVPNProvider(VPNProvider* provider) {
    manager_.vpn_provider_.reset(provider);
    manager_.UpdateProviderMapping();
  }

  void SetManagerRunning(bool running) {
    manager_.running_ = running;
  }

 protected:
  static const int kTestDeviceIndex;
  static const char kTestDeviceName[];
  static const uint8_t kTestMACAddress[];
  static const char kTestIPAddress0[];
  static const int kTestIPAddressPrefix0;
  static const char kTestIPAddress1[];
  static const int kTestIPAddressPrefix1;
  static const char kTestIPAddress2[];
  static const char kTestIPAddress3[];
  static const char kTestIPAddress4[];
  static const char kTestIPAddress5[];
  static const char kTestIPAddress6[];
  static const char kTestIPAddress7[];
  static const int kReceiveByteCount;
  static const int kTransmitByteCount;

  RTNLMessage* BuildLinkMessage(RTNLMessage::Mode mode);
  RTNLMessage* BuildLinkMessageWithInterfaceName(RTNLMessage::Mode mode,
                                                 const string& interface_name);
  RTNLMessage* BuildAddressMessage(RTNLMessage::Mode mode,
                                   const IPAddress& address,
                                   unsigned char flags,
                                   unsigned char scope);
  RTNLMessage* BuildRdnssMessage(RTNLMessage::Mode mode,
                                 uint32_t lifetime,
                                 const vector<IPAddress>& dns_servers);
  void SendMessageToDeviceInfo(const RTNLMessage& message);

  MockControl control_interface_;
  MockMetrics metrics_;
  StrictMock<MockManager> manager_;
  DeviceInfo device_info_;
  TestEventDispatcherForDeviceInfo dispatcher_;
  MockRoutingTable routing_table_;
#if !defined(DISABLE_WIFI)
  MockNetlinkManager netlink_manager_;
#endif  // DISABLE_WIFI
  StrictMock<MockRTNLHandler> rtnl_handler_;
  MockSockets* mock_sockets_;  // Owned by DeviceInfo.
  MockTime time_;
};

const int DeviceInfoTest::kTestDeviceIndex = 123456;
const char DeviceInfoTest::kTestDeviceName[] = "test-device";
const uint8_t DeviceInfoTest::kTestMACAddress[] = {
    0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff };
const char DeviceInfoTest::kTestIPAddress0[] = "192.168.1.1";
const int DeviceInfoTest::kTestIPAddressPrefix0 = 24;
const char DeviceInfoTest::kTestIPAddress1[] = "fe80::1aa9:5ff:abcd:1234";
const int DeviceInfoTest::kTestIPAddressPrefix1 = 64;
const char DeviceInfoTest::kTestIPAddress2[] = "fe80::1aa9:5ff:abcd:1235";
const char DeviceInfoTest::kTestIPAddress3[] = "fe80::1aa9:5ff:abcd:1236";
const char DeviceInfoTest::kTestIPAddress4[] = "fe80::1aa9:5ff:abcd:1237";
const char DeviceInfoTest::kTestIPAddress5[] = "192.168.1.2";
const char DeviceInfoTest::kTestIPAddress6[] = "192.168.2.2";
const char DeviceInfoTest::kTestIPAddress7[] = "fe80::1aa9:5ff:abcd:1238";
const int DeviceInfoTest::kReceiveByteCount = 1234;
const int DeviceInfoTest::kTransmitByteCount = 5678;

RTNLMessage* DeviceInfoTest::BuildLinkMessageWithInterfaceName(
    RTNLMessage::Mode mode, const string& interface_name) {
  RTNLMessage* message = new RTNLMessage(
      RTNLMessage::kTypeLink,
      mode,
      0,
      0,
      0,
      kTestDeviceIndex,
      IPAddress::kFamilyIPv4);
  message->SetAttribute(static_cast<uint16_t>(IFLA_IFNAME),
                        ByteString(interface_name, true));
  ByteString test_address(kTestMACAddress, sizeof(kTestMACAddress));
  message->SetAttribute(IFLA_ADDRESS, test_address);
  return message;
}

RTNLMessage* DeviceInfoTest::BuildLinkMessage(RTNLMessage::Mode mode) {
  return BuildLinkMessageWithInterfaceName(mode, kTestDeviceName);
}

RTNLMessage* DeviceInfoTest::BuildAddressMessage(RTNLMessage::Mode mode,
                                                 const IPAddress& address,
                                                 unsigned char flags,
                                                 unsigned char scope) {
  RTNLMessage* message = new RTNLMessage(
      RTNLMessage::kTypeAddress,
      mode,
      0,
      0,
      0,
      kTestDeviceIndex,
      address.family());
  message->SetAttribute(IFA_ADDRESS, address.address());
  message->set_address_status(
      RTNLMessage::AddressStatus(address.prefix(), flags, scope));
  return message;
}

RTNLMessage* DeviceInfoTest::BuildRdnssMessage(RTNLMessage::Mode mode,
    uint32_t lifetime, const vector<IPAddress>& dns_servers) {
  RTNLMessage* message = new RTNLMessage(
      RTNLMessage::kTypeRdnss,
      mode,
      0,
      0,
      0,
      kTestDeviceIndex,
      IPAddress::kFamilyIPv6);
  message->set_rdnss_option(
      RTNLMessage::RdnssOption(lifetime, dns_servers));
  return message;
}

void DeviceInfoTest::SendMessageToDeviceInfo(const RTNLMessage& message) {
  if (message.type() == RTNLMessage::kTypeLink) {
    device_info_.LinkMsgHandler(message);
  } else if (message.type() == RTNLMessage::kTypeAddress) {
    device_info_.AddressMsgHandler(message);
  } else if (message.type() == RTNLMessage::kTypeRdnss) {
    device_info_.RdnssMsgHandler(message);
  } else {
    NOTREACHED();
  }
}

MATCHER_P(IsIPAddress, address, "") {
  // NB: IPAddress objects don't support the "==" operator as per style, so
  // we need a custom matcher.
  return address.Equals(arg);
}

TEST_F(DeviceInfoTest, StartStop) {
  EXPECT_FALSE(device_info_.link_listener_.get());
  EXPECT_FALSE(device_info_.address_listener_.get());
  EXPECT_TRUE(device_info_.infos_.empty());

  EXPECT_CALL(rtnl_handler_, RequestDump(RTNLHandler::kRequestLink |
                                         RTNLHandler::kRequestAddr));
  EXPECT_CALL(dispatcher_, PostDelayedTask(
      _, DeviceInfo::kRequestLinkStatisticsIntervalMilliseconds));
  device_info_.Start();
  EXPECT_TRUE(device_info_.link_listener_.get());
  EXPECT_TRUE(device_info_.address_listener_.get());
  EXPECT_TRUE(device_info_.infos_.empty());
  Mock::VerifyAndClearExpectations(&rtnl_handler_);

  CreateInterfaceAddress();
  EXPECT_FALSE(device_info_.infos_.empty());

  device_info_.Stop();
  EXPECT_FALSE(device_info_.link_listener_.get());
  EXPECT_FALSE(device_info_.address_listener_.get());
  EXPECT_TRUE(device_info_.infos_.empty());
}

TEST_F(DeviceInfoTest, RegisterDevice) {
  scoped_refptr<MockDevice> device0(new MockDevice(
      &control_interface_, &dispatcher_, &metrics_, &manager_,
      "null0", "addr0", kTestDeviceIndex));

  EXPECT_CALL(*device0, Initialize());
  device_info_.RegisterDevice(device0);
}

TEST_F(DeviceInfoTest, RequestLinkStatistics) {
  EXPECT_CALL(rtnl_handler_, RequestDump(RTNLHandler::kRequestLink));
  EXPECT_CALL(dispatcher_, PostDelayedTask(
      _, DeviceInfo::kRequestLinkStatisticsIntervalMilliseconds));
  device_info_.RequestLinkStatistics();
}

TEST_F(DeviceInfoTest, DeviceEnumeration) {
  unique_ptr<RTNLMessage> message(BuildLinkMessage(RTNLMessage::kModeAdd));
  message->set_link_status(RTNLMessage::LinkStatus(0, IFF_LOWER_UP, 0));
  EXPECT_FALSE(device_info_.GetDevice(kTestDeviceIndex).get());
  EXPECT_EQ(-1, device_info_.GetIndex(kTestDeviceName));
  SendMessageToDeviceInfo(*message);
  EXPECT_TRUE(device_info_.GetDevice(kTestDeviceIndex).get());
  unsigned int flags = 0;
  EXPECT_TRUE(device_info_.GetFlags(kTestDeviceIndex, &flags));
  EXPECT_EQ(IFF_LOWER_UP, flags);
  ByteString address;
  EXPECT_TRUE(device_info_.GetMACAddress(kTestDeviceIndex, &address));
  EXPECT_FALSE(address.IsEmpty());
  EXPECT_TRUE(address.Equals(ByteString(kTestMACAddress,
                                        sizeof(kTestMACAddress))));
  EXPECT_EQ(kTestDeviceIndex, device_info_.GetIndex(kTestDeviceName));

  message.reset(BuildLinkMessage(RTNLMessage::kModeAdd));
  message->set_link_status(RTNLMessage::LinkStatus(0, IFF_UP | IFF_RUNNING, 0));
  SendMessageToDeviceInfo(*message);
  EXPECT_TRUE(device_info_.GetFlags(kTestDeviceIndex, &flags));
  EXPECT_EQ(IFF_UP | IFF_RUNNING, flags);

  message.reset(BuildLinkMessage(RTNLMessage::kModeDelete));
  EXPECT_CALL(manager_, DeregisterDevice(_)).Times(1);
  SendMessageToDeviceInfo(*message);
  EXPECT_FALSE(device_info_.GetDevice(kTestDeviceIndex).get());
  EXPECT_FALSE(device_info_.GetFlags(kTestDeviceIndex, nullptr));
  EXPECT_EQ(-1, device_info_.GetIndex(kTestDeviceName));
}

TEST_F(DeviceInfoTest, DeviceRemovedEvent) {
  // Remove a Wifi device.
  scoped_refptr<MockDevice> device0(new MockDevice(
      &control_interface_, &dispatcher_, &metrics_, &manager_,
      "null0", "addr0", kTestDeviceIndex));
  device_info_.infos_[kTestDeviceIndex].device = device0;
  unique_ptr<RTNLMessage> message(BuildLinkMessage(RTNLMessage::kModeDelete));
  EXPECT_CALL(*device0, technology()).WillRepeatedly(Return(Technology::kWifi));
  EXPECT_CALL(manager_, DeregisterDevice(_)).Times(1);
  EXPECT_CALL(metrics_, DeregisterDevice(kTestDeviceIndex)).Times(1);
  SendMessageToDeviceInfo(*message);
  Mock::VerifyAndClearExpectations(device0.get());

  // Deregister a Cellular device.
  scoped_refptr<MockDevice> device1(new MockDevice(
      &control_interface_, &dispatcher_, &metrics_, &manager_,
      "null0", "addr0", kTestDeviceIndex));
  device_info_.infos_[kTestDeviceIndex].device = device1;
  EXPECT_CALL(*device1, technology()).
      WillRepeatedly(Return(Technology::kCellular));
  EXPECT_CALL(manager_, DeregisterDevice(_)).Times(1);
  EXPECT_CALL(metrics_, DeregisterDevice(kTestDeviceIndex)).Times(1);
  device_info_.DeregisterDevice(device1);
}

TEST_F(DeviceInfoTest, GetUninitializedTechnologies) {
  vector<string> technologies = device_info_.GetUninitializedTechnologies();
  set<string> expected_technologies;

  EXPECT_THAT(set<string>(technologies.begin(), technologies.end()),
              ContainerEq(expected_technologies));

  device_info_.infos_[0].technology = Technology::kUnknown;
  EXPECT_THAT(set<string>(technologies.begin(), technologies.end()),
              ContainerEq(expected_technologies));

  device_info_.infos_[1].technology = Technology::kCellular;
  technologies = device_info_.GetUninitializedTechnologies();
  expected_technologies.insert(Technology::NameFromIdentifier(
      Technology::kCellular));
  EXPECT_THAT(set<string>(technologies.begin(), technologies.end()),
              ContainerEq(expected_technologies));

  device_info_.infos_[2].technology = Technology::kWiMax;
  technologies = device_info_.GetUninitializedTechnologies();
  expected_technologies.insert(Technology::NameFromIdentifier(
      Technology::kWiMax));
  EXPECT_THAT(set<string>(technologies.begin(), technologies.end()),
              ContainerEq(expected_technologies));

  scoped_refptr<MockDevice> device(new MockDevice(
      &control_interface_, &dispatcher_, &metrics_, &manager_,
      "null0", "addr0", 1));
  device_info_.infos_[1].device = device;
  technologies = device_info_.GetUninitializedTechnologies();
  expected_technologies.erase(Technology::NameFromIdentifier(
      Technology::kCellular));
  EXPECT_THAT(set<string>(technologies.begin(), technologies.end()),
              ContainerEq(expected_technologies));

  device_info_.infos_[3].technology = Technology::kCellular;
  technologies = device_info_.GetUninitializedTechnologies();
  EXPECT_THAT(set<string>(technologies.begin(), technologies.end()),
              ContainerEq(expected_technologies));

  device_info_.infos_[3].device = device;
  device_info_.infos_[1].device = nullptr;
  technologies = device_info_.GetUninitializedTechnologies();
  EXPECT_THAT(set<string>(technologies.begin(), technologies.end()),
              ContainerEq(expected_technologies));
}

TEST_F(DeviceInfoTest, GetByteCounts) {
  uint64_t rx_bytes, tx_bytes;
  EXPECT_FALSE(device_info_.GetByteCounts(
      kTestDeviceIndex, &rx_bytes, &tx_bytes));

  // No link statistics in the message.
  unique_ptr<RTNLMessage> message(BuildLinkMessage(RTNLMessage::kModeAdd));
  SendMessageToDeviceInfo(*message);
  EXPECT_TRUE(device_info_.GetByteCounts(
      kTestDeviceIndex, &rx_bytes, &tx_bytes));
  EXPECT_EQ(0, rx_bytes);
  EXPECT_EQ(0, tx_bytes);

  // Short link statistics message.
  message.reset(BuildLinkMessage(RTNLMessage::kModeAdd));
  struct rtnl_link_stats64 stats;
  memset(&stats, 0, sizeof(stats));
  stats.rx_bytes = kReceiveByteCount;
  stats.tx_bytes = kTransmitByteCount;
  ByteString stats_bytes0(reinterpret_cast<const unsigned char*>(&stats),
                          sizeof(stats) - 1);
  message->SetAttribute(IFLA_STATS64, stats_bytes0);
  SendMessageToDeviceInfo(*message);
  EXPECT_TRUE(device_info_.GetByteCounts(
      kTestDeviceIndex, &rx_bytes, &tx_bytes));
  EXPECT_EQ(0, rx_bytes);
  EXPECT_EQ(0, tx_bytes);

  // Correctly sized link statistics message.
  message.reset(BuildLinkMessage(RTNLMessage::kModeAdd));
  ByteString stats_bytes1(reinterpret_cast<const unsigned char*>(&stats),
                          sizeof(stats));
  message->SetAttribute(IFLA_STATS64, stats_bytes1);
  SendMessageToDeviceInfo(*message);
  EXPECT_TRUE(device_info_.GetByteCounts(
      kTestDeviceIndex, &rx_bytes, &tx_bytes));
  EXPECT_EQ(kReceiveByteCount, rx_bytes);
  EXPECT_EQ(kTransmitByteCount, tx_bytes);
}

#if !defined(DISABLE_CELLULAR)

TEST_F(DeviceInfoTest, CreateDeviceCellular) {
  IPAddress address = CreateInterfaceAddress();

  // A cellular device should be offered to ModemInfo.
  StrictMock<MockModemInfo> modem_info;
  EXPECT_CALL(manager_, modem_info()).WillOnce(Return(&modem_info));
  EXPECT_CALL(modem_info, OnDeviceInfoAvailable(kTestDeviceName)).Times(1);
  EXPECT_CALL(routing_table_, FlushRoutes(kTestDeviceIndex)).Times(1);
  EXPECT_CALL(rtnl_handler_, RemoveInterfaceAddress(kTestDeviceIndex,
                                                    IsIPAddress(address)));
  EXPECT_FALSE(CreateDevice(
      kTestDeviceName, "address", kTestDeviceIndex, Technology::kCellular));
}

#endif  // DISABLE_CELLULAR

#if !defined(DISABLE_WIMAX)

TEST_F(DeviceInfoTest, CreateDeviceWiMax) {
  IPAddress address = CreateInterfaceAddress();

  // A WiMax device should be offered to WiMaxProvider.
  StrictMock<MockWiMaxProvider> wimax_provider;
  EXPECT_CALL(manager_, wimax_provider()).WillOnce(Return(&wimax_provider));
  EXPECT_CALL(wimax_provider, OnDeviceInfoAvailable(kTestDeviceName)).Times(1);
  EXPECT_CALL(routing_table_, FlushRoutes(kTestDeviceIndex)).Times(1);
  EXPECT_CALL(rtnl_handler_, RemoveInterfaceAddress(kTestDeviceIndex,
                                                    IsIPAddress(address)));
  device_info_.infos_[kTestDeviceIndex].mac_address =
      ByteString(kTestMACAddress, sizeof(kTestMACAddress));
  EXPECT_FALSE(CreateDevice(
      kTestDeviceName, "address", kTestDeviceIndex, Technology::kWiMax));
  // The MAC address is clear such that it is obtained via
  // GetMACAddressFromKernel() instead.
  EXPECT_TRUE(device_info_.infos_[kTestDeviceIndex].mac_address.IsEmpty());
}

#endif  // DISABLE_WIMAX

TEST_F(DeviceInfoTest, CreateDeviceEthernet) {
  IPAddress address = CreateInterfaceAddress();

  // An Ethernet device should cause routes and addresses to be flushed.
  EXPECT_CALL(routing_table_, FlushRoutes(kTestDeviceIndex)).Times(1);
  EXPECT_CALL(rtnl_handler_, RemoveInterfaceAddress(kTestDeviceIndex,
                                                    IsIPAddress(address)));
  DeviceRefPtr device = CreateDevice(
      kTestDeviceName, "address", kTestDeviceIndex, Technology::kEthernet);
  EXPECT_TRUE(device);
  Mock::VerifyAndClearExpectations(&routing_table_);
  Mock::VerifyAndClearExpectations(&rtnl_handler_);

  // The Ethernet device destructor should not call DeregisterService()
  // while being destructed, since the Manager may itself be partially
  // destructed at this time.
  EXPECT_CALL(manager_, DeregisterService(_)).Times(0);
  device = nullptr;
}

TEST_F(DeviceInfoTest, CreateDeviceVirtioEthernet) {
  IPAddress address = CreateInterfaceAddress();

  // VirtioEthernet is identical to Ethernet from the perspective of this test.
  EXPECT_CALL(routing_table_, FlushRoutes(kTestDeviceIndex)).Times(1);
  EXPECT_CALL(rtnl_handler_, RemoveInterfaceAddress(kTestDeviceIndex,
                                                    IsIPAddress(address)));
  DeviceRefPtr device = CreateDevice(
      kTestDeviceName, "address", kTestDeviceIndex,
      Technology::kVirtioEthernet);
  EXPECT_TRUE(device);
  Mock::VerifyAndClearExpectations(&routing_table_);
  Mock::VerifyAndClearExpectations(&rtnl_handler_);
}

#if !defined(DISABLE_WIFI)
MATCHER_P(IsGetInterfaceMessage, index, "") {
  if (arg->message_type() != Nl80211Message::GetMessageType()) {
    return false;
  }
  const Nl80211Message* msg = reinterpret_cast<const Nl80211Message*>(arg);
  if (msg->command() != NL80211_CMD_GET_INTERFACE) {
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

TEST_F(DeviceInfoTest, CreateDeviceWiFi) {
  IPAddress address = CreateInterfaceAddress();

  // WiFi looks a lot like Ethernet too.
  EXPECT_CALL(routing_table_, FlushRoutes(kTestDeviceIndex));
  EXPECT_CALL(rtnl_handler_, RemoveInterfaceAddress(kTestDeviceIndex,
                                                    IsIPAddress(address)));

  // Set the nl80211 message type to some non-default value.
  Nl80211Message::SetMessageType(1234);

  EXPECT_CALL(
      netlink_manager_,
      SendNl80211Message(IsGetInterfaceMessage(kTestDeviceIndex), _, _, _));
  EXPECT_FALSE(CreateDevice(
      kTestDeviceName, "address", kTestDeviceIndex, Technology::kWifi));
}
#endif  // DISABLE_WIFI

TEST_F(DeviceInfoTest, CreateDeviceTunnelAccepted) {
  IPAddress address = CreateInterfaceAddress();

  // A VPN device should be offered to VPNProvider.
  MockVPNProvider* vpn_provider = new StrictMock<MockVPNProvider>;
  SetVPNProvider(vpn_provider);
  EXPECT_CALL(*vpn_provider,
              OnDeviceInfoAvailable(kTestDeviceName, kTestDeviceIndex))
      .WillOnce(Return(true));
  EXPECT_CALL(routing_table_, FlushRoutes(kTestDeviceIndex)).Times(1);
  EXPECT_CALL(rtnl_handler_, RemoveInterfaceAddress(kTestDeviceIndex,
                                                    IsIPAddress(address)));
  EXPECT_CALL(rtnl_handler_, RemoveInterface(_)).Times(0);
  EXPECT_FALSE(CreateDevice(
      kTestDeviceName, "address", kTestDeviceIndex, Technology::kTunnel));
}

TEST_F(DeviceInfoTest, CreateDeviceTunnelRejected) {
  IPAddress address = CreateInterfaceAddress();

  // A VPN device should be offered to VPNProvider.
  MockVPNProvider* vpn_provider = new StrictMock<MockVPNProvider>;
  SetVPNProvider(vpn_provider);
  EXPECT_CALL(*vpn_provider,
              OnDeviceInfoAvailable(kTestDeviceName, kTestDeviceIndex))
      .WillOnce(Return(false));
  EXPECT_CALL(routing_table_, FlushRoutes(kTestDeviceIndex)).Times(1);
  EXPECT_CALL(rtnl_handler_, RemoveInterfaceAddress(kTestDeviceIndex,
                                                    IsIPAddress(address)));
  // Since the device was rejected by the VPNProvider, DeviceInfo will
  // remove the interface.
  EXPECT_CALL(rtnl_handler_, RemoveInterface(kTestDeviceIndex)).Times(1);
  EXPECT_FALSE(CreateDevice(
      kTestDeviceName, "address", kTestDeviceIndex, Technology::kTunnel));
}

TEST_F(DeviceInfoTest, CreateDevicePPP) {
  IPAddress address = CreateInterfaceAddress();

  // A VPN device should be offered to VPNProvider.
  MockVPNProvider* vpn_provider = new StrictMock<MockVPNProvider>;
  SetVPNProvider(vpn_provider);
  EXPECT_CALL(*vpn_provider,
              OnDeviceInfoAvailable(kTestDeviceName, kTestDeviceIndex))
      .WillOnce(Return(false));
  EXPECT_CALL(routing_table_, FlushRoutes(kTestDeviceIndex)).Times(1);
  EXPECT_CALL(rtnl_handler_, RemoveInterfaceAddress(kTestDeviceIndex,
                                                    IsIPAddress(address)));
  // We do not remove PPP interfaces even if the provider does not accept it.
  EXPECT_CALL(rtnl_handler_, RemoveInterface(_)).Times(0);
  EXPECT_FALSE(CreateDevice(
      kTestDeviceName, "address", kTestDeviceIndex, Technology::kPPP));
}

TEST_F(DeviceInfoTest, CreateDeviceLoopback) {
  // A loopback device should be brought up, and nothing else done to it.
  EXPECT_CALL(routing_table_, FlushRoutes(_)).Times(0);
  EXPECT_CALL(rtnl_handler_, RemoveInterfaceAddress(_, _)).Times(0);
  EXPECT_CALL(rtnl_handler_,
              SetInterfaceFlags(kTestDeviceIndex, IFF_UP, IFF_UP)).Times(1);
  EXPECT_FALSE(CreateDevice(
      kTestDeviceName, "address", kTestDeviceIndex, Technology::kLoopback));
}

TEST_F(DeviceInfoTest, CreateDeviceCDCEthernet) {
  // A cdc_ether / cdc_ncm device should be postponed to a task.
  EXPECT_CALL(manager_, modem_info()).Times(0);
  EXPECT_CALL(routing_table_, FlushRoutes(_)).Times(0);
  EXPECT_CALL(rtnl_handler_, RemoveInterfaceAddress(_, _)).Times(0);
  EXPECT_CALL(dispatcher_,
              PostDelayedTask(_, GetDelayedDeviceCreationMilliseconds()));
  EXPECT_TRUE(GetDelayedDevices().empty());
  EXPECT_FALSE(CreateDevice(
      kTestDeviceName, "address", kTestDeviceIndex, Technology::kCDCEthernet));
  EXPECT_FALSE(GetDelayedDevices().empty());
  EXPECT_EQ(1, GetDelayedDevices().size());
  EXPECT_EQ(kTestDeviceIndex, *GetDelayedDevices().begin());
}

TEST_F(DeviceInfoTest, CreateDeviceUnknown) {
  IPAddress address = CreateInterfaceAddress();

  // An unknown (blacklisted, unhandled, etc) device won't be flushed or
  // registered.
  EXPECT_CALL(routing_table_, FlushRoutes(_)).Times(0);
  EXPECT_CALL(rtnl_handler_, RemoveInterfaceAddress(_, _)).Times(0);
  EXPECT_TRUE(CreateDevice(
      kTestDeviceName, "address", kTestDeviceIndex, Technology::kUnknown));
}

TEST_F(DeviceInfoTest, DeviceBlackList) {
  // Manager is not running by default.
  EXPECT_CALL(rtnl_handler_, RequestDump(RTNLHandler::kRequestLink)).Times(0);
  device_info_.AddDeviceToBlackList(kTestDeviceName);
  unique_ptr<RTNLMessage> message(BuildLinkMessage(RTNLMessage::kModeAdd));
  SendMessageToDeviceInfo(*message);

  DeviceRefPtr device = device_info_.GetDevice(kTestDeviceIndex);
  ASSERT_TRUE(device.get());
  EXPECT_TRUE(device->technology() == Technology::kBlacklisted);
}

TEST_F(DeviceInfoTest, AddDeviceToBlackListWithManagerRunning) {
  SetManagerRunning(true);
  EXPECT_CALL(rtnl_handler_, RequestDump(RTNLHandler::kRequestLink)).Times(1);
  device_info_.AddDeviceToBlackList(kTestDeviceName);
  unique_ptr<RTNLMessage> message(BuildLinkMessage(RTNLMessage::kModeAdd));
  SendMessageToDeviceInfo(*message);

  DeviceRefPtr device = device_info_.GetDevice(kTestDeviceIndex);
  ASSERT_TRUE(device.get());
  EXPECT_TRUE(device->technology() == Technology::kBlacklisted);
}

TEST_F(DeviceInfoTest, RenamedBlacklistedDevice) {
  device_info_.AddDeviceToBlackList(kTestDeviceName);
  unique_ptr<RTNLMessage> message(BuildLinkMessage(RTNLMessage::kModeAdd));
  SendMessageToDeviceInfo(*message);

  DeviceRefPtr device = device_info_.GetDevice(kTestDeviceIndex);
  ASSERT_TRUE(device.get());
  EXPECT_TRUE(device->technology() == Technology::kBlacklisted);

  // Rename the test device.
  const char kRenamedDeviceName[] = "renamed-device";
  unique_ptr<RTNLMessage> rename_message(
      BuildLinkMessageWithInterfaceName(
          RTNLMessage::kModeAdd, kRenamedDeviceName));
  EXPECT_CALL(manager_, DeregisterDevice(_));
  EXPECT_CALL(metrics_, DeregisterDevice(kTestDeviceIndex));
  SendMessageToDeviceInfo(*rename_message);

  DeviceRefPtr renamed_device = device_info_.GetDevice(kTestDeviceIndex);
  ASSERT_TRUE(renamed_device.get());

  // Expect that a different device has been created.
  EXPECT_NE(device.get(), renamed_device.get());

  // Since we didn't create a uevent file for kRenamedDeviceName, its
  // technology should be unknown.
  EXPECT_TRUE(renamed_device->technology() == Technology::kUnknown);
}

TEST_F(DeviceInfoTest, RenamedNonBlacklistedDevice) {
  const char kInitialDeviceName[] = "initial-device";
  unique_ptr<RTNLMessage> initial_message(
      BuildLinkMessageWithInterfaceName(
          RTNLMessage::kModeAdd, kInitialDeviceName));
  SendMessageToDeviceInfo(*initial_message);
  unique_ptr<RTNLMessage> message(BuildLinkMessage(RTNLMessage::kModeAdd));

  DeviceRefPtr initial_device = device_info_.GetDevice(kTestDeviceIndex);
  ASSERT_TRUE(initial_device.get());

  // Since we didn't create a uevent file for kInitialDeviceName, its
  // technology should be unknown.
  EXPECT_TRUE(initial_device->technology() == Technology::kUnknown);

  // Rename the test device.
  const char kRenamedDeviceName[] = "renamed-device";
  device_info_.AddDeviceToBlackList(kRenamedDeviceName);
  unique_ptr<RTNLMessage> rename_message(
      BuildLinkMessageWithInterfaceName(
          RTNLMessage::kModeAdd, kRenamedDeviceName));
  EXPECT_CALL(manager_, DeregisterDevice(_)).Times(0);
  EXPECT_CALL(metrics_, DeregisterDevice(kTestDeviceIndex)).Times(0);
  SendMessageToDeviceInfo(*rename_message);

  DeviceRefPtr renamed_device = device_info_.GetDevice(kTestDeviceIndex);
  ASSERT_TRUE(renamed_device.get());

  // Expect that the the presence of a renamed device does not cause a new
  // Device entry to be created if the initial device was not blacklisted.
  EXPECT_EQ(initial_device.get(), renamed_device.get());
  EXPECT_TRUE(initial_device->technology() == Technology::kUnknown);
}

TEST_F(DeviceInfoTest, DeviceAddressList) {
  unique_ptr<RTNLMessage> message(BuildLinkMessage(RTNLMessage::kModeAdd));
  SendMessageToDeviceInfo(*message);

  vector<DeviceInfo::AddressData> addresses;
  EXPECT_TRUE(device_info_.GetAddresses(kTestDeviceIndex, &addresses));
  EXPECT_TRUE(addresses.empty());

  // Add an address to the device address list
  IPAddress ip_address0(IPAddress::kFamilyIPv4);
  EXPECT_TRUE(ip_address0.SetAddressFromString(kTestIPAddress0));
  ip_address0.set_prefix(kTestIPAddressPrefix0);
  message.reset(BuildAddressMessage(RTNLMessage::kModeAdd, ip_address0, 0, 0));
  SendMessageToDeviceInfo(*message);
  EXPECT_TRUE(device_info_.GetAddresses(kTestDeviceIndex, &addresses));
  EXPECT_EQ(1, addresses.size());
  EXPECT_TRUE(ip_address0.Equals(addresses[0].address));

  // Re-adding the same address shouldn't cause the address list to change
  SendMessageToDeviceInfo(*message);
  EXPECT_TRUE(device_info_.GetAddresses(kTestDeviceIndex, &addresses));
  EXPECT_EQ(1, addresses.size());
  EXPECT_TRUE(ip_address0.Equals(addresses[0].address));

  // Adding a new address should expand the list
  IPAddress ip_address1(IPAddress::kFamilyIPv6);
  EXPECT_TRUE(ip_address1.SetAddressFromString(kTestIPAddress1));
  ip_address1.set_prefix(kTestIPAddressPrefix1);
  message.reset(BuildAddressMessage(RTNLMessage::kModeAdd, ip_address1, 0, 0));
  SendMessageToDeviceInfo(*message);
  EXPECT_TRUE(device_info_.GetAddresses(kTestDeviceIndex, &addresses));
  EXPECT_EQ(2, addresses.size());
  EXPECT_TRUE(ip_address0.Equals(addresses[0].address));
  EXPECT_TRUE(ip_address1.Equals(addresses[1].address));

  // Deleting an address should reduce the list
  message.reset(BuildAddressMessage(RTNLMessage::kModeDelete,
                                    ip_address0,
                                    0,
                                    0));
  SendMessageToDeviceInfo(*message);
  EXPECT_TRUE(device_info_.GetAddresses(kTestDeviceIndex, &addresses));
  EXPECT_EQ(1, addresses.size());
  EXPECT_TRUE(ip_address1.Equals(addresses[0].address));

  // Delete last item
  message.reset(BuildAddressMessage(RTNLMessage::kModeDelete,
                                    ip_address1,
                                    0,
                                    0));
  SendMessageToDeviceInfo(*message);
  EXPECT_TRUE(device_info_.GetAddresses(kTestDeviceIndex, &addresses));
  EXPECT_TRUE(addresses.empty());

  // Delete device
  message.reset(BuildLinkMessage(RTNLMessage::kModeDelete));
  EXPECT_CALL(manager_, DeregisterDevice(_)).Times(1);
  SendMessageToDeviceInfo(*message);

  // Should be able to handle message for interface that doesn't exist
  message.reset(BuildAddressMessage(RTNLMessage::kModeAdd, ip_address0, 0, 0));
  SendMessageToDeviceInfo(*message);
  EXPECT_FALSE(device_info_.GetDevice(kTestDeviceIndex).get());
}

TEST_F(DeviceInfoTest, FlushAddressList) {
  unique_ptr<RTNLMessage> message(BuildLinkMessage(RTNLMessage::kModeAdd));
  SendMessageToDeviceInfo(*message);

  IPAddress address1(IPAddress::kFamilyIPv6);
  EXPECT_TRUE(address1.SetAddressFromString(kTestIPAddress1));
  address1.set_prefix(kTestIPAddressPrefix1);
  message.reset(BuildAddressMessage(RTNLMessage::kModeAdd,
                                    address1,
                                    0,
                                    RT_SCOPE_UNIVERSE));
  SendMessageToDeviceInfo(*message);
  IPAddress address2(IPAddress::kFamilyIPv6);
  EXPECT_TRUE(address2.SetAddressFromString(kTestIPAddress2));
  message.reset(BuildAddressMessage(RTNLMessage::kModeAdd,
                                    address2,
                                    IFA_F_TEMPORARY,
                                    RT_SCOPE_UNIVERSE));
  SendMessageToDeviceInfo(*message);
  IPAddress address3(IPAddress::kFamilyIPv6);
  EXPECT_TRUE(address3.SetAddressFromString(kTestIPAddress3));
  message.reset(BuildAddressMessage(RTNLMessage::kModeAdd,
                                    address3,
                                    0,
                                    RT_SCOPE_LINK));
  SendMessageToDeviceInfo(*message);
  IPAddress address4(IPAddress::kFamilyIPv6);
  EXPECT_TRUE(address4.SetAddressFromString(kTestIPAddress4));
  message.reset(BuildAddressMessage(RTNLMessage::kModeAdd,
                                    address4,
                                    IFA_F_PERMANENT,
                                    RT_SCOPE_UNIVERSE));
  SendMessageToDeviceInfo(*message);

  // DeviceInfo now has 4 addresses associated with it, but only two of
  // them are valid for flush.
  EXPECT_CALL(rtnl_handler_, RemoveInterfaceAddress(kTestDeviceIndex,
                                                    IsIPAddress(address1)));
  EXPECT_CALL(rtnl_handler_, RemoveInterfaceAddress(kTestDeviceIndex,
                                                    IsIPAddress(address2)));
  device_info_.FlushAddresses(kTestDeviceIndex);
}

TEST_F(DeviceInfoTest, HasOtherAddress) {
  unique_ptr<RTNLMessage> message(BuildLinkMessage(RTNLMessage::kModeAdd));
  SendMessageToDeviceInfo(*message);

  IPAddress address0(IPAddress::kFamilyIPv4);
  EXPECT_TRUE(address0.SetAddressFromString(kTestIPAddress0));

  // There are no addresses on this interface.
  EXPECT_FALSE(device_info_.HasOtherAddress(kTestDeviceIndex, address0));

  message.reset(BuildAddressMessage(RTNLMessage::kModeAdd,
                                    address0,
                                    0,
                                    RT_SCOPE_UNIVERSE));
  SendMessageToDeviceInfo(*message);

  IPAddress address1(IPAddress::kFamilyIPv6);
  EXPECT_TRUE(address1.SetAddressFromString(kTestIPAddress1));
  address1.set_prefix(kTestIPAddressPrefix1);
  message.reset(BuildAddressMessage(RTNLMessage::kModeAdd,
                                    address1,
                                    0,
                                    RT_SCOPE_LINK));
  SendMessageToDeviceInfo(*message);

  IPAddress address2(IPAddress::kFamilyIPv6);
  EXPECT_TRUE(address2.SetAddressFromString(kTestIPAddress2));
  message.reset(BuildAddressMessage(RTNLMessage::kModeAdd,
                                    address2,
                                    IFA_F_TEMPORARY,
                                    RT_SCOPE_UNIVERSE));
  SendMessageToDeviceInfo(*message);

  IPAddress address3(IPAddress::kFamilyIPv6);
  EXPECT_TRUE(address3.SetAddressFromString(kTestIPAddress3));

  // The only IPv6 addresses on this interface are either flagged as
  // temporary, or they are not universally scoped.
  EXPECT_FALSE(device_info_.HasOtherAddress(kTestDeviceIndex, address3));


  message.reset(BuildAddressMessage(RTNLMessage::kModeAdd,
                                    address3,
                                    0,
                                    RT_SCOPE_UNIVERSE));
  SendMessageToDeviceInfo(*message);

  // address0 is on this interface.
  EXPECT_FALSE(device_info_.HasOtherAddress(kTestDeviceIndex, address0));
  // address1 is on this interface.
  EXPECT_FALSE(device_info_.HasOtherAddress(kTestDeviceIndex, address1));
  // address2 is on this interface.
  EXPECT_FALSE(device_info_.HasOtherAddress(kTestDeviceIndex, address2));
  // address3 is on this interface.
  EXPECT_FALSE(device_info_.HasOtherAddress(kTestDeviceIndex, address3));

  IPAddress address4(IPAddress::kFamilyIPv6);
  EXPECT_TRUE(address4.SetAddressFromString(kTestIPAddress4));

  // address4 is not on this interface, but address3 is, and is a qualified
  // IPv6 address.
  EXPECT_TRUE(device_info_.HasOtherAddress(kTestDeviceIndex, address4));

  message.reset(BuildAddressMessage(RTNLMessage::kModeAdd,
                                    address4,
                                    IFA_F_PERMANENT,
                                    RT_SCOPE_UNIVERSE));
  SendMessageToDeviceInfo(*message);

  // address4 is now on this interface.
  EXPECT_FALSE(device_info_.HasOtherAddress(kTestDeviceIndex, address4));

  IPAddress address5(IPAddress::kFamilyIPv4);
  EXPECT_TRUE(address5.SetAddressFromString(kTestIPAddress5));
  // address5 is not on this interface, but address0 is.
  EXPECT_TRUE(device_info_.HasOtherAddress(kTestDeviceIndex, address5));

  message.reset(BuildAddressMessage(RTNLMessage::kModeAdd,
                                    address5,
                                    IFA_F_PERMANENT,
                                    RT_SCOPE_UNIVERSE));
  SendMessageToDeviceInfo(*message);

  // address5 is now on this interface.
  EXPECT_FALSE(device_info_.HasOtherAddress(kTestDeviceIndex, address5));
}

TEST_F(DeviceInfoTest, HasDirectConnectivityTo) {
  unique_ptr<RTNLMessage> message(BuildLinkMessage(RTNLMessage::kModeAdd));
  SendMessageToDeviceInfo(*message);

  IPAddress address0(IPAddress::kFamilyIPv4);
  EXPECT_TRUE(address0.SetAddressFromString(kTestIPAddress0));

  // There are no addresses on this interface.
  EXPECT_FALSE(device_info_.HasDirectConnectivityTo(
      kTestDeviceIndex, address0));

  IPAddress address1(IPAddress::kFamilyIPv6);
  EXPECT_TRUE(address1.SetAddressFromString(kTestIPAddress1));
  message.reset(BuildAddressMessage(RTNLMessage::kModeAdd,
                                    address1,
                                    IFA_F_PERMANENT,
                                    RT_SCOPE_UNIVERSE));
  SendMessageToDeviceInfo(*message);

  // No current addresses are of the same family as |address0|.
  EXPECT_FALSE(device_info_.HasDirectConnectivityTo(
      kTestDeviceIndex, address0));

  IPAddress address6(IPAddress::kFamilyIPv4);
  EXPECT_TRUE(address6.SetAddressFromString(kTestIPAddress6));
  address6.set_prefix(kTestIPAddressPrefix0);
  message.reset(BuildAddressMessage(RTNLMessage::kModeAdd,
                                    address6,
                                    IFA_F_PERMANENT,
                                    RT_SCOPE_UNIVERSE));
  SendMessageToDeviceInfo(*message);

  // |address0| is not reachable from |address6|.
  EXPECT_FALSE(device_info_.HasDirectConnectivityTo(
      kTestDeviceIndex, address0));

  IPAddress address5(IPAddress::kFamilyIPv4);
  EXPECT_TRUE(address5.SetAddressFromString(kTestIPAddress5));
  address5.set_prefix(kTestIPAddressPrefix0);
  message.reset(BuildAddressMessage(RTNLMessage::kModeAdd,
                                    address5,
                                    IFA_F_PERMANENT,
                                    RT_SCOPE_UNIVERSE));
  SendMessageToDeviceInfo(*message);

  // |address0| is reachable from |address5| which is associated with the
  // interface.
  EXPECT_TRUE(device_info_.HasDirectConnectivityTo(
      kTestDeviceIndex, address0));
}

TEST_F(DeviceInfoTest, HasSubdir) {
  base::ScopedTempDir temp_dir;
  EXPECT_TRUE(temp_dir.CreateUniqueTempDir());
  EXPECT_TRUE(base::CreateDirectory(temp_dir.path().Append("child1")));
  FilePath child2 = temp_dir.path().Append("child2");
  EXPECT_TRUE(base::CreateDirectory(child2));
  FilePath grandchild = child2.Append("grandchild");
  EXPECT_TRUE(base::CreateDirectory(grandchild));
  EXPECT_TRUE(base::CreateDirectory(grandchild.Append("greatgrandchild")));
  EXPECT_TRUE(DeviceInfo::HasSubdir(temp_dir.path(),
                                    FilePath("grandchild")));
  EXPECT_TRUE(DeviceInfo::HasSubdir(temp_dir.path(),
                                    FilePath("greatgrandchild")));
  EXPECT_FALSE(DeviceInfo::HasSubdir(temp_dir.path(),
                                     FilePath("nonexistent")));
}

TEST_F(DeviceInfoTest, GetMACAddressFromKernelUnknownDevice) {
  SetSockets();
  EXPECT_CALL(*mock_sockets_, Socket(PF_INET, SOCK_DGRAM, 0)).Times(0);
  ByteString mac_address =
      device_info_.GetMACAddressFromKernel(kTestDeviceIndex);
  EXPECT_TRUE(mac_address.IsEmpty());
}

TEST_F(DeviceInfoTest, GetMACAddressFromKernelUnableToOpenSocket) {
  SetSockets();
  EXPECT_CALL(*mock_sockets_, Socket(PF_INET, SOCK_DGRAM, 0))
      .WillOnce(Return(-1));
  unique_ptr<RTNLMessage> message(BuildLinkMessage(RTNLMessage::kModeAdd));
  message->set_link_status(RTNLMessage::LinkStatus(0, IFF_LOWER_UP, 0));
  SendMessageToDeviceInfo(*message);
  EXPECT_TRUE(device_info_.GetDevice(kTestDeviceIndex).get());
  ByteString mac_address =
      device_info_.GetMACAddressFromKernel(kTestDeviceIndex);
  EXPECT_TRUE(mac_address.IsEmpty());
}

TEST_F(DeviceInfoTest, GetMACAddressFromKernelIoctlFails) {
  SetSockets();
  const int kFd = 99;
  EXPECT_CALL(*mock_sockets_, Socket(PF_INET, SOCK_DGRAM, 0))
      .WillOnce(Return(kFd));
  EXPECT_CALL(*mock_sockets_, Ioctl(kFd, SIOCGIFHWADDR, NotNull()))
      .WillOnce(Return(-1));
  EXPECT_CALL(*mock_sockets_, Close(kFd));

  unique_ptr<RTNLMessage> message(BuildLinkMessage(RTNLMessage::kModeAdd));
  message->set_link_status(RTNLMessage::LinkStatus(0, IFF_LOWER_UP, 0));
  SendMessageToDeviceInfo(*message);
  EXPECT_TRUE(device_info_.GetDevice(kTestDeviceIndex).get());

  ByteString mac_address =
      device_info_.GetMACAddressFromKernel(kTestDeviceIndex);
  EXPECT_TRUE(mac_address.IsEmpty());
}

MATCHER_P2(IfreqEquals, ifindex, ifname, "") {
  const struct ifreq* const ifr = static_cast<struct ifreq*>(arg);
  return (ifr != nullptr) &&
      (ifr->ifr_ifindex == ifindex) &&
      (strcmp(ifname, ifr->ifr_name) == 0);
}

ACTION_P(SetIfreq, ifr) {
  struct ifreq* const ifr_arg = static_cast<struct ifreq*>(arg2);
  *ifr_arg = ifr;
}

TEST_F(DeviceInfoTest, GetMACAddressFromKernel) {
  SetSockets();
  const int kFd = 99;
  struct ifreq ifr;
  static uint8_t kMacAddress[] = {0x00, 0x01, 0x02, 0xaa, 0xbb, 0xcc};
  memcpy(ifr.ifr_hwaddr.sa_data, kMacAddress, sizeof(kMacAddress));
  EXPECT_CALL(*mock_sockets_, Socket(PF_INET, SOCK_DGRAM, 0))
      .WillOnce(Return(kFd));
  EXPECT_CALL(*mock_sockets_,
              Ioctl(kFd, SIOCGIFHWADDR,
                    IfreqEquals(kTestDeviceIndex, kTestDeviceName)))
      .WillOnce(DoAll(SetIfreq(ifr), Return(0)));
  EXPECT_CALL(*mock_sockets_, Close(kFd));

  unique_ptr<RTNLMessage> message(BuildLinkMessage(RTNLMessage::kModeAdd));
  message->set_link_status(RTNLMessage::LinkStatus(0, IFF_LOWER_UP, 0));
  SendMessageToDeviceInfo(*message);
  EXPECT_TRUE(device_info_.GetDevice(kTestDeviceIndex).get());

  ByteString mac_address =
      device_info_.GetMACAddressFromKernel(kTestDeviceIndex);
  EXPECT_THAT(kMacAddress,
              ElementsAreArray(mac_address.GetData(), sizeof(kMacAddress)));
}

TEST_F(DeviceInfoTest, GetMACAddressOfPeerUnknownDevice) {
  SetSockets();
  EXPECT_CALL(*mock_sockets_, Socket(PF_INET, SOCK_DGRAM, 0)).Times(0);
  IPAddress address(IPAddress::kFamilyIPv4);
  EXPECT_TRUE(address.SetAddressFromString(kTestIPAddress0));
  ByteString mac_address;
  EXPECT_FALSE(device_info_.GetDevice(kTestDeviceIndex).get());
  EXPECT_FALSE(device_info_.GetMACAddressOfPeer(
      kTestDeviceIndex, address, &mac_address));
}

TEST_F(DeviceInfoTest, GetMACAddressOfPeerBadAddress) {
  SetSockets();
  unique_ptr<RTNLMessage> message(BuildLinkMessage(RTNLMessage::kModeAdd));
  message->set_link_status(RTNLMessage::LinkStatus(0, IFF_LOWER_UP, 0));
  SendMessageToDeviceInfo(*message);
  EXPECT_TRUE(device_info_.GetDevice(kTestDeviceIndex).get());

  EXPECT_CALL(*mock_sockets_, Socket(PF_INET, SOCK_DGRAM, 0)).Times(0);

  // An improperly formatted IPv4 address should fail.
  IPAddress empty_ipv4_address(IPAddress::kFamilyIPv4);
  ByteString mac_address;
  EXPECT_FALSE(device_info_.GetMACAddressOfPeer(
      kTestDeviceIndex, empty_ipv4_address, &mac_address));

  // IPv6 addresses are not supported.
  IPAddress valid_ipv6_address(IPAddress::kFamilyIPv6);
  EXPECT_TRUE(valid_ipv6_address.SetAddressFromString(kTestIPAddress1));
  EXPECT_FALSE(device_info_.GetMACAddressOfPeer(
      kTestDeviceIndex, valid_ipv6_address, &mac_address));
}

TEST_F(DeviceInfoTest, GetMACAddressOfPeerUnableToOpenSocket) {
  SetSockets();
  EXPECT_CALL(*mock_sockets_, Socket(PF_INET, SOCK_DGRAM, 0))
      .WillOnce(Return(-1));
  unique_ptr<RTNLMessage> message(BuildLinkMessage(RTNLMessage::kModeAdd));
  message->set_link_status(RTNLMessage::LinkStatus(0, IFF_LOWER_UP, 0));
  SendMessageToDeviceInfo(*message);
  IPAddress ip_address(IPAddress::kFamilyIPv4);
  EXPECT_TRUE(ip_address.SetAddressFromString(kTestIPAddress0));
  ByteString mac_address;
  EXPECT_FALSE(device_info_.GetMACAddressOfPeer(
      kTestDeviceIndex, ip_address, &mac_address));
}

TEST_F(DeviceInfoTest, GetMACAddressOfPeerIoctlFails) {
  SetSockets();
  const int kFd = 99;
  EXPECT_CALL(*mock_sockets_, Socket(PF_INET, SOCK_DGRAM, 0))
      .WillOnce(Return(kFd));
  EXPECT_CALL(*mock_sockets_, Ioctl(kFd, SIOCGARP, NotNull()))
      .WillOnce(Return(-1));
  unique_ptr<RTNLMessage> message(BuildLinkMessage(RTNLMessage::kModeAdd));
  message->set_link_status(RTNLMessage::LinkStatus(0, IFF_LOWER_UP, 0));
  SendMessageToDeviceInfo(*message);
  IPAddress ip_address(IPAddress::kFamilyIPv4);
  EXPECT_TRUE(ip_address.SetAddressFromString(kTestIPAddress0));
  ByteString mac_address;
  EXPECT_FALSE(device_info_.GetMACAddressOfPeer(
      kTestDeviceIndex, ip_address, &mac_address));
}

MATCHER_P2(ArpreqEquals, ifname, peer, "") {
  const struct arpreq* const areq = static_cast<struct arpreq*>(arg);
  if (areq == nullptr) {
    return false;
  }

  const struct sockaddr_in* const protocol_address =
      reinterpret_cast<const struct sockaddr_in*>(&areq->arp_pa);
  const struct sockaddr_in* const hardware_address =
      reinterpret_cast<const struct sockaddr_in*>(&areq->arp_ha);

  return
      strcmp(ifname, areq->arp_dev) == 0 &&
      protocol_address->sin_family == AF_INET &&
      memcmp(&protocol_address->sin_addr.s_addr,
             peer.address().GetConstData(),
             peer.address().GetLength()) == 0 &&
      hardware_address->sin_family == ARPHRD_ETHER;
}

ACTION_P(SetArpreq, areq) {
  struct arpreq* const areq_arg = static_cast<struct arpreq*>(arg2);
  *areq_arg = areq;
}

TEST_F(DeviceInfoTest, GetMACAddressOfPeer) {
  unique_ptr<RTNLMessage> message(BuildLinkMessage(RTNLMessage::kModeAdd));
  message->set_link_status(RTNLMessage::LinkStatus(0, IFF_LOWER_UP, 0));
  SendMessageToDeviceInfo(*message);

  SetSockets();

  const int kFd = 99;
  EXPECT_CALL(*mock_sockets_, Socket(PF_INET, SOCK_DGRAM, 0))
      .WillRepeatedly(Return(kFd));

  IPAddress ip_address(IPAddress::kFamilyIPv4);
  EXPECT_TRUE(ip_address.SetAddressFromString(kTestIPAddress0));

  static uint8_t kZeroMacAddress[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  struct arpreq zero_areq_response;
  memcpy(zero_areq_response.arp_ha.sa_data, kZeroMacAddress,
         sizeof(kZeroMacAddress));

  static uint8_t kMacAddress[] = {0x01, 0x02, 0x03, 0xaa, 0xbb, 0xcc};
  struct arpreq areq_response;
  memcpy(areq_response.arp_ha.sa_data, kMacAddress, sizeof(kMacAddress));

  EXPECT_CALL(*mock_sockets_, Ioctl(
      kFd, SIOCGARP, ArpreqEquals(kTestDeviceName, ip_address)))
          .WillOnce(DoAll(SetArpreq(zero_areq_response), Return(0)))
          .WillOnce(DoAll(SetArpreq(areq_response), Return(0)));

  ByteString mac_address;
  EXPECT_FALSE(device_info_.GetMACAddressOfPeer(
      kTestDeviceIndex, ip_address, &mac_address));
  EXPECT_TRUE(device_info_.GetMACAddressOfPeer(
      kTestDeviceIndex, ip_address, &mac_address));
  EXPECT_THAT(kMacAddress,
              ElementsAreArray(mac_address.GetData(), sizeof(kMacAddress)));
}

TEST_F(DeviceInfoTest, IPv6AddressChanged) {
  scoped_refptr<MockDevice> device(new MockDevice(
      &control_interface_, &dispatcher_, &metrics_, &manager_,
      "null0", "addr0", kTestDeviceIndex));

  // Device info entry does not exist.
  EXPECT_FALSE(device_info_.GetPrimaryIPv6Address(kTestDeviceIndex, nullptr));

  device_info_.infos_[kTestDeviceIndex].device = device;

  // Device info entry contains no addresses.
  EXPECT_FALSE(device_info_.GetPrimaryIPv6Address(kTestDeviceIndex, nullptr));

  IPAddress ipv4_address(IPAddress::kFamilyIPv4);
  EXPECT_TRUE(ipv4_address.SetAddressFromString(kTestIPAddress0));
  unique_ptr<RTNLMessage> message(
      BuildAddressMessage(RTNLMessage::kModeAdd, ipv4_address, 0, 0));

  EXPECT_CALL(*device, OnIPv6AddressChanged()).Times(0);

  // We should ignore IPv4 addresses.
  SendMessageToDeviceInfo(*message);
  EXPECT_FALSE(device_info_.GetPrimaryIPv6Address(kTestDeviceIndex, nullptr));

  IPAddress ipv6_address1(IPAddress::kFamilyIPv6);
  EXPECT_TRUE(ipv6_address1.SetAddressFromString(kTestIPAddress1));
  message.reset(BuildAddressMessage(
      RTNLMessage::kModeAdd, ipv6_address1, 0, RT_SCOPE_LINK));

  // We should ignore non-SCOPE_UNIVERSE messages for IPv6.
  SendMessageToDeviceInfo(*message);
  EXPECT_FALSE(device_info_.GetPrimaryIPv6Address(kTestDeviceIndex, nullptr));

  Mock::VerifyAndClearExpectations(device.get());
  IPAddress ipv6_address2(IPAddress::kFamilyIPv6);
  EXPECT_TRUE(ipv6_address2.SetAddressFromString(kTestIPAddress2));
  message.reset(BuildAddressMessage(
      RTNLMessage::kModeAdd, ipv6_address2, IFA_F_TEMPORARY,
      RT_SCOPE_UNIVERSE));

  // Add a temporary address.
  EXPECT_CALL(*device, OnIPv6AddressChanged());
  SendMessageToDeviceInfo(*message);
  IPAddress address0(IPAddress::kFamilyUnknown);
  EXPECT_TRUE(device_info_.GetPrimaryIPv6Address(kTestDeviceIndex, &address0));
  EXPECT_TRUE(address0.Equals(ipv6_address2));
  Mock::VerifyAndClearExpectations(device.get());

  IPAddress ipv6_address3(IPAddress::kFamilyIPv6);
  EXPECT_TRUE(ipv6_address3.SetAddressFromString(kTestIPAddress3));
  message.reset(BuildAddressMessage(
      RTNLMessage::kModeAdd, ipv6_address3, 0, RT_SCOPE_UNIVERSE));

  // Adding a non-temporary address alerts the Device, but does not override
  // the primary address since the previous one was temporary.
  EXPECT_CALL(*device, OnIPv6AddressChanged());
  SendMessageToDeviceInfo(*message);
  IPAddress address1(IPAddress::kFamilyUnknown);
  EXPECT_TRUE(device_info_.GetPrimaryIPv6Address(kTestDeviceIndex, &address1));
  EXPECT_TRUE(address1.Equals(ipv6_address2));
  Mock::VerifyAndClearExpectations(device.get());

  IPAddress ipv6_address4(IPAddress::kFamilyIPv6);
  EXPECT_TRUE(ipv6_address4.SetAddressFromString(kTestIPAddress4));
  message.reset(BuildAddressMessage(
      RTNLMessage::kModeAdd, ipv6_address4, IFA_F_TEMPORARY | IFA_F_DEPRECATED,
      RT_SCOPE_UNIVERSE));

  // Adding a temporary deprecated address alerts the Device, but does not
  // override the primary address since the previous one was non-deprecated.
  EXPECT_CALL(*device, OnIPv6AddressChanged());
  SendMessageToDeviceInfo(*message);
  IPAddress address2(IPAddress::kFamilyUnknown);
  EXPECT_TRUE(device_info_.GetPrimaryIPv6Address(kTestDeviceIndex, &address2));
  EXPECT_TRUE(address2.Equals(ipv6_address2));
  Mock::VerifyAndClearExpectations(device.get());

  IPAddress ipv6_address7(IPAddress::kFamilyIPv6);
  EXPECT_TRUE(ipv6_address7.SetAddressFromString(kTestIPAddress7));
  message.reset(BuildAddressMessage(
      RTNLMessage::kModeAdd, ipv6_address7, IFA_F_TEMPORARY,
      RT_SCOPE_UNIVERSE));

  // Another temporary (non-deprecated) address alerts the Device, and will
  // override the previous primary address.
  EXPECT_CALL(*device, OnIPv6AddressChanged());
  SendMessageToDeviceInfo(*message);
  IPAddress address3(IPAddress::kFamilyUnknown);
  EXPECT_TRUE(device_info_.GetPrimaryIPv6Address(kTestDeviceIndex, &address3));
  EXPECT_TRUE(address3.Equals(ipv6_address7));
}


TEST_F(DeviceInfoTest, IPv6DnsServerAddressesChanged) {
  scoped_refptr<MockDevice> device(new MockDevice(
      &control_interface_, &dispatcher_, &metrics_, &manager_,
      "null0", "addr0", kTestDeviceIndex));
  device_info_.time_ = &time_;
  vector<IPAddress> dns_server_addresses_out;
  uint32_t lifetime_out;

  // Device info entry does not exist.
  EXPECT_FALSE(device_info_.GetIPv6DnsServerAddresses(
      kTestDeviceIndex, &dns_server_addresses_out, &lifetime_out));

  device_info_.infos_[kTestDeviceIndex].device = device;

  // Device info entry contains no IPv6 dns server addresses.
  EXPECT_FALSE(device_info_.GetIPv6DnsServerAddresses(
      kTestDeviceIndex, &dns_server_addresses_out, &lifetime_out));

  // Setup IPv6 dns server addresses.
  IPAddress ipv6_address1(IPAddress::kFamilyIPv6);
  IPAddress ipv6_address2(IPAddress::kFamilyIPv6);
  EXPECT_TRUE(ipv6_address1.SetAddressFromString(kTestIPAddress1));
  EXPECT_TRUE(ipv6_address2.SetAddressFromString(kTestIPAddress2));
  vector<IPAddress> dns_server_addresses_in;
  dns_server_addresses_in.push_back(ipv6_address1);
  dns_server_addresses_in.push_back(ipv6_address2);

  // Infinite lifetime
  const uint32_t kInfiniteLifetime = 0xffffffff;
  unique_ptr<RTNLMessage> message(BuildRdnssMessage(
      RTNLMessage::kModeAdd, kInfiniteLifetime, dns_server_addresses_in));
  EXPECT_CALL(time_, GetSecondsBoottime(_)).
      WillOnce(DoAll(SetArgPointee<0>(0), Return(true)));
  EXPECT_CALL(*device, OnIPv6DnsServerAddressesChanged()).Times(1);
  SendMessageToDeviceInfo(*message);
  EXPECT_CALL(time_, GetSecondsBoottime(_)).Times(0);
  EXPECT_TRUE(device_info_.GetIPv6DnsServerAddresses(
      kTestDeviceIndex, &dns_server_addresses_out, &lifetime_out));
  // Verify addresses and lifetime.
  EXPECT_EQ(kInfiniteLifetime, lifetime_out);
  EXPECT_EQ(2, dns_server_addresses_out.size());
  EXPECT_EQ(kTestIPAddress1, dns_server_addresses_out.at(0).ToString());
  EXPECT_EQ(kTestIPAddress2, dns_server_addresses_out.at(1).ToString());

  // Lifetime of 120, retrieve DNS server addresses after 10 seconds.
  const uint32_t kLifetime120 = 120;
  const uint32_t kElapseTime10 = 10;
  unique_ptr<RTNLMessage> message1(BuildRdnssMessage(
      RTNLMessage::kModeAdd, kLifetime120, dns_server_addresses_in));
  EXPECT_CALL(time_, GetSecondsBoottime(_)).
      WillOnce(DoAll(SetArgPointee<0>(0), Return(true)));
  EXPECT_CALL(*device, OnIPv6DnsServerAddressesChanged()).Times(1);
  SendMessageToDeviceInfo(*message1);
  // 10 seconds passed when GetIPv6DnsServerAddreses is called.
  EXPECT_CALL(time_, GetSecondsBoottime(_)).
      WillOnce(DoAll(SetArgPointee<0>(kElapseTime10), Return(true)));
  EXPECT_TRUE(device_info_.GetIPv6DnsServerAddresses(
      kTestDeviceIndex, &dns_server_addresses_out, &lifetime_out));
  // Verify addresses and lifetime.
  EXPECT_EQ(kLifetime120 - kElapseTime10, lifetime_out);
  EXPECT_EQ(2, dns_server_addresses_out.size());
  EXPECT_EQ(kTestIPAddress1, dns_server_addresses_out.at(0).ToString());
  EXPECT_EQ(kTestIPAddress2, dns_server_addresses_out.at(1).ToString());

  // Lifetime of 120, retrieve DNS server addresses after lifetime expired.
  EXPECT_CALL(time_, GetSecondsBoottime(_)).
      WillOnce(DoAll(SetArgPointee<0>(0), Return(true)));
  EXPECT_CALL(*device, OnIPv6DnsServerAddressesChanged()).Times(1);
  SendMessageToDeviceInfo(*message1);
  // 120 seconds passed when GetIPv6DnsServerAddreses is called.
  EXPECT_CALL(time_, GetSecondsBoottime(_)).
      WillOnce(DoAll(SetArgPointee<0>(kLifetime120), Return(true)));
  EXPECT_TRUE(device_info_.GetIPv6DnsServerAddresses(
      kTestDeviceIndex, &dns_server_addresses_out, &lifetime_out));
  // Verify addresses and lifetime.
  EXPECT_EQ(0, lifetime_out);
  EXPECT_EQ(2, dns_server_addresses_out.size());
  EXPECT_EQ(kTestIPAddress1, dns_server_addresses_out.at(0).ToString());
  EXPECT_EQ(kTestIPAddress2, dns_server_addresses_out.at(1).ToString());
}

class DeviceInfoTechnologyTest : public DeviceInfoTest {
 public:
  DeviceInfoTechnologyTest()
      : DeviceInfoTest(),
        test_device_name_(kTestDeviceName) {}
  virtual ~DeviceInfoTechnologyTest() {}

  virtual void SetUp() {
    CHECK(temp_dir_.CreateUniqueTempDir());
    device_info_root_ = temp_dir_.path().Append("sys/class/net");
    device_info_.device_info_root_ = device_info_root_;
    // Most tests require that the uevent file exist.
    CreateInfoFile("uevent", "xxx");
  }

  Technology::Identifier GetDeviceTechnology() {
    return device_info_.GetDeviceTechnology(test_device_name_);
  }
  FilePath GetInfoPath(const string& name);
  void CreateInfoFile(const string& name, const string& contents);
  void CreateInfoSymLink(const string& name, const string& contents);
  void SetDeviceName(const string& name) {
    test_device_name_ = name;
    EXPECT_TRUE(temp_dir_.Delete());  // nuke old temp dir
    SetUp();
  }

 protected:
  base::ScopedTempDir temp_dir_;
  FilePath device_info_root_;
  string test_device_name_;
};

FilePath DeviceInfoTechnologyTest::GetInfoPath(const string& name) {
  return device_info_root_.Append(test_device_name_).Append(name);
}

void DeviceInfoTechnologyTest::CreateInfoFile(const string& name,
                                              const string& contents) {
  FilePath info_path = GetInfoPath(name);
  EXPECT_TRUE(base::CreateDirectory(info_path.DirName()));
  string contents_newline(contents + "\n");
  EXPECT_TRUE(base::WriteFile(info_path, contents_newline.c_str(),
                              contents_newline.size()));
}

void DeviceInfoTechnologyTest::CreateInfoSymLink(const string& name,
                                                 const string& contents) {
  FilePath info_path = GetInfoPath(name);
  EXPECT_TRUE(base::CreateDirectory(info_path.DirName()));
  EXPECT_TRUE(base::CreateSymbolicLink(FilePath(contents), info_path));
}

TEST_F(DeviceInfoTechnologyTest, Unknown) {
  // With a uevent file but no driver symlink, we should get a pseudo-technology
  // which specifies this condition explicitly.
  EXPECT_EQ(Technology::kNoDeviceSymlink, GetDeviceTechnology());

  // Should be unknown without a uevent file.
  EXPECT_TRUE(base::DeleteFile(GetInfoPath("uevent"), false));
  EXPECT_EQ(Technology::kUnknown, GetDeviceTechnology());
}

TEST_F(DeviceInfoTechnologyTest, IgnoredPrefix) {
  test_device_name_ = "veth0";
  // A new uevent file is needed since the device name has changed.
  CreateInfoFile("uevent", "xxx");
  // A device with a "veth" prefix should be ignored.
  EXPECT_EQ(Technology::kUnknown, GetDeviceTechnology());
}

TEST_F(DeviceInfoTechnologyTest, Loopback) {
  CreateInfoFile("type", base::IntToString(ARPHRD_LOOPBACK));
  EXPECT_EQ(Technology::kLoopback, GetDeviceTechnology());
}

TEST_F(DeviceInfoTechnologyTest, PPP) {
  CreateInfoFile("type", base::IntToString(ARPHRD_PPP));
  EXPECT_EQ(Technology::kPPP, GetDeviceTechnology());
}

TEST_F(DeviceInfoTechnologyTest, Tunnel) {
  CreateInfoFile("tun_flags", base::IntToString(IFF_TUN));
  EXPECT_EQ(Technology::kTunnel, GetDeviceTechnology());
}

TEST_F(DeviceInfoTechnologyTest, WiFi) {
  CreateInfoFile("uevent", "DEVTYPE=wlan");
  EXPECT_EQ(Technology::kWifi, GetDeviceTechnology());
  CreateInfoFile("uevent", "foo\nDEVTYPE=wlan");
  EXPECT_EQ(Technology::kWifi, GetDeviceTechnology());
  CreateInfoFile("type", base::IntToString(ARPHRD_IEEE80211_RADIOTAP));
  EXPECT_EQ(Technology::kWiFiMonitor, GetDeviceTechnology());
}

TEST_F(DeviceInfoTechnologyTest, Ethernet) {
  CreateInfoSymLink("device/driver", "xxx");
  EXPECT_EQ(Technology::kEthernet, GetDeviceTechnology());
}

TEST_F(DeviceInfoTechnologyTest, WiMax) {
  CreateInfoSymLink("device/driver", "gdm_wimax");
  EXPECT_EQ(Technology::kWiMax, GetDeviceTechnology());
}

TEST_F(DeviceInfoTechnologyTest, CellularGobi1) {
  CreateInfoSymLink("device/driver", "blah/foo/gobi");
  EXPECT_EQ(Technology::kCellular, GetDeviceTechnology());
}

TEST_F(DeviceInfoTechnologyTest, CellularGobi2) {
  CreateInfoSymLink("device/driver", "../GobiNet");
  EXPECT_EQ(Technology::kCellular, GetDeviceTechnology());
}

TEST_F(DeviceInfoTechnologyTest, QCUSB) {
  CreateInfoSymLink("device/driver", "QCUSBNet2k");
  EXPECT_EQ(Technology::kCellular, GetDeviceTechnology());
}

TEST_F(DeviceInfoTechnologyTest, CellularCdcMbim) {
  CreateInfoSymLink("device/driver", "cdc_mbim");
  EXPECT_EQ(Technology::kCellular, GetDeviceTechnology());
}

TEST_F(DeviceInfoTechnologyTest, CellularQmiWwan) {
  CreateInfoSymLink("device/driver", "qmi_wwan");
  EXPECT_EQ(Technology::kCellular, GetDeviceTechnology());
}

// Modem with absolute driver path with top-level tty file:
//   /sys/class/net/dev0/device -> /sys/devices/virtual/0/00
//   /sys/devices/virtual/0/00/driver -> /drivers/cdc_ether or /drivers/cdc_ncm
//   /sys/devices/virtual/0/01/tty [empty directory]
TEST_F(DeviceInfoTechnologyTest, CDCEthernetModem1) {
  FilePath device_root(temp_dir_.path().Append("sys/devices/virtual/0"));
  FilePath device_path(device_root.Append("00"));
  FilePath driver_symlink(device_path.Append("driver"));
  EXPECT_TRUE(base::CreateDirectory(device_path));
  CreateInfoSymLink("device", device_path.value());
  EXPECT_TRUE(base::CreateSymbolicLink(FilePath("/drivers/cdc_ether"),
                                       driver_symlink));
  EXPECT_TRUE(base::CreateDirectory(device_root.Append("01/tty")));
  EXPECT_EQ(Technology::kCellular, GetDeviceTechnology());

  EXPECT_TRUE(base::DeleteFile(driver_symlink, false));
  EXPECT_TRUE(base::CreateSymbolicLink(FilePath("/drivers/cdc_ncm"),
                                       driver_symlink));
  EXPECT_EQ(Technology::kCellular, GetDeviceTechnology());
}

// Modem with relative driver path with top-level tty file.
//   /sys/class/net/dev0/device -> ../../../device_dir/0/00
//   /sys/device_dir/0/00/driver -> /drivers/cdc_ether or /drivers/cdc_ncm
//   /sys/device_dir/0/01/tty [empty directory]
TEST_F(DeviceInfoTechnologyTest, CDCEthernetModem2) {
  CreateInfoSymLink("device", "../../../device_dir/0/00");
  FilePath device_root(temp_dir_.path().Append("sys/device_dir/0"));
  FilePath device_path(device_root.Append("00"));
  FilePath driver_symlink(device_path.Append("driver"));
  EXPECT_TRUE(base::CreateDirectory(device_path));
  EXPECT_TRUE(base::CreateSymbolicLink(FilePath("/drivers/cdc_ether"),
                                       driver_symlink));
  EXPECT_TRUE(base::CreateDirectory(device_root.Append("01/tty")));
  EXPECT_EQ(Technology::kCellular, GetDeviceTechnology());

  EXPECT_TRUE(base::DeleteFile(driver_symlink, false));
  EXPECT_TRUE(base::CreateSymbolicLink(FilePath("/drivers/cdc_ncm"),
                                       driver_symlink));
  EXPECT_EQ(Technology::kCellular, GetDeviceTechnology());
}

// Modem with relative driver path with lower-level tty file.
//   /sys/class/net/dev0/device -> ../../../device_dir/0/00
//   /sys/device_dir/0/00/driver -> /drivers/cdc_ether or /drivers/cdc_ncm
//   /sys/device_dir/0/01/yyy/tty [empty directory]
TEST_F(DeviceInfoTechnologyTest, CDCEthernetModem3) {
  CreateInfoSymLink("device", "../../../device_dir/0/00");
  FilePath device_root(temp_dir_.path().Append("sys/device_dir/0"));
  FilePath device_path(device_root.Append("00"));
  FilePath driver_symlink(device_path.Append("driver"));
  EXPECT_TRUE(base::CreateDirectory(device_path));
  EXPECT_TRUE(base::CreateSymbolicLink(FilePath("/drivers/cdc_ether"),
                                       driver_symlink));
  EXPECT_TRUE(base::CreateDirectory(device_root.Append("01/yyy/tty")));
  EXPECT_EQ(Technology::kCellular, GetDeviceTechnology());

  EXPECT_TRUE(base::DeleteFile(driver_symlink, false));
  EXPECT_TRUE(base::CreateSymbolicLink(FilePath("/drivers/cdc_ncm"),
                                       driver_symlink));
  EXPECT_EQ(Technology::kCellular, GetDeviceTechnology());
}

TEST_F(DeviceInfoTechnologyTest, CDCEtherNonModem) {
  CreateInfoSymLink("device", "device_dir");
  CreateInfoSymLink("device_dir/driver", "cdc_ether");
  EXPECT_EQ(Technology::kCDCEthernet, GetDeviceTechnology());
}

TEST_F(DeviceInfoTechnologyTest, CDCNcmNonModem) {
  CreateInfoSymLink("device", "device_dir");
  CreateInfoSymLink("device_dir/driver", "cdc_ncm");
  EXPECT_EQ(Technology::kCDCEthernet, GetDeviceTechnology());
}

TEST_F(DeviceInfoTechnologyTest, PseudoModem) {
  SetDeviceName("pseudomodem");
  CreateInfoSymLink("device", "device_dir");
  CreateInfoSymLink("device_dir/driver", "cdc_ether");
  EXPECT_EQ(Technology::kCellular, GetDeviceTechnology());

  SetDeviceName("pseudomodem9");
  CreateInfoSymLink("device", "device_dir");
  CreateInfoSymLink("device_dir/driver", "cdc_ether");
  EXPECT_EQ(Technology::kCellular, GetDeviceTechnology());
}

class DeviceInfoForDelayedCreationTest : public DeviceInfo {
 public:
  DeviceInfoForDelayedCreationTest(ControlInterface* control_interface,
                                   EventDispatcher* dispatcher,
                                   Metrics* metrics,
                                   Manager* manager)
      : DeviceInfo(control_interface, dispatcher, metrics, manager) {}
  MOCK_METHOD4(CreateDevice, DeviceRefPtr(const std::string& link_name,
                                          const std::string& address,
                                          int interface_index,
                                          Technology::Identifier technology));
  MOCK_METHOD1(GetDeviceTechnology,
               Technology::Identifier(const string& iface_name));
};

class DeviceInfoDelayedCreationTest : public DeviceInfoTest {
 public:
  DeviceInfoDelayedCreationTest()
      : DeviceInfoTest(),
        test_device_info_(
            &control_interface_, &dispatcher_, &metrics_, &manager_) {}
  virtual ~DeviceInfoDelayedCreationTest() {}

  virtual std::set<int>& GetDelayedDevices() {
    return test_device_info_.delayed_devices_;
  }

  void DelayedDeviceCreationTask() {
    test_device_info_.DelayedDeviceCreationTask();
  }

  void AddDelayedDevice(Technology::Identifier delayed_technology) {
    unique_ptr<RTNLMessage> message(BuildLinkMessage(RTNLMessage::kModeAdd));
    EXPECT_CALL(test_device_info_, GetDeviceTechnology(kTestDeviceName))
        .WillOnce(Return(delayed_technology));
    EXPECT_CALL(test_device_info_, CreateDevice(
        kTestDeviceName, _, kTestDeviceIndex, delayed_technology))
        .WillOnce(Return(DeviceRefPtr()));
    test_device_info_.AddLinkMsgHandler(*message);
    Mock::VerifyAndClearExpectations(&test_device_info_);
    // We need to insert the device index ourselves since we have mocked
    // out CreateDevice.  This insertion is tested in CreateDeviceCDCEthernet
    // above.
    GetDelayedDevices().insert(kTestDeviceIndex);
  }

  void EnsureDelayedDevice(Technology::Identifier reported_device_technology,
                           Technology::Identifier created_device_technology) {
    EXPECT_CALL(test_device_info_, GetDeviceTechnology(_))
        .WillOnce(Return(reported_device_technology));
    EXPECT_CALL(test_device_info_, CreateDevice(
        kTestDeviceName, _, kTestDeviceIndex, created_device_technology))
        .WillOnce(Return(DeviceRefPtr()));
    DelayedDeviceCreationTask();
    EXPECT_TRUE(GetDelayedDevices().empty());
  }

#if !defined(DISABLE_WIFI)
  void TriggerOnWiFiInterfaceInfoReceived(const Nl80211Message& message) {
    test_device_info_.OnWiFiInterfaceInfoReceived(message);
  }
#endif  // DISABLE_WIFI

 protected:
  DeviceInfoForDelayedCreationTest test_device_info_;
};

TEST_F(DeviceInfoDelayedCreationTest, NoDevices) {
  EXPECT_TRUE(GetDelayedDevices().empty());
  EXPECT_CALL(test_device_info_, GetDeviceTechnology(_)).Times(0);
  DelayedDeviceCreationTask();
}

TEST_F(DeviceInfoDelayedCreationTest, CDCEthernetDevice) {
  AddDelayedDevice(Technology::kCDCEthernet);
  EnsureDelayedDevice(Technology::kCDCEthernet, Technology::kEthernet);
}

TEST_F(DeviceInfoDelayedCreationTest, CellularDevice) {
  AddDelayedDevice(Technology::kCDCEthernet);
  EnsureDelayedDevice(Technology::kCellular, Technology::kCellular);
}

TEST_F(DeviceInfoDelayedCreationTest, TunnelDevice) {
  AddDelayedDevice(Technology::kNoDeviceSymlink);
  EnsureDelayedDevice(Technology::kTunnel, Technology::kTunnel);
}

TEST_F(DeviceInfoDelayedCreationTest, NoDeviceSymlinkEthernet) {
  AddDelayedDevice(Technology::kNoDeviceSymlink);
  EXPECT_CALL(manager_, ignore_unknown_ethernet())
      .WillOnce(Return(false));
  EnsureDelayedDevice(Technology::kNoDeviceSymlink, Technology::kEthernet);
}

TEST_F(DeviceInfoDelayedCreationTest, NoDeviceSymlinkIgnored) {
  AddDelayedDevice(Technology::kNoDeviceSymlink);
  EXPECT_CALL(manager_, ignore_unknown_ethernet())
      .WillOnce(Return(true));
  EnsureDelayedDevice(Technology::kNoDeviceSymlink, Technology::kUnknown);
}

#if !defined(DISABLE_WIFI)
TEST_F(DeviceInfoDelayedCreationTest, WiFiDevice) {
  ScopedMockLog log;
  EXPECT_CALL(log, Log(logging::LOG_ERROR, _,
                       HasSubstr("Message is not a new interface response")));
  GetInterfaceMessage non_interface_response_message;
  TriggerOnWiFiInterfaceInfoReceived(non_interface_response_message);
  Mock::VerifyAndClearExpectations(&log);

  EXPECT_CALL(log, Log(logging::LOG_ERROR, _,
                       HasSubstr("Message contains no interface index")));
  NewInterfaceMessage message;
  TriggerOnWiFiInterfaceInfoReceived(message);
  Mock::VerifyAndClearExpectations(&log);

  message.attributes()->CreateNl80211Attribute(
      NL80211_ATTR_IFINDEX, NetlinkMessage::MessageContext());
  message.attributes()->SetU32AttributeValue(NL80211_ATTR_IFINDEX,
                                             kTestDeviceIndex);
  EXPECT_CALL(log, Log(logging::LOG_ERROR, _,
                       HasSubstr("Message contains no interface type")));
  TriggerOnWiFiInterfaceInfoReceived(message);
  Mock::VerifyAndClearExpectations(&log);

  message.attributes()->CreateNl80211Attribute(
      NL80211_ATTR_IFTYPE, NetlinkMessage::MessageContext());
  message.attributes()->SetU32AttributeValue(NL80211_ATTR_IFTYPE,
                                             NL80211_IFTYPE_AP);
  EXPECT_CALL(log, Log(logging::LOG_ERROR, _,
                       HasSubstr("Could not find device info for interface")));
  TriggerOnWiFiInterfaceInfoReceived(message);
  Mock::VerifyAndClearExpectations(&log);

  // Use the AddDelayedDevice() method to create a device info entry with no
  // associated device.
  AddDelayedDevice(Technology::kNoDeviceSymlink);

  EXPECT_CALL(log, Log(logging::LOG_INFO, _,
                       HasSubstr("it is not in station mode")));
  TriggerOnWiFiInterfaceInfoReceived(message);
  Mock::VerifyAndClearExpectations(&log);
  Mock::VerifyAndClearExpectations(&manager_);

  message.attributes()->SetU32AttributeValue(NL80211_ATTR_IFTYPE,
                                             NL80211_IFTYPE_STATION);
  EXPECT_CALL(manager_, RegisterDevice(_));
  EXPECT_CALL(manager_, device_info())
      .WillRepeatedly(Return(&test_device_info_));
  EXPECT_CALL(log, Log(_, _, _)).Times(AnyNumber());
  EXPECT_CALL(log, Log(logging::LOG_INFO, _,
                       HasSubstr("Creating WiFi device")));
  TriggerOnWiFiInterfaceInfoReceived(message);
  Mock::VerifyAndClearExpectations(&log);
  Mock::VerifyAndClearExpectations(&manager_);

  EXPECT_CALL(manager_, RegisterDevice(_)).Times(0);
  EXPECT_CALL(log, Log(logging::LOG_ERROR, _,
                       HasSubstr("Device already created for interface")));
  TriggerOnWiFiInterfaceInfoReceived(message);
}
#endif  // DISABLE_WIFI

}  // namespace shill
