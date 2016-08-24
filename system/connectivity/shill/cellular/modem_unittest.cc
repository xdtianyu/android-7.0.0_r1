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

#include "shill/cellular/modem.h"

#include <vector>

#include <base/strings/stringprintf.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <mm/mm-modem.h>
#include <net/if.h>
#include <sys/ioctl.h>

#include "shill/cellular/cellular.h"
#include "shill/cellular/cellular_capability_gsm.h"
#include "shill/cellular/mock_cellular.h"
#include "shill/cellular/mock_modem.h"
#include "shill/cellular/mock_modem_info.h"
#include "shill/manager.h"
#include "shill/mock_device_info.h"
#include "shill/net/mock_rtnl_handler.h"
#include "shill/net/rtnl_handler.h"
#include "shill/test_event_dispatcher.h"

using std::string;
using std::vector;
using testing::_;
using testing::AnyNumber;
using testing::DoAll;
using testing::Return;
using testing::SetArgumentPointee;
using testing::StrEq;
using testing::StrictMock;
using testing::Test;

namespace shill {

namespace {

const int kTestInterfaceIndex = 5;
const char kLinkName[] = "usb0";
const char kService[] = "org.chromium.ModemManager";
const char kPath[] = "/org/chromium/ModemManager/Gobi/0";
const unsigned char kAddress[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05};
const char kAddressAsString[] = "000102030405";

}  // namespace

class ModemTest : public Test {
 public:
  ModemTest()
      : modem_info_(nullptr, &dispatcher_, nullptr, nullptr),
        device_info_(modem_info_.control_interface(), modem_info_.dispatcher(),
                     modem_info_.metrics(), modem_info_.manager()),
        modem_(
            new StrictModem(
                kService,
                kPath,
                &modem_info_,
                nullptr)) {}
  virtual void SetUp();
  virtual void TearDown();

  void ReplaceSingletons() {
    modem_->rtnl_handler_ = &rtnl_handler_;
  }

