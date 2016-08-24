//
// Copyright (C) 2013 The Android Open Source Project
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

#include "shill/cellular/cellular_capability_universal.h"

#include <string>
#include <tuple>
#include <vector>

#include <base/bind.h>
#include <base/strings/string_util.h>
#include <base/strings/stringprintf.h>
#if defined(__ANDROID__)
#include <dbus/service_constants.h>
#else
#include <chromeos/dbus/service_constants.h>
#endif  // __ANDROID__
#include <ModemManager/ModemManager.h>

#include "shill/cellular/cellular.h"
#include "shill/cellular/cellular_bearer.h"
#include "shill/cellular/cellular_service.h"
#include "shill/cellular/mock_cellular.h"
#include "shill/cellular/mock_cellular_service.h"
#include "shill/cellular/mock_mm1_modem_modem3gpp_proxy.h"
#include "shill/cellular/mock_mm1_modem_modemcdma_proxy.h"
#include "shill/cellular/mock_mm1_modem_proxy.h"
#include "shill/cellular/mock_mm1_modem_simple_proxy.h"
#include "shill/cellular/mock_mm1_sim_proxy.h"
#include "shill/cellular/mock_mobile_operator_info.h"
#include "shill/cellular/mock_modem_info.h"
#include "shill/error.h"
#include "shill/mock_adaptors.h"
#include "shill/mock_control.h"
#include "shill/mock_dbus_properties_proxy.h"
#include "shill/mock_event_dispatcher.h"
#include "shill/mock_pending_activation_store.h"
#include "shill/mock_profile.h"
#include "shill/net/mock_rtnl_handler.h"
#include "shill/test_event_dispatcher.h"
#include "shill/testing.h"

using base::Bind;
using base::StringPrintf;
using base::Unretained;
using std::string;
using std::unique_ptr;
using std::vector;
using testing::AnyNumber;
using testing::InSequence;
using testing::Invoke;
using testing::InvokeWithoutArgs;
using testing::Mock;
using testing::NiceMock;
using testing::Return;
using testing::ReturnRef;
using testing::SaveArg;
using testing::_;

namespace shill {

MATCHER_P(HasApn, expected_apn, "") {
  return arg.ContainsString(CellularCapabilityUniversal::kConnectApn) &&
      expected_apn == arg.GetString(CellularCapabilityUniversal::kConnectApn);
}

class CellularCapabilityUniversalTest : public testing::TestWithParam<string> {
 public:
  explicit CellularCapabilityUniversalTest(EventDispatcher* dispatcher)
      : dispatcher_(dispatcher),
        control_interface_(this),
        modem_info_(&control_interface_, dispatcher, nullptr, nullptr),
        modem_3gpp_proxy_(new mm1::MockModemModem3gppProxy()),
        modem_cdma_proxy_(new mm1::MockModemModemCdmaProxy()),
        modem_proxy_(new mm1::MockModemProxy()),
        modem_simple_proxy_(new mm1::MockModemSimpleProxy()),
        sim_proxy_(new mm1::MockSimProxy()),
        properties_proxy_(new MockDBusPropertiesProxy()),
        capability_(nullptr),
        device_adaptor_(nullptr),
        cellular_(new Cellular(&modem_info_,
                               "",
                               "00:01:02:03:04:05",
                               0,
                               Cellular::kTypeUniversal,
                               "",
                               "")),
        service_(new MockCellularService(&modem_info_, cellular_)),
        mock_home_provider_info_(nullptr),
        mock_serving_operator_info_(nullptr) {
    modem_info_.metrics()->RegisterDevice(cellular_->interface_index(),
                                          Technology::kCellular);
  }

  virtual ~CellularCapabilityUniversalTest() {
    cellular_->service_ = nullptr;
    capability_ = nullptr;
    device_adaptor_ = nullptr;
  }

  virtual void SetUp() {
    capability_ = static_cast<CellularCapabilityUniversal*>(
        cellular_->capability_.get());
    device_adaptor_ =
        static_cast<DeviceMockAdaptor*>(cellular_->adaptor());
    cellular_->service_ = service_;

    // kStateUnknown leads to minimal extra work in maintaining
    // activation state.
    ON_CALL(*modem_info_.mock_pending_activation_store(),
            GetActivationState(PendingActivationStore::kIdentifierICCID, _))
        .WillByDefault(Return(PendingActivationStore::kStateUnknown));

    SetMockMobileOperatorInfoObjects();
  }

  virtual void TearDown() {
    capability_->control_interface_ = nullptr;
  }

  void CreateService() {
    // The following constants are never directly accessed by the tests.
    const char kStorageIdentifier[] = "default_test_storage_id";
    const char kFriendlyServiceName[] = "default_test_service_name";
    const char kOperatorCode[] = "10010";
    const char kOperatorName[] = "default_test_operator_name";
    const char kOperatorCountry[] = "us";

    // Simulate all the side-effects of Cellular::CreateService
    auto service = new CellularService(&modem_info_, cellular_);
    service->SetStorageIdentifier(kStorageIdentifier);
    service->SetFriendlyName(kFriendlyServiceName);

    Stringmap serving_operator;
    serving_operator[kOperatorCodeKey] = kOperatorCode;
    serving_operator[kOperatorNameKey] = kOperatorName;
    serving_operator[kOperatorCountryKey] = kOperatorCountry;
    service->set_serving_operator(serving_operator);
    cellular_->set_home_provider(serving_operator);
    cellular_->service_ = service;
  }

  void ClearService() {
    cellular_->service_ = nullptr;
  }

  void ExpectModemAndModem3gppProperties() {
    // Set up mock modem properties.
    KeyValueStore modem_properties;
    string operator_name = "TestOperator";
    string operator_code = "001400";

    modem_properties.SetUint(MM_MODEM_PROPERTY_ACCESSTECHNOLOGIES,
                             kAccessTechnologies);
    std::tuple<uint32_t, bool> signal_signal { 90, true };
    modem_properties.Set(MM_MODEM_PROPERTY_SIGNALQUALITY,
                         brillo::Any(signal_signal));

    // Set up mock modem 3gpp properties.
    KeyValueStore modem3gpp_properties;
    modem3gpp_properties.SetUint(
        MM_MODEM_MODEM3GPP_PROPERTY_ENABLEDFACILITYLOCKS, 0);
    modem3gpp_properties.SetString(MM_MODEM_MODEM3GPP_PROPERTY_IMEI, kImei);

    EXPECT_CALL(*properties_proxy_,
                GetAll(MM_DBUS_INTERFACE_MODEM))
        .WillOnce(Return(modem_properties));
    EXPECT_CALL(*properties_proxy_,
                GetAll(MM_DBUS_INTERFACE_MODEM_MODEM3GPP))
        .WillOnce(Return(modem3gpp_properties));
  }

  void InvokeEnable(bool enable, Error* error,
                    const ResultCallback& callback, int timeout) {
    callback.Run(Error());
  }
  void InvokeEnableFail(bool enable, Error* error,
                        const ResultCallback& callback, int timeout) {
    callback.Run(Error(Error::kOperationFailed));
  }
  void InvokeEnableInWrongState(bool enable, Error* error,
                                const ResultCallback& callback, int timeout) {
    callback.Run(Error(Error::kWrongState));
  }
  void InvokeRegister(const string& operator_id, Error* error,
                      const ResultCallback& callback, int timeout) {
    callback.Run(Error());
  }
  void InvokeSetPowerState(const uint32_t& power_state,
                           Error* error,
                           const ResultCallback& callback,
                           int timeout) {
    callback.Run(Error());
  }
  void Set3gppProxy() {
    capability_->modem_3gpp_proxy_.reset(modem_3gpp_proxy_.release());
  }

  void SetSimpleProxy() {
    capability_->modem_simple_proxy_.reset(modem_simple_proxy_.release());
  }

  void SetMockMobileOperatorInfoObjects() {
    CHECK(!mock_home_provider_info_);
    CHECK(!mock_serving_operator_info_);
    mock_home_provider_info_ =
        new MockMobileOperatorInfo(dispatcher_, "HomeProvider");
    mock_serving_operator_info_ =
        new MockMobileOperatorInfo(dispatcher_, "ServingOperator");
    cellular_->set_home_provider_info(mock_home_provider_info_);
    cellular_->set_serving_operator_info(mock_serving_operator_info_);
  }

  void ReleaseCapabilityProxies() {
    capability_->ReleaseProxies();
  }

  void SetRegistrationDroppedUpdateTimeout(int64_t timeout_milliseconds) {
    capability_->registration_dropped_update_timeout_milliseconds_ =
        timeout_milliseconds;
  }

  MOCK_METHOD1(TestCallback, void(const Error& error));

  MOCK_METHOD0(DummyCallback, void(void));

  void SetMockRegistrationDroppedUpdateCallback() {
    capability_->registration_dropped_update_callback_.Reset(
        Bind(&CellularCapabilityUniversalTest::DummyCallback,
             Unretained(this)));
  }

 protected:
  static const char kActiveBearerPathPrefix[];
  static const char kImei[];
  static const char kInactiveBearerPathPrefix[];
  static const char kSimPath[];
  static const uint32_t kAccessTechnologies;
  static const char kTestMobileProviderDBPath[];

  class TestControl : public MockControl {
   public:
    explicit TestControl(CellularCapabilityUniversalTest* test)
        : test_(test) {
      active_bearer_properties_.SetBool(MM_BEARER_PROPERTY_CONNECTED, true);
      active_bearer_properties_.SetString(MM_BEARER_PROPERTY_INTERFACE,
                                          "/dev/fake");

      KeyValueStore ip4config;
      ip4config.SetUint("method", MM_BEARER_IP_METHOD_DHCP);
      active_bearer_properties_.SetKeyValueStore(
          MM_BEARER_PROPERTY_IP4CONFIG, ip4config);

      inactive_bearer_properties_.SetBool(MM_BEARER_PROPERTY_CONNECTED, false);
    }

    KeyValueStore* mutable_active_bearer_properties() {
      return &active_bearer_properties_;
    }

    KeyValueStore* mutable_inactive_bearer_properties() {
      return &inactive_bearer_properties_;
    }

    virtual mm1::ModemModem3gppProxyInterface* CreateMM1ModemModem3gppProxy(
        const std::string& /*path*/,
        const std::string& /*service*/) {
      return test_->modem_3gpp_proxy_.release();
    }

    virtual mm1::ModemModemCdmaProxyInterface* CreateMM1ModemModemCdmaProxy(
        const std::string& /*path*/,
        const std::string& /*service*/) {
      return test_->modem_cdma_proxy_.release();
    }

    virtual mm1::ModemProxyInterface* CreateMM1ModemProxy(
        const std::string& /*path*/,
        const std::string& /*service*/) {
      return test_->modem_proxy_.release();
    }

    virtual mm1::ModemSimpleProxyInterface* CreateMM1ModemSimpleProxy(
        const std::string& /*path*/,
        const std::string& /*service*/) {
      return test_->modem_simple_proxy_.release();
    }

    virtual mm1::SimProxyInterface* CreateSimProxy(
        const std::string& /*path*/,
        const std::string& /*service*/) {
      mm1::MockSimProxy* sim_proxy = test_->sim_proxy_.release();
      test_->sim_proxy_.reset(new mm1::MockSimProxy());
      return sim_proxy;
    }

    virtual DBusPropertiesProxyInterface* CreateDBusPropertiesProxy(
        const std::string& path,
        const std::string& /*service*/) {
      MockDBusPropertiesProxy* properties_proxy =
          test_->properties_proxy_.release();
      if (path.find(kActiveBearerPathPrefix) != std::string::npos) {
        EXPECT_CALL(*properties_proxy, GetAll(MM_DBUS_INTERFACE_BEARER))
            .Times(AnyNumber())
            .WillRepeatedly(Return(active_bearer_properties_));
      } else {
        EXPECT_CALL(*properties_proxy, GetAll(MM_DBUS_INTERFACE_BEARER))
            .Times(AnyNumber())
            .WillRepeatedly(Return(inactive_bearer_properties_));
      }
      test_->properties_proxy_.reset(new MockDBusPropertiesProxy());
      return properties_proxy;
    }

