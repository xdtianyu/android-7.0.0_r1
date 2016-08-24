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

#include "shill/manager.h"

#include <map>
#include <memory>
#include <set>

#include <base/files/file_util.h>
#include <base/files/scoped_temp_dir.h>
#include <base/stl_util.h>
#include <base/strings/stringprintf.h>
#if defined(__ANDROID__)
#include <dbus/service_constants.h>
#else
#include <chromeos/dbus/service_constants.h>
#endif  // __ANDROID__
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "shill/adaptor_interfaces.h"
#include "shill/ephemeral_profile.h"
#include "shill/error.h"
#include "shill/fake_store.h"
#include "shill/geolocation_info.h"
#include "shill/key_value_store.h"
#include "shill/link_monitor.h"
#include "shill/logging.h"
#include "shill/mock_adaptors.h"
#include "shill/mock_connection.h"
#include "shill/mock_control.h"
#include "shill/mock_crypto_util_proxy.h"
#include "shill/mock_device.h"
#include "shill/mock_device_claimer.h"
#include "shill/mock_device_info.h"
#include "shill/mock_log.h"
#include "shill/mock_metrics.h"
#include "shill/mock_power_manager.h"
#include "shill/mock_profile.h"
#include "shill/mock_resolver.h"
#include "shill/mock_service.h"
#include "shill/mock_store.h"
#include "shill/portal_detector.h"
#include "shill/property_store_unittest.h"
#include "shill/resolver.h"
#include "shill/service_under_test.h"
#include "shill/store_factory.h"
#include "shill/testing.h"
#include "shill/upstart/mock_upstart.h"
#include "shill/wimax/wimax_service.h"

#if !defined(DISABLE_WIFI)
#include "shill/wifi/mock_wifi_provider.h"
#include "shill/wifi/mock_wifi_service.h"
#include "shill/wifi/wifi_service.h"
#if defined(__BRILLO__)
#include "shill/wifi/mock_wifi_driver_hal.h"
#endif  // __BRILLO__
#endif  // DISABLE_WIFI

#if !defined(DISABLE_WIRED_8021X)
#include "shill/ethernet/mock_ethernet_eap_provider.h"
#endif  // DISABLE_WIRED_8021X

using base::Bind;
using base::FilePath;
using base::ScopedTempDir;
using base::Unretained;
using std::map;
using std::set;
using std::string;
using std::vector;

namespace shill {
using ::testing::_;
using ::testing::AnyNumber;
using ::testing::AtLeast;
using ::testing::ContainerEq;
using ::testing::DoAll;
using ::testing::ElementsAre;
using ::testing::HasSubstr;
using ::testing::InSequence;
using ::testing::Invoke;
using ::testing::Mock;
using ::testing::Ne;
using ::testing::NiceMock;
using ::testing::Ref;
using ::testing::Return;
using ::testing::ReturnNull;
using ::testing::ReturnRef;
using ::testing::SaveArg;
using ::testing::SetArgumentPointee;
using ::testing::StrEq;
using ::testing::StrictMock;
using ::testing::Test;
using ::testing::WithArg;

class ManagerTest : public PropertyStoreTest {
 public:
  ManagerTest()
      : power_manager_(new MockPowerManager(nullptr, control_interface())),
        device_info_(new NiceMock<MockDeviceInfo>(control_interface(),
                                                  nullptr,
                                                  nullptr,
                                                  nullptr)),
        manager_adaptor_(new NiceMock<ManagerMockAdaptor>()),
#if !defined(DISABLE_WIRED_8021X)
        ethernet_eap_provider_(new NiceMock<MockEthernetEapProvider>()),
#endif  // DISABLE_WIRED_8021X
#if !defined(DISABLE_WIFI)
        wifi_provider_(new NiceMock<MockWiFiProvider>()),
#endif  // DISABLE_WIFI
        crypto_util_proxy_(
            new NiceMock<MockCryptoUtilProxy>(dispatcher())),
        upstart_(new NiceMock<MockUpstart>(control_interface())) {
    ON_CALL(*control_interface(), CreatePowerManagerProxy(_, _, _))
        .WillByDefault(ReturnNull());

    mock_devices_.push_back(new NiceMock<MockDevice>(control_interface(),
                                                     dispatcher(),
                                                     metrics(),
                                                     manager(),
                                                     "null0",
                                                     "addr0",
                                                     0));
    mock_devices_.push_back(new NiceMock<MockDevice>(control_interface(),
                                                     dispatcher(),
                                                     metrics(),
                                                     manager(),
                                                     "null1",
                                                     "addr1",
                                                     1));
    mock_devices_.push_back(new NiceMock<MockDevice>(control_interface(),
                                                     dispatcher(),
                                                     metrics(),
                                                     manager(),
                                                     "null2",
                                                     "addr2",
                                                     2));
    mock_devices_.push_back(new NiceMock<MockDevice>(control_interface(),
                                                     dispatcher(),
                                                     metrics(),
                                                     manager(),
                                                     "null3",
                                                     "addr3",
                                                     3));
    manager()->connect_profiles_to_rpc_ = false;
    SetRunning(true);

    // Replace the manager's adaptor with a quieter one, and one
    // we can do EXPECT*() against.  Passes ownership.
    manager()->adaptor_.reset(manager_adaptor_);

#if !defined(DISABLE_WIRED_8021X)
    // Replace the manager's Ethernet EAP provider with our mock.
    // Passes ownership.
    manager()->ethernet_eap_provider_.reset(ethernet_eap_provider_);
#endif  // DISABLE_WIRED_8021X

#if !defined(DISABLE_WIFI)
    // Replace the manager's WiFi provider with our mock.  Passes
    // ownership.
    manager()->wifi_provider_.reset(wifi_provider_);
#if defined(__BRILLO__)
    manager()->wifi_driver_hal_ = &wifi_driver_hal_;
#endif  // __BRILLO__
#endif  // DISABLE_WIFI

    // Update the manager's map from technology to provider.
    manager()->UpdateProviderMapping();

    // Replace the manager's crypto util proxy with our mock.  Passes
    // ownership.
    manager()->crypto_util_proxy_.reset(crypto_util_proxy_);

    // Replace the manager's upstart instance with our mock.  Passes
    // ownership.
    manager()->upstart_.reset(upstart_);
  }
  virtual ~ManagerTest() {}

  void SetMetrics(Metrics* metrics) {
    manager()->set_metrics(metrics);
  }

  bool IsDeviceRegistered(const DeviceRefPtr& device,
                          Technology::Identifier tech) {
    auto devices = manager()->FilterByTechnology(tech);
    return (devices.size() == 1 && devices[0].get() == device.get());
  }
  bool ServiceOrderIs(ServiceRefPtr svc1, ServiceRefPtr svc2);

  void AdoptProfile(Manager* manager, ProfileRefPtr profile) {
    manager->profiles_.push_back(profile);
  }

  void SetRunning(bool running) {
    manager()->running_ = running;
  }

  ProfileRefPtr GetEphemeralProfile(Manager* manager) {
    return manager->ephemeral_profile_;
  }

  vector<ProfileRefPtr>& GetProfiles(Manager* manager) {
    return manager->profiles_;
  }

  Profile* CreateProfileForManager(Manager* manager) {
    Profile::Identifier id("rather", "irrelevant");
    std::unique_ptr<FakeStore> storage(new FakeStore());
    if (!storage->Open())
      return nullptr;
    Profile* profile(new Profile(
        control_interface(), metrics(), manager, id, FilePath(), false));
    profile->set_storage(storage.release());  // Passes ownership of "storage".
    return profile;  // Passes ownership of "profile".
  }

  bool CreateBackingStoreForService(ScopedTempDir* temp_dir,
                                    const string& user_identifier,
                                    const string& profile_identifier,
                                    const string& service_name) {
    std::unique_ptr<StoreInterface> store(
        StoreFactory::GetInstance()->CreateStore(
            Profile::GetFinalStoragePath(
                temp_dir->path(),
                Profile::Identifier(user_identifier,
                                    profile_identifier))));
    return store->Open() &&
        store->SetString(service_name, "rather", "irrelevant") &&
        store->Close();
  }

  Error::Type TestCreateProfile(Manager* manager, const string& name) {
    Error error;
    string path;
    manager->CreateProfile(name, &path, &error);
    return error.type();
  }

  Error::Type TestPopAnyProfile(Manager* manager) {
    Error error;
    manager->PopAnyProfile(&error);
    return error.type();
  }

  Error::Type TestPopAllUserProfiles(Manager* manager) {
    Error error;
    manager->PopAllUserProfiles(&error);
    return error.type();
  }

  Error::Type TestPopProfile(Manager* manager, const string& name) {
    Error error;
    manager->PopProfile(name, &error);
    return error.type();
  }

  Error::Type TestPushProfile(Manager* manager, const string& name) {
    Error error;
    string path;
    manager->PushProfile(name, &path, &error);
    return error.type();
  }

  Error::Type TestInsertUserProfile(Manager* manager,
                                    const string& name,
                                    const string& user_hash) {
    Error error;
    string path;
    manager->InsertUserProfile(name, user_hash, &path, &error);
    return error.type();
  }

  scoped_refptr<MockProfile> AddNamedMockProfileToManager(
      Manager* manager, const string& name) {
    scoped_refptr<MockProfile> profile(
        new MockProfile(control_interface(), metrics(), manager, ""));
    EXPECT_CALL(*profile, GetRpcIdentifier()).WillRepeatedly(Return(name));
    EXPECT_CALL(*profile, UpdateDevice(_)).WillRepeatedly(Return(false));
    AdoptProfile(manager, profile);
    return profile;
  }

  void AddMockProfileToManager(Manager* manager) {
    AddNamedMockProfileToManager(manager, "/");
  }

  void CompleteServiceSort() {
    EXPECT_TRUE(IsSortServicesTaskPending());
    dispatcher()->DispatchPendingEvents();
    EXPECT_FALSE(IsSortServicesTaskPending());
  }

  bool IsSortServicesTaskPending() {
    return !manager()->sort_services_task_.IsCancelled();
  }

  void RefreshConnectionState() {
    manager()->RefreshConnectionState();
  }

  RpcIdentifier GetDefaultServiceRpcIdentifier() {
    return manager()->GetDefaultServiceRpcIdentifier(nullptr);
  }

  void SetResolver(Resolver* resolver) {
    manager()->resolver_ = resolver;
  }

  bool SetIgnoredDNSSearchPaths(const string& search_paths, Error* error) {
    return manager()->SetIgnoredDNSSearchPaths(search_paths, error);
  }

  bool SetCheckPortalList(const string& check_portal_list, Error* error) {
    return manager()->SetCheckPortalList(check_portal_list, error);
  }

  const string& GetIgnoredDNSSearchPaths() {
    return manager()->props_.ignored_dns_search_paths;
  }

#if !defined(DISABLE_WIFI)
  WiFiServiceRefPtr ReleaseTempMockService() {
    // Take a reference to hold during this function.
    WiFiServiceRefPtr temp_service = temp_mock_service_;
    temp_mock_service_ = nullptr;
    return temp_service;
  }
#endif  // DISABLE_WIFI

  void SetDeviceClaimer(DeviceClaimer* device_claimer) {
    manager()->device_claimer_.reset(device_claimer);
  }

  void VerifyPassiveMode() {
    EXPECT_NE(nullptr, manager()->device_claimer_.get());
    EXPECT_TRUE(manager()->device_claimer_->default_claimer());
  }

 protected:
  typedef scoped_refptr<MockService> MockServiceRefPtr;

  class ServiceWatcher : public base::SupportsWeakPtr<ServiceWatcher> {
   public:
    ServiceWatcher() {}
    virtual ~ServiceWatcher() {}

    MOCK_METHOD1(OnDefaultServiceChanged, void(const ServiceRefPtr& service));

   private:
    DISALLOW_COPY_AND_ASSIGN(ServiceWatcher);
  };

  class TerminationActionTest :
      public base::SupportsWeakPtr<TerminationActionTest> {
   public:
    static const char kActionName[];

    TerminationActionTest() : manager_(nullptr) {}
    virtual ~TerminationActionTest() {}

    MOCK_METHOD1(Done, void(const Error& error));

    void Action() {
      manager_->TerminationActionComplete("action");
    }

    void set_manager(Manager* manager) { manager_ = manager; }

   private:
    Manager* manager_;
    DISALLOW_COPY_AND_ASSIGN(TerminationActionTest);
  };

  class DestinationVerificationTest :
      public base::SupportsWeakPtr<DestinationVerificationTest> {
   public:
    DestinationVerificationTest() {}
    virtual ~DestinationVerificationTest() {}

    MOCK_METHOD2(ResultBoolCallbackStub, void(const Error& result, bool flag));
    MOCK_METHOD2(ResultStringCallbackStub, void(const Error& result,
                                                const string& value));
   private:
    DISALLOW_COPY_AND_ASSIGN(DestinationVerificationTest);
  };

  class DisableTechnologyReplyHandler :
      public base::SupportsWeakPtr<DisableTechnologyReplyHandler> {
   public:
    DisableTechnologyReplyHandler() {}
    virtual ~DisableTechnologyReplyHandler() {}

    MOCK_METHOD1(ReportResult, void(const Error&));

   private:
    DISALLOW_COPY_AND_ASSIGN(DisableTechnologyReplyHandler);
  };

  class ResultCallbackObserver {
   public:
    ResultCallbackObserver()
        : result_callback_(
              Bind(&ResultCallbackObserver::OnResultCallback,
                   Unretained(this))) {}
    virtual ~ResultCallbackObserver() {}

    MOCK_METHOD1(OnResultCallback, void(const Error& error));

    const ResultCallback& result_callback() const {
      return result_callback_;
    }

   private:
    ResultCallback result_callback_;

    DISALLOW_COPY_AND_ASSIGN(ResultCallbackObserver);
  };

  void SetSuspending(bool suspending) {
    power_manager_->suspending_ = suspending;
  }

  void SetPowerManager() {
    manager()->set_power_manager(power_manager_.release());
  }

  HookTable* GetTerminationActions() {
    return &manager()->termination_actions_;
  }

  void OnSuspendImminent() {
    manager()->OnSuspendImminent();
  }

  void OnDarkSuspendImminent() {
    manager()->OnDarkSuspendImminent();
  }

  void OnSuspendDone() {
    manager()->OnSuspendDone();
  }

  void OnSuspendActionsComplete(const Error& error) {
    manager()->OnSuspendActionsComplete(error);
  }

  vector<string> EnumerateAvailableServices() {
    return manager()->EnumerateAvailableServices(nullptr);
  }

  vector<string> EnumerateWatchedServices() {
    return manager()->EnumerateWatchedServices(nullptr);
  }

  MockServiceRefPtr MakeAutoConnectableService() {
    MockServiceRefPtr service = new NiceMock<MockService>(control_interface(),
                                                          dispatcher(),
                                                          metrics(),
                                                          manager());
    service->SetAutoConnect(true);
    service->SetConnectable(true);
    return service;
  }

#if !defined(DISABLE_WIRED_8021X)
  void SetEapProviderService(const ServiceRefPtr& service) {
    ethernet_eap_provider_->set_service(service);
  }
#endif  // DISABLE_WIRED_8021X

  const std::vector<Technology::Identifier>& GetTechnologyOrder() {
    return manager()->technology_order_;
  }

  std::unique_ptr<MockPowerManager> power_manager_;
  vector<scoped_refptr<MockDevice>> mock_devices_;
  std::unique_ptr<MockDeviceInfo> device_info_;

#if !defined(DISABLE_WIFI)
  // This service is held for the manager, and given ownership in a mock
  // function.  This ensures that when the Manager takes ownership, there
  // is only one reference left.
  scoped_refptr<MockWiFiService> temp_mock_service_;
#endif  // DISABLE_WIFI

  // These pointers are owned by the manager, and only tracked here for
  // EXPECT*()
  ManagerMockAdaptor* manager_adaptor_;
#if !defined(DISABLE_WIRED_8021X)
  MockEthernetEapProvider* ethernet_eap_provider_;
#endif  // DISABLE_WIRED_8021X
#if !defined(DISABLE_WIFI)
  MockWiFiProvider* wifi_provider_;
#if defined(__BRILLO__)
  MockWiFiDriverHal wifi_driver_hal_;
#endif  // __BRILLO__
#endif  // DISABLE_WIFI
  MockCryptoUtilProxy* crypto_util_proxy_;
  MockUpstart* upstart_;
};

const char ManagerTest::TerminationActionTest::kActionName[] = "action";

bool ManagerTest::ServiceOrderIs(ServiceRefPtr svc0, ServiceRefPtr svc1) {
  if (!manager()->sort_services_task_.IsCancelled()) {
    manager()->SortServicesTask();
  }
  return (svc0.get() == manager()->services_[0].get() &&
          svc1.get() == manager()->services_[1].get());
}

void SetErrorPermissionDenied(Error* error) {
  error->Populate(Error::kPermissionDenied);
}

void SetErrorSuccess(Error* error) {
  error->Reset();
}

MATCHER_P(IsError, error, "") {
  return arg.type() == error->type() &&
         arg.message() == error->message();
}

TEST_F(ManagerTest, Contains) {
  EXPECT_TRUE(manager()->store().Contains(kStateProperty));
  EXPECT_FALSE(manager()->store().Contains(""));
}

TEST_F(ManagerTest, PassiveModeDeviceRegistration) {
  manager()->SetPassiveMode();
  VerifyPassiveMode();

  // Setup mock device claimer.
  MockDeviceClaimer* device_claimer = new MockDeviceClaimer("");
  SetDeviceClaimer(device_claimer);
  EXPECT_CALL(*device_claimer, default_claimer()).WillRepeatedly(Return(true));

  ON_CALL(*mock_devices_[0].get(), technology())
      .WillByDefault(Return(Technology::kEthernet));
  ON_CALL(*mock_devices_[1].get(), technology())
      .WillByDefault(Return(Technology::kWifi));

  // Device not released, should not be registered.
  EXPECT_CALL(*device_claimer, IsDeviceReleased(mock_devices_[0]->link_name()))
      .WillOnce(Return(false));
  EXPECT_CALL(*device_claimer, Claim(mock_devices_[0]->link_name(), _))
      .Times(1);
  manager()->RegisterDevice(mock_devices_[0]);
  EXPECT_FALSE(IsDeviceRegistered(mock_devices_[0], Technology::kEthernet));

  // Device is released, should be registered.
  EXPECT_CALL(*device_claimer, IsDeviceReleased(mock_devices_[1]->link_name()))
      .WillOnce(Return(true));
  EXPECT_CALL(*device_claimer, Claim(mock_devices_[1]->link_name(), _))
      .Times(0);
  manager()->RegisterDevice(mock_devices_[1]);
  EXPECT_TRUE(IsDeviceRegistered(mock_devices_[1], Technology::kWifi));
}

TEST_F(ManagerTest, DeviceRegistration) {
  ON_CALL(*mock_devices_[0].get(), technology())
      .WillByDefault(Return(Technology::kEthernet));
  ON_CALL(*mock_devices_[1].get(), technology())
      .WillByDefault(Return(Technology::kWifi));
  ON_CALL(*mock_devices_[2].get(), technology())
      .WillByDefault(Return(Technology::kCellular));

  manager()->RegisterDevice(mock_devices_[0]);
  manager()->RegisterDevice(mock_devices_[1]);
  manager()->RegisterDevice(mock_devices_[2]);

  EXPECT_TRUE(IsDeviceRegistered(mock_devices_[0], Technology::kEthernet));
  EXPECT_TRUE(IsDeviceRegistered(mock_devices_[1], Technology::kWifi));
  EXPECT_TRUE(IsDeviceRegistered(mock_devices_[2], Technology::kCellular));
}

TEST_F(ManagerTest, DeviceRegistrationAndStart) {
  manager()->running_ = true;
  mock_devices_[0]->enabled_persistent_ = true;
  mock_devices_[1]->enabled_persistent_ = false;
  EXPECT_CALL(*mock_devices_[0].get(), SetEnabled(true))
      .Times(1);
  EXPECT_CALL(*mock_devices_[1].get(), SetEnabled(_))
      .Times(0);
  manager()->RegisterDevice(mock_devices_[0]);
  manager()->RegisterDevice(mock_devices_[1]);
}

TEST_F(ManagerTest, DeviceRegistrationWithProfile) {
  MockProfile* profile =
      new MockProfile(control_interface(), metrics(), manager(), "");
  DeviceRefPtr device_ref(mock_devices_[0].get());
  AdoptProfile(manager(), profile);  // Passes ownership.
  EXPECT_CALL(*profile, ConfigureDevice(device_ref));
  EXPECT_CALL(*profile, UpdateDevice(device_ref));
  manager()->RegisterDevice(mock_devices_[0]);
}

TEST_F(ManagerTest, DeviceDeregistration) {
  ON_CALL(*mock_devices_[0].get(), technology())
      .WillByDefault(Return(Technology::kEthernet));
  ON_CALL(*mock_devices_[1].get(), technology())
      .WillByDefault(Return(Technology::kWifi));

  manager()->RegisterDevice(mock_devices_[0]);
  manager()->RegisterDevice(mock_devices_[1]);

  ASSERT_TRUE(IsDeviceRegistered(mock_devices_[0], Technology::kEthernet));
  ASSERT_TRUE(IsDeviceRegistered(mock_devices_[1], Technology::kWifi));

  MockProfile* profile =
      new MockProfile(control_interface(), metrics(), manager(), "");
  AdoptProfile(manager(), profile);  // Passes ownership.

  EXPECT_CALL(*mock_devices_[0].get(), SetEnabled(false));
  EXPECT_CALL(*profile, UpdateDevice(DeviceRefPtr(mock_devices_[0])));
  manager()->DeregisterDevice(mock_devices_[0]);
  EXPECT_FALSE(IsDeviceRegistered(mock_devices_[0], Technology::kEthernet));

  EXPECT_CALL(*mock_devices_[1].get(), SetEnabled(false));
  EXPECT_CALL(*profile, UpdateDevice(DeviceRefPtr(mock_devices_[1])));
  manager()->DeregisterDevice(mock_devices_[1]);
  EXPECT_FALSE(IsDeviceRegistered(mock_devices_[1], Technology::kWifi));
}

TEST_F(ManagerTest, ServiceRegistration) {
  Manager manager(control_interface(),
                  dispatcher(),
                  metrics(),
                  run_path(),
                  storage_path(),
                  string());
  ProfileRefPtr profile(CreateProfileForManager(&manager));
  ASSERT_TRUE(profile.get());
  AdoptProfile(&manager, profile);

  scoped_refptr<MockService> mock_service(
      new NiceMock<MockService>(control_interface(),
                                dispatcher(),
                                metrics(),
                                &manager));
  scoped_refptr<MockService> mock_service2(
      new NiceMock<MockService>(control_interface(),
                                dispatcher(),
                                metrics(),
                                &manager));

  string service1_name(mock_service->unique_name());
  string service2_name(mock_service2->unique_name());

  EXPECT_CALL(*mock_service.get(), GetRpcIdentifier())
      .WillRepeatedly(Return(service1_name));
  EXPECT_CALL(*mock_service2.get(), GetRpcIdentifier())
      .WillRepeatedly(Return(service2_name));
  // TODO(quiche): make this EXPECT_CALL work (crbug.com/203247)
  // EXPECT_CALL(*static_cast<ManagerMockAdaptor*>(manager.adaptor_.get()),
  //             EmitRpcIdentifierArrayChanged(kServicesProperty, _));

  manager.RegisterService(mock_service);
  manager.RegisterService(mock_service2);

  Error error;
  vector<string> rpc_ids = manager.EnumerateAvailableServices(&error);
  set<string> ids(rpc_ids.begin(), rpc_ids.end());
  EXPECT_EQ(2, ids.size());
  EXPECT_TRUE(ContainsKey(ids, mock_service->GetRpcIdentifier()));
  EXPECT_TRUE(ContainsKey(ids, mock_service2->GetRpcIdentifier()));

  EXPECT_NE(nullptr, manager.FindService(service1_name).get());
  EXPECT_NE(nullptr, manager.FindService(service2_name).get());

  manager.set_power_manager(power_manager_.release());
  manager.Stop();
}

TEST_F(ManagerTest, RegisterKnownService) {
  Manager manager(control_interface(),
                  dispatcher(),
                  metrics(),
                  run_path(),
                  storage_path(),
                  string());
  ProfileRefPtr profile(CreateProfileForManager(&manager));
  ASSERT_TRUE(profile.get());
  AdoptProfile(&manager, profile);
  {
    ServiceRefPtr service1(new ServiceUnderTest(control_interface(),
                                                dispatcher(),
                                                metrics(),
                                                &manager));
    ASSERT_TRUE(profile->AdoptService(service1));
    ASSERT_TRUE(profile->ContainsService(service1));
  }  // Force destruction of service1.

  ServiceRefPtr service2(new ServiceUnderTest(control_interface(),
                                              dispatcher(),
                                              metrics(),
                                              &manager));
  manager.RegisterService(service2);
  EXPECT_EQ(service2->profile().get(), profile.get());

  manager.set_power_manager(power_manager_.release());
  manager.Stop();
}

TEST_F(ManagerTest, RegisterUnknownService) {
  Manager manager(control_interface(),
                  dispatcher(),
                  metrics(),
                  run_path(),
                  storage_path(),
                  string());
  ProfileRefPtr profile(CreateProfileForManager(&manager));
  ASSERT_TRUE(profile.get());
  AdoptProfile(&manager, profile);
  {
    ServiceRefPtr service1(new ServiceUnderTest(control_interface(),
                                                dispatcher(),
                                                metrics(),
                                                &manager));
    ASSERT_TRUE(profile->AdoptService(service1));
    ASSERT_TRUE(profile->ContainsService(service1));
  }  // Force destruction of service1.
  scoped_refptr<MockService> mock_service2(
      new NiceMock<MockService>(control_interface(),
                                dispatcher(),
                                metrics(),
                                &manager));
  EXPECT_CALL(*mock_service2.get(), GetStorageIdentifier())
      .WillRepeatedly(Return(mock_service2->unique_name()));
  manager.RegisterService(mock_service2);
  EXPECT_NE(mock_service2->profile().get(), profile.get());

  manager.set_power_manager(power_manager_.release());
  manager.Stop();
}

TEST_F(ManagerTest, DeregisterUnregisteredService) {
  // WiFi assumes that it can deregister a service that is not
  // registered.  (E.g. a hidden service can be deregistered when it
  // loses its last endpoint, and again when WiFi is Stop()-ed.)
  //
  // So test that doing so doesn't cause a crash.
  MockServiceRefPtr service = new NiceMock<MockService>(control_interface(),
                                                        dispatcher(),
                                                        metrics(),
                                                        manager());
  manager()->DeregisterService(service);
}

TEST_F(ManagerTest, GetProperties) {
  AddMockProfileToManager(manager());
  {
    brillo::VariantDictionary props;
    Error error;
    string expected("portal_list");
    manager()->mutable_store()->SetStringProperty(
        kCheckPortalListProperty,
        expected,
        &error);
    manager()->store().GetProperties(&props, &error);
    ASSERT_FALSE(props.find(kCheckPortalListProperty) == props.end());
    EXPECT_TRUE(props[kCheckPortalListProperty].IsTypeCompatible<string>());
    EXPECT_EQ(props[kCheckPortalListProperty].Get<string>(), expected);
  }
  {
    brillo::VariantDictionary props;
    Error error;
    bool expected = true;
    manager()->mutable_store()->SetBoolProperty(kOfflineModeProperty,
                                                expected,
                                                &error);
    manager()->store().GetProperties(&props, &error);
    ASSERT_FALSE(props.find(kOfflineModeProperty) == props.end());
    EXPECT_TRUE(props[kOfflineModeProperty].IsTypeCompatible<bool>());
    EXPECT_EQ(props[kOfflineModeProperty].Get<bool>(), expected);
  }
}

TEST_F(ManagerTest, GetDevicesProperty) {
  AddMockProfileToManager(manager());
  manager()->RegisterDevice(mock_devices_[0]);
  manager()->RegisterDevice(mock_devices_[1]);
  {
    brillo::VariantDictionary props;
    Error error;
    manager()->store().GetProperties(&props, &error);
    ASSERT_FALSE(props.find(kDevicesProperty) == props.end());
    EXPECT_TRUE(
        props[kDevicesProperty].IsTypeCompatible<vector<dbus::ObjectPath>>());
    vector <dbus::ObjectPath> devices =
        props[kDevicesProperty].Get<vector<dbus::ObjectPath>>();
    EXPECT_EQ(2, devices.size());
  }
}

TEST_F(ManagerTest, GetServicesProperty) {
  AddMockProfileToManager(manager());
  brillo::VariantDictionary props;
  Error error;
  manager()->store().GetProperties(&props, &error);
  ASSERT_FALSE(props.find(kServicesProperty) == props.end());
  EXPECT_TRUE(
      props[kServicesProperty].IsTypeCompatible<vector<dbus::ObjectPath>>());
}

TEST_F(ManagerTest, MoveService) {
  Manager manager(control_interface(),
                  dispatcher(),
                  metrics(),
                  run_path(),
                  storage_path(),
                  string());
  scoped_refptr<MockService> s2(new MockService(control_interface(),
                                                dispatcher(),
                                                metrics(),
                                                &manager));
  // Inject an actual profile, backed by a fake StoreInterface
  {
    Profile::Identifier id("irrelevant");
    ProfileRefPtr profile(new Profile(
        control_interface(), metrics(), &manager, id, FilePath(), false));
    MockStore* storage = new MockStore;
    EXPECT_CALL(*storage, ContainsGroup(s2->GetStorageIdentifier()))
        .WillRepeatedly(Return(true));
    EXPECT_CALL(*storage, Flush())
        .Times(AnyNumber())
        .WillRepeatedly(Return(true));
    profile->set_storage(storage);
    AdoptProfile(&manager, profile);
  }
  // Create a profile that already has |s2| in it.
  ProfileRefPtr profile(
      new EphemeralProfile(control_interface(), metrics(), &manager));
  EXPECT_TRUE(profile->AdoptService(s2));

  // Now, move the Service |s2| to another profile.
  EXPECT_CALL(*s2.get(), Save(_)).WillOnce(Return(true));
  ASSERT_TRUE(manager.MoveServiceToProfile(s2, manager.ActiveProfile()));

  // Force destruction of the original Profile, to ensure that the Service
  // is kept alive and populated with data.
  profile = nullptr;
  ASSERT_TRUE(manager.ActiveProfile()->ContainsService(s2));
  manager.set_power_manager(power_manager_.release());
  manager.Stop();
}

TEST_F(ManagerTest, LookupProfileByRpcIdentifier) {
  scoped_refptr<MockProfile> mock_profile(
      new MockProfile(control_interface(), metrics(), manager(), ""));
  const string kProfileName("profile0");
  EXPECT_CALL(*mock_profile, GetRpcIdentifier())
      .WillRepeatedly(Return(kProfileName));
  AdoptProfile(manager(), mock_profile);

  EXPECT_FALSE(manager()->LookupProfileByRpcIdentifier("foo"));
  ProfileRefPtr profile = manager()->LookupProfileByRpcIdentifier(kProfileName);
  EXPECT_EQ(mock_profile.get(), profile.get());
}

TEST_F(ManagerTest, SetProfileForService) {
  scoped_refptr<MockProfile> profile0(
      new MockProfile(control_interface(), metrics(), manager(), ""));
  string profile_name0("profile0");
  EXPECT_CALL(*profile0, GetRpcIdentifier())
      .WillRepeatedly(Return(profile_name0));
  AdoptProfile(manager(), profile0);
  scoped_refptr<MockService> service(new MockService(control_interface(),
                                                     dispatcher(),
                                                     metrics(),
                                                     manager()));
  EXPECT_FALSE(manager()->HasService(service));
  {
    Error error;
    EXPECT_CALL(*profile0, AdoptService(_))
        .WillOnce(Return(true));
    // Expect that setting the profile of a service that does not already
    // have one assigned does not cause a crash.
    manager()->SetProfileForService(service, "profile0", &error);
    EXPECT_TRUE(error.IsSuccess());
  }

  // The service should be registered as a side-effect of the profile being
  // set for this service.
  EXPECT_TRUE(manager()->HasService(service));

  // Since we have mocked Profile::AdoptServie() above, the service's
  // profile was not actually changed.  Do so explicitly now.
  service->set_profile(profile0);

  {
    Error error;
    manager()->SetProfileForService(service, "foo", &error);
    EXPECT_EQ(Error::kInvalidArguments, error.type());
    EXPECT_EQ("Unknown Profile foo requested for Service", error.message());
  }

  {
    Error error;
    manager()->SetProfileForService(service, profile_name0, &error);
    EXPECT_EQ(Error::kInvalidArguments, error.type());
    EXPECT_EQ("Service is already connected to this profile", error.message());
  }

  scoped_refptr<MockProfile> profile1(
      new MockProfile(control_interface(), metrics(), manager(), ""));
  string profile_name1("profile1");
  EXPECT_CALL(*profile1, GetRpcIdentifier())
      .WillRepeatedly(Return(profile_name1));
  AdoptProfile(manager(), profile1);

  {
    Error error;
    EXPECT_CALL(*profile1, AdoptService(_))
        .WillOnce(Return(true));
    EXPECT_CALL(*profile0, AbandonService(_))
        .WillOnce(Return(true));
    manager()->SetProfileForService(service, profile_name1, &error);
    EXPECT_TRUE(error.IsSuccess());
  }
}

TEST_F(ManagerTest, CreateProfile) {
  ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());

