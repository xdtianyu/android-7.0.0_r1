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

#include "shill/cellular/modem_manager.h"

#include <base/stl_util.h>
#include <ModemManager/ModemManager.h>

#include "shill/cellular/mock_dbus_objectmanager_proxy.h"
#include "shill/cellular/mock_modem.h"
#include "shill/cellular/mock_modem_info.h"
#include "shill/cellular/mock_modem_manager_proxy.h"
#include "shill/manager.h"
#include "shill/mock_control.h"
#include "shill/mock_manager.h"
#include "shill/test_event_dispatcher.h"
#include "shill/testing.h"

using std::string;
using std::shared_ptr;
using std::vector;
using testing::_;
using testing::Invoke;
using testing::Pointee;
using testing::Return;
using testing::SaveArg;
using testing::StrEq;
using testing::Test;

namespace shill {

class ModemManagerTest : public Test {
 public:
  ModemManagerTest()
      : manager_(&control_, &dispatcher_, nullptr),
        modem_info_(&control_, &dispatcher_, nullptr, &manager_) {}

  virtual void SetUp() {
    modem_.reset(
        new StrictModem(kService, kModemPath, &modem_info_, &control_));
  }

 protected:
  static const char kService[];
  static const char kPath[];
  static const char kModemPath[];

  shared_ptr<StrictModem> modem_;

  EventDispatcherForTest dispatcher_;
  MockControl control_;
  MockManager manager_;
  MockModemInfo modem_info_;
};

const char ModemManagerTest::kService[] = "org.chromium.ModemManager";
const char ModemManagerTest::kPath[] = "/org/chromium/ModemManager";
const char ModemManagerTest::kModemPath[] = "/org/blah/Modem/blah/0";

class ModemManagerForTest : public ModemManager {
 public:
  ModemManagerForTest(ControlInterface* control_interface,
                      const string& service,
                      const string& path,
                      ModemInfo* modem_info)
    : ModemManager(control_interface, service, path, modem_info) {}

  MOCK_METHOD0(Start, void());
  MOCK_METHOD0(Stop, void());
};

class ModemManagerCoreTest : public ModemManagerTest {
 public:
  ModemManagerCoreTest()
      : ModemManagerTest(),
        modem_manager_(&control_, kService, kPath, &modem_info_) {}

 protected:
  ModemManagerForTest modem_manager_;
};

TEST_F(ModemManagerCoreTest, ConnectDisconnect) {
  EXPECT_FALSE(modem_manager_.service_connected_);
  modem_manager_.Connect();
  EXPECT_TRUE(modem_manager_.service_connected_);

  modem_manager_.RecordAddedModem(modem_);
  EXPECT_EQ(1, modem_manager_.modems_.size());

  modem_manager_.ModemManager::Disconnect();
  EXPECT_EQ(0, modem_manager_.modems_.size());
  EXPECT_FALSE(modem_manager_.service_connected_);
}

TEST_F(ModemManagerCoreTest, AddRemoveModem) {
  modem_manager_.Connect();
  EXPECT_FALSE(modem_manager_.ModemExists(kModemPath));

  // Remove non-existent modem path.
  modem_manager_.RemoveModem(kModemPath);
  EXPECT_FALSE(modem_manager_.ModemExists(kModemPath));

  modem_manager_.RecordAddedModem(modem_);
  EXPECT_TRUE(modem_manager_.ModemExists(kModemPath));

  // Add an already added modem.
  modem_manager_.RecordAddedModem(modem_);
  EXPECT_TRUE(modem_manager_.ModemExists(kModemPath));

  modem_manager_.RemoveModem(kModemPath);
  EXPECT_FALSE(modem_manager_.ModemExists(kModemPath));

  // Remove an already removed modem path.
  modem_manager_.RemoveModem(kModemPath);
  EXPECT_FALSE(modem_manager_.ModemExists(kModemPath));
}

class ModemManagerClassicMockInit : public ModemManagerClassic {
 public:
  ModemManagerClassicMockInit(ControlInterface* control_interface,
                              const string& service,
                              const string& path,
                              ModemInfo* modem_info_) :
      ModemManagerClassic(control_interface, service, path, modem_info_) {}

  MOCK_METHOD1(InitModemClassic, void(shared_ptr<ModemClassic>));
};

class ModemManagerClassicTest : public ModemManagerTest {
 public:
  ModemManagerClassicTest()
      : ModemManagerTest(),
        modem_manager_(&control_, kService, kPath, &modem_info_),
        proxy_(new MockModemManagerProxy()) {}

