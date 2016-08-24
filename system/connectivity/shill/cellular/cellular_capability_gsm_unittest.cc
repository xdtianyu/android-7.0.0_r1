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

#include <string>
#include <vector>

#include <base/bind.h>
#include <base/strings/stringprintf.h>
#if defined(__ANDROID__)
#include <dbus/service_constants.h>
#else
#include <chromeos/dbus/service_constants.h>
#endif  // __ANDROID__
#include <mm/mm-modem.h>

#include "shill/cellular/cellular.h"
#include "shill/cellular/cellular_service.h"
#include "shill/cellular/mock_mobile_operator_info.h"
#include "shill/cellular/mock_modem_gsm_card_proxy.h"
#include "shill/cellular/mock_modem_gsm_network_proxy.h"
#include "shill/cellular/mock_modem_info.h"
#include "shill/cellular/mock_modem_proxy.h"
#include "shill/cellular/mock_modem_simple_proxy.h"
#include "shill/error.h"
#include "shill/mock_adaptors.h"
#include "shill/mock_control.h"
#include "shill/mock_log.h"
#include "shill/mock_profile.h"
#include "shill/test_event_dispatcher.h"
#include "shill/testing.h"

using base::Bind;
using base::StringPrintf;
using base::Unretained;
using std::string;
using std::vector;
using testing::_;
using testing::Invoke;
using testing::NiceMock;
using testing::Return;
using testing::SaveArg;

namespace shill {

class CellularCapabilityGSMTest : public testing::Test {
 public:
  CellularCapabilityGSMTest()
      : control_interface_(this),
        modem_info_(&control_interface_, &dispatcher_, nullptr, nullptr),
        create_card_proxy_from_factory_(false),
        proxy_(new MockModemProxy()),
        simple_proxy_(new MockModemSimpleProxy()),
        card_proxy_(new MockModemGSMCardProxy()),
        network_proxy_(new MockModemGSMNetworkProxy()),
        capability_(nullptr),
        device_adaptor_(nullptr),
        cellular_(new Cellular(&modem_info_,
                               "",
                               kAddress,
                               0,
                               Cellular::kTypeGSM,
                               "",
                               "")),
        mock_home_provider_info_(nullptr),
        mock_serving_operator_info_(nullptr) {
    modem_info_.metrics()->RegisterDevice(cellular_->interface_index(),
                                          Technology::kCellular);
  }

  virtual ~CellularCapabilityGSMTest() {
    cellular_->service_ = nullptr;
    capability_ = nullptr;
    device_adaptor_ = nullptr;
  }

  virtual void SetUp() {
    capability_ =
        static_cast<CellularCapabilityGSM*>(cellular_->capability_.get());
    device_adaptor_ =
        static_cast<DeviceMockAdaptor*>(cellular_->adaptor());
  }

  void InvokeEnable(bool enable, Error* error,
                    const ResultCallback& callback, int timeout) {
    callback.Run(Error());
  }
  void InvokeGetIMEI(Error* error, const GSMIdentifierCallback& callback,
                     int timeout) {
    callback.Run(kIMEI, Error());
  }
  void InvokeGetIMSI(Error* error, const GSMIdentifierCallback& callback,
                     int timeout) {
    callback.Run(kIMSI, Error());
  }
  void InvokeGetIMSIFails(Error* error, const GSMIdentifierCallback& callback,
                          int timeout) {
    callback.Run("", Error(Error::kOperationFailed));
  }
  void InvokeGetMSISDN(Error* error, const GSMIdentifierCallback& callback,
                       int timeout) {
    callback.Run(kMSISDN, Error());
  }
  void InvokeGetMSISDNFail(Error* error, const GSMIdentifierCallback& callback,
                           int timeout) {
    callback.Run("", Error(Error::kOperationFailed));
  }
  void InvokeGetSPN(Error* error, const GSMIdentifierCallback& callback,
                    int timeout) {
    callback.Run(kTestCarrier, Error());
  }
  void InvokeGetSPNFail(Error* error, const GSMIdentifierCallback& callback,
                        int timeout) {
    callback.Run("", Error(Error::kOperationFailed));
  }
  void InvokeGetSignalQuality(Error* error,
                              const SignalQualityCallback& callback,
                              int timeout) {
    callback.Run(kStrength, Error());
  }
  void InvokeGetRegistrationInfo(Error* error,
                                 const RegistrationInfoCallback& callback,
                                 int timeout) {
    callback.Run(MM_MODEM_GSM_NETWORK_REG_STATUS_HOME,
                 kTestNetwork, kTestCarrier, Error());
  }
  void InvokeRegister(const string& network_id,
                      Error* error,
                      const ResultCallback& callback,
                      int timeout) {
    callback.Run(Error());
  }
  void InvokeEnablePIN(const string& pin, bool enable,
                       Error* error, const ResultCallback& callback,
                       int timeout) {
    callback.Run(Error());
  }
  void InvokeSendPIN(const string& pin, Error* error,
                     const ResultCallback& callback, int timeout) {
    callback.Run(Error());
  }
  void InvokeSendPUK(const string& puk, const string& pin, Error* error,
                     const ResultCallback& callback, int timeout) {
    callback.Run(Error());
  }
  void InvokeChangePIN(const string& old_pin, const string& pin, Error* error,
                       const ResultCallback& callback, int timeout) {
    callback.Run(Error());
  }
  void InvokeGetModemStatus(Error* error,
                            const KeyValueStoreCallback& callback,
                            int timeout) {
    KeyValueStore props;
    callback.Run(props, Error());
  }
  void InvokeGetModemInfo(Error* error, const ModemInfoCallback& callback,
                          int timeout) {
    callback.Run("", "", "", Error());
  }