  Manager manager(control_interface(),
                  dispatcher(),
                  metrics(),
                  run_path(),
                  storage_path(),
                  temp_dir.path().value());

  // Invalid name should be rejected.
  EXPECT_EQ(Error::kInvalidArguments, TestCreateProfile(&manager, ""));

  // A profile with invalid characters in it should similarly be rejected.
  EXPECT_EQ(Error::kInvalidArguments,
            TestCreateProfile(&manager, "valid_profile"));

  // We should be able to create a machine profile.
  EXPECT_EQ(Error::kSuccess, TestCreateProfile(&manager, "valid"));

  // We should succeed in creating a valid user profile.  Verify the returned
  // path.
  const char kProfile[] = "~user/profile";
  {
    Error error;
    string path;
    ASSERT_TRUE(base::CreateDirectory(temp_dir.path().Append("user")));
    manager.CreateProfile(kProfile, &path, &error);
    EXPECT_EQ(Error::kSuccess, error.type());
    EXPECT_EQ("/profile_rpc", path);
  }

  // We should fail in creating it a second time (already exists).
  EXPECT_EQ(Error::kAlreadyExists, TestCreateProfile(&manager, kProfile));
}

TEST_F(ManagerTest, PushPopProfile) {
  ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());
  Manager manager(control_interface(),
                  dispatcher(),
                  metrics(),
                  run_path(),
                  storage_path(),
                  temp_dir.path().value());
  vector<ProfileRefPtr>& profiles = GetProfiles(&manager);

  // Pushing an invalid profile should fail.
  EXPECT_EQ(Error::kInvalidArguments, TestPushProfile(&manager, ""));

  // Create and push a default profile. Should succeed.
  const char kDefaultProfile0[] = "default";
  ASSERT_EQ(Error::kSuccess, TestCreateProfile(&manager, kDefaultProfile0));
  EXPECT_EQ(Error::kSuccess, TestPushProfile(&manager, kDefaultProfile0));
  EXPECT_EQ(Error::kSuccess, TestPopProfile(&manager, kDefaultProfile0));

  // Pushing a default profile that does not exist on disk will _not_
  // fail, because we'll use temporary storage for it.
  const char kMissingDefaultProfile[] = "missingdefault";
  EXPECT_EQ(Error::kSuccess,
            TestPushProfile(&manager, kMissingDefaultProfile));
  EXPECT_EQ(1, profiles.size());
  EXPECT_EQ(Error::kSuccess,
            TestPopProfile(&manager, kMissingDefaultProfile));
  EXPECT_EQ(0, profiles.size());

  const char kProfile0[] = "~user/profile0";
  const char kProfile1[] = "~user/profile1";
  ASSERT_TRUE(base::CreateDirectory(temp_dir.path().Append("user")));

  // Create a couple of profiles.
  ASSERT_EQ(Error::kSuccess, TestCreateProfile(&manager, kProfile0));
  ASSERT_EQ(Error::kSuccess, TestCreateProfile(&manager, kProfile1));

  // Push these profiles on the stack.
  EXPECT_EQ(Error::kSuccess, TestPushProfile(&manager, kProfile0));
  EXPECT_EQ(Error::kSuccess, TestPushProfile(&manager, kProfile1));

  // Pushing a profile a second time should fail.
  EXPECT_EQ(Error::kAlreadyExists, TestPushProfile(&manager, kProfile0));
  EXPECT_EQ(Error::kAlreadyExists, TestPushProfile(&manager, kProfile1));

  Error error;
  // Active profile should be the last one we pushed.
  EXPECT_EQ(kProfile1, "~" + manager.ActiveProfile()->GetFriendlyName());

  // Make sure a profile name that doesn't exist fails.
  const char kProfile2Id[] = "profile2";
  const string kProfile2 = base::StringPrintf("~user/%s", kProfile2Id);
  EXPECT_EQ(Error::kNotFound, TestPushProfile(&manager, kProfile2));

  // Create a new service, with a specific storage name.
  scoped_refptr<MockService> service(
      new NiceMock<MockService>(control_interface(),
                                dispatcher(),
                                metrics(),
                                &manager));
  const char kServiceName[] = "service_storage_name";
  EXPECT_CALL(*service.get(), GetStorageIdentifier())
      .WillRepeatedly(Return(kServiceName));
  EXPECT_CALL(*service.get(), Load(_))
      .WillRepeatedly(Return(true));

  // Add this service to the manager -- it should end up in the ephemeral
  // profile.
  manager.RegisterService(service);
  ASSERT_EQ(GetEphemeralProfile(&manager), service->profile());

  // Create storage for a profile that contains the service storage name.
  ASSERT_TRUE(CreateBackingStoreForService(&temp_dir, "user", kProfile2Id,
                                           kServiceName));

  // When we push the profile, the service should move away from the
  // ephemeral profile to this new profile since it has an entry for
  // this service.
  EXPECT_CALL(*service, ClearExplicitlyDisconnected());
  EXPECT_EQ(Error::kSuccess, TestPushProfile(&manager, kProfile2));
  EXPECT_NE(GetEphemeralProfile(&manager), service->profile());
  EXPECT_EQ(kProfile2, "~" + service->profile()->GetFriendlyName());

  // Insert another profile that should supersede ownership of the service.
  const char kProfile3Id[] = "profile3";
  const string kProfile3 = base::StringPrintf("~user/%s", kProfile3Id);
  ASSERT_TRUE(CreateBackingStoreForService(&temp_dir, "user", kProfile3Id,
                                           kServiceName));
  // We don't verify this expectation inline, since this would clear other
  // recurring expectations on the service.
  EXPECT_CALL(*service, ClearExplicitlyDisconnected());
  EXPECT_EQ(Error::kSuccess, TestPushProfile(&manager, kProfile3));
  EXPECT_EQ(kProfile3, "~" + service->profile()->GetFriendlyName());

  // Popping an invalid profile name should fail.
  EXPECT_EQ(Error::kInvalidArguments, TestPopProfile(&manager, "~"));

  // Popping an profile that is not at the top of the stack should fail.
  EXPECT_EQ(Error::kNotSupported, TestPopProfile(&manager, kProfile0));

  // Popping the top profile should succeed.
  EXPECT_CALL(*service, ClearExplicitlyDisconnected());
  EXPECT_EQ(Error::kSuccess, TestPopProfile(&manager, kProfile3));

  // Moreover the service should have switched profiles to profile 2.
  EXPECT_EQ(kProfile2, "~" + service->profile()->GetFriendlyName());

  // Popping the top profile should succeed.
  EXPECT_CALL(*service, ClearExplicitlyDisconnected());
  EXPECT_EQ(Error::kSuccess, TestPopAnyProfile(&manager));

  // The service should now revert to the ephemeral profile.
  EXPECT_EQ(GetEphemeralProfile(&manager), service->profile());

  // Pop the remaining two profiles off the stack.
  EXPECT_CALL(*service, ClearExplicitlyDisconnected()).Times(2);
  EXPECT_EQ(Error::kSuccess, TestPopAnyProfile(&manager));
  EXPECT_EQ(Error::kSuccess, TestPopAnyProfile(&manager));
  Mock::VerifyAndClearExpectations(service.get());

  // Next pop should fail with "stack is empty".
  EXPECT_EQ(Error::kNotFound, TestPopAnyProfile(&manager));

  const char kMachineProfile0[] = "machineprofile0";
  const char kMachineProfile1[] = "machineprofile1";
  ASSERT_EQ(Error::kSuccess, TestCreateProfile(&manager, kMachineProfile0));
  ASSERT_EQ(Error::kSuccess, TestCreateProfile(&manager, kMachineProfile1));

  // Should be able to push a machine profile.
  EXPECT_EQ(Error::kSuccess, TestPushProfile(&manager, kMachineProfile0));

  // Should be able to push a user profile atop a machine profile.
  EXPECT_EQ(Error::kSuccess, TestPushProfile(&manager, kProfile0));

  // Pushing a system-wide profile on top of a user profile should fail.
  EXPECT_EQ(Error::kInvalidArguments,
            TestPushProfile(&manager, kMachineProfile1));

  // However if we pop the user profile, we should be able stack another
  // machine profile on.
  EXPECT_EQ(Error::kSuccess, TestPopAnyProfile(&manager));
  EXPECT_EQ(Error::kSuccess, TestPushProfile(&manager, kMachineProfile1));

  // Add two user profiles to the top of the stack.
  EXPECT_EQ(Error::kSuccess, TestPushProfile(&manager, kProfile0));
  EXPECT_EQ(Error::kSuccess, TestPushProfile(&manager, kProfile1));
  EXPECT_EQ(4, profiles.size());

  // PopAllUserProfiles should remove both user profiles, leaving the two
  // machine profiles.
  EXPECT_EQ(Error::kSuccess, TestPopAllUserProfiles(&manager));
  EXPECT_EQ(2, profiles.size());
  EXPECT_TRUE(profiles[0]->GetUser().empty());
  EXPECT_TRUE(profiles[1]->GetUser().empty());

  // Use InsertUserProfile() instead.  Although a machine profile is valid
  // in this state, it cannot be added via InsertUserProfile.
  EXPECT_EQ(Error::kSuccess, TestPopProfile(&manager, kMachineProfile1));
  EXPECT_EQ(Error::kInvalidArguments,
            TestInsertUserProfile(&manager, kMachineProfile1, "machinehash1"));
  const char kUserHash0[] = "userhash0";
  const char kUserHash1[] = "userhash1";
  EXPECT_EQ(Error::kSuccess,
            TestInsertUserProfile(&manager, kProfile0, kUserHash0));
  EXPECT_EQ(Error::kSuccess,
            TestInsertUserProfile(&manager, kProfile1, kUserHash1));
  EXPECT_EQ(3, profiles.size());
  EXPECT_EQ(kUserHash0, profiles[1]->GetUserHash());
  EXPECT_EQ(kUserHash1, profiles[2]->GetUserHash());
}

TEST_F(ManagerTest, RemoveProfile) {
  ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());
  Manager manager(control_interface(),
                  dispatcher(),
                  metrics(),
                  run_path(),
                  storage_path(),
                  temp_dir.path().value());

  const char kProfile0[] = "profile0";
  FilePath profile_path(
      Profile::GetFinalStoragePath(
          FilePath(storage_path()), Profile::Identifier(kProfile0)));

  ASSERT_EQ(Error::kSuccess, TestCreateProfile(&manager, kProfile0));
  ASSERT_TRUE(base::PathExists(profile_path));

  EXPECT_EQ(Error::kSuccess, TestPushProfile(&manager, kProfile0));

  // Remove should fail since the profile is still on the stack.
  {
    Error error;
    manager.RemoveProfile(kProfile0, &error);
    EXPECT_EQ(Error::kInvalidArguments, error.type());
  }

  // Profile path should still exist.
  EXPECT_TRUE(base::PathExists(profile_path));

  EXPECT_EQ(Error::kSuccess, TestPopAnyProfile(&manager));

  // This should succeed now that the profile is off the stack.
  {
    Error error;
    manager.RemoveProfile(kProfile0, &error);
    EXPECT_EQ(Error::kSuccess, error.type());
  }

  // Profile path should no longer exist.
  EXPECT_FALSE(base::PathExists(profile_path));

  // Another remove succeeds, due to a foible in base::DeleteFile --
  // it is not an error to delete a file that does not exist.
  {
    Error error;
    manager.RemoveProfile(kProfile0, &error);
    EXPECT_EQ(Error::kSuccess, error.type());
  }

  // Let's create an error case that will "work".  Create a non-empty
  // directory in the place of the profile pathname.
  ASSERT_TRUE(base::CreateDirectory(profile_path.Append("foo")));
  {
    Error error;
    manager.RemoveProfile(kProfile0, &error);
    EXPECT_EQ(Error::kOperationFailed, error.type());
  }
}

TEST_F(ManagerTest, RemoveService) {
  MockServiceRefPtr mock_service(
      new NiceMock<MockService>(control_interface(),
                                dispatcher(),
                                metrics(),
                                manager()));

  // Used in expectations which cannot accept a mock refptr.
  const ServiceRefPtr& service = mock_service;

  manager()->RegisterService(service);
  EXPECT_EQ(GetEphemeralProfile(manager()), service->profile().get());

  scoped_refptr<MockProfile> profile(
      new StrictMock<MockProfile>(
          control_interface(), metrics(), manager(), ""));
  AdoptProfile(manager(), profile);

  // If service is ephemeral, it should be unloaded and left ephemeral.
  EXPECT_CALL(*profile, AbandonService(service)).Times(0);
  EXPECT_CALL(*profile, ConfigureService(service)).Times(0);
  EXPECT_CALL(*mock_service, Unload()).WillOnce(Return(false));
  manager()->RemoveService(service);
  Mock::VerifyAndClearExpectations(mock_service.get());
  Mock::VerifyAndClearExpectations(profile.get());
  EXPECT_EQ(GetEphemeralProfile(manager()), service->profile().get());
  EXPECT_TRUE(manager()->HasService(service));  // Since Unload() was false.

  // If service is not ephemeral and the Manager finds a profile to assign
  // the service to, the service should be re-parented.  Note that since we
  // are using a MockProfile, ConfigureService() never actually changes the
  // Service's profile.
  service->set_profile(profile);
  EXPECT_CALL(*profile, AbandonService(service));
  EXPECT_CALL(*profile, ConfigureService(service)).WillOnce(Return(true));
  EXPECT_CALL(*mock_service, Unload()).Times(0);
  manager()->RemoveService(service);
  Mock::VerifyAndClearExpectations(mock_service.get());
  Mock::VerifyAndClearExpectations(profile.get());
  EXPECT_TRUE(manager()->HasService(service));
  EXPECT_EQ(profile.get(), service->profile().get());

  // If service becomes ephemeral since there is no profile to support it,
  // it should be unloaded.
  EXPECT_CALL(*profile, AbandonService(service));
  EXPECT_CALL(*profile, ConfigureService(service)).WillOnce(Return(false));
  EXPECT_CALL(*mock_service, Unload()).WillOnce(Return(true));
  manager()->RemoveService(service);
  EXPECT_FALSE(manager()->HasService(service));
}

TEST_F(ManagerTest, CreateDuplicateProfileWithMissingKeyfile) {
  ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());
  Manager manager(control_interface(),
                  dispatcher(),
                  metrics(),
                  run_path(),
                  storage_path(),
                  temp_dir.path().value());

  const char kProfile0[] = "profile0";
  FilePath profile_path(
      Profile::GetFinalStoragePath(
          FilePath(storage_path()), Profile::Identifier(kProfile0)));
  ASSERT_EQ(Error::kSuccess, TestCreateProfile(&manager, kProfile0));
  ASSERT_TRUE(base::PathExists(profile_path));
  EXPECT_EQ(Error::kSuccess, TestPushProfile(&manager, kProfile0));

  // Ensure that even if the backing filestore is removed, we still can't
  // create a profile twice.
  ASSERT_TRUE(base::DeleteFile(profile_path, false));
  EXPECT_EQ(Error::kAlreadyExists, TestCreateProfile(&manager, kProfile0));
}

TEST_F(ManagerTest, HandleProfileEntryDeletion) {
  MockServiceRefPtr s_not_in_profile(
      new NiceMock<MockService>(control_interface(),
                                dispatcher(),
                                metrics(),
                                manager()));
  MockServiceRefPtr s_not_in_group(
      new NiceMock<MockService>(control_interface(),
                                dispatcher(),
                                metrics(),
                                manager()));
  MockServiceRefPtr s_configure_fail(
      new NiceMock<MockService>(control_interface(),
                                dispatcher(),
                                metrics(),
                                manager()));
  MockServiceRefPtr s_configure_succeed(
      new NiceMock<MockService>(control_interface(),
                                dispatcher(),
                                metrics(),
                                manager()));

  string entry_name("entry_name");
  EXPECT_CALL(*s_not_in_profile.get(), GetStorageIdentifier()).Times(0);
  EXPECT_CALL(*s_not_in_group.get(), GetStorageIdentifier())
      .WillRepeatedly(Return("not_entry_name"));
  EXPECT_CALL(*s_configure_fail.get(), GetStorageIdentifier())
      .WillRepeatedly(Return(entry_name));
  EXPECT_CALL(*s_configure_succeed.get(), GetStorageIdentifier())
      .WillRepeatedly(Return(entry_name));

  manager()->RegisterService(s_not_in_profile);
  manager()->RegisterService(s_not_in_group);
  manager()->RegisterService(s_configure_fail);
  manager()->RegisterService(s_configure_succeed);

  scoped_refptr<MockProfile> profile0(
      new StrictMock<MockProfile>(
          control_interface(), metrics(), manager(), ""));
  scoped_refptr<MockProfile> profile1(
      new StrictMock<MockProfile>(
          control_interface(), metrics(), manager(), ""));

  s_not_in_group->set_profile(profile1);
  s_configure_fail->set_profile(profile1);
  s_configure_succeed->set_profile(profile1);

  AdoptProfile(manager(), profile0);
  AdoptProfile(manager(), profile1);

  CompleteServiceSort();

  // No services are a member of this profile.
  EXPECT_FALSE(manager()->HandleProfileEntryDeletion(profile0, entry_name));
  EXPECT_FALSE(IsSortServicesTaskPending());

  // No services that are members of this profile have this entry name.
  EXPECT_FALSE(manager()->HandleProfileEntryDeletion(profile1, ""));
  EXPECT_FALSE(IsSortServicesTaskPending());

  // Only services that are members of the profile and group will be abandoned.
  EXPECT_CALL(*profile1.get(),
              AbandonService(IsRefPtrTo(s_not_in_profile.get()))).Times(0);
  EXPECT_CALL(*profile1.get(),
              AbandonService(IsRefPtrTo(s_not_in_group.get()))).Times(0);
  EXPECT_CALL(*profile1.get(),
              AbandonService(IsRefPtrTo(s_configure_fail.get())))
      .WillOnce(Return(true));
  EXPECT_CALL(*profile1.get(),
              AbandonService(IsRefPtrTo(s_configure_succeed.get())))
      .WillOnce(Return(true));

  // Never allow services to re-join profile1.
  EXPECT_CALL(*profile1.get(), ConfigureService(_))
      .WillRepeatedly(Return(false));

  // Only allow one of the members of the profile and group to successfully
  // join profile0.
  EXPECT_CALL(*profile0.get(),
              ConfigureService(IsRefPtrTo(s_not_in_profile.get()))).Times(0);
  EXPECT_CALL(*profile0.get(),
              ConfigureService(IsRefPtrTo(s_not_in_group.get()))).Times(0);
  EXPECT_CALL(*profile0.get(),
              ConfigureService(IsRefPtrTo(s_configure_fail.get())))
      .WillOnce(Return(false));
  EXPECT_CALL(*profile0.get(),
              ConfigureService(IsRefPtrTo(s_configure_succeed.get())))
      .WillOnce(Return(true));

  // Expect the failed-to-configure service to have Unload() called on it.
  EXPECT_CALL(*s_not_in_profile.get(), Unload()).Times(0);
  EXPECT_CALL(*s_not_in_group.get(), Unload()).Times(0);
  EXPECT_CALL(*s_configure_fail.get(), Unload()).Times(1);
  EXPECT_CALL(*s_configure_succeed.get(), Unload()).Times(0);

  EXPECT_TRUE(manager()->HandleProfileEntryDeletion(profile1, entry_name));
  EXPECT_TRUE(IsSortServicesTaskPending());

  EXPECT_EQ(GetEphemeralProfile(manager()), s_not_in_profile->profile().get());
  EXPECT_EQ(profile1, s_not_in_group->profile());
  EXPECT_EQ(GetEphemeralProfile(manager()), s_configure_fail->profile());

  // Since we are using a MockProfile, the profile does not actually change,
  // since ConfigureService was not actually called on the service.
  EXPECT_EQ(profile1, s_configure_succeed->profile());
}

