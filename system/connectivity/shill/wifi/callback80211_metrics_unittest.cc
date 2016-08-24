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

// This file provides tests to verify that Callback80211Metrics sends UMA
// notifications for appropriate messages and doesn't send them for
// inappropriate messages.

#include "shill/wifi/callback80211_metrics.h"

#include <memory>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "shill/mock_event_dispatcher.h"
#include "shill/mock_log.h"
#include "shill/mock_metrics.h"
#include "shill/net/ieee80211.h"
#include "shill/net/netlink_packet.h"
#include "shill/net/nl80211_message.h"
#include "shill/refptr_types.h"

using base::Bind;
using std::unique_ptr;
using testing::_;
using testing::Test;

namespace shill {

namespace {

// Unless otherwise specified, these data blocks have been collected by shill
// using NetlinkManager while, simultaneously (and manually) comparing shill
// output with that of the 'iw' code from which it was derived.  The test
// strings represent the raw packet data coming from the kernel.  The
// comments above each of these strings is the markup that 'iw' outputs for
// each of these packets.

// These constants are consistent across the applicable packets, below.

const uint16_t kNl80211FamilyId = 0x13;
const IEEE_80211::WiFiReasonCode kExpectedDisconnectReason =
    IEEE_80211::kReasonCodePreviousAuthenticationInvalid;

// NL80211_CMD_DISCONNECT message.
// wlan0 (phy #0): disconnected (by AP) reason: 2: Previous authentication no
// longer valid

const unsigned char kDisconnectMessage[] = {
  0x30, 0x00, 0x00, 0x00, 0x13, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x30, 0x01, 0x00, 0x00, 0x08, 0x00, 0x01, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x03, 0x00,
  0x04, 0x00, 0x00, 0x00, 0x06, 0x00, 0x36, 0x00,
  0x02, 0x00, 0x00, 0x00, 0x04, 0x00, 0x47, 0x00,
};

// NL80211_CMD_DISCONNECT message.
// Copied from kDisconnectMessage but with most of the payload removed.

const unsigned char kEmptyDisconnectMessage[] = {
  0x1c, 0x00, 0x00, 0x00, 0x13, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x30, 0x01, 0x00, 0x00, 0x08, 0x00, 0x01, 0x00,
  0x00, 0x00, 0x00, 0x00,
};

// NL80211_CMD_DEAUTHENTICATE message.
// wlan0 (phy #0): deauth c0:3f:0e:77:e8:7f -> ff:ff:ff:ff:ff:ff reason 2:
// Previous authentication no longer valid [frame: c0 00 00 00 ff ff ff ff
// ff ff c0 3f 0e 77 e8 7f c0 3f 0e 77 e8 7f c0 0e 02 00]

const unsigned char kDeauthenticateMessage[] = {
  0x44, 0x00, 0x00, 0x00, 0x13, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x27, 0x01, 0x00, 0x00, 0x08, 0x00, 0x01, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x03, 0x00,
  0x04, 0x00, 0x00, 0x00, 0x1e, 0x00, 0x33, 0x00,
  0xc0, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xc0, 0x3f, 0x0e, 0x77, 0xe8, 0x7f,
  0xc0, 0x3f, 0x0e, 0x77, 0xe8, 0x7f, 0xc0, 0x0e,
  0x02, 0x00, 0x00, 0x00,
};

// NL80211_CMD_DEAUTHENTICATE message.
// Copied from kDeauthenticateMessage but with most of the payload
// removed.

const unsigned char kEmptyDeauthenticateMessage[] = {
  0x1c, 0x00, 0x00, 0x00, 0x13, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x27, 0x01, 0x00, 0x00, 0x08, 0x00, 0x01, 0x00,
  0x00, 0x00, 0x00, 0x00,
};

// NL80211_CMD_NEW_STATION message.
// kNewStationMessage is an nl80211 message that's not a deauthenticate or
// disconnect message.  Used to make sure that only those nl80211 messages
// generate an UMA message.
// wlan0: new station c0:3f:0e:77:e8:7f

const unsigned char kNewStationMessage[] = {
  0x34, 0x00, 0x00, 0x00, 0x13, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x13, 0x01, 0x00, 0x00, 0x08, 0x00, 0x03, 0x00,
  0x04, 0x00, 0x00, 0x00, 0x0a, 0x00, 0x06, 0x00,
  0xc0, 0x3f, 0x0e, 0x77, 0xe8, 0x7f, 0x00, 0x00,
  0x08, 0x00, 0x2e, 0x00, 0x13, 0x01, 0x00, 0x00,
  0x04, 0x00, 0x15, 0x00,
};

// CTRL_CMD_GETFAMILY message.
// kGetFamilyMessage is not an nl80211 message.  Used to make sure that
// non-nl80211 messages don't generate an UMA message.
//
// Extracted from net.log.  It's just a non-nl80211 message (it's actually a
// message that's sent to the kernel rather than one received from the kernel
// but the code doesn't differentiate and this message was much shorter than the
// response).

const unsigned char kGetFamilyMessage[] = {
  0x10, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x02, 0x00,
  0x6e, 0x6c, 0x38, 0x30, 0x32, 0x31, 0x31, 0x00
};

}  // namespace

class Callback80211MetricsTest : public Test {
 public:
  Callback80211MetricsTest() :
      metrics_(&dispatcher_), callback_(&metrics_) {
    message_factory_.AddFactoryMethod(
        kNl80211FamilyId, Bind(&Nl80211Message::CreateMessage));
    Nl80211Message::SetMessageType(kNl80211FamilyId);
  }

