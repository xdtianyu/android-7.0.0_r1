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

#include "shill/wimax/wimax_service.h"

#include <string>

#include <base/strings/string_util.h>
#if defined(__ANDROID__)
#include <dbus/service_constants.h>
#else
#include <chromeos/dbus/service_constants.h>
#endif  // __ANDROID__
#include <gtest/gtest.h>

#include "shill/error.h"
#include "shill/mock_adaptors.h"
#include "shill/mock_eap_credentials.h"
#include "shill/mock_manager.h"
#include "shill/mock_metrics.h"
#include "shill/mock_store.h"
#include "shill/nice_mock_control.h"
#include "shill/service_property_change_test.h"
#include "shill/wimax/mock_wimax.h"
#include "shill/wimax/mock_wimax_network_proxy.h"
#include "shill/wimax/mock_wimax_provider.h"

using std::string;
using testing::_;
using testing::AnyNumber;
using testing::Mock;
using testing::NiceMock;
using testing::Return;
using wimax_manager::kEAPAnonymousIdentity;
using wimax_manager::kEAPUserIdentity;
using wimax_manager::kEAPUserPassword;

namespace shill {

namespace {

const char kTestLinkName[] = "wm0";
const char kTestAddress[] = "0123456789AB";
const int kTestInterfaceIndex = 5;
const char kTestPath[] = "/org/chromium/WiMaxManager/Device/wm7";
const char kTestName[] = "Test WiMAX Network";
const char kTestNetworkId[] = "1234abcd";

}  // namespace

class WiMaxServiceTest : public testing::Test {
 public:
  WiMaxServiceTest()
      : proxy_(new MockWiMaxNetworkProxy()),
        manager_(&control_, nullptr, nullptr),
        metrics_(nullptr),
        device_(new MockWiMax(&control_, nullptr, &metrics_, &manager_,
                              kTestLinkName, kTestAddress, kTestInterfaceIndex,
                              kTestPath)),
        service_(new WiMaxService(&control_, nullptr, &metrics_, &manager_)),
        eap_(new MockEapCredentials()) {
    service_->set_friendly_name(kTestName);
    service_->set_network_id(kTestNetworkId);
    service_->InitStorageIdentifier();
    service_->eap_.reset(eap_);  // Passes ownership.
  }

  virtual ~WiMaxServiceTest() {}

 protected:
  virtual void TearDown() {
    service_->device_ = nullptr;
  }

  void ExpectUpdateService() {
    EXPECT_CALL(manager_, HasService(_)).WillOnce(Return(true));
    EXPECT_CALL(manager_, UpdateService(_));
  }

  void SetConnectable(bool connectable) {
    service_->connectable_ = connectable;
  }

  void SetDevice(WiMaxRefPtr device) {
    service_->SetDevice(device);
  }

  ServiceMockAdaptor* GetAdaptor() {
    return static_cast<ServiceMockAdaptor*>(service_->adaptor());
  }