 protected:
  ModemManagerClassicMockInit modem_manager_;
  MockModemManagerProxy* proxy_;
};

TEST_F(ModemManagerClassicTest, StartStop) {
  EXPECT_EQ(nullptr, modem_manager_.proxy_.get());

  EXPECT_CALL(control_, CreateModemManagerProxy(_, kPath, kService, _, _))
      .WillOnce(Return(proxy_));
  modem_manager_.Start();
  EXPECT_NE(nullptr, modem_manager_.proxy_.get());

  modem_manager_.Stop();
  EXPECT_EQ(nullptr, modem_manager_.proxy_.get());
}

TEST_F(ModemManagerClassicTest, Connect) {
  // Setup proxy.
  modem_manager_.proxy_.reset(proxy_);

  EXPECT_CALL(*proxy_, EnumerateDevices())
      .WillOnce(Return(vector<string>(1, kModemPath)));
  EXPECT_CALL(modem_manager_,
              InitModemClassic(
                  Pointee(Field(&Modem::path_, StrEq(kModemPath)))));
  modem_manager_.Connect();
  EXPECT_EQ(1, modem_manager_.modems_.size());
  ASSERT_TRUE(ContainsKey(modem_manager_.modems_, kModemPath));
}

class ModemManager1MockInit : public ModemManager1 {
 public:
  ModemManager1MockInit(ControlInterface* control_interface,
                        const string& service,
                        const string& path,
                        ModemInfo* modem_info_) :
      ModemManager1(control_interface, service, path, modem_info_) {}
  MOCK_METHOD2(InitModem1, void(shared_ptr<Modem1>,
                                const InterfaceToProperties&));
};


class ModemManager1Test : public ModemManagerTest {
 public:
  ModemManager1Test()
      : ModemManagerTest(),
        modem_manager_(&control_, kService, kPath, &modem_info_),
        proxy_(new MockDBusObjectManagerProxy()) {}

 protected:
  virtual void SetUp() {
    proxy_->IgnoreSetCallbacks();
  }

  void Connect(const ObjectsWithProperties& expected_objects) {
    ManagedObjectsCallback get_managed_objects_callback;
    EXPECT_CALL(*proxy_, GetManagedObjects(_, _, _))
        .WillOnce(SaveArg<1>(&get_managed_objects_callback));
    modem_manager_.Connect();
    get_managed_objects_callback.Run(expected_objects, Error());
  }

  static ObjectsWithProperties GetModemWithProperties() {
    KeyValueStore o_fd_mm1_modem;

    InterfaceToProperties properties;
    properties[MM_DBUS_INTERFACE_MODEM] = o_fd_mm1_modem;

    ObjectsWithProperties objects_with_properties;
    objects_with_properties[kModemPath] = properties;

    return objects_with_properties;
  }

  ModemManager1MockInit modem_manager_;
  MockDBusObjectManagerProxy* proxy_;
  MockControl control_;
};

TEST_F(ModemManager1Test, StartStop) {
  EXPECT_EQ(nullptr, modem_manager_.proxy_.get());

  EXPECT_CALL(control_, CreateDBusObjectManagerProxy(kPath, kService, _, _))
      .WillOnce(Return(proxy_));
  EXPECT_CALL(*proxy_, set_interfaces_added_callback(_));
  EXPECT_CALL(*proxy_, set_interfaces_removed_callback(_));
  modem_manager_.Start();
  EXPECT_NE(nullptr, modem_manager_.proxy_.get());

  modem_manager_.Stop();
  EXPECT_EQ(nullptr, modem_manager_.proxy_.get());
}

TEST_F(ModemManager1Test, Connect) {
  // Setup proxy.
  modem_manager_.proxy_.reset(proxy_);

  Connect(GetModemWithProperties());
  EXPECT_EQ(1, modem_manager_.modems_.size());
  EXPECT_TRUE(ContainsKey(modem_manager_.modems_, kModemPath));
}

TEST_F(ModemManager1Test, AddRemoveInterfaces) {
  // Setup proxy.
  modem_manager_.proxy_.reset(proxy_);

  // Have nothing come back from GetManagedObjects
  Connect(ObjectsWithProperties());
  EXPECT_EQ(0, modem_manager_.modems_.size());

  // Add an object that doesn't have a modem interface.  Nothing should be added
  EXPECT_CALL(modem_manager_, InitModem1(_, _)).Times(0);
  modem_manager_.OnInterfacesAddedSignal(kModemPath,
                                         InterfaceToProperties());
  EXPECT_EQ(0, modem_manager_.modems_.size());

  // Actually add a modem
  EXPECT_CALL(modem_manager_, InitModem1(_, _)).Times(1);
  modem_manager_.OnInterfacesAddedSignal(kModemPath,
                                         GetModemWithProperties()[kModemPath]);
  EXPECT_EQ(1, modem_manager_.modems_.size());

  // Remove an irrelevant interface
  vector<string> not_including_modem_interface;
  not_including_modem_interface.push_back("not.a.modem.interface");
  modem_manager_.OnInterfacesRemovedSignal(kModemPath,
                                           not_including_modem_interface);
  EXPECT_EQ(1, modem_manager_.modems_.size());

  // Remove the modem
  vector<string> with_modem_interface;
  with_modem_interface.push_back(MM_DBUS_INTERFACE_MODEM);
  modem_manager_.OnInterfacesRemovedSignal(kModemPath, with_modem_interface);
  EXPECT_EQ(0, modem_manager_.modems_.size());
}

}  // namespace shill
