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

#include "shill/cellular/cellular_capability_gsm.h"

#include <base/bind.h>
#if defined(__ANDROID__)
#include <dbus/service_constants.h>
#else
#include <chromeos/dbus/service_constants.h>
#endif  // __ANDROID__
#include <mm/mm-modem.h>

#include "shill/cellular/cellular.h"
#include "shill/cellular/cellular_service.h"
#include "shill/cellular/mock_modem_cdma_proxy.h"
#include "shill/cellular/mock_modem_gobi_proxy.h"
#include "shill/cellular/mock_modem_gsm_card_proxy.h"
#include "shill/cellular/mock_modem_gsm_network_proxy.h"
#include "shill/cellular/mock_modem_info.h"
#include "shill/cellular/mock_modem_proxy.h"
#include "shill/cellular/mock_modem_simple_proxy.h"
#include "shill/error.h"
#include "shill/mock_adaptors.h"
#include "shill/mock_control.h"
#include "shill/mock_profile.h"
#include "shill/net/mock_rtnl_handler.h"
#include "shill/test_event_dispatcher.h"
#include "shill/testing.h"

using base::Bind;
using base::Unretained;
using std::string;
using testing::InSequence;
using testing::NiceMock;
using testing::_;

namespace shill {

class CellularCapabilityTest : public testing::Test {
 public:
  CellularCapabilityTest()
      : control_interface_(this),
        modem_info_(&control_interface_, &dispatcher_, nullptr, nullptr),
        create_gsm_card_proxy_from_factory_(false),
        proxy_(new MockModemProxy()),
        simple_proxy_(new MockModemSimpleProxy()),
        cdma_proxy_(new MockModemCDMAProxy()),
        gsm_card_proxy_(new MockModemGSMCardProxy()),
        gsm_network_proxy_(new MockModemGSMNetworkProxy()),
        gobi_proxy_(new MockModemGobiProxy()),
        capability_(nullptr),
        device_adaptor_(nullptr),
        cellular_(new Cellular(&modem_info_,
                               "",
                               "",
                               0,
                               Cellular::kTypeGSM,
                               "",
                               "")) {
    modem_info_.metrics()->RegisterDevice(cellular_->interface_index(),
                            Technology::kCellular);
  }

  virtual ~CellularCapabilityTest() {
    cellular_->service_ = nullptr;
    capability_ = nullptr;
    device_adaptor_ = nullptr;
  }