  void InvokeConnectFail(KeyValueStore props, Error* error,
                         const ResultCallback& callback, int timeout) {
    callback.Run(Error(Error::kOperationFailed));
  }

  MOCK_METHOD1(TestCallback, void(const Error& error));

 protected:
  static const char kAddress[];
  static const char kTestMobileProviderDBPath[];
  static const char kTestNetwork[];
  static const char kTestCarrier[];
  static const char kPIN[];
  static const char kPUK[];
  static const char kIMEI[];
  static const char kIMSI[];
  static const char kMSISDN[];
  static const int kStrength;

  class TestControl : public MockControl {
   public:
    explicit TestControl(CellularCapabilityGSMTest* test) : test_(test) {}

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

    virtual ModemGSMCardProxyInterface* CreateModemGSMCardProxy(
        const string& /*path*/,
        const string& /*service*/) {
      // TODO(benchan): This code conditionally returns a nullptr to avoid
      // CellularCapabilityGSM::InitProperties (and thus
      // CellularCapabilityGSM::GetIMSI) from being called during the
      // construction. Remove this workaround after refactoring the tests.
      return test_->create_card_proxy_from_factory_ ?
          test_->card_proxy_.release() : nullptr;
    }

    virtual ModemGSMNetworkProxyInterface* CreateModemGSMNetworkProxy(
        const string& /*path*/,
        const string& /*service*/) {
      return test_->network_proxy_.release();
    }

   private:
    CellularCapabilityGSMTest* test_;
  };

  void SetProxy() {
    capability_->proxy_.reset(proxy_.release());
  }

  void SetCardProxy() {
    capability_->card_proxy_.reset(card_proxy_.release());
  }

  void SetNetworkProxy() {
    capability_->network_proxy_.reset(network_proxy_.release());
  }

  void SetAccessTechnology(uint32_t technology) {
    capability_->access_technology_ = technology;
  }