TEST_F(ManagerTest, HandleProfileEntryDeletionWithUnload) {
  MockServiceRefPtr s_will_remove0(
      new NiceMock<MockService>(control_interface(),
                                dispatcher(),
                                metrics(),
                                manager()));
  MockServiceRefPtr s_will_remove1(
      new NiceMock<MockService>(control_interface(),
                                dispatcher(),
                                metrics(),
                                manager()));
  MockServiceRefPtr s_will_not_remove0(
      new NiceMock<MockService>(control_interface(),
                                dispatcher(),
                                metrics(),
                                manager()));
  MockServiceRefPtr s_will_not_remove1(
      new NiceMock<MockService>(control_interface(),
                                dispatcher(),
                                metrics(),
                                manager()));

  EXPECT_CALL(*metrics(), NotifyDefaultServiceChanged(nullptr))
      .Times(4);  // Once for each registration.

  string entry_name("entry_name");
  EXPECT_CALL(*s_will_remove0.get(), GetStorageIdentifier())
      .WillRepeatedly(Return(entry_name));
  EXPECT_CALL(*s_will_remove1.get(), GetStorageIdentifier())
      .WillRepeatedly(Return(entry_name));
  EXPECT_CALL(*s_will_not_remove0.get(), GetStorageIdentifier())
      .WillRepeatedly(Return(entry_name));
  EXPECT_CALL(*s_will_not_remove1.get(), GetStorageIdentifier())
      .WillRepeatedly(Return(entry_name));

  manager()->RegisterService(s_will_remove0);
  CompleteServiceSort();
  manager()->RegisterService(s_will_not_remove0);
  CompleteServiceSort();
  manager()->RegisterService(s_will_remove1);
  CompleteServiceSort();
  manager()->RegisterService(s_will_not_remove1);
  CompleteServiceSort();

  // One for each service added above.
  ASSERT_EQ(4, manager()->services_.size());

  scoped_refptr<MockProfile> profile(
      new StrictMock<MockProfile>(
          control_interface(), metrics(), manager(), ""));

  s_will_remove0->set_profile(profile);
  s_will_remove1->set_profile(profile);
  s_will_not_remove0->set_profile(profile);
  s_will_not_remove1->set_profile(profile);

  AdoptProfile(manager(), profile);

  // Deny any of the services re-entry to the profile.
  EXPECT_CALL(*profile, ConfigureService(_))
      .WillRepeatedly(Return(false));

  EXPECT_CALL(*profile, AbandonService(ServiceRefPtr(s_will_remove0)))
      .WillOnce(Return(true));
  EXPECT_CALL(*profile, AbandonService(ServiceRefPtr(s_will_remove1)))
      .WillOnce(Return(true));
  EXPECT_CALL(*profile, AbandonService(ServiceRefPtr(s_will_not_remove0)))
      .WillOnce(Return(true));
  EXPECT_CALL(*profile, AbandonService(ServiceRefPtr(s_will_not_remove1)))
      .WillOnce(Return(true));

  EXPECT_CALL(*s_will_remove0, Unload())
      .WillOnce(Return(true));
  EXPECT_CALL(*s_will_remove1, Unload())
      .WillOnce(Return(true));
  EXPECT_CALL(*s_will_not_remove0, Unload())
      .WillOnce(Return(false));
  EXPECT_CALL(*s_will_not_remove1, Unload())
      .WillOnce(Return(false));


  // This will cause all the profiles to be unloaded.
  EXPECT_FALSE(IsSortServicesTaskPending());
  EXPECT_TRUE(manager()->HandleProfileEntryDeletion(profile, entry_name));
  EXPECT_TRUE(IsSortServicesTaskPending());

  // 2 of the 4 services added above should have been unregistered and
  // removed, leaving 2.
  EXPECT_EQ(2, manager()->services_.size());
  EXPECT_EQ(s_will_not_remove0.get(), manager()->services_[0].get());
  EXPECT_EQ(s_will_not_remove1.get(), manager()->services_[1].get());
}

TEST_F(ManagerTest, PopProfileWithUnload) {
  MockServiceRefPtr s_will_remove0(
      new NiceMock<MockService>(control_interface(),
                                dispatcher(),
                                metrics(),
                                manager()));
  MockServiceRefPtr s_will_remove1(
      new NiceMock<MockService>(control_interface(),
                                dispatcher(),
                                metrics(),
                                manager()));
  MockServiceRefPtr s_will_not_remove0(
      new NiceMock<MockService>(control_interface(),
                                dispatcher(),
                                metrics(),
                                manager()));
  MockServiceRefPtr s_will_not_remove1(
      new NiceMock<MockService>(control_interface(),
                                dispatcher(),
                                metrics(),
                                manager()));

  EXPECT_CALL(*metrics(), NotifyDefaultServiceChanged(nullptr))
      .Times(5);  // Once for each registration, and one after profile pop.

  manager()->RegisterService(s_will_remove0);
  CompleteServiceSort();
  manager()->RegisterService(s_will_not_remove0);
  CompleteServiceSort();
  manager()->RegisterService(s_will_remove1);
  CompleteServiceSort();
  manager()->RegisterService(s_will_not_remove1);
  CompleteServiceSort();

  // One for each service added above.
  ASSERT_EQ(4, manager()->services_.size());

  scoped_refptr<MockProfile> profile0(
      new StrictMock<MockProfile>(
          control_interface(), metrics(), manager(), ""));
  scoped_refptr<MockProfile> profile1(
      new StrictMock<MockProfile>(
          control_interface(), metrics(), manager(), ""));

  s_will_remove0->set_profile(profile1);
  s_will_remove1->set_profile(profile1);
  s_will_not_remove0->set_profile(profile1);
  s_will_not_remove1->set_profile(profile1);

  AdoptProfile(manager(), profile0);
  AdoptProfile(manager(), profile1);

  // Deny any of the services entry to profile0, so they will all be unloaded.
  EXPECT_CALL(*profile0, ConfigureService(_))
      .WillRepeatedly(Return(false));

  EXPECT_CALL(*s_will_remove0, Unload())
      .WillOnce(Return(true));
  EXPECT_CALL(*s_will_remove1, Unload())
      .WillOnce(Return(true));
  EXPECT_CALL(*s_will_not_remove0, Unload())
      .WillRepeatedly(Return(false));
  EXPECT_CALL(*s_will_not_remove1, Unload())
      .WillOnce(Return(false));

  // Ignore calls to Profile::GetRpcIdentifier because of emitted changes of the
  // profile list.
  EXPECT_CALL(*profile0, GetRpcIdentifier()).Times(AnyNumber());
  EXPECT_CALL(*profile1, GetRpcIdentifier()).Times(AnyNumber());

  // This will pop profile1, which should cause all our profiles to unload.
  manager()->PopProfileInternal();
  CompleteServiceSort();

  // 2 of the 4 services added above should have been unregistered and
  // removed, leaving 2.
  EXPECT_EQ(2, manager()->services_.size());
  EXPECT_EQ(s_will_not_remove0.get(), manager()->services_[0].get());
  EXPECT_EQ(s_will_not_remove1.get(), manager()->services_[1].get());

  // Expect the unloaded services to lose their profile reference.
  EXPECT_FALSE(s_will_remove0->profile());
  EXPECT_FALSE(s_will_remove1->profile());

  // If we explicitly deregister a service, the effect should be the same
  // with respect to the profile reference.
  ASSERT_TRUE(s_will_not_remove0->profile());
  manager()->DeregisterService(s_will_not_remove0);
  EXPECT_FALSE(s_will_not_remove0->profile());
}

TEST_F(ManagerTest, SetProperty) {
  {
    Error error;
    const bool offline_mode = true;
    EXPECT_TRUE(manager()->mutable_store()->SetAnyProperty(
        kOfflineModeProperty, brillo::Any(offline_mode), &error));
  }
  {
    Error error;
    const string country("a_country");
    EXPECT_TRUE(manager()->mutable_store()->SetAnyProperty(
        kCountryProperty, brillo::Any(country), &error));
  }
  // Attempt to write with value of wrong type should return InvalidArgs.
  {
    Error error;
    EXPECT_FALSE(manager()->mutable_store()->SetAnyProperty(
        kCountryProperty, PropertyStoreTest::kBoolV, &error));
    EXPECT_EQ(Error::kInvalidArguments, error.type());
  }
  {
    Error error;
    EXPECT_FALSE(manager()->mutable_store()->SetAnyProperty(
        kOfflineModeProperty, PropertyStoreTest::kStringV, &error));
    EXPECT_EQ(Error::kInvalidArguments, error.type());
  }
  // Attempt to write R/O property should return InvalidArgs.
  {
    Error error;
    EXPECT_FALSE(manager()->mutable_store()->SetAnyProperty(
        kEnabledTechnologiesProperty, PropertyStoreTest::kStringsV, &error));
    EXPECT_EQ(Error::kInvalidArguments, error.type());
  }
}

TEST_F(ManagerTest, RequestScan) {
  {
    Error error;
    manager()->RegisterDevice(mock_devices_[0].get());
    manager()->RegisterDevice(mock_devices_[1].get());
    EXPECT_CALL(*mock_devices_[0], technology())
        .WillRepeatedly(Return(Technology::kWifi));
    EXPECT_CALL(*mock_devices_[0], Scan(Device::kFullScan, _, _));
    EXPECT_CALL(*mock_devices_[1], technology())
        .WillRepeatedly(Return(Technology::kUnknown));
    EXPECT_CALL(*mock_devices_[1], Scan(_, _, _)).Times(0);
    EXPECT_CALL(*metrics(), NotifyUserInitiatedEvent(
        Metrics::kUserInitiatedEventWifiScan)).Times(1);
    manager()->RequestScan(Device::kFullScan, kTypeWifi, &error);
    manager()->DeregisterDevice(mock_devices_[0].get());
    manager()->DeregisterDevice(mock_devices_[1].get());
    Mock::VerifyAndClearExpectations(mock_devices_[0].get());
    Mock::VerifyAndClearExpectations(mock_devices_[1].get());

    manager()->RegisterDevice(mock_devices_[0].get());
    EXPECT_CALL(*mock_devices_[0], technology())
        .WillRepeatedly(Return(Technology::kWifi));
    EXPECT_CALL(*metrics(), NotifyUserInitiatedEvent(
        Metrics::kUserInitiatedEventWifiScan)).Times(1);
    EXPECT_CALL(*mock_devices_[0], Scan(Device::kFullScan, _, _));
    manager()->RequestScan(Device::kFullScan, kTypeWifi, &error);
    manager()->DeregisterDevice(mock_devices_[0].get());
    Mock::VerifyAndClearExpectations(mock_devices_[0].get());

    manager()->RegisterDevice(mock_devices_[0].get());
    EXPECT_CALL(*mock_devices_[0], technology())
        .WillRepeatedly(Return(Technology::kUnknown));
    EXPECT_CALL(*metrics(), NotifyUserInitiatedEvent(
        Metrics::kUserInitiatedEventWifiScan)).Times(0);
    EXPECT_CALL(*mock_devices_[0], Scan(_, _, _)).Times(0);
    manager()->RequestScan(Device::kFullScan, kTypeWifi, &error);
    manager()->DeregisterDevice(mock_devices_[0].get());
    Mock::VerifyAndClearExpectations(mock_devices_[0].get());
  }

  {
    Error error;
    manager()->RequestScan(Device::kFullScan, "bogus_device_type", &error);
    EXPECT_EQ(Error::kInvalidArguments, error.type());
  }
}

TEST_F(ManagerTest, GetServiceNoType) {
  KeyValueStore args;
  Error e;
  manager()->GetService(args, &e);
  EXPECT_EQ(Error::kInvalidArguments, e.type());
  EXPECT_EQ("must specify service type", e.message());
}

TEST_F(ManagerTest, GetServiceUnknownType) {
  KeyValueStore args;
  Error e;
  args.SetString(kTypeProperty, kTypeEthernet);
  manager()->GetService(args, &e);
  EXPECT_EQ(Error::kNotSupported, e.type());
  EXPECT_EQ("service type is unsupported", e.message());
}

#if !defined(DISABLE_WIRED_8021X)
TEST_F(ManagerTest, GetServiceEthernetEap) {
  KeyValueStore args;
  Error e;
  ServiceRefPtr service = new NiceMock<MockService>(control_interface(),
                                                    dispatcher(),
                                                    metrics(),
                                                    manager());
  args.SetString(kTypeProperty, kTypeEthernetEap);
  SetEapProviderService(service);
  EXPECT_EQ(service, manager()->GetService(args, &e));
  EXPECT_TRUE(e.IsSuccess());
}
#endif  // DISABLE_WIRED_8021X

#if !defined(DISABLE_WIFI)
TEST_F(ManagerTest, GetServiceWifi) {
  KeyValueStore args;
  Error e;
  WiFiServiceRefPtr wifi_service;
  args.SetString(kTypeProperty, kTypeWifi);
  EXPECT_CALL(*wifi_provider_, GetService(_, _))
      .WillRepeatedly(Return(wifi_service));
  manager()->GetService(args, &e);
  EXPECT_TRUE(e.IsSuccess());
}
#endif  // DISABLE_WIFI

TEST_F(ManagerTest, GetServiceVPNUnknownType) {
  KeyValueStore args;
  Error e;
  args.SetString(kTypeProperty, kTypeVPN);
  scoped_refptr<MockProfile> profile(
      new StrictMock<MockProfile>(
          control_interface(), metrics(), manager(), ""));
  AdoptProfile(manager(), profile);
  ServiceRefPtr service = manager()->GetService(args, &e);
  EXPECT_EQ(Error::kNotSupported, e.type());
  EXPECT_FALSE(service);
}

TEST_F(ManagerTest, GetServiceVPN) {
  KeyValueStore args;
  Error e;
  args.SetString(kTypeProperty, kTypeVPN);
  args.SetString(kProviderTypeProperty, kProviderOpenVpn);
  args.SetString(kProviderHostProperty, "10.8.0.1");
  args.SetString(kNameProperty, "vpn-name");
  scoped_refptr<MockProfile> profile(
      new StrictMock<MockProfile>(
          control_interface(), metrics(), manager(), ""));
  AdoptProfile(manager(), profile);

#if defined(DISABLE_VPN)

  ServiceRefPtr service = manager()->GetService(args, &e);
  EXPECT_EQ(Error::kNotSupported, e.type());
  EXPECT_FALSE(service);

#else

  ServiceRefPtr updated_service;
  EXPECT_CALL(*profile, UpdateService(_))
      .WillOnce(DoAll(SaveArg<0>(&updated_service), Return(true)));
  ServiceRefPtr configured_service;
  EXPECT_CALL(*profile, LoadService(_))
      .WillOnce(Return(false));
  EXPECT_CALL(*profile, ConfigureService(_))
      .WillOnce(DoAll(SaveArg<0>(&configured_service), Return(true)));
  ServiceRefPtr service = manager()->GetService(args, &e);
  EXPECT_TRUE(e.IsSuccess());
  EXPECT_TRUE(service);
  EXPECT_EQ(service, updated_service);
  EXPECT_EQ(service, configured_service);

#endif  // DISABLE_VPN
}

#if !defined(DISABLE_WIMAX)

TEST_F(ManagerTest, GetServiceWiMaxNoNetworkId) {
  KeyValueStore args;
  Error e;
  args.SetString(kTypeProperty, kTypeWimax);
  ServiceRefPtr service = manager()->GetService(args, &e);
  EXPECT_EQ(Error::kInvalidArguments, e.type());
  EXPECT_EQ("Missing WiMAX network id.", e.message());
  EXPECT_FALSE(service);
}

TEST_F(ManagerTest, GetServiceWiMax) {
  KeyValueStore args;
  Error e;
  args.SetString(kTypeProperty, kTypeWimax);
  args.SetString(WiMaxService::kNetworkIdProperty, "01234567");
  args.SetString(kNameProperty, "WiMAX Network");
  ServiceRefPtr service = manager()->GetService(args, &e);
  EXPECT_TRUE(e.IsSuccess());
  EXPECT_TRUE(service);
}

#endif  // DISABLE_WIMAX

TEST_F(ManagerTest, ConfigureServiceWithInvalidProfile) {
  // Manager calls ActiveProfile() so we need at least one profile installed.
  scoped_refptr<MockProfile> profile(
      new NiceMock<MockProfile>(
          control_interface(), metrics(), manager(), ""));
  AdoptProfile(manager(), profile);

  KeyValueStore args;
  args.SetString(kProfileProperty, "xxx");
  Error error;
  manager()->ConfigureService(args, &error);
  EXPECT_EQ(Error::kInvalidArguments, error.type());
  EXPECT_EQ("Invalid profile name xxx", error.message());
}

TEST_F(ManagerTest, ConfigureServiceWithGetServiceFailure) {
  // Manager calls ActiveProfile() so we need at least one profile installed.
  scoped_refptr<MockProfile> profile(
      new NiceMock<MockProfile>(
          control_interface(), metrics(), manager(), ""));
  AdoptProfile(manager(), profile);

  KeyValueStore args;
  Error error;
  manager()->ConfigureService(args, &error);
  EXPECT_EQ(Error::kInvalidArguments, error.type());
  EXPECT_EQ("must specify service type", error.message());
}

#if !defined(DISABLE_WIFI)
// TODO(zqiu): Consider creating a TestProvider to provide generic services,
// (MockService) instead of using technology specific (wifi) services. This
// will remove the dependency for wifi from ConfigureXXX tests.
//
// A registered service in the ephemeral profile should be moved to the
// active profile as a part of configuration if no profile was explicitly
// specified.
TEST_F(ManagerTest, ConfigureRegisteredServiceWithoutProfile) {
  scoped_refptr<MockProfile> profile(
      new NiceMock<MockProfile>(
          control_interface(), metrics(), manager(), ""));

  AdoptProfile(manager(), profile);  // This is now the active profile.

  const vector<uint8_t> ssid;
  scoped_refptr<MockWiFiService> service(
      new NiceMock<MockWiFiService>(control_interface(),
                                    dispatcher(),
                                    metrics(),
                                    manager(),
                                    wifi_provider_,
                                    ssid,
                                    "",
                                    "",
                                    false));

  manager()->RegisterService(service);
  service->set_profile(GetEphemeralProfile(manager()));

  EXPECT_CALL(*wifi_provider_, GetService(_, _))
      .WillOnce(Return(service));
  EXPECT_CALL(*profile, UpdateService(ServiceRefPtr(service.get())))
      .WillOnce(Return(true));
  EXPECT_CALL(*profile, AdoptService(ServiceRefPtr(service.get())))
      .WillOnce(Return(true));

  KeyValueStore args;
  args.SetString(kTypeProperty, kTypeWifi);
  Error error;
  manager()->ConfigureService(args, &error);
  EXPECT_TRUE(error.IsSuccess());
}

// If we configure a service that was already registered and explicitly
// specify a profile, it should be moved from the profile it was previously
// in to the specified profile if one was requested.
TEST_F(ManagerTest, ConfigureRegisteredServiceWithProfile) {
  scoped_refptr<MockProfile> profile0(
      new NiceMock<MockProfile>(
          control_interface(), metrics(), manager(), ""));
  scoped_refptr<MockProfile> profile1(
      new NiceMock<MockProfile>(
          control_interface(), metrics(), manager(), ""));

  const string kProfileName0 = "profile0";
  const string kProfileName1 = "profile1";

  EXPECT_CALL(*profile0, GetRpcIdentifier())
      .WillRepeatedly(Return(kProfileName0));
  EXPECT_CALL(*profile1, GetRpcIdentifier())
      .WillRepeatedly(Return(kProfileName1));

  AdoptProfile(manager(), profile0);
  AdoptProfile(manager(), profile1);  // profile1 is now the ActiveProfile.

  const vector<uint8_t> ssid;
  scoped_refptr<MockWiFiService> service(
      new NiceMock<MockWiFiService>(control_interface(),
                                    dispatcher(),
                                    metrics(),
                                    manager(),
                                    wifi_provider_,
                                    ssid,
                                    "",
                                    "",
                                    false));

  manager()->RegisterService(service);
  service->set_profile(profile1);

  EXPECT_CALL(*wifi_provider_, GetService(_, _))
      .WillOnce(Return(service));
  EXPECT_CALL(*profile0, LoadService(ServiceRefPtr(service.get())))
      .WillOnce(Return(true));
  EXPECT_CALL(*profile0, UpdateService(ServiceRefPtr(service.get())))
      .WillOnce(Return(true));
  EXPECT_CALL(*profile0, AdoptService(ServiceRefPtr(service.get())))
      .WillOnce(Return(true));
  EXPECT_CALL(*profile1, AbandonService(ServiceRefPtr(service.get())))
      .WillOnce(Return(true));

  KeyValueStore args;
  args.SetString(kTypeProperty, kTypeWifi);
  args.SetString(kProfileProperty, kProfileName0);
  Error error;
  manager()->ConfigureService(args, &error);
  EXPECT_TRUE(error.IsSuccess());
  service->set_profile(nullptr);  // Breaks refcounting loop.
}

// If we configure a service that is already a member of the specified
// profile, the Manager should not call LoadService or AdoptService again
// on this service.
TEST_F(ManagerTest, ConfigureRegisteredServiceWithSameProfile) {
  scoped_refptr<MockProfile> profile0(
      new NiceMock<MockProfile>(
          control_interface(), metrics(), manager(), ""));

  const string kProfileName0 = "profile0";

  EXPECT_CALL(*profile0, GetRpcIdentifier())
      .WillRepeatedly(Return(kProfileName0));

  AdoptProfile(manager(), profile0);  // profile0 is now the ActiveProfile.

  const vector<uint8_t> ssid;
  scoped_refptr<MockWiFiService> service(
      new NiceMock<MockWiFiService>(control_interface(),
                                    dispatcher(),
                                    metrics(),
                                    manager(),
                                    wifi_provider_,
                                    ssid,
                                    "",
                                    "",
                                    false));

  manager()->RegisterService(service);
  service->set_profile(profile0);

  EXPECT_CALL(*wifi_provider_, GetService(_, _))
      .WillOnce(Return(service));
  EXPECT_CALL(*profile0, LoadService(ServiceRefPtr(service.get())))
      .Times(0);
  EXPECT_CALL(*profile0, UpdateService(ServiceRefPtr(service.get())))
      .WillOnce(Return(true));
  EXPECT_CALL(*profile0, AdoptService(ServiceRefPtr(service.get())))
      .Times(0);

  KeyValueStore args;
  args.SetString(kTypeProperty, kTypeWifi);
  args.SetString(kProfileProperty, kProfileName0);
  Error error;
  manager()->ConfigureService(args, &error);
  EXPECT_TRUE(error.IsSuccess());
  service->set_profile(nullptr);  // Breaks refcounting loop.
}

// An unregistered service should remain unregistered, but its contents should
// be saved to the specified profile nonetheless.
TEST_F(ManagerTest, ConfigureUnregisteredServiceWithProfile) {
  scoped_refptr<MockProfile> profile0(
      new NiceMock<MockProfile>(
          control_interface(), metrics(), manager(), ""));
  scoped_refptr<MockProfile> profile1(
      new NiceMock<MockProfile>(
          control_interface(), metrics(), manager(), ""));

  const string kProfileName0 = "profile0";
  const string kProfileName1 = "profile1";

  EXPECT_CALL(*profile0, GetRpcIdentifier())
      .WillRepeatedly(Return(kProfileName0));
  EXPECT_CALL(*profile1, GetRpcIdentifier())
      .WillRepeatedly(Return(kProfileName1));

  AdoptProfile(manager(), profile0);
  AdoptProfile(manager(), profile1);  // profile1 is now the ActiveProfile.

  const vector<uint8_t> ssid;
  scoped_refptr<MockWiFiService> service(
      new NiceMock<MockWiFiService>(control_interface(),
                                    dispatcher(),
                                    metrics(),
                                    manager(),
                                    wifi_provider_,
                                    ssid,
                                    "",
                                    "",
                                    false));

  service->set_profile(profile1);

  EXPECT_CALL(*wifi_provider_, GetService(_, _))
      .WillOnce(Return(service));
  EXPECT_CALL(*profile0, UpdateService(ServiceRefPtr(service.get())))
      .WillOnce(Return(true));
  EXPECT_CALL(*profile0, AdoptService(_))
      .Times(0);
  EXPECT_CALL(*profile1, AdoptService(_))
      .Times(0);

  KeyValueStore args;
  args.SetString(kTypeProperty, kTypeWifi);
  args.SetString(kProfileProperty, kProfileName0);
  Error error;
  manager()->ConfigureService(args, &error);
  EXPECT_TRUE(error.IsSuccess());
}

TEST_F(ManagerTest, ConfigureServiceForProfileWithNoType) {
  KeyValueStore args;
  Error error;
  ServiceRefPtr service =
      manager()->ConfigureServiceForProfile("", args, &error);
  EXPECT_EQ(Error::kInvalidArguments, error.type());
  EXPECT_EQ("must specify service type", error.message());
  EXPECT_EQ(nullptr, service.get());
}

TEST_F(ManagerTest, ConfigureServiceForProfileWithWrongType) {
  KeyValueStore args;
  args.SetString(kTypeProperty, kTypeCellular);
  Error error;
  ServiceRefPtr service =
      manager()->ConfigureServiceForProfile("", args, &error);
  EXPECT_EQ(Error::kNotSupported, error.type());
  EXPECT_EQ("service type is unsupported", error.message());
  EXPECT_EQ(nullptr, service.get());
}

TEST_F(ManagerTest, ConfigureServiceForProfileWithMissingProfile) {
  KeyValueStore args;
  args.SetString(kTypeProperty, kTypeWifi);
  Error error;
  ServiceRefPtr service =
      manager()->ConfigureServiceForProfile("/profile/foo", args, &error);
  EXPECT_EQ(Error::kNotFound, error.type());
  EXPECT_EQ("Profile specified was not found", error.message());
  EXPECT_EQ(nullptr, service.get());
}

TEST_F(ManagerTest, ConfigureServiceForProfileWithProfileMismatch) {
  const string kProfileName0 = "profile0";
  const string kProfileName1 = "profile1";
  scoped_refptr<MockProfile> profile0(
      AddNamedMockProfileToManager(manager(), kProfileName0));

  KeyValueStore args;
  args.SetString(kTypeProperty, kTypeWifi);
  args.SetString(kProfileProperty, kProfileName1);
  Error error;
  ServiceRefPtr service =
      manager()->ConfigureServiceForProfile(kProfileName0, args, &error);
  EXPECT_EQ(Error::kInvalidArguments, error.type());
  EXPECT_EQ("Profile argument does not match that in "
            "the configuration arguments", error.message());
  EXPECT_EQ(nullptr, service.get());
}

TEST_F(ManagerTest,
       ConfigureServiceForProfileWithNoMatchingServiceFailGetService) {
  const string kProfileName0 = "profile0";
  scoped_refptr<MockProfile> profile0(
      AddNamedMockProfileToManager(manager(), kProfileName0));
  KeyValueStore args;
  args.SetString(kTypeProperty, kTypeWifi);
  args.SetString(kProfileProperty, kProfileName0);

  EXPECT_CALL(*wifi_provider_, FindSimilarService(_, _))
      .WillOnce(Return(WiFiServiceRefPtr()));
  EXPECT_CALL(*wifi_provider_, GetService(_, _))
      .WillOnce(Return(WiFiServiceRefPtr()));
  Error error;
  ServiceRefPtr service =
      manager()->ConfigureServiceForProfile(kProfileName0, args, &error);
  // Since we didn't set the error in the GetService expectation above...
  EXPECT_TRUE(error.IsSuccess());
  EXPECT_EQ(nullptr, service.get());
}

TEST_F(ManagerTest, ConfigureServiceForProfileCreateNewService) {
  const string kProfileName0 = "profile0";
  scoped_refptr<MockProfile> profile0(
      AddNamedMockProfileToManager(manager(), kProfileName0));

  KeyValueStore args;
  args.SetString(kTypeProperty, kTypeWifi);

  scoped_refptr<MockWiFiService> mock_service(
      new NiceMock<MockWiFiService>(control_interface(),
                                    dispatcher(),
                                    metrics(),
                                    manager(),
                                    wifi_provider_,
                                    vector<uint8_t>(),
                                    kModeManaged,
                                    kSecurityNone,
                                    false));
  ServiceRefPtr mock_service_generic(mock_service.get());
  mock_service->set_profile(profile0);
  EXPECT_CALL(*wifi_provider_, FindSimilarService(_, _))
      .WillOnce(Return(WiFiServiceRefPtr()));
  EXPECT_CALL(*wifi_provider_, GetService(_, _)).WillOnce(Return(mock_service));
  EXPECT_CALL(*profile0, UpdateService(mock_service_generic))
      .WillOnce(Return(true));
  Error error;
  ServiceRefPtr service =
      manager()->ConfigureServiceForProfile(kProfileName0, args, &error);
  EXPECT_TRUE(error.IsSuccess());
  EXPECT_EQ(mock_service.get(), service.get());
  mock_service->set_profile(nullptr);  // Breaks reference cycle.
}

