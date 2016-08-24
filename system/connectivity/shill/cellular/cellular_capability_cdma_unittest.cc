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

#include "shill/cellular/cellular_capability_cdma.h"

#include <base/bind.h>
#if defined(__ANDROID__)
#include <dbus/service_constants.h>
#else
#include <chromeos/dbus/service_constants.h>
#endif  // __ANDROID__
#include <gtest/gtest.h>
#include <mm/mm-modem.h>

#include "shill/cellular/cellular.h"
#include "shill/cellular/cellular_service.h"
#include "shill/cellular/mock_cellular.h"
#include "shill/cellular/mock_modem_cdma_proxy.h"
#include "shill/cellular/mock_modem_info.h"
#include "shill/cellular/mock_modem_proxy.h"
#include "shill/error.h"
#include "shill/mock_adaptors.h"
#include "shill/test_event_dispatcher.h"

using base::Bind;
using base::Unretained;
using std::string;
using testing::_;
using testing::InSequence;
using testing::Invoke;
using testing::Return;
using testing::StrEq;

namespace shill {

class CellularCapabilityCDMATest : public testing::Test {
 public:
  CellularCapabilityCDMATest()
      : modem_info_(nullptr, &dispatcher_, nullptr, nullptr),
        cellular_(new MockCellular(&modem_info_,
                                   "",
                                   "",
                                   0,
                                   Cellular::kTypeCDMA,
                                   "",
                                   "")),
        classic_proxy_(new MockModemProxy()),
        proxy_(new MockModemCDMAProxy()),
        capability_(nullptr) {
    modem_info_.metrics()->RegisterDevice(cellular_->interface_index(),
                                          Technology::kCellular);
  }

  virtual ~CellularCapabilityCDMATest() {
    cellular_->service_ = nullptr;
    capability_ = nullptr;
  }

  virtual void SetUp() {
    capability_ =
        static_cast<CellularCapabilityCDMA*>(cellular_->capability_.get());
  }

  void InvokeActivate(const string& carrier, Error* error,
                      const ActivationResultCallback& callback,
                      int timeout) {
    callback.Run(MM_MODEM_CDMA_ACTIVATION_ERROR_NO_ERROR, Error());
  }
  void InvokeActivateError(const string& carrier, Error* error,
                           const ActivationResultCallback& callback,
                           int timeout) {
    callback.Run(MM_MODEM_CDMA_ACTIVATION_ERROR_NO_SIGNAL, Error());
  }
  void InvokeDisconnect(Error* error,
                        const ResultCallback& callback,
                        int timeout) {
    callback.Run(Error());
  }
  void InvokeDisconnectError(Error* error,
                             const ResultCallback& callback,
                             int timeout) {
    Error err(Error::kOperationFailed);
    callback.Run(err);
  }
  void InvokeGetSignalQuality(Error* error,
                              const SignalQualityCallback& callback,
                              int timeout) {
    callback.Run(kStrength, Error());
  }
  void InvokeGetRegistrationState(Error* error,
                                  const RegistrationStateCallback& callback,
                                  int timeout) {
    callback.Run(MM_MODEM_CDMA_REGISTRATION_STATE_REGISTERED,
                 MM_MODEM_CDMA_REGISTRATION_STATE_HOME,
                 Error());
  }

  MOCK_METHOD1(TestCallback, void(const Error& error));

 protected:
  static const char kMEID[];
  static const char kTestCarrier[];
  static const unsigned int kStrength;

  bool IsActivationStarting() const {
    return capability_->activation_starting_;
  }

  void SetRegistrationStateEVDO(uint32_t state) {
    capability_->registration_state_evdo_ = state;
  }

  void SetRegistrationState1x(uint32_t state) {
    capability_->registration_state_1x_ = state;
  }

  void SetProxy() {
    capability_->proxy_.reset(proxy_.release());
    capability_->CellularCapabilityClassic::proxy_.reset(
        classic_proxy_.release());
  }

  void SetService() {
    cellular_->service_ = new CellularService(&modem_info_, cellular_);
  }

  void SetDeviceState(Cellular::State state) {
    cellular_->state_ = state;
  }