   private:
    CellularCapabilityUniversalTest* test_;
    KeyValueStore active_bearer_properties_;
    KeyValueStore inactive_bearer_properties_;
  };

  EventDispatcher* dispatcher_;
  TestControl control_interface_;
  MockModemInfo modem_info_;
  unique_ptr<mm1::MockModemModem3gppProxy> modem_3gpp_proxy_;
  unique_ptr<mm1::MockModemModemCdmaProxy> modem_cdma_proxy_;
  unique_ptr<mm1::MockModemProxy> modem_proxy_;
  unique_ptr<mm1::MockModemSimpleProxy> modem_simple_proxy_;
  unique_ptr<mm1::MockSimProxy> sim_proxy_;
  unique_ptr<MockDBusPropertiesProxy> properties_proxy_;
  CellularCapabilityUniversal* capability_;  // Owned by |cellular_|.
  DeviceMockAdaptor* device_adaptor_;  // Owned by |cellular_|.
  CellularRefPtr cellular_;
  MockCellularService* service_;  // owned by cellular_
  // saved for testing connect operations.
  RpcIdentifierCallback connect_callback_;

  // Set when required and passed to |cellular_|. Owned by |cellular_|.
  MockMobileOperatorInfo* mock_home_provider_info_;
  MockMobileOperatorInfo* mock_serving_operator_info_;
};

// Most of our tests involve using a real EventDispatcher object.
class CellularCapabilityUniversalMainTest
    : public CellularCapabilityUniversalTest {
 public:
  CellularCapabilityUniversalMainTest() :
      CellularCapabilityUniversalTest(&dispatcher_) {}

 protected:
  EventDispatcherForTest dispatcher_;
};

// Tests that involve timers will (or may) use a mock of the event dispatcher
// instead of a real one.
class CellularCapabilityUniversalTimerTest
    : public CellularCapabilityUniversalTest {
 public:
  CellularCapabilityUniversalTimerTest()
      : CellularCapabilityUniversalTest(&mock_dispatcher_) {}

 protected:
  ::testing::StrictMock<MockEventDispatcher> mock_dispatcher_;
};

const char CellularCapabilityUniversalTest::kActiveBearerPathPrefix[] =
    "/bearer/active";
const char CellularCapabilityUniversalTest::kImei[] = "999911110000";
const char CellularCapabilityUniversalTest::kInactiveBearerPathPrefix[] =
    "/bearer/inactive";
const char CellularCapabilityUniversalTest::kSimPath[] = "/foo/sim";
const uint32_t CellularCapabilityUniversalTest::kAccessTechnologies =
    MM_MODEM_ACCESS_TECHNOLOGY_LTE |
    MM_MODEM_ACCESS_TECHNOLOGY_HSPA_PLUS;
const char CellularCapabilityUniversalTest::kTestMobileProviderDBPath[] =
    "provider_db_unittest.bfd";

TEST_F(CellularCapabilityUniversalMainTest, StartModem) {
  ExpectModemAndModem3gppProperties();

  EXPECT_CALL(*modem_proxy_,
              Enable(true, _, _, CellularCapability::kTimeoutEnable))
      .WillOnce(Invoke(
           this, &CellularCapabilityUniversalTest::InvokeEnable));

  Error error;
  EXPECT_CALL(*this, TestCallback(IsSuccess()));
  ResultCallback callback =
      Bind(&CellularCapabilityUniversalTest::TestCallback, Unretained(this));
  capability_->StartModem(&error, callback);

  EXPECT_TRUE(error.IsOngoing());
  EXPECT_EQ(kImei, cellular_->imei());
  EXPECT_EQ(kAccessTechnologies, capability_->access_technologies_);
}

TEST_F(CellularCapabilityUniversalMainTest, StartModemFailure) {
  EXPECT_CALL(*modem_proxy_,
              Enable(true, _, _, CellularCapability::kTimeoutEnable))
      .WillOnce(Invoke(
           this, &CellularCapabilityUniversalTest::InvokeEnableFail));
  EXPECT_CALL(*properties_proxy_, GetAll(MM_DBUS_INTERFACE_MODEM)).Times(0);
  EXPECT_CALL(*properties_proxy_, GetAll(MM_DBUS_INTERFACE_MODEM_MODEM3GPP))
      .Times(0);

  Error error;
  EXPECT_CALL(*this, TestCallback(IsFailure()));
  ResultCallback callback =
      Bind(&CellularCapabilityUniversalTest::TestCallback, Unretained(this));
  capability_->StartModem(&error, callback);
  EXPECT_TRUE(error.IsOngoing());
}

TEST_F(CellularCapabilityUniversalMainTest, StartModemInWrongState) {
  ExpectModemAndModem3gppProperties();

  EXPECT_CALL(*modem_proxy_,
              Enable(true, _, _, CellularCapability::kTimeoutEnable))
      .WillOnce(Invoke(
           this, &CellularCapabilityUniversalTest::InvokeEnableInWrongState))
      .WillOnce(Invoke(
           this, &CellularCapabilityUniversalTest::InvokeEnable));

  Error error;
  EXPECT_CALL(*this, TestCallback(_)).Times(0);
  ResultCallback callback =
      Bind(&CellularCapabilityUniversalTest::TestCallback, Unretained(this));
  capability_->StartModem(&error, callback);
  EXPECT_TRUE(error.IsOngoing());

  // Verify that the modem has not been enabled.
  EXPECT_TRUE(cellular_->imei().empty());
  EXPECT_EQ(0, capability_->access_technologies_);
  Mock::VerifyAndClearExpectations(this);

  // Change the state to kModemStateEnabling and verify that it still has not
  // been enabled.
  capability_->OnModemStateChanged(Cellular::kModemStateEnabling);
  EXPECT_TRUE(cellular_->imei().empty());
  EXPECT_EQ(0, capability_->access_technologies_);
  Mock::VerifyAndClearExpectations(this);

  // Change the state to kModemStateDisabling and verify that it still has not
  // been enabled.
  EXPECT_CALL(*this, TestCallback(_)).Times(0);
  capability_->OnModemStateChanged(Cellular::kModemStateDisabling);
  EXPECT_TRUE(cellular_->imei().empty());
  EXPECT_EQ(0, capability_->access_technologies_);
  Mock::VerifyAndClearExpectations(this);

  // Change the state of the modem to disabled and verify that it gets enabled.
  EXPECT_CALL(*this, TestCallback(IsSuccess()));
  capability_->OnModemStateChanged(Cellular::kModemStateDisabled);
  EXPECT_EQ(kImei, cellular_->imei());
  EXPECT_EQ(kAccessTechnologies, capability_->access_technologies_);
}

TEST_F(CellularCapabilityUniversalMainTest,
       StartModemWithDeferredEnableFailure) {
  EXPECT_CALL(*modem_proxy_,
              Enable(true, _, _, CellularCapability::kTimeoutEnable))
      .Times(2)
      .WillRepeatedly(Invoke(
           this, &CellularCapabilityUniversalTest::InvokeEnableInWrongState));
  EXPECT_CALL(*properties_proxy_, GetAll(MM_DBUS_INTERFACE_MODEM)).Times(0);
  EXPECT_CALL(*properties_proxy_, GetAll(MM_DBUS_INTERFACE_MODEM_MODEM3GPP))
      .Times(0);

  Error error;
  EXPECT_CALL(*this, TestCallback(_)).Times(0);
  ResultCallback callback =
      Bind(&CellularCapabilityUniversalTest::TestCallback, Unretained(this));
  capability_->StartModem(&error, callback);
  EXPECT_TRUE(error.IsOngoing());
  Mock::VerifyAndClearExpectations(this);

  // Change the state of the modem to disabled but fail the deferred enable
  // operation with the WrongState error in order to verify that the deferred
  // enable operation does not trigger another deferred enable operation.
  EXPECT_CALL(*this, TestCallback(IsFailure()));
  capability_->OnModemStateChanged(Cellular::kModemStateDisabled);
}

TEST_F(CellularCapabilityUniversalMainTest, StopModem) {
  // Save pointers to proxies before they are lost by the call to InitProxies
  mm1::MockModemProxy* modem_proxy = modem_proxy_.get();
  EXPECT_CALL(*modem_proxy, set_state_changed_callback(_));
  capability_->InitProxies();

  Error error;
  ResultCallback callback =
      Bind(&CellularCapabilityUniversalTest::TestCallback, Unretained(this));
  capability_->StopModem(&error, callback);
  EXPECT_TRUE(error.IsSuccess());

  ResultCallback disable_callback;
  EXPECT_CALL(*modem_proxy,
              Enable(false, _, _, CellularCapability::kTimeoutEnable))
      .WillOnce(SaveArg<2>(&disable_callback));
  dispatcher_.DispatchPendingEvents();

  ResultCallback set_power_state_callback;
  EXPECT_CALL(
      *modem_proxy,
      SetPowerState(
          MM_MODEM_POWER_STATE_LOW, _, _,
          CellularCapabilityUniversal::kSetPowerStateTimeoutMilliseconds))
      .WillOnce(SaveArg<2>(&set_power_state_callback));
  disable_callback.Run(Error(Error::kSuccess));

  EXPECT_CALL(*this, TestCallback(IsSuccess()));
  set_power_state_callback.Run(Error(Error::kSuccess));
  Mock::VerifyAndClearExpectations(this);

  // TestCallback should get called with success even if the power state
  // callback gets called with an error
  EXPECT_CALL(*this, TestCallback(IsSuccess()));
  set_power_state_callback.Run(Error(Error::kOperationFailed));
}

TEST_F(CellularCapabilityUniversalMainTest, StopModemAltair) {
  // Save pointers to proxies before they are lost by the call to InitProxies
  mm1::MockModemProxy* modem_proxy = modem_proxy_.get();
  EXPECT_CALL(*modem_proxy, set_state_changed_callback(_));
  capability_->InitProxies();

  const char kBearerDBusPath[] = "/bearer/dbus/path";
  capability_->set_active_bearer(
      new CellularBearer(&control_interface_,
                         kBearerDBusPath,
                         cellular_->dbus_service()));  // Passes ownership.

  cellular_->set_mm_plugin(CellularCapabilityUniversal::kAltairLTEMMPlugin);

  Error error;
  ResultCallback callback =
      Bind(&CellularCapabilityUniversalTest::TestCallback, Unretained(this));
  capability_->StopModem(&error, callback);
  EXPECT_TRUE(error.IsSuccess());

  ResultCallback delete_bearer_callback;
  EXPECT_CALL(*modem_proxy,
              DeleteBearer(kBearerDBusPath, _, _,
                           CellularCapability::kTimeoutDefault))
      .WillOnce(SaveArg<2>(&delete_bearer_callback));
  dispatcher_.DispatchPendingEvents();

  ResultCallback disable_callback;
  EXPECT_CALL(*modem_proxy,
              Enable(false, _, _, CellularCapability::kTimeoutEnable))
      .WillOnce(SaveArg<2>(&disable_callback));
  delete_bearer_callback.Run(Error(Error::kSuccess));

  ResultCallback set_power_state_callback;
  EXPECT_CALL(
      *modem_proxy,
      SetPowerState(
          MM_MODEM_POWER_STATE_LOW, _, _,
          CellularCapabilityUniversal::kSetPowerStateTimeoutMilliseconds))
      .WillOnce(SaveArg<2>(&set_power_state_callback));
  disable_callback.Run(Error(Error::kSuccess));

  EXPECT_CALL(*this, TestCallback(IsSuccess()));
  set_power_state_callback.Run(Error(Error::kSuccess));
}

TEST_F(CellularCapabilityUniversalMainTest,
       StopModemAltairDeleteBearerFailure) {
  // Save pointers to proxies before they are lost by the call to InitProxies
  mm1::MockModemProxy* modem_proxy = modem_proxy_.get();
  EXPECT_CALL(*modem_proxy, set_state_changed_callback(_));
  capability_->InitProxies();

  const char kBearerDBusPath[] = "/bearer/dbus/path";
  capability_->set_active_bearer(
      new CellularBearer(&control_interface_,
                         kBearerDBusPath,
                         cellular_->dbus_service()));  // Passes ownership.

  cellular_->set_mm_plugin(CellularCapabilityUniversal::kAltairLTEMMPlugin);

  Error error;
  ResultCallback callback =
      Bind(&CellularCapabilityUniversalTest::TestCallback, Unretained(this));
  capability_->StopModem(&error, callback);
  EXPECT_TRUE(error.IsSuccess());

  ResultCallback delete_bearer_callback;
  EXPECT_CALL(*modem_proxy,
              DeleteBearer(kBearerDBusPath, _, _,
                           CellularCapability::kTimeoutDefault))
      .WillOnce(SaveArg<2>(&delete_bearer_callback));
  dispatcher_.DispatchPendingEvents();

  ResultCallback disable_callback;
  EXPECT_CALL(*modem_proxy,
              Enable(false, _, _, CellularCapability::kTimeoutEnable))
      .WillOnce(SaveArg<2>(&disable_callback));
  delete_bearer_callback.Run(Error(Error::kOperationFailed));

  ResultCallback set_power_state_callback;
  EXPECT_CALL(
      *modem_proxy,
      SetPowerState(
          MM_MODEM_POWER_STATE_LOW, _, _,
          CellularCapabilityUniversal::kSetPowerStateTimeoutMilliseconds))
      .WillOnce(SaveArg<2>(&set_power_state_callback));
  disable_callback.Run(Error(Error::kSuccess));

  EXPECT_CALL(*this, TestCallback(IsSuccess()));
  set_power_state_callback.Run(Error(Error::kSuccess));
}

TEST_F(CellularCapabilityUniversalMainTest, StopModemAltairNotConnected) {
  // Save pointers to proxies before they are lost by the call to InitProxies
  mm1::MockModemProxy* modem_proxy = modem_proxy_.get();
  EXPECT_CALL(*modem_proxy, set_state_changed_callback(_));
  capability_->InitProxies();
  capability_->set_active_bearer(nullptr);
  cellular_->set_mm_plugin(CellularCapabilityUniversal::kAltairLTEMMPlugin);

  Error error;
  ResultCallback callback =
      Bind(&CellularCapabilityUniversalTest::TestCallback, Unretained(this));
  capability_->StopModem(&error, callback);
  EXPECT_TRUE(error.IsSuccess());

  ResultCallback disable_callback;
  EXPECT_CALL(*modem_proxy,
              Enable(false, _, _, CellularCapability::kTimeoutEnable))
      .WillOnce(SaveArg<2>(&disable_callback));
  dispatcher_.DispatchPendingEvents();

  ResultCallback set_power_state_callback;
  EXPECT_CALL(
      *modem_proxy,
      SetPowerState(
          MM_MODEM_POWER_STATE_LOW, _, _,
          CellularCapabilityUniversal::kSetPowerStateTimeoutMilliseconds))
      .WillOnce(SaveArg<2>(&set_power_state_callback));
  disable_callback.Run(Error(Error::kSuccess));

  EXPECT_CALL(*this, TestCallback(IsSuccess()));
  set_power_state_callback.Run(Error(Error::kSuccess));
  Mock::VerifyAndClearExpectations(this);

  // TestCallback should get called with success even if the power state
  // callback gets called with an error
  EXPECT_CALL(*this, TestCallback(IsSuccess()));
  set_power_state_callback.Run(Error(Error::kOperationFailed));
}

TEST_F(CellularCapabilityUniversalMainTest, TerminationAction) {
  ExpectModemAndModem3gppProperties();

  {
    InSequence seq;

    EXPECT_CALL(*modem_proxy_,
                Enable(true, _, _, CellularCapability::kTimeoutEnable))
        .WillOnce(Invoke(this, &CellularCapabilityUniversalTest::InvokeEnable));
    EXPECT_CALL(*modem_proxy_,
                Enable(false, _, _, CellularCapability::kTimeoutEnable))
        .WillOnce(Invoke(this, &CellularCapabilityUniversalTest::InvokeEnable));
    EXPECT_CALL(
        *modem_proxy_,
        SetPowerState(
            MM_MODEM_POWER_STATE_LOW, _, _,
            CellularCapabilityUniversal::kSetPowerStateTimeoutMilliseconds))
        .WillOnce(Invoke(
             this, &CellularCapabilityUniversalTest::InvokeSetPowerState));
  }
  EXPECT_CALL(*this, TestCallback(IsSuccess())).Times(2);

  EXPECT_EQ(Cellular::kStateDisabled, cellular_->state());
  EXPECT_EQ(Cellular::kModemStateUnknown, cellular_->modem_state());
  EXPECT_TRUE(modem_info_.manager()->termination_actions_.IsEmpty());

  // Here we mimic the modem state change from ModemManager. When the modem is
  // enabled, a termination action should be added.
  cellular_->OnModemStateChanged(Cellular::kModemStateEnabled);
  dispatcher_.DispatchPendingEvents();
  EXPECT_EQ(Cellular::kStateEnabled, cellular_->state());
  EXPECT_EQ(Cellular::kModemStateEnabled, cellular_->modem_state());
  EXPECT_FALSE(modem_info_.manager()->termination_actions_.IsEmpty());

  // Running the termination action should disable the modem.
  modem_info_.manager()->RunTerminationActions(Bind(
      &CellularCapabilityUniversalMainTest::TestCallback, Unretained(this)));
  dispatcher_.DispatchPendingEvents();
  // Here we mimic the modem state change from ModemManager. When the modem is
  // disabled, the termination action should be removed.
  cellular_->OnModemStateChanged(Cellular::kModemStateDisabled);
  dispatcher_.DispatchPendingEvents();
  EXPECT_EQ(Cellular::kStateDisabled, cellular_->state());
  EXPECT_EQ(Cellular::kModemStateDisabled, cellular_->modem_state());
  EXPECT_TRUE(modem_info_.manager()->termination_actions_.IsEmpty());

  // No termination action should be called here.
  modem_info_.manager()->RunTerminationActions(Bind(
      &CellularCapabilityUniversalMainTest::TestCallback, Unretained(this)));
  dispatcher_.DispatchPendingEvents();
}

TEST_F(CellularCapabilityUniversalMainTest,
       TerminationActionRemovedByStopModem) {
  ExpectModemAndModem3gppProperties();

  {
    InSequence seq;

    EXPECT_CALL(*modem_proxy_,
                Enable(true, _, _, CellularCapability::kTimeoutEnable))
        .WillOnce(Invoke(this, &CellularCapabilityUniversalTest::InvokeEnable));
    EXPECT_CALL(*modem_proxy_,
                Enable(false, _, _, CellularCapability::kTimeoutEnable))
        .WillOnce(Invoke(this, &CellularCapabilityUniversalTest::InvokeEnable));
    EXPECT_CALL(
        *modem_proxy_,
        SetPowerState(
            MM_MODEM_POWER_STATE_LOW, _, _,
            CellularCapabilityUniversal::kSetPowerStateTimeoutMilliseconds))
        .WillOnce(Invoke(
             this, &CellularCapabilityUniversalTest::InvokeSetPowerState));
  }
  EXPECT_CALL(*this, TestCallback(IsSuccess())).Times(1);

  EXPECT_EQ(Cellular::kStateDisabled, cellular_->state());
  EXPECT_EQ(Cellular::kModemStateUnknown, cellular_->modem_state());
  EXPECT_TRUE(modem_info_.manager()->termination_actions_.IsEmpty());

  // Here we mimic the modem state change from ModemManager. When the modem is
  // enabled, a termination action should be added.
  cellular_->OnModemStateChanged(Cellular::kModemStateEnabled);
  dispatcher_.DispatchPendingEvents();
  EXPECT_EQ(Cellular::kStateEnabled, cellular_->state());
  EXPECT_EQ(Cellular::kModemStateEnabled, cellular_->modem_state());
  EXPECT_FALSE(modem_info_.manager()->termination_actions_.IsEmpty());

  // Verify that the termination action is removed when the modem is disabled
  // not due to a suspend request.
  cellular_->SetEnabled(false);
  dispatcher_.DispatchPendingEvents();
  EXPECT_EQ(Cellular::kStateDisabled, cellular_->state());
  EXPECT_TRUE(modem_info_.manager()->termination_actions_.IsEmpty());

  // No termination action should be called here.
  modem_info_.manager()->RunTerminationActions(Bind(
      &CellularCapabilityUniversalMainTest::TestCallback, Unretained(this)));
  dispatcher_.DispatchPendingEvents();
}

TEST_F(CellularCapabilityUniversalMainTest, DisconnectModemNoBearer) {
  Error error;
  ResultCallback disconnect_callback;
  EXPECT_CALL(*modem_simple_proxy_,
              Disconnect(_, _, _, CellularCapability::kTimeoutDisconnect))
      .Times(0);
  capability_->Disconnect(&error, disconnect_callback);
}

TEST_F(CellularCapabilityUniversalMainTest, DisconnectNoProxy) {
  Error error;
  ResultCallback disconnect_callback;
  EXPECT_CALL(*modem_simple_proxy_,
              Disconnect(_, _, _, CellularCapability::kTimeoutDisconnect))
      .Times(0);
  ReleaseCapabilityProxies();
  capability_->Disconnect(&error, disconnect_callback);
}

TEST_F(CellularCapabilityUniversalMainTest, SimLockStatusChanged) {
  // Set up mock SIM properties
  const char kImsi[] = "310100000001";
  const char kSimIdentifier[] = "9999888";
  const char kOperatorIdentifier[] = "310240";
  const char kOperatorName[] = "Custom SPN";
  KeyValueStore sim_properties;
  sim_properties.SetString(MM_SIM_PROPERTY_IMSI, kImsi);
  sim_properties.SetString(MM_SIM_PROPERTY_SIMIDENTIFIER, kSimIdentifier);
  sim_properties.SetString(MM_SIM_PROPERTY_OPERATORIDENTIFIER,
                           kOperatorIdentifier);
  sim_properties.SetString(MM_SIM_PROPERTY_OPERATORNAME, kOperatorName);

  EXPECT_CALL(*properties_proxy_, GetAll(MM_DBUS_INTERFACE_SIM))
      .WillOnce(Return(sim_properties));
  EXPECT_CALL(*modem_info_.mock_pending_activation_store(),
              GetActivationState(PendingActivationStore::kIdentifierICCID, _))
      .Times(1);

  EXPECT_FALSE(cellular_->sim_present());
  EXPECT_EQ(nullptr, capability_->sim_proxy_);;

  capability_->OnSimPathChanged(kSimPath);
  EXPECT_TRUE(cellular_->sim_present());
  EXPECT_NE(nullptr, capability_->sim_proxy_);;
  EXPECT_EQ(kSimPath, capability_->sim_path_);

  cellular_->set_imsi("");
  cellular_->set_sim_identifier("");
  capability_->spn_ = "";

  // SIM is locked.
  capability_->sim_lock_status_.lock_type = MM_MODEM_LOCK_SIM_PIN;
  capability_->OnSimLockStatusChanged();
  Mock::VerifyAndClearExpectations(modem_info_.mock_pending_activation_store());

  EXPECT_EQ("", cellular_->imsi());
  EXPECT_EQ("", cellular_->sim_identifier());
  EXPECT_EQ("", capability_->spn_);

  // SIM is unlocked.
  properties_proxy_.reset(new MockDBusPropertiesProxy());
  EXPECT_CALL(*properties_proxy_, GetAll(MM_DBUS_INTERFACE_SIM))
      .WillOnce(Return(sim_properties));
  EXPECT_CALL(*modem_info_.mock_pending_activation_store(),
              GetActivationState(PendingActivationStore::kIdentifierICCID, _))
      .Times(1);

  capability_->sim_lock_status_.lock_type = MM_MODEM_LOCK_NONE;
  capability_->OnSimLockStatusChanged();
  Mock::VerifyAndClearExpectations(modem_info_.mock_pending_activation_store());

  EXPECT_EQ(kImsi, cellular_->imsi());
  EXPECT_EQ(kSimIdentifier, cellular_->sim_identifier());
  EXPECT_EQ(kOperatorName, capability_->spn_);

  // SIM is missing and SIM path is "/".
  capability_->OnSimPathChanged(CellularCapabilityUniversal::kRootPath);
  EXPECT_FALSE(cellular_->sim_present());
  EXPECT_EQ(nullptr, capability_->sim_proxy_);;
  EXPECT_EQ(CellularCapabilityUniversal::kRootPath, capability_->sim_path_);

  EXPECT_CALL(*modem_info_.mock_pending_activation_store(),
              GetActivationState(_, _)).Times(0);
  capability_->OnSimLockStatusChanged();
  Mock::VerifyAndClearExpectations(modem_info_.mock_pending_activation_store());

  EXPECT_EQ("", cellular_->imsi());
  EXPECT_EQ("", cellular_->sim_identifier());
  EXPECT_EQ("", capability_->spn_);

  // SIM is missing and SIM path is empty.
  capability_->OnSimPathChanged("");
  EXPECT_FALSE(cellular_->sim_present());
  EXPECT_EQ(nullptr, capability_->sim_proxy_);;
  EXPECT_EQ("", capability_->sim_path_);

  EXPECT_CALL(*modem_info_.mock_pending_activation_store(),
              GetActivationState(_, _)).Times(0);
  capability_->OnSimLockStatusChanged();
  Mock::VerifyAndClearExpectations(modem_info_.mock_pending_activation_store());

  EXPECT_EQ("", cellular_->imsi());
  EXPECT_EQ("", cellular_->sim_identifier());
  EXPECT_EQ("", capability_->spn_);
}

TEST_F(CellularCapabilityUniversalMainTest, PropertiesChanged) {
  // Set up mock modem properties
  KeyValueStore modem_properties;
  modem_properties.SetUint(MM_MODEM_PROPERTY_ACCESSTECHNOLOGIES,
                           kAccessTechnologies);
  modem_properties.SetRpcIdentifier(MM_MODEM_PROPERTY_SIM, kSimPath);

  // Set up mock modem 3gpp properties
  KeyValueStore modem3gpp_properties;
  modem3gpp_properties.SetUint(MM_MODEM_MODEM3GPP_PROPERTY_ENABLEDFACILITYLOCKS,
                               0);
  modem3gpp_properties.SetString(MM_MODEM_MODEM3GPP_PROPERTY_IMEI, kImei);

  // Set up mock modem sim properties
  KeyValueStore sim_properties;

  EXPECT_CALL(*properties_proxy_,
              GetAll(MM_DBUS_INTERFACE_SIM))
      .WillOnce(Return(sim_properties));

  EXPECT_EQ("", cellular_->imei());
  EXPECT_EQ(MM_MODEM_ACCESS_TECHNOLOGY_UNKNOWN,
            capability_->access_technologies_);
  EXPECT_FALSE(capability_->sim_proxy_.get());
  EXPECT_CALL(*device_adaptor_, EmitStringChanged(
      kTechnologyFamilyProperty, kTechnologyFamilyGsm));
  EXPECT_CALL(*device_adaptor_, EmitStringChanged(kImeiProperty, kImei));
  capability_->OnPropertiesChanged(MM_DBUS_INTERFACE_MODEM,
                                   modem_properties, vector<string>());
  EXPECT_EQ(kAccessTechnologies, capability_->access_technologies_);
  EXPECT_EQ(kSimPath, capability_->sim_path_);
  EXPECT_TRUE(capability_->sim_proxy_.get());

  // Changing properties on wrong interface will not have an effect
  capability_->OnPropertiesChanged(MM_DBUS_INTERFACE_MODEM,
                                   modem3gpp_properties,
                                   vector<string>());
  EXPECT_EQ("", cellular_->imei());

  // Changing properties on the right interface gets reflected in the
  // capabilities object
  capability_->OnPropertiesChanged(MM_DBUS_INTERFACE_MODEM_MODEM3GPP,
                                   modem3gpp_properties,
                                   vector<string>());
  EXPECT_EQ(kImei, cellular_->imei());
  Mock::VerifyAndClearExpectations(device_adaptor_);

  // Expect to see changes when the family changes
  modem_properties.Clear();
  modem_properties.SetUint(MM_MODEM_PROPERTY_ACCESSTECHNOLOGIES,
                           MM_MODEM_ACCESS_TECHNOLOGY_1XRTT);
  EXPECT_CALL(*device_adaptor_, EmitStringChanged(
      kTechnologyFamilyProperty, kTechnologyFamilyCdma)).
      Times(1);
  capability_->OnPropertiesChanged(MM_DBUS_INTERFACE_MODEM,
                                       modem_properties,
                                       vector<string>());
  Mock::VerifyAndClearExpectations(device_adaptor_);

  // Back to LTE
  modem_properties.Clear();
  modem_properties.SetUint(MM_MODEM_PROPERTY_ACCESSTECHNOLOGIES,
                           MM_MODEM_ACCESS_TECHNOLOGY_LTE);
  EXPECT_CALL(*device_adaptor_, EmitStringChanged(
      kTechnologyFamilyProperty, kTechnologyFamilyGsm)).
      Times(1);
  capability_->OnPropertiesChanged(MM_DBUS_INTERFACE_MODEM,
                                   modem_properties,
                                   vector<string>());
  Mock::VerifyAndClearExpectations(device_adaptor_);

  // LTE & CDMA - the device adaptor should not be called!
  modem_properties.Clear();
  modem_properties.SetUint(MM_MODEM_PROPERTY_ACCESSTECHNOLOGIES,
                           MM_MODEM_ACCESS_TECHNOLOGY_LTE |
                           MM_MODEM_ACCESS_TECHNOLOGY_1XRTT);
  EXPECT_CALL(*device_adaptor_, EmitStringChanged(_, _)).Times(0);
  capability_->OnPropertiesChanged(MM_DBUS_INTERFACE_MODEM,
                                   modem_properties,
                                   vector<string>());
}

TEST_F(CellularCapabilityUniversalMainTest, UpdateRegistrationState) {
  capability_->InitProxies();

  CreateService();
  cellular_->set_imsi("310240123456789");
  cellular_->set_modem_state(Cellular::kModemStateConnected);
  SetRegistrationDroppedUpdateTimeout(0);

  const Stringmap& home_provider_map = cellular_->home_provider();
  ASSERT_NE(home_provider_map.end(), home_provider_map.find(kOperatorNameKey));
  string home_provider = home_provider_map.find(kOperatorNameKey)->second;
  string ota_name = cellular_->service_->friendly_name();

  // Home --> Roaming should be effective immediately.
  capability_->On3GPPRegistrationChanged(
      MM_MODEM_3GPP_REGISTRATION_STATE_HOME,
      home_provider,
      ota_name);
  EXPECT_EQ(MM_MODEM_3GPP_REGISTRATION_STATE_HOME,
            capability_->registration_state_);
  capability_->On3GPPRegistrationChanged(
      MM_MODEM_3GPP_REGISTRATION_STATE_ROAMING,
      home_provider,
      ota_name);
  EXPECT_EQ(MM_MODEM_3GPP_REGISTRATION_STATE_ROAMING,
            capability_->registration_state_);

  // Idle --> Roaming should be effective immediately.
  capability_->On3GPPRegistrationChanged(
      MM_MODEM_3GPP_REGISTRATION_STATE_IDLE,
      home_provider,
      ota_name);
  dispatcher_.DispatchPendingEvents();
  EXPECT_EQ(MM_MODEM_3GPP_REGISTRATION_STATE_IDLE,
            capability_->registration_state_);
  capability_->On3GPPRegistrationChanged(
      MM_MODEM_3GPP_REGISTRATION_STATE_ROAMING,
      home_provider,
      ota_name);
  EXPECT_EQ(MM_MODEM_3GPP_REGISTRATION_STATE_ROAMING,
            capability_->registration_state_);

  // Idle --> Searching should be effective immediately.
  capability_->On3GPPRegistrationChanged(
      MM_MODEM_3GPP_REGISTRATION_STATE_IDLE,
      home_provider,
      ota_name);
  dispatcher_.DispatchPendingEvents();
  EXPECT_EQ(MM_MODEM_3GPP_REGISTRATION_STATE_IDLE,
            capability_->registration_state_);
  capability_->On3GPPRegistrationChanged(
      MM_MODEM_3GPP_REGISTRATION_STATE_SEARCHING,
      home_provider,
      ota_name);
  EXPECT_EQ(MM_MODEM_3GPP_REGISTRATION_STATE_SEARCHING,
            capability_->registration_state_);

  // Home --> Searching --> Home should never see Searching.
  EXPECT_CALL(*(modem_info_.mock_metrics()),
      Notify3GPPRegistrationDelayedDropPosted());
  EXPECT_CALL(*(modem_info_.mock_metrics()),
      Notify3GPPRegistrationDelayedDropCanceled());

  capability_->On3GPPRegistrationChanged(
      MM_MODEM_3GPP_REGISTRATION_STATE_HOME,
      home_provider,
      ota_name);
  EXPECT_EQ(MM_MODEM_3GPP_REGISTRATION_STATE_HOME,
            capability_->registration_state_);
  capability_->On3GPPRegistrationChanged(
      MM_MODEM_3GPP_REGISTRATION_STATE_SEARCHING,
      home_provider,
      ota_name);
  EXPECT_EQ(MM_MODEM_3GPP_REGISTRATION_STATE_HOME,
            capability_->registration_state_);
  capability_->On3GPPRegistrationChanged(
      MM_MODEM_3GPP_REGISTRATION_STATE_HOME,
      home_provider,
      ota_name);
  EXPECT_EQ(MM_MODEM_3GPP_REGISTRATION_STATE_HOME,
            capability_->registration_state_);
  dispatcher_.DispatchPendingEvents();
  EXPECT_EQ(MM_MODEM_3GPP_REGISTRATION_STATE_HOME,
            capability_->registration_state_);
  Mock::VerifyAndClearExpectations(modem_info_.mock_metrics());

  // Home --> Searching --> wait till dispatch should see Searching
  EXPECT_CALL(*(modem_info_.mock_metrics()),
      Notify3GPPRegistrationDelayedDropPosted());
  capability_->On3GPPRegistrationChanged(
      MM_MODEM_3GPP_REGISTRATION_STATE_HOME,
      home_provider,
      ota_name);
  EXPECT_EQ(MM_MODEM_3GPP_REGISTRATION_STATE_HOME,
            capability_->registration_state_);
  capability_->On3GPPRegistrationChanged(
      MM_MODEM_3GPP_REGISTRATION_STATE_SEARCHING,
      home_provider,
      ota_name);
  EXPECT_EQ(MM_MODEM_3GPP_REGISTRATION_STATE_HOME,
            capability_->registration_state_);
  dispatcher_.DispatchPendingEvents();
  EXPECT_EQ(MM_MODEM_3GPP_REGISTRATION_STATE_SEARCHING,
            capability_->registration_state_);
  Mock::VerifyAndClearExpectations(modem_info_.mock_metrics());

  // Home --> Searching --> Searching --> wait till dispatch should see
  // Searching *and* the first callback should be cancelled.
  EXPECT_CALL(*this, DummyCallback()).Times(0);
  EXPECT_CALL(*(modem_info_.mock_metrics()),
      Notify3GPPRegistrationDelayedDropPosted());

  capability_->On3GPPRegistrationChanged(
      MM_MODEM_3GPP_REGISTRATION_STATE_HOME,
      home_provider,
      ota_name);
  EXPECT_EQ(MM_MODEM_3GPP_REGISTRATION_STATE_HOME,
            capability_->registration_state_);
  capability_->On3GPPRegistrationChanged(
      MM_MODEM_3GPP_REGISTRATION_STATE_SEARCHING,
      home_provider,
      ota_name);
  SetMockRegistrationDroppedUpdateCallback();
  capability_->On3GPPRegistrationChanged(
      MM_MODEM_3GPP_REGISTRATION_STATE_SEARCHING,
      home_provider,
      ota_name);
  EXPECT_EQ(MM_MODEM_3GPP_REGISTRATION_STATE_HOME,
            capability_->registration_state_);
  dispatcher_.DispatchPendingEvents();
  EXPECT_EQ(MM_MODEM_3GPP_REGISTRATION_STATE_SEARCHING,
            capability_->registration_state_);
}

TEST_F(CellularCapabilityUniversalMainTest, IsRegistered) {
  capability_->registration_state_ = MM_MODEM_3GPP_REGISTRATION_STATE_IDLE;
  EXPECT_FALSE(capability_->IsRegistered());

  capability_->registration_state_ = MM_MODEM_3GPP_REGISTRATION_STATE_HOME;
  EXPECT_TRUE(capability_->IsRegistered());

  capability_->registration_state_ = MM_MODEM_3GPP_REGISTRATION_STATE_SEARCHING;
  EXPECT_FALSE(capability_->IsRegistered());

  capability_->registration_state_ = MM_MODEM_3GPP_REGISTRATION_STATE_DENIED;
  EXPECT_FALSE(capability_->IsRegistered());

  capability_->registration_state_ = MM_MODEM_3GPP_REGISTRATION_STATE_UNKNOWN;
  EXPECT_FALSE(capability_->IsRegistered());

  capability_->registration_state_ = MM_MODEM_3GPP_REGISTRATION_STATE_ROAMING;
  EXPECT_TRUE(capability_->IsRegistered());
}

TEST_F(CellularCapabilityUniversalMainTest,
       UpdateRegistrationStateModemNotConnected) {
  capability_->InitProxies();
  CreateService();

  cellular_->set_imsi("310240123456789");
  cellular_->set_modem_state(Cellular::kModemStateRegistered);
  SetRegistrationDroppedUpdateTimeout(0);

  const Stringmap& home_provider_map = cellular_->home_provider();
  ASSERT_NE(home_provider_map.end(), home_provider_map.find(kOperatorNameKey));
  string home_provider = home_provider_map.find(kOperatorNameKey)->second;
  string ota_name = cellular_->service_->friendly_name();

  // Home --> Searching should be effective immediately.
  capability_->On3GPPRegistrationChanged(
      MM_MODEM_3GPP_REGISTRATION_STATE_HOME,
      home_provider,
      ota_name);
  EXPECT_EQ(MM_MODEM_3GPP_REGISTRATION_STATE_HOME,
            capability_->registration_state_);
  capability_->On3GPPRegistrationChanged(
      MM_MODEM_3GPP_REGISTRATION_STATE_SEARCHING,
      home_provider,
      ota_name);
  EXPECT_EQ(MM_MODEM_3GPP_REGISTRATION_STATE_SEARCHING,
            capability_->registration_state_);
}

TEST_F(CellularCapabilityUniversalMainTest, IsValidSimPath) {
  // Invalid paths
  EXPECT_FALSE(capability_->IsValidSimPath(""));
  EXPECT_FALSE(capability_->IsValidSimPath("/"));

  // A valid path
  EXPECT_TRUE(capability_->IsValidSimPath(
      "/org/freedesktop/ModemManager1/SIM/0"));

  // Note that any string that is not one of the above invalid paths is
  // currently regarded as valid, since the ModemManager spec doesn't impose
  // a strict format on the path. The validity of this is subject to change.
  EXPECT_TRUE(capability_->IsValidSimPath("path"));
}

TEST_F(CellularCapabilityUniversalMainTest, NormalizeMdn) {
  EXPECT_EQ("", capability_->NormalizeMdn(""));
  EXPECT_EQ("12345678901", capability_->NormalizeMdn("12345678901"));
  EXPECT_EQ("12345678901", capability_->NormalizeMdn("+1 234 567 8901"));
  EXPECT_EQ("12345678901", capability_->NormalizeMdn("+1-234-567-8901"));
  EXPECT_EQ("12345678901", capability_->NormalizeMdn("+1 (234) 567-8901"));
  EXPECT_EQ("12345678901", capability_->NormalizeMdn("1 234  567 8901 "));
  EXPECT_EQ("2345678901", capability_->NormalizeMdn("(234) 567-8901"));
}

TEST_F(CellularCapabilityUniversalMainTest, SimPathChanged) {
  // Set up mock modem SIM properties
  const char kImsi[] = "310100000001";
  const char kSimIdentifier[] = "9999888";
  const char kOperatorIdentifier[] = "310240";
  const char kOperatorName[] = "Custom SPN";
  KeyValueStore sim_properties;
  sim_properties.SetString(MM_SIM_PROPERTY_IMSI, kImsi);
  sim_properties.SetString(MM_SIM_PROPERTY_SIMIDENTIFIER, kSimIdentifier);
  sim_properties.SetString(MM_SIM_PROPERTY_OPERATORIDENTIFIER,
                           kOperatorIdentifier);
  sim_properties.SetString(MM_SIM_PROPERTY_OPERATORNAME, kOperatorName);

  EXPECT_CALL(*properties_proxy_, GetAll(MM_DBUS_INTERFACE_SIM))
      .Times(1).WillOnce(Return(sim_properties));
  EXPECT_CALL(*modem_info_.mock_pending_activation_store(),
              GetActivationState(PendingActivationStore::kIdentifierICCID, _))
      .Times(1);

  EXPECT_FALSE(cellular_->sim_present());
  EXPECT_EQ(nullptr, capability_->sim_proxy_);;
  EXPECT_EQ("", capability_->sim_path_);
  EXPECT_EQ("", cellular_->imsi());
  EXPECT_EQ("", cellular_->sim_identifier());
  EXPECT_EQ("", capability_->spn_);

  capability_->OnSimPathChanged(kSimPath);
  EXPECT_TRUE(cellular_->sim_present());
  EXPECT_NE(nullptr, capability_->sim_proxy_);;
  EXPECT_EQ(kSimPath, capability_->sim_path_);
  EXPECT_EQ(kImsi, cellular_->imsi());
  EXPECT_EQ(kSimIdentifier, cellular_->sim_identifier());
  EXPECT_EQ(kOperatorName, capability_->spn_);

  // Changing to the same SIM path should be a no-op.
  capability_->OnSimPathChanged(kSimPath);
  EXPECT_TRUE(cellular_->sim_present());
  EXPECT_NE(nullptr, capability_->sim_proxy_);;
  EXPECT_EQ(kSimPath, capability_->sim_path_);
  EXPECT_EQ(kImsi, cellular_->imsi());
  EXPECT_EQ(kSimIdentifier, cellular_->sim_identifier());
  EXPECT_EQ(kOperatorName, capability_->spn_);

  capability_->OnSimPathChanged("");
  Mock::VerifyAndClearExpectations(modem_info_.mock_pending_activation_store());
  Mock::VerifyAndClearExpectations(properties_proxy_.get());
  EXPECT_FALSE(cellular_->sim_present());
  EXPECT_EQ(nullptr, capability_->sim_proxy_);;
  EXPECT_EQ("", capability_->sim_path_);
  EXPECT_EQ("", cellular_->imsi());
  EXPECT_EQ("", cellular_->sim_identifier());
  EXPECT_EQ("", capability_->spn_);

  EXPECT_CALL(*properties_proxy_, GetAll(MM_DBUS_INTERFACE_SIM))
      .Times(1).WillOnce(Return(sim_properties));
  EXPECT_CALL(*modem_info_.mock_pending_activation_store(),
              GetActivationState(PendingActivationStore::kIdentifierICCID, _))
      .Times(1);

  capability_->OnSimPathChanged(kSimPath);
  EXPECT_TRUE(cellular_->sim_present());
  EXPECT_NE(nullptr, capability_->sim_proxy_);;
  EXPECT_EQ(kSimPath, capability_->sim_path_);
  EXPECT_EQ(kImsi, cellular_->imsi());
  EXPECT_EQ(kSimIdentifier, cellular_->sim_identifier());
  EXPECT_EQ(kOperatorName, capability_->spn_);

  capability_->OnSimPathChanged("/");
  EXPECT_FALSE(cellular_->sim_present());
  EXPECT_EQ(nullptr, capability_->sim_proxy_);;
  EXPECT_EQ("/", capability_->sim_path_);
  EXPECT_EQ("", cellular_->imsi());
  EXPECT_EQ("", cellular_->sim_identifier());
  EXPECT_EQ("", capability_->spn_);
}

TEST_F(CellularCapabilityUniversalMainTest, SimPropertiesChanged) {
  // Set up mock modem properties
  KeyValueStore modem_properties;
  modem_properties.SetRpcIdentifier(MM_MODEM_PROPERTY_SIM, kSimPath);

  // Set up mock modem sim properties
  const char kImsi[] = "310100000001";
  KeyValueStore sim_properties;
  sim_properties.SetString(MM_SIM_PROPERTY_IMSI, kImsi);

  EXPECT_CALL(*properties_proxy_, GetAll(MM_DBUS_INTERFACE_SIM))
      .WillOnce(Return(sim_properties));
  EXPECT_CALL(*modem_info_.mock_pending_activation_store(),
              GetActivationState(PendingActivationStore::kIdentifierICCID, _))
      .Times(0);

  EXPECT_FALSE(capability_->sim_proxy_.get());
  capability_->OnPropertiesChanged(MM_DBUS_INTERFACE_MODEM,
                                   modem_properties, vector<string>());
  EXPECT_EQ(kSimPath, capability_->sim_path_);
  EXPECT_TRUE(capability_->sim_proxy_.get());
  EXPECT_EQ(kImsi, cellular_->imsi());
  Mock::VerifyAndClearExpectations(modem_info_.mock_pending_activation_store());

  // Updating the SIM
  KeyValueStore new_properties;
  const char kNewImsi[] = "310240123456789";
  const char kSimIdentifier[] = "9999888";
  const char kOperatorIdentifier[] = "310240";
  const char kOperatorName[] = "Custom SPN";
  EXPECT_CALL(*modem_info_.mock_pending_activation_store(),
              GetActivationState(PendingActivationStore::kIdentifierICCID, _))
      .Times(2);
  EXPECT_CALL(*mock_home_provider_info_, UpdateIMSI(kNewImsi)).Times(2);
  new_properties.SetString(MM_SIM_PROPERTY_IMSI, kNewImsi);
  new_properties.SetString(MM_SIM_PROPERTY_SIMIDENTIFIER, kSimIdentifier);
  new_properties.SetString(MM_SIM_PROPERTY_OPERATORIDENTIFIER,
                           kOperatorIdentifier);
  capability_->OnPropertiesChanged(MM_DBUS_INTERFACE_SIM,
                                   new_properties,
                                   vector<string>());
  EXPECT_EQ(kNewImsi, cellular_->imsi());
  EXPECT_EQ(kSimIdentifier, cellular_->sim_identifier());
  EXPECT_EQ("", capability_->spn_);

  new_properties.SetString(MM_SIM_PROPERTY_OPERATORNAME, kOperatorName);
  capability_->OnPropertiesChanged(MM_DBUS_INTERFACE_SIM,
                                   new_properties,
                                   vector<string>());
  EXPECT_EQ(kOperatorName, capability_->spn_);
}

MATCHER_P(SizeIs, value, "") {
  return static_cast<size_t>(value) == arg.size();
}

TEST_F(CellularCapabilityUniversalMainTest, Reset) {
  // Save pointers to proxies before they are lost by the call to InitProxies
  mm1::MockModemProxy* modem_proxy = modem_proxy_.get();
  EXPECT_CALL(*modem_proxy, set_state_changed_callback(_));
  capability_->InitProxies();

  Error error;
  ResultCallback reset_callback;

  EXPECT_CALL(*modem_proxy, Reset(_, _, CellularCapability::kTimeoutReset))
      .WillOnce(SaveArg<1>(&reset_callback));

  capability_->Reset(&error, ResultCallback());
  EXPECT_TRUE(capability_->resetting_);
  reset_callback.Run(error);
  EXPECT_FALSE(capability_->resetting_);
}

TEST_F(CellularCapabilityUniversalMainTest, UpdateActiveBearer) {
  // Common resources.
  const size_t kPathCount = 3;
  string active_paths[kPathCount], inactive_paths[kPathCount];
  for (size_t i = 0; i < kPathCount; ++i) {
    active_paths[i] = base::StringPrintf("%s/%zu", kActiveBearerPathPrefix, i);
    inactive_paths[i] =
        base::StringPrintf("%s/%zu", kInactiveBearerPathPrefix, i);
  }

  EXPECT_EQ(nullptr, capability_->GetActiveBearer());;

  // Check that |active_bearer_| is set correctly when an active bearer is
  // returned.
  capability_->OnBearersChanged({inactive_paths[0],
                                 inactive_paths[1],
                                 active_paths[2],
                                 inactive_paths[1],
                                 inactive_paths[2]});
  capability_->UpdateActiveBearer();
  ASSERT_NE(nullptr, capability_->GetActiveBearer());;
  EXPECT_EQ(active_paths[2], capability_->GetActiveBearer()->dbus_path());

  // Check that |active_bearer_| is nullptr if no active bearers are returned.
  capability_->OnBearersChanged({inactive_paths[0],
                                 inactive_paths[1],
                                 inactive_paths[2],
                                 inactive_paths[1]});
  capability_->UpdateActiveBearer();
  EXPECT_EQ(nullptr, capability_->GetActiveBearer());;

  // Check that returning multiple bearers causes death.
  capability_->OnBearersChanged({active_paths[0],
                                 inactive_paths[1],
                                 inactive_paths[2],
                                 active_paths[1],
                                 inactive_paths[1]});
  EXPECT_DEATH(capability_->UpdateActiveBearer(),
               "Found more than one active bearer.");

  capability_->OnBearersChanged({});
  capability_->UpdateActiveBearer();
  EXPECT_EQ(nullptr, capability_->GetActiveBearer());;
}

// Validates expected behavior of Connect function
TEST_F(CellularCapabilityUniversalMainTest, Connect) {
  mm1::MockModemSimpleProxy* modem_simple_proxy = modem_simple_proxy_.get();
  SetSimpleProxy();
  Error error;
  KeyValueStore properties;
  capability_->apn_try_list_.clear();
  ResultCallback callback =
      Bind(&CellularCapabilityUniversalTest::TestCallback, Unretained(this));
  string bearer("/foo");

  // Test connect failures
  EXPECT_CALL(*modem_simple_proxy, Connect(_, _, _, _))
      .WillRepeatedly(SaveArg<2>(&connect_callback_));
  capability_->Connect(properties, &error, callback);
  EXPECT_TRUE(error.IsSuccess());
  EXPECT_CALL(*this, TestCallback(IsFailure()));
  EXPECT_CALL(*service_, ClearLastGoodApn());
  connect_callback_.Run(bearer, Error(Error::kOperationFailed));
  Mock::VerifyAndClearExpectations(this);

  // Test connect success
  capability_->Connect(properties, &error, callback);
  EXPECT_TRUE(error.IsSuccess());
  EXPECT_CALL(*this, TestCallback(IsSuccess()));
  connect_callback_.Run(bearer, Error(Error::kSuccess));
  Mock::VerifyAndClearExpectations(this);

  // Test connect failures without a service.  Make sure that shill
  // does not crash if the connect failed and there is no
  // CellularService object.  This can happen if the modem is enabled
  // and then quickly disabled.
  cellular_->service_ = nullptr;
  EXPECT_FALSE(capability_->cellular()->service());
  capability_->Connect(properties, &error, callback);
  EXPECT_TRUE(error.IsSuccess());
  EXPECT_CALL(*this, TestCallback(IsFailure()));
  connect_callback_.Run(bearer, Error(Error::kOperationFailed));
}

// Validates Connect iterates over APNs
TEST_F(CellularCapabilityUniversalMainTest, ConnectApns) {
  mm1::MockModemSimpleProxy* modem_simple_proxy = modem_simple_proxy_.get();
  SetSimpleProxy();
  Error error;
  KeyValueStore properties;
  capability_->apn_try_list_.clear();
  ResultCallback callback =
      Bind(&CellularCapabilityUniversalTest::TestCallback, Unretained(this));
  string bearer("/bearer0");

  const char apn_name_foo[] = "foo";
  const char apn_name_bar[] = "bar";
  EXPECT_CALL(*modem_simple_proxy, Connect(HasApn(apn_name_foo), _, _, _))
      .WillOnce(SaveArg<2>(&connect_callback_));
  Stringmap apn1;
  apn1[kApnProperty] = apn_name_foo;
  capability_->apn_try_list_.push_back(apn1);
  Stringmap apn2;
  apn2[kApnProperty] = apn_name_bar;
  capability_->apn_try_list_.push_back(apn2);
  capability_->FillConnectPropertyMap(&properties);
  capability_->Connect(properties, &error, callback);
  EXPECT_TRUE(error.IsSuccess());
  Mock::VerifyAndClearExpectations(modem_simple_proxy);

  EXPECT_CALL(*modem_simple_proxy, Connect(HasApn(apn_name_bar), _, _, _))
      .WillOnce(SaveArg<2>(&connect_callback_));
  EXPECT_CALL(*service_, ClearLastGoodApn());
  connect_callback_.Run(bearer, Error(Error::kInvalidApn));

  EXPECT_CALL(*service_, SetLastGoodApn(apn2));
  EXPECT_CALL(*this, TestCallback(IsSuccess()));
  connect_callback_.Run(bearer, Error(Error::kSuccess));
}

// Validates GetTypeString and AccessTechnologyToTechnologyFamily
TEST_F(CellularCapabilityUniversalMainTest, GetTypeString) {
  const int gsm_technologies[] = {
    MM_MODEM_ACCESS_TECHNOLOGY_LTE,
    MM_MODEM_ACCESS_TECHNOLOGY_HSPA_PLUS,
    MM_MODEM_ACCESS_TECHNOLOGY_HSPA,
    MM_MODEM_ACCESS_TECHNOLOGY_HSUPA,
    MM_MODEM_ACCESS_TECHNOLOGY_HSDPA,
    MM_MODEM_ACCESS_TECHNOLOGY_UMTS,
    MM_MODEM_ACCESS_TECHNOLOGY_EDGE,
    MM_MODEM_ACCESS_TECHNOLOGY_GPRS,
    MM_MODEM_ACCESS_TECHNOLOGY_GSM_COMPACT,
    MM_MODEM_ACCESS_TECHNOLOGY_GSM,
    MM_MODEM_ACCESS_TECHNOLOGY_LTE | MM_MODEM_ACCESS_TECHNOLOGY_EVDO0,
    MM_MODEM_ACCESS_TECHNOLOGY_GSM | MM_MODEM_ACCESS_TECHNOLOGY_EVDO0,
    MM_MODEM_ACCESS_TECHNOLOGY_LTE | MM_MODEM_ACCESS_TECHNOLOGY_EVDOA,
    MM_MODEM_ACCESS_TECHNOLOGY_GSM | MM_MODEM_ACCESS_TECHNOLOGY_EVDOA,
    MM_MODEM_ACCESS_TECHNOLOGY_LTE | MM_MODEM_ACCESS_TECHNOLOGY_EVDOB,
    MM_MODEM_ACCESS_TECHNOLOGY_GSM | MM_MODEM_ACCESS_TECHNOLOGY_EVDOB,
    MM_MODEM_ACCESS_TECHNOLOGY_GSM | MM_MODEM_ACCESS_TECHNOLOGY_1XRTT,
  };
  for (size_t i = 0; i < arraysize(gsm_technologies); ++i) {
    capability_->access_technologies_ = gsm_technologies[i];
    ASSERT_EQ(capability_->GetTypeString(), kTechnologyFamilyGsm);
  }
  const int cdma_technologies[] = {
    MM_MODEM_ACCESS_TECHNOLOGY_EVDO0,
    MM_MODEM_ACCESS_TECHNOLOGY_EVDOA,
    MM_MODEM_ACCESS_TECHNOLOGY_EVDOA | MM_MODEM_ACCESS_TECHNOLOGY_EVDO0,
    MM_MODEM_ACCESS_TECHNOLOGY_EVDOB,
    MM_MODEM_ACCESS_TECHNOLOGY_EVDOB | MM_MODEM_ACCESS_TECHNOLOGY_EVDO0,
    MM_MODEM_ACCESS_TECHNOLOGY_1XRTT,
  };
  for (size_t i = 0; i < arraysize(cdma_technologies); ++i) {
    capability_->access_technologies_ = cdma_technologies[i];
    ASSERT_EQ(capability_->GetTypeString(), kTechnologyFamilyCdma);
  }
  capability_->access_technologies_ = MM_MODEM_ACCESS_TECHNOLOGY_UNKNOWN;
  ASSERT_EQ(capability_->GetTypeString(), "");
}

TEST_F(CellularCapabilityUniversalMainTest, AllowRoaming) {
  EXPECT_FALSE(cellular_->allow_roaming_);
  EXPECT_FALSE(cellular_->provider_requires_roaming());
  EXPECT_FALSE(capability_->AllowRoaming());
  cellular_->set_provider_requires_roaming(true);
  EXPECT_TRUE(capability_->AllowRoaming());
  cellular_->set_provider_requires_roaming(false);
  cellular_->allow_roaming_ = true;
  EXPECT_TRUE(capability_->AllowRoaming());
}

TEST_F(CellularCapabilityUniversalMainTest, GetMdnForOLP) {
  const string kVzwUUID = "c83d6597-dc91-4d48-a3a7-d86b80123751";
  const string kFooUUID = "foo";
  MockMobileOperatorInfo mock_operator_info(&dispatcher_,
                                            "MobileOperatorInfo");

  mock_operator_info.SetEmptyDefaultsForProperties();
  EXPECT_CALL(mock_operator_info, IsMobileNetworkOperatorKnown())
      .WillRepeatedly(Return(true));
  EXPECT_CALL(mock_operator_info, uuid()).WillRepeatedly(ReturnRef(kVzwUUID));
  capability_->subscription_state_ =
      CellularCapabilityUniversal::kSubscriptionStateUnknown;

  cellular_->set_mdn("");
  EXPECT_EQ("0000000000", capability_->GetMdnForOLP(&mock_operator_info));
  cellular_->set_mdn("0123456789");
  EXPECT_EQ("0123456789", capability_->GetMdnForOLP(&mock_operator_info));
  cellular_->set_mdn("10123456789");
  EXPECT_EQ("0123456789", capability_->GetMdnForOLP(&mock_operator_info));

  cellular_->set_mdn("1021232333");
  capability_->subscription_state_ =
      CellularCapabilityUniversal::kSubscriptionStateUnprovisioned;
  EXPECT_EQ("0000000000", capability_->GetMdnForOLP(&mock_operator_info));
  Mock::VerifyAndClearExpectations(&mock_operator_info);

  mock_operator_info.SetEmptyDefaultsForProperties();
  EXPECT_CALL(mock_operator_info, IsMobileNetworkOperatorKnown())
      .WillRepeatedly(Return(true));
  EXPECT_CALL(mock_operator_info, uuid()).WillRepeatedly(ReturnRef(kFooUUID));

  cellular_->set_mdn("");
  EXPECT_EQ("", capability_->GetMdnForOLP(&mock_operator_info));
  cellular_->set_mdn("0123456789");
  EXPECT_EQ("0123456789", capability_->GetMdnForOLP(&mock_operator_info));
  cellular_->set_mdn("10123456789");
  EXPECT_EQ("10123456789", capability_->GetMdnForOLP(&mock_operator_info));
}

TEST_F(CellularCapabilityUniversalMainTest, UpdateServiceOLP) {
  const MobileOperatorInfo::OnlinePortal kOlp {
      "http://testurl",
      "POST",
      "imei=${imei}&imsi=${imsi}&mdn=${mdn}&min=${min}&iccid=${iccid}"};
  const vector<MobileOperatorInfo::OnlinePortal> kOlpList {kOlp};
  const string kUuidVzw = "c83d6597-dc91-4d48-a3a7-d86b80123751";
  const string kUuidFoo = "foo";

  cellular_->set_imei("1");
  cellular_->set_imsi("2");
  cellular_->set_mdn("10123456789");
  cellular_->set_min("5");
  cellular_->set_sim_identifier("6");

  mock_home_provider_info_->SetEmptyDefaultsForProperties();
  EXPECT_CALL(*mock_home_provider_info_, IsMobileNetworkOperatorKnown())
      .WillRepeatedly(Return(true));
  EXPECT_CALL(*mock_home_provider_info_, olp_list())
      .WillRepeatedly(ReturnRef(kOlpList));
  EXPECT_CALL(*mock_home_provider_info_, uuid())
      .WillOnce(ReturnRef(kUuidVzw));
  CreateService();
  capability_->UpdateServiceOLP();
  // Copy to simplify assertions below.
  Stringmap vzw_olp = cellular_->service()->olp();
  EXPECT_EQ("http://testurl", vzw_olp[kPaymentPortalURL]);
  EXPECT_EQ("POST", vzw_olp[kPaymentPortalMethod]);
  EXPECT_EQ("imei=1&imsi=2&mdn=0123456789&min=5&iccid=6",
            vzw_olp[kPaymentPortalPostData]);
  Mock::VerifyAndClearExpectations(mock_home_provider_info_);

  mock_home_provider_info_->SetEmptyDefaultsForProperties();
  EXPECT_CALL(*mock_home_provider_info_, IsMobileNetworkOperatorKnown())
      .WillRepeatedly(Return(true));
  EXPECT_CALL(*mock_home_provider_info_, olp_list())
      .WillRepeatedly(ReturnRef(kOlpList));
  EXPECT_CALL(*mock_home_provider_info_, uuid())
      .WillOnce(ReturnRef(kUuidFoo));
  capability_->UpdateServiceOLP();
  // Copy to simplify assertions below.
  Stringmap olp = cellular_->service()->olp();
  EXPECT_EQ("http://testurl", olp[kPaymentPortalURL]);
  EXPECT_EQ("POST", olp[kPaymentPortalMethod]);
  EXPECT_EQ("imei=1&imsi=2&mdn=10123456789&min=5&iccid=6",
            olp[kPaymentPortalPostData]);
}

TEST_F(CellularCapabilityUniversalMainTest, IsMdnValid) {
  cellular_->set_mdn("");
  EXPECT_FALSE(capability_->IsMdnValid());
  cellular_->set_mdn("0000000");
  EXPECT_FALSE(capability_->IsMdnValid());
  cellular_->set_mdn("0000001");
  EXPECT_TRUE(capability_->IsMdnValid());
  cellular_->set_mdn("1231223");
  EXPECT_TRUE(capability_->IsMdnValid());
}

TEST_F(CellularCapabilityUniversalTimerTest, CompleteActivation) {
  const char kIccid[] = "1234567";

  cellular_->set_sim_identifier(kIccid);
  EXPECT_CALL(*modem_info_.mock_pending_activation_store(),
              SetActivationState(PendingActivationStore::kIdentifierICCID,
                                 kIccid,
                                 PendingActivationStore::kStatePending))
      .Times(1);
  EXPECT_CALL(*modem_info_.mock_pending_activation_store(),
              GetActivationState(PendingActivationStore::kIdentifierICCID,
                                 kIccid))
      .WillOnce(Return(PendingActivationStore::kStatePending));
  EXPECT_CALL(*service_, SetActivationState(kActivationStateActivating))
      .Times(1);
  EXPECT_CALL(*modem_proxy_.get(), Reset(_, _, _)).Times(1);
  Error error;
  capability_->InitProxies();
  capability_->CompleteActivation(&error);
  Mock::VerifyAndClearExpectations(modem_info_.mock_pending_activation_store());
  Mock::VerifyAndClearExpectations(service_);
  Mock::VerifyAndClearExpectations(&mock_dispatcher_);
}

TEST_F(CellularCapabilityUniversalMainTest, UpdateServiceActivationState) {
  const char kIccid[] = "1234567";
  const vector<MobileOperatorInfo::OnlinePortal> olp_list {
    {"some@url", "some_method", "some_post_data"}
  };
  capability_->subscription_state_ =
      CellularCapabilityUniversal::kSubscriptionStateUnprovisioned;
  cellular_->set_sim_identifier("");
  cellular_->set_mdn("0000000000");
  EXPECT_CALL(*mock_home_provider_info_, IsMobileNetworkOperatorKnown())
      .WillRepeatedly(Return(true));
  EXPECT_CALL(*mock_home_provider_info_, olp_list())
      .WillRepeatedly(ReturnRef(olp_list));

  service_->SetAutoConnect(false);
  EXPECT_CALL(*service_, SetActivationState(kActivationStateNotActivated))
      .Times(1);
  capability_->UpdateServiceActivationState();
  Mock::VerifyAndClearExpectations(service_);
  EXPECT_FALSE(service_->auto_connect());

  cellular_->set_mdn("1231231122");
  capability_->subscription_state_ =
      CellularCapabilityUniversal::kSubscriptionStateUnknown;
  EXPECT_CALL(*service_, SetActivationState(kActivationStateActivated))
      .Times(1);
  capability_->UpdateServiceActivationState();
  Mock::VerifyAndClearExpectations(service_);
  EXPECT_TRUE(service_->auto_connect());

  // Make sure we don't overwrite auto-connect if a service is already
  // activated before calling UpdateServiceActivationState().
  service_->SetAutoConnect(false);
  EXPECT_FALSE(service_->auto_connect());
  const string activation_state = kActivationStateActivated;
  EXPECT_CALL(*service_, activation_state())
      .WillOnce(ReturnRef(activation_state));
  EXPECT_CALL(*service_, SetActivationState(kActivationStateActivated))
      .Times(1);
  capability_->UpdateServiceActivationState();
  Mock::VerifyAndClearExpectations(service_);
  EXPECT_FALSE(service_->auto_connect());

  service_->SetAutoConnect(false);
  cellular_->set_mdn("0000000000");
  cellular_->set_sim_identifier(kIccid);
  EXPECT_CALL(*modem_info_.mock_pending_activation_store(),
              GetActivationState(PendingActivationStore::kIdentifierICCID,
                                 kIccid))
      .Times(1)
      .WillRepeatedly(Return(PendingActivationStore::kStatePending));
  EXPECT_CALL(*service_, SetActivationState(kActivationStateActivating))
      .Times(1);
  capability_->UpdateServiceActivationState();
  Mock::VerifyAndClearExpectations(service_);
  Mock::VerifyAndClearExpectations(modem_info_.mock_pending_activation_store());
  EXPECT_FALSE(service_->auto_connect());

  EXPECT_CALL(*modem_info_.mock_pending_activation_store(),
              GetActivationState(PendingActivationStore::kIdentifierICCID,
                                 kIccid))
      .Times(2)
      .WillRepeatedly(Return(PendingActivationStore::kStateActivated));
  EXPECT_CALL(*service_, SetActivationState(kActivationStateActivated))
      .Times(1);
  capability_->UpdateServiceActivationState();
  Mock::VerifyAndClearExpectations(service_);
  Mock::VerifyAndClearExpectations(modem_info_.mock_pending_activation_store());
  EXPECT_TRUE(service_->auto_connect());

  // SubscriptionStateUnprovisioned overrides valid MDN.
  capability_->subscription_state_ =
      CellularCapabilityUniversal::kSubscriptionStateUnprovisioned;
  cellular_->set_mdn("1231231122");
  cellular_->set_sim_identifier("");
  service_->SetAutoConnect(false);
  EXPECT_CALL(*service_, SetActivationState(kActivationStateNotActivated))
      .Times(1);
  capability_->UpdateServiceActivationState();
  Mock::VerifyAndClearExpectations(service_);
  EXPECT_FALSE(service_->auto_connect());

  // SubscriptionStateProvisioned overrides invalid MDN.
  capability_->subscription_state_ =
      CellularCapabilityUniversal::kSubscriptionStateProvisioned;
  cellular_->set_mdn("0000000000");
  cellular_->set_sim_identifier("");
  service_->SetAutoConnect(false);
  EXPECT_CALL(*service_, SetActivationState(kActivationStateActivated))
      .Times(1);
  capability_->UpdateServiceActivationState();
  Mock::VerifyAndClearExpectations(service_);
  EXPECT_TRUE(service_->auto_connect());
}

TEST_F(CellularCapabilityUniversalMainTest, UpdatePendingActivationState) {
  const char kIccid[] = "1234567";

  capability_->InitProxies();
  capability_->registration_state_ =
      MM_MODEM_3GPP_REGISTRATION_STATE_SEARCHING;

  // No MDN, no ICCID.
  cellular_->set_mdn("0000000");
  capability_->subscription_state_ =
      CellularCapabilityUniversal::kSubscriptionStateUnknown;
  cellular_->set_sim_identifier("");
  EXPECT_CALL(*modem_info_.mock_pending_activation_store(),
              GetActivationState(PendingActivationStore::kIdentifierICCID, _))
      .Times(0);
  capability_->UpdatePendingActivationState();
  Mock::VerifyAndClearExpectations(modem_info_.mock_pending_activation_store());

  // Valid MDN, but subsciption_state_ Unprovisioned
  cellular_->set_mdn("1234567");
  capability_->subscription_state_ =
      CellularCapabilityUniversal::kSubscriptionStateUnprovisioned;
  cellular_->set_sim_identifier("");
  EXPECT_CALL(*modem_info_.mock_pending_activation_store(),
              GetActivationState(PendingActivationStore::kIdentifierICCID, _))
      .Times(0);
  capability_->UpdatePendingActivationState();
  Mock::VerifyAndClearExpectations(modem_info_.mock_pending_activation_store());

  // ICCID known.
  cellular_->set_sim_identifier(kIccid);

  // After the modem has reset.
  capability_->reset_done_ = true;
  EXPECT_CALL(*modem_info_.mock_pending_activation_store(),
              GetActivationState(PendingActivationStore::kIdentifierICCID,
                                 kIccid))
      .Times(1).WillOnce(Return(PendingActivationStore::kStatePending));
  EXPECT_CALL(*modem_info_.mock_pending_activation_store(),
              SetActivationState(PendingActivationStore::kIdentifierICCID,
                                 kIccid,
                                 PendingActivationStore::kStateActivated))
      .Times(1);
  capability_->UpdatePendingActivationState();
  Mock::VerifyAndClearExpectations(modem_info_.mock_pending_activation_store());

  // Not registered.
  capability_->registration_state_ =
      MM_MODEM_3GPP_REGISTRATION_STATE_SEARCHING;
  EXPECT_CALL(*modem_info_.mock_pending_activation_store(),
              GetActivationState(PendingActivationStore::kIdentifierICCID,
                                 kIccid))
      .Times(2).WillRepeatedly(Return(PendingActivationStore::kStateActivated));
  EXPECT_CALL(*service_, AutoConnect()).Times(0);
  capability_->UpdatePendingActivationState();
  Mock::VerifyAndClearExpectations(service_);

  // Service, registered.
  capability_->registration_state_ =
      MM_MODEM_3GPP_REGISTRATION_STATE_HOME;
  EXPECT_CALL(*service_, AutoConnect()).Times(1);
  capability_->UpdatePendingActivationState();

  cellular_->service_->activation_state_ = kActivationStateNotActivated;

  Mock::VerifyAndClearExpectations(service_);
  Mock::VerifyAndClearExpectations(modem_info_.mock_pending_activation_store());

  // Device is connected.
  cellular_->state_ = Cellular::kStateConnected;
  capability_->UpdatePendingActivationState();

  // Device is linked.
  cellular_->state_ = Cellular::kStateLinked;
  capability_->UpdatePendingActivationState();

  // Got valid MDN, subscription_state_ is kSubscriptionStateUnknown
  EXPECT_CALL(*modem_info_.mock_pending_activation_store(),
              RemoveEntry(PendingActivationStore::kIdentifierICCID, kIccid));
  cellular_->state_ = Cellular::kStateRegistered;
  cellular_->set_mdn("1020304");
  capability_->subscription_state_ =
      CellularCapabilityUniversal::kSubscriptionStateUnknown;
  capability_->UpdatePendingActivationState();
  Mock::VerifyAndClearExpectations(modem_info_.mock_pending_activation_store());

  // Got invalid MDN, subscription_state_ is kSubscriptionStateProvisioned
  EXPECT_CALL(*modem_info_.mock_pending_activation_store(),
              RemoveEntry(PendingActivationStore::kIdentifierICCID, kIccid));
  cellular_->state_ = Cellular::kStateRegistered;
  cellular_->set_mdn("0000000");
  capability_->subscription_state_ =
      CellularCapabilityUniversal::kSubscriptionStateProvisioned;
  capability_->UpdatePendingActivationState();
  Mock::VerifyAndClearExpectations(modem_info_.mock_pending_activation_store());
}

TEST_F(CellularCapabilityUniversalMainTest, IsServiceActivationRequired) {
  const vector<MobileOperatorInfo::OnlinePortal> empty_list;
  const vector<MobileOperatorInfo::OnlinePortal> olp_list {
    {"some@url", "some_method", "some_post_data"}
  };

  capability_->subscription_state_ =
      CellularCapabilityUniversal::kSubscriptionStateProvisioned;
  EXPECT_FALSE(capability_->IsServiceActivationRequired());

  capability_->subscription_state_ =
      CellularCapabilityUniversal::kSubscriptionStateUnprovisioned;
  EXPECT_TRUE(capability_->IsServiceActivationRequired());

  capability_->subscription_state_ =
      CellularCapabilityUniversal::kSubscriptionStateUnknown;
  cellular_->set_mdn("0000000000");
  EXPECT_FALSE(capability_->IsServiceActivationRequired());

  EXPECT_CALL(*mock_home_provider_info_, IsMobileNetworkOperatorKnown())
      .WillRepeatedly(Return(false));
  EXPECT_FALSE(capability_->IsServiceActivationRequired());
  Mock::VerifyAndClearExpectations(mock_home_provider_info_);

  EXPECT_CALL(*mock_home_provider_info_, IsMobileNetworkOperatorKnown())
      .WillRepeatedly(Return(true));
  EXPECT_CALL(*mock_home_provider_info_, olp_list())
      .WillRepeatedly(ReturnRef(empty_list));
  EXPECT_FALSE(capability_->IsServiceActivationRequired());
  Mock::VerifyAndClearExpectations(mock_home_provider_info_);

  // Set expectations for all subsequent cases.
  EXPECT_CALL(*mock_home_provider_info_, IsMobileNetworkOperatorKnown())
      .WillRepeatedly(Return(true));
  EXPECT_CALL(*mock_home_provider_info_, olp_list())
      .WillRepeatedly(ReturnRef(olp_list));

  cellular_->set_mdn("");
  EXPECT_TRUE(capability_->IsServiceActivationRequired());
  cellular_->set_mdn("1234567890");
  EXPECT_FALSE(capability_->IsServiceActivationRequired());
  cellular_->set_mdn("0000000000");
  EXPECT_TRUE(capability_->IsServiceActivationRequired());

  const char kIccid[] = "1234567890";
  cellular_->set_sim_identifier(kIccid);
  EXPECT_CALL(*modem_info_.mock_pending_activation_store(),
              GetActivationState(PendingActivationStore::kIdentifierICCID,
                                 kIccid))
      .WillOnce(Return(PendingActivationStore::kStateActivated))
      .WillOnce(Return(PendingActivationStore::kStatePending))
      .WillOnce(Return(PendingActivationStore::kStateUnknown));
  EXPECT_FALSE(capability_->IsServiceActivationRequired());
  EXPECT_FALSE(capability_->IsServiceActivationRequired());
  EXPECT_TRUE(capability_->IsServiceActivationRequired());
  Mock::VerifyAndClearExpectations(modem_info_.mock_pending_activation_store());
}

TEST_F(CellularCapabilityUniversalMainTest, OnModemCurrentCapabilitiesChanged) {
  EXPECT_FALSE(cellular_->scanning_supported());
  capability_->OnModemCurrentCapabilitiesChanged(MM_MODEM_CAPABILITY_LTE);
  EXPECT_FALSE(cellular_->scanning_supported());
  capability_->OnModemCurrentCapabilitiesChanged(MM_MODEM_CAPABILITY_CDMA_EVDO);
  EXPECT_FALSE(cellular_->scanning_supported());
  capability_->OnModemCurrentCapabilitiesChanged(MM_MODEM_CAPABILITY_GSM_UMTS);
  EXPECT_TRUE(cellular_->scanning_supported());
  capability_->OnModemCurrentCapabilitiesChanged(
      MM_MODEM_CAPABILITY_GSM_UMTS | MM_MODEM_CAPABILITY_CDMA_EVDO);
  EXPECT_TRUE(cellular_->scanning_supported());
}

TEST_F(CellularCapabilityUniversalMainTest, GetNetworkTechnologyStringOnE362) {
  cellular_->set_model_id("");;
  capability_->access_technologies_ = 0;
  EXPECT_TRUE(capability_->GetNetworkTechnologyString().empty());

  cellular_->set_mm_plugin(CellularCapabilityUniversal::kNovatelLTEMMPlugin);
  EXPECT_EQ(kNetworkTechnologyLte, capability_->GetNetworkTechnologyString());

  capability_->access_technologies_ = MM_MODEM_ACCESS_TECHNOLOGY_GPRS;
  EXPECT_EQ(kNetworkTechnologyLte, capability_->GetNetworkTechnologyString());

  cellular_->set_mm_plugin("");
  EXPECT_EQ(kNetworkTechnologyGprs, capability_->GetNetworkTechnologyString());
}

TEST_F(CellularCapabilityUniversalMainTest, GetOutOfCreditsDetectionType) {
  cellular_->set_model_id("");;
  EXPECT_EQ(OutOfCreditsDetector::OOCTypeNone,
            capability_->GetOutOfCreditsDetectionType());
  cellular_->set_mm_plugin(CellularCapabilityUniversal::kAltairLTEMMPlugin);
  EXPECT_EQ(OutOfCreditsDetector::OOCTypeSubscriptionState,
            capability_->GetOutOfCreditsDetectionType());
}

TEST_F(CellularCapabilityUniversalMainTest, SimLockStatusToProperty) {
  Error error;
  KeyValueStore store = capability_->SimLockStatusToProperty(&error);
  EXPECT_FALSE(store.GetBool(kSIMLockEnabledProperty));
  EXPECT_TRUE(store.GetString(kSIMLockTypeProperty).empty());
  EXPECT_EQ(0, store.GetUint(kSIMLockRetriesLeftProperty));

  capability_->sim_lock_status_.enabled = true;
  capability_->sim_lock_status_.retries_left = 3;
  capability_->sim_lock_status_.lock_type = MM_MODEM_LOCK_SIM_PIN;
  store = capability_->SimLockStatusToProperty(&error);
  EXPECT_TRUE(store.GetBool(kSIMLockEnabledProperty));
  EXPECT_EQ("sim-pin", store.GetString(kSIMLockTypeProperty));
  EXPECT_EQ(3, store.GetUint(kSIMLockRetriesLeftProperty));

  capability_->sim_lock_status_.lock_type = MM_MODEM_LOCK_SIM_PUK;
  store = capability_->SimLockStatusToProperty(&error);
  EXPECT_EQ("sim-puk", store.GetString(kSIMLockTypeProperty));

  capability_->sim_lock_status_.lock_type = MM_MODEM_LOCK_SIM_PIN2;
  store = capability_->SimLockStatusToProperty(&error);
  EXPECT_TRUE(store.GetString(kSIMLockTypeProperty).empty());

  capability_->sim_lock_status_.lock_type = MM_MODEM_LOCK_SIM_PUK2;
  store = capability_->SimLockStatusToProperty(&error);
  EXPECT_TRUE(store.GetString(kSIMLockTypeProperty).empty());
}

TEST_F(CellularCapabilityUniversalMainTest, OnLockRetriesChanged) {
  CellularCapabilityUniversal::LockRetryData data;
  const uint32_t kDefaultRetries = 999;

  capability_->OnLockRetriesChanged(data);
  EXPECT_EQ(kDefaultRetries, capability_->sim_lock_status_.retries_left);

  data[MM_MODEM_LOCK_SIM_PIN] = 3;
  data[MM_MODEM_LOCK_SIM_PUK] = 10;
  capability_->OnLockRetriesChanged(data);
  EXPECT_EQ(3, capability_->sim_lock_status_.retries_left);

  capability_->sim_lock_status_.lock_type = MM_MODEM_LOCK_SIM_PUK;
  capability_->OnLockRetriesChanged(data);
  EXPECT_EQ(10, capability_->sim_lock_status_.retries_left);

  capability_->sim_lock_status_.lock_type = MM_MODEM_LOCK_SIM_PIN;
  capability_->OnLockRetriesChanged(data);
  EXPECT_EQ(3, capability_->sim_lock_status_.retries_left);

  data.clear();
  capability_->OnLockRetriesChanged(data);
  EXPECT_EQ(kDefaultRetries, capability_->sim_lock_status_.retries_left);
}

TEST_F(CellularCapabilityUniversalMainTest, OnLockTypeChanged) {
  EXPECT_EQ(MM_MODEM_LOCK_UNKNOWN, capability_->sim_lock_status_.lock_type);

  capability_->OnLockTypeChanged(MM_MODEM_LOCK_NONE);
  EXPECT_EQ(MM_MODEM_LOCK_NONE, capability_->sim_lock_status_.lock_type);
  EXPECT_FALSE(capability_->sim_lock_status_.enabled);

  capability_->OnLockTypeChanged(MM_MODEM_LOCK_SIM_PIN);
  EXPECT_EQ(MM_MODEM_LOCK_SIM_PIN, capability_->sim_lock_status_.lock_type);
  EXPECT_TRUE(capability_->sim_lock_status_.enabled);

  capability_->sim_lock_status_.enabled = false;
  capability_->OnLockTypeChanged(MM_MODEM_LOCK_SIM_PUK);
  EXPECT_EQ(MM_MODEM_LOCK_SIM_PUK, capability_->sim_lock_status_.lock_type);
  EXPECT_TRUE(capability_->sim_lock_status_.enabled);
}

TEST_F(CellularCapabilityUniversalMainTest, OnSimLockPropertiesChanged) {
  EXPECT_EQ(MM_MODEM_LOCK_UNKNOWN, capability_->sim_lock_status_.lock_type);
  EXPECT_EQ(0, capability_->sim_lock_status_.retries_left);

  KeyValueStore changed;
  vector<string> invalidated;

  capability_->OnModemPropertiesChanged(changed, invalidated);
  EXPECT_EQ(MM_MODEM_LOCK_UNKNOWN, capability_->sim_lock_status_.lock_type);
  EXPECT_EQ(0, capability_->sim_lock_status_.retries_left);

  // Unlock retries changed, but the SIM wasn't locked.
  CellularCapabilityUniversal::LockRetryData retry_data;
  retry_data[MM_MODEM_LOCK_SIM_PIN] = 3;
  changed.Set(MM_MODEM_PROPERTY_UNLOCKRETRIES, brillo::Any(retry_data));

  capability_->OnModemPropertiesChanged(changed, invalidated);
  EXPECT_EQ(MM_MODEM_LOCK_UNKNOWN, capability_->sim_lock_status_.lock_type);
  EXPECT_EQ(3, capability_->sim_lock_status_.retries_left);

  // Unlock retries changed and the SIM got locked.
  changed.SetUint(MM_MODEM_PROPERTY_UNLOCKREQUIRED,
                  static_cast<uint32_t>(MM_MODEM_LOCK_SIM_PIN));
  capability_->OnModemPropertiesChanged(changed, invalidated);
  EXPECT_EQ(MM_MODEM_LOCK_SIM_PIN, capability_->sim_lock_status_.lock_type);
  EXPECT_EQ(3, capability_->sim_lock_status_.retries_left);

  // Only unlock retries changed.
  changed.Remove(MM_MODEM_PROPERTY_UNLOCKREQUIRED);
  retry_data[MM_MODEM_LOCK_SIM_PIN] = 2;
  changed.Set(MM_MODEM_PROPERTY_UNLOCKRETRIES, brillo::Any(retry_data));
  capability_->OnModemPropertiesChanged(changed, invalidated);
  EXPECT_EQ(MM_MODEM_LOCK_SIM_PIN, capability_->sim_lock_status_.lock_type);
  EXPECT_EQ(2, capability_->sim_lock_status_.retries_left);

  // Unlock retries changed with a value that doesn't match the current
  // lock type. Default to whatever count is available.
  retry_data.clear();
  retry_data[MM_MODEM_LOCK_SIM_PIN2] = 2;
  changed.Set(MM_MODEM_PROPERTY_UNLOCKRETRIES, brillo::Any(retry_data));
  capability_->OnModemPropertiesChanged(changed, invalidated);
  EXPECT_EQ(MM_MODEM_LOCK_SIM_PIN, capability_->sim_lock_status_.lock_type);
  EXPECT_EQ(2, capability_->sim_lock_status_.retries_left);
}

}  // namespace shill