TEST_F(ManagerTest, ConfigureServiceForProfileMatchingServiceByGUID) {
  scoped_refptr<MockService> mock_service(
      new NiceMock<MockService>(control_interface(),
                                dispatcher(),
                                metrics(),
                                manager()));
  const string kGUID = "a guid";
  mock_service->SetGuid(kGUID, nullptr);
  manager()->RegisterService(mock_service);
  ServiceRefPtr mock_service_generic(mock_service.get());

  const string kProfileName = "profile";
  scoped_refptr<MockProfile> profile(
      AddNamedMockProfileToManager(manager(), kProfileName));
  mock_service->set_profile(profile);

  EXPECT_CALL(*mock_service, technology())
     .WillOnce(Return(Technology::kCellular))
     .WillOnce(Return(Technology::kWifi));

  EXPECT_CALL(*wifi_provider_, FindSimilarService(_, _)).Times(0);
  EXPECT_CALL(*wifi_provider_, GetService(_, _)).Times(0);
  EXPECT_CALL(*profile, AdoptService(mock_service_generic)).Times(0);

  KeyValueStore args;
  args.SetString(kTypeProperty, kTypeWifi);
  args.SetString(kGuidProperty, kGUID);

  // The first attempt should fail because the service reports a technology
  // other than "WiFi".
  {
    Error error;
    ServiceRefPtr service =
        manager()->ConfigureServiceForProfile(kProfileName, args, &error);
    EXPECT_EQ(nullptr, service.get());
    EXPECT_EQ(Error::kNotSupported, error.type());
    EXPECT_EQ("This GUID matches a non-wifi service", error.message());
  }

  EXPECT_CALL(*mock_service, Configure(_, _)).Times(1);
  EXPECT_CALL(*profile, UpdateService(mock_service_generic)).Times(1);

  {
    Error error;
    ServiceRefPtr service =
        manager()->ConfigureServiceForProfile(kProfileName, args, &error);
    EXPECT_TRUE(error.IsSuccess());
    EXPECT_EQ(mock_service.get(), service.get());
    EXPECT_EQ(profile.get(), service->profile().get());
  }
  mock_service->set_profile(nullptr);  // Breaks reference cycle.
}

TEST_F(ManagerTest, ConfigureServiceForProfileMatchingServiceAndProfile) {
  const string kProfileName = "profile";
  scoped_refptr<MockProfile> profile(
      AddNamedMockProfileToManager(manager(), kProfileName));

  scoped_refptr<MockWiFiService> mock_service(
      new NiceMock<MockWiFiService>(control_interface(),
                                    dispatcher(),
                                    metrics(),
                                    manager(),
                                    wifi_provider_,
                                    vector<uint8_t>(),
                                    kModeManaged,
                                    kSecurityNone,
                                    false));
  mock_service->set_profile(profile);
  ServiceRefPtr mock_service_generic(mock_service.get());

  KeyValueStore args;
  args.SetString(kTypeProperty, kTypeWifi);
  EXPECT_CALL(*wifi_provider_, FindSimilarService(_, _))
      .WillOnce(Return(mock_service));
  EXPECT_CALL(*wifi_provider_, GetService(_, _)).Times(0);
  EXPECT_CALL(*profile, AdoptService(mock_service_generic)).Times(0);
  EXPECT_CALL(*mock_service, Configure(_, _)).Times(1);
  EXPECT_CALL(*profile, UpdateService(mock_service_generic)).Times(1);

  Error error;
  ServiceRefPtr service =
      manager()->ConfigureServiceForProfile(kProfileName, args, &error);
  EXPECT_TRUE(error.IsSuccess());
  EXPECT_EQ(mock_service.get(), service.get());
  EXPECT_EQ(profile.get(), service->profile().get());
  mock_service->set_profile(nullptr);  // Breaks reference cycle.
}

TEST_F(ManagerTest, ConfigureServiceForProfileMatchingServiceEphemeralProfile) {
  const string kProfileName = "profile";
  scoped_refptr<MockProfile> profile(
      AddNamedMockProfileToManager(manager(), kProfileName));

  scoped_refptr<MockWiFiService> mock_service(
      new NiceMock<MockWiFiService>(control_interface(),
                                    dispatcher(),
                                    metrics(),
                                    manager(),
                                    wifi_provider_,
                                    vector<uint8_t>(),
                                    kModeManaged,
                                    kSecurityNone,
                                    false));
  mock_service->set_profile(GetEphemeralProfile(manager()));
  ServiceRefPtr mock_service_generic(mock_service.get());

  KeyValueStore args;
  args.SetString(kTypeProperty, kTypeWifi);
  EXPECT_CALL(*wifi_provider_, FindSimilarService(_, _))
      .WillOnce(Return(mock_service));
  EXPECT_CALL(*wifi_provider_, GetService(_, _)).Times(0);
  EXPECT_CALL(*mock_service, Configure(_, _)).Times(1);
  EXPECT_CALL(*profile, UpdateService(mock_service_generic)).Times(1);

  Error error;
  ServiceRefPtr service =
      manager()->ConfigureServiceForProfile(kProfileName, args, &error);
  EXPECT_TRUE(error.IsSuccess());
  EXPECT_EQ(mock_service.get(), service.get());
  EXPECT_EQ(profile.get(), service->profile().get());
  mock_service->set_profile(nullptr);  // Breaks reference cycle.
}

TEST_F(ManagerTest, ConfigureServiceForProfileMatchingServicePrecedingProfile) {
  const string kProfileName0 = "profile0";
  scoped_refptr<MockProfile> profile0(
      AddNamedMockProfileToManager(manager(), kProfileName0));
  const string kProfileName1 = "profile1";
  scoped_refptr<MockProfile> profile1(
      AddNamedMockProfileToManager(manager(), kProfileName1));

  scoped_refptr<MockWiFiService> mock_service(
      new NiceMock<MockWiFiService>(control_interface(),
                                    dispatcher(),
                                    metrics(),
                                    manager(),
                                    wifi_provider_,
                                    vector<uint8_t>(),
                                    kModeManaged,
                                    kSecurityNone,
                                    false));
  manager()->RegisterService(mock_service);
  mock_service->set_profile(profile0);
  ServiceRefPtr mock_service_generic(mock_service.get());

  KeyValueStore args;
  args.SetString(kTypeProperty, kTypeWifi);
  EXPECT_CALL(*wifi_provider_, FindSimilarService(_, _))
      .WillOnce(Return(mock_service));
  EXPECT_CALL(*wifi_provider_, GetService(_, _)).Times(0);
  EXPECT_CALL(*profile0, AbandonService(_)).Times(0);
  EXPECT_CALL(*profile1, AdoptService(_)).Times(0);
  // This happens once to make the service loadable for the ConfigureService
  // below, and a second time after the service is modified.
  EXPECT_CALL(*profile1, ConfigureService(mock_service_generic)).Times(0);
  EXPECT_CALL(*wifi_provider_, CreateTemporaryService(_, _)).Times(0);
  EXPECT_CALL(*mock_service, Configure(_, _)).Times(1);
  EXPECT_CALL(*profile1, UpdateService(mock_service_generic)).Times(1);

  Error error;
  ServiceRefPtr service =
      manager()->ConfigureServiceForProfile(kProfileName1, args, &error);
  EXPECT_TRUE(error.IsSuccess());
  EXPECT_EQ(mock_service.get(), service.get());
  mock_service->set_profile(nullptr);  // Breaks reference cycle.
}

TEST_F(ManagerTest,
       ConfigureServiceForProfileMatchingServiceProceedingProfile) {
  const string kProfileName0 = "profile0";
  scoped_refptr<MockProfile> profile0(
      AddNamedMockProfileToManager(manager(), kProfileName0));
  const string kProfileName1 = "profile1";
  scoped_refptr<MockProfile> profile1(
      AddNamedMockProfileToManager(manager(), kProfileName1));

  scoped_refptr<MockWiFiService> matching_service(
      new StrictMock<MockWiFiService>(control_interface(),
                                      dispatcher(),
                                      metrics(),
                                      manager(),
                                      wifi_provider_,
                                      vector<uint8_t>(),
                                      kModeManaged,
                                      kSecurityNone,
                                      false));
  matching_service->set_profile(profile1);

  // We need to get rid of our reference to this mock service as soon
  // as Manager::ConfigureServiceForProfile() takes a reference in its
  // call to WiFiProvider::CreateTemporaryService().  This way the
  // latter function can keep a DCHECK(service->HasOneRef() even in
  // unit tests.
  temp_mock_service_ =
      new NiceMock<MockWiFiService>(control_interface(),
                                    dispatcher(),
                                    metrics(),
                                    manager(),
                                    wifi_provider_,
                                    vector<uint8_t>(),
                                    kModeManaged,
                                    kSecurityNone,
                                    false);

  // Only hold a pointer here so we don't affect the refcount.
  MockWiFiService* mock_service_ptr = temp_mock_service_.get();

  KeyValueStore args;
  args.SetString(kTypeProperty, kTypeWifi);
  EXPECT_CALL(*wifi_provider_, FindSimilarService(_, _))
      .WillOnce(Return(matching_service));
  EXPECT_CALL(*wifi_provider_, GetService(_, _)).Times(0);
  EXPECT_CALL(*profile1, AbandonService(_)).Times(0);
  EXPECT_CALL(*profile0, AdoptService(_)).Times(0);
  EXPECT_CALL(*wifi_provider_, CreateTemporaryService(_, _))
      .WillOnce(InvokeWithoutArgs(this, &ManagerTest::ReleaseTempMockService));
  EXPECT_CALL(*profile0, ConfigureService(IsRefPtrTo(mock_service_ptr)))
      .Times(1);
  EXPECT_CALL(*mock_service_ptr, Configure(_, _)).Times(1);
  EXPECT_CALL(*profile0, UpdateService(IsRefPtrTo(mock_service_ptr))).Times(1);

  Error error;
  ServiceRefPtr service =
      manager()->ConfigureServiceForProfile(kProfileName0, args, &error);
  EXPECT_TRUE(error.IsSuccess());
  EXPECT_EQ(nullptr, service.get());
  EXPECT_EQ(profile1.get(), matching_service->profile().get());
}

#if defined(__BRILLO__)
TEST_F(ManagerTest, SetupApModeInterface) {
  const string kApInterfaceName = "Test-Interface";
  string ap_interface;
  Error error;

  // Failed to setup AP mode interface.
  EXPECT_CALL(wifi_driver_hal_, SetupApModeInterface()).WillOnce(Return(""));
  EXPECT_FALSE(
      manager()->SetupApModeInterface(&ap_interface, &error));
  Mock::VerifyAndClearExpectations(&wifi_driver_hal_);
  EXPECT_TRUE(error.IsFailure());
  EXPECT_EQ("Failed to setup AP mode interface", error.message());

  // AP mode interface setup succeed.
  error.Reset();
  EXPECT_CALL(wifi_driver_hal_, SetupApModeInterface())
      .WillOnce(Return(kApInterfaceName));
  EXPECT_TRUE(
      manager()->SetupApModeInterface(&ap_interface, &error));
  Mock::VerifyAndClearExpectations(&wifi_driver_hal_);
  Mock::VerifyAndClearExpectations(control_interface());
  EXPECT_TRUE(error.IsSuccess());
  EXPECT_EQ(kApInterfaceName, ap_interface);
}

TEST_F(ManagerTest, SetupStationModeInterface) {
  const string kStationInterfaceName = "Test-Interface";
  string station_interface;
  Error error;

  // Failed to setup station mode interface.
  EXPECT_CALL(wifi_driver_hal_, SetupStationModeInterface())
      .WillOnce(Return(""));
  EXPECT_FALSE(
      manager()->SetupStationModeInterface(&station_interface, &error));
  Mock::VerifyAndClearExpectations(&wifi_driver_hal_);
  EXPECT_TRUE(error.IsFailure());
  EXPECT_EQ("Failed to setup station mode interface", error.message());

  // Station mode interface setup succeed.
  error.Reset();
  EXPECT_CALL(wifi_driver_hal_, SetupStationModeInterface())
      .WillOnce(Return(kStationInterfaceName));
  EXPECT_TRUE(
      manager()->SetupStationModeInterface(&station_interface, &error));
  Mock::VerifyAndClearExpectations(&wifi_driver_hal_);
  EXPECT_TRUE(error.IsSuccess());
  EXPECT_EQ(kStationInterfaceName, station_interface);
}

TEST_F(ManagerTest, OnApModeSetterVanished) {
  const string kStationInterfaceName = "Test-Interface";

  EXPECT_CALL(wifi_driver_hal_, SetupStationModeInterface())
      .WillOnce(Return(kStationInterfaceName));
  manager()->OnApModeSetterVanished();
  Mock::VerifyAndClearExpectations(&wifi_driver_hal_);
}
#endif  // __BRILLO__
#endif  // DISABLE_WIFI

TEST_F(ManagerTest, FindMatchingService) {
  KeyValueStore args;
  {
    Error error;
    ServiceRefPtr service = manager()->FindMatchingService(args, &error);
    EXPECT_EQ(Error::kNotFound, error.type());
  }

  scoped_refptr<MockService> mock_service0(
      new NiceMock<MockService>(control_interface(),
                                dispatcher(),
                                metrics(),
                                manager()));
  scoped_refptr<MockService> mock_service1(
      new NiceMock<MockService>(control_interface(),
                                dispatcher(),
                                metrics(),
                                manager()));
  manager()->RegisterService(mock_service0);
  manager()->RegisterService(mock_service1);
  EXPECT_CALL(*mock_service0, DoPropertiesMatch(_))
      .WillOnce(Return(true))
      .WillRepeatedly(Return(false));
  {
    Error error;
    EXPECT_EQ(mock_service0, manager()->FindMatchingService(args, &error));
    EXPECT_TRUE(error.IsSuccess());
  }
  EXPECT_CALL(*mock_service1, DoPropertiesMatch(_))
      .WillOnce(Return(true))
      .WillRepeatedly(Return(false));
  {
    Error error;
    EXPECT_EQ(mock_service1, manager()->FindMatchingService(args, &error));
    EXPECT_TRUE(error.IsSuccess());
  }
  {
    Error error;
    EXPECT_FALSE(manager()->FindMatchingService(args, &error));
    EXPECT_EQ(Error::kNotFound, error.type());
  }
}

TEST_F(ManagerTest, TechnologyOrder) {
  // If the Manager is not running, setting the technology order should not
  // lauch a service sorting task.
  SetRunning(false);
  Error error;
  manager()->SetTechnologyOrder("vpn,ethernet,wifi,wimax,cellular", &error);
  ASSERT_TRUE(error.IsSuccess());
  EXPECT_FALSE(IsSortServicesTaskPending());
  EXPECT_THAT(GetTechnologyOrder(), ElementsAre(Technology::kVPN,
                                                Technology::kEthernet,
                                                Technology::kWifi,
                                                Technology::kWiMax,
                                                Technology::kCellular));

  SetRunning(true);
  manager()->SetTechnologyOrder(string(kTypeEthernet) + "," + string(kTypeWifi),
                                &error);
  EXPECT_TRUE(IsSortServicesTaskPending());
  ASSERT_TRUE(error.IsSuccess());
  EXPECT_EQ(manager()->GetTechnologyOrder(),
            string(kTypeEthernet) + "," + string(kTypeWifi));

  manager()->SetTechnologyOrder(string(kTypeEthernet) + "x," +
                                string(kTypeWifi), &error);
  ASSERT_FALSE(error.IsSuccess());
  EXPECT_EQ(Error::kInvalidArguments, error.type());
  EXPECT_EQ(string(kTypeEthernet) + "," + string(kTypeWifi),
            manager()->GetTechnologyOrder());
}

TEST_F(ManagerTest, ConnectionStatusCheck) {
  // Setup mock metrics and service.
  MockMetrics mock_metrics(dispatcher());
  SetMetrics(&mock_metrics);
  scoped_refptr<MockService> mock_service = new NiceMock<MockService>(
      control_interface(), dispatcher(), metrics(), manager());
  manager()->RegisterService(mock_service);

  // Device not connected.
  EXPECT_CALL(*mock_service.get(), IsConnected())
      .WillOnce(Return(false));
  EXPECT_CALL(mock_metrics,
      NotifyDeviceConnectionStatus(Metrics::kConnectionStatusOffline));
  manager()->ConnectionStatusCheck();

  // Device connected, but not online.
  EXPECT_CALL(*mock_service.get(), IsConnected())
      .WillOnce(Return(true));
  EXPECT_CALL(*mock_service.get(), IsOnline())
      .WillOnce(Return(false));
  EXPECT_CALL(mock_metrics,
      NotifyDeviceConnectionStatus(Metrics::kConnectionStatusOnline)).Times(0);
  EXPECT_CALL(mock_metrics,
      NotifyDeviceConnectionStatus(Metrics::kConnectionStatusConnected));
  manager()->ConnectionStatusCheck();

  // Device connected and online.
  EXPECT_CALL(*mock_service.get(), IsConnected())
      .WillOnce(Return(true));
  EXPECT_CALL(*mock_service.get(), IsOnline())
      .WillOnce(Return(true));
  EXPECT_CALL(mock_metrics,
      NotifyDeviceConnectionStatus(Metrics::kConnectionStatusOnline));
  EXPECT_CALL(mock_metrics,
      NotifyDeviceConnectionStatus(Metrics::kConnectionStatusConnected));
  manager()->ConnectionStatusCheck();
}

TEST_F(ManagerTest, DevicePresenceStatusCheck) {
  // Setup mock metrics and service.
  MockMetrics mock_metrics(dispatcher());
  SetMetrics(&mock_metrics);

  manager()->RegisterDevice(mock_devices_[0]);
  manager()->RegisterDevice(mock_devices_[1]);
  manager()->RegisterDevice(mock_devices_[2]);
  manager()->RegisterDevice(mock_devices_[3]);

  ON_CALL(*mock_devices_[0].get(), technology())
      .WillByDefault(Return(Technology::kEthernet));
  ON_CALL(*mock_devices_[1].get(), technology())
      .WillByDefault(Return(Technology::kWifi));
  ON_CALL(*mock_devices_[2].get(), technology())
      .WillByDefault(Return(Technology::kCellular));
  ON_CALL(*mock_devices_[3].get(), technology())
      .WillByDefault(Return(Technology::kWifi));

  EXPECT_CALL(mock_metrics,
      NotifyDevicePresenceStatus(Technology::kEthernet, true));
  EXPECT_CALL(mock_metrics,
      NotifyDevicePresenceStatus(Technology::kWifi, true));
  EXPECT_CALL(mock_metrics,
      NotifyDevicePresenceStatus(Technology::kWiMax, false));
  EXPECT_CALL(mock_metrics,
      NotifyDevicePresenceStatus(Technology::kCellular, true));
  manager()->DevicePresenceStatusCheck();
}

TEST_F(ManagerTest, SortServicesWithConnection) {
  MockMetrics mock_metrics(dispatcher());
  SetMetrics(&mock_metrics);

  scoped_refptr<MockService> mock_service0(
      new NiceMock<MockService>(control_interface(),
                                dispatcher(),
                                metrics(),
                                manager()));
  scoped_refptr<MockService> mock_service1(
      new NiceMock<MockService>(control_interface(),
                                dispatcher(),
                                metrics(),
                                manager()));

  scoped_refptr<MockConnection> mock_connection0(
      new NiceMock<MockConnection>(device_info_.get()));
  scoped_refptr<MockConnection> mock_connection1(
      new NiceMock<MockConnection>(device_info_.get()));

  // A single registered Service, without a connection.  The
  // DefaultService should be nullptr.  If a change notification is
  // generated, it should reference kNullPath.
  EXPECT_CALL(mock_metrics, NotifyDefaultServiceChanged(nullptr));
  EXPECT_CALL(*manager_adaptor_,
              EmitRpcIdentifierChanged(
                  kDefaultServiceProperty,
                  control_interface()->NullRPCIdentifier()))
      .Times(AnyNumber());
  manager()->RegisterService(mock_service0);
  CompleteServiceSort();

  // Adding another Service, also without a connection, does not
  // change DefaultService.  Furthermore, we do not send a change
  // notification for DefaultService.
  EXPECT_CALL(mock_metrics, NotifyDefaultServiceChanged(nullptr));
  EXPECT_CALL(*manager_adaptor_,
              EmitRpcIdentifierChanged(kDefaultServiceProperty, _))
      .Times(0);
  manager()->RegisterService(mock_service1);
  CompleteServiceSort();

  // An explicit sort doesn't change anything, and does not emit a
  // change notification for DefaultService.
  EXPECT_CALL(mock_metrics, NotifyDefaultServiceChanged(nullptr));
  EXPECT_CALL(*manager_adaptor_,
              EmitRpcIdentifierChanged(kDefaultServiceProperty, _))
      .Times(0);
  manager()->SortServicesTask();
  EXPECT_TRUE(ServiceOrderIs(mock_service0, mock_service1));

  // Re-ordering the unconnected Services doesn't change
  // DefaultService, and (hence) does not emit a change notification
  // for DefaultService.
  mock_service1->SetPriority(1, nullptr);
  EXPECT_CALL(mock_metrics, NotifyDefaultServiceChanged(nullptr));
  EXPECT_CALL(*manager_adaptor_,
              EmitRpcIdentifierChanged(kDefaultServiceProperty, _))
      .Times(0);
  manager()->SortServicesTask();
  EXPECT_TRUE(ServiceOrderIs(mock_service1, mock_service0));

  // Re-ordering the unconnected Services doesn't change
  // DefaultService, and (hence) does not emit a change notification
  // for DefaultService.
  mock_service1->SetPriority(0, nullptr);
  EXPECT_CALL(mock_metrics, NotifyDefaultServiceChanged(nullptr));
  EXPECT_CALL(*manager_adaptor_,
              EmitRpcIdentifierChanged(kDefaultServiceProperty, _))
      .Times(0);
  manager()->SortServicesTask();
  EXPECT_TRUE(ServiceOrderIs(mock_service0, mock_service1));

  mock_service0->set_mock_connection(mock_connection0);
  mock_service1->set_mock_connection(mock_connection1);

  // If both Services have Connections, the DefaultService follows
  // from ServiceOrderIs.  We notify others of the change in
  // DefaultService.
  EXPECT_CALL(*mock_connection0.get(), SetIsDefault(true));
  EXPECT_CALL(mock_metrics, NotifyDefaultServiceChanged(mock_service0.get()));
  EXPECT_CALL(*manager_adaptor_,
              EmitRpcIdentifierChanged(kDefaultServiceProperty, _));
  manager()->SortServicesTask();
  EXPECT_TRUE(ServiceOrderIs(mock_service0, mock_service1));

  ServiceWatcher service_watcher;
  int tag =
      manager()->RegisterDefaultServiceCallback(
          Bind(&ServiceWatcher::OnDefaultServiceChanged,
               service_watcher.AsWeakPtr()));
  EXPECT_EQ(1, tag);

  // Changing the ordering causes the DefaultService to change, and
  // appropriate notifications are sent.
  mock_service1->SetPriority(1, nullptr);
  EXPECT_CALL(*mock_connection0.get(), SetIsDefault(false));
  EXPECT_CALL(*mock_connection1.get(), SetIsDefault(true));
  EXPECT_CALL(service_watcher, OnDefaultServiceChanged(_));
  EXPECT_CALL(mock_metrics, NotifyDefaultServiceChanged(mock_service1.get()));
  EXPECT_CALL(*manager_adaptor_,
              EmitRpcIdentifierChanged(kDefaultServiceProperty, _));
  manager()->SortServicesTask();
  EXPECT_TRUE(ServiceOrderIs(mock_service1, mock_service0));

  // Deregistering a DefaultServiceCallback works as expected.  (Later
  // code causes DefaultService changes, but we see no further calls
  // to |service_watcher|.)
  manager()->DeregisterDefaultServiceCallback(tag);
  EXPECT_CALL(service_watcher, OnDefaultServiceChanged(_)).Times(0);

  // Deregistering the current DefaultService causes the other Service
  // to become default.  Appropriate notifications are sent.
  EXPECT_CALL(*mock_connection0.get(), SetIsDefault(true));
  EXPECT_CALL(mock_metrics, NotifyDefaultServiceChanged(mock_service0.get()));
  EXPECT_CALL(*manager_adaptor_,
              EmitRpcIdentifierChanged(kDefaultServiceProperty, _));
  mock_service1->set_mock_connection(nullptr);  // So DeregisterService works.
  manager()->DeregisterService(mock_service1);
  CompleteServiceSort();

  // Deregistering the only Service causes the DefaultService to become
  // nullptr.  Appropriate notifications are sent.
  EXPECT_CALL(mock_metrics, NotifyDefaultServiceChanged(nullptr));
  EXPECT_CALL(*manager_adaptor_,
              EmitRpcIdentifierChanged(kDefaultServiceProperty, _));
  mock_service0->set_mock_connection(nullptr);  // So DeregisterService works.
  manager()->DeregisterService(mock_service0);
  CompleteServiceSort();

  // An explicit sort doesn't change anything, and does not generate
  // an external notification.
  EXPECT_CALL(mock_metrics, NotifyDefaultServiceChanged(nullptr));
  EXPECT_CALL(*manager_adaptor_,
              EmitRpcIdentifierChanged(kDefaultServiceProperty, _)).Times(0);
  manager()->SortServicesTask();
}

TEST_F(ManagerTest, NotifyDefaultServiceChanged) {
  EXPECT_EQ(0, manager()->default_service_callback_tag_);
  EXPECT_TRUE(manager()->default_service_callbacks_.empty());

  MockMetrics mock_metrics(dispatcher());
  SetMetrics(&mock_metrics);

  scoped_refptr<MockService> mock_service(
      new NiceMock<MockService>(
          control_interface(), dispatcher(), metrics(), manager()));
  ServiceRefPtr service = mock_service;
  ServiceRefPtr null_service;

  EXPECT_CALL(mock_metrics, NotifyDefaultServiceChanged(nullptr));
  manager()->NotifyDefaultServiceChanged(null_service);

  ServiceWatcher service_watcher1;
  ServiceWatcher service_watcher2;
  int tag1 =
      manager()->RegisterDefaultServiceCallback(
          Bind(&ServiceWatcher::OnDefaultServiceChanged,
               service_watcher1.AsWeakPtr()));
  EXPECT_EQ(1, tag1);
  int tag2 =
      manager()->RegisterDefaultServiceCallback(
          Bind(&ServiceWatcher::OnDefaultServiceChanged,
               service_watcher2.AsWeakPtr()));
  EXPECT_EQ(2, tag2);

  EXPECT_CALL(service_watcher1, OnDefaultServiceChanged(null_service));
  EXPECT_CALL(service_watcher2, OnDefaultServiceChanged(null_service));
  EXPECT_CALL(mock_metrics, NotifyDefaultServiceChanged(nullptr));
  manager()->NotifyDefaultServiceChanged(null_service);

  EXPECT_CALL(service_watcher1, OnDefaultServiceChanged(service));
  EXPECT_CALL(service_watcher2, OnDefaultServiceChanged(service));
  EXPECT_CALL(mock_metrics, NotifyDefaultServiceChanged(service.get()));
  manager()->NotifyDefaultServiceChanged(mock_service);

  manager()->DeregisterDefaultServiceCallback(tag1);
  EXPECT_CALL(service_watcher1, OnDefaultServiceChanged(_)).Times(0);
  EXPECT_CALL(service_watcher2, OnDefaultServiceChanged(service));
  EXPECT_CALL(mock_metrics, NotifyDefaultServiceChanged(service.get()));
  manager()->NotifyDefaultServiceChanged(mock_service);
  EXPECT_EQ(1, manager()->default_service_callbacks_.size());

  manager()->DeregisterDefaultServiceCallback(tag2);
  EXPECT_CALL(service_watcher2, OnDefaultServiceChanged(_)).Times(0);
  EXPECT_CALL(mock_metrics, NotifyDefaultServiceChanged(service.get()));
  manager()->NotifyDefaultServiceChanged(mock_service);

  EXPECT_EQ(2, manager()->default_service_callback_tag_);
  EXPECT_TRUE(manager()->default_service_callbacks_.empty());
}

