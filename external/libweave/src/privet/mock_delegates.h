// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBWEAVE_SRC_PRIVET_MOCK_DELEGATES_H_
#define LIBWEAVE_SRC_PRIVET_MOCK_DELEGATES_H_

#include <set>
#include <string>
#include <utility>

#include <base/values.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "src/config.h"
#include "src/privet/cloud_delegate.h"
#include "src/privet/device_delegate.h"
#include "src/privet/security_delegate.h"
#include "src/privet/wifi_delegate.h"

using testing::_;
using testing::Return;
using testing::ReturnRef;
using testing::SetArgPointee;

namespace weave {

namespace privet {

struct TestUserId : public UserAppId {
  TestUserId(const std::string& user_id)
      : UserAppId{AuthType::kAnonymous, {user_id.begin(), user_id.end()}, {}} {}
};

ACTION_TEMPLATE(RunCallback,
                HAS_1_TEMPLATE_PARAMS(int, k),
                AND_0_VALUE_PARAMS()) {
  return std::get<k>(args).Run();
}

ACTION_TEMPLATE(RunCallback,
                HAS_1_TEMPLATE_PARAMS(int, k),
                AND_1_VALUE_PARAMS(p0)) {
  return std::get<k>(args).Run(p0);
}

class MockDeviceDelegate : public DeviceDelegate {
  using IntPair = std::pair<uint16_t, uint16_t>;

 public:
  MOCK_CONST_METHOD0(GetHttpEnpoint, IntPair());
  MOCK_CONST_METHOD0(GetHttpsEnpoint, IntPair());
  MOCK_CONST_METHOD0(GetHttpRequestTimeout, base::TimeDelta());
  MOCK_METHOD3(PostDelayedTask,
               void(const tracked_objects::Location&,
                    const base::Closure&,
                    base::TimeDelta));

  MockDeviceDelegate() {
    EXPECT_CALL(*this, GetHttpEnpoint())
        .WillRepeatedly(Return(std::make_pair(0, 0)));
    EXPECT_CALL(*this, GetHttpsEnpoint())
        .WillRepeatedly(Return(std::make_pair(0, 0)));
  }
};

class MockSecurityDelegate : public SecurityDelegate {
 public:
  MOCK_METHOD7(CreateAccessToken,
               bool(AuthType,
                    const std::string&,
                    AuthScope,
                    std::string*,
                    AuthScope*,
                    base::TimeDelta*,
                    ErrorPtr*));
  MOCK_CONST_METHOD3(ParseAccessToken,
                     bool(const std::string&, UserInfo*, ErrorPtr*));
  MOCK_CONST_METHOD0(GetPairingTypes, std::set<PairingType>());
  MOCK_CONST_METHOD0(GetCryptoTypes, std::set<CryptoType>());
  MOCK_CONST_METHOD0(GetAuthTypes, std::set<AuthType>());
  MOCK_METHOD1(ClaimRootClientAuthToken, std::string(ErrorPtr*));
  MOCK_METHOD2(ConfirmClientAuthToken, bool(const std::string&, ErrorPtr*));
  MOCK_METHOD5(
      StartPairing,
      bool(PairingType, CryptoType, std::string*, std::string*, ErrorPtr*));
  MOCK_METHOD5(ConfirmPairing,
               bool(const std::string&,
                    const std::string&,
                    std::string*,
                    std::string*,
                    ErrorPtr*));
  MOCK_METHOD2(CancelPairing, bool(const std::string&, ErrorPtr*));
  MOCK_METHOD0(CreateSessionId, std::string());

