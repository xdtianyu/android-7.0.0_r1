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

#include <base/files/scoped_temp_dir.h>
#include <ModemManager/ModemManager.h>

#include "shill/cellular/cellular_capability.h"
#include "shill/cellular/mock_cellular.h"
#include "shill/cellular/mock_modem_info.h"
#include "shill/manager.h"
#include "shill/mock_control.h"
#include "shill/mock_dbus_properties_proxy.h"
#include "shill/mock_device_info.h"
#include "shill/net/mock_rtnl_handler.h"
#include "shill/net/rtnl_handler.h"
#include "shill/test_event_dispatcher.h"
#include "shill/testing.h"

using base::FilePath;
using std::string;
using std::vector;
using testing::_;
using testing::DoAll;
using testing::Return;
using testing::SetArgumentPointee;
using testing::Test;

namespace shill {

namespace {

const int kTestInterfaceIndex = 5;
const char kLinkName[] = "usb0";
const char kService[] = "org.chromium.ModemManager";
const char kPath[] = "/org/chromium/ModemManager/Gobi/0";
const unsigned char kAddress[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05};

}  // namespace

class Modem1Test : public Test {
 public:
  Modem1Test()
      : modem_info_(nullptr, &dispatcher_, nullptr, nullptr),
        device_info_(modem_info_.control_interface(), modem_info_.dispatcher(),
                     modem_info_.metrics(), modem_info_.manager()),
        proxy_(new MockDBusPropertiesProxy()),
        modem_(
            new Modem1(
                kService,
                kPath,
                &modem_info_,
                &control_interface_)) {}
  virtual void SetUp();
  virtual void TearDown();

  void ReplaceSingletons() {
    modem_->rtnl_handler_ = &rtnl_handler_;
  }

 protected:
  EventDispatcherForTest dispatcher_;
  MockModemInfo modem_info_;
  MockDeviceInfo device_info_;
  std::unique_ptr<MockDBusPropertiesProxy> proxy_;
  MockControl control_interface_;
  std::unique_ptr<Modem1> modem_;
  MockRTNLHandler rtnl_handler_;
  ByteString expected_address_;
};

void Modem1Test::SetUp() {
  EXPECT_EQ(kService, modem_->service_);
  EXPECT_EQ(kPath, modem_->path_);
  ReplaceSingletons();
  expected_address_ = ByteString(kAddress, arraysize(kAddress));

  EXPECT_CALL(rtnl_handler_, GetInterfaceIndex(kLinkName)).
      WillRepeatedly(Return(kTestInterfaceIndex));

  EXPECT_CALL(*modem_info_.mock_manager(), device_info())
      .WillRepeatedly(Return(&device_info_));
  EXPECT_CALL(device_info_, GetMACAddress(kTestInterfaceIndex, _)).
      WillOnce(DoAll(SetArgumentPointee<1>(expected_address_),
                     Return(true)));
}

void Modem1Test::TearDown() {
  modem_.reset();
}

TEST_F(Modem1Test, CreateDeviceMM1) {
  InterfaceToProperties properties;

  KeyValueStore modem_properties;
  modem_properties.SetUint(MM_MODEM_PROPERTY_UNLOCKREQUIRED,
                           MM_MODEM_LOCK_NONE);
  vector<std::tuple<string, uint32_t>> ports = {
      std::make_tuple(kLinkName, MM_MODEM_PORT_TYPE_NET) };
  modem_properties.Set(MM_MODEM_PROPERTY_PORTS, brillo::Any(ports));
  properties[MM_DBUS_INTERFACE_MODEM] = modem_properties;

  KeyValueStore modem3gpp_properties;
  modem3gpp_properties.SetUint(
      MM_MODEM_MODEM3GPP_PROPERTY_REGISTRATIONSTATE,
      MM_MODEM_3GPP_REGISTRATION_STATE_HOME);
  properties[MM_DBUS_INTERFACE_MODEM_MODEM3GPP] = modem3gpp_properties;

  EXPECT_CALL(control_interface_, CreateDBusPropertiesProxy(kPath, kService))
      .WillOnce(ReturnAndReleasePointee(&proxy_));
  modem_->CreateDeviceMM1(properties);
  EXPECT_TRUE(modem_->device().get());
  EXPECT_TRUE(modem_->device()->capability_->IsRegistered());
}

}  // namespace shill