TEST_F(ManagerTest, ReportServicesOnSameNetwork) {
  int connection_id1 = 100;
  int connection_id2 = 200;
  scoped_refptr<MockService> mock_service1 =
      new NiceMock<MockService>(control_interface(), dispatcher(),
                                metrics(), manager());
  mock_service1->set_connection_id(connection_id1);
  scoped_refptr<MockService> mock_service2 =
      new NiceMock<MockService>(control_interface(), dispatcher(),
                                metrics(), manager());
  mock_service2->set_connection_id(connection_id1);
  scoped_refptr<MockService> mock_service3 =
      new NiceMock<MockService>(control_interface(), dispatcher(),
                                metrics(), manager());
  mock_service3->set_connection_id(connection_id2);

  manager()->RegisterService(mock_service1);
  manager()->RegisterService(mock_service2);
  manager()->RegisterService(mock_service3);

  EXPECT_CALL(*metrics(), NotifyServicesOnSameNetwork(2));
  manager()->ReportServicesOnSameNetwork(connection_id1);

  EXPECT_CALL(*metrics(), NotifyServicesOnSameNetwork(1));
  manager()->ReportServicesOnSameNetwork(connection_id2);
}

TEST_F(ManagerTest, AvailableTechnologies) {
  mock_devices_.push_back(new NiceMock<MockDevice>(control_interface(),
                                                   dispatcher(),
                                                   metrics(),
                                                   manager(),
                                                   "null4",
                                                   "addr4",
                                                   0));
  manager()->RegisterDevice(mock_devices_[0]);
  manager()->RegisterDevice(mock_devices_[1]);
  manager()->RegisterDevice(mock_devices_[2]);
  manager()->RegisterDevice(mock_devices_[3]);

  ON_CALL(*mock_devices_[0].get(), technology())
      .WillByDefault(Return(Technology::kEthernet));
  ON_CALL(*mock_devices_[1].get(), technology())
      .WillByDefault(Return(Technology::kWifi));
  ON_CALL(*mock_devices_[2].get(), technology())
      .WillByDefault(Return(Technology::kCellular));
  ON_CALL(*mock_devices_[3].get(), technology())
      .WillByDefault(Return(Technology::kWifi));

  set<string> expected_technologies;
  expected_technologies.insert(Technology::NameFromIdentifier(
      Technology::kEthernet));
  expected_technologies.insert(Technology::NameFromIdentifier(
      Technology::kWifi));
  expected_technologies.insert(Technology::NameFromIdentifier(
      Technology::kCellular));
  Error error;
  vector<string> technologies = manager()->AvailableTechnologies(&error);

  EXPECT_THAT(set<string>(technologies.begin(), technologies.end()),
              ContainerEq(expected_technologies));
}

TEST_F(ManagerTest, ConnectedTechnologies) {
  scoped_refptr<MockService> connected_service1(
      new NiceMock<MockService>(control_interface(),
                                dispatcher(),
                                metrics(),
                                manager()));
  scoped_refptr<MockService> connected_service2(
      new NiceMock<MockService>(control_interface(),
                                dispatcher(),
                                metrics(),
                                manager()));
  scoped_refptr<MockService> disconnected_service1(
      new NiceMock<MockService>(control_interface(),
                                dispatcher(),
                                metrics(),
                                manager()));
  scoped_refptr<MockService> disconnected_service2(
      new NiceMock<MockService>(control_interface(),
                                dispatcher(),
                                metrics(),
                                manager()));

  ON_CALL(*connected_service1.get(), IsConnected())
      .WillByDefault(Return(true));
  ON_CALL(*connected_service2.get(), IsConnected())
      .WillByDefault(Return(true));

  manager()->RegisterService(connected_service1);
  manager()->RegisterService(connected_service2);
  manager()->RegisterService(disconnected_service1);
  manager()->RegisterService(disconnected_service2);

  manager()->RegisterDevice(mock_devices_[0]);
  manager()->RegisterDevice(mock_devices_[1]);
  manager()->RegisterDevice(mock_devices_[2]);
  manager()->RegisterDevice(mock_devices_[3]);

  ON_CALL(*mock_devices_[0].get(), technology())
      .WillByDefault(Return(Technology::kEthernet));
  ON_CALL(*mock_devices_[1].get(), technology())
      .WillByDefault(Return(Technology::kWifi));
  ON_CALL(*mock_devices_[2].get(), technology())
      .WillByDefault(Return(Technology::kCellular));
  ON_CALL(*mock_devices_[3].get(), technology())
      .WillByDefault(Return(Technology::kWifi));

  mock_devices_[0]->SelectService(connected_service1);
  mock_devices_[1]->SelectService(disconnected_service1);
  mock_devices_[2]->SelectService(disconnected_service2);
  mock_devices_[3]->SelectService(connected_service2);

  set<string> expected_technologies;
  expected_technologies.insert(Technology::NameFromIdentifier(
      Technology::kEthernet));
  expected_technologies.insert(Technology::NameFromIdentifier(
      Technology::kWifi));
  Error error;

  vector<string> technologies = manager()->ConnectedTechnologies(&error);
  EXPECT_THAT(set<string>(technologies.begin(), technologies.end()),
              ContainerEq(expected_technologies));
}

TEST_F(ManagerTest, DefaultTechnology) {
  scoped_refptr<MockService> connected_service(
      new NiceMock<MockService>(control_interface(),
                                dispatcher(),
                                metrics(),
                                manager()));
  scoped_refptr<MockService> disconnected_service(
      new NiceMock<MockService>(control_interface(),
                                dispatcher(),
                                metrics(),
                                manager()));

  // Connected. WiFi.
  ON_CALL(*connected_service.get(), IsConnected())
      .WillByDefault(Return(true));
  ON_CALL(*connected_service.get(), state())
      .WillByDefault(Return(Service::kStateConnected));
  ON_CALL(*connected_service.get(), technology())
      .WillByDefault(Return(Technology::kWifi));

  // Disconnected. Ethernet.
  ON_CALL(*disconnected_service.get(), technology())
      .WillByDefault(Return(Technology::kEthernet));

  manager()->RegisterService(disconnected_service);
  CompleteServiceSort();
  Error error;
  EXPECT_THAT(manager()->DefaultTechnology(&error), StrEq(""));


  manager()->RegisterService(connected_service);
  CompleteServiceSort();
  // Connected service should be brought to the front now.
  string expected_technology =
      Technology::NameFromIdentifier(Technology::kWifi);
  EXPECT_THAT(manager()->DefaultTechnology(&error), StrEq(expected_technology));
}

TEST_F(ManagerTest, Stop) {
  scoped_refptr<MockProfile> profile(
      new NiceMock<MockProfile>(
          control_interface(), metrics(), manager(), ""));
  AdoptProfile(manager(), profile);
  scoped_refptr<MockService> service(
      new NiceMock<MockService>(control_interface(),
                                dispatcher(),
                                metrics(),
                                manager()));
  manager()->RegisterService(service);
  manager()->RegisterDevice(mock_devices_[0]);
  SetPowerManager();
  EXPECT_TRUE(manager()->power_manager());
  EXPECT_CALL(*profile.get(),
              UpdateDevice(DeviceRefPtr(mock_devices_[0].get())))
      .WillOnce(Return(true));
  EXPECT_CALL(*mock_devices_[0].get(), SetEnabled(false));
#if !defined(DISABLE_WIFI)
  EXPECT_CALL(*profile.get(), UpdateWiFiProvider(_)).WillOnce(Return(true));
#endif  // DISABLE_WIFI
  EXPECT_CALL(*profile.get(), Save()).WillOnce(Return(true));
  EXPECT_CALL(*service.get(), Disconnect(_, HasSubstr("Stop"))).Times(1);
  manager()->Stop();
  EXPECT_FALSE(manager()->power_manager());
}

TEST_F(ManagerTest, UpdateServiceConnected) {
  scoped_refptr<MockService> mock_service(
      new NiceMock<MockService>(control_interface(),
                                dispatcher(),
                                metrics(),
                                manager()));
  manager()->RegisterService(mock_service);
  EXPECT_FALSE(mock_service->retain_auto_connect());
  EXPECT_FALSE(mock_service->auto_connect());

  EXPECT_CALL(*mock_service.get(), IsConnected())
      .WillRepeatedly(Return(true));
  EXPECT_CALL(*mock_service.get(), EnableAndRetainAutoConnect());
  manager()->UpdateService(mock_service);
}

TEST_F(ManagerTest, UpdateServiceConnectedPersistAutoConnect) {
  // This tests the case where the user connects to a service that is
  // currently associated with a profile.  We want to make sure that the
  // auto_connect flag is set and that the is saved to the current profile.
  scoped_refptr<MockService> mock_service(
      new NiceMock<MockService>(control_interface(),
                                dispatcher(),
                                metrics(),
                                manager()));
  manager()->RegisterService(mock_service);
  EXPECT_FALSE(mock_service->retain_auto_connect());
  EXPECT_FALSE(mock_service->auto_connect());

  scoped_refptr<MockProfile> profile(
      new MockProfile(
          control_interface(), metrics(), manager(), ""));

  mock_service->set_profile(profile);
  EXPECT_CALL(*mock_service, IsConnected())
      .WillRepeatedly(Return(true));
  EXPECT_CALL(*profile,
              UpdateService(static_cast<ServiceRefPtr>(mock_service)));
  EXPECT_CALL(*mock_service.get(), EnableAndRetainAutoConnect());
  manager()->UpdateService(mock_service);
  // This releases the ref on the mock profile.
  mock_service->set_profile(nullptr);
}

TEST_F(ManagerTest, UpdateServiceLogging) {
  ScopedMockLog log;
  MockServiceRefPtr mock_service(
      new NiceMock<MockService>(control_interface(),
                                dispatcher(),
                                metrics(),
                                manager()));
  string updated_message = base::StringPrintf(
      "Service %s updated;", mock_service->unique_name().c_str());

  // An idle service should not create a log message by default.
  EXPECT_CALL(*mock_service.get(), state())
      .WillRepeatedly(Return(Service::kStateIdle));
  EXPECT_CALL(log, Log(logging::LOG_INFO, _, HasSubstr(updated_message)))
      .Times(0);
  manager()->RegisterService(mock_service);
  CompleteServiceSort();
  manager()->UpdateService(mock_service);
  CompleteServiceSort();
  Mock::VerifyAndClearExpectations(mock_service.get());
  Mock::VerifyAndClearExpectations(&log);

  // A service leaving the idle state should create a log message.
  EXPECT_CALL(*mock_service.get(), state())
      .WillRepeatedly(Return(Service::kStateAssociating));
  EXPECT_CALL(log, Log(logging::LOG_INFO, _, HasSubstr(updated_message)))
      .Times(1);
  manager()->UpdateService(mock_service.get());
  CompleteServiceSort();
  Mock::VerifyAndClearExpectations(&log);

  // A service in a non-idle state should not create a log message if its
  // state did not change.
  EXPECT_CALL(log, Log(logging::LOG_INFO, _, HasSubstr(updated_message)))
      .Times(0);
  manager()->UpdateService(mock_service);
  CompleteServiceSort();
  Mock::VerifyAndClearExpectations(mock_service.get());
  Mock::VerifyAndClearExpectations(&log);

  // A service transitioning between two non-idle states should create
  // a log message.
  EXPECT_CALL(*mock_service.get(), state())
      .WillRepeatedly(Return(Service::kStateConnected));
  EXPECT_CALL(log, Log(logging::LOG_INFO, _, HasSubstr(updated_message)))
      .Times(1);
  manager()->UpdateService(mock_service.get());
  CompleteServiceSort();
  Mock::VerifyAndClearExpectations(mock_service.get());
  Mock::VerifyAndClearExpectations(&log);

  // A service transitioning from a non-idle state to idle should create
  // a log message.
  EXPECT_CALL(*mock_service.get(), state())
      .WillRepeatedly(Return(Service::kStateIdle));
  EXPECT_CALL(log, Log(logging::LOG_INFO, _, HasSubstr(updated_message)))
      .Times(1);
  manager()->UpdateService(mock_service.get());
  CompleteServiceSort();
}

TEST_F(ManagerTest, SaveSuccessfulService) {
  scoped_refptr<MockProfile> profile(
      new StrictMock<MockProfile>(
          control_interface(), metrics(), manager(), ""));
  AdoptProfile(manager(), profile);
  scoped_refptr<MockService> service(
      new NiceMock<MockService>(control_interface(),
                                dispatcher(),
                                metrics(),
                                manager()));

  // Re-cast this back to a ServiceRefPtr, so EXPECT arguments work correctly.
  ServiceRefPtr expect_service(service.get());

  EXPECT_CALL(*profile.get(), ConfigureService(expect_service))
      .WillOnce(Return(false));
  manager()->RegisterService(service);

  EXPECT_CALL(*service.get(), state())
      .WillRepeatedly(Return(Service::kStateConnected));
  EXPECT_CALL(*service.get(), IsConnected())
      .WillRepeatedly(Return(true));
  EXPECT_CALL(*profile.get(), AdoptService(expect_service))
      .WillOnce(Return(true));
  manager()->UpdateService(service);
}

TEST_F(ManagerTest, UpdateDevice) {
  MockProfile* profile0 =
      new MockProfile(control_interface(), metrics(), manager(), "");
  MockProfile* profile1 =
      new MockProfile(control_interface(), metrics(), manager(), "");
  MockProfile* profile2 =
      new MockProfile(control_interface(), metrics(), manager(), "");
  AdoptProfile(manager(), profile0);  // Passes ownership.
  AdoptProfile(manager(), profile1);  // Passes ownership.
  AdoptProfile(manager(), profile2);  // Passes ownership.
  DeviceRefPtr device_ref(mock_devices_[0].get());
  EXPECT_CALL(*profile0, UpdateDevice(device_ref)).Times(0);
  EXPECT_CALL(*profile1, UpdateDevice(device_ref)).WillOnce(Return(true));
  EXPECT_CALL(*profile2, UpdateDevice(device_ref)).WillOnce(Return(false));
  manager()->UpdateDevice(mock_devices_[0]);
}

TEST_F(ManagerTest, EnumerateProfiles) {
  vector<string> profile_paths;
  for (size_t i = 0; i < 10; i++) {
    scoped_refptr<MockProfile> profile(
      new StrictMock<MockProfile>(
          control_interface(), metrics(), manager(), ""));
    profile_paths.push_back(base::StringPrintf("/profile/%zd", i));
    EXPECT_CALL(*profile.get(), GetRpcIdentifier())
        .WillOnce(Return(profile_paths.back()));
    AdoptProfile(manager(), profile);
  }

  Error error;
  vector<string> returned_paths = manager()->EnumerateProfiles(&error);
  EXPECT_TRUE(error.IsSuccess());
  EXPECT_EQ(profile_paths.size(), returned_paths.size());
  for (size_t i = 0; i < profile_paths.size(); i++) {
    EXPECT_EQ(profile_paths[i], returned_paths[i]);
  }
}

TEST_F(ManagerTest, EnumerateServiceInnerDevices) {
  MockServiceRefPtr service1 = new NiceMock<MockService>(control_interface(),
                                                         dispatcher(),
                                                         metrics(),
                                                         manager());
  MockServiceRefPtr service2 = new NiceMock<MockService>(control_interface(),
                                                         dispatcher(),
                                                         metrics(),
                                                         manager());
  const string kDeviceRpcID = "/rpc/";
  manager()->RegisterService(service1);
  manager()->RegisterService(service2);
  EXPECT_CALL(*service1.get(), GetInnerDeviceRpcIdentifier())
      .WillRepeatedly(Return(kDeviceRpcID));
  EXPECT_CALL(*service2.get(), GetInnerDeviceRpcIdentifier())
      .WillRepeatedly(Return(""));
  Error error;
  EXPECT_EQ(vector<string>{kDeviceRpcID}, manager()->EnumerateDevices(&error));
  EXPECT_TRUE(error.IsSuccess());
}

TEST_F(ManagerTest, AutoConnectOnRegister) {
  MockServiceRefPtr service = MakeAutoConnectableService();
  EXPECT_CALL(*service.get(), AutoConnect());
  manager()->RegisterService(service);
  dispatcher()->DispatchPendingEvents();
}

TEST_F(ManagerTest, AutoConnectOnUpdate) {
  MockServiceRefPtr service1 = MakeAutoConnectableService();
  service1->SetPriority(1, nullptr);
  MockServiceRefPtr service2 = MakeAutoConnectableService();
  service2->SetPriority(2, nullptr);
  manager()->RegisterService(service1);
  manager()->RegisterService(service2);
  dispatcher()->DispatchPendingEvents();

  EXPECT_CALL(*service1.get(), AutoConnect());
  EXPECT_CALL(*service2.get(), state())
      .WillRepeatedly(Return(Service::kStateFailure));
  EXPECT_CALL(*service2.get(), IsFailed())
      .WillRepeatedly(Return(true));
  EXPECT_CALL(*service2.get(), IsConnected())
      .WillRepeatedly(Return(false));
  manager()->UpdateService(service2);
  dispatcher()->DispatchPendingEvents();
}

TEST_F(ManagerTest, AutoConnectOnDeregister) {
  MockServiceRefPtr service1 = MakeAutoConnectableService();
  service1->SetPriority(1, nullptr);
  MockServiceRefPtr service2 = MakeAutoConnectableService();
  service2->SetPriority(2, nullptr);
  manager()->RegisterService(service1);
  manager()->RegisterService(service2);
  dispatcher()->DispatchPendingEvents();

  EXPECT_CALL(*service1.get(), AutoConnect());
  manager()->DeregisterService(service2);
  dispatcher()->DispatchPendingEvents();
}

TEST_F(ManagerTest, AutoConnectOnSuspending) {
  MockServiceRefPtr service = MakeAutoConnectableService();
  SetSuspending(true);
  SetPowerManager();
  EXPECT_CALL(*service, AutoConnect()).Times(0);
  manager()->RegisterService(service);
  dispatcher()->DispatchPendingEvents();
}

TEST_F(ManagerTest, AutoConnectOnNotSuspending) {
  MockServiceRefPtr service = MakeAutoConnectableService();
  SetSuspending(false);
  SetPowerManager();
  EXPECT_CALL(*service, AutoConnect());
  manager()->RegisterService(service);
  dispatcher()->DispatchPendingEvents();
}

TEST_F(ManagerTest, AutoConnectWhileNotRunning) {
  SetRunning(false);
  MockServiceRefPtr service = MakeAutoConnectableService();
  EXPECT_CALL(*service, AutoConnect()).Times(0);
  manager()->RegisterService(service);
  dispatcher()->DispatchPendingEvents();
}

TEST_F(ManagerTest, Suspend) {
  MockServiceRefPtr service = MakeAutoConnectableService();
  SetPowerManager();
  EXPECT_CALL(*service, AutoConnect());
  manager()->RegisterService(service);
  manager()->RegisterDevice(mock_devices_[0]);
  dispatcher()->DispatchPendingEvents();

  EXPECT_CALL(*mock_devices_[0], OnBeforeSuspend(_));
  OnSuspendImminent();
  EXPECT_CALL(*service, AutoConnect()).Times(0);
  dispatcher()->DispatchPendingEvents();
  Mock::VerifyAndClearExpectations(mock_devices_[0].get());

  EXPECT_CALL(*mock_devices_[0], OnAfterResume());
  OnSuspendDone();
  EXPECT_CALL(*service, AutoConnect());
  dispatcher()->DispatchPendingEvents();
  Mock::VerifyAndClearExpectations(mock_devices_[0].get());
}

TEST_F(ManagerTest, AddTerminationAction) {
  EXPECT_TRUE(GetTerminationActions()->IsEmpty());
  manager()->AddTerminationAction("action1", base::Closure());
  EXPECT_FALSE(GetTerminationActions()->IsEmpty());
  manager()->AddTerminationAction("action2", base::Closure());
}

TEST_F(ManagerTest, RemoveTerminationAction) {
  const char kKey1[] = "action1";
  const char kKey2[] = "action2";

  // Removing an action when the hook table is empty.
  EXPECT_TRUE(GetTerminationActions()->IsEmpty());
  manager()->RemoveTerminationAction("unknown");

  // Fill hook table with two items.
  manager()->AddTerminationAction(kKey1, base::Closure());
  EXPECT_FALSE(GetTerminationActions()->IsEmpty());
  manager()->AddTerminationAction(kKey2, base::Closure());

  // Removing an action that ends up with a non-empty hook table.
  manager()->RemoveTerminationAction(kKey1);
  EXPECT_FALSE(GetTerminationActions()->IsEmpty());

  // Removing the last action.
  manager()->RemoveTerminationAction(kKey2);
  EXPECT_TRUE(GetTerminationActions()->IsEmpty());
}

TEST_F(ManagerTest, RunTerminationActions) {
  TerminationActionTest test_action;
  const string kActionName = "action";

  EXPECT_CALL(test_action, Done(_));
  manager()->RunTerminationActions(Bind(&TerminationActionTest::Done,
                                        test_action.AsWeakPtr()));

  manager()->AddTerminationAction(TerminationActionTest::kActionName,
                                  Bind(&TerminationActionTest::Action,
                                       test_action.AsWeakPtr()));
  test_action.set_manager(manager());
  EXPECT_CALL(test_action, Done(_));
  manager()->RunTerminationActions(Bind(&TerminationActionTest::Done,
                                        test_action.AsWeakPtr()));
}

TEST_F(ManagerTest, OnSuspendImminentDevicesPresent) {
  EXPECT_CALL(*mock_devices_[0].get(), OnBeforeSuspend(_));
  EXPECT_CALL(*mock_devices_[1].get(), OnBeforeSuspend(_));
  EXPECT_CALL(*mock_devices_[2].get(), OnBeforeSuspend(_));
  manager()->RegisterDevice(mock_devices_[0]);
  manager()->RegisterDevice(mock_devices_[1]);
  manager()->RegisterDevice(mock_devices_[2]);
  SetPowerManager();
  OnSuspendImminent();
}

TEST_F(ManagerTest, OnSuspendImminentNoDevicesPresent) {
  EXPECT_CALL(*power_manager_, ReportSuspendReadiness());
  SetPowerManager();
  OnSuspendImminent();
}

TEST_F(ManagerTest, OnDarkSuspendImminentDevicesPresent) {
  EXPECT_CALL(*mock_devices_[0].get(), OnDarkResume(_));
  EXPECT_CALL(*mock_devices_[1].get(), OnDarkResume(_));
  EXPECT_CALL(*mock_devices_[2].get(), OnDarkResume(_));
  manager()->RegisterDevice(mock_devices_[0]);
  manager()->RegisterDevice(mock_devices_[1]);
  manager()->RegisterDevice(mock_devices_[2]);
  SetPowerManager();
  OnDarkSuspendImminent();
}

TEST_F(ManagerTest, OnDarkSuspendImminentNoDevicesPresent) {
  EXPECT_CALL(*power_manager_, ReportDarkSuspendReadiness());
  SetPowerManager();
  OnDarkSuspendImminent();
}

TEST_F(ManagerTest, OnSuspendActionsComplete) {
  Error error;
  EXPECT_CALL(*power_manager_, ReportSuspendReadiness());
  SetPowerManager();
  OnSuspendActionsComplete(error);
}

TEST_F(ManagerTest, RecheckPortal) {
  EXPECT_CALL(*mock_devices_[0].get(), RequestPortalDetection())
      .WillOnce(Return(false));
  EXPECT_CALL(*mock_devices_[1].get(), RequestPortalDetection())
      .WillOnce(Return(true));
  EXPECT_CALL(*mock_devices_[2].get(), RequestPortalDetection())
      .Times(0);

  manager()->RegisterDevice(mock_devices_[0]);
  manager()->RegisterDevice(mock_devices_[1]);
  manager()->RegisterDevice(mock_devices_[2]);

  manager()->RecheckPortal(nullptr);
}

TEST_F(ManagerTest, RecheckPortalOnService) {
  MockServiceRefPtr service = new NiceMock<MockService>(control_interface(),
                                                        dispatcher(),
                                                        metrics(),
                                                        manager());
  EXPECT_CALL(*mock_devices_[0].get(),
              IsConnectedToService(IsRefPtrTo(service)))
      .WillOnce(Return(false));
  EXPECT_CALL(*mock_devices_[1].get(),
              IsConnectedToService(IsRefPtrTo(service)))
      .WillOnce(Return(true));
  EXPECT_CALL(*mock_devices_[1].get(), RestartPortalDetection())
      .WillOnce(Return(true));
  EXPECT_CALL(*mock_devices_[2].get(), IsConnectedToService(_))
      .Times(0);

  manager()->RegisterDevice(mock_devices_[0]);
  manager()->RegisterDevice(mock_devices_[1]);
  manager()->RegisterDevice(mock_devices_[2]);

  manager()->RecheckPortalOnService(service);
}

TEST_F(ManagerTest, GetDefaultService) {
  EXPECT_FALSE(manager()->GetDefaultService().get());
  EXPECT_EQ(control_interface()->NullRPCIdentifier(),
            GetDefaultServiceRpcIdentifier());

  scoped_refptr<MockService> mock_service(
      new NiceMock<MockService>(control_interface(),
                                dispatcher(),
                                metrics(),
                                manager()));

  manager()->RegisterService(mock_service);
  EXPECT_FALSE(manager()->GetDefaultService().get());
  EXPECT_EQ(control_interface()->NullRPCIdentifier(),
            GetDefaultServiceRpcIdentifier());

  scoped_refptr<MockConnection> mock_connection(
      new NiceMock<MockConnection>(device_info_.get()));
  mock_service->set_mock_connection(mock_connection);
  EXPECT_EQ(mock_service.get(), manager()->GetDefaultService().get());
  EXPECT_EQ(mock_service->GetRpcIdentifier(), GetDefaultServiceRpcIdentifier());

  mock_service->set_mock_connection(nullptr);
  manager()->DeregisterService(mock_service);
}

