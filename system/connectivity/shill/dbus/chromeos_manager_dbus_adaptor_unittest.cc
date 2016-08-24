//
// Copyright (C) 2016 The Android Open Source Project
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

#include "shill/dbus/chromeos_manager_dbus_adaptor.h"

#include <memory>

#include <brillo/errors/error.h>
#include <dbus/bus.h>
#include <dbus/message.h>
#include <dbus/mock_bus.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "shill/dbus/mock_dbus_service_watcher.h"
#include "shill/dbus/mock_dbus_service_watcher_factory.h"
#include "shill/error.h"
#include "shill/mock_control.h"
#include "shill/mock_event_dispatcher.h"
#include "shill/mock_manager.h"
#include "shill/mock_metrics.h"

using dbus::MockBus;
using dbus::Response;
using std::string;
using testing::_;
using testing::DoAll;
using testing::Invoke;
using testing::Return;
using testing::SetArgPointee;
using testing::Test;
using testing::WithArg;

namespace shill {

class ChromeosManagerDBusAdaptorTest : public Test {
 public:
  ChromeosManagerDBusAdaptorTest()
      : adaptor_bus_(new MockBus(dbus::Bus::Options())),
        proxy_bus_(new MockBus(dbus::Bus::Options())),
        metrics_(&dispatcher_),
        manager_(&control_interface_, &dispatcher_, &metrics_),
        manager_adaptor_(adaptor_bus_, proxy_bus_, &manager_) {}

  virtual ~ChromeosManagerDBusAdaptorTest() {}

  virtual void SetUp() {
    manager_adaptor_.dbus_service_watcher_factory_ =
        &dbus_service_watcher_factory_;
  }

  virtual void TearDown() {}

