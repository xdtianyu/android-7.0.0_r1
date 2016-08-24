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

// This file provides tests for individual messages.  It tests
// NetlinkMessageFactory's ability to create specific message types and it
// tests the various NetlinkMessage types' ability to parse those
// messages.

// This file tests the public interface to NetlinkMessage.

#include "shill/net/nl80211_message.h"

#include <memory>
#include <string>
#include <vector>

#include <base/strings/stringprintf.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "shill/net/mock_netlink_socket.h"
#include "shill/net/netlink_attribute.h"
#include "shill/net/netlink_packet.h"

using base::Bind;
using base::StringPrintf;
using base::Unretained;
using std::string;
using std::unique_ptr;
using std::vector;
using testing::_;
using testing::EndsWith;
using testing::Invoke;
using testing::Return;
using testing::Test;

namespace shill {

namespace {

// These data blocks have been collected by shill using NetlinkManager while,
// simultaneously (and manually) comparing shill output with that of the 'iw'
// code from which it was derived.  The test strings represent the raw packet
// data coming from the kernel.  The comments above each of these strings is
// the markup that 'iw' outputs for each of these packets.

// These constants are consistent throughout the packets, below.

const uint32_t kExpectedIfIndex = 4;
const uint32_t kWiPhy = 0;
const uint16_t kNl80211FamilyId = 0x13;
const char kExpectedMacAddress[] = "c0:3f:0e:77:e8:7f";

const uint8_t kMacAddressBytes[] = {
  0xc0, 0x3f, 0x0e, 0x77, 0xe8, 0x7f
};

const uint8_t kRespIeBytes[] = {
  0x01, 0x08, 0x82, 0x84,
  0x8b, 0x96, 0x0c, 0x12,
  0x18, 0x24, 0x32, 0x04,
  0x30, 0x48, 0x60, 0x6c
};


// wlan0 (phy #0): scan started

const uint32_t kScanFrequencyTrigger[] = {
  2412, 2417, 2422, 2427, 2432, 2437, 2442, 2447,
  2452, 2457, 2462, 2467, 2472, 2484, 5180, 5200,
  5220, 5240, 5260, 5280, 5300, 5320, 5500, 5520,
  5540, 5560, 5580, 5600, 5620, 5640, 5660, 5680,
  5700, 5745, 5765, 5785, 5805, 5825
};

const unsigned char kNL80211_CMD_TRIGGER_SCAN[] = {
  0x68, 0x01, 0x00, 0x00, 0x13, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x21, 0x01, 0x00, 0x00, 0x08, 0x00, 0x01, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x03, 0x00,
  0x04, 0x00, 0x00, 0x00, 0x08, 0x00, 0x2d, 0x00,
  0x04, 0x00, 0x00, 0x00, 0x34, 0x01, 0x2c, 0x00,
  0x08, 0x00, 0x00, 0x00, 0x6c, 0x09, 0x00, 0x00,
  0x08, 0x00, 0x01, 0x00, 0x71, 0x09, 0x00, 0x00,
  0x08, 0x00, 0x02, 0x00, 0x76, 0x09, 0x00, 0x00,
  0x08, 0x00, 0x03, 0x00, 0x7b, 0x09, 0x00, 0x00,
  0x08, 0x00, 0x04, 0x00, 0x80, 0x09, 0x00, 0x00,
  0x08, 0x00, 0x05, 0x00, 0x85, 0x09, 0x00, 0x00,
  0x08, 0x00, 0x06, 0x00, 0x8a, 0x09, 0x00, 0x00,
  0x08, 0x00, 0x07, 0x00, 0x8f, 0x09, 0x00, 0x00,
  0x08, 0x00, 0x08, 0x00, 0x94, 0x09, 0x00, 0x00,
  0x08, 0x00, 0x09, 0x00, 0x99, 0x09, 0x00, 0x00,
  0x08, 0x00, 0x0a, 0x00, 0x9e, 0x09, 0x00, 0x00,
  0x08, 0x00, 0x0b, 0x00, 0xa3, 0x09, 0x00, 0x00,
  0x08, 0x00, 0x0c, 0x00, 0xa8, 0x09, 0x00, 0x00,
  0x08, 0x00, 0x0d, 0x00, 0xb4, 0x09, 0x00, 0x00,
  0x08, 0x00, 0x0e, 0x00, 0x3c, 0x14, 0x00, 0x00,
  0x08, 0x00, 0x0f, 0x00, 0x50, 0x14, 0x00, 0x00,
  0x08, 0x00, 0x10, 0x00, 0x64, 0x14, 0x00, 0x00,
  0x08, 0x00, 0x11, 0x00, 0x78, 0x14, 0x00, 0x00,
  0x08, 0x00, 0x12, 0x00, 0x8c, 0x14, 0x00, 0x00,
  0x08, 0x00, 0x13, 0x00, 0xa0, 0x14, 0x00, 0x00,
  0x08, 0x00, 0x14, 0x00, 0xb4, 0x14, 0x00, 0x00,
  0x08, 0x00, 0x15, 0x00, 0xc8, 0x14, 0x00, 0x00,
  0x08, 0x00, 0x16, 0x00, 0x7c, 0x15, 0x00, 0x00,
  0x08, 0x00, 0x17, 0x00, 0x90, 0x15, 0x00, 0x00,
  0x08, 0x00, 0x18, 0x00, 0xa4, 0x15, 0x00, 0x00,
  0x08, 0x00, 0x19, 0x00, 0xb8, 0x15, 0x00, 0x00,
  0x08, 0x00, 0x1a, 0x00, 0xcc, 0x15, 0x00, 0x00,
  0x08, 0x00, 0x1b, 0x00, 0xe0, 0x15, 0x00, 0x00,
  0x08, 0x00, 0x1c, 0x00, 0xf4, 0x15, 0x00, 0x00,
  0x08, 0x00, 0x1d, 0x00, 0x08, 0x16, 0x00, 0x00,
  0x08, 0x00, 0x1e, 0x00, 0x1c, 0x16, 0x00, 0x00,
  0x08, 0x00, 0x1f, 0x00, 0x30, 0x16, 0x00, 0x00,
  0x08, 0x00, 0x20, 0x00, 0x44, 0x16, 0x00, 0x00,
  0x08, 0x00, 0x21, 0x00, 0x71, 0x16, 0x00, 0x00,
  0x08, 0x00, 0x22, 0x00, 0x85, 0x16, 0x00, 0x00,
  0x08, 0x00, 0x23, 0x00, 0x99, 0x16, 0x00, 0x00,
  0x08, 0x00, 0x24, 0x00, 0xad, 0x16, 0x00, 0x00,
  0x08, 0x00, 0x25, 0x00, 0xc1, 0x16, 0x00, 0x00,
  0x08, 0x00, 0x73, 0x00, 0x00, 0x00, 0x00, 0x00,
};


// wlan0 (phy #0): scan finished: 2412 2417 2422 2427 2432 2437 2442 2447 2452
// 2457 2462 2467 2472 2484 5180 5200 5220 5240 5260 5280 5300 5320 5500 5520
// 5540 5560 5580 5600 5620 5640 5660 5680 5700 5745 5765 5785 5805 5825, ""

const uint32_t kScanFrequencyResults[] = {
  2412, 2417, 2422, 2427, 2432, 2437, 2442, 2447,
  2452, 2457, 2462, 2467, 2472, 2484, 5180, 5200,
  5220, 5240, 5260, 5280, 5300, 5320, 5500, 5520,
  5540, 5560, 5580, 5600, 5620, 5640, 5660, 5680,
  5700, 5745, 5765, 5785, 5805, 5825
};

const unsigned char kNL80211_CMD_NEW_SCAN_RESULTS[] = {
  0x68, 0x01, 0x00, 0x00, 0x13, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x22, 0x01, 0x00, 0x00, 0x08, 0x00, 0x01, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x03, 0x00,
  0x04, 0x00, 0x00, 0x00, 0x08, 0x00, 0x2d, 0x00,
  0x04, 0x00, 0x00, 0x00, 0x34, 0x01, 0x2c, 0x00,
  0x08, 0x00, 0x00, 0x00, 0x6c, 0x09, 0x00, 0x00,
  0x08, 0x00, 0x01, 0x00, 0x71, 0x09, 0x00, 0x00,
  0x08, 0x00, 0x02, 0x00, 0x76, 0x09, 0x00, 0x00,
  0x08, 0x00, 0x03, 0x00, 0x7b, 0x09, 0x00, 0x00,
  0x08, 0x00, 0x04, 0x00, 0x80, 0x09, 0x00, 0x00,
  0x08, 0x00, 0x05, 0x00, 0x85, 0x09, 0x00, 0x00,
  0x08, 0x00, 0x06, 0x00, 0x8a, 0x09, 0x00, 0x00,
  0x08, 0x00, 0x07, 0x00, 0x8f, 0x09, 0x00, 0x00,
  0x08, 0x00, 0x08, 0x00, 0x94, 0x09, 0x00, 0x00,
  0x08, 0x00, 0x09, 0x00, 0x99, 0x09, 0x00, 0x00,
  0x08, 0x00, 0x0a, 0x00, 0x9e, 0x09, 0x00, 0x00,
  0x08, 0x00, 0x0b, 0x00, 0xa3, 0x09, 0x00, 0x00,
  0x08, 0x00, 0x0c, 0x00, 0xa8, 0x09, 0x00, 0x00,
  0x08, 0x00, 0x0d, 0x00, 0xb4, 0x09, 0x00, 0x00,
  0x08, 0x00, 0x0e, 0x00, 0x3c, 0x14, 0x00, 0x00,
  0x08, 0x00, 0x0f, 0x00, 0x50, 0x14, 0x00, 0x00,
  0x08, 0x00, 0x10, 0x00, 0x64, 0x14, 0x00, 0x00,
  0x08, 0x00, 0x11, 0x00, 0x78, 0x14, 0x00, 0x00,
  0x08, 0x00, 0x12, 0x00, 0x8c, 0x14, 0x00, 0x00,
  0x08, 0x00, 0x13, 0x00, 0xa0, 0x14, 0x00, 0x00,
  0x08, 0x00, 0x14, 0x00, 0xb4, 0x14, 0x00, 0x00,
  0x08, 0x00, 0x15, 0x00, 0xc8, 0x14, 0x00, 0x00,
  0x08, 0x00, 0x16, 0x00, 0x7c, 0x15, 0x00, 0x00,
  0x08, 0x00, 0x17, 0x00, 0x90, 0x15, 0x00, 0x00,
  0x08, 0x00, 0x18, 0x00, 0xa4, 0x15, 0x00, 0x00,
  0x08, 0x00, 0x19, 0x00, 0xb8, 0x15, 0x00, 0x00,
  0x08, 0x00, 0x1a, 0x00, 0xcc, 0x15, 0x00, 0x00,
  0x08, 0x00, 0x1b, 0x00, 0xe0, 0x15, 0x00, 0x00,
  0x08, 0x00, 0x1c, 0x00, 0xf4, 0x15, 0x00, 0x00,
  0x08, 0x00, 0x1d, 0x00, 0x08, 0x16, 0x00, 0x00,
  0x08, 0x00, 0x1e, 0x00, 0x1c, 0x16, 0x00, 0x00,
  0x08, 0x00, 0x1f, 0x00, 0x30, 0x16, 0x00, 0x00,
  0x08, 0x00, 0x20, 0x00, 0x44, 0x16, 0x00, 0x00,
  0x08, 0x00, 0x21, 0x00, 0x71, 0x16, 0x00, 0x00,
  0x08, 0x00, 0x22, 0x00, 0x85, 0x16, 0x00, 0x00,
  0x08, 0x00, 0x23, 0x00, 0x99, 0x16, 0x00, 0x00,
  0x08, 0x00, 0x24, 0x00, 0xad, 0x16, 0x00, 0x00,
  0x08, 0x00, 0x25, 0x00, 0xc1, 0x16, 0x00, 0x00,
  0x08, 0x00, 0x73, 0x00, 0x00, 0x00, 0x00, 0x00,
};


// wlan0: new station c0:3f:0e:77:e8:7f

const uint32_t kNewStationExpectedGeneration = 275;

const unsigned char kNL80211_CMD_NEW_STATION[] = {
  0x34, 0x00, 0x00, 0x00, 0x13, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x13, 0x01, 0x00, 0x00, 0x08, 0x00, 0x03, 0x00,
  0x04, 0x00, 0x00, 0x00, 0x0a, 0x00, 0x06, 0x00,
  0xc0, 0x3f, 0x0e, 0x77, 0xe8, 0x7f, 0x00, 0x00,
  0x08, 0x00, 0x2e, 0x00, 0x13, 0x01, 0x00, 0x00,
  0x04, 0x00, 0x15, 0x00,
};


// wlan0 (phy #0): auth c0:3f:0e:77:e8:7f -> 48:5d:60:77:2d:cf status: 0:
// Successful [frame: b0 00 3a 01 48 5d 60 77 2d cf c0 3f 0e 77 e8 7f c0
// 3f 0e 77 e8 7f 30 07 00 00 02 00 00 00]

const unsigned char kAuthenticateFrame[] = {
  0xb0, 0x00, 0x3a, 0x01, 0x48, 0x5d, 0x60, 0x77,
  0x2d, 0xcf, 0xc0, 0x3f, 0x0e, 0x77, 0xe8, 0x7f,
  0xc0, 0x3f, 0x0e, 0x77, 0xe8, 0x7f, 0x30, 0x07,
  0x00, 0x00, 0x02, 0x00, 0x00, 0x00
};

const unsigned char kNL80211_CMD_AUTHENTICATE[] = {
  0x48, 0x00, 0x00, 0x00, 0x13, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x25, 0x01, 0x00, 0x00, 0x08, 0x00, 0x01, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x03, 0x00,
  0x04, 0x00, 0x00, 0x00, 0x22, 0x00, 0x33, 0x00,
  0xb0, 0x00, 0x3a, 0x01, 0x48, 0x5d, 0x60, 0x77,
  0x2d, 0xcf, 0xc0, 0x3f, 0x0e, 0x77, 0xe8, 0x7f,
  0xc0, 0x3f, 0x0e, 0x77, 0xe8, 0x7f, 0x30, 0x07,
  0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00,
};


// wlan0 (phy #0): assoc c0:3f:0e:77:e8:7f -> 48:5d:60:77:2d:cf status: 0:
// Successful [frame: 10 00 3a 01 48 5d 60 77 2d cf c0 3f 0e 77 e8 7f c0 3f 0e
// 77 e8 7f 40 07 01 04 00 00 01 c0 01 08 82 84 8b 96 0c 12 18 24 32 04 30 48
// 60 6c]

const unsigned char kAssociateFrame[] = {
  0x10, 0x00, 0x3a, 0x01, 0x48, 0x5d, 0x60, 0x77,
  0x2d, 0xcf, 0xc0, 0x3f, 0x0e, 0x77, 0xe8, 0x7f,
  0xc0, 0x3f, 0x0e, 0x77, 0xe8, 0x7f, 0x40, 0x07,
  0x01, 0x04, 0x00, 0x00, 0x01, 0xc0, 0x01, 0x08,
  0x82, 0x84, 0x8b, 0x96, 0x0c, 0x12, 0x18, 0x24,
  0x32, 0x04, 0x30, 0x48, 0x60, 0x6c
};

const unsigned char kNL80211_CMD_ASSOCIATE[] = {
  0x58, 0x00, 0x00, 0x00, 0x13, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x26, 0x01, 0x00, 0x00, 0x08, 0x00, 0x01, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x03, 0x00,
  0x04, 0x00, 0x00, 0x00, 0x32, 0x00, 0x33, 0x00,
  0x10, 0x00, 0x3a, 0x01, 0x48, 0x5d, 0x60, 0x77,
  0x2d, 0xcf, 0xc0, 0x3f, 0x0e, 0x77, 0xe8, 0x7f,
  0xc0, 0x3f, 0x0e, 0x77, 0xe8, 0x7f, 0x40, 0x07,
  0x01, 0x04, 0x00, 0x00, 0x01, 0xc0, 0x01, 0x08,
  0x82, 0x84, 0x8b, 0x96, 0x0c, 0x12, 0x18, 0x24,
  0x32, 0x04, 0x30, 0x48, 0x60, 0x6c, 0x00, 0x00,
};


// wlan0 (phy #0): connected to c0:3f:0e:77:e8:7f

const uint16_t kExpectedConnectStatus = 0;

const unsigned char kNL80211_CMD_CONNECT[] = {
  0x4c, 0x00, 0x00, 0x00, 0x13, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x2e, 0x01, 0x00, 0x00, 0x08, 0x00, 0x01, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x03, 0x00,
  0x04, 0x00, 0x00, 0x00, 0x0a, 0x00, 0x06, 0x00,
  0xc0, 0x3f, 0x0e, 0x77, 0xe8, 0x7f, 0x00, 0x00,
  0x06, 0x00, 0x48, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x14, 0x00, 0x4e, 0x00, 0x01, 0x08, 0x82, 0x84,
  0x8b, 0x96, 0x0c, 0x12, 0x18, 0x24, 0x32, 0x04,
  0x30, 0x48, 0x60, 0x6c,
};


// wlan0 (phy #0): deauth c0:3f:0e:77:e8:7f -> ff:ff:ff:ff:ff:ff reason 2:
// Previous authentication no longer valid [frame: c0 00 00 00 ff ff ff ff
// ff ff c0 3f 0e 77 e8 7f c0 3f 0e 77 e8 7f c0 0e 02 00]

const unsigned char kDeauthenticateFrame[] = {
  0xc0, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xc0, 0x3f, 0x0e, 0x77, 0xe8, 0x7f,
  0xc0, 0x3f, 0x0e, 0x77, 0xe8, 0x7f, 0xc0, 0x0e,
  0x02, 0x00
};

const unsigned char kNL80211_CMD_DEAUTHENTICATE[] = {
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


// wlan0 (phy #0): disconnected (by AP) reason: 2: Previous authentication no
// longer valid

const uint16_t kExpectedDisconnectReason = 2;

const unsigned char kNL80211_CMD_DISCONNECT[] = {
  0x30, 0x00, 0x00, 0x00, 0x13, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x30, 0x01, 0x00, 0x00, 0x08, 0x00, 0x01, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x03, 0x00,
  0x04, 0x00, 0x00, 0x00, 0x06, 0x00, 0x36, 0x00,
  0x02, 0x00, 0x00, 0x00, 0x04, 0x00, 0x47, 0x00,
};


// wlan0 (phy #0): connection quality monitor event: peer c0:3f:0e:77:e8:7f
// didn't ACK 50 packets

const uint32_t kExpectedCqmNotAcked = 50;

const unsigned char kNL80211_CMD_NOTIFY_CQM[] = {
  0x3c, 0x00, 0x00, 0x00, 0x13, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x40, 0x01, 0x00, 0x00, 0x08, 0x00, 0x01, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x03, 0x00,
  0x04, 0x00, 0x00, 0x00, 0x0a, 0x00, 0x06, 0x00,
  0xc0, 0x3f, 0x0e, 0x77, 0xe8, 0x7f, 0x00, 0x00,
  0x0c, 0x00, 0x5e, 0x00, 0x08, 0x00, 0x04, 0x00,
  0x32, 0x00, 0x00, 0x00,
};


// wlan0 (phy #0): disassoc 48:5d:60:77:2d:cf -> c0:3f:0e:77:e8:7f reason 3:
// Deauthenticated because sending station is  [frame: a0 00 00 00 c0 3f 0e
// 77 e8 7f 48 5d 60 77 2d cf c0 3f 0e 77 e8 7f 00 00 03 00]

const unsigned char kDisassociateFrame[] = {
  0xa0, 0x00, 0x00, 0x00, 0xc0, 0x3f, 0x0e, 0x77,
  0xe8, 0x7f, 0x48, 0x5d, 0x60, 0x77, 0x2d, 0xcf,
  0xc0, 0x3f, 0x0e, 0x77, 0xe8, 0x7f, 0x00, 0x00,
  0x03, 0x00
};

const unsigned char kNL80211_CMD_DISASSOCIATE[] = {
  0x44, 0x00, 0x00, 0x00, 0x13, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x28, 0x01, 0x00, 0x00, 0x08, 0x00, 0x01, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x03, 0x00,
  0x04, 0x00, 0x00, 0x00, 0x1e, 0x00, 0x33, 0x00,
  0xa0, 0x00, 0x00, 0x00, 0xc0, 0x3f, 0x0e, 0x77,
  0xe8, 0x7f, 0x48, 0x5d, 0x60, 0x77, 0x2d, 0xcf,
  0xc0, 0x3f, 0x0e, 0x77, 0xe8, 0x7f, 0x00, 0x00,
  0x03, 0x00, 0x00, 0x00,
};

// This is just a NL80211_CMD_NEW_STATION message with the command changed to
// 0xfe (which is, intentionally, not a supported command).

const unsigned char kCmdNL80211_CMD_UNKNOWN = 0xfe;
const unsigned char kNL80211_CMD_UNKNOWN[] = {
  0x34, 0x00, 0x00, 0x00, 0x13, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0xfe, 0x01, 0x00, 0x00, 0x08, 0x00, 0x03, 0x00,
  0x04, 0x00, 0x00, 0x00, 0x0a, 0x00, 0x06, 0x00,
  0xc0, 0x3f, 0x0e, 0x77, 0xe8, 0x7f, 0x00, 0x00,
  0x08, 0x00, 0x2e, 0x00, 0x13, 0x01, 0x00, 0x00,
  0x04, 0x00, 0x15, 0x00,
};

}  // namespace

class NetlinkMessageTest : public Test {
 public:
  NetlinkMessageTest() {
    message_factory_.AddFactoryMethod(
        kNl80211FamilyId, Bind(&Nl80211Message::CreateMessage));
    Nl80211Message::SetMessageType(kNl80211FamilyId);
  }