 protected:
  MockEventDispatcher dispatcher_;
  MockMetrics metrics_;
  NetlinkMessageFactory message_factory_;
  Callback80211Metrics callback_;
};

// Make sure that notifications happen for correctly formed messages.
TEST_F(Callback80211MetricsTest, DisconnectMessage) {
  NetlinkPacket packet(kDisconnectMessage, sizeof(kDisconnectMessage));
  unique_ptr<NetlinkMessage> netlink_message(message_factory_.CreateMessage(
      &packet, NetlinkMessage::MessageContext()));
  EXPECT_CALL(metrics_, Notify80211Disconnect(Metrics::kDisconnectedByAp,
                                              kExpectedDisconnectReason));
  callback_.CollectDisconnectStatistics(*netlink_message);
}

TEST_F(Callback80211MetricsTest, DeauthMessage) {
  NetlinkPacket packet(kDeauthenticateMessage, sizeof(kDeauthenticateMessage));
  unique_ptr<NetlinkMessage> netlink_message(message_factory_.CreateMessage(
      &packet, NetlinkMessage::MessageContext()));
  EXPECT_CALL(metrics_, Notify80211Disconnect(Metrics::kDisconnectedNotByAp,
                                              kExpectedDisconnectReason));
  callback_.CollectDisconnectStatistics(*netlink_message);
}

// Make sure there's no notification if there's no reason code in the message.
TEST_F(Callback80211MetricsTest, EmptyDisconnectMessage) {
  NetlinkPacket packet(
      kEmptyDisconnectMessage, sizeof(kEmptyDisconnectMessage));
  unique_ptr<NetlinkMessage> netlink_message(message_factory_.CreateMessage(
      &packet, NetlinkMessage::MessageContext()));
  EXPECT_CALL(metrics_, Notify80211Disconnect(_, _)).Times(0);
  callback_.CollectDisconnectStatistics(*netlink_message);
}

TEST_F(Callback80211MetricsTest, EmptyDeauthMessage) {
  NetlinkPacket packet(
      kEmptyDeauthenticateMessage, sizeof(kEmptyDeauthenticateMessage));
  unique_ptr<NetlinkMessage> netlink_message(message_factory_.CreateMessage(
      &packet, NetlinkMessage::MessageContext()));
  EXPECT_CALL(metrics_, Notify80211Disconnect(_, _)).Times(0);
  callback_.CollectDisconnectStatistics(*netlink_message);
}

// Make sure the callback doesn't notify anyone for message of the wrong type.
TEST_F(Callback80211MetricsTest, Nl80211NotDisconnectDeauthMessage) {
  NetlinkPacket packet(kNewStationMessage, sizeof(kNewStationMessage));
  unique_ptr<NetlinkMessage> netlink_message(message_factory_.CreateMessage(
      &packet, NetlinkMessage::MessageContext()));
  EXPECT_CALL(metrics_, Notify80211Disconnect(_, _)).Times(0);
  callback_.CollectDisconnectStatistics(*netlink_message);
}

TEST_F(Callback80211MetricsTest, NotNl80211Message) {
  NetlinkPacket packet(kGetFamilyMessage, sizeof(kGetFamilyMessage));
  unique_ptr<NetlinkMessage> netlink_message(message_factory_.CreateMessage(
      &packet, NetlinkMessage::MessageContext()));
  EXPECT_CALL(metrics_, Notify80211Disconnect(_, _)).Times(0);
  callback_.CollectDisconnectStatistics(*netlink_message);
}

}  // namespace shill