  std::unique_ptr<MockWiMaxNetworkProxy> proxy_;
  NiceMockControl control_;
  MockManager manager_;
  NiceMock<MockMetrics> metrics_;
  scoped_refptr<MockWiMax> device_;
  WiMaxServiceRefPtr service_;
  MockEapCredentials* eap_;  // Owned by |service_|.
};

TEST_F(WiMaxServiceTest, GetConnectParameters) {
  KeyValueStore parameters;
  EXPECT_CALL(*eap_, PopulateWiMaxProperties(&parameters));
  service_->GetConnectParameters(&parameters);
}

TEST_F(WiMaxServiceTest, GetDeviceRpcId) {
  Error error;
  EXPECT_EQ(control_.NullRPCIdentifier(), service_->GetDeviceRpcId(&error));
  EXPECT_EQ(Error::kNotFound, error.type());
  service_->device_ = device_;
  error.Reset();
  EXPECT_EQ(DeviceMockAdaptor::kRpcId, service_->GetDeviceRpcId(&error));
  EXPECT_TRUE(error.IsSuccess());
}

TEST_F(WiMaxServiceTest, OnSignalStrengthChanged) {
  const int kStrength = 55;
  service_->OnSignalStrengthChanged(kStrength);
  EXPECT_EQ(kStrength, service_->strength());
}

TEST_F(WiMaxServiceTest, StartStop) {
  static const char kName[] = "My WiMAX Network";
  const uint32_t kIdentifier = 0x1234abcd;
  const int kStrength = 66;
  EXPECT_FALSE(service_->connectable());
  EXPECT_FALSE(service_->IsStarted());
  EXPECT_FALSE(service_->IsVisible());
  EXPECT_EQ(0, service_->strength());
  EXPECT_FALSE(service_->proxy_.get());
  EXPECT_CALL(*proxy_, Name(_)).WillOnce(Return(kName));
  EXPECT_CALL(*proxy_, Identifier(_)).WillOnce(Return(kIdentifier));
  EXPECT_CALL(*proxy_, SignalStrength(_)).WillOnce(Return(kStrength));
  EXPECT_CALL(*proxy_, set_signal_strength_changed_callback(_));
  ServiceMockAdaptor* adaptor = GetAdaptor();
  EXPECT_CALL(*adaptor, EmitBoolChanged(kConnectableProperty, _))
     .Times(AnyNumber());
  EXPECT_CALL(*adaptor, EmitBoolChanged(kVisibleProperty, true));
  ExpectUpdateService();
  service_->need_passphrase_ = false;
  EXPECT_TRUE(service_->Start(proxy_.release()));
  EXPECT_TRUE(service_->IsStarted());
  EXPECT_TRUE(service_->IsVisible());
  EXPECT_EQ(kStrength, service_->strength());
  EXPECT_EQ(kName, service_->network_name());
  EXPECT_EQ(kTestName, service_->friendly_name());
  EXPECT_EQ(kTestNetworkId, service_->network_id());
  EXPECT_TRUE(service_->connectable());
  EXPECT_TRUE(service_->proxy_.get());

  service_->device_ = device_;
  EXPECT_CALL(*device_, OnServiceStopped(_));
  EXPECT_CALL(*adaptor, EmitBoolChanged(kVisibleProperty, false));
  ExpectUpdateService();
  service_->Stop();
  EXPECT_FALSE(service_->IsStarted());
  EXPECT_FALSE(service_->IsVisible());
  EXPECT_EQ(0, service_->strength());
  EXPECT_FALSE(service_->proxy_.get());
}

TEST_F(WiMaxServiceTest, Connectable) {
  EXPECT_TRUE(service_->Is8021x());
  EXPECT_TRUE(service_->need_passphrase_);
  EXPECT_FALSE(service_->connectable());

  EXPECT_CALL(*eap_, IsConnectableUsingPassphrase())
      .WillOnce(Return(false))
      .WillRepeatedly(Return(true));

  // No WiMaxCredentials.
  service_->OnEapCredentialsChanged(Service::kReasonPropertyUpdate);
  EXPECT_TRUE(service_->need_passphrase_);
  EXPECT_FALSE(service_->connectable());

  // Not started (no proxy).
  service_->OnEapCredentialsChanged(Service::kReasonPropertyUpdate);
  EXPECT_FALSE(service_->need_passphrase_);
  EXPECT_FALSE(service_->connectable());

  // Connectable.
  service_->proxy_.reset(proxy_.release());
  ExpectUpdateService();
  service_->OnEapCredentialsChanged(Service::kReasonPropertyUpdate);
  EXPECT_FALSE(service_->need_passphrase_);
  EXPECT_TRUE(service_->connectable());

  // Reset WimaxConnectable state.
  Mock::VerifyAndClearExpectations(eap_);
  EXPECT_CALL(*eap_, set_password(""));
  EXPECT_CALL(*eap_, IsConnectableUsingPassphrase())
      .WillRepeatedly(Return(false));
  ExpectUpdateService();
  service_->ClearPassphrase();
  EXPECT_TRUE(service_->need_passphrase_);
  EXPECT_FALSE(service_->connectable());
}

TEST_F(WiMaxServiceTest, ChangeCredResetHasEverConnected) {
  service_->has_ever_connected_ = true;
  EXPECT_TRUE(service_->has_ever_connected());
  service_->OnEapCredentialsChanged(Service::kReasonPropertyUpdate);
  EXPECT_FALSE(service_->has_ever_connected());
}

TEST_F(WiMaxServiceTest, ConvertIdentifierToNetworkId) {
  EXPECT_EQ("00000000", WiMaxService::ConvertIdentifierToNetworkId(0));
  EXPECT_EQ("abcd1234", WiMaxService::ConvertIdentifierToNetworkId(0xabcd1234));
  EXPECT_EQ("ffffffff", WiMaxService::ConvertIdentifierToNetworkId(0xffffffff));
}

TEST_F(WiMaxServiceTest, StorageIdentifier) {
  static const char kStorageId[] = "wimax_test_wimax_network_1234abcd";
  EXPECT_EQ(kStorageId, service_->GetStorageIdentifier());
  EXPECT_EQ(kStorageId,
            WiMaxService::CreateStorageIdentifier(kTestNetworkId, kTestName));
}

TEST_F(WiMaxServiceTest, Save) {
  NiceMock<MockStore> storage;
  string storage_id = service_->GetStorageIdentifier();
  EXPECT_CALL(storage, SetString(storage_id, _, _))
      .WillRepeatedly(Return(true));
  EXPECT_CALL(storage, DeleteKey(storage_id, _)).WillRepeatedly(Return(true));
  EXPECT_CALL(storage, SetString(storage_id,
                                 WiMaxService::kStorageNetworkId,
                                 kTestNetworkId));
  EXPECT_TRUE(service_->Save(&storage));
}

TEST_F(WiMaxServiceTest, Connect) {
  // Connect but not connectable.
  Error error;
  EXPECT_FALSE(service_->connectable());
  service_->Connect(&error, "in test");
  EXPECT_EQ(Error::kOperationFailed, error.type());
  SetConnectable(true);

  // No carrier device available.
  MockWiMaxProvider provider;
  scoped_refptr<MockWiMax> null_device;
  EXPECT_CALL(manager_, wimax_provider()).WillOnce(Return(&provider));
  EXPECT_CALL(provider, SelectCarrier(_)).WillOnce(Return(null_device));
  error.Reset();
  service_->Connect(&error, "in test");
  EXPECT_EQ(Error::kNoCarrier, error.type());

  // Successful connect.
  EXPECT_CALL(manager_, wimax_provider()).WillOnce(Return(&provider));
  EXPECT_CALL(provider, SelectCarrier(_)).WillOnce(Return(device_));
  EXPECT_CALL(*device_, ConnectTo(_, _));
  error.Reset();
  service_->Connect(&error, "in test");
  EXPECT_TRUE(error.IsSuccess());

  // Connect while already connected.
  // TODO(benchan): Check for error if we populate error again after changing
  // the way that Chrome handles Error::kAlreadyConnected situation.
  service_->Connect(&error, "in test");

  // Successful disconnect.
  EXPECT_CALL(*eap_, set_password(_)).Times(0);
  EXPECT_CALL(*device_, DisconnectFrom(_, _));
  error.Reset();
  service_->Disconnect(&error, "in test");
  EXPECT_TRUE(error.IsSuccess());
  EXPECT_TRUE(service_->connectable());

  // Disconnect while not connected.
  service_->Disconnect(&error, "in test");
  EXPECT_EQ(Error::kNotConnected, error.type());
}

TEST_F(WiMaxServiceTest, Unload) {
  MockWiMaxProvider provider;
  EXPECT_CALL(manager_, wimax_provider())
      .Times(2)
      .WillRepeatedly(Return(&provider));
  EXPECT_CALL(*eap_, Reset());
  EXPECT_CALL(*eap_, set_password(""));
  EXPECT_CALL(*eap_, IsConnectableUsingPassphrase())
      .WillRepeatedly(Return(false));
  EXPECT_CALL(provider, OnServiceUnloaded(_)).WillOnce(Return(false));
  EXPECT_FALSE(service_->Unload());
  Mock::VerifyAndClearExpectations(eap_);

  EXPECT_CALL(*eap_, Reset());
  EXPECT_CALL(*eap_, set_password(""));
  EXPECT_CALL(*eap_, IsConnectableUsingPassphrase())
      .WillRepeatedly(Return(false));
  EXPECT_CALL(provider, OnServiceUnloaded(_)).WillOnce(Return(true));
  EXPECT_TRUE(service_->Unload());
}

TEST_F(WiMaxServiceTest, SetState) {
  service_->device_ = device_;
  EXPECT_EQ(Service::kStateIdle, service_->state());

  EXPECT_CALL(manager_, UpdateService(_));
  service_->SetState(Service::kStateAssociating);
  EXPECT_EQ(Service::kStateAssociating, service_->state());
  EXPECT_TRUE(service_->device_);

  EXPECT_CALL(manager_, UpdateService(_));
  service_->SetState(Service::kStateFailure);
  EXPECT_EQ(Service::kStateFailure, service_->state());
  EXPECT_FALSE(service_->device_);
}

TEST_F(WiMaxServiceTest, IsAutoConnectable) {
  EXPECT_FALSE(service_->connectable());
  const char* reason = "";

  EXPECT_FALSE(service_->IsAutoConnectable(&reason));

  MockWiMaxProvider provider;
  EXPECT_CALL(manager_, wimax_provider())
      .Times(2)
      .WillRepeatedly(Return(&provider));

  SetConnectable(true);
  EXPECT_CALL(provider, SelectCarrier(_)).WillOnce(Return(device_));
  EXPECT_CALL(*device_, IsIdle()).WillOnce(Return(false));
  reason = "";
  EXPECT_FALSE(service_->IsAutoConnectable(&reason));
  EXPECT_EQ(WiMaxService::kAutoConnBusy, reason);

  EXPECT_CALL(provider, SelectCarrier(_)).WillOnce(Return(device_));
  EXPECT_CALL(*device_, IsIdle()).WillOnce(Return(true));
  reason = "";
  EXPECT_TRUE(service_->IsAutoConnectable(&reason));
  EXPECT_STREQ("", reason);
}

TEST_F(WiMaxServiceTest, PropertyChanges) {
  ServiceMockAdaptor* adaptor = GetAdaptor();
  TestCommonPropertyChanges(service_, adaptor);
  TestAutoConnectPropertyChange(service_, adaptor);

  EXPECT_CALL(*adaptor,
              EmitRpcIdentifierChanged(kDeviceProperty, _));
  SetDevice(device_);
  Mock::VerifyAndClearExpectations(adaptor);

  EXPECT_CALL(*adaptor,
              EmitRpcIdentifierChanged(kDeviceProperty, _));
  SetDevice(nullptr);
  Mock::VerifyAndClearExpectations(adaptor);
}

// Custom property setters should return false, and make no changes, if
// the new value is the same as the old value.
TEST_F(WiMaxServiceTest, CustomSetterNoopChange) {
  TestCustomSetterNoopChange(service_, &manager_);
}

}  // namespace shill