 protected:
  // Helper function to provide an array of scan frequencies from a message's
  // NL80211_ATTR_SCAN_FREQUENCIES attribute.
  static bool GetScanFrequenciesFromMessage(const Nl80211Message& message,
                                            vector<uint32_t>* value) {
    if (!value) {
      LOG(ERROR) << "Null |value| parameter";
      return false;
    }

    AttributeListConstRefPtr frequency_list;
    if (!message.const_attributes()->ConstGetNestedAttributeList(
        NL80211_ATTR_SCAN_FREQUENCIES, &frequency_list) || !frequency_list) {
      LOG(ERROR) << "Couldn't get NL80211_ATTR_SCAN_FREQUENCIES attribute";
      return false;
    }

    AttributeIdIterator freq_iter(*frequency_list);
    value->clear();
    for (; !freq_iter.AtEnd(); freq_iter.Advance()) {
      uint32_t freq = 0;
      if (frequency_list->GetU32AttributeValue(freq_iter.GetId(), &freq)) {
        value->push_back(freq);
      }
    }
    return true;
  }

  // Helper function to provide an array of SSIDs from a message's
  // NL80211_ATTR_SCAN_SSIDS attribute.
  static bool GetScanSsidsFromMessage(const Nl80211Message& message,
                                      vector<string>* value) {
    if (!value) {
      LOG(ERROR) << "Null |value| parameter";
      return false;
    }

    AttributeListConstRefPtr ssid_list;
    if (!message.const_attributes()->ConstGetNestedAttributeList(
        NL80211_ATTR_SCAN_SSIDS, &ssid_list) || !ssid_list) {
      LOG(ERROR) << "Couldn't get NL80211_ATTR_SCAN_SSIDS attribute";
      return false;
    }

    AttributeIdIterator ssid_iter(*ssid_list);
    value->clear();
    for (; !ssid_iter.AtEnd(); ssid_iter.Advance()) {
      string ssid;
      if (ssid_list->GetStringAttributeValue(ssid_iter.GetId(), &ssid)) {
        value->push_back(ssid);
      }
    }
    return true;
  }