TEST_F(ManagerTest, GetServiceWithGUID) {
  scoped_refptr<MockService> mock_service0(
      new NiceMock<MockService>(control_interface(),
                                dispatcher(),
                                metrics(),
                                manager()));

  scoped_refptr<MockService> mock_service1(
      new NiceMock<MockService>(control_interface(),
                                dispatcher(),
                                metrics(),
                                manager()));

  EXPECT_CALL(*mock_service0.get(), Configure(_, _))
      .Times(0);
  EXPECT_CALL(*mock_service1.get(), Configure(_, _))
      .Times(0);

  manager()->RegisterService(mock_service0);
  manager()->RegisterService(mock_service1);

  const string kGUID0 = "GUID0";
  const string kGUID1 = "GUID1";

  {
    Error error;
    ServiceRefPtr service = manager()->GetServiceWithGUID(kGUID0, &error);
    EXPECT_FALSE(error.IsSuccess());
    EXPECT_FALSE(service);
  }

  KeyValueStore args;
  args.SetString(kGuidProperty, kGUID1);

  {
    Error error;
    ServiceRefPtr service = manager()->GetService(args, &error);
    EXPECT_EQ(Error::kInvalidArguments, error.type());
    EXPECT_FALSE(service);
  }

  mock_service0->SetGuid(kGUID0, nullptr);
  mock_service1->SetGuid(kGUID1, nullptr);

  {
    Error error;
    ServiceRefPtr service = manager()->GetServiceWithGUID(kGUID0, &error);
    EXPECT_TRUE(error.IsSuccess());
    EXPECT_EQ(mock_service0.get(), service.get());
  }

  {
    Error error;
    EXPECT_CALL(*mock_service1.get(), Configure(_, &error))
        .Times(1);
    ServiceRefPtr service = manager()->GetService(args, &error);
    EXPECT_TRUE(error.IsSuccess());
    EXPECT_EQ(mock_service1.get(), service.get());
  }

  manager()->DeregisterService(mock_service0);
  manager()->DeregisterService(mock_service1);
}


TEST_F(ManagerTest, CalculateStateOffline) {
  EXPECT_FALSE(manager()->IsConnected());
  EXPECT_EQ("offline", manager()->CalculateState(nullptr));

  MockMetrics mock_metrics(dispatcher());
  SetMetrics(&mock_metrics);
  EXPECT_CALL(mock_metrics, NotifyDefaultServiceChanged(_))
      .Times(AnyNumber());
  scoped_refptr<MockService> mock_service0(
      new NiceMock<MockService>(control_interface(),
                                dispatcher(),
                                metrics(),
                                manager()));

  scoped_refptr<MockService> mock_service1(
      new NiceMock<MockService>(control_interface(),
                                dispatcher(),
                                metrics(),
                                manager()));

  EXPECT_CALL(*mock_service0.get(), IsConnected())
      .WillRepeatedly(Return(false));
  EXPECT_CALL(*mock_service1.get(), IsConnected())
      .WillRepeatedly(Return(false));

  manager()->RegisterService(mock_service0);
  manager()->RegisterService(mock_service1);

  EXPECT_FALSE(manager()->IsConnected());
  EXPECT_EQ("offline", manager()->CalculateState(nullptr));

  manager()->DeregisterService(mock_service0);
  manager()->DeregisterService(mock_service1);
}

TEST_F(ManagerTest, CalculateStateOnline) {
  MockMetrics mock_metrics(dispatcher());
  SetMetrics(&mock_metrics);
  EXPECT_CALL(mock_metrics, NotifyDefaultServiceChanged(_))
      .Times(AnyNumber());
  scoped_refptr<MockService> mock_service0(
      new NiceMock<MockService>(control_interface(),
                                dispatcher(),
                                metrics(),
                                manager()));

  scoped_refptr<MockService> mock_service1(
      new NiceMock<MockService>(control_interface(),
                                dispatcher(),
                                metrics(),
                                manager()));

  EXPECT_CALL(*mock_service0.get(), IsConnected())
      .WillRepeatedly(Return(false));
  EXPECT_CALL(*mock_service1.get(), IsConnected())
      .WillRepeatedly(Return(true));
  EXPECT_CALL(*mock_service0.get(), state())
      .WillRepeatedly(Return(Service::kStateIdle));
  EXPECT_CALL(*mock_service1.get(), state())
      .WillRepeatedly(Return(Service::kStateConnected));

  manager()->RegisterService(mock_service0);
  manager()->RegisterService(mock_service1);
  CompleteServiceSort();

  EXPECT_TRUE(manager()->IsConnected());
  EXPECT_EQ("online", manager()->CalculateState(nullptr));

  manager()->DeregisterService(mock_service0);
  manager()->DeregisterService(mock_service1);
}

TEST_F(ManagerTest, RefreshConnectionState) {
  EXPECT_CALL(*manager_adaptor_,
              EmitStringChanged(kConnectionStateProperty, kStateIdle));
  EXPECT_CALL(*upstart_, NotifyDisconnected());
  EXPECT_CALL(*upstart_, NotifyConnected()).Times(0);
  RefreshConnectionState();
  Mock::VerifyAndClearExpectations(manager_adaptor_);
  Mock::VerifyAndClearExpectations(upstart_);

  scoped_refptr<MockService> mock_service(
      new NiceMock<MockService>(control_interface(),
                                dispatcher(),
                                metrics(),
                                manager()));
  EXPECT_CALL(*manager_adaptor_,
              EmitStringChanged(kConnectionStateProperty, _)).Times(0);
  EXPECT_CALL(*upstart_, NotifyDisconnected()).Times(0);
  EXPECT_CALL(*upstart_, NotifyConnected());
  manager()->RegisterService(mock_service);
  RefreshConnectionState();

  scoped_refptr<MockConnection> mock_connection(
      new NiceMock<MockConnection>(device_info_.get()));
  mock_service->set_mock_connection(mock_connection);
  EXPECT_CALL(*mock_service, state())
      .WillOnce(Return(Service::kStateIdle));
  RefreshConnectionState();

  Mock::VerifyAndClearExpectations(manager_adaptor_);
  EXPECT_CALL(*mock_service, state())
      .WillOnce(Return(Service::kStatePortal));
  EXPECT_CALL(*mock_service, IsConnected())
      .WillOnce(Return(true));
  EXPECT_CALL(*manager_adaptor_,
              EmitStringChanged(kConnectionStateProperty, kStatePortal));
  RefreshConnectionState();
  Mock::VerifyAndClearExpectations(manager_adaptor_);
  Mock::VerifyAndClearExpectations(upstart_);

  mock_service->set_mock_connection(nullptr);
  manager()->DeregisterService(mock_service);

  EXPECT_CALL(*manager_adaptor_,
              EmitStringChanged(kConnectionStateProperty, kStateIdle));
  EXPECT_CALL(*upstart_, NotifyDisconnected());
  EXPECT_CALL(*upstart_, NotifyConnected()).Times(0);
  RefreshConnectionState();
}

TEST_F(ManagerTest, StartupPortalList) {
  // Simulate loading value from the default profile.
  const string kProfileValue("wifi,vpn");
  manager()->props_.check_portal_list = kProfileValue;

  EXPECT_EQ(kProfileValue, manager()->GetCheckPortalList(nullptr));
  EXPECT_TRUE(manager()->IsPortalDetectionEnabled(Technology::kWifi));
  EXPECT_FALSE(manager()->IsPortalDetectionEnabled(Technology::kCellular));

  const string kStartupValue("cellular,ethernet");
  manager()->SetStartupPortalList(kStartupValue);
  // Ensure profile value is not overwritten, so when we save the default
  // profile, the correct value will still be written.
  EXPECT_EQ(kProfileValue, manager()->props_.check_portal_list);

  // However we should read back a different list.
  EXPECT_EQ(kStartupValue, manager()->GetCheckPortalList(nullptr));
  EXPECT_FALSE(manager()->IsPortalDetectionEnabled(Technology::kWifi));
  EXPECT_TRUE(manager()->IsPortalDetectionEnabled(Technology::kCellular));

  const string kRuntimeValue("ppp");
  // Setting a runtime value over the control API should overwrite both
  // the profile value and what we read back.
  Error error;
  manager()->mutable_store()->SetStringProperty(
      kCheckPortalListProperty,
      kRuntimeValue,
      &error);
  ASSERT_TRUE(error.IsSuccess());
  EXPECT_EQ(kRuntimeValue, manager()->GetCheckPortalList(nullptr));
  EXPECT_EQ(kRuntimeValue, manager()->props_.check_portal_list);
  EXPECT_FALSE(manager()->IsPortalDetectionEnabled(Technology::kCellular));
  EXPECT_TRUE(manager()->IsPortalDetectionEnabled(Technology::kPPP));
}

TEST_F(ManagerTest, LinkMonitorEnabled) {
  const string kEnabledTechnologies("wifi,vpn");
  manager()->props_.link_monitor_technologies = kEnabledTechnologies;
  EXPECT_TRUE(manager()->IsTechnologyLinkMonitorEnabled(Technology::kWifi));
  EXPECT_FALSE(
      manager()->IsTechnologyLinkMonitorEnabled(Technology::kCellular));
}

TEST_F(ManagerTest, IsTechnologyAutoConnectDisabled) {
  const string kNoAutoConnectTechnologies("wifi,cellular");
  manager()->props_.no_auto_connect_technologies = kNoAutoConnectTechnologies;
  EXPECT_TRUE(manager()->IsTechnologyAutoConnectDisabled(Technology::kWifi));
  EXPECT_TRUE(
      manager()->IsTechnologyAutoConnectDisabled(Technology::kCellular));
  EXPECT_FALSE(
      manager()->IsTechnologyAutoConnectDisabled(Technology::kEthernet));
}

TEST_F(ManagerTest, SetEnabledStateForTechnologyPersistentCheck) {
  Error error(Error::kOperationInitiated);
  DisableTechnologyReplyHandler disable_technology_reply_handler;
  ResultCallback disable_technology_callback(
      Bind(&DisableTechnologyReplyHandler::ReportResult,
           disable_technology_reply_handler.AsWeakPtr()));
  EXPECT_CALL(disable_technology_reply_handler, ReportResult(_)).Times(0);
  EXPECT_CALL(*mock_devices_[0], SetEnabledPersistent(false, _, _));

  ON_CALL(*mock_devices_[0], technology())
      .WillByDefault(Return(Technology::kEthernet));
  manager()->RegisterDevice(mock_devices_[0]);
  manager()->SetEnabledStateForTechnology(kTypeEthernet, false, true,
                                          &error, disable_technology_callback);

  EXPECT_CALL(*mock_devices_[0], SetEnabledNonPersistent(false, _, _));
  manager()->SetEnabledStateForTechnology(kTypeEthernet, false, false,
                                          &error, disable_technology_callback);
}

TEST_F(ManagerTest, SetEnabledStateForTechnology) {
  Error error(Error::kOperationInitiated);
  DisableTechnologyReplyHandler disable_technology_reply_handler;
  ResultCallback disable_technology_callback(
      Bind(&DisableTechnologyReplyHandler::ReportResult,
           disable_technology_reply_handler.AsWeakPtr()));
  EXPECT_CALL(disable_technology_reply_handler, ReportResult(_)).Times(0);

  manager()->SetEnabledStateForTechnology(kTypeEthernet, false, true,
                                          &error, disable_technology_callback);
  EXPECT_TRUE(error.IsSuccess());

  ON_CALL(*mock_devices_[0], technology())
      .WillByDefault(Return(Technology::kEthernet));
  ON_CALL(*mock_devices_[1], technology())
      .WillByDefault(Return(Technology::kCellular));
  ON_CALL(*mock_devices_[2], technology())
      .WillByDefault(Return(Technology::kCellular));

  manager()->RegisterDevice(mock_devices_[0]);
  manager()->RegisterDevice(mock_devices_[1]);

  // Ethernet Device is disabled, so disable succeeds immediately.
  EXPECT_CALL(*mock_devices_[0], SetEnabledPersistent(false, _, _))
      .WillOnce(WithArg<1>(Invoke(SetErrorSuccess)));
  error.Populate(Error::kOperationInitiated);
  manager()->SetEnabledStateForTechnology(kTypeEthernet, false, true,
                                          &error, disable_technology_callback);
  EXPECT_TRUE(error.IsSuccess());

  // Ethernet Device is enabled, and mock doesn't change error from
  // kOperationInitiated, so expect disable to say operation in progress.
  EXPECT_CALL(*mock_devices_[0], SetEnabledPersistent(false, _, _));
  mock_devices_[0]->enabled_ = true;
  error.Populate(Error::kOperationInitiated);
  manager()->SetEnabledStateForTechnology(kTypeEthernet, false, true,
                                          &error, disable_technology_callback);
  EXPECT_TRUE(error.IsOngoing());

  // Ethernet Device is disabled, and mock doesn't change error from
  // kOperationInitiated, so expect enable to say operation in progress.
  EXPECT_CALL(*mock_devices_[0], SetEnabledPersistent(true, _, _));
  mock_devices_[0]->enabled_ = false;
  error.Populate(Error::kOperationInitiated);
  manager()->SetEnabledStateForTechnology(kTypeEthernet, true, true,
                                          &error, disable_technology_callback);
  EXPECT_TRUE(error.IsOngoing());

  // Cellular Device is enabled, but disable failed.
  EXPECT_CALL(*mock_devices_[1], SetEnabledPersistent(false, _, _))
      .WillOnce(WithArg<1>(Invoke(SetErrorPermissionDenied)));
  mock_devices_[1]->enabled_ = true;
  error.Populate(Error::kOperationInitiated);
  manager()->SetEnabledStateForTechnology(kTypeCellular, false, true,
                                          &error, disable_technology_callback);
  EXPECT_EQ(Error::kPermissionDenied, error.type());

  // Multiple Cellular Devices in enabled state. Should indicate IsOngoing
  // if one is in progress (even if the other completed immediately).
  manager()->RegisterDevice(mock_devices_[2]);
  EXPECT_CALL(*mock_devices_[1], SetEnabledPersistent(false, _, _))
      .WillOnce(WithArg<1>(Invoke(SetErrorPermissionDenied)));
  EXPECT_CALL(*mock_devices_[2], SetEnabledPersistent(false, _, _));
  mock_devices_[1]->enabled_ = true;
  mock_devices_[2]->enabled_ = true;
  error.Populate(Error::kOperationInitiated);
  manager()->SetEnabledStateForTechnology(kTypeCellular, false, true,
                                          &error, disable_technology_callback);
  EXPECT_TRUE(error.IsOngoing());

  // ...and order doesn't matter.
  EXPECT_CALL(*mock_devices_[1], SetEnabledPersistent(false, _, _));
  EXPECT_CALL(*mock_devices_[2], SetEnabledPersistent(false, _, _))
      .WillOnce(WithArg<1>(Invoke(SetErrorPermissionDenied)));
  mock_devices_[1]->enabled_ = true;
  mock_devices_[2]->enabled_ = true;
  error.Populate(Error::kOperationInitiated);
  manager()->SetEnabledStateForTechnology(kTypeCellular, false, true,
                                          &error, disable_technology_callback);
  EXPECT_TRUE(error.IsOngoing());
  Mock::VerifyAndClearExpectations(&disable_technology_reply_handler);

  // Multiple Cellular Devices in enabled state. Even if all disable
  // operations complete asynchronously, we only get one call to the
  // DisableTechnologyReplyHandler::ReportResult.
  ResultCallback device1_result_callback;
  ResultCallback device2_result_callback;
  EXPECT_CALL(*mock_devices_[1], SetEnabledPersistent(false, _, _))
      .WillOnce(SaveArg<2>(&device1_result_callback));
  EXPECT_CALL(*mock_devices_[2], SetEnabledPersistent(false, _, _))
      .WillOnce(DoAll(WithArg<1>(Invoke(SetErrorPermissionDenied)),
                      SaveArg<2>(&device2_result_callback)));
  EXPECT_CALL(disable_technology_reply_handler, ReportResult(_));
  mock_devices_[1]->enabled_ = true;
  mock_devices_[2]->enabled_ = true;
  error.Populate(Error::kOperationInitiated);
  manager()->SetEnabledStateForTechnology(kTypeCellular, false, true,
                                          &error, disable_technology_callback);
  EXPECT_TRUE(error.IsOngoing());
  device1_result_callback.Run(Error(Error::kSuccess));
  device2_result_callback.Run(Error(Error::kSuccess));
}

TEST_F(ManagerTest, IgnoredSearchList) {
  std::unique_ptr<MockResolver> resolver(new StrictMock<MockResolver>());
  vector<string> ignored_paths;
  SetResolver(resolver.get());

  const string kIgnored0 = "chromium.org";
  ignored_paths.push_back(kIgnored0);
  EXPECT_CALL(*resolver.get(), set_ignored_search_list(ignored_paths));
  SetIgnoredDNSSearchPaths(kIgnored0, nullptr);
  EXPECT_EQ(kIgnored0, GetIgnoredDNSSearchPaths());

  const string kIgnored1 = "google.com";
  const string kIgnoredSum = kIgnored0 + "," + kIgnored1;
  ignored_paths.push_back(kIgnored1);
  EXPECT_CALL(*resolver.get(), set_ignored_search_list(ignored_paths));
  SetIgnoredDNSSearchPaths(kIgnoredSum, nullptr);
  EXPECT_EQ(kIgnoredSum, GetIgnoredDNSSearchPaths());

  ignored_paths.clear();
  EXPECT_CALL(*resolver.get(), set_ignored_search_list(ignored_paths));
  SetIgnoredDNSSearchPaths("", nullptr);
  EXPECT_EQ("", GetIgnoredDNSSearchPaths());

  SetResolver(Resolver::GetInstance());
}

TEST_F(ManagerTest, ServiceStateChangeEmitsServices) {
  // Test to make sure that every service state-change causes the
  // Manager to emit a new service list.
  scoped_refptr<MockService> mock_service(
      new NiceMock<MockService>(control_interface(),
                                dispatcher(),
                                metrics(),
                                manager()));
  EXPECT_CALL(*mock_service, state())
      .WillRepeatedly(Return(Service::kStateIdle));

  manager()->RegisterService(mock_service);
  EXPECT_CALL(
      *manager_adaptor_, EmitRpcIdentifierArrayChanged(
          kServiceCompleteListProperty, _)).Times(1);
  EXPECT_CALL(
      *manager_adaptor_, EmitRpcIdentifierArrayChanged(
          kServicesProperty, _)).Times(1);
  EXPECT_CALL(
      *manager_adaptor_, EmitRpcIdentifierArrayChanged(
          kServiceWatchListProperty, _)).Times(1);
  CompleteServiceSort();

  Mock::VerifyAndClearExpectations(manager_adaptor_);
  EXPECT_CALL(
      *manager_adaptor_, EmitRpcIdentifierArrayChanged(
          kServiceCompleteListProperty, _)).Times(1);
  EXPECT_CALL(
      *manager_adaptor_, EmitRpcIdentifierArrayChanged(
          kServicesProperty, _)).Times(1);
  EXPECT_CALL(
      *manager_adaptor_, EmitRpcIdentifierArrayChanged(
          kServiceWatchListProperty, _)).Times(1);
  manager()->UpdateService(mock_service.get());
  CompleteServiceSort();

  manager()->DeregisterService(mock_service);
}

TEST_F(ManagerTest, EnumerateServices) {
  scoped_refptr<MockService> mock_service(
      new NiceMock<MockService>(control_interface(),
                                dispatcher(),
                                metrics(),
                                manager()));
  manager()->RegisterService(mock_service);

  EXPECT_CALL(*mock_service, state())
      .WillRepeatedly(Return(Service::kStateConnected));
  EXPECT_CALL(*mock_service, IsVisible())
      .WillRepeatedly(Return(false));
  EXPECT_TRUE(EnumerateAvailableServices().empty());
  EXPECT_TRUE(EnumerateWatchedServices().empty());

  EXPECT_CALL(*mock_service, state())
      .WillRepeatedly(Return(Service::kStateIdle));
  EXPECT_TRUE(EnumerateAvailableServices().empty());
  EXPECT_TRUE(EnumerateWatchedServices().empty());

  EXPECT_CALL(*mock_service, IsVisible())
      .WillRepeatedly(Return(true));
  Service::ConnectState unwatched_states[] = {
      Service::kStateUnknown,
      Service::kStateIdle,
      Service::kStateFailure
  };
  for (size_t i = 0; i < arraysize(unwatched_states); ++i) {
    EXPECT_CALL(*mock_service, state())
        .WillRepeatedly(Return(unwatched_states[i]));
    EXPECT_FALSE(EnumerateAvailableServices().empty());
    EXPECT_TRUE(EnumerateWatchedServices().empty());
  }

  Service::ConnectState watched_states[] = {
      Service::kStateAssociating,
      Service::kStateConfiguring,
      Service::kStateConnected,
      Service::kStatePortal,
      Service::kStateOnline
  };
  for (size_t i = 0; i < arraysize(watched_states); ++i) {
    EXPECT_CALL(*mock_service, state())
        .WillRepeatedly(Return(watched_states[i]));
    EXPECT_FALSE(EnumerateAvailableServices().empty());
    EXPECT_FALSE(EnumerateWatchedServices().empty());
  }

  manager()->DeregisterService(mock_service);
}

TEST_F(ManagerTest, ConnectToBestServices) {
  scoped_refptr<MockService> wifi_service0(
      new NiceMock<MockService>(control_interface(),
                                dispatcher(),
                                metrics(),
                                manager()));
  EXPECT_CALL(*wifi_service0.get(), state())
      .WillRepeatedly(Return(Service::kStateIdle));
  EXPECT_CALL(*wifi_service0.get(), IsConnected())
      .WillRepeatedly(Return(false));
  wifi_service0->SetConnectable(true);
  wifi_service0->SetAutoConnect(true);
  wifi_service0->SetSecurity(Service::kCryptoAes, true, true);
  EXPECT_CALL(*wifi_service0.get(), technology())
      .WillRepeatedly(Return(Technology::kWifi));
  EXPECT_CALL(*wifi_service0.get(), IsVisible())
      .WillRepeatedly(Return(false));
  EXPECT_CALL(*wifi_service0.get(), explicitly_disconnected())
      .WillRepeatedly(Return(false));

  scoped_refptr<MockService> wifi_service1(
      new NiceMock<MockService>(control_interface(),
                                dispatcher(),
                                metrics(),
                                manager()));
  EXPECT_CALL(*wifi_service1.get(), state())
      .WillRepeatedly(Return(Service::kStateIdle));
  EXPECT_CALL(*wifi_service1.get(), IsVisible())
      .WillRepeatedly(Return(true));
  EXPECT_CALL(*wifi_service1.get(), IsConnected())
      .WillRepeatedly(Return(false));
  wifi_service1->SetAutoConnect(true);
  wifi_service1->SetConnectable(true);
  wifi_service1->SetSecurity(Service::kCryptoRc4, true, true);
  EXPECT_CALL(*wifi_service1.get(), technology())
      .WillRepeatedly(Return(Technology::kWifi));
  EXPECT_CALL(*wifi_service1.get(), explicitly_disconnected())
      .WillRepeatedly(Return(false));

  scoped_refptr<MockService> wifi_service2(
      new NiceMock<MockService>(control_interface(),
                                dispatcher(),
                                metrics(),
                                manager()));
  EXPECT_CALL(*wifi_service2.get(), state())
      .WillRepeatedly(Return(Service::kStateConnected));
  EXPECT_CALL(*wifi_service2.get(), IsConnected())
      .WillRepeatedly(Return(true));
  EXPECT_CALL(*wifi_service2.get(), IsVisible())
      .WillRepeatedly(Return(true));
  wifi_service2->SetAutoConnect(true);
  wifi_service2->SetConnectable(true);
  wifi_service2->SetSecurity(Service::kCryptoNone, false, false);
  EXPECT_CALL(*wifi_service2.get(), technology())
      .WillRepeatedly(Return(Technology::kWifi));
  EXPECT_CALL(*wifi_service2.get(), explicitly_disconnected())
      .WillRepeatedly(Return(false));

  manager()->RegisterService(wifi_service0);
  manager()->RegisterService(wifi_service1);
  manager()->RegisterService(wifi_service2);

  CompleteServiceSort();
  EXPECT_TRUE(ServiceOrderIs(wifi_service2, wifi_service0));

  scoped_refptr<MockService> cell_service(
      new NiceMock<MockService>(control_interface(),
                                dispatcher(),
                                metrics(),
                                manager()));

  EXPECT_CALL(*cell_service.get(), state())
      .WillRepeatedly(Return(Service::kStateIdle));
  EXPECT_CALL(*cell_service.get(), IsConnected())
      .WillRepeatedly(Return(false));
  EXPECT_CALL(*cell_service.get(), IsVisible())
      .WillRepeatedly(Return(true));
  cell_service->SetAutoConnect(true);
  cell_service->SetConnectable(true);
  EXPECT_CALL(*cell_service.get(), technology())
      .WillRepeatedly(Return(Technology::kCellular));
  EXPECT_CALL(*cell_service.get(), explicitly_disconnected())
      .WillRepeatedly(Return(true));
  manager()->RegisterService(cell_service);

  scoped_refptr<MockService> wimax_service(
      new NiceMock<MockService>(control_interface(),
                                dispatcher(),
                                metrics(),
                                manager()));

  EXPECT_CALL(*wimax_service.get(), state())
      .WillRepeatedly(Return(Service::kStateConnected));
  EXPECT_CALL(*wimax_service.get(), IsConnected())
      .WillRepeatedly(Return(true));
  EXPECT_CALL(*wimax_service.get(), IsVisible())
      .WillRepeatedly(Return(true));
  wimax_service->SetAutoConnect(true);
  wimax_service->SetConnectable(true);
  EXPECT_CALL(*wimax_service.get(), technology())
      .WillRepeatedly(Return(Technology::kWiMax));
  EXPECT_CALL(*wimax_service.get(), explicitly_disconnected())
      .WillRepeatedly(Return(false));
  manager()->RegisterService(wimax_service);

  scoped_refptr<MockService> vpn_service(
      new NiceMock<MockService>(control_interface(),
                                dispatcher(),
                                metrics(),
                                manager()));

  EXPECT_CALL(*vpn_service.get(), state())
      .WillRepeatedly(Return(Service::kStateIdle));
  EXPECT_CALL(*vpn_service.get(), IsConnected())
      .WillRepeatedly(Return(false));
  EXPECT_CALL(*vpn_service.get(), IsVisible())
      .WillRepeatedly(Return(true));
  wifi_service2->SetAutoConnect(false);
  vpn_service->SetConnectable(true);
  EXPECT_CALL(*vpn_service.get(), technology())
      .WillRepeatedly(Return(Technology::kVPN));
  manager()->RegisterService(vpn_service);

  // The connected services should be at the top.
  EXPECT_TRUE(ServiceOrderIs(wifi_service2, wimax_service));

  EXPECT_CALL(*wifi_service0.get(), Connect(_, _)).Times(0);  // Not visible.
  EXPECT_CALL(*wifi_service1.get(), Connect(_, _));
  EXPECT_CALL(*wifi_service2.get(), Connect(_, _)).Times(0);  // Lower prio.
  EXPECT_CALL(*cell_service.get(), Connect(_, _))
      .Times(0);  // Explicitly disconnected.
  EXPECT_CALL(*wimax_service.get(), Connect(_, _)).Times(0);  // Is connected.
  EXPECT_CALL(*vpn_service.get(), Connect(_, _)).Times(0);  // Not autoconnect.

  manager()->ConnectToBestServices(nullptr);
  dispatcher()->DispatchPendingEvents();

  // After this operation, since the Connect calls above are mocked and
  // no actual state changes have occurred, we should expect that the
  // service sorting order will not have changed.
  EXPECT_TRUE(ServiceOrderIs(wifi_service2, wimax_service));
}