  MockSecurityDelegate() {
    EXPECT_CALL(*this, CreateAccessToken(_, _, _, _, _, _, _))
        .WillRepeatedly(DoAll(
            SetArgPointee<3>("GuestAccessToken"),
            SetArgPointee<4>(AuthScope::kViewer),
            SetArgPointee<5>(base::TimeDelta::FromSeconds(15)), Return(true)));

    EXPECT_CALL(*this, ClaimRootClientAuthToken(_))
        .WillRepeatedly(Return("RootClientAuthToken"));

    EXPECT_CALL(*this, ConfirmClientAuthToken("DerivedClientAuthToken", _))
        .WillRepeatedly(Return(true));

    EXPECT_CALL(*this, ParseAccessToken(_, _, _))
        .WillRepeatedly(DoAll(SetArgPointee<1>(UserInfo{
                                  AuthScope::kViewer,
                                  UserAppId{AuthType::kLocal,
                                            {'1', '2', '3', '4', '5', '6', '7'},
                                            {}}}),
                              Return(true)));

    EXPECT_CALL(*this, GetPairingTypes())
        .WillRepeatedly(Return(std::set<PairingType>{
            PairingType::kPinCode, PairingType::kEmbeddedCode,
        }));

    EXPECT_CALL(*this, GetCryptoTypes())
        .WillRepeatedly(Return(std::set<CryptoType>{
            CryptoType::kSpake_p224,
        }));
    EXPECT_CALL(*this, GetAuthTypes())
        .WillRepeatedly(Return(std::set<AuthType>{
            AuthType::kAnonymous, AuthType::kPairing, AuthType::kLocal,
        }));

    EXPECT_CALL(*this, StartPairing(_, _, _, _, _))
        .WillRepeatedly(DoAll(SetArgPointee<2>("testSession"),
                              SetArgPointee<3>("testCommitment"),
                              Return(true)));

    EXPECT_CALL(*this, ConfirmPairing(_, _, _, _, _))
        .WillRepeatedly(DoAll(SetArgPointee<2>("testFingerprint"),
                              SetArgPointee<3>("testSignature"), Return(true)));
    EXPECT_CALL(*this, CancelPairing(_, _)).WillRepeatedly(Return(true));
    EXPECT_CALL(*this, CreateSessionId()).WillRepeatedly(Return("SessionId"));
  }
};

class MockWifiDelegate : public WifiDelegate {
 public:
  MOCK_CONST_METHOD0(GetConnectionState, const ConnectionState&());
  MOCK_CONST_METHOD0(GetSetupState, const SetupState&());
  MOCK_METHOD3(ConfigureCredentials,
               bool(const std::string&, const std::string&, ErrorPtr*));
  MOCK_CONST_METHOD0(GetCurrentlyConnectedSsid, std::string());
  MOCK_CONST_METHOD0(GetHostedSsid, std::string());
  MOCK_CONST_METHOD0(GetTypes, std::set<WifiType>());

  MockWifiDelegate() {
    EXPECT_CALL(*this, GetConnectionState())
        .WillRepeatedly(ReturnRef(connection_state_));
    EXPECT_CALL(*this, GetSetupState()).WillRepeatedly(ReturnRef(setup_state_));
    EXPECT_CALL(*this, GetCurrentlyConnectedSsid())
        .WillRepeatedly(Return("TestSsid"));
    EXPECT_CALL(*this, GetHostedSsid()).WillRepeatedly(Return(""));
    EXPECT_CALL(*this, GetTypes())
        .WillRepeatedly(Return(std::set<WifiType>{WifiType::kWifi24}));
  }

