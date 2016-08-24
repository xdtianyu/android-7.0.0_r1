//
// Copyright (C) 2015 The Android Open Source Project
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

#include "shill/wifi/tdls_manager.h"

#include <map>
#include <string>

#if defined(__ANDROID__)
#include <dbus/service_constants.h>
#else
#include <chromeos/dbus/service_constants.h>
#endif  // __ANDROID__
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "shill/error.h"
#include "shill/mock_event_dispatcher.h"
#include "shill/supplicant/mock_supplicant_interface_proxy.h"
#include "shill/supplicant/wpa_supplicant.h"

using std::string;
using std::vector;
using ::testing::_;
using ::testing::Mock;
using ::testing::Return;
using ::testing::SetArgumentPointee;
using ::testing::StrEq;
using ::testing::StrictMock;

namespace shill {

class TDLSManagerTest : public testing::Test {
 public:
  TDLSManagerTest()
      : tdls_manager_(&event_dispatcher_, &supplicant_interface_proxy_, "") {}

  void SetPeerDiscovering(const string& peer_mac_address) {
    tdls_manager_.peer_discovery_state_[peer_mac_address] =
        TDLSManager::PeerDiscoveryState::kRequestSent;
  }
  bool IsPeerDiscovering(const string& peer_mac_address) {
    return tdls_manager_.CheckDiscoveryState(peer_mac_address) ==
        TDLSManager::PeerDiscoveryState::kRequestSent;
  }

  void SetPeerDiscovered(const string& peer_mac_address) {
    tdls_manager_.peer_discovery_state_[peer_mac_address] =
        TDLSManager::PeerDiscoveryState::kResponseReceived;
  }
  bool IsPeerDiscovered(const string& peer_mac_address) {
    return tdls_manager_.CheckDiscoveryState(peer_mac_address) ==
        TDLSManager::PeerDiscoveryState::kResponseReceived;
  }

  bool IsPeerDiscoveryCleanupTimerSetup() {
    return !tdls_manager_.peer_discovery_cleanup_callback_.IsCancelled();
  }

  void OnPeerDiscoveryCleanup() {
    return tdls_manager_.PeerDiscoveryCleanup();
  }