  NetlinkMessageFactory message_factory_;
};

TEST_F(NetlinkMessageTest, Parse_NL80211_CMD_TRIGGER_SCAN) {
  NetlinkPacket trigger_scan_packet(
      kNL80211_CMD_TRIGGER_SCAN, sizeof(kNL80211_CMD_TRIGGER_SCAN));
  unique_ptr<NetlinkMessage> netlink_message(
      message_factory_.CreateMessage(
          &trigger_scan_packet, NetlinkMessage::MessageContext()));

  EXPECT_NE(nullptr, netlink_message);
  EXPECT_EQ(kNl80211FamilyId, netlink_message->message_type());
  // The following is legal if the message_type is kNl80211FamilyId.
  unique_ptr<Nl80211Message> message(static_cast<Nl80211Message*>(
      netlink_message.release()));

  EXPECT_EQ(NL80211_CMD_TRIGGER_SCAN, message->command());

  {
    uint32_t value;
    EXPECT_TRUE(message->const_attributes()->GetU32AttributeValue(
        NL80211_ATTR_WIPHY, &value));
    EXPECT_EQ(kWiPhy, value);
  }

  {
    uint32_t value;
    EXPECT_TRUE(message->const_attributes()->GetU32AttributeValue(
        NL80211_ATTR_IFINDEX, &value));
    EXPECT_EQ(kExpectedIfIndex, value);
  }

  // Make sure the scan frequencies in the attribute are the ones we expect.
  {
    vector<uint32_t>list;
    EXPECT_TRUE(GetScanFrequenciesFromMessage(*message, &list));
    EXPECT_EQ(list.size(), arraysize(kScanFrequencyTrigger));
    int i = 0;
    vector<uint32_t>::const_iterator j = list.begin();
    while (j != list.end()) {
      EXPECT_EQ(kScanFrequencyTrigger[i], *j);
      ++i;
      ++j;
    }
  }

  {
    vector<string> ssids;
    EXPECT_TRUE(GetScanSsidsFromMessage(*message, &ssids));
    EXPECT_EQ(1, ssids.size());
    EXPECT_EQ(0, ssids[0].compare(""));  // Expect a single, empty SSID.
  }

  EXPECT_TRUE(message->const_attributes()->IsFlagAttributeTrue(
      NL80211_ATTR_SUPPORT_MESH_AUTH));
}

TEST_F(NetlinkMessageTest, Parse_NL80211_CMD_NEW_SCAN_RESULTS) {
  NetlinkPacket new_scan_results_packet(
      kNL80211_CMD_NEW_SCAN_RESULTS, sizeof(kNL80211_CMD_NEW_SCAN_RESULTS));
  unique_ptr<NetlinkMessage> netlink_message(message_factory_.CreateMessage(
      &new_scan_results_packet, NetlinkMessage::MessageContext()));

  EXPECT_NE(nullptr, netlink_message);
  EXPECT_EQ(kNl80211FamilyId, netlink_message->message_type());
  // The following is legal if the message_type is kNl80211FamilyId.
  unique_ptr<Nl80211Message> message(static_cast<Nl80211Message*>(
      netlink_message.release()));

  EXPECT_EQ(NL80211_CMD_NEW_SCAN_RESULTS, message->command());

  {
    uint32_t value;
    EXPECT_TRUE(message->const_attributes()->GetU32AttributeValue(
        NL80211_ATTR_WIPHY, &value));
    EXPECT_EQ(kWiPhy, value);
  }

  {
    uint32_t value;
    EXPECT_TRUE(message->const_attributes()->GetU32AttributeValue(
        NL80211_ATTR_IFINDEX, &value));
    EXPECT_EQ(kExpectedIfIndex, value);
  }

  // Make sure the scan frequencies in the attribute are the ones we expect.
  {
    vector<uint32_t>list;
    EXPECT_TRUE(GetScanFrequenciesFromMessage(*message, &list));
    EXPECT_EQ(arraysize(kScanFrequencyResults), list.size());
    int i = 0;
    vector<uint32_t>::const_iterator j = list.begin();
    while (j != list.end()) {
      EXPECT_EQ(kScanFrequencyResults[i], *j);
      ++i;
      ++j;
    }
  }

  {
    vector<string> ssids;
    EXPECT_TRUE(GetScanSsidsFromMessage(*message, &ssids));
    EXPECT_EQ(1, ssids.size());
    EXPECT_EQ(0, ssids[0].compare(""));  // Expect a single, empty SSID.
  }

  EXPECT_TRUE(message->const_attributes()->IsFlagAttributeTrue(
      NL80211_ATTR_SUPPORT_MESH_AUTH));
}

TEST_F(NetlinkMessageTest, Parse_NL80211_CMD_NEW_STATION) {
  NetlinkPacket netlink_packet(
      kNL80211_CMD_NEW_STATION, sizeof(kNL80211_CMD_NEW_STATION));
  unique_ptr<NetlinkMessage> netlink_message(message_factory_.CreateMessage(
      &netlink_packet, NetlinkMessage::MessageContext()));

  EXPECT_NE(nullptr, netlink_message);
  EXPECT_EQ(kNl80211FamilyId, netlink_message->message_type());
  // The following is legal if the message_type is kNl80211FamilyId.
  unique_ptr<Nl80211Message> message(static_cast<Nl80211Message*>(
      netlink_message.release()));
  EXPECT_EQ(NL80211_CMD_NEW_STATION, message->command());

  {
    uint32_t value;
    EXPECT_TRUE(message->const_attributes()->GetU32AttributeValue(
        NL80211_ATTR_IFINDEX, &value));
    EXPECT_EQ(kExpectedIfIndex, value);
  }

  {
    string value;
    EXPECT_TRUE(message->const_attributes()->GetAttributeAsString(
        NL80211_ATTR_MAC, &value));
    EXPECT_EQ(0, strncmp(value.c_str(), kExpectedMacAddress, value.length()));
  }

  {
    AttributeListConstRefPtr nested;
    EXPECT_TRUE(message->const_attributes()->ConstGetNestedAttributeList(
        NL80211_ATTR_STA_INFO, &nested));
  }

  {
    uint32_t value;
    EXPECT_TRUE(message->const_attributes()->GetU32AttributeValue(
        NL80211_ATTR_GENERATION, &value));
    EXPECT_EQ(kNewStationExpectedGeneration, value);
  }
}

TEST_F(NetlinkMessageTest, Parse_NL80211_CMD_AUTHENTICATE) {
  NetlinkPacket netlink_packet(
      kNL80211_CMD_AUTHENTICATE, sizeof(kNL80211_CMD_AUTHENTICATE));
  unique_ptr<NetlinkMessage> netlink_message(message_factory_.CreateMessage(
      &netlink_packet, NetlinkMessage::MessageContext()));

  EXPECT_NE(nullptr, netlink_message);
  EXPECT_EQ(kNl80211FamilyId, netlink_message->message_type());
  // The following is legal if the message_type is kNl80211FamilyId.
  unique_ptr<Nl80211Message> message(static_cast<Nl80211Message*>(
      netlink_message.release()));
  EXPECT_EQ(NL80211_CMD_AUTHENTICATE, message->command());

  {
    uint32_t value;
    EXPECT_TRUE(message->const_attributes()->GetU32AttributeValue(
        NL80211_ATTR_WIPHY, &value));
    EXPECT_EQ(kWiPhy, value);
  }

  {
    uint32_t value;
    EXPECT_TRUE(message->const_attributes()->GetU32AttributeValue(
        NL80211_ATTR_IFINDEX, &value));
    EXPECT_EQ(kExpectedIfIndex, value);
  }

  {
    ByteString rawdata;
    EXPECT_TRUE(message->const_attributes()->GetRawAttributeValue(
        NL80211_ATTR_FRAME, &rawdata));
    EXPECT_FALSE(rawdata.IsEmpty());
    Nl80211Frame frame(rawdata);
    Nl80211Frame expected_frame(ByteString(kAuthenticateFrame,
                                           sizeof(kAuthenticateFrame)));
    EXPECT_EQ(Nl80211Frame::kAuthFrameType, frame.frame_type());
    EXPECT_TRUE(frame.IsEqual(expected_frame));
  }
}

TEST_F(NetlinkMessageTest, Parse_NL80211_CMD_ASSOCIATE) {
  NetlinkPacket netlink_packet(
      kNL80211_CMD_ASSOCIATE, sizeof(kNL80211_CMD_ASSOCIATE));
  unique_ptr<NetlinkMessage> netlink_message(message_factory_.CreateMessage(
      &netlink_packet, NetlinkMessage::MessageContext()));

  EXPECT_NE(nullptr, netlink_message);
  EXPECT_EQ(kNl80211FamilyId, netlink_message->message_type());
  // The following is legal if the message_type is kNl80211FamilyId.
  unique_ptr<Nl80211Message> message(static_cast<Nl80211Message*>(
      netlink_message.release()));
  EXPECT_EQ(NL80211_CMD_ASSOCIATE, message->command());

  {
    uint32_t value;
    EXPECT_TRUE(message->const_attributes()->GetU32AttributeValue(
        NL80211_ATTR_WIPHY, &value));
    EXPECT_EQ(kWiPhy, value);
  }

  {
    uint32_t value;
    EXPECT_TRUE(message->const_attributes()->GetU32AttributeValue(
        NL80211_ATTR_IFINDEX, &value));
    EXPECT_EQ(kExpectedIfIndex, value);
  }

  {
    ByteString rawdata;
    EXPECT_TRUE(message->const_attributes()->GetRawAttributeValue(
        NL80211_ATTR_FRAME, &rawdata));
    EXPECT_FALSE(rawdata.IsEmpty());
    Nl80211Frame frame(rawdata);
    Nl80211Frame expected_frame(ByteString(kAssociateFrame,
                                           sizeof(kAssociateFrame)));
    EXPECT_EQ(Nl80211Frame::kAssocResponseFrameType, frame.frame_type());
    EXPECT_TRUE(frame.IsEqual(expected_frame));
  }
}

TEST_F(NetlinkMessageTest, Parse_NL80211_CMD_CONNECT) {
  NetlinkPacket netlink_packet(
      kNL80211_CMD_CONNECT, sizeof(kNL80211_CMD_CONNECT));
  unique_ptr<NetlinkMessage> netlink_message(message_factory_.CreateMessage(
      &netlink_packet, NetlinkMessage::MessageContext()));

  EXPECT_NE(nullptr, netlink_message);
  EXPECT_EQ(kNl80211FamilyId, netlink_message->message_type());
  // The following is legal if the message_type is kNl80211FamilyId.
  unique_ptr<Nl80211Message> message(static_cast<Nl80211Message*>(
      netlink_message.release()));
  EXPECT_EQ(NL80211_CMD_CONNECT, message->command());

  {
    uint32_t value;
    EXPECT_TRUE(message->const_attributes()->GetU32AttributeValue(
        NL80211_ATTR_WIPHY, &value));
    EXPECT_EQ(kWiPhy, value);
  }

  {
    uint32_t value;
    EXPECT_TRUE(message->const_attributes()->GetU32AttributeValue(
        NL80211_ATTR_IFINDEX, &value));
    EXPECT_EQ(kExpectedIfIndex, value);
  }

  {
    string value;
    EXPECT_TRUE(message->const_attributes()->GetAttributeAsString(
        NL80211_ATTR_MAC, &value));
    EXPECT_EQ(0, strncmp(value.c_str(), kExpectedMacAddress, value.length()));
  }

  {
    uint16_t value;
    EXPECT_TRUE(message->const_attributes()->GetU16AttributeValue(
        NL80211_ATTR_STATUS_CODE, &value));
    EXPECT_EQ(kExpectedConnectStatus, value);
  }

  {
    ByteString rawdata;
    EXPECT_TRUE(message->const_attributes()->GetRawAttributeValue(
        NL80211_ATTR_RESP_IE, &rawdata));
    EXPECT_TRUE(rawdata.Equals(
        ByteString(kRespIeBytes, arraysize(kRespIeBytes))));
  }
}

TEST_F(NetlinkMessageTest, Build_NL80211_CMD_CONNECT) {
  // Build the message that is found in kNL80211_CMD_CONNECT.
  ConnectMessage message;
  EXPECT_TRUE(message.attributes()->CreateNl80211Attribute(
      NL80211_ATTR_WIPHY, NetlinkMessage::MessageContext()));
  EXPECT_TRUE(
      message.attributes()->SetU32AttributeValue(NL80211_ATTR_WIPHY, kWiPhy));

  EXPECT_TRUE(message.attributes()->CreateNl80211Attribute(
      NL80211_ATTR_IFINDEX, NetlinkMessage::MessageContext()));
  EXPECT_TRUE(message.attributes()->SetU32AttributeValue(
      NL80211_ATTR_IFINDEX, kExpectedIfIndex));

  EXPECT_TRUE(message.attributes()->CreateNl80211Attribute(
      NL80211_ATTR_MAC, NetlinkMessage::MessageContext()));
  EXPECT_TRUE(message.attributes()->SetRawAttributeValue(NL80211_ATTR_MAC,
      ByteString(kMacAddressBytes, arraysize(kMacAddressBytes))));

  // In the middle, let's try adding an attribute without populating it.
  EXPECT_TRUE(message.attributes()->CreateNl80211Attribute(
      NL80211_ATTR_REG_TYPE, NetlinkMessage::MessageContext()));

  EXPECT_TRUE(message.attributes()->CreateNl80211Attribute(
      NL80211_ATTR_STATUS_CODE, NetlinkMessage::MessageContext()));
  EXPECT_TRUE(message.attributes()->SetU16AttributeValue(
      NL80211_ATTR_STATUS_CODE, kExpectedConnectStatus));

  EXPECT_TRUE(message.attributes()->CreateNl80211Attribute(
      NL80211_ATTR_RESP_IE, NetlinkMessage::MessageContext()));
  EXPECT_TRUE(message.attributes()->SetRawAttributeValue(NL80211_ATTR_RESP_IE,
      ByteString(kRespIeBytes, arraysize(kRespIeBytes))));

  // Encode the message to a ByteString and remove all the run-specific
  // values.
  static const uint32_t kArbitrarySequenceNumber = 42;
  ByteString message_bytes = message.Encode(kArbitrarySequenceNumber);
  nlmsghdr* header = reinterpret_cast<nlmsghdr*>(message_bytes.GetData());
  header->nlmsg_flags = 0;  // Overwrite with known values.
  header->nlmsg_seq = 0;
  header->nlmsg_pid = 0;

  // Verify that the messages are equal.
  EXPECT_TRUE(message_bytes.Equals(
      ByteString(kNL80211_CMD_CONNECT, arraysize(kNL80211_CMD_CONNECT))));
}


TEST_F(NetlinkMessageTest, Parse_NL80211_CMD_DEAUTHENTICATE) {
  NetlinkPacket netlink_packet(
      kNL80211_CMD_DEAUTHENTICATE, sizeof(kNL80211_CMD_DEAUTHENTICATE));
  unique_ptr<NetlinkMessage> netlink_message(message_factory_.CreateMessage(
      &netlink_packet, NetlinkMessage::MessageContext()));

  EXPECT_NE(nullptr, netlink_message);
  EXPECT_EQ(kNl80211FamilyId, netlink_message->message_type());
  // The following is legal if the message_type is kNl80211FamilyId.
  unique_ptr<Nl80211Message> message(static_cast<Nl80211Message*>(
      netlink_message.release()));
  EXPECT_EQ(NL80211_CMD_DEAUTHENTICATE, message->command());

  {
    uint32_t value;
    EXPECT_TRUE(message->const_attributes()->GetU32AttributeValue(
        NL80211_ATTR_WIPHY, &value));
    EXPECT_EQ(kWiPhy, value);
  }

  {
    uint32_t value;
    EXPECT_TRUE(message->const_attributes()->GetU32AttributeValue(
        NL80211_ATTR_IFINDEX, &value));
    EXPECT_EQ(kExpectedIfIndex, value);
  }

  {
    ByteString rawdata;
    EXPECT_TRUE(message->const_attributes()->GetRawAttributeValue(
        NL80211_ATTR_FRAME, &rawdata));
    EXPECT_FALSE(rawdata.IsEmpty());
    Nl80211Frame frame(rawdata);
    Nl80211Frame expected_frame(ByteString(kDeauthenticateFrame,
                                           sizeof(kDeauthenticateFrame)));
    EXPECT_EQ(Nl80211Frame::kDeauthFrameType, frame.frame_type());
    EXPECT_TRUE(frame.IsEqual(expected_frame));
  }
}

TEST_F(NetlinkMessageTest, Parse_NL80211_CMD_DISCONNECT) {
  NetlinkPacket netlink_packet(
      kNL80211_CMD_DISCONNECT, sizeof(kNL80211_CMD_DISCONNECT));
  unique_ptr<NetlinkMessage> netlink_message(message_factory_.CreateMessage(
      &netlink_packet, NetlinkMessage::MessageContext()));

  EXPECT_NE(nullptr, netlink_message);
  EXPECT_EQ(kNl80211FamilyId, netlink_message->message_type());
  // The following is legal if the message_type is kNl80211FamilyId.
  unique_ptr<Nl80211Message> message(static_cast<Nl80211Message*>(
      netlink_message.release()));
  EXPECT_EQ(NL80211_CMD_DISCONNECT, message->command());

  {
    uint32_t value;
    EXPECT_TRUE(message->const_attributes()->GetU32AttributeValue(
        NL80211_ATTR_WIPHY, &value));
    EXPECT_EQ(kWiPhy, value);
  }

  {
    uint32_t value;
    EXPECT_TRUE(message->const_attributes()->GetU32AttributeValue(
        NL80211_ATTR_IFINDEX, &value));
    EXPECT_EQ(kExpectedIfIndex, value);
  }

  {
    uint16_t value;
    EXPECT_TRUE(message->const_attributes()->GetU16AttributeValue(
        NL80211_ATTR_REASON_CODE, &value));
    EXPECT_EQ(kExpectedDisconnectReason, value);
  }

  EXPECT_TRUE(message->const_attributes()->IsFlagAttributeTrue(
      NL80211_ATTR_DISCONNECTED_BY_AP));
}

TEST_F(NetlinkMessageTest, Parse_NL80211_CMD_NOTIFY_CQM) {
  NetlinkPacket netlink_packet(
      kNL80211_CMD_NOTIFY_CQM, sizeof(kNL80211_CMD_NOTIFY_CQM));
  unique_ptr<NetlinkMessage> netlink_message(message_factory_.CreateMessage(
      &netlink_packet, NetlinkMessage::MessageContext()));

  EXPECT_NE(nullptr, netlink_message);
  EXPECT_EQ(kNl80211FamilyId, netlink_message->message_type());
  // The following is legal if the message_type is kNl80211FamilyId.
  unique_ptr<Nl80211Message> message(static_cast<Nl80211Message*>(
      netlink_message.release()));
  EXPECT_EQ(NL80211_CMD_NOTIFY_CQM, message->command());

  {
    uint32_t value;
    EXPECT_TRUE(message->const_attributes()->GetU32AttributeValue(
        NL80211_ATTR_WIPHY, &value));
    EXPECT_EQ(kWiPhy, value);
  }

  {
    uint32_t value;
    EXPECT_TRUE(message->const_attributes()->GetU32AttributeValue(
        NL80211_ATTR_IFINDEX, &value));
    EXPECT_EQ(kExpectedIfIndex, value);
  }

  {
    string value;
    EXPECT_TRUE(message->const_attributes()->GetAttributeAsString(
        NL80211_ATTR_MAC, &value));
    EXPECT_EQ(0, strncmp(value.c_str(), kExpectedMacAddress, value.length()));
  }

  {
    AttributeListConstRefPtr nested;
    EXPECT_TRUE(message->const_attributes()->ConstGetNestedAttributeList(
        NL80211_ATTR_CQM, &nested));
    uint32_t threshold_event;
    EXPECT_FALSE(nested->GetU32AttributeValue(
        NL80211_ATTR_CQM_RSSI_THRESHOLD_EVENT, &threshold_event));
    uint32_t pkt_loss_event;
    EXPECT_TRUE(nested->GetU32AttributeValue(
        NL80211_ATTR_CQM_PKT_LOSS_EVENT, &pkt_loss_event));
    EXPECT_EQ(kExpectedCqmNotAcked, pkt_loss_event);
  }
}

TEST_F(NetlinkMessageTest, Parse_NL80211_CMD_DISASSOCIATE) {
  NetlinkPacket netlink_packet(
      kNL80211_CMD_DISASSOCIATE, sizeof(kNL80211_CMD_DISASSOCIATE));
  unique_ptr<NetlinkMessage> netlink_message(message_factory_.CreateMessage(
      &netlink_packet, NetlinkMessage::MessageContext()));

  EXPECT_NE(nullptr, netlink_message);
  EXPECT_EQ(kNl80211FamilyId, netlink_message->message_type());
  // The following is legal if the message_type is kNl80211FamilyId.
  unique_ptr<Nl80211Message> message(static_cast<Nl80211Message*>(
      netlink_message.release()));
  EXPECT_EQ(NL80211_CMD_DISASSOCIATE, message->command());


  {
    uint32_t value;
    EXPECT_TRUE(message->const_attributes()->GetU32AttributeValue(
        NL80211_ATTR_WIPHY, &value));
    EXPECT_EQ(kWiPhy, value);
  }

  {
    uint32_t value;
    EXPECT_TRUE(message->const_attributes()->GetU32AttributeValue(
        NL80211_ATTR_IFINDEX, &value));
    EXPECT_EQ(kExpectedIfIndex, value);
  }

  {
    ByteString rawdata;
    EXPECT_TRUE(message->const_attributes()->GetRawAttributeValue(
        NL80211_ATTR_FRAME, &rawdata));
    EXPECT_FALSE(rawdata.IsEmpty());
    Nl80211Frame frame(rawdata);
    Nl80211Frame expected_frame(ByteString(kDisassociateFrame,
                                           sizeof(kDisassociateFrame)));
    EXPECT_EQ(Nl80211Frame::kDisassocFrameType, frame.frame_type());
    EXPECT_TRUE(frame.IsEqual(expected_frame));
  }
}

// This test is to ensure that an unknown nl80211 message generates an
// Nl80211UnknownMessage with all Nl80211 parts.
TEST_F(NetlinkMessageTest, Parse_NL80211_CMD_UNKNOWN) {
  NetlinkPacket netlink_packet(
      kNL80211_CMD_UNKNOWN, sizeof(kNL80211_CMD_UNKNOWN));
  unique_ptr<NetlinkMessage> netlink_message(message_factory_.CreateMessage(
      &netlink_packet, NetlinkMessage::MessageContext()));
  ASSERT_NE(nullptr, netlink_message.get());
  EXPECT_EQ(kNl80211FamilyId, netlink_message->message_type());
  // The following is legal if the message_type is kNl80211FamilyId.
  unique_ptr<Nl80211Message> message(static_cast<Nl80211Message*>(
      netlink_message.release()));
  EXPECT_EQ(kCmdNL80211_CMD_UNKNOWN, message->command());
}

}  // namespace shill