  EventDispatcherForTest dispatcher_;
  MockModemInfo modem_info_;
  scoped_refptr<MockCellular> cellular_;
  std::unique_ptr<MockModemProxy> classic_proxy_;
  std::unique_ptr<MockModemCDMAProxy> proxy_;
  CellularCapabilityCDMA* capability_;  // Owned by |cellular_|.
};

const char CellularCapabilityCDMATest::kMEID[] = "D1234567EF8901";
const char CellularCapabilityCDMATest::kTestCarrier[] = "The Cellular Carrier";
const unsigned int CellularCapabilityCDMATest::kStrength = 90;

TEST_F(CellularCapabilityCDMATest, PropertyStore) {
  EXPECT_TRUE(cellular_->store().Contains(kPRLVersionProperty));
}

TEST_F(CellularCapabilityCDMATest, Activate) {
  SetDeviceState(Cellular::kStateEnabled);
  EXPECT_CALL(*proxy_, Activate(kTestCarrier, _, _,
                                CellularCapability::kTimeoutActivate))
      .WillOnce(Invoke(this,
                       &CellularCapabilityCDMATest::InvokeActivate));
  EXPECT_CALL(*this, TestCallback(_));
  SetProxy();
  SetService();
  capability_->Activate(kTestCarrier, nullptr,
      Bind(&CellularCapabilityCDMATest::TestCallback, Unretained(this)));
  EXPECT_EQ(MM_MODEM_CDMA_ACTIVATION_STATE_ACTIVATING,
            capability_->activation_state());
  EXPECT_EQ(kActivationStateActivating,
            cellular_->service()->activation_state());
  EXPECT_EQ("", cellular_->service()->error());
}

TEST_F(CellularCapabilityCDMATest, ActivateWhileConnected) {
  SetDeviceState(Cellular::kStateConnected);
  {
    InSequence dummy;

    EXPECT_CALL(*cellular_, Disconnect(_, StrEq("Activate")));
    EXPECT_CALL(*proxy_, Activate(kTestCarrier, _, _,
                                  CellularCapability::kTimeoutActivate))
        .WillOnce(Invoke(this,
                         &CellularCapabilityCDMATest::InvokeActivate));
    EXPECT_CALL(*this, TestCallback(_));
  }
  SetProxy();
  SetService();
  Error error;
  capability_->Activate(kTestCarrier, &error,
      Bind(&CellularCapabilityCDMATest::TestCallback, Unretained(this)));
  // So now we should be "activating" while we wait for a disconnect.
  EXPECT_TRUE(IsActivationStarting());
  EXPECT_TRUE(capability_->IsActivating());
  EXPECT_EQ(MM_MODEM_CDMA_ACTIVATION_STATE_NOT_ACTIVATED,
            capability_->activation_state());
  // Simulate a disconnect.
  SetDeviceState(Cellular::kStateRegistered);
  capability_->DisconnectCleanup();
  // Now the modem is actually activating.
  EXPECT_EQ(MM_MODEM_CDMA_ACTIVATION_STATE_ACTIVATING,
            capability_->activation_state());
  EXPECT_EQ(kActivationStateActivating,
            cellular_->service()->activation_state());
  EXPECT_EQ("", cellular_->service()->error());
  EXPECT_FALSE(IsActivationStarting());
  EXPECT_TRUE(capability_->IsActivating());
}

TEST_F(CellularCapabilityCDMATest, ActivateWhileConnectedButFail) {
  SetDeviceState(Cellular::kStateConnected);
  {
    InSequence dummy;

    EXPECT_CALL(*cellular_, Disconnect(_, StrEq("Activate")));
    EXPECT_CALL(*proxy_, Activate(kTestCarrier, _, _,
                                  CellularCapability::kTimeoutActivate))
        .Times(0);
  }
  SetProxy();
  SetService();
  Error error;
  capability_->Activate(kTestCarrier, &error,
      Bind(&CellularCapabilityCDMATest::TestCallback, Unretained(this)));
  // So now we should be "activating" while we wait for a disconnect.
  EXPECT_TRUE(IsActivationStarting());
  EXPECT_TRUE(capability_->IsActivating());
  EXPECT_EQ(MM_MODEM_CDMA_ACTIVATION_STATE_NOT_ACTIVATED,
            capability_->activation_state());
  // Similulate a failed disconnect (the modem is still connected!).
  capability_->DisconnectCleanup();
  EXPECT_EQ(MM_MODEM_CDMA_ACTIVATION_STATE_NOT_ACTIVATED,
            capability_->activation_state());
  EXPECT_EQ(kActivationStateNotActivated,
            cellular_->service()->activation_state());
  EXPECT_EQ(kErrorActivationFailed, cellular_->service()->error());
  EXPECT_FALSE(IsActivationStarting());
  EXPECT_FALSE(capability_->IsActivating());
}

TEST_F(CellularCapabilityCDMATest, ActivateError) {
  SetDeviceState(Cellular::kStateEnabled);
  EXPECT_CALL(*proxy_, Activate(kTestCarrier, _, _,
                                CellularCapability::kTimeoutActivate))
      .WillOnce(Invoke(this,
                       &CellularCapabilityCDMATest::InvokeActivateError));
  EXPECT_CALL(*this, TestCallback(_));
  SetProxy();
  SetService();
  capability_->Activate(kTestCarrier, nullptr,
      Bind(&CellularCapabilityCDMATest::TestCallback, Unretained(this)));
  EXPECT_EQ(MM_MODEM_CDMA_ACTIVATION_STATE_NOT_ACTIVATED,
            capability_->activation_state());
  EXPECT_EQ(kActivationStateNotActivated,
            cellular_->service()->activation_state());
  EXPECT_EQ(kErrorActivationFailed,
            cellular_->service()->error());
}

TEST_F(CellularCapabilityCDMATest, GetActivationStateString) {
  EXPECT_EQ(kActivationStateActivated,
            CellularCapabilityCDMA::GetActivationStateString(
                MM_MODEM_CDMA_ACTIVATION_STATE_ACTIVATED));
  EXPECT_EQ(kActivationStateActivating,
            CellularCapabilityCDMA::GetActivationStateString(
                MM_MODEM_CDMA_ACTIVATION_STATE_ACTIVATING));
  EXPECT_EQ(kActivationStateNotActivated,
            CellularCapabilityCDMA::GetActivationStateString(
                MM_MODEM_CDMA_ACTIVATION_STATE_NOT_ACTIVATED));
  EXPECT_EQ(kActivationStatePartiallyActivated,
            CellularCapabilityCDMA::GetActivationStateString(
                MM_MODEM_CDMA_ACTIVATION_STATE_PARTIALLY_ACTIVATED));
  EXPECT_EQ(kActivationStateUnknown,
            CellularCapabilityCDMA::GetActivationStateString(123));
}

TEST_F(CellularCapabilityCDMATest, GetActivationErrorString) {
  EXPECT_EQ(kErrorNeedEvdo,
            CellularCapabilityCDMA::GetActivationErrorString(
                MM_MODEM_CDMA_ACTIVATION_ERROR_WRONG_RADIO_INTERFACE));
  EXPECT_EQ(kErrorNeedHomeNetwork,
            CellularCapabilityCDMA::GetActivationErrorString(
                MM_MODEM_CDMA_ACTIVATION_ERROR_ROAMING));
  EXPECT_EQ(kErrorOtaspFailed,
            CellularCapabilityCDMA::GetActivationErrorString(
                MM_MODEM_CDMA_ACTIVATION_ERROR_COULD_NOT_CONNECT));
  EXPECT_EQ(kErrorOtaspFailed,
            CellularCapabilityCDMA::GetActivationErrorString(
                MM_MODEM_CDMA_ACTIVATION_ERROR_SECURITY_AUTHENTICATION_FAILED));
  EXPECT_EQ(kErrorOtaspFailed,
            CellularCapabilityCDMA::GetActivationErrorString(
                MM_MODEM_CDMA_ACTIVATION_ERROR_PROVISIONING_FAILED));
  EXPECT_EQ("",
            CellularCapabilityCDMA::GetActivationErrorString(
                MM_MODEM_CDMA_ACTIVATION_ERROR_NO_ERROR));
  EXPECT_EQ(kErrorActivationFailed,
            CellularCapabilityCDMA::GetActivationErrorString(
                MM_MODEM_CDMA_ACTIVATION_ERROR_NO_SIGNAL));
  EXPECT_EQ(kErrorActivationFailed,
            CellularCapabilityCDMA::GetActivationErrorString(1234));
}

TEST_F(CellularCapabilityCDMATest, IsRegisteredEVDO) {
  EXPECT_FALSE(capability_->IsRegistered());
  SetRegistrationStateEVDO(MM_MODEM_CDMA_REGISTRATION_STATE_UNKNOWN);
  EXPECT_FALSE(capability_->IsRegistered());
  SetRegistrationStateEVDO(MM_MODEM_CDMA_REGISTRATION_STATE_REGISTERED);
  EXPECT_TRUE(capability_->IsRegistered());
  SetRegistrationStateEVDO(MM_MODEM_CDMA_REGISTRATION_STATE_HOME);
  EXPECT_TRUE(capability_->IsRegistered());
  SetRegistrationStateEVDO(MM_MODEM_CDMA_REGISTRATION_STATE_ROAMING);
  EXPECT_TRUE(capability_->IsRegistered());
}

TEST_F(CellularCapabilityCDMATest, IsRegistered1x) {
  EXPECT_FALSE(capability_->IsRegistered());
  SetRegistrationState1x(MM_MODEM_CDMA_REGISTRATION_STATE_UNKNOWN);
  EXPECT_FALSE(capability_->IsRegistered());
  SetRegistrationState1x(MM_MODEM_CDMA_REGISTRATION_STATE_REGISTERED);
  EXPECT_TRUE(capability_->IsRegistered());
  SetRegistrationState1x(MM_MODEM_CDMA_REGISTRATION_STATE_HOME);
  EXPECT_TRUE(capability_->IsRegistered());
  SetRegistrationState1x(MM_MODEM_CDMA_REGISTRATION_STATE_ROAMING);
  EXPECT_TRUE(capability_->IsRegistered());
}

TEST_F(CellularCapabilityCDMATest, GetNetworkTechnologyString) {
  EXPECT_EQ("", capability_->GetNetworkTechnologyString());
  SetRegistrationStateEVDO(MM_MODEM_CDMA_REGISTRATION_STATE_HOME);
  EXPECT_EQ(kNetworkTechnologyEvdo,
            capability_->GetNetworkTechnologyString());
  SetRegistrationStateEVDO(MM_MODEM_CDMA_REGISTRATION_STATE_UNKNOWN);
  SetRegistrationState1x(MM_MODEM_CDMA_REGISTRATION_STATE_HOME);
  EXPECT_EQ(kNetworkTechnology1Xrtt,
            capability_->GetNetworkTechnologyString());
}

TEST_F(CellularCapabilityCDMATest, GetRoamingStateString) {
  EXPECT_EQ(kRoamingStateUnknown,
            capability_->GetRoamingStateString());
  SetRegistrationStateEVDO(MM_MODEM_CDMA_REGISTRATION_STATE_REGISTERED);
  EXPECT_EQ(kRoamingStateUnknown,
            capability_->GetRoamingStateString());
  SetRegistrationStateEVDO(MM_MODEM_CDMA_REGISTRATION_STATE_HOME);
  EXPECT_EQ(kRoamingStateHome, capability_->GetRoamingStateString());
  SetRegistrationStateEVDO(MM_MODEM_CDMA_REGISTRATION_STATE_ROAMING);
  EXPECT_EQ(kRoamingStateRoaming,
            capability_->GetRoamingStateString());
  SetRegistrationStateEVDO(MM_MODEM_CDMA_REGISTRATION_STATE_UNKNOWN);
  SetRegistrationState1x(MM_MODEM_CDMA_REGISTRATION_STATE_REGISTERED);
  EXPECT_EQ(kRoamingStateUnknown,
            capability_->GetRoamingStateString());
  SetRegistrationState1x(MM_MODEM_CDMA_REGISTRATION_STATE_HOME);
  EXPECT_EQ(kRoamingStateHome, capability_->GetRoamingStateString());
  SetRegistrationState1x(MM_MODEM_CDMA_REGISTRATION_STATE_ROAMING);
  EXPECT_EQ(kRoamingStateRoaming,
            capability_->GetRoamingStateString());
}

TEST_F(CellularCapabilityCDMATest, GetSignalQuality) {
  EXPECT_CALL(*proxy_,
              GetSignalQuality(nullptr, _, CellularCapability::kTimeoutDefault))
      .WillOnce(Invoke(this,
                       &CellularCapabilityCDMATest::InvokeGetSignalQuality));
  SetProxy();
  SetService();
  EXPECT_EQ(0, cellular_->service()->strength());
  capability_->GetSignalQuality();
  EXPECT_EQ(kStrength, cellular_->service()->strength());
}

TEST_F(CellularCapabilityCDMATest, GetRegistrationState) {
  EXPECT_FALSE(cellular_->service().get());
  EXPECT_EQ(MM_MODEM_CDMA_REGISTRATION_STATE_UNKNOWN,
            capability_->registration_state_1x());
  EXPECT_EQ(MM_MODEM_CDMA_REGISTRATION_STATE_UNKNOWN,
            capability_->registration_state_evdo());
  EXPECT_CALL(*proxy_,
              GetRegistrationState(nullptr, _,
                                   CellularCapability::kTimeoutDefault))
      .WillOnce(Invoke(
          this,
          &CellularCapabilityCDMATest::InvokeGetRegistrationState));
  SetProxy();
  cellular_->state_ = Cellular::kStateEnabled;
  EXPECT_CALL(*modem_info_.mock_manager(), RegisterService(_));
  capability_->GetRegistrationState();
  dispatcher_.DispatchPendingEvents();
  EXPECT_EQ(MM_MODEM_CDMA_REGISTRATION_STATE_REGISTERED,
            capability_->registration_state_1x());
  EXPECT_EQ(MM_MODEM_CDMA_REGISTRATION_STATE_HOME,
            capability_->registration_state_evdo());
  EXPECT_TRUE(cellular_->service().get());
}

}  // namespace shill
