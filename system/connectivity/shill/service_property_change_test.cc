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

#include "shill/service_property_change_test.h"

#include <string>

#if defined(__ANDROID__)
#include <dbus/service_constants.h>
#else
#include <chromeos/dbus/service_constants.h>
#endif  // __ANDROID__
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "shill/error.h"
#include "shill/mock_adaptors.h"
#include "shill/mock_manager.h"
#include "shill/mock_profile.h"
#include "shill/refptr_types.h"
#include "shill/service.h"

using std::string;
using testing::_;
using testing::AnyNumber;
using testing::Mock;
using testing::NiceMock;

namespace shill {

// Some of these tests are duplicative, as we also have broader tests
// for specific setters. However, it's convenient to have all the property
// change notifications documented (and tested) in one place.

void TestCommonPropertyChanges(ServiceRefPtr service,
                               ServiceMockAdaptor* adaptor) {
  Error error;

  EXPECT_EQ(Service::kStateIdle, service->state());
  EXPECT_CALL(*adaptor, EmitStringChanged(kStateProperty, _));
  service->SetState(Service::kStateConnected);
  Mock::VerifyAndClearExpectations(adaptor);

  // TODO(quiche): Once crbug.com/216664 is resolved, add a test
  // that service->SetConnection emits kIPConfigProperty changed.

  bool connectable = service->connectable();
  EXPECT_CALL(*adaptor, EmitBoolChanged(kConnectableProperty, _));
  service->SetConnectable(!connectable);
  Mock::VerifyAndClearExpectations(adaptor);

  EXPECT_EQ(string(), service->guid());
  EXPECT_CALL(*adaptor, EmitStringChanged(kGuidProperty, _));
  service->SetGuid("some garbage", &error);
  Mock::VerifyAndClearExpectations(adaptor);

  // Depending on our caller, AutoConnect may be true.
  service->ClearAutoConnect(nullptr);
  EXPECT_FALSE(service->auto_connect());
  EXPECT_CALL(*adaptor, EmitBoolChanged(kAutoConnectProperty, _));
  service->SetAutoConnect(true);
  Mock::VerifyAndClearExpectations(adaptor);

  EXPECT_EQ(0, service->priority());
  EXPECT_CALL(*adaptor, EmitIntChanged(kPriorityProperty, _));
  service->SetPriority(1, &error);
  Mock::VerifyAndClearExpectations(adaptor);

  EXPECT_EQ(string(), service->GetProxyConfig(&error));
  EXPECT_CALL(*adaptor, EmitStringChanged(kProxyConfigProperty, _));
  service->SetProxyConfig("some garbage", &error);
  Mock::VerifyAndClearExpectations(adaptor);

  uint8_t strength = service->strength();
  EXPECT_CALL(*adaptor,
              EmitUint8Changed(kSignalStrengthProperty, _));
  service->SetStrength(strength+1);
  Mock::VerifyAndClearExpectations(adaptor);

  EXPECT_EQ(string(), service->error_details());
  EXPECT_CALL(*adaptor, EmitStringChanged(kErrorDetailsProperty, _));
  service->SetErrorDetails("some garbage");
  Mock::VerifyAndClearExpectations(adaptor);

  EXPECT_EQ(Service::kFailureUnknown, service->failure());
  EXPECT_EQ(Service::ConnectFailureToString(Service::kFailureUnknown),
            service->error());
  EXPECT_CALL(*adaptor, EmitStringChanged(kStateProperty, _));
  EXPECT_CALL(*adaptor, EmitStringChanged(kErrorProperty, _));
  service->SetFailure(Service::kFailureAAA);
  Mock::VerifyAndClearExpectations(adaptor);

  EXPECT_NE(Service::ConnectFailureToString(Service::kFailureUnknown),
            service->error());
  EXPECT_CALL(*adaptor, EmitStringChanged(kStateProperty, _));
  EXPECT_CALL(*adaptor, EmitStringChanged(kErrorDetailsProperty, _));
  EXPECT_CALL(*adaptor, EmitStringChanged(kErrorProperty, _));
  service->SetState(Service::kStateConnected);
  Mock::VerifyAndClearExpectations(adaptor);

  EXPECT_EQ(Service::ConnectFailureToString(Service::kFailureUnknown),
            service->error());
  EXPECT_CALL(*adaptor, EmitStringChanged(kStateProperty, _));
  EXPECT_CALL(*adaptor, EmitStringChanged(kErrorProperty, _));
  service->SetFailureSilent(Service::kFailureAAA);
  Mock::VerifyAndClearExpectations(adaptor);
}

void TestAutoConnectPropertyChange(ServiceRefPtr service,
                                   ServiceMockAdaptor* adaptor) {
  bool auto_connect = service->auto_connect();
  EXPECT_CALL(*adaptor, EmitBoolChanged(kAutoConnectProperty, _));
  service->SetAutoConnect(!auto_connect);
  Mock::VerifyAndClearExpectations(adaptor);
}

void TestNamePropertyChange(ServiceRefPtr service,
                            ServiceMockAdaptor* adaptor) {
  Error error;
  string name = service->GetNameProperty(&error);
  EXPECT_CALL(*adaptor, EmitStringChanged(kNameProperty, _));
  service->SetNameProperty(name + " and some new stuff", &error);
  Mock::VerifyAndClearExpectations(adaptor);
}

void TestCustomSetterNoopChange(ServiceRefPtr service,
                                MockManager* mock_manager) {
  // SetAutoConnectFull
  {
    Error error;
    EXPECT_CALL(*mock_manager, UpdateService(_)).Times(0);
    EXPECT_FALSE(service->SetAutoConnectFull(service->auto_connect(), &error));
    EXPECT_TRUE(error.IsSuccess());
    Mock::VerifyAndClearExpectations(mock_manager);
  }

  // SetCheckPortal
  {
    Error error;
    EXPECT_FALSE(service->SetCheckPortal(service->check_portal_, &error));
    EXPECT_TRUE(error.IsSuccess());
  }

  // SetNameProperty
  {
    Error error;
    EXPECT_FALSE(service->SetNameProperty(service->friendly_name_, &error));
    EXPECT_TRUE(error.IsSuccess());
  }

  // SetProfileRpcId
  {
    Error error;
    scoped_refptr<MockProfile> profile(
        new NiceMock<MockProfile>(nullptr, nullptr, nullptr));
    service->set_profile(profile);
    EXPECT_FALSE(service->SetProfileRpcId(profile->GetRpcIdentifier(),
                                           &error));
    EXPECT_TRUE(error.IsSuccess());
  }

  // SetProxyConfig
  {
    Error error;
    static const string kProxyConfig = "some opaque blob";
    // Set to known value.
    EXPECT_TRUE(service->SetProxyConfig(kProxyConfig, &error));
    EXPECT_TRUE(error.IsSuccess());
    // Set to same value.
    EXPECT_FALSE(service->SetProxyConfig(kProxyConfig, &error));
    EXPECT_TRUE(error.IsSuccess());
  }
}

}  // namespace shill