TEST_F(ManagerTest, CreateConnectivityReport) {
  // Add devices
  // WiFi
  auto wifi_device = make_scoped_refptr(
       new NiceMock<MockDevice>(control_interface(),
                                dispatcher(),
                                metrics(),
                                manager(),
                                "null",
                                "addr",
                                0));
  manager()->RegisterDevice(wifi_device);
  // Cell
  auto cell_device = make_scoped_refptr(
      new NiceMock<MockDevice>(control_interface(),
                               dispatcher(),
                               metrics(),
                               manager(),
                               "null",
                               "addr",
                               1));
  manager()->RegisterDevice(cell_device);
  // WiMax
  auto wimax_device = make_scoped_refptr(
      new NiceMock<MockDevice>(control_interface(),
                               dispatcher(),
                               metrics(),
                               manager(),
                               "null",
                               "addr",
                               2));
  manager()->RegisterDevice(wimax_device);
  // Ethernet
  auto eth_device = make_scoped_refptr(
      new NiceMock<MockDevice>(control_interface(),
                               dispatcher(),
                               metrics(),
                               manager(),
                               "null",
                               "addr",
                               3));
  manager()->RegisterDevice(eth_device);
  // VPN Device -- base device for a service that will not be connected
  auto vpn_device = make_scoped_refptr(
      new NiceMock<MockDevice>(control_interface(),
                               dispatcher(),
                               metrics(),
                               manager(),
                               "null",
                               "addr",
                               4));
  manager()->RegisterDevice(vpn_device);

  // Add service for multiple devices
  // WiFi
  MockServiceRefPtr wifi_service =
      new NiceMock<MockService>(control_interface(),
                                dispatcher(),
                                metrics(),
                                manager());
  manager()->RegisterService(wifi_service);
  EXPECT_CALL(*wifi_service.get(), state())
      .WillRepeatedly(Return(Service::kStateConnected));
  EXPECT_CALL(*wifi_service.get(), IsConnected())
      .WillRepeatedly(Return(true));
  EXPECT_CALL(*wifi_device.get(),
              IsConnectedToService(_)).WillRepeatedly(Return(false));
  EXPECT_CALL(*wifi_device.get(),
              IsConnectedToService(IsRefPtrTo(wifi_service)))
      .WillRepeatedly(Return(true));

  // Cell
  MockServiceRefPtr cell_service =
      new NiceMock<MockService>(control_interface(),
                                dispatcher(),
                                metrics(),
                                manager());
  manager()->RegisterService(cell_service);
  EXPECT_CALL(*cell_service.get(), state())
      .WillRepeatedly(Return(Service::kStateConnected));
  EXPECT_CALL(*cell_service.get(), IsConnected())
      .WillRepeatedly(Return(true));
  EXPECT_CALL(*cell_device.get(),
              IsConnectedToService(_)).WillRepeatedly(Return(false));
  EXPECT_CALL(*cell_device.get(),
              IsConnectedToService(IsRefPtrTo(cell_service)))
      .WillRepeatedly(Return(true));

  // WiMax
  MockServiceRefPtr wimax_service =
      new NiceMock<MockService>(control_interface(),
                                dispatcher(),
                                metrics(),
                                manager());
  manager()->RegisterService(wimax_service);
  EXPECT_CALL(*wimax_service.get(), state())
      .WillRepeatedly(Return(Service::kStateConnected));
  EXPECT_CALL(*wimax_service.get(), IsConnected())
      .WillRepeatedly(Return(true));

  EXPECT_CALL(*wimax_device.get(),
              IsConnectedToService(_)).WillRepeatedly(Return(false));
  EXPECT_CALL(*wimax_device.get(),
              IsConnectedToService(IsRefPtrTo(wimax_service)))
      .WillRepeatedly(Return(true));

  // Ethernet
  MockServiceRefPtr eth_service =
      new NiceMock<MockService>(control_interface(),
                                dispatcher(),
                                metrics(),
                                manager());
  manager()->RegisterService(eth_service);
  EXPECT_CALL(*eth_service.get(), state())
      .WillRepeatedly(Return(Service::kStateConnected));
  EXPECT_CALL(*eth_service.get(), IsConnected())
      .WillRepeatedly(Return(true));
  EXPECT_CALL(*eth_device.get(),
              IsConnectedToService(_)).WillRepeatedly(Return(false));
  EXPECT_CALL(*eth_device.get(),
              IsConnectedToService(IsRefPtrTo(eth_service)))
      .WillRepeatedly(Return(true));

  // VPN: Service exists but is not connected and will not trigger a
  // connectivity report.
  MockServiceRefPtr vpn_service =
      new NiceMock<MockService>(control_interface(),
                                dispatcher(),
                                metrics(),
                                manager());
  manager()->RegisterService(vpn_service);
  EXPECT_CALL(*vpn_service.get(), state())
      .WillRepeatedly(Return(Service::kStateIdle));
  EXPECT_CALL(*vpn_service.get(), IsConnected())
      .WillRepeatedly(Return(false));

  EXPECT_CALL(*wifi_device.get(), StartConnectivityTest())
      .WillOnce(Return(true));
  EXPECT_CALL(*cell_device.get(), StartConnectivityTest())
      .WillOnce(Return(true));
  EXPECT_CALL(*wimax_device.get(), StartConnectivityTest())
      .WillOnce(Return(true));
  EXPECT_CALL(*eth_device.get(), StartConnectivityTest())
      .WillOnce(Return(true));
  EXPECT_CALL(*vpn_device.get(), StartConnectivityTest()).Times(0);
  manager()->CreateConnectivityReport(nullptr);
  dispatcher()->DispatchPendingEvents();
}

#if !defined(DISABLE_WIFI)
TEST_F(ManagerTest, VerifyWhenNotConnected) {
  const string kFakeCertificate("fake cert");
  const string kFakePublicKey("fake public key");
  const string kFakeNonce("fake public key");
  const string kFakeSignedData("fake signed data");
  const string kFakeUdn("fake udn");
  const vector<uint8_t> kSSID(10, 87);
  const string kConfiguredSSID("AConfiguredDestination");
  const vector<uint8_t> kConfiguredSSIDVector(kConfiguredSSID.begin(),
                                              kConfiguredSSID.end());
  const string kConfiguredBSSID("aa:bb:aa:bb:aa:bb");
  scoped_refptr<MockWiFiService> mock_destination(
      new NiceMock<MockWiFiService>(control_interface(), dispatcher(),
                                    metrics(), manager(), wifi_provider_,
                                    kSSID, "", "none", false));
  // Register this service, but don't mark it as connected.
  manager()->RegisterService(mock_destination);
  // Verify that if we're not connected to anything, verification fails.
  {
    LOG(INFO) << "Can't verify if not connected.";
    EXPECT_CALL(*crypto_util_proxy_,
                VerifyDestination(_, _, _, _, _, _, _, _, _)).Times(0);
    Error error(Error::kOperationInitiated);
    manager()->VerifyDestination(kFakeCertificate, kFakePublicKey, kFakeNonce,
                                 kFakeSignedData, kFakeUdn, "", "",
                                 ResultBoolCallback(), &error);
    EXPECT_TRUE(error.IsFailure());
    Mock::VerifyAndClearExpectations(crypto_util_proxy_);
  }
  {
    // However, if the destination is already configured, we might be
    // connected to it via something other than WiFi, and we shouldn't
    // enforce the WiFi check.
    EXPECT_CALL(*crypto_util_proxy_,
                VerifyDestination(kFakeCertificate, kFakePublicKey, kFakeNonce,
                                  kFakeSignedData, kFakeUdn,
                                  kConfiguredSSIDVector, kConfiguredBSSID,
                                  _, _)).Times(1).WillOnce(Return(true));
    Error error(Error::kOperationInitiated);
    manager()->VerifyDestination(kFakeCertificate, kFakePublicKey, kFakeNonce,
                                 kFakeSignedData, kFakeUdn, kConfiguredSSID,
                                 kConfiguredBSSID, ResultBoolCallback(),
                                 &error);
    EXPECT_FALSE(error.IsFailure());
    Mock::VerifyAndClearExpectations(crypto_util_proxy_);
  }
}

TEST_F(ManagerTest, VerifyDestination) {
  const string kFakeCertificate("fake cert");
  const string kFakePublicKey("fake public key");
  const string kFakeNonce("fake public key");
  const string kFakeSignedData("fake signed data");
  const string kFakeUdn("fake udn");
  const char kSSIDStr[] = "fake ssid";
  const vector<uint8_t> kSSID(kSSIDStr, kSSIDStr + arraysize(kSSIDStr));
  const string kConfiguredSSID("AConfiguredDestination");
  const vector<uint8_t> kConfiguredSSIDVector(kConfiguredSSID.begin(),
                                              kConfiguredSSID.end());
  const string kConfiguredBSSID("aa:bb:aa:bb:aa:bb");
  const string kFakeData("muffin man");
  scoped_refptr<MockWiFiService> mock_destination(
      new NiceMock<MockWiFiService>(control_interface(),
                                    dispatcher(),
                                    metrics(),
                                    manager(),
                                    wifi_provider_,
                                    kSSID,
                                    "",
                                    "none",
                                    false));
  manager()->RegisterService(mock_destination);
  // Making the service look online will let service lookup in
  // VerifyDestinatoin succeed.
  EXPECT_CALL(*mock_destination.get(), IsConnected())
      .WillRepeatedly(Return(true));
  StrictMock<DestinationVerificationTest> dv_test;

  // Lead off by verifying that the basic VerifyDestination flow works.
  {
    LOG(INFO) << "Basic VerifyDestination flow.";
    ResultBoolCallback passed_down_callback;
    EXPECT_CALL(*crypto_util_proxy_, VerifyDestination(kFakeCertificate,
                                                       kFakePublicKey,
                                                       kFakeNonce,
                                                       kFakeSignedData,
                                                       kFakeUdn,
                                                       kSSID,
                                                       _,
                                                       _,
                                                       _))
        .Times(1)
        .WillOnce(DoAll(SaveArg<7>(&passed_down_callback), Return(true)));
    // Ask the manager to verify the current destination.  This should look
    // up our previously registered service, and pass some metadata about
    // that service down to the CryptoUtilProxy to verify.
    Error error(Error::kOperationInitiated);
    ResultBoolCallback cb = Bind(
        &DestinationVerificationTest::ResultBoolCallbackStub,
        dv_test.AsWeakPtr());
    manager()->VerifyDestination(kFakeCertificate,
                                 kFakePublicKey,
                                 kFakeNonce,
                                 kFakeSignedData,
                                 kFakeUdn,
                                 // Ask to be verified against that service.
                                 "", "",
                                 cb,
                                 &error);
    // We assert here, because if the operation is not ongoing, it is
    // inconsistent with shim behavior to call the callback anyway.
    ASSERT_TRUE(error.IsOngoing());
    Mock::VerifyAndClearExpectations(crypto_util_proxy_);
    EXPECT_CALL(dv_test, ResultBoolCallbackStub(_, true)).Times(1);
    // Call the callback passed into the CryptoUtilProxy, which
    // should find its way into the callback passed into the manager.
    // In real code, that callback passed into the manager is from the
    // DBus adaptor.
    Error e;
    passed_down_callback.Run(e, true);
    Mock::VerifyAndClearExpectations(&dv_test);
  }

  // Now for a slightly more complex variant.  When we encrypt data,
  // we do the same verification step but monkey with the callback to
  // link ourselves to an encrypt step afterward.
  {
    LOG(INFO) << "Basic VerifyAndEncryptData";
    ResultBoolCallback passed_down_callback;
    EXPECT_CALL(*crypto_util_proxy_, VerifyDestination(kFakeCertificate,
                                                       kFakePublicKey,
                                                       kFakeNonce,
                                                       kFakeSignedData,
                                                       kFakeUdn,
                                                       kSSID,
                                                       _,
                                                       _,
                                                       _))
        .WillOnce(DoAll(SaveArg<7>(&passed_down_callback), Return(true)));

    Error error(Error::kOperationInitiated);
    ResultStringCallback cb = Bind(
        &DestinationVerificationTest::ResultStringCallbackStub,
        dv_test.AsWeakPtr());
    manager()->VerifyAndEncryptData(kFakeCertificate,
                                    kFakePublicKey,
                                    kFakeNonce,
                                    kFakeSignedData,
                                    kFakeUdn,
                                    "", "",
                                    kFakeData,
                                    cb,
                                    &error);
    ASSERT_TRUE(error.IsOngoing());
    Mock::VerifyAndClearExpectations(crypto_util_proxy_);
    // Now, if we call that passed down callback, we should see encrypt being
    // called.
    ResultStringCallback second_passed_down_callback;
    EXPECT_CALL(*crypto_util_proxy_, EncryptData(kFakePublicKey,
                                                 kFakeData,
                                                 _,
                                                 _))
        .Times(1)
        .WillOnce(DoAll(SaveArg<2>(&second_passed_down_callback),
                        Return(true)));
    Error e;
    passed_down_callback.Run(e, true);
    Mock::VerifyAndClearExpectations(crypto_util_proxy_);
    EXPECT_CALL(dv_test, ResultStringCallbackStub(_, _)).Times(1);
    // And if we call the second passed down callback, we should see the
    // original function we passed down to VerifyDestination getting called.
    e.Reset();
    second_passed_down_callback.Run(e, "");
    Mock::VerifyAndClearExpectations(&dv_test);
  }

  // If verification fails on the way to trying to encrypt, we should ditch
  // without calling encrypt at all.
  {
    LOG(INFO) << "Failed VerifyAndEncryptData";
    ResultBoolCallback passed_down_callback;
    EXPECT_CALL(*crypto_util_proxy_, VerifyDestination(kFakeCertificate,
                                                       kFakePublicKey,
                                                       kFakeNonce,
                                                       kFakeSignedData,
                                                       kFakeUdn,
                                                       kSSID,
                                                       _,
                                                       _,
                                                       _))
        .WillOnce(DoAll(SaveArg<7>(&passed_down_callback), Return(true)));

    Error error(Error::kOperationInitiated);
    ResultStringCallback cb = Bind(
        &DestinationVerificationTest::ResultStringCallbackStub,
        dv_test.AsWeakPtr());
    manager()->VerifyAndEncryptData(kFakeCertificate,
                                    kFakePublicKey,
                                    kFakeNonce,
                                    kFakeSignedData,
                                    kFakeUdn,
                                    "", "",
                                    kFakeData,
                                    cb,
                                    &error);
    ASSERT_TRUE(error.IsOngoing());
    Mock::VerifyAndClearExpectations(crypto_util_proxy_);
    Error e(Error::kOperationFailed);
    EXPECT_CALL(*crypto_util_proxy_, EncryptData(_, _, _, _)).Times(0);
    // Although we're ditching, this callback is what cleans up the pending
    // DBus call.
    EXPECT_CALL(dv_test, ResultStringCallbackStub(_, string(""))).Times(1);
    passed_down_callback.Run(e, false);
    Mock::VerifyAndClearExpectations(&dv_test);
  }
}
#endif  // DISABLE_WIFI

TEST_F(ManagerTest, IsProfileBefore) {
  scoped_refptr<MockProfile> profile0(
      new NiceMock<MockProfile>(
          control_interface(), metrics(), manager(), ""));
  scoped_refptr<MockProfile> profile1(
      new NiceMock<MockProfile>(
          control_interface(), metrics(), manager(), ""));

  AdoptProfile(manager(), profile0);
  AdoptProfile(manager(), profile1);  // profile1 is after profile0.
  EXPECT_TRUE(manager()->IsProfileBefore(profile0, profile1));
  EXPECT_FALSE(manager()->IsProfileBefore(profile1, profile0));

  // A few abnormal cases, but it's good to track their behavior.
  scoped_refptr<MockProfile> profile2(
      new NiceMock<MockProfile>(
          control_interface(), metrics(), manager(), ""));
  EXPECT_TRUE(manager()->IsProfileBefore(profile0, profile2));
  EXPECT_TRUE(manager()->IsProfileBefore(profile1, profile2));
  EXPECT_FALSE(manager()->IsProfileBefore(profile2, profile0));
  EXPECT_FALSE(manager()->IsProfileBefore(profile2, profile1));
}

TEST_F(ManagerTest, GetLoadableProfileEntriesForService) {
  MockStore storage0;
  MockStore storage1;
  MockStore storage2;

  scoped_refptr<MockProfile> profile0(
      new NiceMock<MockProfile>(
          control_interface(), metrics(), manager(), ""));
  scoped_refptr<MockProfile> profile1(
      new NiceMock<MockProfile>(
          control_interface(), metrics(), manager(), ""));
  scoped_refptr<MockProfile> profile2(
      new NiceMock<MockProfile>(
          control_interface(), metrics(), manager(), ""));

  AdoptProfile(manager(), profile0);
  AdoptProfile(manager(), profile1);
  AdoptProfile(manager(), profile2);

  scoped_refptr<MockService> service(
      new NiceMock<MockService>(control_interface(),
                                dispatcher(),
                                metrics(),
                                manager()));

  EXPECT_CALL(*profile0, GetConstStorage()).WillOnce(Return(&storage0));
  EXPECT_CALL(*profile1, GetConstStorage()).WillOnce(Return(&storage1));
  EXPECT_CALL(*profile2, GetConstStorage()).WillOnce(Return(&storage2));

  const string kEntry0("aluminum_crutch");
  const string kEntry2("rehashed_faces");

  EXPECT_CALL(*service, GetLoadableStorageIdentifier(Ref(storage0)))
      .WillOnce(Return(kEntry0));
  EXPECT_CALL(*service, GetLoadableStorageIdentifier(Ref(storage1)))
      .WillOnce(Return(""));
  EXPECT_CALL(*service, GetLoadableStorageIdentifier(Ref(storage2)))
      .WillOnce(Return(kEntry2));

  const string kProfileRpc0("service_station");
  const string kProfileRpc2("crystal_tiaras");

  EXPECT_CALL(*profile0, GetRpcIdentifier()).WillOnce(Return(kProfileRpc0));
  EXPECT_CALL(*profile1, GetRpcIdentifier()).Times(0);
  EXPECT_CALL(*profile2, GetRpcIdentifier()).WillOnce(Return(kProfileRpc2));

  map<string, string> entries =
      manager()->GetLoadableProfileEntriesForService(service);
  EXPECT_EQ(2, entries.size());
  EXPECT_TRUE(ContainsKey(entries, kProfileRpc0));
  EXPECT_TRUE(ContainsKey(entries, kProfileRpc2));
  EXPECT_EQ(kEntry0, entries[kProfileRpc0]);
  EXPECT_EQ(kEntry2, entries[kProfileRpc2]);
}

#if !defined(DISABLE_WIFI)
TEST_F(ManagerTest, InitializeProfilesInformsProviders) {
  ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());
  Manager manager(control_interface(),
                  dispatcher(),
                  metrics(),
                  run_path(),
                  storage_path(),
                  temp_dir.path().value());
  // Can't use |wifi_provider_|, because it's owned by the Manager
  // object in the fixture.
  MockWiFiProvider* wifi_provider = new NiceMock<MockWiFiProvider>();
  manager.wifi_provider_.reset(wifi_provider);  // pass ownership
  manager.UpdateProviderMapping();
  // Give manager a valid place to write the user profile list.
  manager.user_profile_list_path_ = temp_dir.path().Append("user_profile_list");

  // With no user profiles, the WiFiProvider should be called once
  // (for the default profile).
  EXPECT_CALL(*wifi_provider, CreateServicesFromProfile(_));
  manager.InitializeProfiles();
  Mock::VerifyAndClearExpectations(wifi_provider);

  // With |n| user profiles, the WiFiProvider should be called |n+1|
  // times. First, create 2 user profiles...
  const char kProfile0[] = "~user/profile0";
  const char kProfile1[] = "~user/profile1";
  string profile_rpc_path;
  Error error;
  ASSERT_TRUE(base::CreateDirectory(temp_dir.path().Append("user")));
  manager.CreateProfile(kProfile0, &profile_rpc_path, &error);
  manager.PushProfile(kProfile0, &profile_rpc_path, &error);
  manager.CreateProfile(kProfile1, &profile_rpc_path, &error);
  manager.PushProfile(kProfile1, &profile_rpc_path, &error);

  // ... then reset manager state ...
  manager.profiles_.clear();

  // ...then check that the WiFiProvider is notified about all three
  // profiles (one default, two user).
  EXPECT_CALL(*wifi_provider, CreateServicesFromProfile(_)).Times(3);
  manager.InitializeProfiles();
  Mock::VerifyAndClearExpectations(wifi_provider);
}
#endif  // DISABLE_WIFI

TEST_F(ManagerTest, InitializeProfilesHandlesDefaults) {
  ScopedTempDir temp_dir;
  std::unique_ptr<Manager> manager;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());

  // Instantiate a Manager with empty persistent storage. Check that
  // defaults are set.
  //
  // Note that we use the same directory for default and user profiles.
  // This doesn't affect the test results, because we don't push a
  // user profile.
  manager.reset(new Manager(control_interface(),
                            dispatcher(),
                            metrics(),
                            run_path(),
                            temp_dir.path().value(),
                            temp_dir.path().value()));
  manager->InitializeProfiles();
  EXPECT_EQ(PortalDetector::kDefaultCheckPortalList,
            manager->props_.check_portal_list);
  EXPECT_EQ(Resolver::kDefaultIgnoredSearchList,
            manager->props_.ignored_dns_search_paths);
  EXPECT_EQ(LinkMonitor::kDefaultLinkMonitorTechnologies,
            manager->props_.link_monitor_technologies);
  EXPECT_EQ(ConnectivityTrial::kDefaultURL,
            manager->props_.portal_url);
  EXPECT_EQ(PortalDetector::kDefaultCheckIntervalSeconds,
            manager->props_.portal_check_interval_seconds);

  // Change one of the settings.
  static const string kCustomCheckPortalList = "fiber0";
  Error error;
  manager->SetCheckPortalList(kCustomCheckPortalList, &error);
  manager->profiles_[0]->Save();

  // Instantiate a new manager. It should have our settings for
  // check_portal_list, rather than the default.
  manager.reset(new Manager(control_interface(),
                            dispatcher(),
                            metrics(),
                            run_path(),
                            temp_dir.path().value(),
                            temp_dir.path().value()));
  manager->InitializeProfiles();
  EXPECT_EQ(kCustomCheckPortalList, manager->props_.check_portal_list);

  // If we clear the persistent storage, we again get the default value.
  ASSERT_TRUE(temp_dir.Delete());
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());
  manager.reset(new Manager(control_interface(),
                            dispatcher(),
                            metrics(),
                            run_path(),
                            temp_dir.path().value(),
                            temp_dir.path().value()));
  manager->InitializeProfiles();
  EXPECT_EQ(PortalDetector::kDefaultCheckPortalList,
            manager->props_.check_portal_list);
}

TEST_F(ManagerTest, ProfileStackChangeLogging) {
  ScopedTempDir temp_dir;
  std::unique_ptr<Manager> manager;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());
  manager.reset(new Manager(control_interface(),
                            dispatcher(),
                            metrics(),
                            run_path(),
                            temp_dir.path().value(),
                            temp_dir.path().value()));

  ScopedMockLog log;
  EXPECT_CALL(log, Log(_, _, _)).Times(AnyNumber());
  EXPECT_CALL(log, Log(logging::LOG_INFO, _, HasSubstr("1 profile(s)")));
  manager->InitializeProfiles();

  const char kProfile0[] = "~user/profile0";
  const char kProfile1[] = "~user/profile1";
  const char kProfile2[] = "~user/profile2";
  ASSERT_TRUE(base::CreateDirectory(temp_dir.path().Append("user")));
  TestCreateProfile(manager.get(), kProfile0);
  TestCreateProfile(manager.get(), kProfile1);
  TestCreateProfile(manager.get(), kProfile2);

  EXPECT_CALL(log, Log(logging::LOG_INFO, _, HasSubstr("2 profile(s)")));
  TestPushProfile(manager.get(), kProfile0);

  EXPECT_CALL(log, Log(logging::LOG_INFO, _, HasSubstr("3 profile(s)")));
  TestInsertUserProfile(manager.get(), kProfile1, "not-so-random-string");

  EXPECT_CALL(log, Log(logging::LOG_INFO, _, HasSubstr("4 profile(s)")));
  TestInsertUserProfile(manager.get(), kProfile2, "very-random-string");

  EXPECT_CALL(log, Log(logging::LOG_INFO, _, HasSubstr("3 profile(s)")));
  TestPopProfile(manager.get(), kProfile2);

  EXPECT_CALL(log, Log(logging::LOG_INFO, _, HasSubstr("2 profile(s)")));
  TestPopAnyProfile(manager.get());

  EXPECT_CALL(log, Log(logging::LOG_INFO, _, HasSubstr("1 profile(s)")));
  TestPopAllUserProfiles(manager.get());
}

// Custom property setters should return false, and make no changes, if
// the new value is the same as the old value.
TEST_F(ManagerTest, CustomSetterNoopChange) {
  // SetCheckPortalList
  {
    static const string kCheckPortalList = "weird-device,weirder-device";
    Error error;
    // Set to known value.
    EXPECT_TRUE(SetCheckPortalList(kCheckPortalList, &error));
    EXPECT_TRUE(error.IsSuccess());
    // Set to same value.
    EXPECT_FALSE(SetCheckPortalList(kCheckPortalList, &error));
    EXPECT_TRUE(error.IsSuccess());
  }

  // SetIgnoredDNSSearchPaths
  {
    NiceMock<MockResolver> resolver;
    static const string kIgnoredPaths = "example.com,example.org";
    Error error;
    SetResolver(&resolver);
    // Set to known value.
    EXPECT_CALL(resolver, set_ignored_search_list(_));
    EXPECT_TRUE(SetIgnoredDNSSearchPaths(kIgnoredPaths, &error));
    EXPECT_TRUE(error.IsSuccess());
    Mock::VerifyAndClearExpectations(&resolver);
    // Set to same value.
    EXPECT_CALL(resolver, set_ignored_search_list(_)).Times(0);
    EXPECT_FALSE(SetIgnoredDNSSearchPaths(kIgnoredPaths, &error));
    EXPECT_TRUE(error.IsSuccess());
    Mock::VerifyAndClearExpectations(&resolver);
  }
}

