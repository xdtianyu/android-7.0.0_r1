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

#include "shill/cellular/cellular_capability_universal_cdma.h"

#include <string>
#include <vector>

#include <base/strings/string_number_conversions.h>
#include <base/strings/string_util.h>
#include <base/strings/stringprintf.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <ModemManager/ModemManager.h>

#include "shill/cellular/cellular.h"
#include "shill/cellular/cellular_service.h"
#include "shill/cellular/mock_cellular_service.h"
#include "shill/cellular/mock_mm1_modem_modem3gpp_proxy.h"
#include "shill/cellular/mock_mm1_modem_modemcdma_proxy.h"
#include "shill/cellular/mock_mm1_modem_proxy.h"
#include "shill/cellular/mock_mm1_modem_simple_proxy.h"
#include "shill/cellular/mock_mm1_sim_proxy.h"
#include "shill/cellular/mock_mobile_operator_info.h"
#include "shill/cellular/mock_modem_info.h"
#include "shill/mock_adaptors.h"
#include "shill/mock_control.h"
#include "shill/mock_dbus_properties_proxy.h"
#include "shill/mock_manager.h"
#include "shill/mock_metrics.h"
#include "shill/mock_pending_activation_store.h"
#include "shill/nice_mock_control.h"
#include "shill/test_event_dispatcher.h"

using base::StringPrintf;
using base::UintToString;
using std::string;
using std::unique_ptr;
using std::vector;
using testing::Invoke;
using testing::Mock;
using testing::NiceMock;
using testing::Return;
using testing::SetArgumentPointee;
using testing::_;

namespace shill {

class CellularCapabilityUniversalCDMATest : public testing::Test {
 public:
  explicit CellularCapabilityUniversalCDMATest(EventDispatcher* dispatcher)
      : dispatcher_(dispatcher),
        control_interface_(this),
        capability_(nullptr),
        device_adaptor_(nullptr),
        modem_info_(&control_interface_, dispatcher, nullptr, nullptr),
        modem_3gpp_proxy_(new mm1::MockModemModem3gppProxy()),
        modem_cdma_proxy_(new mm1::MockModemModemCdmaProxy()),
        modem_proxy_(new mm1::MockModemProxy()),
        modem_simple_proxy_(new mm1::MockModemSimpleProxy()),
        sim_proxy_(new mm1::MockSimProxy()),
        properties_proxy_(new MockDBusPropertiesProxy()),
        cellular_(new Cellular(&modem_info_,
                               "",
                               kMachineAddress,
                               0,
                               Cellular::kTypeUniversalCDMA,
                               "",
                               "")),
        service_(new MockCellularService(&modem_info_,
                                         cellular_)),
        mock_home_provider_info_(nullptr),
        mock_serving_operator_info_(nullptr) {}

  virtual ~CellularCapabilityUniversalCDMATest() {
    cellular_->service_ = nullptr;
    capability_ = nullptr;
    device_adaptor_ = nullptr;
  }

  virtual void SetUp() {
    capability_ = static_cast<CellularCapabilityUniversalCDMA*>(
        cellular_->capability_.get());
    device_adaptor_ =
        static_cast<NiceMock<DeviceMockAdaptor>*>(cellular_->adaptor());
    cellular_->service_ = service_;
  }

  virtual void TearDown() {
    capability_->control_interface_ = nullptr;
  }

  void SetService() {
    cellular_->service_ = new CellularService(&modem_info_, cellular_);
  }

  void ClearService() {
    cellular_->service_ = nullptr;
  }

  void ReleaseCapabilityProxies() {
    capability_->ReleaseProxies();
  }

  void SetCdmaProxy() {
    capability_->modem_cdma_proxy_.reset(modem_cdma_proxy_.release());
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

 protected:
  static const char kEsn[];
  static const char kMachineAddress[];
  static const char kMeid[];

  class TestControl : public MockControl {
   public:
    explicit TestControl(CellularCapabilityUniversalCDMATest* test)
        : test_(test) {}

    // TODO(armansito): Some of these methods won't be necessary after 3GPP
    // gets refactored out of CellularCapabilityUniversal.
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
      return test_->sim_proxy_.release();
    }

    virtual DBusPropertiesProxyInterface* CreateDBusPropertiesProxy(
        const std::string& /*path*/,
        const std::string& /*service*/) {
      return test_->properties_proxy_.release();
    }

   private:
    CellularCapabilityUniversalCDMATest* test_;
  };