  virtual void SetUp() {
    static_cast<Device*>(cellular_.get())->rtnl_handler_ = &rtnl_handler_;

    capability_ = static_cast<CellularCapabilityClassic*>(
        cellular_->capability_.get());
    device_adaptor_ =
        static_cast<DeviceMockAdaptor*>(cellular_->adaptor());
    ASSERT_NE(nullptr, device_adaptor_);;
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

  CellularCapabilityGSM* GetGsmCapability() {
    return static_cast<CellularCapabilityGSM*>(cellular_->capability_.get());
  }

  void ReleaseCapabilityProxies() {
    capability_->ReleaseProxies();
  }

  void InvokeEnable(bool enable, Error* error,
                    const ResultCallback& callback, int timeout) {
    callback.Run(Error());
  }
  void InvokeEnableFail(bool enable, Error* error,
                        const ResultCallback& callback, int timeout) {
    callback.Run(Error(Error::kOperationFailed));
  }
  void InvokeDisconnect(Error* error, const ResultCallback& callback,
                        int timeout) {
    callback.Run(Error());
  }
  void InvokeDisconnectFail(Error* error, const ResultCallback& callback,
                            int timeout) {
    callback.Run(Error(Error::kOperationFailed));
  }
  void InvokeGetModemStatus(Error* error,
                            const KeyValueStoreCallback& callback,
                            int timeout) {
    KeyValueStore props;
    props.SetString("carrier", kTestCarrier);
    props.SetString("unknown-property", "irrelevant-value");
    callback.Run(props, Error());
  }
  void InvokeGetModemInfo(Error* error, const ModemInfoCallback& callback,
                          int timeout) {
    callback.Run(kManufacturer, kModelID, kHWRev, Error());
  }
  void InvokeSetCarrier(const string& carrier, Error* error,
                        const ResultCallback& callback, int timeout) {
    callback.Run(Error());
  }

  MOCK_METHOD1(TestCallback, void(const Error& error));

 protected:
  static const char kTestMobileProviderDBPath[];
  static const char kTestCarrier[];
  static const char kManufacturer[];
  static const char kModelID[];
  static const char kHWRev[];

  class TestControl : public MockControl {
   public:
    explicit TestControl(CellularCapabilityTest* test) : test_(test) {}

    virtual ModemProxyInterface* CreateModemProxy(
        const string& /*path*/,
        const string& /*service*/) {
      return test_->proxy_.release();
    }

    virtual ModemSimpleProxyInterface* CreateModemSimpleProxy(
        const string& /*path*/,
        const string& /*service*/) {
      return test_->simple_proxy_.release();
    }

    virtual ModemCDMAProxyInterface* CreateModemCDMAProxy(
        const string& /*path*/,
        const string& /*service*/) {
      return test_->cdma_proxy_.release();
    }

    virtual ModemGSMCardProxyInterface* CreateModemGSMCardProxy(
        const string& /*path*/,
        const string& /*service*/) {
      // TODO(benchan): This code conditionally returns a nullptr to avoid
      // CellularCapabilityGSM::InitProperties (and thus
      // CellularCapabilityGSM::GetIMSI) from being called during the
      // construction. Remove this workaround after refactoring the tests.
      return test_->create_gsm_card_proxy_from_factory_ ?
          test_->gsm_card_proxy_.release() : nullptr;
    }

    virtual ModemGSMNetworkProxyInterface* CreateModemGSMNetworkProxy(
        const string& /*path*/,
        const string& /*service*/) {
      return test_->gsm_network_proxy_.release();
    }

    virtual ModemGobiProxyInterface* CreateModemGobiProxy(
        const string& /*path*/,
        const string& /*service*/) {
      return test_->gobi_proxy_.release();
    }

   private:
    CellularCapabilityTest* test_;
  };

  void SetProxy() {
    capability_->proxy_.reset(proxy_.release());
  }

  void SetSimpleProxy() {
    capability_->simple_proxy_.reset(simple_proxy_.release());
  }

  void SetGSMNetworkProxy() {
    CellularCapabilityGSM* gsm_capability =
        static_cast<CellularCapabilityGSM*>(cellular_->capability_.get());
    gsm_capability->network_proxy_.reset(gsm_network_proxy_.release());
  }

  void SetCellularType(Cellular::Type type) {
    cellular_->InitCapability(type);
    capability_ = static_cast<CellularCapabilityClassic*>(
        cellular_->capability_.get());
  }

  void AllowCreateGSMCardProxyFromFactory() {
    create_gsm_card_proxy_from_factory_ = true;
  }

  EventDispatcherForTest dispatcher_;
  TestControl control_interface_;
  MockModemInfo modem_info_;
  MockRTNLHandler rtnl_handler_;
  bool create_gsm_card_proxy_from_factory_;
  std::unique_ptr<MockModemProxy> proxy_;
  std::unique_ptr<MockModemSimpleProxy> simple_proxy_;
  std::unique_ptr<MockModemCDMAProxy> cdma_proxy_;
  std::unique_ptr<MockModemGSMCardProxy> gsm_card_proxy_;
  std::unique_ptr<MockModemGSMNetworkProxy> gsm_network_proxy_;
  std::unique_ptr<MockModemGobiProxy> gobi_proxy_;
  CellularCapabilityClassic* capability_;  // Owned by |cellular_|.
  DeviceMockAdaptor* device_adaptor_;  // Owned by |cellular_|.
  CellularRefPtr cellular_;
};

const char CellularCapabilityTest::kTestMobileProviderDBPath[] =
    "provider_db_unittest.bfd";
const char CellularCapabilityTest::kTestCarrier[] = "The Cellular Carrier";
const char CellularCapabilityTest::kManufacturer[] = "Company";
const char CellularCapabilityTest::kModelID[] = "Gobi 2000";
const char CellularCapabilityTest::kHWRev[] = "A00B1234";

TEST_F(CellularCapabilityTest, GetModemStatus) {
  SetCellularType(Cellular::kTypeCDMA);
  EXPECT_CALL(*simple_proxy_,
              GetModemStatus(_, _, CellularCapability::kTimeoutDefault)).
      WillOnce(Invoke(this, &CellularCapabilityTest::InvokeGetModemStatus));
  EXPECT_CALL(*this, TestCallback(IsSuccess()));
  SetSimpleProxy();
  ResultCallback callback =
      Bind(&CellularCapabilityTest::TestCallback, Unretained(this));
  capability_->GetModemStatus(callback);
  EXPECT_EQ(kTestCarrier, cellular_->carrier());
}

TEST_F(CellularCapabilityTest, GetModemInfo) {
  EXPECT_CALL(*proxy_, GetModemInfo(_, _, CellularCapability::kTimeoutDefault))
      .WillOnce(Invoke(this, &CellularCapabilityTest::InvokeGetModemInfo));
  EXPECT_CALL(*this, TestCallback(IsSuccess()));
  SetProxy();
  ResultCallback callback =
      Bind(&CellularCapabilityTest::TestCallback, Unretained(this));
  capability_->GetModemInfo(callback);
  EXPECT_EQ(kManufacturer, cellular_->manufacturer());
  EXPECT_EQ(kModelID, cellular_->model_id());
  EXPECT_EQ(kHWRev, cellular_->hardware_revision());
}

TEST_F(CellularCapabilityTest, EnableModemSucceed) {
  EXPECT_CALL(*proxy_, Enable(true, _, _, CellularCapability::kTimeoutEnable))
      .WillOnce(Invoke(this, &CellularCapabilityTest::InvokeEnable));
  EXPECT_CALL(*this, TestCallback(IsSuccess()));
  ResultCallback callback =
      Bind(&CellularCapabilityTest::TestCallback, Unretained(this));
  SetProxy();
  capability_->EnableModem(callback);
}

TEST_F(CellularCapabilityTest, EnableModemFail) {
  EXPECT_CALL(*proxy_, Enable(true, _, _, CellularCapability::kTimeoutEnable))
      .WillOnce(Invoke(this, &CellularCapabilityTest::InvokeEnableFail));
  EXPECT_CALL(*this, TestCallback(IsFailure()));
  ResultCallback callback =
      Bind(&CellularCapabilityTest::TestCallback, Unretained(this));
  SetProxy();
  capability_->EnableModem(callback);
}

TEST_F(CellularCapabilityTest, FinishEnable) {
  EXPECT_CALL(*gsm_network_proxy_,
              GetRegistrationInfo(nullptr, _,
                                  CellularCapability::kTimeoutDefault));
  EXPECT_CALL(
      *gsm_network_proxy_,
      GetSignalQuality(nullptr, _, CellularCapability::kTimeoutDefault));
  EXPECT_CALL(*this, TestCallback(IsSuccess()));
  SetGSMNetworkProxy();
  capability_->FinishEnable(
      Bind(&CellularCapabilityTest::TestCallback, Unretained(this)));
}

TEST_F(CellularCapabilityTest, UnsupportedOperation) {
  Error error;
  EXPECT_CALL(*this, TestCallback(IsSuccess())).Times(0);
  capability_->CellularCapability::Reset(
      &error,
      Bind(&CellularCapabilityTest::TestCallback, Unretained(this)));
  EXPECT_TRUE(error.IsFailure());
  EXPECT_EQ(Error::kNotSupported, error.type());
}

TEST_F(CellularCapabilityTest, AllowRoaming) {
  EXPECT_FALSE(cellular_->GetAllowRoaming(nullptr));
  cellular_->SetAllowRoaming(false, nullptr);
  EXPECT_FALSE(cellular_->GetAllowRoaming(nullptr));

  {
    InSequence seq;
    EXPECT_CALL(*device_adaptor_,
                EmitBoolChanged(kCellularAllowRoamingProperty, true));
    EXPECT_CALL(*device_adaptor_,
                EmitBoolChanged(kCellularAllowRoamingProperty, false));
  }

  cellular_->state_ = Cellular::kStateConnected;
  static_cast<CellularCapabilityGSM*>(capability_)->registration_state_ =
      MM_MODEM_GSM_NETWORK_REG_STATUS_ROAMING;
  cellular_->SetAllowRoaming(true, nullptr);
  EXPECT_TRUE(cellular_->GetAllowRoaming(nullptr));
  EXPECT_EQ(Cellular::kStateConnected, cellular_->state_);

  EXPECT_CALL(*proxy_, Disconnect(_, _, CellularCapability::kTimeoutDisconnect))
      .WillOnce(Invoke(this, &CellularCapabilityTest::InvokeDisconnect));
  SetProxy();
  cellular_->state_ = Cellular::kStateConnected;
  cellular_->SetAllowRoaming(false, nullptr);
  EXPECT_FALSE(cellular_->GetAllowRoaming(nullptr));
  EXPECT_EQ(Cellular::kStateRegistered, cellular_->state_);
}

TEST_F(CellularCapabilityTest, SetCarrier) {
  static const char kCarrier[] = "Generic UMTS";
  EXPECT_CALL(
      *gobi_proxy_,
      SetCarrier(kCarrier, _, _,
                 CellularCapabilityClassic::kTimeoutSetCarrierMilliseconds))
      .WillOnce(Invoke(this, &CellularCapabilityTest::InvokeSetCarrier));
  EXPECT_CALL(*this, TestCallback(IsSuccess()));
  Error error;
  capability_->SetCarrier(kCarrier, &error,
                          Bind(&CellularCapabilityTest::TestCallback,
                               Unretained(this)));
  EXPECT_TRUE(error.IsSuccess());
}

MATCHER_P(HasApn, apn, "") {
  return arg.ContainsString(kApnProperty) && apn == arg.GetString(kApnProperty);
}

MATCHER(HasNoApn, "") {
  return !arg.ContainsString(kApnProperty);
}

TEST_F(CellularCapabilityTest, TryApns) {
  static const string kLastGoodApn("remembered.apn");
  static const string kLastGoodUsername("remembered.user");
  static const string kSuppliedApn("my.apn");
  static const string kTmobileApn1("epc.tmobile.com");
  static const string kTmobileApn2("wap.voicestream.com");
  static const string kTmobileApn3("internet2.voicestream.com");
  static const string kTmobileApn4("internet3.voicestream.com");
  const Stringmaps kDatabaseApnList {{{ kApnProperty, kTmobileApn1 }},
                                     {{ kApnProperty, kTmobileApn2 }},
                                     {{ kApnProperty, kTmobileApn3 }},
                                     {{ kApnProperty, kTmobileApn4 }}};


  CreateService();
  // Supply the database APNs to |cellular_| object.
  cellular_->set_apn_list(kDatabaseApnList);
  ProfileRefPtr profile(new NiceMock<MockProfile>(
      modem_info_.control_interface(), modem_info_.metrics(),
      modem_info_.manager()));
  cellular_->service()->set_profile(profile);

  Error error;
  Stringmap apn_info;
  KeyValueStore props;
  CellularCapabilityGSM* gsm_capability = GetGsmCapability();

  apn_info[kApnProperty] = kLastGoodApn;
  apn_info[kApnUsernameProperty] = kLastGoodUsername;
  cellular_->service()->SetLastGoodApn(apn_info);
  props.Clear();
  EXPECT_TRUE(props.IsEmpty());
  gsm_capability->SetupConnectProperties(&props);
  // We expect the list to contain the last good APN, plus
  // the 4 APNs from the mobile provider info database.
  EXPECT_EQ(5, gsm_capability->apn_try_list_.size());
  EXPECT_TRUE(props.ContainsString(kApnProperty));
  EXPECT_EQ(kLastGoodApn, props.GetString(kApnProperty));
  EXPECT_TRUE(props.ContainsString(kApnUsernameProperty));
  EXPECT_EQ(kLastGoodUsername,
            props.GetString(kApnUsernameProperty));

  apn_info.clear();
  props.Clear();
  apn_info[kApnProperty] = kSuppliedApn;
  // Setting the APN has the side effect of clearing the LastGoodApn,
  // so the try list will have 5 elements, with the first one being
  // the supplied APN.
  cellular_->service()->SetApn(apn_info, &error);
  EXPECT_TRUE(props.IsEmpty());
  gsm_capability->SetupConnectProperties(&props);
  EXPECT_EQ(5, gsm_capability->apn_try_list_.size());
  EXPECT_TRUE(props.ContainsString(kApnProperty));
  EXPECT_EQ(kSuppliedApn, props.GetString(kApnProperty));

  apn_info.clear();
  props.Clear();
  apn_info[kApnProperty] = kLastGoodApn;
  apn_info[kApnUsernameProperty] = kLastGoodUsername;
  // Now when LastGoodAPN is set, it will be the one selected.
  cellular_->service()->SetLastGoodApn(apn_info);
  EXPECT_TRUE(props.IsEmpty());
  gsm_capability->SetupConnectProperties(&props);
  // We expect the list to contain the last good APN, plus
  // the user-supplied APN, plus the 4 APNs from the mobile
  // provider info database.
  EXPECT_EQ(6, gsm_capability->apn_try_list_.size());
  EXPECT_TRUE(props.ContainsString(kApnProperty));
  EXPECT_EQ(kLastGoodApn, props.GetString(kApnProperty));

  // Now try all the given APNs.
  using testing::InSequence;
  {
    InSequence dummy;
    EXPECT_CALL(*simple_proxy_, Connect(HasApn(kLastGoodApn), _, _, _));
    EXPECT_CALL(*simple_proxy_, Connect(HasApn(kSuppliedApn), _, _, _));
    EXPECT_CALL(*simple_proxy_, Connect(HasApn(kTmobileApn1), _, _, _));
    EXPECT_CALL(*simple_proxy_, Connect(HasApn(kTmobileApn2), _, _, _));
    EXPECT_CALL(*simple_proxy_, Connect(HasApn(kTmobileApn3), _, _, _));
    EXPECT_CALL(*simple_proxy_, Connect(HasApn(kTmobileApn4), _, _, _));
    EXPECT_CALL(*simple_proxy_, Connect(HasNoApn(), _, _, _));
  }
  SetSimpleProxy();
  gsm_capability->Connect(props, &error, ResultCallback());
  Error cerror(Error::kInvalidApn);
  gsm_capability->OnConnectReply(ResultCallback(), cerror);
  EXPECT_EQ(5, gsm_capability->apn_try_list_.size());
  gsm_capability->OnConnectReply(ResultCallback(), cerror);
  EXPECT_EQ(4, gsm_capability->apn_try_list_.size());
  gsm_capability->OnConnectReply(ResultCallback(), cerror);
  EXPECT_EQ(3, gsm_capability->apn_try_list_.size());
  gsm_capability->OnConnectReply(ResultCallback(), cerror);
  EXPECT_EQ(2, gsm_capability->apn_try_list_.size());
  gsm_capability->OnConnectReply(ResultCallback(), cerror);
  EXPECT_EQ(1, gsm_capability->apn_try_list_.size());
  gsm_capability->OnConnectReply(ResultCallback(), cerror);
  EXPECT_EQ(0, gsm_capability->apn_try_list_.size());
}

TEST_F(CellularCapabilityTest, StopModemDisconnectSuccess) {
  EXPECT_CALL(*proxy_, Disconnect(_, _, CellularCapability::kTimeoutDisconnect))
      .WillOnce(Invoke(this,
                       &CellularCapabilityTest::InvokeDisconnect));
  EXPECT_CALL(*proxy_, Enable(_, _, _, CellularCapability::kTimeoutEnable))
      .WillOnce(Invoke(this,
                       &CellularCapabilityTest::InvokeEnable));
  EXPECT_CALL(*this, TestCallback(IsSuccess()));
  SetProxy();

  Error error;
  capability_->StopModem(
      &error, Bind(&CellularCapabilityTest::TestCallback, Unretained(this)));
  dispatcher_.DispatchPendingEvents();
}

TEST_F(CellularCapabilityTest, StopModemDisconnectFail) {
  EXPECT_CALL(*proxy_, Disconnect(_, _, CellularCapability::kTimeoutDisconnect))
      .WillOnce(Invoke(this,
                       &CellularCapabilityTest::InvokeDisconnectFail));
  EXPECT_CALL(*proxy_, Enable(_, _, _, CellularCapability::kTimeoutEnable))
      .WillOnce(Invoke(this,
                       &CellularCapabilityTest::InvokeEnable));
  EXPECT_CALL(*this, TestCallback(IsSuccess()));
  SetProxy();

  Error error;
  capability_->StopModem(
      &error, Bind(&CellularCapabilityTest::TestCallback, Unretained(this)));
  dispatcher_.DispatchPendingEvents();
}

TEST_F(CellularCapabilityTest, DisconnectNoProxy) {
  Error error;
  ResultCallback disconnect_callback;
  EXPECT_CALL(*proxy_, Disconnect(_, _, CellularCapability::kTimeoutDisconnect))
      .Times(0);
  ReleaseCapabilityProxies();
  capability_->Disconnect(&error, disconnect_callback);
}

}  // namespace shill
