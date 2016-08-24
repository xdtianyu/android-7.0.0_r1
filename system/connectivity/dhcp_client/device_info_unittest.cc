//
// Copyright (C) 2015 The Android Open Source Project
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

#include <dhcp_client/device_info.h>

#include <net/if.h>
#include <sys/ioctl.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <shill/net/byte_string.h>
#include <shill/net/mock_sockets.h>
#include <shill/net/mock_rtnl_handler.h>

using shill::ByteString;
using shill::MockRTNLHandler;
using shill::MockSockets;
using ::testing::_;
using ::testing::DoAll;
using ::testing::ElementsAreArray;
using ::testing::Return;

namespace {

const int kFakeFd = 99;
const unsigned int kFakeInterfaceIndex = 1;
const std::string kFakeDeviceName = "eth0";
const std::string kFakeLongDeviceName = "a_long_device_name";
const uint8_t kFakeMacAddress[] = {0x00, 0x01, 0x02, 0xaa, 0xbb, 0xcc};

}

namespace dhcp_client {

class DeviceInfoTest : public testing::Test {
 public:
  DeviceInfoTest() {}

  void SetUp() {
    device_info_ = DeviceInfo::GetInstance();
    sockets_ = new MockSockets();
    device_info_->sockets_.reset(sockets_);
    device_info_->rtnl_handler_ = &rtnl_handler_;
  }

 protected:
  DeviceInfo* device_info_;
  MockRTNLHandler rtnl_handler_;
  MockSockets* sockets_;  // Owned by device_info_.
};

ACTION_P(SetIfreq, ifr) {
  struct ifreq* const ifr_arg = static_cast<struct ifreq*>(arg2);
  *ifr_arg = ifr;
}

MATCHER_P(IfreqEquals, ifname, "") {
  const struct ifreq* const ifr = static_cast<struct ifreq*>(arg);
  return (ifr != nullptr) &&
      (strcmp(ifname, ifr->ifr_name) == 0);
}

TEST_F(DeviceInfoTest, GetDeviceInfoSucceed) {
  ByteString mac_address;
  unsigned int interface_index;
  struct ifreq ifr;
  memcpy(ifr.ifr_hwaddr.sa_data, kFakeMacAddress, sizeof(kFakeMacAddress));
  EXPECT_CALL(*sockets_, Socket(AF_INET, SOCK_DGRAM, 0))
      .WillOnce(Return(kFakeFd));
  EXPECT_CALL(*sockets_, Ioctl(kFakeFd,
                               SIOCGIFHWADDR,
                               IfreqEquals(kFakeDeviceName.c_str())))
      .WillOnce(DoAll(SetIfreq(ifr), Return(0)));
  EXPECT_CALL(rtnl_handler_, GetInterfaceIndex(kFakeDeviceName))
      .WillOnce(Return(kFakeInterfaceIndex));
  EXPECT_TRUE(device_info_->GetDeviceInfo(kFakeDeviceName,
                                          &mac_address,
                                          &interface_index));
  EXPECT_EQ(interface_index, kFakeInterfaceIndex);
  EXPECT_THAT(kFakeMacAddress,
              ElementsAreArray(mac_address.GetConstData(),
                               sizeof(kFakeMacAddress)));
}

TEST_F(DeviceInfoTest, GetDeviceInfoNameTooLong) {
  ByteString mac_address;
  unsigned int interface_index;
  EXPECT_FALSE(device_info_->GetDeviceInfo(kFakeLongDeviceName,
                                           &mac_address,
                                           &interface_index));
}

TEST_F(DeviceInfoTest, GetDeviceInfoFailedToCreateSocket) {
  ByteString mac_address;
  unsigned int interface_index;
  EXPECT_CALL(*sockets_, Socket(AF_INET, SOCK_DGRAM, 0)).WillOnce(Return(-1));
  EXPECT_FALSE(device_info_->GetDeviceInfo(kFakeDeviceName,
                                           &mac_address,
                                           &interface_index));
}

TEST_F(DeviceInfoTest, GetDeviceInfoFailedToGetHardwareAddr) {
  ByteString mac_address;
  unsigned int interface_index;
  EXPECT_CALL(*sockets_, Socket(AF_INET, SOCK_DGRAM, 0))
      .WillOnce(Return(kFakeFd));
  EXPECT_CALL(*sockets_, Ioctl(kFakeFd, SIOCGIFHWADDR, _)).WillOnce(Return(-1));
  EXPECT_FALSE(device_info_->GetDeviceInfo(kFakeDeviceName,
                                           &mac_address,
                                           &interface_index));
}

TEST_F(DeviceInfoTest, GetDeviceInfoFailedToGetInterfaceIndex) {
  ByteString mac_address;
  unsigned int interface_index;
  EXPECT_CALL(*sockets_, Socket(AF_INET, SOCK_DGRAM, 0))
      .WillOnce(Return(kFakeFd));
  EXPECT_CALL(*sockets_, Ioctl(kFakeFd, SIOCGIFHWADDR, _)).WillOnce(Return(0));
  EXPECT_CALL(rtnl_handler_, GetInterfaceIndex(kFakeDeviceName))
      .WillOnce(Return(-1));
  EXPECT_FALSE(device_info_->GetDeviceInfo(kFakeDeviceName,
                                           &mac_address,
                                           &interface_index));
}

}  // namespace dhcp_client