  EventDispatcher* dispatcher_;
  TestControl control_interface_;
  CellularCapabilityUniversalCDMA* capability_;
  NiceMock<DeviceMockAdaptor>* device_adaptor_;
  MockModemInfo modem_info_;
  // TODO(armansito): Remove |modem_3gpp_proxy_| after refactor.
  unique_ptr<mm1::MockModemModem3gppProxy> modem_3gpp_proxy_;
  unique_ptr<mm1::MockModemModemCdmaProxy> modem_cdma_proxy_;
  unique_ptr<mm1::MockModemProxy> modem_proxy_;
  unique_ptr<mm1::MockModemSimpleProxy> modem_simple_proxy_;
  unique_ptr<mm1::MockSimProxy> sim_proxy_;
  unique_ptr<MockDBusPropertiesProxy> properties_proxy_;
  CellularRefPtr cellular_;
  MockCellularService* service_;

  // Set when required and passed to |cellular_|. Owned by |cellular_|.
  MockMobileOperatorInfo* mock_home_provider_info_;
  MockMobileOperatorInfo* mock_serving_operator_info_;
};

// static
const char CellularCapabilityUniversalCDMATest::kEsn[] = "0000";
// static
const char CellularCapabilityUniversalCDMATest::kMachineAddress[] =
    "TestMachineAddress";
// static
const char CellularCapabilityUniversalCDMATest::kMeid[] = "11111111111111";

class CellularCapabilityUniversalCDMAMainTest
    : public CellularCapabilityUniversalCDMATest {
 public:
  CellularCapabilityUniversalCDMAMainTest()
      : CellularCapabilityUniversalCDMATest(&dispatcher_) {}

 private:
  EventDispatcherForTest dispatcher_;
};

class CellularCapabilityUniversalCDMADispatcherTest
    : public CellularCapabilityUniversalCDMATest {
 public:
  CellularCapabilityUniversalCDMADispatcherTest()
      : CellularCapabilityUniversalCDMATest(nullptr) {}
};

TEST_F(CellularCapabilityUniversalCDMAMainTest, PropertiesChanged) {
  // Set up mock modem CDMA properties.
  KeyValueStore modem_cdma_properties;
  modem_cdma_properties.SetString(MM_MODEM_MODEMCDMA_PROPERTY_MEID, kMeid);
  modem_cdma_properties.SetString(MM_MODEM_MODEMCDMA_PROPERTY_ESN, kEsn);

  SetUp();

  EXPECT_TRUE(cellular_->meid().empty());
  EXPECT_TRUE(cellular_->esn().empty());

  // Changing properties on wrong interface will not have an effect
  capability_->OnPropertiesChanged(MM_DBUS_INTERFACE_MODEM,
                                   modem_cdma_properties,
                                   vector<string>());
  EXPECT_TRUE(cellular_->meid().empty());
  EXPECT_TRUE(cellular_->esn().empty());

  // Changing properties on the right interface gets reflected in the
  // capabilities object
  capability_->OnPropertiesChanged(MM_DBUS_INTERFACE_MODEM_MODEMCDMA,
                                   modem_cdma_properties,
                                   vector<string>());
  EXPECT_EQ(kMeid, cellular_->meid());
  EXPECT_EQ(kEsn, cellular_->esn());
}

TEST_F(CellularCapabilityUniversalCDMAMainTest, OnCDMARegistrationChanged) {
  EXPECT_EQ(0, capability_->sid_);
  EXPECT_EQ(0, capability_->nid_);
  EXPECT_EQ(MM_MODEM_CDMA_REGISTRATION_STATE_UNKNOWN,
            capability_->cdma_1x_registration_state_);
  EXPECT_EQ(MM_MODEM_CDMA_REGISTRATION_STATE_UNKNOWN,
            capability_->cdma_evdo_registration_state_);

  const unsigned kSid = 2;
  const unsigned kNid = 1;
  SetMockMobileOperatorInfoObjects();
  EXPECT_CALL(*mock_serving_operator_info_, UpdateSID(UintToString(kSid)));
  EXPECT_CALL(*mock_serving_operator_info_, UpdateNID(UintToString(kNid)));
  capability_->OnCDMARegistrationChanged(
      MM_MODEM_CDMA_REGISTRATION_STATE_UNKNOWN,
      MM_MODEM_CDMA_REGISTRATION_STATE_HOME,
      kSid,
      kNid);
  EXPECT_EQ(kSid, capability_->sid_);
  EXPECT_EQ(kNid, capability_->nid_);
  EXPECT_EQ(MM_MODEM_CDMA_REGISTRATION_STATE_UNKNOWN,
            capability_->cdma_1x_registration_state_);
  EXPECT_EQ(MM_MODEM_CDMA_REGISTRATION_STATE_HOME,
            capability_->cdma_evdo_registration_state_);

  EXPECT_TRUE(capability_->IsRegistered());
}

TEST_F(CellularCapabilityUniversalCDMAMainTest, UpdateServiceOLP) {
  const MobileOperatorInfo::OnlinePortal kOlp {
      "http://testurl",
      "POST",
      "esn=${esn}&mdn=${mdn}&meid=${meid}"};
  const vector<MobileOperatorInfo::OnlinePortal> kOlpList {kOlp};
  const string kUuidVzw = "c83d6597-dc91-4d48-a3a7-d86b80123751";
  const string kUuidFoo = "foo";

  SetMockMobileOperatorInfoObjects();
  cellular_->set_esn("0");
  cellular_->set_mdn("10123456789");
  cellular_->set_meid("4");


  mock_serving_operator_info_->SetEmptyDefaultsForProperties();
  EXPECT_CALL(*mock_serving_operator_info_, IsMobileNetworkOperatorKnown())
      .WillRepeatedly(Return(true));
  EXPECT_CALL(*mock_serving_operator_info_, olp_list())
      .WillRepeatedly(ReturnRef(kOlpList));
  EXPECT_CALL(*mock_serving_operator_info_, uuid())
      .WillOnce(ReturnRef(kUuidVzw));
  SetService();
  capability_->UpdateServiceOLP();
  // Copy to simplify assertions below.
  Stringmap vzw_olp = cellular_->service()->olp();
  EXPECT_EQ("http://testurl", vzw_olp[kPaymentPortalURL]);
  EXPECT_EQ("POST", vzw_olp[kPaymentPortalMethod]);
  EXPECT_EQ("esn=0&mdn=0123456789&meid=4",
            vzw_olp[kPaymentPortalPostData]);
  Mock::VerifyAndClearExpectations(mock_serving_operator_info_);

  mock_serving_operator_info_->SetEmptyDefaultsForProperties();
  EXPECT_CALL(*mock_serving_operator_info_, IsMobileNetworkOperatorKnown())
      .WillRepeatedly(Return(true));
  EXPECT_CALL(*mock_serving_operator_info_, olp_list())
      .WillRepeatedly(ReturnRef(kOlpList));
  EXPECT_CALL(*mock_serving_operator_info_, uuid())
      .WillOnce(ReturnRef(kUuidFoo));
  capability_->UpdateServiceOLP();
  // Copy to simplify assertions below.
  Stringmap olp = cellular_->service()->olp();
  EXPECT_EQ("http://testurl", olp[kPaymentPortalURL]);
  EXPECT_EQ("POST", olp[kPaymentPortalMethod]);
  EXPECT_EQ("esn=0&mdn=10123456789&meid=4",
            olp[kPaymentPortalPostData]);
}

TEST_F(CellularCapabilityUniversalCDMAMainTest, ActivateAutomatic) {
  const string activation_code {"1234"};
  SetMockMobileOperatorInfoObjects();

  mm1::MockModemModemCdmaProxy* cdma_proxy = modem_cdma_proxy_.get();
  SetUp();
  capability_->InitProxies();

  // Cases when activation fails because |activation_code| is not available.
  EXPECT_CALL(*mock_serving_operator_info_, IsMobileNetworkOperatorKnown())
      .WillRepeatedly(Return(false));
  EXPECT_CALL(*cdma_proxy, Activate(_, _, _, _)).Times(0);
  capability_->ActivateAutomatic();
  Mock::VerifyAndClearExpectations(mock_serving_operator_info_);
  Mock::VerifyAndClearExpectations(modem_cdma_proxy_.get());
  EXPECT_CALL(*mock_serving_operator_info_, IsMobileNetworkOperatorKnown())
      .WillRepeatedly(Return(true));
  mock_serving_operator_info_->SetEmptyDefaultsForProperties();
  EXPECT_CALL(*cdma_proxy, Activate(_, _, _, _)).Times(0);
  capability_->ActivateAutomatic();
  Mock::VerifyAndClearExpectations(mock_serving_operator_info_);
  Mock::VerifyAndClearExpectations(modem_cdma_proxy_.get());

  // These expectations hold for all subsequent tests.
  EXPECT_CALL(*mock_serving_operator_info_, IsMobileNetworkOperatorKnown())
      .WillRepeatedly(Return(true));
  EXPECT_CALL(*mock_serving_operator_info_, activation_code())
      .WillRepeatedly(ReturnRef(activation_code));

  EXPECT_CALL(*modem_info_.mock_pending_activation_store(),
              GetActivationState(PendingActivationStore::kIdentifierMEID, _))
      .WillOnce(Return(PendingActivationStore::kStatePending))
      .WillOnce(Return(PendingActivationStore::kStateActivated));
  EXPECT_CALL(*modem_info_.mock_pending_activation_store(),
              SetActivationState(_, _, _))
      .Times(0);
  EXPECT_CALL(*cdma_proxy, Activate(_, _, _, _)).Times(0);
  capability_->ActivateAutomatic();
  capability_->ActivateAutomatic();
  Mock::VerifyAndClearExpectations(modem_info_.mock_pending_activation_store());
  Mock::VerifyAndClearExpectations(modem_cdma_proxy_.get());

  EXPECT_CALL(*modem_info_.mock_pending_activation_store(),
              GetActivationState(PendingActivationStore::kIdentifierMEID, _))
      .WillOnce(Return(PendingActivationStore::kStateUnknown))
      .WillOnce(Return(PendingActivationStore::kStateFailureRetry));
  EXPECT_CALL(*modem_info_.mock_pending_activation_store(),
              SetActivationState(_, _, PendingActivationStore::kStatePending))
      .Times(2);
  EXPECT_CALL(*cdma_proxy, Activate(_, _, _, _)).Times(2);
  capability_->ActivateAutomatic();
  capability_->ActivateAutomatic();
  Mock::VerifyAndClearExpectations(modem_info_.mock_pending_activation_store());
  Mock::VerifyAndClearExpectations(modem_cdma_proxy_.get());
}

TEST_F(CellularCapabilityUniversalCDMAMainTest, IsServiceActivationRequired) {
  const vector<MobileOperatorInfo::OnlinePortal> empty_list;
  const vector<MobileOperatorInfo::OnlinePortal> olp_list {
    {"some@url", "some_method", "some_post_data"}
  };
  SetMockMobileOperatorInfoObjects();

  capability_->activation_state_ =
      MM_MODEM_CDMA_ACTIVATION_STATE_NOT_ACTIVATED;
  EXPECT_CALL(*mock_serving_operator_info_, IsMobileNetworkOperatorKnown())
      .WillRepeatedly(Return(false));
  EXPECT_FALSE(capability_->IsServiceActivationRequired());
  Mock::VerifyAndClearExpectations(mock_serving_operator_info_);

  capability_->activation_state_ =
      MM_MODEM_CDMA_ACTIVATION_STATE_NOT_ACTIVATED;
  EXPECT_CALL(*mock_serving_operator_info_, IsMobileNetworkOperatorKnown())
      .WillRepeatedly(Return(true));
  EXPECT_CALL(*mock_serving_operator_info_, olp_list())
      .WillRepeatedly(ReturnRef(empty_list));
  EXPECT_FALSE(capability_->IsServiceActivationRequired());
  Mock::VerifyAndClearExpectations(mock_serving_operator_info_);

  // These expectations hold for all subsequent tests.
  EXPECT_CALL(*mock_serving_operator_info_, IsMobileNetworkOperatorKnown())
      .WillRepeatedly(Return(true));
  EXPECT_CALL(*mock_serving_operator_info_, olp_list())
      .WillRepeatedly(ReturnRef(olp_list));

  capability_->activation_state_ =
      MM_MODEM_CDMA_ACTIVATION_STATE_NOT_ACTIVATED;
  EXPECT_TRUE(capability_->IsServiceActivationRequired());
  capability_->activation_state_ =
      MM_MODEM_CDMA_ACTIVATION_STATE_ACTIVATING;
  EXPECT_FALSE(capability_->IsServiceActivationRequired());
  capability_->activation_state_ =
      MM_MODEM_CDMA_ACTIVATION_STATE_ACTIVATED;
  EXPECT_FALSE(capability_->IsServiceActivationRequired());
}

TEST_F(CellularCapabilityUniversalCDMAMainTest,
       UpdateServiceActivationStateProperty) {
  const vector<MobileOperatorInfo::OnlinePortal> olp_list {
    {"some@url", "some_method", "some_post_data"}
  };
  SetMockMobileOperatorInfoObjects();
  EXPECT_CALL(*mock_serving_operator_info_, IsMobileNetworkOperatorKnown())
      .WillRepeatedly(Return(true));
  EXPECT_CALL(*mock_serving_operator_info_, olp_list())
      .WillRepeatedly(ReturnRef(olp_list));

  EXPECT_CALL(*modem_info_.mock_pending_activation_store(),
              GetActivationState(_, _))
      .WillOnce(Return(PendingActivationStore::kStatePending))
      .WillRepeatedly(Return(PendingActivationStore::kStateUnknown));

  capability_->activation_state_ = MM_MODEM_CDMA_ACTIVATION_STATE_NOT_ACTIVATED;
  EXPECT_CALL(*service_, SetActivationState(kActivationStateActivating))
      .Times(1);
  capability_->UpdateServiceActivationStateProperty();
  Mock::VerifyAndClearExpectations(service_);

  EXPECT_CALL(*service_, SetActivationState(kActivationStateNotActivated))
      .Times(1);
  capability_->UpdateServiceActivationStateProperty();
  Mock::VerifyAndClearExpectations(service_);

  capability_->activation_state_ = MM_MODEM_CDMA_ACTIVATION_STATE_ACTIVATING;
  EXPECT_CALL(*service_, SetActivationState(kActivationStateActivating))
      .Times(1);
  capability_->UpdateServiceActivationStateProperty();
  Mock::VerifyAndClearExpectations(service_);

  capability_->activation_state_ = MM_MODEM_CDMA_ACTIVATION_STATE_ACTIVATED;
  EXPECT_CALL(*service_, SetActivationState(kActivationStateActivated))
      .Times(1);
  capability_->UpdateServiceActivationStateProperty();
  Mock::VerifyAndClearExpectations(service_);
  Mock::VerifyAndClearExpectations(modem_info_.mock_pending_activation_store());
}

TEST_F(CellularCapabilityUniversalCDMAMainTest, IsActivating) {
  EXPECT_CALL(*modem_info_.mock_pending_activation_store(),
              GetActivationState(_, _))
      .WillOnce(Return(PendingActivationStore::kStatePending))
      .WillOnce(Return(PendingActivationStore::kStatePending))
      .WillOnce(Return(PendingActivationStore::kStateFailureRetry))
      .WillRepeatedly(Return(PendingActivationStore::kStateUnknown));

  capability_->activation_state_ =
      MM_MODEM_CDMA_ACTIVATION_STATE_NOT_ACTIVATED;
  EXPECT_TRUE(capability_->IsActivating());
  EXPECT_TRUE(capability_->IsActivating());
  capability_->activation_state_ = MM_MODEM_CDMA_ACTIVATION_STATE_ACTIVATING;
  EXPECT_TRUE(capability_->IsActivating());
  EXPECT_TRUE(capability_->IsActivating());
  capability_->activation_state_ =
      MM_MODEM_CDMA_ACTIVATION_STATE_NOT_ACTIVATED;
  EXPECT_FALSE(capability_->IsActivating());
}

TEST_F(CellularCapabilityUniversalCDMAMainTest, IsRegistered) {
  capability_->cdma_1x_registration_state_ =
      MM_MODEM_CDMA_REGISTRATION_STATE_UNKNOWN;
  capability_->cdma_evdo_registration_state_ =
      MM_MODEM_CDMA_REGISTRATION_STATE_UNKNOWN;
  EXPECT_FALSE(capability_->IsRegistered());

  capability_->cdma_evdo_registration_state_ =
      MM_MODEM_CDMA_REGISTRATION_STATE_REGISTERED;
  EXPECT_TRUE(capability_->IsRegistered());

  capability_->cdma_evdo_registration_state_ =
      MM_MODEM_CDMA_REGISTRATION_STATE_HOME;
  EXPECT_TRUE(capability_->IsRegistered());

  capability_->cdma_evdo_registration_state_ =
      MM_MODEM_CDMA_REGISTRATION_STATE_ROAMING;
  EXPECT_TRUE(capability_->IsRegistered());

  capability_->cdma_1x_registration_state_ =
      MM_MODEM_CDMA_REGISTRATION_STATE_REGISTERED;
  capability_->cdma_evdo_registration_state_ =
      MM_MODEM_CDMA_REGISTRATION_STATE_UNKNOWN;
  EXPECT_TRUE(capability_->IsRegistered());

  capability_->cdma_evdo_registration_state_ =
      MM_MODEM_CDMA_REGISTRATION_STATE_REGISTERED;
  EXPECT_TRUE(capability_->IsRegistered());

  capability_->cdma_evdo_registration_state_ =
      MM_MODEM_CDMA_REGISTRATION_STATE_HOME;
  EXPECT_TRUE(capability_->IsRegistered());

  capability_->cdma_evdo_registration_state_ =
      MM_MODEM_CDMA_REGISTRATION_STATE_ROAMING;
  EXPECT_TRUE(capability_->IsRegistered());

  capability_->cdma_1x_registration_state_ =
      MM_MODEM_CDMA_REGISTRATION_STATE_HOME;
  capability_->cdma_evdo_registration_state_ =
      MM_MODEM_CDMA_REGISTRATION_STATE_UNKNOWN;
  EXPECT_TRUE(capability_->IsRegistered());

  capability_->cdma_evdo_registration_state_ =
      MM_MODEM_CDMA_REGISTRATION_STATE_REGISTERED;
  EXPECT_TRUE(capability_->IsRegistered());

  capability_->cdma_evdo_registration_state_ =
      MM_MODEM_CDMA_REGISTRATION_STATE_HOME;
  EXPECT_TRUE(capability_->IsRegistered());

  capability_->cdma_evdo_registration_state_ =
      MM_MODEM_CDMA_REGISTRATION_STATE_ROAMING;
  EXPECT_TRUE(capability_->IsRegistered());

  capability_->cdma_1x_registration_state_ =
      MM_MODEM_CDMA_REGISTRATION_STATE_ROAMING;
  capability_->cdma_evdo_registration_state_ =
      MM_MODEM_CDMA_REGISTRATION_STATE_UNKNOWN;
  EXPECT_TRUE(capability_->IsRegistered());

  capability_->cdma_evdo_registration_state_ =
      MM_MODEM_CDMA_REGISTRATION_STATE_REGISTERED;
  EXPECT_TRUE(capability_->IsRegistered());

  capability_->cdma_evdo_registration_state_ =
      MM_MODEM_CDMA_REGISTRATION_STATE_HOME;
  EXPECT_TRUE(capability_->IsRegistered());

  capability_->cdma_evdo_registration_state_ =
      MM_MODEM_CDMA_REGISTRATION_STATE_ROAMING;
  EXPECT_TRUE(capability_->IsRegistered());
}

TEST_F(CellularCapabilityUniversalCDMAMainTest, SetupConnectProperties) {
  KeyValueStore map;
  capability_->SetupConnectProperties(&map);
  EXPECT_EQ(1, map.properties().size());
  EXPECT_EQ("#777", map.GetString("number"));
}

TEST_F(CellularCapabilityUniversalCDMADispatcherTest,
       UpdatePendingActivationState) {
  capability_->activation_state_ = MM_MODEM_CDMA_ACTIVATION_STATE_ACTIVATED;
  EXPECT_CALL(*modem_info_.mock_pending_activation_store(), RemoveEntry(_, _))
      .Times(1);
  EXPECT_CALL(*modem_info_.mock_pending_activation_store(),
              GetActivationState(_, _))
      .Times(0);
  EXPECT_CALL(*modem_info_.mock_dispatcher(), PostTask(_)).Times(0);
  capability_->UpdatePendingActivationState();
  Mock::VerifyAndClearExpectations(modem_info_.mock_pending_activation_store());
  Mock::VerifyAndClearExpectations(modem_info_.mock_dispatcher());

  capability_->activation_state_ = MM_MODEM_CDMA_ACTIVATION_STATE_ACTIVATING;
  EXPECT_CALL(*modem_info_.mock_pending_activation_store(), RemoveEntry(_, _))
      .Times(0);
  EXPECT_CALL(*modem_info_.mock_pending_activation_store(),
              GetActivationState(_, _))
      .Times(2)
      .WillRepeatedly(Return(PendingActivationStore::kStateUnknown));
  EXPECT_CALL(*modem_info_.mock_dispatcher(), PostTask(_)).Times(0);
  capability_->UpdatePendingActivationState();
  Mock::VerifyAndClearExpectations(modem_info_.mock_pending_activation_store());
  Mock::VerifyAndClearExpectations(modem_info_.mock_dispatcher());

  capability_->activation_state_ = MM_MODEM_CDMA_ACTIVATION_STATE_NOT_ACTIVATED;
  EXPECT_CALL(*modem_info_.mock_pending_activation_store(), RemoveEntry(_, _))
      .Times(0);
  EXPECT_CALL(*modem_info_.mock_pending_activation_store(),
              GetActivationState(_, _))
      .Times(2)
      .WillRepeatedly(Return(PendingActivationStore::kStatePending));
  EXPECT_CALL(*modem_info_.mock_dispatcher(), PostTask(_)).Times(0);
  capability_->UpdatePendingActivationState();
  Mock::VerifyAndClearExpectations(modem_info_.mock_pending_activation_store());
  Mock::VerifyAndClearExpectations(modem_info_.mock_dispatcher());

  EXPECT_CALL(*modem_info_.mock_pending_activation_store(), RemoveEntry(_, _))
      .Times(0);
  EXPECT_CALL(*modem_info_.mock_pending_activation_store(),
              GetActivationState(_, _))
      .Times(2)
      .WillRepeatedly(Return(PendingActivationStore::kStateFailureRetry));
  EXPECT_CALL(*modem_info_.mock_dispatcher(), PostTask(_)).Times(1);
  capability_->UpdatePendingActivationState();
  Mock::VerifyAndClearExpectations(modem_info_.mock_pending_activation_store());
  Mock::VerifyAndClearExpectations(modem_info_.mock_dispatcher());

  EXPECT_CALL(*modem_info_.mock_pending_activation_store(), RemoveEntry(_, _))
      .Times(0);
  EXPECT_CALL(*modem_info_.mock_pending_activation_store(),
              GetActivationState(_, _))
      .Times(4)
      .WillOnce(Return(PendingActivationStore::kStateActivated))
      .WillOnce(Return(PendingActivationStore::kStateActivated))
      .WillOnce(Return(PendingActivationStore::kStateUnknown))
      .WillOnce(Return(PendingActivationStore::kStateUnknown));
  EXPECT_CALL(*modem_info_.mock_dispatcher(), PostTask(_)).Times(0);
  capability_->UpdatePendingActivationState();
  capability_->UpdatePendingActivationState();
  Mock::VerifyAndClearExpectations(modem_info_.mock_pending_activation_store());
  Mock::VerifyAndClearExpectations(modem_info_.mock_dispatcher());
}

}  // namespace shill