  ConnectionState connection_state_{ConnectionState::kOffline};
  SetupState setup_state_{SetupState::kNone};
};

class MockCloudDelegate : public CloudDelegate {
 public:
  MOCK_CONST_METHOD0(GetDeviceId, std::string());
  MOCK_CONST_METHOD0(GetModelId, std::string());
  MOCK_CONST_METHOD0(GetName, std::string());
  MOCK_CONST_METHOD0(GetDescription, std::string());
  MOCK_CONST_METHOD0(GetLocation, std::string());
  MOCK_METHOD3(UpdateDeviceInfo,
               void(const std::string&,
                    const std::string&,
                    const std::string&));
  MOCK_CONST_METHOD0(GetOemName, std::string());
  MOCK_CONST_METHOD0(GetModelName, std::string());
  MOCK_CONST_METHOD0(GetAnonymousMaxScope, AuthScope());
  MOCK_CONST_METHOD0(GetConnectionState, const ConnectionState&());
  MOCK_CONST_METHOD0(GetSetupState, const SetupState&());
  MOCK_METHOD3(Setup, bool(const std::string&, const std::string&, ErrorPtr*));
  MOCK_CONST_METHOD0(GetCloudId, std::string());
  MOCK_CONST_METHOD0(GetLegacyState, const base::DictionaryValue&());
  MOCK_CONST_METHOD0(GetLegacyCommandDef, const base::DictionaryValue&());
  MOCK_CONST_METHOD0(GetComponents, const base::DictionaryValue&());
  MOCK_CONST_METHOD2(FindComponent,
                     const base::DictionaryValue*(const std::string& path,
                                                  ErrorPtr* error));
  MOCK_CONST_METHOD0(GetTraits, const base::DictionaryValue&());
  MOCK_METHOD3(AddCommand,
               void(const base::DictionaryValue&,
                    const UserInfo&,
                    const CommandDoneCallback&));
  MOCK_METHOD3(GetCommand,
               void(const std::string&,
                    const UserInfo&,
                    const CommandDoneCallback&));
  MOCK_METHOD3(CancelCommand,
               void(const std::string&,
                    const UserInfo&,
                    const CommandDoneCallback&));
  MOCK_METHOD2(ListCommands, void(const UserInfo&, const CommandDoneCallback&));

  MockCloudDelegate() {
    EXPECT_CALL(*this, GetDeviceId()).WillRepeatedly(Return("TestId"));
    EXPECT_CALL(*this, GetModelId()).WillRepeatedly(Return("ABMID"));
    EXPECT_CALL(*this, GetName()).WillRepeatedly(Return("TestDevice"));
    EXPECT_CALL(*this, GetDescription()).WillRepeatedly(Return(""));
    EXPECT_CALL(*this, GetLocation()).WillRepeatedly(Return(""));
    EXPECT_CALL(*this, UpdateDeviceInfo(_, _, _)).WillRepeatedly(Return());
    EXPECT_CALL(*this, GetOemName()).WillRepeatedly(Return("Chromium"));
    EXPECT_CALL(*this, GetModelName()).WillRepeatedly(Return("Brillo"));
    EXPECT_CALL(*this, GetAnonymousMaxScope())
        .WillRepeatedly(Return(AuthScope::kUser));
    EXPECT_CALL(*this, GetConnectionState())
        .WillRepeatedly(ReturnRef(connection_state_));
    EXPECT_CALL(*this, GetSetupState()).WillRepeatedly(ReturnRef(setup_state_));
    EXPECT_CALL(*this, GetCloudId()).WillRepeatedly(Return("TestCloudId"));
    test_dict_.Set("test", new base::DictionaryValue);
    EXPECT_CALL(*this, GetLegacyState()).WillRepeatedly(ReturnRef(test_dict_));
    EXPECT_CALL(*this, GetLegacyCommandDef())
        .WillRepeatedly(ReturnRef(test_dict_));
    EXPECT_CALL(*this, GetTraits()).WillRepeatedly(ReturnRef(test_dict_));
    EXPECT_CALL(*this, GetComponents()).WillRepeatedly(ReturnRef(test_dict_));
    EXPECT_CALL(*this, FindComponent(_, _)).Times(0);
  }

  ConnectionState connection_state_{ConnectionState::kOnline};
  SetupState setup_state_{SetupState::kNone};
  base::DictionaryValue test_dict_;
};

}  // namespace privet
}  // namespace weave

#endif  // LIBWEAVE_SRC_PRIVET_MOCK_DELEGATES_H_