 protected:
  StrictMock<MockEventDispatcher> event_dispatcher_;
  StrictMock<MockSupplicantInterfaceProxy> supplicant_interface_proxy_;
  TDLSManager tdls_manager_;
};

TEST_F(TDLSManagerTest, DiscoverPeer) {
  const char kPeer[] = "peer";
  Error error;

  EXPECT_FALSE(IsPeerDiscovering(kPeer));
  EXPECT_FALSE(IsPeerDiscoveryCleanupTimerSetup());

  // TDLS discover operation succeed.
  EXPECT_CALL(supplicant_interface_proxy_, TDLSDiscover(StrEq(kPeer)))
      .WillOnce(Return(true));
  // Post delayed task for discover peer cleanup timer.
  EXPECT_CALL(event_dispatcher_, PostDelayedTask(_, _)).Times(1);
  EXPECT_EQ("",
            tdls_manager_.PerformOperation(
                kPeer, kTDLSDiscoverOperation, &error));
  EXPECT_TRUE(error.IsSuccess());
  EXPECT_TRUE(IsPeerDiscovering(kPeer));
  EXPECT_TRUE(IsPeerDiscoveryCleanupTimerSetup());
  Mock::VerifyAndClearExpectations(&supplicant_interface_proxy_);
  Mock::VerifyAndClearExpectations(&event_dispatcher_);

  // TDLS discover operation failed.
  error.Reset();
  EXPECT_CALL(supplicant_interface_proxy_, TDLSDiscover(StrEq(kPeer)))
      .WillOnce(Return(false));
  EXPECT_CALL(event_dispatcher_, PostDelayedTask(_, _)).Times(0);
  EXPECT_EQ("",
            tdls_manager_.PerformOperation(
                kPeer, kTDLSDiscoverOperation, &error));
  EXPECT_EQ(Error::kOperationFailed, error.type());
  Mock::VerifyAndClearExpectations(&supplicant_interface_proxy_);
  Mock::VerifyAndClearExpectations(&event_dispatcher_);
}

TEST_F(TDLSManagerTest, SetupPeer) {
  const char kPeer[] = "peer";
  Error error;

  // TDLS setup operation succeed.
  EXPECT_CALL(supplicant_interface_proxy_, TDLSSetup(StrEq(kPeer)))
      .WillOnce(Return(true));
  EXPECT_EQ("",
            tdls_manager_.PerformOperation(
                kPeer, kTDLSSetupOperation, &error));
  EXPECT_TRUE(error.IsSuccess());
  Mock::VerifyAndClearExpectations(&supplicant_interface_proxy_);

  // TDLS setup operation failed.
  error.Reset();
  EXPECT_CALL(supplicant_interface_proxy_, TDLSSetup(StrEq(kPeer)))
      .WillOnce(Return(false));
  EXPECT_EQ("",
            tdls_manager_.PerformOperation(
                kPeer, kTDLSSetupOperation, &error));
  EXPECT_EQ(Error::kOperationFailed, error.type());
  Mock::VerifyAndClearExpectations(&supplicant_interface_proxy_);
}

TEST_F(TDLSManagerTest, TeardownPeer) {
  const char kPeer[] = "peer";
  Error error;

  // TDLS teardown operation succeed.
  EXPECT_CALL(supplicant_interface_proxy_, TDLSTeardown(StrEq(kPeer)))
      .WillOnce(Return(true));
  EXPECT_EQ("",
            tdls_manager_.PerformOperation(
                kPeer, kTDLSTeardownOperation, &error));
  EXPECT_TRUE(error.IsSuccess());
  Mock::VerifyAndClearExpectations(&supplicant_interface_proxy_);

  // TDLS teardown operation failed.
  error.Reset();
  EXPECT_CALL(supplicant_interface_proxy_, TDLSTeardown(StrEq(kPeer)))
      .WillOnce(Return(false));
  EXPECT_EQ("",
            tdls_manager_.PerformOperation(
                kPeer, kTDLSTeardownOperation, &error));
  EXPECT_EQ(Error::kOperationFailed, error.type());
  Mock::VerifyAndClearExpectations(&supplicant_interface_proxy_);
}

TEST_F(TDLSManagerTest, PeerStatus) {
  const char kPeer[] = "peer";
  Error error;

  // TDLS status operation succeed.
  const std::map<string, string> kTDLSStatusMap {
    { "Baby, I don't care", kTDLSUnknownState },
    { WPASupplicant::kTDLSStateConnected, kTDLSConnectedState },
    { WPASupplicant::kTDLSStateDisabled, kTDLSDisabledState },
    { WPASupplicant::kTDLSStatePeerDoesNotExist, kTDLSNonexistentState },
    { WPASupplicant::kTDLSStatePeerNotConnected, kTDLSDisconnectedState },
  };
  for (const auto& it : kTDLSStatusMap) {
    error.Reset();
    EXPECT_CALL(supplicant_interface_proxy_, TDLSStatus(StrEq(kPeer), _))
        .WillOnce(DoAll(SetArgumentPointee<1>(it.first), Return(true)));
    EXPECT_EQ(it.second,
              tdls_manager_.PerformOperation(
                  kPeer, kTDLSStatusOperation, &error));
    EXPECT_TRUE(error.IsSuccess());
    Mock::VerifyAndClearExpectations(&supplicant_interface_proxy_);
  }

  // Discovered Peer in non-existent state should return "Disconnected" state.
  error.Reset();
  SetPeerDiscovered(kPeer);
  EXPECT_CALL(supplicant_interface_proxy_, TDLSStatus(StrEq(kPeer), _))
      .WillOnce(
          DoAll(SetArgumentPointee<1>(
                    string(WPASupplicant::kTDLSStatePeerDoesNotExist)),
                Return(true)));
  EXPECT_EQ(kTDLSDisconnectedState,
            tdls_manager_.PerformOperation(
                kPeer, kTDLSStatusOperation, &error));
  EXPECT_TRUE(error.IsSuccess());
  Mock::VerifyAndClearExpectations(&supplicant_interface_proxy_);

  // TDLS status operation failed.
  error.Reset();
  EXPECT_CALL(supplicant_interface_proxy_, TDLSStatus(StrEq(kPeer), _))
      .WillOnce(Return(false));
  EXPECT_EQ("",
            tdls_manager_.PerformOperation(
                kPeer, kTDLSStatusOperation, &error));
  EXPECT_EQ(Error::kOperationFailed, error.type());
  Mock::VerifyAndClearExpectations(&supplicant_interface_proxy_);
}

TEST_F(TDLSManagerTest, OnDiscoverResponseReceived) {
  const char kPeer[] = "peer";

  // Received discover response for a peer without discover request.
  EXPECT_FALSE(IsPeerDiscovering(kPeer));
  EXPECT_FALSE(IsPeerDiscovered(kPeer));
  tdls_manager_.OnDiscoverResponseReceived(kPeer);
  EXPECT_FALSE(IsPeerDiscovering(kPeer));
  EXPECT_FALSE(IsPeerDiscovered(kPeer));

  // Receive discover response for a peer with discover request.
  SetPeerDiscovering(kPeer);
  EXPECT_TRUE(IsPeerDiscovering(kPeer));
  tdls_manager_.OnDiscoverResponseReceived(kPeer);
  EXPECT_TRUE(IsPeerDiscovered(kPeer));
}

TEST_F(TDLSManagerTest, PeerDiscoveryCleanup) {
  const char kPeer[] = "peer";

  // Start TDLS discover for a peer |kPeer|.
  Error error;
  EXPECT_CALL(supplicant_interface_proxy_, TDLSDiscover(StrEq(kPeer)))
      .WillOnce(Return(true));
  // Post delayed task for discover peer cleanup timer.
  EXPECT_CALL(event_dispatcher_, PostDelayedTask(_, _)).Times(1);
  EXPECT_EQ("",
            tdls_manager_.PerformOperation(
                kPeer, kTDLSDiscoverOperation, &error));
  EXPECT_TRUE(error.IsSuccess());
  EXPECT_TRUE(IsPeerDiscovering(kPeer));
  EXPECT_TRUE(IsPeerDiscoveryCleanupTimerSetup());
  Mock::VerifyAndClearExpectations(&supplicant_interface_proxy_);
  Mock::VerifyAndClearExpectations(&event_dispatcher_);

  // Peer discovery cleanup.
  OnPeerDiscoveryCleanup();
  EXPECT_FALSE(IsPeerDiscovering(kPeer));
}

}  // namespace shill