 protected:
  scoped_refptr<MockBus> adaptor_bus_;
  scoped_refptr<MockBus> proxy_bus_;
  MockControl control_interface_;
  MockEventDispatcher dispatcher_;
  MockMetrics metrics_;
  MockManager manager_;
  MockDBusServiceWatcherFactory dbus_service_watcher_factory_;
  ChromeosManagerDBusAdaptor manager_adaptor_;
};

void SetErrorTypeSuccess(Error* error) { error->Populate(Error::kSuccess); }

void SetErrorTypeFailure(Error* error) {
  error->Populate(Error::kOperationFailed);
}

TEST_F(ChromeosManagerDBusAdaptorTest, ClaimInterface) {
  brillo::ErrorPtr error;
  string kDefaultClaimerName = "";
  string kNonDefaultClaimerName = "test_claimer";
  string kInterfaceName = "test_interface";
  scoped_ptr<Response> message(Response::CreateEmpty());

  // Watcher for device claimer is not created when we fail to claim the device.
  EXPECT_EQ(nullptr, manager_adaptor_.watcher_for_device_claimer_.get());
  EXPECT_CALL(manager_, ClaimDevice(_, kInterfaceName, _))
      .WillOnce(WithArg<2>(Invoke(SetErrorTypeFailure)));
  EXPECT_CALL(dbus_service_watcher_factory_, CreateDBusServiceWatcher(_, _, _))
      .Times(0);
  manager_adaptor_.ClaimInterface(&error, message.get(), kNonDefaultClaimerName,
                                  kInterfaceName);
  EXPECT_EQ(nullptr, manager_adaptor_.watcher_for_device_claimer_.get());

  // Watcher for device claimer is not created when we succeed in claiming the
  // device from the default claimer.
  EXPECT_EQ(nullptr, manager_adaptor_.watcher_for_device_claimer_.get());
  EXPECT_CALL(manager_, ClaimDevice(_, kInterfaceName, _))
      .WillOnce(WithArg<2>(Invoke(SetErrorTypeSuccess)));
  EXPECT_CALL(dbus_service_watcher_factory_, CreateDBusServiceWatcher(_, _, _))
      .Times(0);
  manager_adaptor_.ClaimInterface(&error, message.get(), kDefaultClaimerName,
                                  kInterfaceName);
  EXPECT_EQ(nullptr, manager_adaptor_.watcher_for_device_claimer_.get());

  // Watcher for device claimer is created when we succeed in claiming the
  // device from a non-default claimer.
  EXPECT_EQ(nullptr, manager_adaptor_.watcher_for_device_claimer_.get());
  EXPECT_CALL(manager_, ClaimDevice(_, kInterfaceName, _))
      .WillOnce(WithArg<2>(Invoke(SetErrorTypeSuccess)));
  EXPECT_CALL(dbus_service_watcher_factory_, CreateDBusServiceWatcher(_, _, _))
      .WillOnce(Return(new MockDBusServiceWatcher()));
  manager_adaptor_.ClaimInterface(&error, message.get(), kNonDefaultClaimerName,
                                  kInterfaceName);
  EXPECT_NE(nullptr, manager_adaptor_.watcher_for_device_claimer_.get());
}

TEST_F(ChromeosManagerDBusAdaptorTest, ReleaseInterface) {
  brillo::ErrorPtr error;
  string kClaimerName = "test_claimer";
  string kInterfaceName = "test_interface";
  scoped_ptr<Response> message(Response::CreateEmpty());

  // Setup watcher for device claimer.
  manager_adaptor_.watcher_for_device_claimer_.reset(
      new MockDBusServiceWatcher());

  // If the device claimer is not removed, do not reset the watcher for device
  // claimer.
  EXPECT_CALL(manager_, ReleaseDevice(_, kInterfaceName, _, _))
      .WillOnce(SetArgPointee<2>(false));
  manager_adaptor_.ReleaseInterface(&error, message.get(), kClaimerName,
                                  kInterfaceName);
  EXPECT_NE(nullptr, manager_adaptor_.watcher_for_device_claimer_.get());

  // If the device claimer is removed, reset the watcher for device claimer.
  EXPECT_CALL(manager_, ReleaseDevice(_, kInterfaceName, _, _))
      .WillOnce(SetArgPointee<2>(true));
  manager_adaptor_.ReleaseInterface(&error, message.get(), kClaimerName,
                                    kInterfaceName);
  EXPECT_EQ(nullptr, manager_adaptor_.watcher_for_device_claimer_.get());
}

TEST_F(ChromeosManagerDBusAdaptorTest, SetupApModeInterface) {
  brillo::ErrorPtr error;
  string out_interface_name;
  scoped_ptr<Response> message(Response::CreateEmpty());

#if !defined(DISABLE_WIFI) && defined(__BRILLO__)
  // Watcher for AP mode setter is not created when we fail to setup AP mode
  // interface.
  EXPECT_EQ(nullptr, manager_adaptor_.watcher_for_ap_mode_setter_.get());
  EXPECT_CALL(manager_, SetupApModeInterface(&out_interface_name, _))
      .WillOnce(DoAll(WithArg<1>(Invoke(SetErrorTypeFailure)), Return(false)));
  EXPECT_CALL(dbus_service_watcher_factory_, CreateDBusServiceWatcher(_, _, _))
      .Times(0);
  EXPECT_FALSE(manager_adaptor_.SetupApModeInterface(&error, message.get(),
                                                     &out_interface_name));
  EXPECT_EQ(nullptr, manager_adaptor_.watcher_for_ap_mode_setter_.get());

  // Watcher for AP mode setter is created when we succeed in AP mode
  // interface setup.
  EXPECT_EQ(nullptr, manager_adaptor_.watcher_for_ap_mode_setter_.get());
  EXPECT_CALL(manager_, SetupApModeInterface(&out_interface_name, _))
      .WillOnce(DoAll(WithArg<1>(Invoke(SetErrorTypeSuccess)), Return(true)));
  EXPECT_CALL(dbus_service_watcher_factory_, CreateDBusServiceWatcher(_, _, _))
      .WillOnce(Return(new MockDBusServiceWatcher()));
  EXPECT_TRUE(manager_adaptor_.SetupApModeInterface(&error, message.get(),
                                                    &out_interface_name));
  EXPECT_NE(nullptr, manager_adaptor_.watcher_for_ap_mode_setter_.get());
#else
  EXPECT_CALL(dbus_service_watcher_factory_, CreateDBusServiceWatcher(_, _, _))
      .Times(0);
  EXPECT_FALSE(manager_adaptor_.SetupApModeInterface(&error, message.get(),
                                                     &out_interface_name));
#endif  // !DISABLE_WIFI && __BRILLO__
}

TEST_F(ChromeosManagerDBusAdaptorTest, SetupStationModeInterface) {
  brillo::ErrorPtr error;
  string out_interface_name;

#if !defined(DISABLE_WIFI) && defined(__BRILLO__)
  // Setup watcher for AP mode setter.
  manager_adaptor_.watcher_for_ap_mode_setter_.reset(
      new MockDBusServiceWatcher());

  // Reset watcher for AP mode setter after setting up station mode interface.
  EXPECT_CALL(manager_, SetupStationModeInterface(&out_interface_name, _))
      .WillOnce(Return(true));
  EXPECT_TRUE(
      manager_adaptor_.SetupStationModeInterface(&error, &out_interface_name));
  EXPECT_EQ(nullptr, manager_adaptor_.watcher_for_ap_mode_setter_.get());
#else
  EXPECT_FALSE(
      manager_adaptor_.SetupStationModeInterface(&error, &out_interface_name));
#endif  // !DISABLE_WIFI && __BRILLO__
}

TEST_F(ChromeosManagerDBusAdaptorTest, OnApModeSetterVanished) {
  // Setup watcher for AP mode setter.
  manager_adaptor_.watcher_for_ap_mode_setter_.reset(
      new MockDBusServiceWatcher());

  // Reset watcher for AP mode setter after AP mode setter vanishes.
#if !defined(DISABLE_WIFI) && defined(__BRILLO__)
  EXPECT_CALL(manager_, OnApModeSetterVanished());
#endif  // !DISABLE_WIFI && __BRILLO__
  manager_adaptor_.OnApModeSetterVanished();
  EXPECT_EQ(nullptr, manager_adaptor_.watcher_for_ap_mode_setter_.get());
}

TEST_F(ChromeosManagerDBusAdaptorTest, OnDeviceClaimerVanished) {
  // Setup watcher for device claimer.
  manager_adaptor_.watcher_for_device_claimer_.reset(
      new MockDBusServiceWatcher());

  // Reset watcher for device claimer after the device claimer vanishes.
  EXPECT_CALL(manager_, OnDeviceClaimerVanished());
  manager_adaptor_.OnDeviceClaimerVanished();
  EXPECT_EQ(nullptr, manager_adaptor_.watcher_for_device_claimer_.get());
}

}  // namespace shill
