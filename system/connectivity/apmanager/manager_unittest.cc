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

#include "apmanager/manager.h"

#include <gtest/gtest.h>

#include "apmanager/fake_device_adaptor.h"
#include "apmanager/mock_control.h"
#include "apmanager/mock_device.h"

using ::testing::_;
using ::testing::Return;
using ::testing::ReturnNew;

namespace apmanager {

class ManagerTest : public testing::Test {
 public:
  ManagerTest() : manager_(&control_interface_) {
    ON_CALL(control_interface_, CreateDeviceAdaptorRaw())
        .WillByDefault(ReturnNew<FakeDeviceAdaptor>());
  }

  void RegisterDevice(scoped_refptr<Device> device) {
    manager_.devices_.push_back(device);
  }

 protected:
  MockControl control_interface_;
  Manager manager_;
};

TEST_F(ManagerTest, GetAvailableDevice) {
  // Register a device without AP support (no preferred AP interface).
  scoped_refptr<MockDevice> device0 = new MockDevice(&manager_);
  RegisterDevice(device0);

  // No available device for AP operation.
  EXPECT_EQ(nullptr, manager_.GetAvailableDevice());

  // Add AP support to the device.
  const char kTestInterface0[] = "test-interface0";
  device0->SetPreferredApInterface(kTestInterface0);
  EXPECT_EQ(device0, manager_.GetAvailableDevice());

  // Register another device with AP support.
  const char kTestInterface1[] = "test-interface1";
  scoped_refptr<MockDevice> device1 = new MockDevice(&manager_);
  device1->SetPreferredApInterface(kTestInterface1);
  RegisterDevice(device1);

  // Both devices are idle by default, should return the first added device.
  EXPECT_EQ(device0, manager_.GetAvailableDevice());

  // Set first one to be in used, should return the non-used device.
  device0->SetInUse(true);
  EXPECT_EQ(device1, manager_.GetAvailableDevice());

  // Both devices are in used, should return a nullptr.
  device1->SetInUse(true);
  EXPECT_EQ(nullptr, manager_.GetAvailableDevice());
}

TEST_F(ManagerTest, GetDeviceFromInterfaceName) {
  // Register two devices
  scoped_refptr<MockDevice> device0 = new MockDevice(&manager_);
  scoped_refptr<MockDevice> device1 = new MockDevice(&manager_);
  RegisterDevice(device0);
  RegisterDevice(device1);

  const char kTestInterface0[] = "test-interface0";
  const char kTestInterface1[] = "test-interface1";

  // interface0 belongs to device0.
  EXPECT_CALL(*device0.get(), InterfaceExists(kTestInterface0))
      .WillOnce(Return(true));
  EXPECT_EQ(device0, manager_.GetDeviceFromInterfaceName(kTestInterface0));

  // interface1 belongs to device1.
  EXPECT_CALL(*device0.get(), InterfaceExists(_))
      .WillRepeatedly(Return(false));
  EXPECT_CALL(*device1.get(), InterfaceExists(kTestInterface1))
      .WillOnce(Return(true));
  EXPECT_EQ(device1, manager_.GetDeviceFromInterfaceName(kTestInterface1));

  // "random" interface is not found.
  EXPECT_CALL(*device1.get(), InterfaceExists(_))
      .WillRepeatedly(Return(false));
  EXPECT_EQ(nullptr, manager_.GetDeviceFromInterfaceName("random"));
}

}  // namespace apmanager