TEST_F(ManagerTest, GeoLocation) {
  EXPECT_TRUE(manager()->GetNetworksForGeolocation().empty());

  auto device = make_scoped_refptr(new NiceMock<MockDevice>(control_interface(),
                                                            dispatcher(),
                                                            metrics(),
                                                            manager(),
                                                            "null",
                                                            "addr",
                                                            0));

  // Manager should ignore gelocation info from technologies it does not know.
  EXPECT_CALL(*device, technology())
      .Times(AtLeast(1))
      .WillRepeatedly(Return(Technology::kEthernet));
  EXPECT_CALL(*device, GetGeolocationObjects()).Times(0);
  manager()->OnDeviceGeolocationInfoUpdated(device);
  Mock::VerifyAndClearExpectations(device.get());
  EXPECT_TRUE(manager()->GetNetworksForGeolocation().empty());

  // Manager should add WiFi geolocation info.
  EXPECT_CALL(*device, technology())
      .Times(AtLeast(1))
      .WillRepeatedly(Return(Technology::kWifi));
  EXPECT_CALL(*device, GetGeolocationObjects())
      .WillOnce(Return(vector<GeolocationInfo>()));
  manager()->OnDeviceGeolocationInfoUpdated(device);
  Mock::VerifyAndClearExpectations(device.get());
  auto location_infos = manager()->GetNetworksForGeolocation();
  EXPECT_EQ(1, location_infos.size());
  EXPECT_TRUE(ContainsKey(location_infos, kGeoWifiAccessPointsProperty));

  // Manager should inclusively add cellular info.
  EXPECT_CALL(*device, technology())
      .Times(AtLeast(1))
      .WillRepeatedly(Return(Technology::kCellular));
  EXPECT_CALL(*device, GetGeolocationObjects())
      .WillOnce(Return(vector<GeolocationInfo>()));
  manager()->OnDeviceGeolocationInfoUpdated(device);
  location_infos = manager()->GetNetworksForGeolocation();
  EXPECT_EQ(2, location_infos.size());
  EXPECT_TRUE(ContainsKey(location_infos, kGeoWifiAccessPointsProperty));
  EXPECT_TRUE(ContainsKey(location_infos, kGeoCellTowersProperty));
}

TEST_F(ManagerTest, IsWifiIdle) {
  // No registered service.
  EXPECT_FALSE(manager()->IsWifiIdle());

  scoped_refptr<MockService> wifi_service(new MockService(control_interface(),
                                                          dispatcher(),
                                                          metrics(),
                                                          manager()));

  scoped_refptr<MockService> cell_service(new MockService(control_interface(),
                                                          dispatcher(),
                                                          metrics(),
                                                          manager()));

  manager()->RegisterService(wifi_service);
  manager()->RegisterService(cell_service);

  EXPECT_CALL(*wifi_service.get(), technology())
      .WillRepeatedly(Return(Technology::kWifi));
  EXPECT_CALL(*cell_service.get(), technology())
      .WillRepeatedly(Return(Technology::kCellular));

  // Cellular is connected.
  EXPECT_CALL(*cell_service.get(), IsConnected())
      .WillRepeatedly(Return(true));
  manager()->UpdateService(cell_service);

  // No wifi connection attempt.
  EXPECT_CALL(*wifi_service.get(), IsConnecting())
      .WillRepeatedly(Return(false));
  EXPECT_CALL(*wifi_service.get(), IsConnected())
      .WillRepeatedly(Return(false));
  manager()->UpdateService(wifi_service);
  EXPECT_TRUE(manager()->IsWifiIdle());

  // Attempt wifi connection.
  Mock::VerifyAndClearExpectations(wifi_service.get());
  EXPECT_CALL(*wifi_service.get(), technology())
      .WillRepeatedly(Return(Technology::kWifi));
  EXPECT_CALL(*wifi_service.get(), IsConnecting())
      .WillRepeatedly(Return(true));
  EXPECT_CALL(*wifi_service.get(), IsConnected())
      .WillRepeatedly(Return(false));
  manager()->UpdateService(wifi_service);
  EXPECT_FALSE(manager()->IsWifiIdle());

  // wifi connected.
  Mock::VerifyAndClearExpectations(wifi_service.get());
  EXPECT_CALL(*wifi_service.get(), technology())
      .WillRepeatedly(Return(Technology::kWifi));
  EXPECT_CALL(*wifi_service.get(), IsConnecting())
      .WillRepeatedly(Return(false));
  EXPECT_CALL(*wifi_service.get(), IsConnected())
      .WillRepeatedly(Return(true));
  manager()->UpdateService(wifi_service);
  EXPECT_FALSE(manager()->IsWifiIdle());
}

TEST_F(ManagerTest, DetectMultiHomedDevices) {
  vector<scoped_refptr<MockConnection>> mock_connections;
  vector<ConnectionRefPtr> device_connections;
  mock_devices_.push_back(new NiceMock<MockDevice>(control_interface(),
                                                   dispatcher(),
                                                   metrics(),
                                                   manager(),
                                                   "null4",
                                                   "addr4",
                                                   0));
  mock_devices_.push_back(new NiceMock<MockDevice>(control_interface(),
                                                   dispatcher(),
                                                   metrics(),
                                                   manager(),
                                                   "null5",
                                                   "addr5",
                                                   0));
  for (const auto& device : mock_devices_) {
    manager()->RegisterDevice(device);
    mock_connections.emplace_back(
        new NiceMock<MockConnection>(device_info_.get()));
    device_connections.emplace_back(mock_connections.back());
  }
  EXPECT_CALL(*mock_connections[1], GetSubnetName()).WillOnce(Return("1"));
  EXPECT_CALL(*mock_connections[2], GetSubnetName()).WillOnce(Return("2"));
  EXPECT_CALL(*mock_connections[3], GetSubnetName()).WillOnce(Return("1"));
  EXPECT_CALL(*mock_connections[4], GetSubnetName()).WillOnce(Return(""));
  EXPECT_CALL(*mock_connections[5], GetSubnetName()).WillOnce(Return(""));

  // Do not assign a connection to mock_devices_[0].
  EXPECT_CALL(*mock_devices_[1], connection())
      .WillRepeatedly(ReturnRef(device_connections[1]));
  EXPECT_CALL(*mock_devices_[2], connection())
      .WillRepeatedly(ReturnRef(device_connections[2]));
  EXPECT_CALL(*mock_devices_[3], connection())
      .WillRepeatedly(ReturnRef(device_connections[3]));
  EXPECT_CALL(*mock_devices_[4], connection())
      .WillRepeatedly(ReturnRef(device_connections[4]));
  EXPECT_CALL(*mock_devices_[5], connection())
      .WillRepeatedly(ReturnRef(device_connections[5]));

  EXPECT_CALL(*mock_devices_[0], SetIsMultiHomed(false));
  EXPECT_CALL(*mock_devices_[1], SetIsMultiHomed(true));
  EXPECT_CALL(*mock_devices_[2], SetIsMultiHomed(false));
  EXPECT_CALL(*mock_devices_[3], SetIsMultiHomed(true));
  EXPECT_CALL(*mock_devices_[4], SetIsMultiHomed(false));
  EXPECT_CALL(*mock_devices_[5], SetIsMultiHomed(false));
  manager()->DetectMultiHomedDevices();
}

TEST_F(ManagerTest, IsTechnologyProhibited) {
  // Test initial state.
  EXPECT_EQ("", manager()->props_.prohibited_technologies);
  EXPECT_FALSE(manager()->IsTechnologyProhibited(Technology::kWiMax));
  EXPECT_FALSE(manager()->IsTechnologyProhibited(Technology::kVPN));

  Error smoke_error;
  EXPECT_FALSE(manager()->SetProhibitedTechnologies("smoke_signal",
                                                    &smoke_error));
  EXPECT_EQ(Error::kInvalidArguments, smoke_error.type());

  ON_CALL(*mock_devices_[0], technology())
      .WillByDefault(Return(Technology::kVPN));
  ON_CALL(*mock_devices_[1], technology())
      .WillByDefault(Return(Technology::kWiMax));
  ON_CALL(*mock_devices_[2], technology())
      .WillByDefault(Return(Technology::kWifi));

  manager()->RegisterDevice(mock_devices_[0]);
  manager()->RegisterDevice(mock_devices_[1]);
  manager()->RegisterDevice(mock_devices_[2]);

  // Registered devices of prohibited technology types should be disabled.
  EXPECT_CALL(*mock_devices_[0], SetEnabledNonPersistent(false, _, _));
  EXPECT_CALL(*mock_devices_[1], SetEnabledNonPersistent(false, _, _));
  EXPECT_CALL(*mock_devices_[2], SetEnabledNonPersistent(false, _, _)).Times(0);
  Error error;
  manager()->SetProhibitedTechnologies("wimax,vpn", &error);
  EXPECT_TRUE(manager()->IsTechnologyProhibited(Technology::kVPN));
  EXPECT_TRUE(manager()->IsTechnologyProhibited(Technology::kWiMax));
  EXPECT_FALSE(manager()->IsTechnologyProhibited(Technology::kWifi));
  Mock::VerifyAndClearExpectations(mock_devices_[0].get());
  Mock::VerifyAndClearExpectations(mock_devices_[1].get());
  Mock::VerifyAndClearExpectations(mock_devices_[2].get());

  // Newly registered devices should be disabled.
  mock_devices_.push_back(new NiceMock<MockDevice>(control_interface(),
                                                   dispatcher(),
                                                   metrics(),
                                                   manager(),
                                                   "null4",
                                                   "addr4",
                                                   0));
  mock_devices_.push_back(new NiceMock<MockDevice>(control_interface(),
                                                   dispatcher(),
                                                   metrics(),
                                                   manager(),
                                                   "null5",
                                                   "addr5",
                                                   0));
  ON_CALL(*mock_devices_[3], technology())
      .WillByDefault(Return(Technology::kVPN));
  ON_CALL(*mock_devices_[4], technology())
      .WillByDefault(Return(Technology::kWiMax));
  ON_CALL(*mock_devices_[5], technology())
      .WillByDefault(Return(Technology::kWifi));

  EXPECT_CALL(*mock_devices_[3], SetEnabledNonPersistent(false, _, _));
  EXPECT_CALL(*mock_devices_[4], SetEnabledNonPersistent(false, _, _));
  EXPECT_CALL(*mock_devices_[5], SetEnabledPersistent(false, _, _)).Times(0);

  manager()->RegisterDevice(mock_devices_[3]);
  manager()->RegisterDevice(mock_devices_[4]);
  manager()->RegisterDevice(mock_devices_[5]);
  Mock::VerifyAndClearExpectations(mock_devices_[3].get());
  Mock::VerifyAndClearExpectations(mock_devices_[4].get());
  Mock::VerifyAndClearExpectations(mock_devices_[5].get());

  // Calls to enable a non-prohibited technology should succeed.
  Error enable_error(Error::kOperationInitiated);
  DisableTechnologyReplyHandler technology_reply_handler;
  ResultCallback enable_technology_callback(
      Bind(&DisableTechnologyReplyHandler::ReportResult,
           technology_reply_handler.AsWeakPtr()));
  EXPECT_CALL(*mock_devices_[2], SetEnabledPersistent(true, _, _));
  EXPECT_CALL(*mock_devices_[5], SetEnabledPersistent(true, _, _));
  manager()->SetEnabledStateForTechnology(
      "wifi", true, true, &enable_error, enable_technology_callback);
  EXPECT_EQ(Error::kOperationInitiated, enable_error.type());

  // Calls to enable a prohibited technology should fail.
  Error enable_prohibited_error(Error::kOperationInitiated);
  EXPECT_CALL(*mock_devices_[0], SetEnabledPersistent(true, _, _)).Times(0);
  EXPECT_CALL(*mock_devices_[3], SetEnabledPersistent(true, _, _)).Times(0);
  manager()->SetEnabledStateForTechnology(
      "vpn", true, true, &enable_prohibited_error, enable_technology_callback);
  EXPECT_EQ(Error::kPermissionDenied, enable_prohibited_error.type());
}

TEST_F(ManagerTest, ClaimBlacklistedDevice) {
  const string kClaimerName = "test_claimer";
  const string kDeviceName = "test_device";

  // Set blacklisted devices.
  vector<string> blacklisted_devices = { kDeviceName };
  manager()->SetBlacklistedDevices(blacklisted_devices);

  Error error;
  manager()->ClaimDevice(kClaimerName, kDeviceName, &error);
  EXPECT_TRUE(error.IsFailure());
  EXPECT_EQ("Not allowed to claim unmanaged device", error.message());
  // Verify device claimer is not created.
  EXPECT_EQ(nullptr, manager()->device_claimer_.get());
}

TEST_F(ManagerTest, ReleaseBlacklistedDevice) {
  const string kClaimerName = "test_claimer";
  const string kDeviceName = "test_device";

  // Set blacklisted devices.
  vector<string> blacklisted_devices = { kDeviceName };
  manager()->SetBlacklistedDevices(blacklisted_devices);

  Error error;
  bool claimer_removed;
  manager()->ReleaseDevice(kClaimerName, kDeviceName, &claimer_removed, &error);
  EXPECT_TRUE(error.IsFailure());
  EXPECT_FALSE(claimer_removed);
  EXPECT_EQ("Not allowed to release unmanaged device", error.message());
}

TEST_F(ManagerTest, BlacklistedDeviceIsNotManaged) {
  const string kDeviceName = "test_device";

  vector<string> blacklisted_devices = { kDeviceName };
  manager()->SetBlacklistedDevices(blacklisted_devices);
  EXPECT_FALSE(manager()->DeviceManagementAllowed(kDeviceName));
}

TEST_F(ManagerTest, NonBlacklistedDeviceIsManaged) {
  const string kDeviceName = "test_device";

  vector<string> blacklisted_devices = { "other_device" };
  manager()->SetBlacklistedDevices(blacklisted_devices);
  EXPECT_TRUE(manager()->DeviceManagementAllowed(kDeviceName));
}

TEST_F(ManagerTest, WhitelistedDeviceIsManaged) {
  const string kDeviceName = "test_device";

  vector<string> whitelisted_devices = { kDeviceName };
  manager()->SetWhitelistedDevices(whitelisted_devices);
  EXPECT_TRUE(manager()->DeviceManagementAllowed(kDeviceName));
}

TEST_F(ManagerTest, NonWhitelistedDeviceIsNotManaged) {
  const string kDeviceName = "test_device";

  vector<string> whitelisted_devices = { "other_device" };
  manager()->SetWhitelistedDevices(whitelisted_devices);
  EXPECT_FALSE(manager()->DeviceManagementAllowed(kDeviceName));
}

TEST_F(ManagerTest, DevicesIsManagedByDefault) {
  EXPECT_TRUE(manager()->DeviceManagementAllowed("test_device"));
}

TEST_F(ManagerTest, ClaimDeviceWithoutClaimer) {
  const char kClaimerName[] = "test_claimer1";
  const char kDeviceName[] = "test_device";

  // Claim device when device claimer doesn't exist yet.
  Error error;
  manager()->ClaimDevice(kClaimerName, kDeviceName, &error);
  EXPECT_TRUE(error.IsSuccess());
  EXPECT_TRUE(manager()->device_info()->IsDeviceBlackListed(kDeviceName));
  // Verify device claimer is created.
  EXPECT_NE(nullptr, manager()->device_claimer_.get());
}

TEST_F(ManagerTest, ClaimDeviceWithClaimer) {
  const char kClaimer1Name[] = "test_claimer1";
  const char kClaimer2Name[] = "test_claimer2";
  const char kDeviceName[] = "test_device";

  // Setup device claimer.
  MockDeviceClaimer* device_claimer = new MockDeviceClaimer(kClaimer1Name);
  SetDeviceClaimer(device_claimer);

  // Claim device with empty string name.
  const char kEmptyDeviceNameError[] = "Empty device name";
  Error error;
  manager()->ClaimDevice(kClaimer1Name, "", &error);
  EXPECT_EQ(string(kEmptyDeviceNameError), error.message());

  // Device claim succeed.
  error.Reset();
  EXPECT_CALL(*device_claimer, Claim(kDeviceName, _)).WillOnce(Return(true));
  manager()->ClaimDevice(kClaimer1Name, kDeviceName, &error);
  EXPECT_EQ(Error::kSuccess, error.type());
  Mock::VerifyAndClearExpectations(device_claimer);

  // Claimer mismatch, current implementation only allows one claimer at a time.
  const char kInvalidClaimerError[] =
      "Invalid claimer name test_claimer2. Claimer test_claimer1 already exist";
  error.Reset();
  EXPECT_CALL(*device_claimer, Claim(_, _)).Times(0);
  manager()->ClaimDevice(kClaimer2Name, kDeviceName, &error);
  EXPECT_EQ(string(kInvalidClaimerError), error.message());
}

TEST_F(ManagerTest, ClaimRegisteredDevice) {
  const char kClaimerName[] = "test_claimer";

  // Setup device claimer.
  MockDeviceClaimer* device_claimer = new MockDeviceClaimer(kClaimerName);
  SetDeviceClaimer(device_claimer);

  // Register a device to manager.
  ON_CALL(*mock_devices_[0].get(), technology())
      .WillByDefault(Return(Technology::kWifi));
  manager()->RegisterDevice(mock_devices_[0]);
  // Verify device is registered.
  EXPECT_TRUE(IsDeviceRegistered(mock_devices_[0], Technology::kWifi));

  // Claim the registered device.
  Error error;
  EXPECT_CALL(*device_claimer, Claim(mock_devices_[0]->link_name(), _))
      .WillOnce(Return(true));
  manager()->ClaimDevice(kClaimerName, mock_devices_[0]->link_name(), &error);
  EXPECT_EQ(Error::kSuccess, error.type());
  Mock::VerifyAndClearExpectations(device_claimer);

  // Expect device to not be registered anymore.
  EXPECT_FALSE(IsDeviceRegistered(mock_devices_[0], Technology::kWifi));
}

TEST_F(ManagerTest, ReleaseDevice) {
  const char kClaimerName[] = "test_claimer";
  const char kWrongClaimerName[] = "test_claimer1";
  const char kDeviceName[] = "test_device";

  // Release device without claimer.
  const char kNoClaimerError[] = "Device claimer doesn't exist";
  Error error;
  bool claimer_removed;
  manager()->ReleaseDevice(kClaimerName, kDeviceName, &claimer_removed, &error);
  EXPECT_EQ(string(kNoClaimerError), error.message());
  EXPECT_FALSE(claimer_removed);

  // Setup device claimer.
  MockDeviceClaimer* device_claimer = new MockDeviceClaimer(kClaimerName);
  SetDeviceClaimer(device_claimer);

  // Release device from wrong claimer.
  const char kClaimerMismatchError[] =
      "Invalid claimer name test_claimer1. Claimer test_claimer already exist";
  error.Reset();
  manager()->ReleaseDevice(kWrongClaimerName, kDeviceName, &claimer_removed,
                           &error);
  EXPECT_EQ(string(kClaimerMismatchError), error.message());
  EXPECT_FALSE(claimer_removed);

  // Release one of multiple device from a non-default claimer.
  error.Reset();
  EXPECT_CALL(*device_claimer, Release(kDeviceName, &error))
      .WillOnce(Return(true));
  EXPECT_CALL(*device_claimer, default_claimer()).WillOnce(Return(false));
  EXPECT_CALL(*device_claimer, DevicesClaimed()).WillOnce(Return(true));
  manager()->ReleaseDevice(kClaimerName, kDeviceName, &claimer_removed, &error);
  Mock::VerifyAndClearExpectations(device_claimer);
  EXPECT_TRUE(error.IsSuccess());
  EXPECT_FALSE(claimer_removed);

  // Release a device with default claimer. Claimer should not be resetted.
  error.Reset();
  EXPECT_CALL(*device_claimer, Release(kDeviceName, &error))
      .WillOnce(Return(true));
  EXPECT_CALL(*device_claimer, default_claimer()).WillOnce(Return(true));
  EXPECT_CALL(*device_claimer, DevicesClaimed()).Times(0);
  manager()->ReleaseDevice(kClaimerName, kDeviceName, &claimer_removed, &error);
  Mock::VerifyAndClearExpectations(device_claimer);
  EXPECT_TRUE(error.IsSuccess());
  EXPECT_FALSE(claimer_removed);
  EXPECT_NE(nullptr, manager()->device_claimer_.get());

  // Release last device with non-default claimer. Claimer should be resetted.
  error.Reset();
  EXPECT_CALL(*device_claimer, Release(kDeviceName, &error))
      .WillOnce(Return(true));
  EXPECT_CALL(*device_claimer, default_claimer()).WillOnce(Return(false));
  EXPECT_CALL(*device_claimer, DevicesClaimed()).WillOnce(Return(false));
  manager()->ReleaseDevice(kClaimerName, kDeviceName, &claimer_removed, &error);
  Mock::VerifyAndClearExpectations(device_claimer);
  EXPECT_TRUE(error.IsSuccess());
  EXPECT_TRUE(claimer_removed);
  EXPECT_EQ(nullptr, manager()->device_claimer_.get());
}

TEST_F(ManagerTest, GetEnabledDeviceWithTechnology) {
  auto ethernet_device = mock_devices_[0];
  auto wifi_device = mock_devices_[1];
  auto cellular_device = mock_devices_[2];
  ON_CALL(*ethernet_device.get(), technology())
      .WillByDefault(Return(Technology::kEthernet));
  ON_CALL(*wifi_device.get(), technology())
      .WillByDefault(Return(Technology::kWifi));
  ON_CALL(*cellular_device.get(), technology())
      .WillByDefault(Return(Technology::kCellular));
  ethernet_device->enabled_ = true;
  wifi_device->enabled_ = true;
  cellular_device->enabled_ = true;

  manager()->RegisterDevice(ethernet_device);
  manager()->RegisterDevice(wifi_device);
  manager()->RegisterDevice(cellular_device);

  EXPECT_EQ(ethernet_device,
            manager()->GetEnabledDeviceWithTechnology(Technology::kEthernet));
  EXPECT_EQ(wifi_device,
            manager()->GetEnabledDeviceWithTechnology(Technology::kWifi));
  EXPECT_EQ(cellular_device,
            manager()->GetEnabledDeviceWithTechnology(Technology::kCellular));
}

TEST_F(ManagerTest, GetEnabledDeviceByLinkName) {
  auto ethernet_device = mock_devices_[0];
  auto wifi_device = mock_devices_[1];
  auto disabled_wifi_device = mock_devices_[2];
  ON_CALL(*ethernet_device.get(), technology())
      .WillByDefault(Return(Technology::kEthernet));
  ON_CALL(*wifi_device.get(), technology())
      .WillByDefault(Return(Technology::kWifi));
  ON_CALL(*disabled_wifi_device.get(), technology())
      .WillByDefault(Return(Technology::kWifi));
  ethernet_device->enabled_ = true;
  wifi_device->enabled_ = true;
  disabled_wifi_device->enabled_ = false;

  manager()->RegisterDevice(ethernet_device);
  manager()->RegisterDevice(wifi_device);

  EXPECT_EQ(ethernet_device,
            manager()->GetEnabledDeviceByLinkName(
                ethernet_device->link_name()));
  EXPECT_EQ(wifi_device,
            manager()->GetEnabledDeviceByLinkName(wifi_device->link_name()));
  EXPECT_EQ(nullptr,
            manager()->GetEnabledDeviceByLinkName(
                disabled_wifi_device->link_name()));
}

TEST_F(ManagerTest, AcceptHostnameFrom) {
  EXPECT_FALSE(manager()->ShouldAcceptHostnameFrom("eth0"));
  EXPECT_FALSE(manager()->ShouldAcceptHostnameFrom("eth1"));
  EXPECT_FALSE(manager()->ShouldAcceptHostnameFrom("wlan0"));

  manager()->SetAcceptHostnameFrom("eth0");
  EXPECT_TRUE(manager()->ShouldAcceptHostnameFrom("eth0"));
  EXPECT_FALSE(manager()->ShouldAcceptHostnameFrom("eth1"));
  EXPECT_FALSE(manager()->ShouldAcceptHostnameFrom("wlan0"));

  manager()->SetAcceptHostnameFrom("eth1");
  EXPECT_FALSE(manager()->ShouldAcceptHostnameFrom("eth0"));
  EXPECT_TRUE(manager()->ShouldAcceptHostnameFrom("eth1"));
  EXPECT_FALSE(manager()->ShouldAcceptHostnameFrom("wlan0"));

  manager()->SetAcceptHostnameFrom("eth*");
  EXPECT_TRUE(manager()->ShouldAcceptHostnameFrom("eth0"));
  EXPECT_TRUE(manager()->ShouldAcceptHostnameFrom("eth1"));
  EXPECT_FALSE(manager()->ShouldAcceptHostnameFrom("wlan0"));

  manager()->SetAcceptHostnameFrom("wlan*");
  EXPECT_FALSE(manager()->ShouldAcceptHostnameFrom("eth0"));
  EXPECT_FALSE(manager()->ShouldAcceptHostnameFrom("eth1"));
  EXPECT_TRUE(manager()->ShouldAcceptHostnameFrom("wlan0"));

  manager()->SetAcceptHostnameFrom("ether*");
  EXPECT_FALSE(manager()->ShouldAcceptHostnameFrom("eth0"));
  EXPECT_FALSE(manager()->ShouldAcceptHostnameFrom("eth1"));
  EXPECT_FALSE(manager()->ShouldAcceptHostnameFrom("wlan0"));
}

TEST_F(ManagerTest, DHCPv6EnabledDevices) {
  EXPECT_FALSE(manager()->IsDHCPv6EnabledForDevice("eth0"));
  EXPECT_FALSE(manager()->IsDHCPv6EnabledForDevice("eth1"));
  EXPECT_FALSE(manager()->IsDHCPv6EnabledForDevice("wlan0"));

  vector<string> enabled_devices;
  enabled_devices.push_back("eth0");
  manager()->SetDHCPv6EnabledDevices(enabled_devices);
  EXPECT_TRUE(manager()->IsDHCPv6EnabledForDevice("eth0"));
  EXPECT_FALSE(manager()->IsDHCPv6EnabledForDevice("eth1"));
  EXPECT_FALSE(manager()->IsDHCPv6EnabledForDevice("wlan0"));

  enabled_devices.push_back("eth1");
  manager()->SetDHCPv6EnabledDevices(enabled_devices);
  EXPECT_TRUE(manager()->IsDHCPv6EnabledForDevice("eth0"));
  EXPECT_TRUE(manager()->IsDHCPv6EnabledForDevice("eth1"));
  EXPECT_FALSE(manager()->IsDHCPv6EnabledForDevice("wlan0"));

  enabled_devices.push_back("wlan0");
  manager()->SetDHCPv6EnabledDevices(enabled_devices);
  EXPECT_TRUE(manager()->IsDHCPv6EnabledForDevice("eth0"));
  EXPECT_TRUE(manager()->IsDHCPv6EnabledForDevice("eth1"));
  EXPECT_TRUE(manager()->IsDHCPv6EnabledForDevice("wlan0"));
}

TEST_F(ManagerTest, FilterPrependDNSServersByFamily) {
  const struct {
    IPAddress::Family family;
    string prepend_value;
    vector<string> output_list;
  } expectations[] = {
    {IPAddress::kFamilyIPv4, "", {}},
    {IPAddress::kFamilyIPv4, "8.8.8.8", {"8.8.8.8"}},
    {IPAddress::kFamilyIPv4, "8.8.8.8,2001:4860:4860::8888", {"8.8.8.8"}},
    {IPAddress::kFamilyIPv4, "2001:4860:4860::8844", {}},
    {IPAddress::kFamilyIPv6, "", {}},
    {IPAddress::kFamilyIPv6, "8.8.8.8", {}},
    {IPAddress::kFamilyIPv6, "2001:4860:4860::8844",
        {"2001:4860:4860::8844"}},
    {IPAddress::kFamilyIPv6, "8.8.8.8,2001:4860:4860::8888",
        {"2001:4860:4860::8888"}}
  };

  for (const auto& expectation : expectations) {
    manager()->SetPrependDNSServers(expectation.prepend_value);
    auto dns_servers =
        manager()->FilterPrependDNSServersByFamily(expectation.family);
    EXPECT_EQ(expectation.output_list, dns_servers);
  }
}

}  // namespace shill