  void SetRegistrationState(uint32_t state) {
    capability_->registration_state_ = state;
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

  void SetMockMobileOperatorInfoObjects() {
    CHECK(!mock_home_provider_info_);
    CHECK(!mock_serving_operator_info_);
    mock_home_provider_info_ =
        new MockMobileOperatorInfo(&dispatcher_, "HomeProvider");
    mock_serving_operator_info_ =
        new MockMobileOperatorInfo(&dispatcher_, "ServingOperator");
    cellular_->set_home_provider_info(mock_home_provider_info_);
    cellular_->set_serving_operator_info(mock_serving_operator_info_);
  }

  void SetupCommonProxiesExpectations() {
    EXPECT_CALL(*proxy_, set_state_changed_callback(_));
    EXPECT_CALL(*network_proxy_, set_signal_quality_callback(_));
    EXPECT_CALL(*network_proxy_, set_network_mode_callback(_));
    EXPECT_CALL(*network_proxy_, set_registration_info_callback(_));
  }

  void SetupCommonStartModemExpectations() {
    SetupCommonProxiesExpectations();

    EXPECT_CALL(*proxy_, Enable(_, _, _, CellularCapability::kTimeoutEnable))
        .WillOnce(Invoke(this, &CellularCapabilityGSMTest::InvokeEnable));
    EXPECT_CALL(*card_proxy_,
                GetIMEI(_, _, CellularCapability::kTimeoutDefault))
        .WillOnce(Invoke(this, &CellularCapabilityGSMTest::InvokeGetIMEI));
    EXPECT_CALL(*card_proxy_,
                GetIMSI(_, _, CellularCapability::kTimeoutDefault))
        .WillOnce(Invoke(this, &CellularCapabilityGSMTest::InvokeGetIMSI));
    EXPECT_CALL(*network_proxy_, AccessTechnology());
    EXPECT_CALL(*card_proxy_, EnabledFacilityLocks());
    EXPECT_CALL(*proxy_,
                GetModemInfo(_, _, CellularCapability::kTimeoutDefault))
        .WillOnce(Invoke(this, &CellularCapabilityGSMTest::InvokeGetModemInfo));
    EXPECT_CALL(*network_proxy_,
                GetRegistrationInfo(_, _, CellularCapability::kTimeoutDefault));
    EXPECT_CALL(*network_proxy_,
                GetSignalQuality(_, _, CellularCapability::kTimeoutDefault));
    EXPECT_CALL(*this, TestCallback(IsSuccess()));
  }

  void InitProxies() {
    AllowCreateCardProxyFromFactory();
    capability_->InitProxies();
  }

  void AllowCreateCardProxyFromFactory() {
    create_card_proxy_from_factory_ = true;
  }

  EventDispatcherForTest dispatcher_;
  TestControl control_interface_;
  MockModemInfo modem_info_;
  bool create_card_proxy_from_factory_;
  std::unique_ptr<MockModemProxy> proxy_;
  std::unique_ptr<MockModemSimpleProxy> simple_proxy_;
  std::unique_ptr<MockModemGSMCardProxy> card_proxy_;
  std::unique_ptr<MockModemGSMNetworkProxy> network_proxy_;
  CellularCapabilityGSM* capability_;  // Owned by |cellular_|.
  DeviceMockAdaptor* device_adaptor_;  // Owned by |cellular_|.
  CellularRefPtr cellular_;

  // Set when required and passed to |cellular_|. Owned by |cellular_|.
  MockMobileOperatorInfo* mock_home_provider_info_;
  MockMobileOperatorInfo* mock_serving_operator_info_;
};

const char CellularCapabilityGSMTest::kAddress[] = "1122334455";
const char CellularCapabilityGSMTest::kTestMobileProviderDBPath[] =
    "provider_db_unittest.bfd";
const char CellularCapabilityGSMTest::kTestCarrier[] = "The Cellular Carrier";
const char CellularCapabilityGSMTest::kTestNetwork[] = "310555";
const char CellularCapabilityGSMTest::kPIN[] = "9876";
const char CellularCapabilityGSMTest::kPUK[] = "8765";
const char CellularCapabilityGSMTest::kIMEI[] = "987654321098765";
const char CellularCapabilityGSMTest::kIMSI[] = "310150123456789";
const char CellularCapabilityGSMTest::kMSISDN[] = "12345678901";
const int CellularCapabilityGSMTest::kStrength = 80;

TEST_F(CellularCapabilityGSMTest, PropertyStore) {
  EXPECT_TRUE(cellular_->store().Contains(kSIMLockStatusProperty));
}

TEST_F(CellularCapabilityGSMTest, GetIMEI) {
  EXPECT_CALL(*card_proxy_, GetIMEI(_, _, CellularCapability::kTimeoutDefault))
      .WillOnce(Invoke(this,
                       &CellularCapabilityGSMTest::InvokeGetIMEI));
  EXPECT_CALL(*this, TestCallback(IsSuccess()));
  SetCardProxy();
  ASSERT_TRUE(cellular_->imei().empty());
  capability_->GetIMEI(Bind(&CellularCapabilityGSMTest::TestCallback,
                            Unretained(this)));
  EXPECT_EQ(kIMEI, cellular_->imei());
}

TEST_F(CellularCapabilityGSMTest, GetIMSI) {
  SetMockMobileOperatorInfoObjects();
  EXPECT_CALL(*card_proxy_, GetIMSI(_, _, CellularCapability::kTimeoutDefault))
      .WillOnce(Invoke(this,
                       &CellularCapabilityGSMTest::InvokeGetIMSI));
  EXPECT_CALL(*this, TestCallback(IsSuccess()));
  SetCardProxy();
  ResultCallback callback = Bind(&CellularCapabilityGSMTest::TestCallback,
                                 Unretained(this));
  EXPECT_TRUE(cellular_->imsi().empty());
  EXPECT_FALSE(cellular_->sim_present());
  EXPECT_CALL(*mock_home_provider_info_, UpdateIMSI(kIMSI));
  capability_->GetIMSI(callback);
  EXPECT_EQ(kIMSI, cellular_->imsi());
  EXPECT_TRUE(cellular_->sim_present());
}

// In this test, the call to the proxy's GetIMSI() will always indicate failure,
// which will cause the retry logic to call the proxy again a number of times.
// Eventually, the retries expire.
TEST_F(CellularCapabilityGSMTest, GetIMSIFails) {
  ScopedMockLog log;
  EXPECT_CALL(log, Log(logging::LOG_INFO,
                       ::testing::EndsWith("cellular_capability_gsm.cc"),
                       ::testing::StartsWith("GetIMSI failed - ")));
  EXPECT_CALL(*card_proxy_, GetIMSI(_, _, CellularCapability::kTimeoutDefault))
      .Times(CellularCapabilityGSM::kGetIMSIRetryLimit + 2)
      .WillRepeatedly(Invoke(this,
                             &CellularCapabilityGSMTest::InvokeGetIMSIFails));
  EXPECT_CALL(*this, TestCallback(IsFailure())).Times(2);
  SetCardProxy();
  ResultCallback callback = Bind(&CellularCapabilityGSMTest::TestCallback,
                                 Unretained(this));
  EXPECT_TRUE(cellular_->imsi().empty());
  EXPECT_FALSE(cellular_->sim_present());

  capability_->sim_lock_status_.lock_type = "sim-pin";
  capability_->GetIMSI(callback);
  EXPECT_TRUE(cellular_->imsi().empty());
  EXPECT_TRUE(cellular_->sim_present());

  capability_->sim_lock_status_.lock_type.clear();
  cellular_->set_sim_present(false);
  capability_->get_imsi_retries_ = 0;
  EXPECT_EQ(CellularCapabilityGSM::kGetIMSIRetryDelayMilliseconds,
            capability_->get_imsi_retry_delay_milliseconds_);

  // Set the delay to zero to speed up the test.
  capability_->get_imsi_retry_delay_milliseconds_ = 0;
  capability_->GetIMSI(callback);
  for (int i = 0; i < CellularCapabilityGSM::kGetIMSIRetryLimit; ++i) {
    dispatcher_.DispatchPendingEvents();
  }
  EXPECT_EQ(CellularCapabilityGSM::kGetIMSIRetryLimit + 1,
            capability_->get_imsi_retries_);
  EXPECT_TRUE(cellular_->imsi().empty());
  EXPECT_FALSE(cellular_->sim_present());
}

TEST_F(CellularCapabilityGSMTest, GetMSISDN) {
  EXPECT_CALL(*card_proxy_, GetMSISDN(_, _,
                                      CellularCapability::kTimeoutDefault))
      .WillOnce(Invoke(this,
                       &CellularCapabilityGSMTest::InvokeGetMSISDN));
  EXPECT_CALL(*this, TestCallback(IsSuccess()));
  SetCardProxy();
  ASSERT_TRUE(cellular_->mdn().empty());
  capability_->GetMSISDN(Bind(&CellularCapabilityGSMTest::TestCallback,
                            Unretained(this)));
  EXPECT_EQ(kMSISDN, cellular_->mdn());
}

TEST_F(CellularCapabilityGSMTest, GetSPN) {
  EXPECT_CALL(*card_proxy_, GetSPN(_, _, CellularCapability::kTimeoutDefault))
      .WillOnce(Invoke(this,
                       &CellularCapabilityGSMTest::InvokeGetSPN));
  EXPECT_CALL(*this, TestCallback(IsSuccess()));
  SetCardProxy();
  ASSERT_TRUE(capability_->spn_.empty());
  capability_->GetSPN(Bind(&CellularCapabilityGSMTest::TestCallback,
                            Unretained(this)));
  EXPECT_EQ(kTestCarrier, capability_->spn_);
}

TEST_F(CellularCapabilityGSMTest, GetSignalQuality) {
  EXPECT_CALL(*network_proxy_,
              GetSignalQuality(_, _, CellularCapability::kTimeoutDefault))
      .WillOnce(Invoke(this,
                       &CellularCapabilityGSMTest::InvokeGetSignalQuality));
  SetNetworkProxy();
  CreateService();
  EXPECT_EQ(0, cellular_->service()->strength());
  capability_->GetSignalQuality();
  EXPECT_EQ(kStrength, cellular_->service()->strength());
}

TEST_F(CellularCapabilityGSMTest, RegisterOnNetwork) {
  EXPECT_CALL(*network_proxy_, Register(kTestNetwork, _, _,
                                        CellularCapability::kTimeoutRegister))
      .WillOnce(Invoke(this, &CellularCapabilityGSMTest::InvokeRegister));
  EXPECT_CALL(*this, TestCallback(IsSuccess()));
  SetNetworkProxy();
  Error error;
  capability_->RegisterOnNetwork(kTestNetwork, &error,
                                 Bind(&CellularCapabilityGSMTest::TestCallback,
                                      Unretained(this)));
  EXPECT_EQ(kTestNetwork, cellular_->selected_network());
}

TEST_F(CellularCapabilityGSMTest, IsRegistered) {
  EXPECT_FALSE(capability_->IsRegistered());
  SetRegistrationState(MM_MODEM_GSM_NETWORK_REG_STATUS_IDLE);
  EXPECT_FALSE(capability_->IsRegistered());
  SetRegistrationState(MM_MODEM_GSM_NETWORK_REG_STATUS_HOME);
  EXPECT_TRUE(capability_->IsRegistered());
  SetRegistrationState(MM_MODEM_GSM_NETWORK_REG_STATUS_SEARCHING);
  EXPECT_FALSE(capability_->IsRegistered());
  SetRegistrationState(MM_MODEM_GSM_NETWORK_REG_STATUS_DENIED);
  EXPECT_FALSE(capability_->IsRegistered());
  SetRegistrationState(MM_MODEM_GSM_NETWORK_REG_STATUS_UNKNOWN);
  EXPECT_FALSE(capability_->IsRegistered());
  SetRegistrationState(MM_MODEM_GSM_NETWORK_REG_STATUS_ROAMING);
  EXPECT_TRUE(capability_->IsRegistered());
}

TEST_F(CellularCapabilityGSMTest, GetRegistrationState) {
  ASSERT_FALSE(capability_->IsRegistered());
  EXPECT_CALL(*network_proxy_,
              GetRegistrationInfo(_, _, CellularCapability::kTimeoutDefault))
      .WillOnce(Invoke(this,
                       &CellularCapabilityGSMTest::InvokeGetRegistrationInfo));
  SetNetworkProxy();
  capability_->GetRegistrationState();
  EXPECT_TRUE(capability_->IsRegistered());
  EXPECT_EQ(MM_MODEM_GSM_NETWORK_REG_STATUS_HOME,
            capability_->registration_state_);
}

TEST_F(CellularCapabilityGSMTest, RequirePIN) {
  EXPECT_CALL(*card_proxy_, EnablePIN(kPIN, true, _, _,
                                      CellularCapability::kTimeoutDefault))
      .WillOnce(Invoke(this, &CellularCapabilityGSMTest::InvokeEnablePIN));
  EXPECT_CALL(*this, TestCallback(IsSuccess()));
  SetCardProxy();
  Error error;
  capability_->RequirePIN(kPIN, true, &error,
                          Bind(&CellularCapabilityGSMTest::TestCallback,
                               Unretained(this)));
  EXPECT_TRUE(error.IsSuccess());
}

TEST_F(CellularCapabilityGSMTest, EnterPIN) {
  EXPECT_CALL(*card_proxy_, SendPIN(kPIN, _, _,
                                    CellularCapability::kTimeoutDefault))
      .WillOnce(Invoke(this, &CellularCapabilityGSMTest::InvokeSendPIN));
  EXPECT_CALL(*this, TestCallback(IsSuccess()));
  SetCardProxy();
  Error error;
  capability_->EnterPIN(kPIN, &error,
                        Bind(&CellularCapabilityGSMTest::TestCallback,
                             Unretained(this)));
  EXPECT_TRUE(error.IsSuccess());
}

TEST_F(CellularCapabilityGSMTest, UnblockPIN) {
  EXPECT_CALL(*card_proxy_, SendPUK(kPUK, kPIN, _, _,
                                    CellularCapability::kTimeoutDefault))
      .WillOnce(Invoke(this, &CellularCapabilityGSMTest::InvokeSendPUK));
  EXPECT_CALL(*this, TestCallback(IsSuccess()));
  SetCardProxy();
  Error error;
  capability_->UnblockPIN(kPUK, kPIN, &error,
                          Bind(&CellularCapabilityGSMTest::TestCallback,
                             Unretained(this)));
  EXPECT_TRUE(error.IsSuccess());
}

TEST_F(CellularCapabilityGSMTest, ChangePIN) {
  static const char kOldPIN[] = "1111";
  EXPECT_CALL(*card_proxy_, ChangePIN(kOldPIN, kPIN, _, _,
                                    CellularCapability::kTimeoutDefault))
      .WillOnce(Invoke(this, &CellularCapabilityGSMTest::InvokeChangePIN));
  EXPECT_CALL(*this, TestCallback(IsSuccess()));
  SetCardProxy();
  Error error;
  capability_->ChangePIN(kOldPIN, kPIN, &error,
                         Bind(&CellularCapabilityGSMTest::TestCallback,
                             Unretained(this)));
  EXPECT_TRUE(error.IsSuccess());
}


TEST_F(CellularCapabilityGSMTest, ParseScanResult) {
  static const char kID[] = "123";
  static const char kLongName[] = "long name";
  static const char kShortName[] = "short name";
  GSMScanResult result;
  result[CellularCapabilityGSM::kNetworkPropertyStatus] = "1";
  result[CellularCapabilityGSM::kNetworkPropertyID] = kID;
  result[CellularCapabilityGSM::kNetworkPropertyLongName] = kLongName;
  result[CellularCapabilityGSM::kNetworkPropertyShortName] = kShortName;
  result[CellularCapabilityGSM::kNetworkPropertyAccessTechnology] = "3";
  result["unknown property"] = "random value";
  Stringmap parsed = capability_->ParseScanResult(result);
  EXPECT_EQ(5, parsed.size());
  EXPECT_EQ("available", parsed[kStatusProperty]);
  EXPECT_EQ(kID, parsed[kNetworkIdProperty]);
  EXPECT_EQ(kLongName, parsed[kLongNameProperty]);
  EXPECT_EQ(kShortName, parsed[kShortNameProperty]);
  EXPECT_EQ(kNetworkTechnologyEdge, parsed[kTechnologyProperty]);
}

TEST_F(CellularCapabilityGSMTest, ParseScanResultProviderLookup) {
  static const char kID[] = "10001";
  const string kLongName = "TestNetworkLongName";
  // Replace the |MobileOperatorInfo| used by |ParseScanResult| by a mock.
  auto* mock_mobile_operator_info = new MockMobileOperatorInfo(
      &dispatcher_,
      "MockParseScanResult");
  capability_->mobile_operator_info_.reset(mock_mobile_operator_info);

  mock_mobile_operator_info->SetEmptyDefaultsForProperties();
  EXPECT_CALL(*mock_mobile_operator_info, UpdateMCCMNC(kID));
  EXPECT_CALL(*mock_mobile_operator_info, IsMobileNetworkOperatorKnown()).
      WillOnce(Return(true));
  EXPECT_CALL(*mock_mobile_operator_info, operator_name()).
      WillRepeatedly(ReturnRef(kLongName));
  GSMScanResult result;
  result[CellularCapabilityGSM::kNetworkPropertyID] = kID;
  Stringmap parsed = capability_->ParseScanResult(result);
  EXPECT_EQ(2, parsed.size());
  EXPECT_EQ(kID, parsed[kNetworkIdProperty]);
  EXPECT_EQ(kLongName, parsed[kLongNameProperty]);
}

TEST_F(CellularCapabilityGSMTest, SetAccessTechnology) {
  capability_->SetAccessTechnology(MM_MODEM_GSM_ACCESS_TECH_GSM);
  EXPECT_EQ(MM_MODEM_GSM_ACCESS_TECH_GSM, capability_->access_technology_);
  CreateService();
  SetRegistrationState(MM_MODEM_GSM_NETWORK_REG_STATUS_HOME);
  capability_->SetAccessTechnology(MM_MODEM_GSM_ACCESS_TECH_GPRS);
  EXPECT_EQ(MM_MODEM_GSM_ACCESS_TECH_GPRS, capability_->access_technology_);
  EXPECT_EQ(kNetworkTechnologyGprs, cellular_->service()->network_technology());
}

TEST_F(CellularCapabilityGSMTest, AllowRoaming) {
  EXPECT_FALSE(cellular_->allow_roaming_);
  EXPECT_FALSE(cellular_->provider_requires_roaming());
  EXPECT_FALSE(capability_->AllowRoaming());
  cellular_->set_provider_requires_roaming(true);
  EXPECT_TRUE(capability_->AllowRoaming());
  cellular_->set_provider_requires_roaming(false);
  cellular_->allow_roaming_ = true;
  EXPECT_TRUE(capability_->AllowRoaming());
}

TEST_F(CellularCapabilityGSMTest, GetNetworkTechnologyString) {
  EXPECT_EQ("", capability_->GetNetworkTechnologyString());
  SetAccessTechnology(MM_MODEM_GSM_ACCESS_TECH_GSM);
  EXPECT_EQ(kNetworkTechnologyGsm, capability_->GetNetworkTechnologyString());
  SetAccessTechnology(MM_MODEM_GSM_ACCESS_TECH_GSM_COMPACT);
  EXPECT_EQ(kNetworkTechnologyGsm, capability_->GetNetworkTechnologyString());
  SetAccessTechnology(MM_MODEM_GSM_ACCESS_TECH_GPRS);
  EXPECT_EQ(kNetworkTechnologyGprs, capability_->GetNetworkTechnologyString());
  SetAccessTechnology(MM_MODEM_GSM_ACCESS_TECH_EDGE);
  EXPECT_EQ(kNetworkTechnologyEdge, capability_->GetNetworkTechnologyString());
  SetAccessTechnology(MM_MODEM_GSM_ACCESS_TECH_UMTS);
  EXPECT_EQ(kNetworkTechnologyUmts, capability_->GetNetworkTechnologyString());
  SetAccessTechnology(MM_MODEM_GSM_ACCESS_TECH_HSDPA);
  EXPECT_EQ(kNetworkTechnologyHspa, capability_->GetNetworkTechnologyString());
  SetAccessTechnology(MM_MODEM_GSM_ACCESS_TECH_HSUPA);
  EXPECT_EQ(kNetworkTechnologyHspa, capability_->GetNetworkTechnologyString());
  SetAccessTechnology(MM_MODEM_GSM_ACCESS_TECH_HSPA);
  EXPECT_EQ(kNetworkTechnologyHspa, capability_->GetNetworkTechnologyString());
  SetAccessTechnology(MM_MODEM_GSM_ACCESS_TECH_HSPA_PLUS);
  EXPECT_EQ(kNetworkTechnologyHspaPlus,
            capability_->GetNetworkTechnologyString());
}

TEST_F(CellularCapabilityGSMTest, GetRoamingStateString) {
  EXPECT_EQ(kRoamingStateUnknown, capability_->GetRoamingStateString());
  SetRegistrationState(MM_MODEM_GSM_NETWORK_REG_STATUS_HOME);
  EXPECT_EQ(kRoamingStateHome, capability_->GetRoamingStateString());
  SetRegistrationState(MM_MODEM_GSM_NETWORK_REG_STATUS_ROAMING);
  EXPECT_EQ(kRoamingStateRoaming, capability_->GetRoamingStateString());
  SetRegistrationState(MM_MODEM_GSM_NETWORK_REG_STATUS_SEARCHING);
  EXPECT_EQ(kRoamingStateUnknown, capability_->GetRoamingStateString());
  SetRegistrationState(MM_MODEM_GSM_NETWORK_REG_STATUS_DENIED);
  EXPECT_EQ(kRoamingStateUnknown, capability_->GetRoamingStateString());
  SetRegistrationState(MM_MODEM_GSM_NETWORK_REG_STATUS_IDLE);
  EXPECT_EQ(kRoamingStateUnknown, capability_->GetRoamingStateString());
}

TEST_F(CellularCapabilityGSMTest, OnPropertiesChanged) {
  EXPECT_EQ(MM_MODEM_GSM_ACCESS_TECH_UNKNOWN, capability_->access_technology_);
  EXPECT_FALSE(capability_->sim_lock_status_.enabled);
  EXPECT_EQ("", capability_->sim_lock_status_.lock_type);
  EXPECT_EQ(0, capability_->sim_lock_status_.retries_left);
  KeyValueStore props;
  static const char kLockType[] = "sim-pin";
  const int kRetries = 3;
  props.SetUint(CellularCapabilityGSM::kPropertyAccessTechnology,
                MM_MODEM_GSM_ACCESS_TECH_EDGE);
  props.SetUint(CellularCapabilityGSM::kPropertyEnabledFacilityLocks,
                MM_MODEM_GSM_FACILITY_SIM);
  props.SetString(CellularCapabilityGSM::kPropertyUnlockRequired, kLockType);
  props.SetUint(CellularCapabilityGSM::kPropertyUnlockRetries, kRetries);
  // Call with the 'wrong' interface and nothing should change.
  capability_->OnPropertiesChanged(MM_MODEM_GSM_INTERFACE, props,
                                   vector<string>());
  EXPECT_EQ(MM_MODEM_GSM_ACCESS_TECH_UNKNOWN, capability_->access_technology_);
  EXPECT_FALSE(capability_->sim_lock_status_.enabled);
  EXPECT_EQ("", capability_->sim_lock_status_.lock_type);
  EXPECT_EQ(0, capability_->sim_lock_status_.retries_left);

  // Call with the MM_MODEM_GSM_NETWORK_INTERFACE interface and expect a change
  // to the enabled state of the SIM lock.
  KeyValueStore lock_status;
  lock_status.SetBool(kSIMLockEnabledProperty, true);
  lock_status.SetString(kSIMLockTypeProperty, "");
  lock_status.SetUint(kSIMLockRetriesLeftProperty, 0);

  EXPECT_CALL(*device_adaptor_, EmitKeyValueStoreChanged(
      kSIMLockStatusProperty,
      KeyValueStoreEq(lock_status)));

  capability_->OnPropertiesChanged(MM_MODEM_GSM_NETWORK_INTERFACE, props,
                                   vector<string>());
  EXPECT_EQ(MM_MODEM_GSM_ACCESS_TECH_EDGE, capability_->access_technology_);
  capability_->OnPropertiesChanged(MM_MODEM_GSM_CARD_INTERFACE, props,
                                   vector<string>());
  EXPECT_TRUE(capability_->sim_lock_status_.enabled);
  EXPECT_TRUE(capability_->sim_lock_status_.lock_type.empty());
  EXPECT_EQ(0, capability_->sim_lock_status_.retries_left);

  // Some properties are sent on the MM_MODEM_INTERFACE.
  capability_->sim_lock_status_.enabled = false;
  capability_->sim_lock_status_.lock_type = "";
  capability_->sim_lock_status_.retries_left = 0;
  KeyValueStore lock_status2;
  lock_status2.SetBool(kSIMLockEnabledProperty, false);
  lock_status2.SetString(kSIMLockTypeProperty, kLockType);
  lock_status2.SetUint(kSIMLockRetriesLeftProperty, kRetries);
  EXPECT_CALL(*device_adaptor_,
              EmitKeyValueStoreChanged(kSIMLockStatusProperty,
                                       KeyValueStoreEq(lock_status2)));
  capability_->OnPropertiesChanged(MM_MODEM_INTERFACE, props,
                                   vector<string>());
  EXPECT_FALSE(capability_->sim_lock_status_.enabled);
  EXPECT_EQ(kLockType, capability_->sim_lock_status_.lock_type);
  EXPECT_EQ(kRetries, capability_->sim_lock_status_.retries_left);
}

TEST_F(CellularCapabilityGSMTest, StartModemSuccess) {
  SetupCommonStartModemExpectations();
  EXPECT_CALL(*card_proxy_,
              GetSPN(_, _, CellularCapability::kTimeoutDefault))
      .WillOnce(Invoke(this, &CellularCapabilityGSMTest::InvokeGetSPN));
  EXPECT_CALL(*card_proxy_,
              GetMSISDN(_, _, CellularCapability::kTimeoutDefault))
      .WillOnce(Invoke(this, &CellularCapabilityGSMTest::InvokeGetMSISDN));
  AllowCreateCardProxyFromFactory();

  Error error;
  capability_->StartModem(
      &error, Bind(&CellularCapabilityGSMTest::TestCallback, Unretained(this)));
  dispatcher_.DispatchPendingEvents();
}

TEST_F(CellularCapabilityGSMTest, StartModemGetSPNFail) {
  SetupCommonStartModemExpectations();
  EXPECT_CALL(*card_proxy_,
              GetSPN(_, _, CellularCapability::kTimeoutDefault))
      .WillOnce(Invoke(this, &CellularCapabilityGSMTest::InvokeGetSPNFail));
  EXPECT_CALL(*card_proxy_,
              GetMSISDN(_, _, CellularCapability::kTimeoutDefault))
      .WillOnce(Invoke(this, &CellularCapabilityGSMTest::InvokeGetMSISDN));
  AllowCreateCardProxyFromFactory();

  Error error;
  capability_->StartModem(
      &error, Bind(&CellularCapabilityGSMTest::TestCallback, Unretained(this)));
  dispatcher_.DispatchPendingEvents();
}

TEST_F(CellularCapabilityGSMTest, StartModemGetMSISDNFail) {
  SetupCommonStartModemExpectations();
  EXPECT_CALL(*card_proxy_,
              GetSPN(_, _, CellularCapability::kTimeoutDefault))
      .WillOnce(Invoke(this, &CellularCapabilityGSMTest::InvokeGetSPN));
  EXPECT_CALL(*card_proxy_,
              GetMSISDN(_, _, CellularCapability::kTimeoutDefault))
      .WillOnce(Invoke(this, &CellularCapabilityGSMTest::InvokeGetMSISDNFail));
  AllowCreateCardProxyFromFactory();

  Error error;
  capability_->StartModem(
      &error, Bind(&CellularCapabilityGSMTest::TestCallback, Unretained(this)));
  dispatcher_.DispatchPendingEvents();
}

TEST_F(CellularCapabilityGSMTest, ConnectFailureNoService) {
  // Make sure we don't crash if the connect failed and there is no
  // CellularService object.  This can happen if the modem is enabled and
  // then quickly disabled.
  SetupCommonProxiesExpectations();
  EXPECT_CALL(*simple_proxy_,
              Connect(_, _, _, CellularCapabilityGSM::kTimeoutConnect))
       .WillOnce(Invoke(this, &CellularCapabilityGSMTest::InvokeConnectFail));
  EXPECT_CALL(*this, TestCallback(IsFailure()));
  InitProxies();
  EXPECT_FALSE(capability_->cellular()->service());
  Error error;
  KeyValueStore props;
  capability_->Connect(props, &error,
                       Bind(&CellularCapabilityGSMTest::TestCallback,
                            Unretained(this)));
}

}  // namespace shill