 protected:
  EventDispatcherForTest dispatcher_;
  MockModemInfo modem_info_;
  MockDeviceInfo device_info_;
  std::unique_ptr<StrictModem> modem_;
  MockRTNLHandler rtnl_handler_;
  ByteString expected_address_;
};

void ModemTest::SetUp() {
  EXPECT_EQ(kService, modem_->service_);
  EXPECT_EQ(kPath, modem_->path_);
  ReplaceSingletons();
  expected_address_ = ByteString(kAddress, arraysize(kAddress));

  EXPECT_CALL(rtnl_handler_, GetInterfaceIndex(kLinkName)).
      WillRepeatedly(Return(kTestInterfaceIndex));

  EXPECT_CALL(*modem_info_.mock_manager(), device_info())
      .WillRepeatedly(Return(&device_info_));
}

void ModemTest::TearDown() {
  modem_.reset();
}

MATCHER_P2(HasPropertyWithValueU32, key, value, "") {
  return arg.ContainsUint(key) && value == arg.GetUint(key);
}

TEST_F(ModemTest, PendingDevicePropertiesAndCreate) {
  static const char kSentinel[] = "sentinel";
  static const uint32_t kSentinelValue = 17;

  InterfaceToProperties properties;
  properties[MM_MODEM_INTERFACE].SetUint(kSentinel, kSentinelValue);

  EXPECT_CALL(*modem_, GetLinkName(_, _)).WillRepeatedly(DoAll(
      SetArgumentPointee<1>(string(kLinkName)),
      Return(true)));
  EXPECT_CALL(rtnl_handler_, GetInterfaceIndex(StrEq(kLinkName))).
      WillRepeatedly(Return(kTestInterfaceIndex));

  // The first time we call CreateDeviceFromModemProperties,
  // GetMACAddress will fail.
  EXPECT_CALL(device_info_, GetMACAddress(kTestInterfaceIndex, _)).
      WillOnce(Return(false));
  EXPECT_CALL(*modem_, GetModemInterface()).
      WillRepeatedly(Return(MM_MODEM_INTERFACE));
  modem_->CreateDeviceFromModemProperties(properties);
  EXPECT_FALSE(modem_->device_.get());

  // On the second time, we allow GetMACAddress to succeed.  Now we
  // expect a device to be built
  EXPECT_CALL(device_info_, GetMACAddress(kTestInterfaceIndex, _)).
      WillOnce(DoAll(SetArgumentPointee<1>(expected_address_),
                     Return(true)));

  // modem will take ownership
  MockCellular* cellular = new MockCellular(
      &modem_info_,
      kLinkName,
      kAddressAsString,
      kTestInterfaceIndex,
      Cellular::kTypeCDMA,
      kService,
      kPath);

  EXPECT_CALL(*modem_,
              ConstructCellular(StrEq(kLinkName),
                                StrEq(kAddressAsString),
                                kTestInterfaceIndex)).
      WillOnce(Return(cellular));

  EXPECT_CALL(*cellular, OnPropertiesChanged(
      _,
      HasPropertyWithValueU32(kSentinel, kSentinelValue),
      _));
  EXPECT_CALL(device_info_, RegisterDevice(_));
  modem_->OnDeviceInfoAvailable(kLinkName);

  EXPECT_TRUE(modem_->device_.get());

  // Add expectations for the eventual |modem_| destruction.
  EXPECT_CALL(*cellular, DestroyService());
  EXPECT_CALL(device_info_, DeregisterDevice(_));
}

TEST_F(ModemTest, EarlyDeviceProperties) {
  // OnDeviceInfoAvailable called before
  // CreateDeviceFromModemProperties: Do nothing
  modem_->OnDeviceInfoAvailable(kLinkName);
  EXPECT_FALSE(modem_->device_.get());
}

TEST_F(ModemTest, CreateDeviceEarlyFailures) {
  InterfaceToProperties properties;

  EXPECT_CALL(*modem_, ConstructCellular(_, _, _)).Times(0);
  EXPECT_CALL(*modem_, GetModemInterface()).
      WillRepeatedly(Return(MM_MODEM_INTERFACE));

  // No modem interface properties:  no device created
  modem_->CreateDeviceFromModemProperties(properties);
  EXPECT_FALSE(modem_->device_.get());

  properties[MM_MODEM_INTERFACE] = KeyValueStore();

  // Link name, but no ifindex: no device created
  EXPECT_CALL(*modem_, GetLinkName(_, _)).WillOnce(DoAll(
      SetArgumentPointee<1>(string(kLinkName)),
      Return(true)));
  EXPECT_CALL(rtnl_handler_, GetInterfaceIndex(StrEq(kLinkName))).WillOnce(
      Return(-1));
  modem_->CreateDeviceFromModemProperties(properties);
  EXPECT_FALSE(modem_->device_.get());

  // The params are good, but the device is blacklisted.
  EXPECT_CALL(*modem_, GetLinkName(_, _)).WillOnce(DoAll(
      SetArgumentPointee<1>(string(kLinkName)),
      Return(true)));
  EXPECT_CALL(rtnl_handler_, GetInterfaceIndex(StrEq(kLinkName)))
      .WillOnce(Return(kTestInterfaceIndex));
  EXPECT_CALL(device_info_, GetMACAddress(kTestInterfaceIndex, _))
      .WillOnce(DoAll(SetArgumentPointee<1>(expected_address_),
                      Return(true)));
  EXPECT_CALL(device_info_, IsDeviceBlackListed(kLinkName))
      .WillRepeatedly(Return(true));
  modem_->CreateDeviceFromModemProperties(properties);
  EXPECT_FALSE(modem_->device_.get());

  // No link name: see CreateDevicePPP.
}

TEST_F(ModemTest, CreateDevicePPP) {
  InterfaceToProperties properties;
  properties[MM_MODEM_INTERFACE] = KeyValueStore();

  string dev_name(
      base::StringPrintf(Modem::kFakeDevNameFormat, Modem::fake_dev_serial_));

  // |modem_| will take ownership.
  MockCellular* cellular = new MockCellular(
      &modem_info_,
      dev_name,
      Modem::kFakeDevAddress,
      Modem::kFakeDevInterfaceIndex,
      Cellular::kTypeUniversal,
      kService,
      kPath);

  EXPECT_CALL(*modem_, GetModemInterface()).
      WillRepeatedly(Return(MM_MODEM_INTERFACE));
  // No link name: assumed to be a PPP dongle.
  EXPECT_CALL(*modem_, GetLinkName(_, _)).WillOnce(Return(false));
  EXPECT_CALL(*modem_,
              ConstructCellular(dev_name,
                                StrEq(Modem::kFakeDevAddress),
                                Modem::kFakeDevInterfaceIndex)).
      WillOnce(Return(cellular));
  EXPECT_CALL(device_info_, RegisterDevice(_));

  modem_->CreateDeviceFromModemProperties(properties);
  EXPECT_TRUE(modem_->device_.get());

  // Add expectations for the eventual |modem_| destruction.
  EXPECT_CALL(*cellular, DestroyService());
  EXPECT_CALL(device_info_, DeregisterDevice(_));
}

TEST_F(ModemTest, GetDeviceParams) {
  string mac_address;
  int interface_index = 2;
  EXPECT_CALL(rtnl_handler_, GetInterfaceIndex(_)).WillOnce(Return(-1));
  EXPECT_CALL(device_info_, GetMACAddress(_, _)).Times(AnyNumber())
      .WillRepeatedly(Return(false));
  EXPECT_FALSE(modem_->GetDeviceParams(&mac_address, &interface_index));
  EXPECT_EQ(-1, interface_index);

  EXPECT_CALL(rtnl_handler_, GetInterfaceIndex(_)).WillOnce(Return(-2));
  EXPECT_CALL(device_info_, GetMACAddress(_, _)).Times(AnyNumber())
      .WillRepeatedly(Return(false));
  EXPECT_FALSE(modem_->GetDeviceParams(&mac_address, &interface_index));
  EXPECT_EQ(-2, interface_index);

  EXPECT_CALL(rtnl_handler_, GetInterfaceIndex(_)).WillOnce(Return(1));
  EXPECT_CALL(device_info_, GetMACAddress(_, _)).WillOnce(Return(false));
  EXPECT_FALSE(modem_->GetDeviceParams(&mac_address, &interface_index));
  EXPECT_EQ(1, interface_index);

  EXPECT_CALL(rtnl_handler_, GetInterfaceIndex(_)).WillOnce(Return(2));
  EXPECT_CALL(device_info_, GetMACAddress(2, _)).
      WillOnce(DoAll(SetArgumentPointee<1>(expected_address_),
                     Return(true)));
  EXPECT_TRUE(modem_->GetDeviceParams(&mac_address, &interface_index));
  EXPECT_EQ(2, interface_index);
  EXPECT_EQ(kAddressAsString, mac_address);
}

TEST_F(ModemTest, RejectPPPModem) {
  // TODO(rochberg):  Port this to ModemClassic
}

}  // namespace shill
