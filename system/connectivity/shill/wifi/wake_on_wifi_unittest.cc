//
// Copyright (C) 2014 The Android Open Source Project
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

#include "shill/wifi/wake_on_wifi.h"

#include <linux/nl80211.h>

#include <set>
#include <string>
#include <utility>

#include <base/message_loop/message_loop.h>
#if defined(__ANDROID__)
#include <dbus/service_constants.h>
#else
#include <chromeos/dbus/service_constants.h>
#endif  // __ANDROID__
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "shill/error.h"
#include "shill/event_dispatcher.h"
#include "shill/ip_address_store.h"
#include "shill/logging.h"
#include "shill/mock_event_dispatcher.h"
#include "shill/mock_log.h"
#include "shill/mock_metrics.h"
#include "shill/net/byte_string.h"
#include "shill/net/ip_address.h"
#include "shill/net/mock_netlink_manager.h"
#include "shill/net/mock_time.h"
#include "shill/net/netlink_message_matchers.h"
#include "shill/net/netlink_packet.h"
#include "shill/net/nl80211_message.h"
#include "shill/net/shill_time.h"
#include "shill/nice_mock_control.h"
#include "shill/test_event_dispatcher.h"
#include "shill/testing.h"

using base::Bind;
using base::Closure;
using base::Unretained;
using std::set;
using std::string;
using std::vector;
using testing::_;
using ::testing::AnyNumber;
using ::testing::HasSubstr;
using ::testing::Return;

namespace shill {

namespace {

const uint16_t kNl80211FamilyId = 0x13;

const uint8_t kSSIDBytes1[] = {0x47, 0x6f, 0x6f, 0x67, 0x6c, 0x65,
                               0x47, 0x75, 0x65, 0x73, 0x74};
// Bytes representing a NL80211_CMD_SET_WOWLAN reporting that the system woke
// up because of an SSID match. The net detect results report a single SSID
// match represented by kSSIDBytes1, occurring in the frequencies in
// kSSID1FreqMatches.
const uint8_t kWakeReasonSSIDNlMsg[] = {
    0x90, 0x00, 0x00, 0x00, 0x13, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x4a, 0x01, 0x00, 0x00, 0x08, 0x00, 0x01, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x99, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x03, 0x00, 0x02, 0x00, 0x00, 0x00,
    0x60, 0x00, 0x75, 0x00, 0x5c, 0x00, 0x13, 0x00, 0x58, 0x00, 0x00, 0x00,
    0x0f, 0x00, 0x34, 0x00, 0x47, 0x6f, 0x6f, 0x67, 0x6c, 0x65, 0x47, 0x75,
    0x65, 0x73, 0x74, 0x00, 0x44, 0x00, 0x2c, 0x00, 0x08, 0x00, 0x00, 0x00,
    0x6c, 0x09, 0x00, 0x00, 0x08, 0x00, 0x01, 0x00, 0x85, 0x09, 0x00, 0x00,
    0x08, 0x00, 0x02, 0x00, 0x9e, 0x09, 0x00, 0x00, 0x08, 0x00, 0x03, 0x00,
    0x3c, 0x14, 0x00, 0x00, 0x08, 0x00, 0x04, 0x00, 0x78, 0x14, 0x00, 0x00,
    0x08, 0x00, 0x05, 0x00, 0x71, 0x16, 0x00, 0x00, 0x08, 0x00, 0x06, 0x00,
    0xad, 0x16, 0x00, 0x00, 0x08, 0x00, 0x07, 0x00, 0xc1, 0x16, 0x00, 0x00};
const uint32_t kTimeToNextLeaseRenewalShort = 1;
const uint32_t kTimeToNextLeaseRenewalLong = 1000;
const uint32_t kNetDetectScanIntervalSeconds = 120;
// These blobs represent NL80211 messages from the kernel reporting the NIC's
// wake-on-packet settings, sent in response to NL80211_CMD_GET_WOWLAN requests.
const uint8_t kResponseNoIPAddresses[] = {
    0x14, 0x00, 0x00, 0x00, 0x13, 0x00, 0x01, 0x00, 0x01, 0x00,
    0x00, 0x00, 0x57, 0x40, 0x00, 0x00, 0x49, 0x01, 0x00, 0x00};
const uint8_t kResponseIPV40[] = {
    0x4C, 0x00, 0x00, 0x00, 0x13, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00,
    0x00, 0x57, 0x40, 0x00, 0x00, 0x49, 0x01, 0x00, 0x00, 0x38, 0x00,
    0x75, 0x00, 0x34, 0x00, 0x04, 0x00, 0x30, 0x00, 0x01, 0x00, 0x08,
    0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x3C, 0x22, 0x00, 0x02, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xC0, 0xA8, 0x0A, 0x14, 0x00, 0x00};
const uint8_t kResponseIPV40WakeOnDisconnect[] = {
    0x50, 0x00, 0x00, 0x00, 0x13, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x57, 0x40, 0x00, 0x00, 0x49, 0x01, 0x00, 0x00, 0x3C, 0x00, 0x75, 0x00,
    0x04, 0x00, 0x02, 0x00, 0x34, 0x00, 0x04, 0x00, 0x30, 0x00, 0x01, 0x00,
    0x08, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x3C, 0x22, 0x00, 0x02, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0xC0, 0xA8, 0x0A, 0x14, 0x00, 0x00};
const uint8_t kResponseIPV401[] = {
    0x7C, 0x00, 0x00, 0x00, 0x13, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x57, 0x40, 0x00, 0x00, 0x49, 0x01, 0x00, 0x00, 0x68, 0x00, 0x75, 0x00,
    0x64, 0x00, 0x04, 0x00, 0x30, 0x00, 0x01, 0x00, 0x08, 0x00, 0x01, 0x00,
    0x00, 0x00, 0x00, 0x3C, 0x22, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02,
    0x03, 0x04, 0x00, 0x00, 0x30, 0x00, 0x02, 0x00, 0x08, 0x00, 0x01, 0x00,
    0x00, 0x00, 0x00, 0x3C, 0x22, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC0, 0xA8,
    0x0A, 0x14, 0x00, 0x00};
const uint8_t kResponseIPV401IPV60[] = {
    0xB8, 0x00, 0x00, 0x00, 0x13, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x57, 0x40, 0x00, 0x00, 0x49, 0x01, 0x00, 0x00, 0xA4, 0x00, 0x75, 0x00,
    0xA0, 0x00, 0x04, 0x00, 0x30, 0x00, 0x01, 0x00, 0x08, 0x00, 0x01, 0x00,
    0x00, 0x00, 0x00, 0x3C, 0x22, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02,
    0x03, 0x04, 0x00, 0x00, 0x30, 0x00, 0x02, 0x00, 0x08, 0x00, 0x01, 0x00,
    0x00, 0x00, 0x00, 0x3C, 0x22, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC0, 0xA8,
    0x0A, 0x14, 0x00, 0x00, 0x3C, 0x00, 0x03, 0x00, 0x09, 0x00, 0x01, 0x00,
    0x00, 0x00, 0xC0, 0xFF, 0x3F, 0x00, 0x00, 0x00, 0x2A, 0x00, 0x02, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFE, 0xDC,
    0xBA, 0x98, 0x76, 0x54, 0x32, 0x10, 0xFE, 0xDC, 0xBA, 0x98, 0x76, 0x54,
    0x32, 0x10, 0x00, 0x00};
const uint8_t kResponseIPV401IPV601[] = {
    0xF4, 0x00, 0x00, 0x00, 0x13, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x57, 0x40, 0x00, 0x00, 0x49, 0x01, 0x00, 0x00, 0xE0, 0x00, 0x75, 0x00,
    0xDC, 0x00, 0x04, 0x00, 0x30, 0x00, 0x01, 0x00, 0x08, 0x00, 0x01, 0x00,
    0x00, 0x00, 0x00, 0x3C, 0x22, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02,
    0x03, 0x04, 0x00, 0x00, 0x3C, 0x00, 0x02, 0x00, 0x09, 0x00, 0x01, 0x00,
    0x00, 0x00, 0xC0, 0xFF, 0x3F, 0x00, 0x00, 0x00, 0x2A, 0x00, 0x02, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x80,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x08, 0x00, 0x20, 0x0C,
    0x41, 0x7A, 0x00, 0x00, 0x30, 0x00, 0x03, 0x00, 0x08, 0x00, 0x01, 0x00,
    0x00, 0x00, 0x00, 0x3C, 0x22, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC0, 0xA8,
    0x0A, 0x14, 0x00, 0x00, 0x3C, 0x00, 0x04, 0x00, 0x09, 0x00, 0x01, 0x00,
    0x00, 0x00, 0xC0, 0xFF, 0x3F, 0x00, 0x00, 0x00, 0x2A, 0x00, 0x02, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFE, 0xDC,
    0xBA, 0x98, 0x76, 0x54, 0x32, 0x10, 0xFE, 0xDC, 0xBA, 0x98, 0x76, 0x54,
    0x32, 0x10, 0x00, 0x00};
// This blob represents an NL80211 messages from the kernel reporting that the
// NIC is programmed to wake on the SSIDs represented by kSSIDBytes1 and
// kSSIDBytes2, and scans for these SSIDs at interval
// kNetDetectScanIntervalSeconds.
const uint8_t kResponseWakeOnSSID[] = {
    0x60, 0x01, 0x00, 0x00, 0x13, 0x00, 0x00, 0x00, 0x9a, 0x01, 0x00, 0x00,
    0xfa, 0x02, 0x00, 0x00, 0x49, 0x01, 0x00, 0x00, 0x4c, 0x01, 0x75, 0x00,
    0x48, 0x01, 0x12, 0x00, 0x08, 0x00, 0x77, 0x00, 0xc0, 0xd4, 0x01, 0x00,
    0x0c, 0x01, 0x2c, 0x00, 0x08, 0x00, 0x00, 0x00, 0x6c, 0x09, 0x00, 0x00,
    0x08, 0x00, 0x01, 0x00, 0x71, 0x09, 0x00, 0x00, 0x08, 0x00, 0x02, 0x00,
    0x76, 0x09, 0x00, 0x00, 0x08, 0x00, 0x03, 0x00, 0x7b, 0x09, 0x00, 0x00,
    0x08, 0x00, 0x04, 0x00, 0x80, 0x09, 0x00, 0x00, 0x08, 0x00, 0x05, 0x00,
    0x85, 0x09, 0x00, 0x00, 0x08, 0x00, 0x06, 0x00, 0x8a, 0x09, 0x00, 0x00,
    0x08, 0x00, 0x07, 0x00, 0x8f, 0x09, 0x00, 0x00, 0x08, 0x00, 0x08, 0x00,
    0x94, 0x09, 0x00, 0x00, 0x08, 0x00, 0x09, 0x00, 0x99, 0x09, 0x00, 0x00,
    0x08, 0x00, 0x0a, 0x00, 0x9e, 0x09, 0x00, 0x00, 0x08, 0x00, 0x0b, 0x00,
    0x3c, 0x14, 0x00, 0x00, 0x08, 0x00, 0x0c, 0x00, 0x50, 0x14, 0x00, 0x00,
    0x08, 0x00, 0x0d, 0x00, 0x64, 0x14, 0x00, 0x00, 0x08, 0x00, 0x0e, 0x00,
    0x78, 0x14, 0x00, 0x00, 0x08, 0x00, 0x0f, 0x00, 0x8c, 0x14, 0x00, 0x00,
    0x08, 0x00, 0x10, 0x00, 0xa0, 0x14, 0x00, 0x00, 0x08, 0x00, 0x11, 0x00,
    0xb4, 0x14, 0x00, 0x00, 0x08, 0x00, 0x12, 0x00, 0xc8, 0x14, 0x00, 0x00,
    0x08, 0x00, 0x13, 0x00, 0x7c, 0x15, 0x00, 0x00, 0x08, 0x00, 0x14, 0x00,
    0x90, 0x15, 0x00, 0x00, 0x08, 0x00, 0x15, 0x00, 0xa4, 0x15, 0x00, 0x00,
    0x08, 0x00, 0x16, 0x00, 0xb8, 0x15, 0x00, 0x00, 0x08, 0x00, 0x17, 0x00,
    0xcc, 0x15, 0x00, 0x00, 0x08, 0x00, 0x18, 0x00, 0x1c, 0x16, 0x00, 0x00,
    0x08, 0x00, 0x19, 0x00, 0x30, 0x16, 0x00, 0x00, 0x08, 0x00, 0x1a, 0x00,
    0x44, 0x16, 0x00, 0x00, 0x08, 0x00, 0x1b, 0x00, 0x58, 0x16, 0x00, 0x00,
    0x08, 0x00, 0x1c, 0x00, 0x71, 0x16, 0x00, 0x00, 0x08, 0x00, 0x1d, 0x00,
    0x85, 0x16, 0x00, 0x00, 0x08, 0x00, 0x1e, 0x00, 0x99, 0x16, 0x00, 0x00,
    0x08, 0x00, 0x1f, 0x00, 0xad, 0x16, 0x00, 0x00, 0x08, 0x00, 0x20, 0x00,
    0xc1, 0x16, 0x00, 0x00, 0x30, 0x00, 0x84, 0x00, 0x14, 0x00, 0x00, 0x00,
    0x0f, 0x00, 0x01, 0x00, 0x47, 0x6f, 0x6f, 0x67, 0x6c, 0x65, 0x47, 0x75,
    0x65, 0x73, 0x74, 0x00, 0x18, 0x00, 0x01, 0x00, 0x12, 0x00, 0x01, 0x00,
    0x54, 0x50, 0x2d, 0x4c, 0x49, 0x4e, 0x4b, 0x5f, 0x38, 0x37, 0x36, 0x44,
    0x33, 0x35, 0x00, 0x00};
const uint8_t kSSIDBytes2[] = {0x54, 0x50, 0x2d, 0x4c, 0x49, 0x4e, 0x4b,
                               0x5f, 0x38, 0x37, 0x36, 0x44, 0x33, 0x35};

// Bytes representing a NL80211_CMD_NEW_WIPHY message reporting the WiFi
// capabilities of a NIC. This message reports that the NIC supports wake on
// pattern (on up to |kNewWiphyNlMsg_MaxPatterns| registered patterns), supports
// wake on SSID (on up to |kNewWiphyNlMsg_MaxSSIDs| SSIDs), and supports wake on
// disconnect.
const uint8_t kNewWiphyNlMsg[] = {
    0xb8, 0x0d, 0x00, 0x00, 0x13, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
    0xd9, 0x53, 0x00, 0x00, 0x03, 0x01, 0x00, 0x00, 0x08, 0x00, 0x01, 0x00,
    0x02, 0x00, 0x00, 0x00, 0x09, 0x00, 0x02, 0x00, 0x70, 0x68, 0x79, 0x30,
    0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x2e, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x05, 0x00, 0x3d, 0x00, 0x07, 0x00, 0x00, 0x00, 0x05, 0x00, 0x3e, 0x00,
    0x04, 0x00, 0x00, 0x00, 0x08, 0x00, 0x3f, 0x00, 0xff, 0xff, 0xff, 0xff,
    0x08, 0x00, 0x40, 0x00, 0xff, 0xff, 0xff, 0xff, 0x05, 0x00, 0x59, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x05, 0x00, 0x2b, 0x00, 0x14, 0x00, 0x00, 0x00,
    0x05, 0x00, 0x7b, 0x00, 0x14, 0x00, 0x00, 0x00, 0x06, 0x00, 0x38, 0x00,
    0xa9, 0x01, 0x00, 0x00, 0x06, 0x00, 0x7c, 0x00, 0xe6, 0x01, 0x00, 0x00,
    0x05, 0x00, 0x85, 0x00, 0x0b, 0x00, 0x00, 0x00, 0x04, 0x00, 0x68, 0x00,
    0x04, 0x00, 0x82, 0x00, 0x1c, 0x00, 0x39, 0x00, 0x04, 0xac, 0x0f, 0x00,
    0x02, 0xac, 0x0f, 0x00, 0x01, 0xac, 0x0f, 0x00, 0x05, 0xac, 0x0f, 0x00,
    0x06, 0xac, 0x0f, 0x00, 0x01, 0x72, 0x14, 0x00, 0x05, 0x00, 0x56, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x66, 0x00, 0x08, 0x00, 0x71, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x72, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x24, 0x00, 0x20, 0x00, 0x04, 0x00, 0x01, 0x00, 0x04, 0x00, 0x02, 0x00,
    0x04, 0x00, 0x03, 0x00, 0x04, 0x00, 0x04, 0x00, 0x04, 0x00, 0x06, 0x00,
    0x04, 0x00, 0x08, 0x00, 0x04, 0x00, 0x09, 0x00, 0x04, 0x00, 0x0a, 0x00,
    0x94, 0x05, 0x16, 0x00, 0xe8, 0x01, 0x00, 0x00, 0x14, 0x00, 0x03, 0x00,
    0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2c, 0x01,
    0x01, 0x00, 0x00, 0x00, 0x06, 0x00, 0x04, 0x00, 0xe2, 0x11, 0x00, 0x00,
    0x05, 0x00, 0x05, 0x00, 0x03, 0x00, 0x00, 0x00, 0x05, 0x00, 0x06, 0x00,
    0x05, 0x00, 0x00, 0x00, 0x18, 0x01, 0x01, 0x00, 0x14, 0x00, 0x00, 0x00,
    0x08, 0x00, 0x01, 0x00, 0x6c, 0x09, 0x00, 0x00, 0x08, 0x00, 0x06, 0x00,
    0x98, 0x08, 0x00, 0x00, 0x14, 0x00, 0x01, 0x00, 0x08, 0x00, 0x01, 0x00,
    0x71, 0x09, 0x00, 0x00, 0x08, 0x00, 0x06, 0x00, 0x98, 0x08, 0x00, 0x00,
    0x14, 0x00, 0x02, 0x00, 0x08, 0x00, 0x01, 0x00, 0x76, 0x09, 0x00, 0x00,
    0x08, 0x00, 0x06, 0x00, 0x98, 0x08, 0x00, 0x00, 0x14, 0x00, 0x03, 0x00,
    0x08, 0x00, 0x01, 0x00, 0x7b, 0x09, 0x00, 0x00, 0x08, 0x00, 0x06, 0x00,
    0x98, 0x08, 0x00, 0x00, 0x14, 0x00, 0x04, 0x00, 0x08, 0x00, 0x01, 0x00,
    0x80, 0x09, 0x00, 0x00, 0x08, 0x00, 0x06, 0x00, 0x98, 0x08, 0x00, 0x00,
    0x14, 0x00, 0x05, 0x00, 0x08, 0x00, 0x01, 0x00, 0x85, 0x09, 0x00, 0x00,
    0x08, 0x00, 0x06, 0x00, 0x98, 0x08, 0x00, 0x00, 0x14, 0x00, 0x06, 0x00,
    0x08, 0x00, 0x01, 0x00, 0x8a, 0x09, 0x00, 0x00, 0x08, 0x00, 0x06, 0x00,
    0x98, 0x08, 0x00, 0x00, 0x14, 0x00, 0x07, 0x00, 0x08, 0x00, 0x01, 0x00,
    0x8f, 0x09, 0x00, 0x00, 0x08, 0x00, 0x06, 0x00, 0x98, 0x08, 0x00, 0x00,
    0x14, 0x00, 0x08, 0x00, 0x08, 0x00, 0x01, 0x00, 0x94, 0x09, 0x00, 0x00,
    0x08, 0x00, 0x06, 0x00, 0x98, 0x08, 0x00, 0x00, 0x14, 0x00, 0x09, 0x00,
    0x08, 0x00, 0x01, 0x00, 0x99, 0x09, 0x00, 0x00, 0x08, 0x00, 0x06, 0x00,
    0x98, 0x08, 0x00, 0x00, 0x14, 0x00, 0x0a, 0x00, 0x08, 0x00, 0x01, 0x00,
    0x9e, 0x09, 0x00, 0x00, 0x08, 0x00, 0x06, 0x00, 0x98, 0x08, 0x00, 0x00,
    0x1c, 0x00, 0x0b, 0x00, 0x08, 0x00, 0x01, 0x00, 0xa3, 0x09, 0x00, 0x00,
    0x04, 0x00, 0x03, 0x00, 0x04, 0x00, 0x03, 0x00, 0x08, 0x00, 0x06, 0x00,
    0x98, 0x08, 0x00, 0x00, 0x1c, 0x00, 0x0c, 0x00, 0x08, 0x00, 0x01, 0x00,
    0xa8, 0x09, 0x00, 0x00, 0x04, 0x00, 0x03, 0x00, 0x04, 0x00, 0x03, 0x00,
    0x08, 0x00, 0x06, 0x00, 0x98, 0x08, 0x00, 0x00, 0xa0, 0x00, 0x02, 0x00,
    0x0c, 0x00, 0x00, 0x00, 0x08, 0x00, 0x01, 0x00, 0x0a, 0x00, 0x00, 0x00,
    0x10, 0x00, 0x01, 0x00, 0x08, 0x00, 0x01, 0x00, 0x14, 0x00, 0x00, 0x00,
    0x04, 0x00, 0x02, 0x00, 0x10, 0x00, 0x02, 0x00, 0x08, 0x00, 0x01, 0x00,
    0x37, 0x00, 0x00, 0x00, 0x04, 0x00, 0x02, 0x00, 0x10, 0x00, 0x03, 0x00,
    0x08, 0x00, 0x01, 0x00, 0x6e, 0x00, 0x00, 0x00, 0x04, 0x00, 0x02, 0x00,
    0x0c, 0x00, 0x04, 0x00, 0x08, 0x00, 0x01, 0x00, 0x3c, 0x00, 0x00, 0x00,
    0x0c, 0x00, 0x05, 0x00, 0x08, 0x00, 0x01, 0x00, 0x5a, 0x00, 0x00, 0x00,
    0x0c, 0x00, 0x06, 0x00, 0x08, 0x00, 0x01, 0x00, 0x78, 0x00, 0x00, 0x00,
    0x0c, 0x00, 0x07, 0x00, 0x08, 0x00, 0x01, 0x00, 0xb4, 0x00, 0x00, 0x00,
    0x0c, 0x00, 0x08, 0x00, 0x08, 0x00, 0x01, 0x00, 0xf0, 0x00, 0x00, 0x00,
    0x0c, 0x00, 0x09, 0x00, 0x08, 0x00, 0x01, 0x00, 0x68, 0x01, 0x00, 0x00,
    0x0c, 0x00, 0x0a, 0x00, 0x08, 0x00, 0x01, 0x00, 0xe0, 0x01, 0x00, 0x00,
    0x0c, 0x00, 0x0b, 0x00, 0x08, 0x00, 0x01, 0x00, 0x1c, 0x02, 0x00, 0x00,
    0xa8, 0x03, 0x01, 0x00, 0x14, 0x00, 0x03, 0x00, 0xff, 0xff, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2c, 0x01, 0x01, 0x00, 0x00, 0x00,
    0x06, 0x00, 0x04, 0x00, 0xe2, 0x11, 0x00, 0x00, 0x05, 0x00, 0x05, 0x00,
    0x03, 0x00, 0x00, 0x00, 0x05, 0x00, 0x06, 0x00, 0x05, 0x00, 0x00, 0x00,
    0x0c, 0x00, 0x07, 0x00, 0xfa, 0xff, 0x00, 0x00, 0xfa, 0xff, 0x00, 0x00,
    0x08, 0x00, 0x08, 0x00, 0xa0, 0x71, 0x80, 0x03, 0x00, 0x03, 0x01, 0x00,
    0x1c, 0x00, 0x00, 0x00, 0x08, 0x00, 0x01, 0x00, 0x3c, 0x14, 0x00, 0x00,
    0x04, 0x00, 0x03, 0x00, 0x04, 0x00, 0x03, 0x00, 0x08, 0x00, 0x06, 0x00,
    0x98, 0x08, 0x00, 0x00, 0x1c, 0x00, 0x01, 0x00, 0x08, 0x00, 0x01, 0x00,
    0x50, 0x14, 0x00, 0x00, 0x04, 0x00, 0x03, 0x00, 0x04, 0x00, 0x03, 0x00,
    0x08, 0x00, 0x06, 0x00, 0x98, 0x08, 0x00, 0x00, 0x1c, 0x00, 0x02, 0x00,
    0x08, 0x00, 0x01, 0x00, 0x64, 0x14, 0x00, 0x00, 0x04, 0x00, 0x03, 0x00,
    0x04, 0x00, 0x03, 0x00, 0x08, 0x00, 0x06, 0x00, 0x98, 0x08, 0x00, 0x00,
    0x1c, 0x00, 0x03, 0x00, 0x08, 0x00, 0x01, 0x00, 0x78, 0x14, 0x00, 0x00,
    0x04, 0x00, 0x03, 0x00, 0x04, 0x00, 0x03, 0x00, 0x08, 0x00, 0x06, 0x00,
    0x98, 0x08, 0x00, 0x00, 0x20, 0x00, 0x04, 0x00, 0x08, 0x00, 0x01, 0x00,
    0x8c, 0x14, 0x00, 0x00, 0x04, 0x00, 0x03, 0x00, 0x04, 0x00, 0x03, 0x00,
    0x04, 0x00, 0x05, 0x00, 0x08, 0x00, 0x06, 0x00, 0x98, 0x08, 0x00, 0x00,
    0x20, 0x00, 0x05, 0x00, 0x08, 0x00, 0x01, 0x00, 0xa0, 0x14, 0x00, 0x00,
    0x04, 0x00, 0x03, 0x00, 0x04, 0x00, 0x03, 0x00, 0x04, 0x00, 0x05, 0x00,
    0x08, 0x00, 0x06, 0x00, 0x98, 0x08, 0x00, 0x00, 0x20, 0x00, 0x06, 0x00,
    0x08, 0x00, 0x01, 0x00, 0xb4, 0x14, 0x00, 0x00, 0x04, 0x00, 0x03, 0x00,
    0x04, 0x00, 0x03, 0x00, 0x04, 0x00, 0x05, 0x00, 0x08, 0x00, 0x06, 0x00,
    0x98, 0x08, 0x00, 0x00, 0x20, 0x00, 0x07, 0x00, 0x08, 0x00, 0x01, 0x00,
    0xc8, 0x14, 0x00, 0x00, 0x04, 0x00, 0x03, 0x00, 0x04, 0x00, 0x03, 0x00,
    0x04, 0x00, 0x05, 0x00, 0x08, 0x00, 0x06, 0x00, 0x98, 0x08, 0x00, 0x00,
    0x20, 0x00, 0x08, 0x00, 0x08, 0x00, 0x01, 0x00, 0x7c, 0x15, 0x00, 0x00,
    0x04, 0x00, 0x03, 0x00, 0x04, 0x00, 0x03, 0x00, 0x04, 0x00, 0x05, 0x00,
    0x08, 0x00, 0x06, 0x00, 0x98, 0x08, 0x00, 0x00, 0x20, 0x00, 0x09, 0x00,
    0x08, 0x00, 0x01, 0x00, 0x90, 0x15, 0x00, 0x00, 0x04, 0x00, 0x03, 0x00,
    0x04, 0x00, 0x03, 0x00, 0x04, 0x00, 0x05, 0x00, 0x08, 0x00, 0x06, 0x00,
    0x98, 0x08, 0x00, 0x00, 0x20, 0x00, 0x0a, 0x00, 0x08, 0x00, 0x01, 0x00,
    0xa4, 0x15, 0x00, 0x00, 0x04, 0x00, 0x03, 0x00, 0x04, 0x00, 0x03, 0x00,
    0x04, 0x00, 0x05, 0x00, 0x08, 0x00, 0x06, 0x00, 0x98, 0x08, 0x00, 0x00,
    0x20, 0x00, 0x0b, 0x00, 0x08, 0x00, 0x01, 0x00, 0xb8, 0x15, 0x00, 0x00,
    0x04, 0x00, 0x03, 0x00, 0x04, 0x00, 0x03, 0x00, 0x04, 0x00, 0x05, 0x00,
    0x08, 0x00, 0x06, 0x00, 0x98, 0x08, 0x00, 0x00, 0x20, 0x00, 0x0c, 0x00,
    0x08, 0x00, 0x01, 0x00, 0xcc, 0x15, 0x00, 0x00, 0x04, 0x00, 0x03, 0x00,
    0x04, 0x00, 0x03, 0x00, 0x04, 0x00, 0x05, 0x00, 0x08, 0x00, 0x06, 0x00,
    0x98, 0x08, 0x00, 0x00, 0x20, 0x00, 0x0d, 0x00, 0x08, 0x00, 0x01, 0x00,
    0xe0, 0x15, 0x00, 0x00, 0x04, 0x00, 0x03, 0x00, 0x04, 0x00, 0x03, 0x00,
    0x04, 0x00, 0x05, 0x00, 0x08, 0x00, 0x06, 0x00, 0x98, 0x08, 0x00, 0x00,
    0x20, 0x00, 0x0e, 0x00, 0x08, 0x00, 0x01, 0x00, 0xf4, 0x15, 0x00, 0x00,
    0x04, 0x00, 0x03, 0x00, 0x04, 0x00, 0x03, 0x00, 0x04, 0x00, 0x05, 0x00,
    0x08, 0x00, 0x06, 0x00, 0x98, 0x08, 0x00, 0x00, 0x20, 0x00, 0x0f, 0x00,
    0x08, 0x00, 0x01, 0x00, 0x08, 0x16, 0x00, 0x00, 0x04, 0x00, 0x03, 0x00,
    0x04, 0x00, 0x03, 0x00, 0x04, 0x00, 0x05, 0x00, 0x08, 0x00, 0x06, 0x00,
    0x98, 0x08, 0x00, 0x00, 0x20, 0x00, 0x10, 0x00, 0x08, 0x00, 0x01, 0x00,
    0x1c, 0x16, 0x00, 0x00, 0x04, 0x00, 0x03, 0x00, 0x04, 0x00, 0x03, 0x00,
    0x04, 0x00, 0x05, 0x00, 0x08, 0x00, 0x06, 0x00, 0x98, 0x08, 0x00, 0x00,
    0x20, 0x00, 0x11, 0x00, 0x08, 0x00, 0x01, 0x00, 0x30, 0x16, 0x00, 0x00,
    0x04, 0x00, 0x03, 0x00, 0x04, 0x00, 0x03, 0x00, 0x04, 0x00, 0x05, 0x00,
    0x08, 0x00, 0x06, 0x00, 0x98, 0x08, 0x00, 0x00, 0x20, 0x00, 0x12, 0x00,
    0x08, 0x00, 0x01, 0x00, 0x44, 0x16, 0x00, 0x00, 0x04, 0x00, 0x03, 0x00,
    0x04, 0x00, 0x03, 0x00, 0x04, 0x00, 0x05, 0x00, 0x08, 0x00, 0x06, 0x00,
    0x98, 0x08, 0x00, 0x00, 0x20, 0x00, 0x13, 0x00, 0x08, 0x00, 0x01, 0x00,
    0x58, 0x16, 0x00, 0x00, 0x04, 0x00, 0x03, 0x00, 0x04, 0x00, 0x03, 0x00,
    0x04, 0x00, 0x05, 0x00, 0x08, 0x00, 0x06, 0x00, 0x98, 0x08, 0x00, 0x00,
    0x1c, 0x00, 0x14, 0x00, 0x08, 0x00, 0x01, 0x00, 0x71, 0x16, 0x00, 0x00,
    0x04, 0x00, 0x03, 0x00, 0x04, 0x00, 0x03, 0x00, 0x08, 0x00, 0x06, 0x00,
    0x98, 0x08, 0x00, 0x00, 0x1c, 0x00, 0x15, 0x00, 0x08, 0x00, 0x01, 0x00,
    0x85, 0x16, 0x00, 0x00, 0x04, 0x00, 0x03, 0x00, 0x04, 0x00, 0x03, 0x00,
    0x08, 0x00, 0x06, 0x00, 0x98, 0x08, 0x00, 0x00, 0x1c, 0x00, 0x16, 0x00,
    0x08, 0x00, 0x01, 0x00, 0x99, 0x16, 0x00, 0x00, 0x04, 0x00, 0x03, 0x00,
    0x04, 0x00, 0x03, 0x00, 0x08, 0x00, 0x06, 0x00, 0x98, 0x08, 0x00, 0x00,
    0x1c, 0x00, 0x17, 0x00, 0x08, 0x00, 0x01, 0x00, 0xad, 0x16, 0x00, 0x00,
    0x04, 0x00, 0x03, 0x00, 0x04, 0x00, 0x03, 0x00, 0x08, 0x00, 0x06, 0x00,
    0x98, 0x08, 0x00, 0x00, 0x1c, 0x00, 0x18, 0x00, 0x08, 0x00, 0x01, 0x00,
    0xc1, 0x16, 0x00, 0x00, 0x04, 0x00, 0x03, 0x00, 0x04, 0x00, 0x03, 0x00,
    0x08, 0x00, 0x06, 0x00, 0x98, 0x08, 0x00, 0x00, 0x64, 0x00, 0x02, 0x00,
    0x0c, 0x00, 0x00, 0x00, 0x08, 0x00, 0x01, 0x00, 0x3c, 0x00, 0x00, 0x00,
    0x0c, 0x00, 0x01, 0x00, 0x08, 0x00, 0x01, 0x00, 0x5a, 0x00, 0x00, 0x00,
    0x0c, 0x00, 0x02, 0x00, 0x08, 0x00, 0x01, 0x00, 0x78, 0x00, 0x00, 0x00,
    0x0c, 0x00, 0x03, 0x00, 0x08, 0x00, 0x01, 0x00, 0xb4, 0x00, 0x00, 0x00,
    0x0c, 0x00, 0x04, 0x00, 0x08, 0x00, 0x01, 0x00, 0xf0, 0x00, 0x00, 0x00,
    0x0c, 0x00, 0x05, 0x00, 0x08, 0x00, 0x01, 0x00, 0x68, 0x01, 0x00, 0x00,
    0x0c, 0x00, 0x06, 0x00, 0x08, 0x00, 0x01, 0x00, 0xe0, 0x01, 0x00, 0x00,
    0x0c, 0x00, 0x07, 0x00, 0x08, 0x00, 0x01, 0x00, 0x1c, 0x02, 0x00, 0x00,
    0xdc, 0x00, 0x32, 0x00, 0x08, 0x00, 0x01, 0x00, 0x07, 0x00, 0x00, 0x00,
    0x08, 0x00, 0x02, 0x00, 0x06, 0x00, 0x00, 0x00, 0x08, 0x00, 0x03, 0x00,
    0x0b, 0x00, 0x00, 0x00, 0x08, 0x00, 0x04, 0x00, 0x0f, 0x00, 0x00, 0x00,
    0x08, 0x00, 0x05, 0x00, 0x13, 0x00, 0x00, 0x00, 0x08, 0x00, 0x06, 0x00,
    0x19, 0x00, 0x00, 0x00, 0x08, 0x00, 0x07, 0x00, 0x25, 0x00, 0x00, 0x00,
    0x08, 0x00, 0x08, 0x00, 0x26, 0x00, 0x00, 0x00, 0x08, 0x00, 0x09, 0x00,
    0x27, 0x00, 0x00, 0x00, 0x08, 0x00, 0x0a, 0x00, 0x28, 0x00, 0x00, 0x00,
    0x08, 0x00, 0x0b, 0x00, 0x2b, 0x00, 0x00, 0x00, 0x08, 0x00, 0x0c, 0x00,
    0x37, 0x00, 0x00, 0x00, 0x08, 0x00, 0x0d, 0x00, 0x39, 0x00, 0x00, 0x00,
    0x08, 0x00, 0x0e, 0x00, 0x3b, 0x00, 0x00, 0x00, 0x08, 0x00, 0x0f, 0x00,
    0x43, 0x00, 0x00, 0x00, 0x08, 0x00, 0x10, 0x00, 0x31, 0x00, 0x00, 0x00,
    0x08, 0x00, 0x11, 0x00, 0x41, 0x00, 0x00, 0x00, 0x08, 0x00, 0x12, 0x00,
    0x42, 0x00, 0x00, 0x00, 0x08, 0x00, 0x13, 0x00, 0x4b, 0x00, 0x00, 0x00,
    0x08, 0x00, 0x14, 0x00, 0x54, 0x00, 0x00, 0x00, 0x08, 0x00, 0x15, 0x00,
    0x57, 0x00, 0x00, 0x00, 0x08, 0x00, 0x16, 0x00, 0x55, 0x00, 0x00, 0x00,
    0x08, 0x00, 0x17, 0x00, 0x59, 0x00, 0x00, 0x00, 0x08, 0x00, 0x18, 0x00,
    0x5c, 0x00, 0x00, 0x00, 0x08, 0x00, 0x19, 0x00, 0x2d, 0x00, 0x00, 0x00,
    0x08, 0x00, 0x1a, 0x00, 0x2e, 0x00, 0x00, 0x00, 0x08, 0x00, 0x1b, 0x00,
    0x30, 0x00, 0x00, 0x00, 0x08, 0x00, 0x6f, 0x00, 0x10, 0x27, 0x00, 0x00,
    0x04, 0x00, 0x6c, 0x00, 0x30, 0x04, 0x63, 0x00, 0x04, 0x00, 0x00, 0x00,
    0x84, 0x00, 0x01, 0x00, 0x06, 0x00, 0x65, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x06, 0x00, 0x65, 0x00, 0x10, 0x00, 0x00, 0x00, 0x06, 0x00, 0x65, 0x00,
    0x20, 0x00, 0x00, 0x00, 0x06, 0x00, 0x65, 0x00, 0x30, 0x00, 0x00, 0x00,
    0x06, 0x00, 0x65, 0x00, 0x40, 0x00, 0x00, 0x00, 0x06, 0x00, 0x65, 0x00,
    0x50, 0x00, 0x00, 0x00, 0x06, 0x00, 0x65, 0x00, 0x60, 0x00, 0x00, 0x00,
    0x06, 0x00, 0x65, 0x00, 0x70, 0x00, 0x00, 0x00, 0x06, 0x00, 0x65, 0x00,
    0x80, 0x00, 0x00, 0x00, 0x06, 0x00, 0x65, 0x00, 0x90, 0x00, 0x00, 0x00,
    0x06, 0x00, 0x65, 0x00, 0xa0, 0x00, 0x00, 0x00, 0x06, 0x00, 0x65, 0x00,
    0xb0, 0x00, 0x00, 0x00, 0x06, 0x00, 0x65, 0x00, 0xc0, 0x00, 0x00, 0x00,
    0x06, 0x00, 0x65, 0x00, 0xd0, 0x00, 0x00, 0x00, 0x06, 0x00, 0x65, 0x00,
    0xe0, 0x00, 0x00, 0x00, 0x06, 0x00, 0x65, 0x00, 0xf0, 0x00, 0x00, 0x00,
    0x84, 0x00, 0x02, 0x00, 0x06, 0x00, 0x65, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x06, 0x00, 0x65, 0x00, 0x10, 0x00, 0x00, 0x00, 0x06, 0x00, 0x65, 0x00,
    0x20, 0x00, 0x00, 0x00, 0x06, 0x00, 0x65, 0x00, 0x30, 0x00, 0x00, 0x00,
    0x06, 0x00, 0x65, 0x00, 0x40, 0x00, 0x00, 0x00, 0x06, 0x00, 0x65, 0x00,
    0x50, 0x00, 0x00, 0x00, 0x06, 0x00, 0x65, 0x00, 0x60, 0x00, 0x00, 0x00,
    0x06, 0x00, 0x65, 0x00, 0x70, 0x00, 0x00, 0x00, 0x06, 0x00, 0x65, 0x00,
    0x80, 0x00, 0x00, 0x00, 0x06, 0x00, 0x65, 0x00, 0x90, 0x00, 0x00, 0x00,
    0x06, 0x00, 0x65, 0x00, 0xa0, 0x00, 0x00, 0x00, 0x06, 0x00, 0x65, 0x00,
    0xb0, 0x00, 0x00, 0x00, 0x06, 0x00, 0x65, 0x00, 0xc0, 0x00, 0x00, 0x00,
    0x06, 0x00, 0x65, 0x00, 0xd0, 0x00, 0x00, 0x00, 0x06, 0x00, 0x65, 0x00,
    0xe0, 0x00, 0x00, 0x00, 0x06, 0x00, 0x65, 0x00, 0xf0, 0x00, 0x00, 0x00,
    0x84, 0x00, 0x03, 0x00, 0x06, 0x00, 0x65, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x06, 0x00, 0x65, 0x00, 0x10, 0x00, 0x00, 0x00, 0x06, 0x00, 0x65, 0x00,
    0x20, 0x00, 0x00, 0x00, 0x06, 0x00, 0x65, 0x00, 0x30, 0x00, 0x00, 0x00,
    0x06, 0x00, 0x65, 0x00, 0x40, 0x00, 0x00, 0x00, 0x06, 0x00, 0x65, 0x00,
    0x50, 0x00, 0x00, 0x00, 0x06, 0x00, 0x65, 0x00, 0x60, 0x00, 0x00, 0x00,
    0x06, 0x00, 0x65, 0x00, 0x70, 0x00, 0x00, 0x00, 0x06, 0x00, 0x65, 0x00,
    0x80, 0x00, 0x00, 0x00, 0x06, 0x00, 0x65, 0x00, 0x90, 0x00, 0x00, 0x00,
    0x06, 0x00, 0x65, 0x00, 0xa0, 0x00, 0x00, 0x00, 0x06, 0x00, 0x65, 0x00,
    0xb0, 0x00, 0x00, 0x00, 0x06, 0x00, 0x65, 0x00, 0xc0, 0x00, 0x00, 0x00,
    0x06, 0x00, 0x65, 0x00, 0xd0, 0x00, 0x00, 0x00, 0x06, 0x00, 0x65, 0x00,
    0xe0, 0x00, 0x00, 0x00, 0x06, 0x00, 0x65, 0x00, 0xf0, 0x00, 0x00, 0x00,
    0x84, 0x00, 0x04, 0x00, 0x06, 0x00, 0x65, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x06, 0x00, 0x65, 0x00, 0x10, 0x00, 0x00, 0x00, 0x06, 0x00, 0x65, 0x00,
    0x20, 0x00, 0x00, 0x00, 0x06, 0x00, 0x65, 0x00, 0x30, 0x00, 0x00, 0x00,
    0x06, 0x00, 0x65, 0x00, 0x40, 0x00, 0x00, 0x00, 0x06, 0x00, 0x65, 0x00,
    0x50, 0x00, 0x00, 0x00, 0x06, 0x00, 0x65, 0x00, 0x60, 0x00, 0x00, 0x00,
    0x06, 0x00, 0x65, 0x00, 0x70, 0x00, 0x00, 0x00, 0x06, 0x00, 0x65, 0x00,
    0x80, 0x00, 0x00, 0x00, 0x06, 0x00, 0x65, 0x00, 0x90, 0x00, 0x00, 0x00,
    0x06, 0x00, 0x65, 0x00, 0xa0, 0x00, 0x00, 0x00, 0x06, 0x00, 0x65, 0x00,
    0xb0, 0x00, 0x00, 0x00, 0x06, 0x00, 0x65, 0x00, 0xc0, 0x00, 0x00, 0x00,
    0x06, 0x00, 0x65, 0x00, 0xd0, 0x00, 0x00, 0x00, 0x06, 0x00, 0x65, 0x00,
    0xe0, 0x00, 0x00, 0x00, 0x06, 0x00, 0x65, 0x00, 0xf0, 0x00, 0x00, 0x00,
    0x04, 0x00, 0x05, 0x00, 0x04, 0x00, 0x06, 0x00, 0x84, 0x00, 0x07, 0x00,
    0x06, 0x00, 0x65, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x65, 0x00,
    0x10, 0x00, 0x00, 0x00, 0x06, 0x00, 0x65, 0x00, 0x20, 0x00, 0x00, 0x00,
    0x06, 0x00, 0x65, 0x00, 0x30, 0x00, 0x00, 0x00, 0x06, 0x00, 0x65, 0x00,
    0x40, 0x00, 0x00, 0x00, 0x06, 0x00, 0x65, 0x00, 0x50, 0x00, 0x00, 0x00,
    0x06, 0x00, 0x65, 0x00, 0x60, 0x00, 0x00, 0x00, 0x06, 0x00, 0x65, 0x00,
    0x70, 0x00, 0x00, 0x00, 0x06, 0x00, 0x65, 0x00, 0x80, 0x00, 0x00, 0x00,
    0x06, 0x00, 0x65, 0x00, 0x90, 0x00, 0x00, 0x00, 0x06, 0x00, 0x65, 0x00,
    0xa0, 0x00, 0x00, 0x00, 0x06, 0x00, 0x65, 0x00, 0xb0, 0x00, 0x00, 0x00,
    0x06, 0x00, 0x65, 0x00, 0xc0, 0x00, 0x00, 0x00, 0x06, 0x00, 0x65, 0x00,
    0xd0, 0x00, 0x00, 0x00, 0x06, 0x00, 0x65, 0x00, 0xe0, 0x00, 0x00, 0x00,
    0x06, 0x00, 0x65, 0x00, 0xf0, 0x00, 0x00, 0x00, 0x84, 0x00, 0x08, 0x00,
    0x06, 0x00, 0x65, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x65, 0x00,
    0x10, 0x00, 0x00, 0x00, 0x06, 0x00, 0x65, 0x00, 0x20, 0x00, 0x00, 0x00,
    0x06, 0x00, 0x65, 0x00, 0x30, 0x00, 0x00, 0x00, 0x06, 0x00, 0x65, 0x00,
    0x40, 0x00, 0x00, 0x00, 0x06, 0x00, 0x65, 0x00, 0x50, 0x00, 0x00, 0x00,
    0x06, 0x00, 0x65, 0x00, 0x60, 0x00, 0x00, 0x00, 0x06, 0x00, 0x65, 0x00,
    0x70, 0x00, 0x00, 0x00, 0x06, 0x00, 0x65, 0x00, 0x80, 0x00, 0x00, 0x00,
    0x06, 0x00, 0x65, 0x00, 0x90, 0x00, 0x00, 0x00, 0x06, 0x00, 0x65, 0x00,
    0xa0, 0x00, 0x00, 0x00, 0x06, 0x00, 0x65, 0x00, 0xb0, 0x00, 0x00, 0x00,
    0x06, 0x00, 0x65, 0x00, 0xc0, 0x00, 0x00, 0x00, 0x06, 0x00, 0x65, 0x00,
    0xd0, 0x00, 0x00, 0x00, 0x06, 0x00, 0x65, 0x00, 0xe0, 0x00, 0x00, 0x00,
    0x06, 0x00, 0x65, 0x00, 0xf0, 0x00, 0x00, 0x00, 0x84, 0x00, 0x09, 0x00,
    0x06, 0x00, 0x65, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x65, 0x00,
    0x10, 0x00, 0x00, 0x00, 0x06, 0x00, 0x65, 0x00, 0x20, 0x00, 0x00, 0x00,
    0x06, 0x00, 0x65, 0x00, 0x30, 0x00, 0x00, 0x00, 0x06, 0x00, 0x65, 0x00,
    0x40, 0x00, 0x00, 0x00, 0x06, 0x00, 0x65, 0x00, 0x50, 0x00, 0x00, 0x00,
    0x06, 0x00, 0x65, 0x00, 0x60, 0x00, 0x00, 0x00, 0x06, 0x00, 0x65, 0x00,
    0x70, 0x00, 0x00, 0x00, 0x06, 0x00, 0x65, 0x00, 0x80, 0x00, 0x00, 0x00,
    0x06, 0x00, 0x65, 0x00, 0x90, 0x00, 0x00, 0x00, 0x06, 0x00, 0x65, 0x00,
    0xa0, 0x00, 0x00, 0x00, 0x06, 0x00, 0x65, 0x00, 0xb0, 0x00, 0x00, 0x00,
    0x06, 0x00, 0x65, 0x00, 0xc0, 0x00, 0x00, 0x00, 0x06, 0x00, 0x65, 0x00,
    0xd0, 0x00, 0x00, 0x00, 0x06, 0x00, 0x65, 0x00, 0xe0, 0x00, 0x00, 0x00,
    0x06, 0x00, 0x65, 0x00, 0xf0, 0x00, 0x00, 0x00, 0x84, 0x00, 0x0a, 0x00,
    0x06, 0x00, 0x65, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x65, 0x00,
    0x10, 0x00, 0x00, 0x00, 0x06, 0x00, 0x65, 0x00, 0x20, 0x00, 0x00, 0x00,
    0x06, 0x00, 0x65, 0x00, 0x30, 0x00, 0x00, 0x00, 0x06, 0x00, 0x65, 0x00,
    0x40, 0x00, 0x00, 0x00, 0x06, 0x00, 0x65, 0x00, 0x50, 0x00, 0x00, 0x00,
    0x06, 0x00, 0x65, 0x00, 0x60, 0x00, 0x00, 0x00, 0x06, 0x00, 0x65, 0x00,
    0x70, 0x00, 0x00, 0x00, 0x06, 0x00, 0x65, 0x00, 0x80, 0x00, 0x00, 0x00,
    0x06, 0x00, 0x65, 0x00, 0x90, 0x00, 0x00, 0x00, 0x06, 0x00, 0x65, 0x00,
    0xa0, 0x00, 0x00, 0x00, 0x06, 0x00, 0x65, 0x00, 0xb0, 0x00, 0x00, 0x00,
    0x06, 0x00, 0x65, 0x00, 0xc0, 0x00, 0x00, 0x00, 0x06, 0x00, 0x65, 0x00,
    0xd0, 0x00, 0x00, 0x00, 0x06, 0x00, 0x65, 0x00, 0xe0, 0x00, 0x00, 0x00,
    0x06, 0x00, 0x65, 0x00, 0xf0, 0x00, 0x00, 0x00, 0x40, 0x01, 0x64, 0x00,
    0x04, 0x00, 0x00, 0x00, 0x24, 0x00, 0x01, 0x00, 0x06, 0x00, 0x65, 0x00,
    0x40, 0x00, 0x00, 0x00, 0x06, 0x00, 0x65, 0x00, 0xb0, 0x00, 0x00, 0x00,
    0x06, 0x00, 0x65, 0x00, 0xc0, 0x00, 0x00, 0x00, 0x06, 0x00, 0x65, 0x00,
    0xd0, 0x00, 0x00, 0x00, 0x14, 0x00, 0x02, 0x00, 0x06, 0x00, 0x65, 0x00,
    0x40, 0x00, 0x00, 0x00, 0x06, 0x00, 0x65, 0x00, 0xd0, 0x00, 0x00, 0x00,
    0x3c, 0x00, 0x03, 0x00, 0x06, 0x00, 0x65, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x06, 0x00, 0x65, 0x00, 0x20, 0x00, 0x00, 0x00, 0x06, 0x00, 0x65, 0x00,
    0x40, 0x00, 0x00, 0x00, 0x06, 0x00, 0x65, 0x00, 0xa0, 0x00, 0x00, 0x00,
    0x06, 0x00, 0x65, 0x00, 0xb0, 0x00, 0x00, 0x00, 0x06, 0x00, 0x65, 0x00,
    0xc0, 0x00, 0x00, 0x00, 0x06, 0x00, 0x65, 0x00, 0xd0, 0x00, 0x00, 0x00,
    0x3c, 0x00, 0x04, 0x00, 0x06, 0x00, 0x65, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x06, 0x00, 0x65, 0x00, 0x20, 0x00, 0x00, 0x00, 0x06, 0x00, 0x65, 0x00,
    0x40, 0x00, 0x00, 0x00, 0x06, 0x00, 0x65, 0x00, 0xa0, 0x00, 0x00, 0x00,
    0x06, 0x00, 0x65, 0x00, 0xb0, 0x00, 0x00, 0x00, 0x06, 0x00, 0x65, 0x00,
    0xc0, 0x00, 0x00, 0x00, 0x06, 0x00, 0x65, 0x00, 0xd0, 0x00, 0x00, 0x00,
    0x04, 0x00, 0x05, 0x00, 0x04, 0x00, 0x06, 0x00, 0x1c, 0x00, 0x07, 0x00,
    0x06, 0x00, 0x65, 0x00, 0xb0, 0x00, 0x00, 0x00, 0x06, 0x00, 0x65, 0x00,
    0xc0, 0x00, 0x00, 0x00, 0x06, 0x00, 0x65, 0x00, 0xd0, 0x00, 0x00, 0x00,
    0x14, 0x00, 0x08, 0x00, 0x06, 0x00, 0x65, 0x00, 0x40, 0x00, 0x00, 0x00,
    0x06, 0x00, 0x65, 0x00, 0xd0, 0x00, 0x00, 0x00, 0x3c, 0x00, 0x09, 0x00,
    0x06, 0x00, 0x65, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x65, 0x00,
    0x20, 0x00, 0x00, 0x00, 0x06, 0x00, 0x65, 0x00, 0x40, 0x00, 0x00, 0x00,
    0x06, 0x00, 0x65, 0x00, 0xa0, 0x00, 0x00, 0x00, 0x06, 0x00, 0x65, 0x00,
    0xb0, 0x00, 0x00, 0x00, 0x06, 0x00, 0x65, 0x00, 0xc0, 0x00, 0x00, 0x00,
    0x06, 0x00, 0x65, 0x00, 0xd0, 0x00, 0x00, 0x00, 0x14, 0x00, 0x0a, 0x00,
    0x06, 0x00, 0x65, 0x00, 0x40, 0x00, 0x00, 0x00, 0x06, 0x00, 0x65, 0x00,
    0xd0, 0x00, 0x00, 0x00, 0x3c, 0x00, 0x76, 0x00, 0x04, 0x00, 0x02, 0x00,
    0x04, 0x00, 0x03, 0x00, 0x04, 0x00, 0x05, 0x00, 0x04, 0x00, 0x06, 0x00,
    0x04, 0x00, 0x07, 0x00, 0x04, 0x00, 0x08, 0x00, 0x04, 0x00, 0x09, 0x00,
    0x14, 0x00, 0x04, 0x00, 0x14, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00,
    0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x12, 0x00,
    0x0b, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x79, 0x00, 0x04, 0x00, 0x04, 0x00,
    0x04, 0x00, 0x06, 0x00, 0x60, 0x00, 0x78, 0x00, 0x5c, 0x00, 0x01, 0x00,
    0x48, 0x00, 0x01, 0x00, 0x14, 0x00, 0x01, 0x00, 0x08, 0x00, 0x01, 0x00,
    0x01, 0x00, 0x00, 0x00, 0x08, 0x00, 0x02, 0x00, 0x04, 0x00, 0x02, 0x00,
    0x1c, 0x00, 0x02, 0x00, 0x08, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x10, 0x00, 0x02, 0x00, 0x04, 0x00, 0x03, 0x00, 0x04, 0x00, 0x08, 0x00,
    0x04, 0x00, 0x09, 0x00, 0x14, 0x00, 0x03, 0x00, 0x08, 0x00, 0x01, 0x00,
    0x01, 0x00, 0x00, 0x00, 0x08, 0x00, 0x02, 0x00, 0x04, 0x00, 0x0a, 0x00,
    0x08, 0x00, 0x04, 0x00, 0x01, 0x00, 0x00, 0x00, 0x08, 0x00, 0x02, 0x00,
    0x03, 0x00, 0x00, 0x00, 0x08, 0x00, 0x8f, 0x00, 0xe3, 0x1a, 0x00, 0x07,
    0x1e, 0x00, 0x94, 0x00, 0x63, 0x48, 0x1f, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0c, 0x00, 0xa9, 0x00,
    0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x0c, 0x00, 0xaa, 0x00,
    0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40};
const char kIPV4Address0[] = "192.168.10.20";
const char kIPV4Address1[] = "1.2.3.4";
const char kIPV6Address0[] = "FEDC:BA98:7654:3210:FEDC:BA98:7654:3210";
const char kIPV6Address1[] = "1080:0:0:0:8:800:200C:417A";

#if !defined(DISABLE_WAKE_ON_WIFI)

// Zero-byte pattern prefixes to match the offsetting bytes in the Ethernet
// frame that lie before the source IP address field.
const uint8_t kIPV4PatternPrefix[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                      0x00, 0x00, 0x00, 0x00, 0x00};
const uint8_t kIPV6PatternPrefix[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

// These masks have bits set to 1 to match bytes in an IP address pattern that
// represent the source IP address of the frame. They are padded with zero
// bits in front to ignore the frame offset and at the end to byte-align the
// mask itself.
const uint8_t kIPV4MaskBytes[] = {0x00, 0x00, 0x00, 0x3c};
const uint8_t kIPV6MaskBytes[] = {0x00, 0x00, 0xc0, 0xff, 0x3f};

const uint8_t kIPV4Address0Bytes[] = {0xc0, 0xa8, 0x0a, 0x14};
const uint8_t kIPV4Address1Bytes[] = {0x01, 0x02, 0x03, 0x04};

const uint8_t kIPV6Address0Bytes[] = {0xfe, 0xdc, 0xba, 0x98, 0x76, 0x54,
                                      0x32, 0x10, 0xfe, 0xdc, 0xba, 0x98,
                                      0x76, 0x54, 0x32, 0x10};
const uint8_t kIPV6Address1Bytes[] = {0x10, 0x80, 0x00, 0x00, 0x00, 0x00,
                                      0x00, 0x00, 0x00, 0x08, 0x08, 0x00,
                                      0x20, 0x0c, 0x41, 0x7a};
const char kIPV6Address2[] = "1080::8:800:200C:417A";
const uint8_t kIPV6Address2Bytes[] = {0x10, 0x80, 0x00, 0x00, 0x00, 0x00,
                                      0x00, 0x00, 0x00, 0x08, 0x08, 0x00,
                                      0x20, 0x0c, 0x41, 0x7a};
const char kIPV6Address3[] = "FF01::101";
const uint8_t kIPV6Address3Bytes[] = {0xff, 0x01, 0x00, 0x00, 0x00, 0x00,
                                      0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                      0x00, 0x00, 0x01, 0x01};
const char kIPV6Address4[] = "::1";
const uint8_t kIPV6Address4Bytes[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                      0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                      0x00, 0x00, 0x00, 0x01};
const char kIPV6Address5[] = "::";
const uint8_t kIPV6Address5Bytes[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                      0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                      0x00, 0x00, 0x00, 0x00};
const char kIPV6Address6[] = "0:0:0:0:0:FFFF:129.144.52.38";
const uint8_t kIPV6Address6Bytes[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                      0x00, 0x00, 0x00, 0x00, 0xff, 0xff,
                                      0x81, 0x90, 0x34, 0x26};
const char kIPV6Address7[] = "::DEDE:190.144.52.38";
const uint8_t kIPV6Address7Bytes[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                      0x00, 0x00, 0x00, 0x00, 0xde, 0xde,
                                      0xbe, 0x90, 0x34, 0x26};

const uint32_t kNewWiphyNlMsg_MaxPatterns = 20;
const uint32_t kNewWiphyNlMsg_MaxSSIDs = 11;
const int kNewWiphyNlMsg_PattSupportOffset = 3300;
const int kNewWiphyNlMsg_WowlanTrigNetDetectAttributeOffset = 3316;
const int kNewWiphyNlMsg_WowlanTrigDisconnectAttributeOffset = 3268;

const uint32_t kSSID1FreqMatches[] = {2412, 2437, 2462, 5180,
                                      5240, 5745, 5805, 5825};

const uint32_t kWakeReasonNlMsg_WiphyIndex = 0;
// NL80211_CMD_GET_WOWLAN message with nlmsg_type 0x16, which is different from
// kNl80211FamilyId (0x13).
const uint8_t kWrongMessageTypeNlMsg[] = {
    0x14, 0x00, 0x00, 0x00, 0x16, 0x00, 0x01, 0x00, 0x01, 0x00,
    0x00, 0x00, 0x57, 0x40, 0x00, 0x00, 0x49, 0x01, 0x00, 0x00};
// Bytes representing a NL80211_CMD_SET_WOWLAN reporting that the system woke
// up because of a reason other than wake on WiFi.
const uint8_t kWakeReasonUnsupportedNlMsg[] = {
    0x30, 0x00, 0x00, 0x00, 0x13, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x4a, 0x01, 0x00, 0x00, 0x08, 0x00, 0x01, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x99, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x03, 0x00, 0x03, 0x00, 0x00, 0x00};
// Bytes representing a NL80211_CMD_SET_WOWLAN reporting that the system woke
// up because of a disconnect.
const uint8_t kWakeReasonDisconnectNlMsg[] = {
    0x38, 0x00, 0x00, 0x00, 0x13, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x4a, 0x01, 0x00, 0x00, 0x08, 0x00, 0x01, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x99, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x03, 0x00, 0x03, 0x00, 0x00, 0x00,
    0x08, 0x00, 0x75, 0x00, 0x04, 0x00, 0x02, 0x00};
// Bytes representing a NL80211_CMD_SET_WOWLAN reporting that the system woke
// up because of a a match with packet pattern index
// kWakeReasonPatternNlMsg_PattIndex.
const uint8_t kWakeReasonPatternNlMsg[] = {
    0xac, 0x00, 0x00, 0x00, 0x13, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x4a, 0x01, 0x00, 0x00, 0x08, 0x00, 0x01, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x99, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x03, 0x00, 0x03, 0x00, 0x00, 0x00,
    0x7c, 0x00, 0x75, 0x00, 0x08, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x08, 0x00, 0x0d, 0x00, 0x62, 0x00, 0x00, 0x00, 0x66, 0x00, 0x0c, 0x00,
    0x6c, 0x29, 0x95, 0x16, 0x54, 0x68, 0x6c, 0x71, 0xd9, 0x8b, 0x3c, 0x6c,
    0x08, 0x00, 0x45, 0x00, 0x00, 0x54, 0x00, 0x00, 0x40, 0x00, 0x40, 0x01,
    0xb7, 0xdd, 0xc0, 0xa8, 0x00, 0xfe, 0xc0, 0xa8, 0x00, 0x7d, 0x08, 0x00,
    0x3f, 0x51, 0x28, 0x64, 0x00, 0x01, 0xb1, 0x0b, 0xd0, 0x54, 0x00, 0x00,
    0x00, 0x00, 0x4b, 0x16, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x11,
    0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d,
    0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29,
    0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35,
    0x36, 0x37, 0x00, 0x00};
const uint32_t kWakeReasonPatternNlMsg_PattIndex = 0;

#endif  // DISABLE_WAKE_ON_WIFI

}  // namespace

class WakeOnWiFiTest : public ::testing::Test {
 public:
  WakeOnWiFiTest() : metrics_(nullptr) {}
  virtual ~WakeOnWiFiTest() {}

  virtual void SetUp() {
    Nl80211Message::SetMessageType(kNl80211FamilyId);
    // Assume our NIC has reported its wiphy index, and that it supports wake
    // all wake triggers.
    wake_on_wifi_->wiphy_index_received_ = true;
    wake_on_wifi_->wake_on_wifi_triggers_supported_.insert(
        WakeOnWiFi::kWakeTriggerPattern);
    wake_on_wifi_->wake_on_wifi_triggers_supported_.insert(
        WakeOnWiFi::kWakeTriggerDisconnect);
    wake_on_wifi_->wake_on_wifi_triggers_supported_.insert(
        WakeOnWiFi::kWakeTriggerSSID);
    // By default our tests assume that the NIC supports more SSIDs than
    // whitelisted SSIDs.
    wake_on_wifi_->wake_on_wifi_max_ssids_ = 999;
    wake_on_wifi_->dark_resume_history_.time_ = &time_;

    ON_CALL(netlink_manager_, SendNl80211Message(_, _, _, _))
        .WillByDefault(Return(true));
  }

  void SetWakeOnWiFiMaxSSIDs(uint32_t max_ssids) {
    wake_on_wifi_->wake_on_wifi_max_ssids_ = max_ssids;
  }

  void EnableWakeOnWiFiFeaturesPacket() {
    wake_on_wifi_->wake_on_wifi_features_enabled_ =
        kWakeOnWiFiFeaturesEnabledPacket;
  }

  void EnableWakeOnWiFiFeaturesDarkConnect() {
    wake_on_wifi_->wake_on_wifi_features_enabled_ =
        kWakeOnWiFiFeaturesEnabledDarkConnect;
  }

  void EnableWakeOnWiFiFeaturesPacketDarkConnect() {
    wake_on_wifi_->wake_on_wifi_features_enabled_ =
        kWakeOnWiFiFeaturesEnabledPacketDarkConnect;
  }

  void SetWakeOnWiFiFeaturesNotSupported() {
    wake_on_wifi_->wake_on_wifi_features_enabled_ =
        kWakeOnWiFiFeaturesEnabledNotSupported;
  }

  void DisableWakeOnWiFiFeatures() {
    wake_on_wifi_->wake_on_wifi_features_enabled_ =
        kWakeOnWiFiFeaturesEnabledNone;
  }

  void AddWakeOnPacketConnection(const string& ip_endpoint, Error* error) {
    wake_on_wifi_->AddWakeOnPacketConnection(ip_endpoint, error);
  }

  void RemoveWakeOnPacketConnection(const string& ip_endpoint, Error* error) {
    wake_on_wifi_->RemoveWakeOnPacketConnection(ip_endpoint, error);
  }

  void RemoveAllWakeOnPacketConnections(Error* error) {
    wake_on_wifi_->RemoveAllWakeOnPacketConnections(error);
  }

  bool CreateIPAddressPatternAndMask(const IPAddress& ip_addr,
                                     ByteString* pattern, ByteString* mask) {
    return WakeOnWiFi::CreateIPAddressPatternAndMask(ip_addr, pattern, mask);
  }

  bool ConfigureWiphyIndex(Nl80211Message* msg, int32_t index) {
    return WakeOnWiFi::ConfigureWiphyIndex(msg, index);
  }

  bool ConfigureDisableWakeOnWiFiMessage(SetWakeOnPacketConnMessage* msg,
                                         uint32_t wiphy_index, Error* error) {
    return WakeOnWiFi::ConfigureDisableWakeOnWiFiMessage(msg, wiphy_index,
                                                         error);
  }

  bool WakeOnWiFiSettingsMatch(const Nl80211Message& msg,
                               const set<WakeOnWiFi::WakeOnWiFiTrigger>& trigs,
                               const IPAddressStore& addrs,
                               uint32_t net_detect_scan_period_seconds,
                               const vector<ByteString>& ssid_whitelist) {
    return WakeOnWiFi::WakeOnWiFiSettingsMatch(
        msg, trigs, addrs, net_detect_scan_period_seconds, ssid_whitelist);
  }

  bool ConfigureSetWakeOnWiFiSettingsMessage(
      SetWakeOnPacketConnMessage* msg,
      const set<WakeOnWiFi::WakeOnWiFiTrigger>& trigs,
      const IPAddressStore& addrs, uint32_t wiphy_index,
      uint32_t net_detect_scan_period_seconds,
      const vector<ByteString>& ssid_whitelist, Error* error) {
    return WakeOnWiFi::ConfigureSetWakeOnWiFiSettingsMessage(
        msg, trigs, addrs, wiphy_index, net_detect_scan_period_seconds,
        ssid_whitelist, error);
  }

  void RequestWakeOnPacketSettings() {
    wake_on_wifi_->RequestWakeOnPacketSettings();
  }

  void VerifyWakeOnWiFiSettings(const Nl80211Message& nl80211_message) {
    wake_on_wifi_->VerifyWakeOnWiFiSettings(nl80211_message);
  }

  size_t GetWakeOnWiFiMaxPatterns() {
    return wake_on_wifi_->wake_on_wifi_max_patterns_;
  }

  uint32_t GetWakeOnWiFiMaxSSIDs() {
    return wake_on_wifi_->wake_on_wifi_max_ssids_;
  }

  void SetWakeOnWiFiMaxPatterns(size_t max_patterns) {
    wake_on_wifi_->wake_on_wifi_max_patterns_ = max_patterns;
  }

  void ApplyWakeOnWiFiSettings() { wake_on_wifi_->ApplyWakeOnWiFiSettings(); }

  void DisableWakeOnWiFi() { wake_on_wifi_->DisableWakeOnWiFi(); }

  set<WakeOnWiFi::WakeOnWiFiTrigger>* GetWakeOnWiFiTriggers() {
    return &wake_on_wifi_->wake_on_wifi_triggers_;
  }

  set<WakeOnWiFi::WakeOnWiFiTrigger>* GetWakeOnWiFiTriggersSupported() {
    return &wake_on_wifi_->wake_on_wifi_triggers_supported_;
  }

  void ClearWakeOnWiFiTriggersSupported() {
    wake_on_wifi_->wake_on_wifi_triggers_supported_.clear();
  }

  IPAddressStore* GetWakeOnPacketConnections() {
    return &wake_on_wifi_->wake_on_packet_connections_;
  }

  void RetrySetWakeOnPacketConnections() {
    wake_on_wifi_->RetrySetWakeOnPacketConnections();
  }

  void SetSuspendActionsDoneCallback() {
    wake_on_wifi_->suspend_actions_done_callback_ =
        Bind(&WakeOnWiFiTest::DoneCallback, Unretained(this));
  }

  void ResetSuspendActionsDoneCallback() {
    wake_on_wifi_->suspend_actions_done_callback_.Reset();
  }

  bool SuspendActionsCallbackIsNull() {
    return wake_on_wifi_->suspend_actions_done_callback_.is_null();
  }

  void RunSuspendActionsCallback(const Error& error) {
    wake_on_wifi_->suspend_actions_done_callback_.Run(error);
  }

  int GetNumSetWakeOnPacketRetries() {
    return wake_on_wifi_->num_set_wake_on_packet_retries_;
  }

  void SetNumSetWakeOnPacketRetries(int retries) {
    wake_on_wifi_->num_set_wake_on_packet_retries_ = retries;
  }

  void OnBeforeSuspend(bool is_connected,
                       const vector<ByteString>& ssid_whitelist,
                       bool have_dhcp_lease,
                       uint32_t time_to_next_lease_renewal) {
    ResultCallback done_callback(
        Bind(&WakeOnWiFiTest::DoneCallback, Unretained(this)));
    Closure renew_dhcp_lease_callback(
        Bind(&WakeOnWiFiTest::RenewDHCPLeaseCallback, Unretained(this)));
    Closure remove_supplicant_networks_callback(Bind(
        &WakeOnWiFiTest::RemoveSupplicantNetworksCallback, Unretained(this)));
    wake_on_wifi_->OnBeforeSuspend(is_connected, ssid_whitelist, done_callback,
                                   renew_dhcp_lease_callback,
                                   remove_supplicant_networks_callback,
                                   have_dhcp_lease, time_to_next_lease_renewal);
  }

  void OnDarkResume(bool is_connected,
                    const vector<ByteString>& ssid_whitelist) {
    ResultCallback done_callback(
        Bind(&WakeOnWiFiTest::DoneCallback, Unretained(this)));
    Closure renew_dhcp_lease_callback(
        Bind(&WakeOnWiFiTest::RenewDHCPLeaseCallback, Unretained(this)));
    WakeOnWiFi::InitiateScanCallback initiate_scan_callback(
        Bind(&WakeOnWiFiTest::InitiateScanCallback, Unretained(this)));
    Closure remove_supplicant_networks_callback(Bind(
        &WakeOnWiFiTest::RemoveSupplicantNetworksCallback, Unretained(this)));
    wake_on_wifi_->OnDarkResume(
        is_connected, ssid_whitelist, done_callback, renew_dhcp_lease_callback,
        initiate_scan_callback, remove_supplicant_networks_callback);
  }

  void OnAfterResume() { wake_on_wifi_->OnAfterResume(); }

  void BeforeSuspendActions(bool is_connected, bool start_lease_renewal_timer,
                            uint32_t time_to_next_lease_renewal) {
    SetDarkResumeActionsTimeOutCallback();
    EXPECT_FALSE(DarkResumeActionsTimeOutCallbackIsCancelled());
    EXPECT_CALL(metrics_,
                NotifyBeforeSuspendActions(is_connected, GetInDarkResume()));
    Closure remove_supplicant_networks_callback(Bind(
        &WakeOnWiFiTest::RemoveSupplicantNetworksCallback, Unretained(this)));
    wake_on_wifi_->BeforeSuspendActions(is_connected, start_lease_renewal_timer,
                                        time_to_next_lease_renewal,
                                        remove_supplicant_networks_callback);
    EXPECT_TRUE(DarkResumeActionsTimeOutCallbackIsCancelled());
  }

  void OnConnectedAndReachable(bool start_lease_renewal_timer,
                               uint32_t time_to_next_lease_renewal) {
    wake_on_wifi_->OnConnectedAndReachable(start_lease_renewal_timer,
                                           time_to_next_lease_renewal);
  }

  void SetInDarkResume(bool val) { wake_on_wifi_->in_dark_resume_ = val; }

  bool GetInDarkResume() { return wake_on_wifi_->in_dark_resume_; }

  void SetWiphyIndexReceivedToFalse() {
    wake_on_wifi_->wiphy_index_received_ = false;
  }

  void SetWiphyIndex(uint32_t wiphy_index) {
    wake_on_wifi_->wiphy_index_ = wiphy_index;
  }

  void ParseWakeOnWiFiCapabilities(const Nl80211Message& nl80211_message) {
    wake_on_wifi_->ParseWakeOnWiFiCapabilities(nl80211_message);
  }

  bool SetWakeOnWiFiFeaturesEnabled(const std::string& enabled, Error* error) {
    return wake_on_wifi_->SetWakeOnWiFiFeaturesEnabled(enabled, error);
  }

  const string& GetWakeOnWiFiFeaturesEnabled() {
    return wake_on_wifi_->wake_on_wifi_features_enabled_;
  }

  void SetDarkResumeActionsTimeOutCallback() {
    wake_on_wifi_->dark_resume_actions_timeout_callback_.Reset(Bind(
        &WakeOnWiFiTest::DarkResumeActionsTimeoutCallback, Unretained(this)));
  }

  bool DarkResumeActionsTimeOutCallbackIsCancelled() {
    return wake_on_wifi_->dark_resume_actions_timeout_callback_.IsCancelled();
  }

  void StartDHCPLeaseRenewalTimer() {
    wake_on_wifi_->dhcp_lease_renewal_timer_.Start(
        FROM_HERE, base::TimeDelta::FromSeconds(kTimeToNextLeaseRenewalLong),
        Bind(&WakeOnWiFiTest::OnTimerWakeDoNothing, Unretained(this)));
  }

  void StartWakeToScanTimer() {
    wake_on_wifi_->wake_to_scan_timer_.Start(
        FROM_HERE, base::TimeDelta::FromSeconds(kTimeToNextLeaseRenewalLong),
        Bind(&WakeOnWiFiTest::OnTimerWakeDoNothing, Unretained(this)));
  }

  void StopDHCPLeaseRenewalTimer() {
    wake_on_wifi_->dhcp_lease_renewal_timer_.Stop();
  }

  void StopWakeToScanTimer() { wake_on_wifi_->wake_to_scan_timer_.Stop(); }

  bool DHCPLeaseRenewalTimerIsRunning() {
    return wake_on_wifi_->dhcp_lease_renewal_timer_.IsRunning();
  }

  bool WakeToScanTimerIsRunning() {
    return wake_on_wifi_->wake_to_scan_timer_.IsRunning();
  }

  void SetDarkResumeActionsTimeoutMilliseconds(int64_t timeout) {
    wake_on_wifi_->DarkResumeActionsTimeoutMilliseconds = timeout;
  }

  void InitStateForDarkResume() {
    SetInDarkResume(true);
    GetWakeOnPacketConnections()->AddUnique(IPAddress("1.1.1.1"));
    EnableWakeOnWiFiFeaturesPacketDarkConnect();
    SetDarkResumeActionsTimeoutMilliseconds(0);
  }

  void SetExpectationsDisconnectedBeforeSuspend() {
    EXPECT_TRUE(GetWakeOnWiFiTriggers()->empty());
    EXPECT_CALL(*this, DoneCallback(_)).Times(0);
    EXPECT_CALL(*this, RemoveSupplicantNetworksCallback()).Times(1);
    EXPECT_CALL(netlink_manager_,
                SendNl80211Message(
                    IsNl80211Command(kNl80211FamilyId,
                                     SetWakeOnPacketConnMessage::kCommand),
                    _, _, _));
  }

  void SetExpectationsConnectedBeforeSuspend() {
    EXPECT_TRUE(GetWakeOnWiFiTriggers()->empty());
    EXPECT_CALL(*this, DoneCallback(_)).Times(0);
    EXPECT_CALL(netlink_manager_,
                SendNl80211Message(
                    IsNl80211Command(kNl80211FamilyId,
                                     SetWakeOnPacketConnMessage::kCommand),
                    _, _, _));
  }

  void VerifyStateConnectedBeforeSuspend() {
    EXPECT_TRUE(DarkResumeActionsTimeOutCallbackIsCancelled());
    EXPECT_FALSE(GetInDarkResume());
    EXPECT_EQ(GetWakeOnWiFiTriggers()->size(), 2);
    EXPECT_TRUE(
        GetWakeOnWiFiTriggers()->find(WakeOnWiFi::kWakeTriggerPattern) !=
        GetWakeOnWiFiTriggers()->end());
    EXPECT_TRUE(
        GetWakeOnWiFiTriggers()->find(WakeOnWiFi::kWakeTriggerDisconnect) !=
        GetWakeOnWiFiTriggers()->end());
  }

  void VerifyStateDisconnectedBeforeSuspend() {
    EXPECT_TRUE(DarkResumeActionsTimeOutCallbackIsCancelled());
    EXPECT_FALSE(GetInDarkResume());
    EXPECT_EQ(GetWakeOnWiFiTriggers()->size(), 1);
    EXPECT_FALSE(
        GetWakeOnWiFiTriggers()->find(WakeOnWiFi::kWakeTriggerPattern) !=
        GetWakeOnWiFiTriggers()->end());
    EXPECT_TRUE(GetWakeOnWiFiTriggers()->find(WakeOnWiFi::kWakeTriggerSSID) !=
                GetWakeOnWiFiTriggers()->end());
  }

  void ReportConnectedToServiceAfterWake(bool is_connected) {
    wake_on_wifi_->ReportConnectedToServiceAfterWake(is_connected);
  }

  void OnNoAutoConnectableServicesAfterScan(
      const vector<ByteString>& ssid_whitelist) {
    Closure remove_supplicant_networks_callback(Bind(
        &WakeOnWiFiTest::RemoveSupplicantNetworksCallback, Unretained(this)));
    WakeOnWiFi::InitiateScanCallback initiate_scan_callback(
        Bind(&WakeOnWiFiTest::InitiateScanCallback, Unretained(this)));
    wake_on_wifi_->OnNoAutoConnectableServicesAfterScan(
        ssid_whitelist, remove_supplicant_networks_callback,
        initiate_scan_callback);
  }

  EventHistory* GetDarkResumeHistory() {
    return &wake_on_wifi_->dark_resume_history_;
  }

  void SetNetDetectScanPeriodSeconds(uint32_t period) {
    wake_on_wifi_->net_detect_scan_period_seconds_ = period;
  }

  void AddSSIDToWhitelist(const uint8_t* ssid, int num_bytes,
                          vector<ByteString>* whitelist) {
    vector<uint8_t> ssid_vector(ssid, ssid + num_bytes);
    whitelist->push_back(ByteString(ssid_vector));
  }

  vector<ByteString>* GetWakeOnSSIDWhitelist() {
    return &wake_on_wifi_->wake_on_ssid_whitelist_;
  }

  void OnWakeupReasonReceived(const NetlinkMessage& netlink_message) {
    wake_on_wifi_->OnWakeupReasonReceived(netlink_message);
  }

  WiFi::FreqSet ParseWakeOnSSIDResults(AttributeListConstRefPtr results_list) {
    return wake_on_wifi_->ParseWakeOnSSIDResults(results_list);
  }

  NetlinkMessage::MessageContext GetWakeupReportMsgContext() {
    NetlinkMessage::MessageContext context;
    context.nl80211_cmd = NL80211_CMD_SET_WOWLAN;
    context.is_broadcast = true;
    return context;
  }

  void SetLastWakeReason(WakeOnWiFi::WakeOnWiFiTrigger reason) {
    wake_on_wifi_->last_wake_reason_ = reason;
  }

  WakeOnWiFi::WakeOnWiFiTrigger GetLastWakeReason() {
    return wake_on_wifi_->last_wake_reason_;
  }

  void OnScanStarted(bool is_active_scan) {
    wake_on_wifi_->OnScanStarted(is_active_scan);
  }

  const WiFi::FreqSet& GetLastSSIDMatchFreqs() {
    return wake_on_wifi_->last_ssid_match_freqs_;
  }

  void AddResultToLastSSIDResults() {
    wake_on_wifi_->last_ssid_match_freqs_.insert(1);
  }

  void InitiateScanInDarkResume(const WiFi::FreqSet& freqs) {
    wake_on_wifi_->InitiateScanInDarkResume(
        Bind(&WakeOnWiFiTest::InitiateScanCallback, Unretained(this)), freqs);
  }

  int GetDarkResumeScanRetriesLeft() {
    return wake_on_wifi_->dark_resume_scan_retries_left_;
  }

  void SetDarkResumeScanRetriesLeft(int retries) {
    wake_on_wifi_->dark_resume_scan_retries_left_ = retries;
  }

  Timestamp GetTimestampBootTime(int boottime_seconds) {
    struct timeval monotonic = {.tv_sec = 0, .tv_usec = 0};
    struct timeval boottime = {.tv_sec = boottime_seconds, .tv_usec = 0};
    return Timestamp(monotonic, boottime, "");
  }

  MOCK_METHOD1(DoneCallback, void(const Error &));
  MOCK_METHOD0(RenewDHCPLeaseCallback, void(void));
  MOCK_METHOD1(InitiateScanCallback, void(const WiFi::FreqSet&));
  MOCK_METHOD0(RemoveSupplicantNetworksCallback, void(void));
  MOCK_METHOD0(DarkResumeActionsTimeoutCallback, void(void));
  MOCK_METHOD0(OnTimerWakeDoNothing, void(void));
  MOCK_METHOD1(RecordDarkResumeWakeReasonCallback,
               void(const string& wake_reason));

 protected:
  NiceMockControl control_interface_;
  MockMetrics metrics_;
  MockNetlinkManager netlink_manager_;
  MockTime time_;
  std::unique_ptr<WakeOnWiFi> wake_on_wifi_;
};

class WakeOnWiFiTestWithDispatcher : public WakeOnWiFiTest {
 public:
  WakeOnWiFiTestWithDispatcher() : WakeOnWiFiTest() {
    wake_on_wifi_.reset(
        new WakeOnWiFi(&netlink_manager_, &dispatcher_, &metrics_,
                       Bind(&WakeOnWiFiTest::RecordDarkResumeWakeReasonCallback,
                            Unretained(this))));
  }
  virtual ~WakeOnWiFiTestWithDispatcher() {}

 protected:
  EventDispatcherForTest dispatcher_;
};

class WakeOnWiFiTestWithMockDispatcher : public WakeOnWiFiTest {
 public:
  WakeOnWiFiTestWithMockDispatcher() : WakeOnWiFiTest() {
    wake_on_wifi_.reset(
        new WakeOnWiFi(&netlink_manager_, &mock_dispatcher_, &metrics_,
                       Bind(&WakeOnWiFiTest::RecordDarkResumeWakeReasonCallback,
                            Unretained(this))));
  }
  virtual ~WakeOnWiFiTestWithMockDispatcher() {}

 protected:
  // TODO(zqiu): message loop is needed by AlarmTimer, should restructure the
  // code so that it can be mocked out.
  base::MessageLoopForIO message_loop_;
  MockEventDispatcher mock_dispatcher_;
};

ByteString CreatePattern(const unsigned char* prefix, size_t prefix_len,
                         const unsigned char* addr, size_t addr_len) {
  ByteString result(prefix, prefix_len);
  result.Append(ByteString(addr, addr_len));
  return result;
}

#if !defined(DISABLE_WAKE_ON_WIFI)

TEST_F(WakeOnWiFiTestWithMockDispatcher, CreateIPAddressPatternAndMask) {
  ByteString pattern;
  ByteString mask;
  ByteString expected_pattern;

  CreateIPAddressPatternAndMask(IPAddress(kIPV4Address0), &pattern, &mask);
  expected_pattern =
      CreatePattern(kIPV4PatternPrefix, sizeof(kIPV4PatternPrefix),
                    kIPV4Address0Bytes, sizeof(kIPV4Address0Bytes));
  EXPECT_TRUE(pattern.Equals(expected_pattern));
  EXPECT_TRUE(mask.Equals(ByteString(kIPV4MaskBytes, sizeof(kIPV4MaskBytes))));

  pattern.Clear();
  expected_pattern.Clear();
  mask.Clear();
  CreateIPAddressPatternAndMask(IPAddress(kIPV4Address1), &pattern, &mask);
  expected_pattern =
      CreatePattern(kIPV4PatternPrefix, sizeof(kIPV4PatternPrefix),
                    kIPV4Address1Bytes, sizeof(kIPV4Address1Bytes));
  EXPECT_TRUE(pattern.Equals(expected_pattern));
  EXPECT_TRUE(mask.Equals(ByteString(kIPV4MaskBytes, sizeof(kIPV4MaskBytes))));

  pattern.Clear();
  expected_pattern.Clear();
  mask.Clear();
  CreateIPAddressPatternAndMask(IPAddress(kIPV6Address0), &pattern, &mask);
  expected_pattern =
      CreatePattern(kIPV6PatternPrefix, sizeof(kIPV6PatternPrefix),
                    kIPV6Address0Bytes, sizeof(kIPV6Address0Bytes));
  EXPECT_TRUE(pattern.Equals(expected_pattern));
  EXPECT_TRUE(mask.Equals(ByteString(kIPV6MaskBytes, sizeof(kIPV6MaskBytes))));

  pattern.Clear();
  expected_pattern.Clear();
  mask.Clear();
  CreateIPAddressPatternAndMask(IPAddress(kIPV6Address1), &pattern, &mask);
  expected_pattern =
      CreatePattern(kIPV6PatternPrefix, sizeof(kIPV6PatternPrefix),
                    kIPV6Address1Bytes, sizeof(kIPV6Address1Bytes));
  EXPECT_TRUE(pattern.Equals(expected_pattern));
  EXPECT_TRUE(mask.Equals(ByteString(kIPV6MaskBytes, sizeof(kIPV6MaskBytes))));

  pattern.Clear();
  expected_pattern.Clear();
  mask.Clear();
  CreateIPAddressPatternAndMask(IPAddress(kIPV6Address2), &pattern, &mask);
  expected_pattern =
      CreatePattern(kIPV6PatternPrefix, sizeof(kIPV6PatternPrefix),
                    kIPV6Address2Bytes, sizeof(kIPV6Address2Bytes));
  EXPECT_TRUE(pattern.Equals(expected_pattern));
  EXPECT_TRUE(mask.Equals(ByteString(kIPV6MaskBytes, sizeof(kIPV6MaskBytes))));

  pattern.Clear();
  expected_pattern.Clear();
  mask.Clear();
  CreateIPAddressPatternAndMask(IPAddress(kIPV6Address3), &pattern, &mask);
  expected_pattern =
      CreatePattern(kIPV6PatternPrefix, sizeof(kIPV6PatternPrefix),
                    kIPV6Address3Bytes, sizeof(kIPV6Address3Bytes));
  EXPECT_TRUE(pattern.Equals(expected_pattern));
  EXPECT_TRUE(mask.Equals(ByteString(kIPV6MaskBytes, sizeof(kIPV6MaskBytes))));

  pattern.Clear();
  expected_pattern.Clear();
  mask.Clear();
  CreateIPAddressPatternAndMask(IPAddress(kIPV6Address4), &pattern, &mask);
  expected_pattern =
      CreatePattern(kIPV6PatternPrefix, sizeof(kIPV6PatternPrefix),
                    kIPV6Address4Bytes, sizeof(kIPV6Address4Bytes));
  EXPECT_TRUE(pattern.Equals(expected_pattern));
  EXPECT_TRUE(mask.Equals(ByteString(kIPV6MaskBytes, sizeof(kIPV6MaskBytes))));

  pattern.Clear();
  expected_pattern.Clear();
  mask.Clear();
  CreateIPAddressPatternAndMask(IPAddress(kIPV6Address5), &pattern, &mask);
  expected_pattern =
      CreatePattern(kIPV6PatternPrefix, sizeof(kIPV6PatternPrefix),
                    kIPV6Address5Bytes, sizeof(kIPV6Address5Bytes));
  EXPECT_TRUE(pattern.Equals(expected_pattern));
  EXPECT_TRUE(mask.Equals(ByteString(kIPV6MaskBytes, sizeof(kIPV6MaskBytes))));

  pattern.Clear();
  expected_pattern.Clear();
  mask.Clear();
  CreateIPAddressPatternAndMask(IPAddress(kIPV6Address6), &pattern, &mask);
  expected_pattern =
      CreatePattern(kIPV6PatternPrefix, sizeof(kIPV6PatternPrefix),
                    kIPV6Address6Bytes, sizeof(kIPV6Address6Bytes));
  EXPECT_TRUE(pattern.Equals(expected_pattern));
  EXPECT_TRUE(mask.Equals(ByteString(kIPV6MaskBytes, sizeof(kIPV6MaskBytes))));

  pattern.Clear();
  expected_pattern.Clear();
  mask.Clear();
  CreateIPAddressPatternAndMask(IPAddress(kIPV6Address7), &pattern, &mask);
  expected_pattern =
      CreatePattern(kIPV6PatternPrefix, sizeof(kIPV6PatternPrefix),
                    kIPV6Address7Bytes, sizeof(kIPV6Address7Bytes));
  EXPECT_TRUE(pattern.Equals(expected_pattern));
  EXPECT_TRUE(mask.Equals(ByteString(kIPV6MaskBytes, sizeof(kIPV6MaskBytes))));
}

TEST_F(WakeOnWiFiTestWithMockDispatcher, ConfigureWiphyIndex) {
  SetWakeOnPacketConnMessage msg;
  uint32_t value;
  EXPECT_FALSE(
      msg.attributes()->GetU32AttributeValue(NL80211_ATTR_WIPHY, &value));

  ConfigureWiphyIndex(&msg, 137);
  EXPECT_TRUE(
      msg.attributes()->GetU32AttributeValue(NL80211_ATTR_WIPHY, &value));
  EXPECT_EQ(value, 137);
}

TEST_F(WakeOnWiFiTestWithMockDispatcher, ConfigureDisableWakeOnWiFiMessage) {
  SetWakeOnPacketConnMessage msg;
  Error e;
  uint32_t value;
  EXPECT_FALSE(
      msg.attributes()->GetU32AttributeValue(NL80211_ATTR_WIPHY, &value));

  ConfigureDisableWakeOnWiFiMessage(&msg, 57, &e);
  EXPECT_EQ(e.type(), Error::Type::kSuccess);
  EXPECT_TRUE(
      msg.attributes()->GetU32AttributeValue(NL80211_ATTR_WIPHY, &value));
  EXPECT_EQ(value, 57);
}

TEST_F(WakeOnWiFiTestWithMockDispatcher, WakeOnWiFiSettingsMatch) {
  IPAddressStore all_addresses;
  set<WakeOnWiFi::WakeOnWiFiTrigger> trigs;
  vector<ByteString> whitelist;
  const uint32_t interval = kNetDetectScanIntervalSeconds;

  GetWakeOnPacketConnMessage msg0;
  NetlinkPacket packet0(kResponseNoIPAddresses, sizeof(kResponseNoIPAddresses));
  msg0.InitFromPacket(&packet0, NetlinkMessage::MessageContext());
  EXPECT_TRUE(
      WakeOnWiFiSettingsMatch(msg0, trigs, all_addresses, interval, whitelist));

  trigs.insert(WakeOnWiFi::kWakeTriggerPattern);
  all_addresses.AddUnique(
      IPAddress(string(kIPV4Address0, sizeof(kIPV4Address0))));
  GetWakeOnPacketConnMessage msg1;
  NetlinkPacket packet1(kResponseIPV40, sizeof(kResponseIPV40));
  msg1.InitFromPacket(&packet1, NetlinkMessage::MessageContext());
  EXPECT_TRUE(
      WakeOnWiFiSettingsMatch(msg1, trigs, all_addresses, interval, whitelist));
  EXPECT_FALSE(
      WakeOnWiFiSettingsMatch(msg0, trigs, all_addresses, interval, whitelist));

  // Test matching of wake on disconnect trigger.
  trigs.insert(WakeOnWiFi::kWakeTriggerDisconnect);
  GetWakeOnPacketConnMessage msg2;
  NetlinkPacket packet2(
      kResponseIPV40WakeOnDisconnect, sizeof(kResponseIPV40WakeOnDisconnect));
  msg2.InitFromPacket(&packet2, NetlinkMessage::MessageContext());
  EXPECT_TRUE(
      WakeOnWiFiSettingsMatch(msg2, trigs, all_addresses, interval, whitelist));
  EXPECT_FALSE(
      WakeOnWiFiSettingsMatch(msg1, trigs, all_addresses, interval, whitelist));
  EXPECT_FALSE(
      WakeOnWiFiSettingsMatch(msg0, trigs, all_addresses, interval, whitelist));

  trigs.erase(WakeOnWiFi::kWakeTriggerDisconnect);
  all_addresses.AddUnique(
      IPAddress(string(kIPV4Address1, sizeof(kIPV4Address1))));
  GetWakeOnPacketConnMessage msg3;
  NetlinkPacket packet3(kResponseIPV401, sizeof(kResponseIPV401));
  msg3.InitFromPacket(&packet3, NetlinkMessage::MessageContext());
  EXPECT_TRUE(
      WakeOnWiFiSettingsMatch(msg3, trigs, all_addresses, interval, whitelist));
  EXPECT_FALSE(
      WakeOnWiFiSettingsMatch(msg2, trigs, all_addresses, interval, whitelist));
  EXPECT_FALSE(
      WakeOnWiFiSettingsMatch(msg1, trigs, all_addresses, interval, whitelist));
  EXPECT_FALSE(
      WakeOnWiFiSettingsMatch(msg0, trigs, all_addresses, interval, whitelist));

  all_addresses.AddUnique(
      IPAddress(string(kIPV6Address0, sizeof(kIPV6Address0))));
  GetWakeOnPacketConnMessage msg4;
  NetlinkPacket packet4(kResponseIPV401IPV60, sizeof(kResponseIPV401IPV60));
  msg4.InitFromPacket(&packet4, NetlinkMessage::MessageContext());
  EXPECT_TRUE(
      WakeOnWiFiSettingsMatch(msg4, trigs, all_addresses, interval, whitelist));
  EXPECT_FALSE(
      WakeOnWiFiSettingsMatch(msg3, trigs, all_addresses, interval, whitelist));
  EXPECT_FALSE(
      WakeOnWiFiSettingsMatch(msg2, trigs, all_addresses, interval, whitelist));
  EXPECT_FALSE(
      WakeOnWiFiSettingsMatch(msg1, trigs, all_addresses, interval, whitelist));
  EXPECT_FALSE(
      WakeOnWiFiSettingsMatch(msg0, trigs, all_addresses, interval, whitelist));

  all_addresses.AddUnique(
      IPAddress(string(kIPV6Address1, sizeof(kIPV6Address1))));
  GetWakeOnPacketConnMessage msg5;
  NetlinkPacket packet5(kResponseIPV401IPV601, sizeof(kResponseIPV401IPV601));
  msg5.InitFromPacket(&packet5, NetlinkMessage::MessageContext());
  EXPECT_TRUE(
      WakeOnWiFiSettingsMatch(msg5, trigs, all_addresses, interval, whitelist));
  EXPECT_FALSE(
      WakeOnWiFiSettingsMatch(msg4, trigs, all_addresses, interval, whitelist));
  EXPECT_FALSE(
      WakeOnWiFiSettingsMatch(msg3, trigs, all_addresses, interval, whitelist));
  EXPECT_FALSE(
      WakeOnWiFiSettingsMatch(msg2, trigs, all_addresses, interval, whitelist));
  EXPECT_FALSE(
      WakeOnWiFiSettingsMatch(msg1, trigs, all_addresses, interval, whitelist));
  EXPECT_FALSE(
      WakeOnWiFiSettingsMatch(msg0, trigs, all_addresses, interval, whitelist));

  // Test matching of wake on SSID trigger.
  all_addresses.Clear();
  trigs.clear();
  trigs.insert(WakeOnWiFi::kWakeTriggerSSID);
  AddSSIDToWhitelist(kSSIDBytes1, sizeof(kSSIDBytes1), &whitelist);
  AddSSIDToWhitelist(kSSIDBytes2, sizeof(kSSIDBytes2), &whitelist);
  GetWakeOnPacketConnMessage msg6;
  NetlinkPacket packet6(kResponseWakeOnSSID, sizeof(kResponseWakeOnSSID));
  msg6.InitFromPacket(&packet6, NetlinkMessage::MessageContext());
  EXPECT_TRUE(
      WakeOnWiFiSettingsMatch(msg6, trigs, all_addresses, interval, whitelist));
  EXPECT_FALSE(
      WakeOnWiFiSettingsMatch(msg5, trigs, all_addresses, interval, whitelist));
  EXPECT_FALSE(
      WakeOnWiFiSettingsMatch(msg4, trigs, all_addresses, interval, whitelist));
  EXPECT_FALSE(
      WakeOnWiFiSettingsMatch(msg3, trigs, all_addresses, interval, whitelist));
  EXPECT_FALSE(
      WakeOnWiFiSettingsMatch(msg2, trigs, all_addresses, interval, whitelist));
  EXPECT_FALSE(
      WakeOnWiFiSettingsMatch(msg1, trigs, all_addresses, interval, whitelist));
  EXPECT_FALSE(
      WakeOnWiFiSettingsMatch(msg0, trigs, all_addresses, interval, whitelist));

  // Test that we get a mismatch if triggers are present in the message that we
  // don't expect.
  trigs.clear();
  EXPECT_FALSE(
      WakeOnWiFiSettingsMatch(msg6, trigs, all_addresses, interval, whitelist));
  EXPECT_FALSE(
      WakeOnWiFiSettingsMatch(msg5, trigs, all_addresses, interval, whitelist));
  EXPECT_FALSE(
      WakeOnWiFiSettingsMatch(msg4, trigs, all_addresses, interval, whitelist));
  EXPECT_FALSE(
      WakeOnWiFiSettingsMatch(msg3, trigs, all_addresses, interval, whitelist));
  EXPECT_FALSE(
      WakeOnWiFiSettingsMatch(msg2, trigs, all_addresses, interval, whitelist));
  EXPECT_FALSE(
      WakeOnWiFiSettingsMatch(msg1, trigs, all_addresses, interval, whitelist));
}

TEST_F(WakeOnWiFiTestWithMockDispatcher,
       ConfigureSetWakeOnWiFiSettingsMessage) {
  IPAddressStore all_addresses;
  set<WakeOnWiFi::WakeOnWiFiTrigger> trigs;
  const int index = 1;  // wiphy device number
  vector<ByteString> whitelist;
  const uint32_t interval = kNetDetectScanIntervalSeconds;
  SetWakeOnPacketConnMessage msg0;
  Error e;
  trigs.insert(WakeOnWiFi::kWakeTriggerPattern);
  all_addresses.AddUnique(
      IPAddress(string(kIPV4Address0, sizeof(kIPV4Address0))));
  EXPECT_TRUE(ConfigureSetWakeOnWiFiSettingsMessage(
      &msg0, trigs, all_addresses, index, interval, whitelist, &e));
  EXPECT_TRUE(
      WakeOnWiFiSettingsMatch(msg0, trigs, all_addresses, interval, whitelist));

  SetWakeOnPacketConnMessage msg1;
  all_addresses.AddUnique(
      IPAddress(string(kIPV4Address1, sizeof(kIPV4Address1))));
  EXPECT_TRUE(ConfigureSetWakeOnWiFiSettingsMessage(
      &msg1, trigs, all_addresses, index, interval, whitelist, &e));
  EXPECT_TRUE(
      WakeOnWiFiSettingsMatch(msg1, trigs, all_addresses, interval, whitelist));

  SetWakeOnPacketConnMessage msg2;
  all_addresses.AddUnique(
      IPAddress(string(kIPV6Address0, sizeof(kIPV6Address0))));
  EXPECT_TRUE(ConfigureSetWakeOnWiFiSettingsMessage(
      &msg2, trigs, all_addresses, index, interval, whitelist, &e));
  EXPECT_TRUE(
      WakeOnWiFiSettingsMatch(msg2, trigs, all_addresses, interval, whitelist));

  SetWakeOnPacketConnMessage msg3;
  all_addresses.AddUnique(
      IPAddress(string(kIPV6Address1, sizeof(kIPV6Address1))));
  EXPECT_TRUE(ConfigureSetWakeOnWiFiSettingsMessage(
      &msg3, trigs, all_addresses, index, interval, whitelist, &e));
  EXPECT_TRUE(
      WakeOnWiFiSettingsMatch(msg3, trigs, all_addresses, interval, whitelist));

  SetWakeOnPacketConnMessage msg4;
  all_addresses.AddUnique(
      IPAddress(string(kIPV6Address2, sizeof(kIPV6Address2))));
  EXPECT_TRUE(ConfigureSetWakeOnWiFiSettingsMessage(
      &msg4, trigs, all_addresses, index, interval, whitelist, &e));
  EXPECT_TRUE(
      WakeOnWiFiSettingsMatch(msg4, trigs, all_addresses, interval, whitelist));

  SetWakeOnPacketConnMessage msg5;
  all_addresses.AddUnique(
      IPAddress(string(kIPV6Address3, sizeof(kIPV6Address3))));
  EXPECT_TRUE(ConfigureSetWakeOnWiFiSettingsMessage(
      &msg5, trigs, all_addresses, index, interval, whitelist, &e));
  EXPECT_TRUE(
      WakeOnWiFiSettingsMatch(msg5, trigs, all_addresses, interval, whitelist));

  SetWakeOnPacketConnMessage msg6;
  all_addresses.AddUnique(
      IPAddress(string(kIPV6Address4, sizeof(kIPV6Address4))));
  EXPECT_TRUE(ConfigureSetWakeOnWiFiSettingsMessage(
      &msg6, trigs, all_addresses, index, interval, whitelist, &e));
  EXPECT_TRUE(
      WakeOnWiFiSettingsMatch(msg6, trigs, all_addresses, interval, whitelist));

  SetWakeOnPacketConnMessage msg7;
  all_addresses.AddUnique(
      IPAddress(string(kIPV6Address5, sizeof(kIPV6Address5))));
  EXPECT_TRUE(ConfigureSetWakeOnWiFiSettingsMessage(
      &msg7, trigs, all_addresses, index, interval, whitelist, &e));
  EXPECT_TRUE(
      WakeOnWiFiSettingsMatch(msg7, trigs, all_addresses, interval, whitelist));

  SetWakeOnPacketConnMessage msg8;
  all_addresses.AddUnique(
      IPAddress(string(kIPV6Address6, sizeof(kIPV6Address6))));
  EXPECT_TRUE(ConfigureSetWakeOnWiFiSettingsMessage(
      &msg8, trigs, all_addresses, index, interval, whitelist, &e));
  EXPECT_TRUE(
      WakeOnWiFiSettingsMatch(msg8, trigs, all_addresses, interval, whitelist));

  SetWakeOnPacketConnMessage msg9;
  all_addresses.AddUnique(
      IPAddress(string(kIPV6Address7, sizeof(kIPV6Address7))));
  EXPECT_TRUE(ConfigureSetWakeOnWiFiSettingsMessage(
      &msg9, trigs, all_addresses, index, interval, whitelist, &e));
  EXPECT_TRUE(
      WakeOnWiFiSettingsMatch(msg9, trigs, all_addresses, interval, whitelist));

  SetWakeOnPacketConnMessage msg10;
  all_addresses.Clear();
  trigs.clear();
  trigs.insert(WakeOnWiFi::kWakeTriggerSSID);
  AddSSIDToWhitelist(kSSIDBytes1, sizeof(kSSIDBytes1), &whitelist);
  AddSSIDToWhitelist(kSSIDBytes2, sizeof(kSSIDBytes2), &whitelist);
  EXPECT_TRUE(ConfigureSetWakeOnWiFiSettingsMessage(
      &msg10, trigs, all_addresses, index, interval, whitelist, &e));
  EXPECT_TRUE(WakeOnWiFiSettingsMatch(msg10, trigs, all_addresses, interval,
                                      whitelist));
}

TEST_F(WakeOnWiFiTestWithMockDispatcher, RequestWakeOnPacketSettings) {
  EXPECT_CALL(
      netlink_manager_,
      SendNl80211Message(IsNl80211Command(kNl80211FamilyId,
                                          GetWakeOnPacketConnMessage::kCommand),
                         _, _, _)).Times(1);
  RequestWakeOnPacketSettings();
}

TEST_F(WakeOnWiFiTestWithMockDispatcher,
       VerifyWakeOnWiFiSettings_NoWakeOnPacketRules) {
  ScopedMockLog log;
  // Create an Nl80211 response to a NL80211_CMD_GET_WOWLAN request
  // indicating that there are no wake-on-packet rules programmed into the NIC.
  GetWakeOnPacketConnMessage msg;
  NetlinkPacket packet(kResponseNoIPAddresses, sizeof(kResponseNoIPAddresses));
  msg.InitFromPacket(&packet, NetlinkMessage::MessageContext());
  // Successful verification and consequent invocation of callback.
  SetSuspendActionsDoneCallback();
  ScopeLogger::GetInstance()->EnableScopesByName("wifi");
  ScopeLogger::GetInstance()->set_verbose_level(2);
  EXPECT_TRUE(GetWakeOnPacketConnections()->Empty());
  EXPECT_FALSE(SuspendActionsCallbackIsNull());
  EXPECT_CALL(*this, DoneCallback(ErrorTypeIs(Error::kSuccess))).Times(1);
  EXPECT_CALL(log, Log(_, _, _)).Times(AnyNumber());
  EXPECT_CALL(
      log, Log(_, _, HasSubstr("Wake on WiFi settings successfully verified")));
  EXPECT_CALL(metrics_, NotifyVerifyWakeOnWiFiSettingsResult(
                            Metrics::kVerifyWakeOnWiFiSettingsResultSuccess));
  VerifyWakeOnWiFiSettings(msg);
  // Suspend action callback cleared after being invoked.
  EXPECT_TRUE(SuspendActionsCallbackIsNull());
  ScopeLogger::GetInstance()->EnableScopesByName("-wifi");
  ScopeLogger::GetInstance()->set_verbose_level(0);

  // Unsuccessful verification if locally stored settings do not match.
  GetWakeOnPacketConnections()->AddUnique(IPAddress("1.1.1.1"));
  GetWakeOnWiFiTriggers()->insert(WakeOnWiFi::kWakeTriggerPattern);
  EXPECT_CALL(log,
              Log(logging::LOG_ERROR, _,
                  HasSubstr(
                      " failed: discrepancy between wake-on-packet settings on "
                      "NIC and those in local data structure detected")));
  EXPECT_CALL(metrics_, NotifyVerifyWakeOnWiFiSettingsResult(
                            Metrics::kVerifyWakeOnWiFiSettingsResultFailure));
  VerifyWakeOnWiFiSettings(msg);
}

TEST_F(WakeOnWiFiTestWithMockDispatcher,
       VerifyWakeOnWiFiSettings_WakeOnPatternAndDisconnectRules) {
  ScopedMockLog log;
  // Create a non-trivial Nl80211 response to a NL80211_CMD_GET_WOWLAN request
  // indicating that that the NIC wakes on packets from 192.168.10.20 and on
  // disconnects.
  GetWakeOnPacketConnMessage msg;
  NetlinkPacket packet(
      kResponseIPV40WakeOnDisconnect, sizeof(kResponseIPV40WakeOnDisconnect));
  msg.InitFromPacket(&packet, NetlinkMessage::MessageContext());
  // Successful verification and consequent invocation of callback.
  SetSuspendActionsDoneCallback();
  EXPECT_FALSE(SuspendActionsCallbackIsNull());
  GetWakeOnPacketConnections()->AddUnique(IPAddress("192.168.10.20"));
  GetWakeOnWiFiTriggers()->insert(WakeOnWiFi::kWakeTriggerPattern);
  GetWakeOnWiFiTriggers()->insert(WakeOnWiFi::kWakeTriggerDisconnect);
  ScopeLogger::GetInstance()->EnableScopesByName("wifi");
  ScopeLogger::GetInstance()->set_verbose_level(2);
  EXPECT_CALL(*this, DoneCallback(ErrorTypeIs(Error::kSuccess))).Times(1);
  EXPECT_CALL(log, Log(_, _, _)).Times(AnyNumber());
  EXPECT_CALL(
      log, Log(_, _, HasSubstr("Wake on WiFi settings successfully verified")));
  EXPECT_CALL(metrics_, NotifyVerifyWakeOnWiFiSettingsResult(
                            Metrics::kVerifyWakeOnWiFiSettingsResultSuccess));
  VerifyWakeOnWiFiSettings(msg);
  // Suspend action callback cleared after being invoked.
  EXPECT_TRUE(SuspendActionsCallbackIsNull());
  ScopeLogger::GetInstance()->EnableScopesByName("-wifi");
  ScopeLogger::GetInstance()->set_verbose_level(0);

  // Unsuccessful verification if locally stored settings do not match.
  GetWakeOnWiFiTriggers()->erase(WakeOnWiFi::kWakeTriggerDisconnect);
  EXPECT_CALL(log, Log(_, _, _)).Times(AnyNumber());
  EXPECT_CALL(log,
              Log(logging::LOG_ERROR, _,
                  HasSubstr(
                      " failed: discrepancy between wake-on-packet settings on "
                      "NIC and those in local data structure detected")));
  EXPECT_CALL(metrics_, NotifyVerifyWakeOnWiFiSettingsResult(
                            Metrics::kVerifyWakeOnWiFiSettingsResultFailure));
  VerifyWakeOnWiFiSettings(msg);
}

TEST_F(WakeOnWiFiTestWithMockDispatcher,
       VerifyWakeOnWiFiSettings_WakeOnSSIDRules) {
  ScopedMockLog log;
  // Create a non-trivial Nl80211 response to a NL80211_CMD_GET_WOWLAN request
  // indicating that that the NIC wakes on two SSIDs represented by kSSIDBytes1
  // and kSSIDBytes2 and scans for them at interval
  // kNetDetectScanIntervalSeconds.
  GetWakeOnPacketConnMessage msg;
  NetlinkPacket packet(kResponseWakeOnSSID, sizeof(kResponseWakeOnSSID));
  msg.InitFromPacket(&packet, NetlinkMessage::MessageContext());
  // Successful verification and consequent invocation of callback.
  SetSuspendActionsDoneCallback();
  EXPECT_FALSE(SuspendActionsCallbackIsNull());
  GetWakeOnWiFiTriggers()->insert(WakeOnWiFi::kWakeTriggerSSID);
  AddSSIDToWhitelist(kSSIDBytes1, sizeof(kSSIDBytes1),
                     GetWakeOnSSIDWhitelist());
  AddSSIDToWhitelist(kSSIDBytes2, sizeof(kSSIDBytes2),
                     GetWakeOnSSIDWhitelist());
  SetNetDetectScanPeriodSeconds(kNetDetectScanIntervalSeconds);
  ScopeLogger::GetInstance()->EnableScopesByName("wifi");
  ScopeLogger::GetInstance()->set_verbose_level(2);
  EXPECT_CALL(*this, DoneCallback(ErrorTypeIs(Error::kSuccess))).Times(1);
  EXPECT_CALL(log, Log(_, _, _)).Times(AnyNumber());
  EXPECT_CALL(
      log, Log(_, _, HasSubstr("Wake on WiFi settings successfully verified")));
  EXPECT_CALL(metrics_, NotifyVerifyWakeOnWiFiSettingsResult(
                            Metrics::kVerifyWakeOnWiFiSettingsResultSuccess));
  VerifyWakeOnWiFiSettings(msg);
  // Suspend action callback cleared after being invoked.
  EXPECT_TRUE(SuspendActionsCallbackIsNull());
  ScopeLogger::GetInstance()->EnableScopesByName("-wifi");
  ScopeLogger::GetInstance()->set_verbose_level(0);
}

TEST_F(WakeOnWiFiTestWithMockDispatcher,
       VerifyWakeOnWiFiSettingsSuccess_NoDoneCallback) {
  ScopedMockLog log;
  // Create an Nl80211 response to a NL80211_CMD_GET_WOWLAN request
  // indicating that there are no wake-on-packet rules programmed into the NIC.
  GetWakeOnPacketConnMessage msg;
  NetlinkPacket packet(kResponseNoIPAddresses, sizeof(kResponseNoIPAddresses));
  msg.InitFromPacket(&packet, NetlinkMessage::MessageContext());
  // Successful verification, but since there is no suspend action callback
  // set, no callback is invoked.
  EXPECT_TRUE(SuspendActionsCallbackIsNull());
  EXPECT_TRUE(GetWakeOnPacketConnections()->Empty());
  ScopeLogger::GetInstance()->EnableScopesByName("wifi");
  ScopeLogger::GetInstance()->set_verbose_level(2);
  EXPECT_CALL(*this, DoneCallback(_)).Times(0);
  EXPECT_CALL(log, Log(_, _, _)).Times(AnyNumber());
  EXPECT_CALL(
      log, Log(_, _, HasSubstr("Wake on WiFi settings successfully verified")));
  EXPECT_CALL(metrics_, NotifyVerifyWakeOnWiFiSettingsResult(
                            Metrics::kVerifyWakeOnWiFiSettingsResultSuccess));
  VerifyWakeOnWiFiSettings(msg);
  ScopeLogger::GetInstance()->EnableScopesByName("-wifi");
  ScopeLogger::GetInstance()->set_verbose_level(0);
}

TEST_F(WakeOnWiFiTestWithMockDispatcher,
       RetrySetWakeOnPacketConnections_LessThanMaxRetries) {
  ScopedMockLog log;
  // Max retries not reached yet, so send Nl80211 message to program NIC again.
  GetWakeOnWiFiTriggers()->insert(WakeOnWiFi::kWakeTriggerDisconnect);
  SetNumSetWakeOnPacketRetries(WakeOnWiFi::kMaxSetWakeOnPacketRetries - 1);
  EXPECT_CALL(
      netlink_manager_,
      SendNl80211Message(IsNl80211Command(kNl80211FamilyId,
                                          SetWakeOnPacketConnMessage::kCommand),
                         _, _, _)).Times(1);
  RetrySetWakeOnPacketConnections();
  EXPECT_EQ(GetNumSetWakeOnPacketRetries(),
            WakeOnWiFi::kMaxSetWakeOnPacketRetries);
}

TEST_F(WakeOnWiFiTestWithMockDispatcher,
       RetrySetWakeOnPacketConnections_MaxAttemptsWithCallbackSet) {
  ScopedMockLog log;
  // Max retry attempts reached. Suspend actions done callback is set, so it
  // is invoked.
  SetNumSetWakeOnPacketRetries(WakeOnWiFi::kMaxSetWakeOnPacketRetries);
  SetSuspendActionsDoneCallback();
  ScopeLogger::GetInstance()->EnableScopesByName("wifi");
  ScopeLogger::GetInstance()->set_verbose_level(3);
  EXPECT_FALSE(SuspendActionsCallbackIsNull());
  EXPECT_CALL(*this, DoneCallback(ErrorTypeIs(Error::kOperationFailed)))
      .Times(1);
  EXPECT_CALL(netlink_manager_, SendNl80211Message(_, _, _, _)).Times(0);
  EXPECT_CALL(log, Log(_, _, _)).Times(AnyNumber());
  EXPECT_CALL(log, Log(_, _, HasSubstr("max retry attempts reached")));
  RetrySetWakeOnPacketConnections();
  EXPECT_TRUE(SuspendActionsCallbackIsNull());
  EXPECT_EQ(GetNumSetWakeOnPacketRetries(), 0);
  ScopeLogger::GetInstance()->EnableScopesByName("-wifi");
  ScopeLogger::GetInstance()->set_verbose_level(0);
}

TEST_F(WakeOnWiFiTestWithMockDispatcher,
       RetrySetWakeOnPacketConnections_MaxAttemptsCallbackUnset) {
  ScopedMockLog log;
  // If there is no suspend action callback set, no suspend callback should be
  // invoked.
  SetNumSetWakeOnPacketRetries(WakeOnWiFi::kMaxSetWakeOnPacketRetries);
  ScopeLogger::GetInstance()->EnableScopesByName("wifi");
  ScopeLogger::GetInstance()->set_verbose_level(3);
  EXPECT_TRUE(SuspendActionsCallbackIsNull());
  EXPECT_CALL(*this, DoneCallback(_)).Times(0);
  EXPECT_CALL(log, Log(_, _, _)).Times(AnyNumber());
  EXPECT_CALL(log, Log(_, _, HasSubstr("max retry attempts reached")));
  RetrySetWakeOnPacketConnections();
  ScopeLogger::GetInstance()->EnableScopesByName("-wifi");
  ScopeLogger::GetInstance()->set_verbose_level(0);
}

TEST_F(WakeOnWiFiTestWithMockDispatcher,
       ParseWakeOnWiFiCapabilities_DisconnectPatternSSIDSupported) {
  ClearWakeOnWiFiTriggersSupported();
  NewWiphyMessage msg;
  NetlinkPacket packet(kNewWiphyNlMsg, sizeof(kNewWiphyNlMsg));
  msg.InitFromPacket(&packet, NetlinkMessage::MessageContext());
  ParseWakeOnWiFiCapabilities(msg);
  EXPECT_TRUE(GetWakeOnWiFiTriggersSupported()->find(
                  WakeOnWiFi::kWakeTriggerDisconnect) !=
              GetWakeOnWiFiTriggersSupported()->end());
  EXPECT_TRUE(
      GetWakeOnWiFiTriggersSupported()->find(WakeOnWiFi::kWakeTriggerPattern) !=
      GetWakeOnWiFiTriggersSupported()->end());
  EXPECT_TRUE(
      GetWakeOnWiFiTriggersSupported()->find(WakeOnWiFi::kWakeTriggerSSID) !=
      GetWakeOnWiFiTriggersSupported()->end());
  EXPECT_EQ(GetWakeOnWiFiMaxPatterns(), kNewWiphyNlMsg_MaxPatterns);
  EXPECT_EQ(GetWakeOnWiFiMaxSSIDs(), kNewWiphyNlMsg_MaxSSIDs);
}

TEST_F(WakeOnWiFiTestWithMockDispatcher,
       ParseWakeOnWiFiCapabilities_UnsupportedPatternLen) {
  ClearWakeOnWiFiTriggersSupported();
  NewWiphyMessage msg;
  // Modify the range of support pattern lengths to [0-1] bytes, which is less
  // than what we need to use  our IPV4 (30 bytes) or IPV6 (38 bytes) patterns.
  MutableNetlinkPacket packet(kNewWiphyNlMsg, sizeof(kNewWiphyNlMsg));
  struct nl80211_pattern_support* patt_support =
      reinterpret_cast<struct nl80211_pattern_support*>(
          &packet.GetMutablePayload()->GetData()[
              kNewWiphyNlMsg_PattSupportOffset]);
  patt_support->min_pattern_len = 0;
  patt_support->max_pattern_len = 1;
  msg.InitFromPacket(&packet, NetlinkMessage::MessageContext());
  ParseWakeOnWiFiCapabilities(msg);
  EXPECT_TRUE(GetWakeOnWiFiTriggersSupported()->find(
                  WakeOnWiFi::kWakeTriggerDisconnect) !=
              GetWakeOnWiFiTriggersSupported()->end());
  EXPECT_TRUE(
      GetWakeOnWiFiTriggersSupported()->find(WakeOnWiFi::kWakeTriggerSSID) !=
      GetWakeOnWiFiTriggersSupported()->end());
  // Ensure that ParseWakeOnWiFiCapabilities realizes that our IP address
  // patterns cannot be used given the support pattern length range reported.
  EXPECT_FALSE(
      GetWakeOnWiFiTriggersSupported()->find(WakeOnWiFi::kWakeTriggerPattern) !=
      GetWakeOnWiFiTriggersSupported()->end());
}

TEST_F(WakeOnWiFiTestWithMockDispatcher,
       ParseWakeOnWiFiCapabilities_DisconnectNotSupported) {
  ClearWakeOnWiFiTriggersSupported();
  NewWiphyMessage msg;
  // Change the NL80211_WOWLAN_TRIG_DISCONNECT flag attribute into the
  // NL80211_WOWLAN_TRIG_MAGIC_PKT flag attribute, so that this message
  // no longer reports wake on disconnect as a supported capability.
  MutableNetlinkPacket packet(kNewWiphyNlMsg, sizeof(kNewWiphyNlMsg));
  struct nlattr* wowlan_trig_disconnect_attr =
      reinterpret_cast<struct nlattr*>(
          &packet.GetMutablePayload()->GetData()[
              kNewWiphyNlMsg_WowlanTrigDisconnectAttributeOffset]);
  wowlan_trig_disconnect_attr->nla_type = NL80211_WOWLAN_TRIG_MAGIC_PKT;
  msg.InitFromPacket(&packet, NetlinkMessage::MessageContext());
  ParseWakeOnWiFiCapabilities(msg);
  EXPECT_TRUE(
      GetWakeOnWiFiTriggersSupported()->find(WakeOnWiFi::kWakeTriggerPattern) !=
      GetWakeOnWiFiTriggersSupported()->end());
  EXPECT_TRUE(
      GetWakeOnWiFiTriggersSupported()->find(WakeOnWiFi::kWakeTriggerSSID) !=
      GetWakeOnWiFiTriggersSupported()->end());
  // Ensure that ParseWakeOnWiFiCapabilities realizes that wake on disconnect
  // is not supported.
  EXPECT_FALSE(GetWakeOnWiFiTriggersSupported()->find(
                   WakeOnWiFi::kWakeTriggerDisconnect) !=
               GetWakeOnWiFiTriggersSupported()->end());
}

TEST_F(WakeOnWiFiTestWithMockDispatcher,
       ParseWakeOnWiFiCapabilities_SSIDNotSupported) {
  ClearWakeOnWiFiTriggersSupported();
  NewWiphyMessage msg;
  // Change the NL80211_WOWLAN_TRIG_NET_DETECT flag attribute type to an invalid
  // attribute type (0), so that this message no longer reports wake on SSID
  // as a supported capability.
  MutableNetlinkPacket packet(kNewWiphyNlMsg, sizeof(kNewWiphyNlMsg));
  struct nlattr* wowlan_trig_net_detect_attr =
      reinterpret_cast<struct nlattr*>(
          &packet.GetMutablePayload()->GetData()[
              kNewWiphyNlMsg_WowlanTrigNetDetectAttributeOffset]);
  wowlan_trig_net_detect_attr->nla_type = 0;
  msg.InitFromPacket(&packet, NetlinkMessage::MessageContext());
  ParseWakeOnWiFiCapabilities(msg);
  EXPECT_TRUE(
      GetWakeOnWiFiTriggersSupported()->find(WakeOnWiFi::kWakeTriggerPattern) !=
      GetWakeOnWiFiTriggersSupported()->end());
  EXPECT_TRUE(GetWakeOnWiFiTriggersSupported()->find(
                  WakeOnWiFi::kWakeTriggerDisconnect) !=
              GetWakeOnWiFiTriggersSupported()->end());
  // Ensure that ParseWakeOnWiFiCapabilities realizes that wake on SSID is not
  // supported.
  EXPECT_FALSE(
      GetWakeOnWiFiTriggersSupported()->find(WakeOnWiFi::kWakeTriggerSSID) !=
      GetWakeOnWiFiTriggersSupported()->end());
}

TEST_F(WakeOnWiFiTestWithMockDispatcher,
       ApplyWakeOnWiFiSettings_WiphyIndexNotReceived) {
  ScopedMockLog log;
  // ApplyWakeOnWiFiSettings should return immediately if the wifi interface
  // index has not been received when the function is called.
  SetWiphyIndexReceivedToFalse();
  EXPECT_CALL(netlink_manager_,
              SendNl80211Message(IsDisableWakeOnWiFiMsg(), _, _, _)).Times(0);
  EXPECT_CALL(log, Log(_, _, _)).Times(AnyNumber());
  EXPECT_CALL(log, Log(logging::LOG_ERROR, _,
                       HasSubstr("Interface index not yet received")));
  ApplyWakeOnWiFiSettings();
}

TEST_F(WakeOnWiFiTestWithMockDispatcher,
       ApplyWakeOnWiFiSettings_WiphyIndexReceived) {
  // Disable wake on WiFi if there are no wake on WiFi triggers registered.
  EXPECT_CALL(
      netlink_manager_,
      SendNl80211Message(IsNl80211Command(kNl80211FamilyId,
                                          SetWakeOnPacketConnMessage::kCommand),
                         _, _, _)).Times(0);
  EXPECT_CALL(netlink_manager_,
              SendNl80211Message(IsDisableWakeOnWiFiMsg(), _, _, _)).Times(1);
  ApplyWakeOnWiFiSettings();

  // Otherwise, program the NIC.
  IPAddress ip_addr("1.1.1.1");
  GetWakeOnPacketConnections()->AddUnique(ip_addr);
  GetWakeOnWiFiTriggers()->insert(WakeOnWiFi::kWakeTriggerPattern);
  EXPECT_FALSE(GetWakeOnPacketConnections()->Empty());
  EXPECT_CALL(
      netlink_manager_,
      SendNl80211Message(IsNl80211Command(kNl80211FamilyId,
                                          SetWakeOnPacketConnMessage::kCommand),
                         _, _, _)).Times(1);
  EXPECT_CALL(netlink_manager_,
              SendNl80211Message(IsDisableWakeOnWiFiMsg(), _, _, _)).Times(0);
  ApplyWakeOnWiFiSettings();
}

TEST_F(WakeOnWiFiTestWithMockDispatcher,
       BeforeSuspendActions_ReportDoneImmediately) {
  ScopedMockLog log;
  const bool is_connected = true;
  const bool start_lease_renewal_timer = true;
  AddSSIDToWhitelist(kSSIDBytes1, sizeof(kSSIDBytes1),
                     GetWakeOnSSIDWhitelist());
  // If no triggers are supported, no triggers will be programmed into the NIC.
  ClearWakeOnWiFiTriggersSupported();
  SetSuspendActionsDoneCallback();
  SetInDarkResume(true);
  SetLastWakeReason(WakeOnWiFi::kWakeTriggerPattern);
  AddResultToLastSSIDResults();
  // Do not report done immediately in dark resume, since we need to program it
  // to disable wake on WiFi.
  EXPECT_CALL(*this, DoneCallback(_)).Times(0);
  BeforeSuspendActions(is_connected, start_lease_renewal_timer,
                       kTimeToNextLeaseRenewalLong);
  EXPECT_FALSE(GetInDarkResume());
  EXPECT_EQ(WakeOnWiFi::kWakeTriggerUnsupported, GetLastWakeReason());
  EXPECT_TRUE(GetLastSSIDMatchFreqs().empty());

  SetInDarkResume(false);
  SetLastWakeReason(WakeOnWiFi::kWakeTriggerPattern);
  AddResultToLastSSIDResults();
  // Report done immediately on normal suspend, since wake on WiFi should
  // already have been disabled on the NIC on a previous resume.
  EXPECT_CALL(*this, DoneCallback(_)).Times(1);
  EXPECT_CALL(log, Log(_, _, _)).Times(AnyNumber());
  ScopeLogger::GetInstance()->EnableScopesByName("wifi");
  ScopeLogger::GetInstance()->set_verbose_level(1);
  EXPECT_CALL(
      log,
      Log(_, _,
          HasSubstr(
              "No need to disable wake on WiFi on NIC in regular suspend")));
  BeforeSuspendActions(is_connected, start_lease_renewal_timer,
                       kTimeToNextLeaseRenewalLong);
  EXPECT_EQ(WakeOnWiFi::kWakeTriggerUnsupported, GetLastWakeReason());
  EXPECT_TRUE(GetLastSSIDMatchFreqs().empty());
  ScopeLogger::GetInstance()->EnableScopesByName("-wifi");
  ScopeLogger::GetInstance()->set_verbose_level(0);
}

TEST_F(WakeOnWiFiTestWithMockDispatcher,
       BeforeSuspendActions_FeaturesDisabledOrTriggersUnsupported) {
  const bool is_connected = true;
  const bool start_lease_renewal_timer = true;
  AddSSIDToWhitelist(kSSIDBytes1, sizeof(kSSIDBytes1),
                     GetWakeOnSSIDWhitelist());
  SetInDarkResume(false);
  SetSuspendActionsDoneCallback();
  // No features enabled, so no triggers programmed.
  DisableWakeOnWiFiFeatures();
  SetLastWakeReason(WakeOnWiFi::kWakeTriggerPattern);
  AddResultToLastSSIDResults();
  EXPECT_TRUE(GetWakeOnWiFiTriggers()->empty());
  EXPECT_CALL(*this, DoneCallback(_));
  BeforeSuspendActions(is_connected, start_lease_renewal_timer,
                       kTimeToNextLeaseRenewalLong);
  EXPECT_TRUE(GetWakeOnWiFiTriggers()->empty());
  EXPECT_EQ(WakeOnWiFi::kWakeTriggerUnsupported, GetLastWakeReason());
  EXPECT_TRUE(GetLastSSIDMatchFreqs().empty());

  // No triggers supported, so no triggers programmed.
  SetSuspendActionsDoneCallback();
  EnableWakeOnWiFiFeaturesPacketDarkConnect();
  GetWakeOnWiFiTriggersSupported()->clear();
  SetLastWakeReason(WakeOnWiFi::kWakeTriggerPattern);
  AddResultToLastSSIDResults();
  EXPECT_TRUE(GetWakeOnWiFiTriggers()->empty());
  EXPECT_CALL(*this, DoneCallback(_));
  BeforeSuspendActions(is_connected, start_lease_renewal_timer,
                       kTimeToNextLeaseRenewalLong);
  EXPECT_TRUE(GetWakeOnWiFiTriggers()->empty());
  EXPECT_EQ(WakeOnWiFi::kWakeTriggerUnsupported, GetLastWakeReason());
  EXPECT_TRUE(GetLastSSIDMatchFreqs().empty());

  // Only wake on packet feature enabled and supported.
  EnableWakeOnWiFiFeaturesPacket();
  GetWakeOnWiFiTriggersSupported()->insert(WakeOnWiFi::kWakeTriggerPattern);
  GetWakeOnPacketConnections()->AddUnique(IPAddress("1.1.1.1"));
  SetLastWakeReason(WakeOnWiFi::kWakeTriggerPattern);
  AddResultToLastSSIDResults();
  EXPECT_TRUE(GetWakeOnWiFiTriggers()->empty());
  BeforeSuspendActions(is_connected, start_lease_renewal_timer,
                       kTimeToNextLeaseRenewalLong);
  EXPECT_EQ(GetWakeOnWiFiTriggers()->size(), 1);
  EXPECT_TRUE(GetWakeOnWiFiTriggers()->find(WakeOnWiFi::kWakeTriggerPattern) !=
              GetWakeOnWiFiTriggers()->end());
  EXPECT_EQ(WakeOnWiFi::kWakeTriggerUnsupported, GetLastWakeReason());
  EXPECT_TRUE(GetLastSSIDMatchFreqs().empty());

  // Only wake on SSID feature supported.
  EnableWakeOnWiFiFeaturesDarkConnect();
  GetWakeOnPacketConnections()->Clear();
  GetWakeOnWiFiTriggersSupported()->clear();
  GetWakeOnWiFiTriggersSupported()->insert(WakeOnWiFi::kWakeTriggerDisconnect);
  GetWakeOnWiFiTriggersSupported()->insert(WakeOnWiFi::kWakeTriggerSSID);
  GetWakeOnWiFiTriggers()->clear();
  SetLastWakeReason(WakeOnWiFi::kWakeTriggerPattern);
  AddResultToLastSSIDResults();
  EXPECT_TRUE(GetWakeOnWiFiTriggers()->empty());
  BeforeSuspendActions(is_connected, start_lease_renewal_timer,
                       kTimeToNextLeaseRenewalLong);
  EXPECT_EQ(GetWakeOnWiFiTriggers()->size(), 1);
  EXPECT_TRUE(
      GetWakeOnWiFiTriggers()->find(WakeOnWiFi::kWakeTriggerDisconnect) !=
      GetWakeOnWiFiTriggers()->end());
  EXPECT_EQ(WakeOnWiFi::kWakeTriggerUnsupported, GetLastWakeReason());
  EXPECT_TRUE(GetLastSSIDMatchFreqs().empty());
}

TEST_F(WakeOnWiFiTestWithMockDispatcher,
       BeforeSuspendActions_ConnectedBeforeSuspend) {
  const bool is_connected = true;
  const bool start_lease_renewal_timer = true;
  AddSSIDToWhitelist(kSSIDBytes1, sizeof(kSSIDBytes1),
                     GetWakeOnSSIDWhitelist());
  SetSuspendActionsDoneCallback();
  EnableWakeOnWiFiFeaturesPacketDarkConnect();
  GetWakeOnPacketConnections()->AddUnique(IPAddress("1.1.1.1"));

  SetInDarkResume(true);
  GetWakeOnWiFiTriggers()->clear();
  EXPECT_TRUE(GetWakeOnWiFiTriggers()->empty());
  StartWakeToScanTimer();
  StopDHCPLeaseRenewalTimer();
  SetLastWakeReason(WakeOnWiFi::kWakeTriggerPattern);
  AddResultToLastSSIDResults();
  EXPECT_TRUE(WakeToScanTimerIsRunning());
  EXPECT_FALSE(DHCPLeaseRenewalTimerIsRunning());
  EXPECT_CALL(*this, DoneCallback(_)).Times(0);
  BeforeSuspendActions(is_connected, start_lease_renewal_timer,
                       kTimeToNextLeaseRenewalLong);
  EXPECT_FALSE(GetInDarkResume());
  EXPECT_EQ(GetWakeOnWiFiTriggers()->size(), 2);
  EXPECT_TRUE(
      GetWakeOnWiFiTriggers()->find(WakeOnWiFi::kWakeTriggerDisconnect) !=
      GetWakeOnWiFiTriggers()->end());
  EXPECT_TRUE(GetWakeOnWiFiTriggers()->find(WakeOnWiFi::kWakeTriggerPattern) !=
              GetWakeOnWiFiTriggers()->end());
  EXPECT_TRUE(DHCPLeaseRenewalTimerIsRunning());
  EXPECT_FALSE(WakeToScanTimerIsRunning());
  EXPECT_EQ(WakeOnWiFi::kWakeTriggerUnsupported, GetLastWakeReason());
  EXPECT_TRUE(GetLastSSIDMatchFreqs().empty());
}

TEST_F(WakeOnWiFiTestWithMockDispatcher,
       BeforeSuspendActions_DisconnectedBeforeSuspend) {
  const bool is_connected = false;
  const bool start_lease_renewal_timer = true;
  AddSSIDToWhitelist(kSSIDBytes1, sizeof(kSSIDBytes1),
                     GetWakeOnSSIDWhitelist());
  AddSSIDToWhitelist(kSSIDBytes2, sizeof(kSSIDBytes2),
                     GetWakeOnSSIDWhitelist());
  SetSuspendActionsDoneCallback();
  EnableWakeOnWiFiFeaturesPacketDarkConnect();

  // Do not start wake to scan timer if there are less whitelisted SSIDs (2)
  // than net detect SSIDs we support (10).
  SetInDarkResume(true);
  GetWakeOnWiFiTriggers()->clear();
  StopWakeToScanTimer();
  StartDHCPLeaseRenewalTimer();
  SetWakeOnWiFiMaxSSIDs(10);
  SetLastWakeReason(WakeOnWiFi::kWakeTriggerPattern);
  AddResultToLastSSIDResults();
  EXPECT_EQ(2, GetWakeOnSSIDWhitelist()->size());
  EXPECT_FALSE(WakeToScanTimerIsRunning());
  EXPECT_TRUE(DHCPLeaseRenewalTimerIsRunning());
  EXPECT_CALL(*this, DoneCallback(_)).Times(0);
  BeforeSuspendActions(is_connected, start_lease_renewal_timer,
                       kTimeToNextLeaseRenewalLong);
  EXPECT_EQ(2, GetWakeOnSSIDWhitelist()->size());
  EXPECT_FALSE(GetInDarkResume());
  EXPECT_EQ(GetWakeOnWiFiTriggers()->size(), 1);
  EXPECT_TRUE(GetWakeOnWiFiTriggers()->find(WakeOnWiFi::kWakeTriggerSSID) !=
              GetWakeOnWiFiTriggers()->end());
  EXPECT_FALSE(DHCPLeaseRenewalTimerIsRunning());
  EXPECT_FALSE(WakeToScanTimerIsRunning());
  EXPECT_EQ(WakeOnWiFi::kWakeTriggerUnsupported, GetLastWakeReason());
  EXPECT_TRUE(GetLastSSIDMatchFreqs().empty());

  // Start wake to scan timer if there are more whitelisted SSIDs (2) than
  // net detect SSIDs we support (1). Also, truncate the wake on SSID whitelist
  // so that it only contains as many SSIDs as we support (1).
  SetInDarkResume(true);
  GetWakeOnWiFiTriggers()->clear();
  StopWakeToScanTimer();
  StartDHCPLeaseRenewalTimer();
  SetWakeOnWiFiMaxSSIDs(1);
  SetLastWakeReason(WakeOnWiFi::kWakeTriggerPattern);
  AddResultToLastSSIDResults();
  EXPECT_EQ(2, GetWakeOnSSIDWhitelist()->size());
  EXPECT_FALSE(WakeToScanTimerIsRunning());
  EXPECT_TRUE(DHCPLeaseRenewalTimerIsRunning());
  EXPECT_CALL(*this, DoneCallback(_)).Times(0);
  BeforeSuspendActions(is_connected, start_lease_renewal_timer,
                       kTimeToNextLeaseRenewalLong);
  EXPECT_EQ(1, GetWakeOnSSIDWhitelist()->size());
  EXPECT_FALSE(GetInDarkResume());
  EXPECT_EQ(GetWakeOnWiFiTriggers()->size(), 1);
  EXPECT_TRUE(GetWakeOnWiFiTriggers()->find(WakeOnWiFi::kWakeTriggerSSID) !=
              GetWakeOnWiFiTriggers()->end());
  EXPECT_FALSE(DHCPLeaseRenewalTimerIsRunning());
  EXPECT_TRUE(WakeToScanTimerIsRunning());
  EXPECT_EQ(WakeOnWiFi::kWakeTriggerUnsupported, GetLastWakeReason());
  EXPECT_TRUE(GetLastSSIDMatchFreqs().empty());

  // Neither add the wake on SSID trigger nor start the wake to scan timer if
  // there are no whitelisted SSIDs.
  SetInDarkResume(true);
  GetWakeOnSSIDWhitelist()->clear();
  StopWakeToScanTimer();
  StartDHCPLeaseRenewalTimer();
  SetWakeOnWiFiMaxSSIDs(10);
  SetLastWakeReason(WakeOnWiFi::kWakeTriggerPattern);
  AddResultToLastSSIDResults();
  EXPECT_TRUE(GetWakeOnSSIDWhitelist()->empty());
  EXPECT_FALSE(WakeToScanTimerIsRunning());
  EXPECT_TRUE(DHCPLeaseRenewalTimerIsRunning());
  EXPECT_CALL(*this, DoneCallback(_)).Times(0);
  BeforeSuspendActions(is_connected, start_lease_renewal_timer,
                       kTimeToNextLeaseRenewalLong);
  EXPECT_TRUE(GetWakeOnSSIDWhitelist()->empty());
  EXPECT_FALSE(GetInDarkResume());
  EXPECT_TRUE(GetWakeOnWiFiTriggers()->empty());
  EXPECT_FALSE(DHCPLeaseRenewalTimerIsRunning());
  EXPECT_FALSE(WakeToScanTimerIsRunning());
  EXPECT_EQ(WakeOnWiFi::kWakeTriggerUnsupported, GetLastWakeReason());
  EXPECT_TRUE(GetLastSSIDMatchFreqs().empty());
}

TEST_F(WakeOnWiFiTestWithMockDispatcher, DisableWakeOnWiFi_ClearsTriggers) {
  GetWakeOnWiFiTriggers()->insert(WakeOnWiFi::kWakeTriggerPattern);
  EXPECT_FALSE(GetWakeOnWiFiTriggers()->empty());
  DisableWakeOnWiFi();
  EXPECT_TRUE(GetWakeOnWiFiTriggers()->empty());
}

TEST_F(WakeOnWiFiTestWithMockDispatcher, ParseWakeOnSSIDResults) {
  SetWakeOnPacketConnMessage msg;
  NetlinkPacket packet(kWakeReasonSSIDNlMsg, sizeof(kWakeReasonSSIDNlMsg));
  msg.InitFromPacket(&packet, GetWakeupReportMsgContext());
  AttributeListConstRefPtr triggers;
  ASSERT_TRUE(msg.const_attributes()->ConstGetNestedAttributeList(
      NL80211_ATTR_WOWLAN_TRIGGERS, &triggers));
  AttributeListConstRefPtr results_list;
  ASSERT_TRUE(triggers->ConstGetNestedAttributeList(
      NL80211_WOWLAN_TRIG_NET_DETECT_RESULTS, &results_list));
  WiFi::FreqSet freqs = ParseWakeOnSSIDResults(results_list);
  EXPECT_EQ(arraysize(kSSID1FreqMatches), freqs.size());
  for (uint32_t freq : kSSID1FreqMatches) {
    EXPECT_TRUE(freqs.find(freq) != freqs.end());
  }
}

TEST_F(WakeOnWiFiTestWithMockDispatcher, OnScanStarted_NotInDarkResume) {
  SetInDarkResume(false);
  EXPECT_CALL(metrics_, NotifyScanStartedInDarkResume(_)).Times(0);
  OnScanStarted(false);
}

TEST_F(WakeOnWiFiTestWithMockDispatcher, OnScanStarted_IgnoredWakeReasons) {
  // Do not log metrics if we entered dark resume because of wake on SSID or
  // wake on disconnect.
  SetInDarkResume(true);
  SetLastWakeReason(WakeOnWiFi::kWakeTriggerSSID);
  EXPECT_CALL(metrics_, NotifyScanStartedInDarkResume(_)).Times(0);
  OnScanStarted(false);

  SetLastWakeReason(WakeOnWiFi::kWakeTriggerDisconnect);
  EXPECT_CALL(metrics_, NotifyScanStartedInDarkResume(_)).Times(0);
  OnScanStarted(false);
}

TEST_F(WakeOnWiFiTestWithMockDispatcher, OnScanStarted_LogMetrics) {
  // Log metrics if we entered dark resume because of wake on pattern or an
  // unsupported wake reason.
  SetInDarkResume(true);
  SetLastWakeReason(WakeOnWiFi::kWakeTriggerUnsupported);
  EXPECT_CALL(metrics_, NotifyScanStartedInDarkResume(_));
  OnScanStarted(false);

  SetLastWakeReason(WakeOnWiFi::kWakeTriggerPattern);
  EXPECT_CALL(metrics_, NotifyScanStartedInDarkResume(_));
  OnScanStarted(false);

  // Log error if an active scan is launched.
  ScopedMockLog log;
  EXPECT_CALL(log, Log(_, _, _)).Times(AnyNumber());
  EXPECT_CALL(log,
              Log(logging::LOG_ERROR, _,
                  HasSubstr("Unexpected active scan launched in dark resume")));
  EXPECT_CALL(metrics_, NotifyScanStartedInDarkResume(_));
  OnScanStarted(true);
}

TEST_F(WakeOnWiFiTestWithDispatcher, InitiateScanInDarkResume) {
  WiFi::FreqSet freqs;

  // If we are not scanning on specific frequencies, do not enable the retry
  // mechanism.
  EXPECT_EQ(0, GetDarkResumeScanRetriesLeft());
  EXPECT_CALL(*this, InitiateScanCallback(freqs));
  InitiateScanInDarkResume(freqs);
  EXPECT_EQ(0, GetDarkResumeScanRetriesLeft());

  // Otherwise, start channel specific passive scan with retries.
  freqs.insert(1);
  EXPECT_LE(freqs.size(), WakeOnWiFi::kMaxFreqsForDarkResumeScanRetries);
  EXPECT_EQ(0, GetDarkResumeScanRetriesLeft());
  EXPECT_CALL(*this, InitiateScanCallback(freqs));
  InitiateScanInDarkResume(freqs);
  EXPECT_EQ(WakeOnWiFi::kMaxDarkResumeScanRetries,
            GetDarkResumeScanRetriesLeft());
}

TEST_F(WakeOnWiFiTestWithMockDispatcher, AddRemoveWakeOnPacketConnection) {
  const string bad_ip_string("1.1");
  const string ip_string1("192.168.0.19");
  const string ip_string2("192.168.0.55");
  const string ip_string3("192.168.0.74");
  IPAddress ip_addr1(ip_string1);
  IPAddress ip_addr2(ip_string2);
  IPAddress ip_addr3(ip_string3);
  Error e;

  // Add and remove operations will fail if we provide an invalid IP address
  // string.
  EnableWakeOnWiFiFeaturesPacket();
  AddWakeOnPacketConnection(bad_ip_string, &e);
  EXPECT_EQ(e.type(), Error::kInvalidArguments);
  EXPECT_STREQ(e.message().c_str(),
               ("Invalid ip_address " + bad_ip_string).c_str());
  RemoveWakeOnPacketConnection(bad_ip_string, &e);
  EXPECT_EQ(e.type(), Error::kInvalidArguments);
  EXPECT_STREQ(e.message().c_str(),
               ("Invalid ip_address " + bad_ip_string).c_str());

  // Add and remove operations will fail if WiFi device does not support
  // pattern matching functionality, even if the feature is enabled.
  EnableWakeOnWiFiFeaturesPacket();
  ClearWakeOnWiFiTriggersSupported();
  AddWakeOnPacketConnection(ip_string1, &e);
  EXPECT_EQ(e.type(), Error::kNotSupported);
  EXPECT_STREQ(e.message().c_str(),
               "Wake on IP address patterns not supported by this WiFi device");
  RemoveAllWakeOnPacketConnections(&e);
  EXPECT_EQ(e.type(), Error::kNotSupported);
  EXPECT_STREQ(e.message().c_str(),
               "Wake on IP address patterns not supported by this WiFi device");
  RemoveWakeOnPacketConnection(ip_string2, &e);
  EXPECT_EQ(e.type(), Error::kNotSupported);
  EXPECT_STREQ(e.message().c_str(),
               "Wake on IP address patterns not supported by this WiFi device");

  // Add operation will fail if pattern matching is supported but the max number
  // of IP address patterns have already been registered.
  EnableWakeOnWiFiFeaturesPacketDarkConnect();
  GetWakeOnWiFiTriggersSupported()->insert(WakeOnWiFi::kWakeTriggerPattern);
  SetWakeOnWiFiMaxPatterns(1);
  GetWakeOnPacketConnections()->AddUnique(IPAddress(ip_string1));
  AddWakeOnPacketConnection(ip_string2, &e);
  EXPECT_EQ(e.type(), Error::kOperationFailed);
  EXPECT_STREQ(e.message().c_str(),
               "Max number of IP address patterns already registered");

  // Add and remove operations will still execute even when the wake on packet
  // feature has been disabled.
  GetWakeOnPacketConnections()->Clear();
  SetWakeOnWiFiMaxPatterns(50);
  DisableWakeOnWiFiFeatures();
  GetWakeOnWiFiTriggersSupported()->insert(WakeOnWiFi::kWakeTriggerPattern);
  AddWakeOnPacketConnection(ip_string1, &e);
  EXPECT_EQ(GetWakeOnPacketConnections()->Count(), 1);
  EXPECT_TRUE(GetWakeOnPacketConnections()->Contains(ip_addr1));
  AddWakeOnPacketConnection(ip_string2, &e);
  EXPECT_EQ(GetWakeOnPacketConnections()->Count(), 2);
  EXPECT_TRUE(GetWakeOnPacketConnections()->Contains(ip_addr2));
  RemoveWakeOnPacketConnection(ip_string1, &e);
  EXPECT_EQ(GetWakeOnPacketConnections()->Count(), 1);
  RemoveAllWakeOnPacketConnections(&e);
  EXPECT_TRUE(GetWakeOnPacketConnections()->Empty());

  // Normal functioning of add/remove operations when wake on WiFi features
  // are enabled, the NIC supports pattern matching, and the max number
  // of patterns have not been registered yet.
  EnableWakeOnWiFiFeaturesPacketDarkConnect();
  GetWakeOnPacketConnections()->Clear();
  EXPECT_TRUE(GetWakeOnPacketConnections()->Empty());
  AddWakeOnPacketConnection(ip_string1, &e);
  EXPECT_EQ(GetWakeOnPacketConnections()->Count(), 1);
  EXPECT_TRUE(GetWakeOnPacketConnections()->Contains(ip_addr1));
  EXPECT_FALSE(GetWakeOnPacketConnections()->Contains(ip_addr2));
  EXPECT_FALSE(GetWakeOnPacketConnections()->Contains(ip_addr3));

  AddWakeOnPacketConnection(ip_string2, &e);
  EXPECT_EQ(GetWakeOnPacketConnections()->Count(), 2);
  EXPECT_TRUE(GetWakeOnPacketConnections()->Contains(ip_addr1));
  EXPECT_TRUE(GetWakeOnPacketConnections()->Contains(ip_addr2));
  EXPECT_FALSE(GetWakeOnPacketConnections()->Contains(ip_addr3));

  AddWakeOnPacketConnection(ip_string3, &e);
  EXPECT_EQ(GetWakeOnPacketConnections()->Count(), 3);
  EXPECT_TRUE(GetWakeOnPacketConnections()->Contains(ip_addr1));
  EXPECT_TRUE(GetWakeOnPacketConnections()->Contains(ip_addr2));
  EXPECT_TRUE(GetWakeOnPacketConnections()->Contains(ip_addr3));

  RemoveWakeOnPacketConnection(ip_string2, &e);
  EXPECT_EQ(GetWakeOnPacketConnections()->Count(), 2);
  EXPECT_TRUE(GetWakeOnPacketConnections()->Contains(ip_addr1));
  EXPECT_FALSE(GetWakeOnPacketConnections()->Contains(ip_addr2));
  EXPECT_TRUE(GetWakeOnPacketConnections()->Contains(ip_addr3));

  // Remove fails if no such address is registered.
  RemoveWakeOnPacketConnection(ip_string2, &e);
  EXPECT_EQ(e.type(), Error::kNotFound);
  EXPECT_STREQ(e.message().c_str(),
               "No such IP address match registered to wake device");
  EXPECT_EQ(GetWakeOnPacketConnections()->Count(), 2);

  RemoveWakeOnPacketConnection(ip_string1, &e);
  EXPECT_EQ(GetWakeOnPacketConnections()->Count(), 1);
  EXPECT_FALSE(GetWakeOnPacketConnections()->Contains(ip_addr1));
  EXPECT_FALSE(GetWakeOnPacketConnections()->Contains(ip_addr2));
  EXPECT_TRUE(GetWakeOnPacketConnections()->Contains(ip_addr3));

  AddWakeOnPacketConnection(ip_string2, &e);
  EXPECT_EQ(GetWakeOnPacketConnections()->Count(), 2);
  EXPECT_FALSE(GetWakeOnPacketConnections()->Contains(ip_addr1));
  EXPECT_TRUE(GetWakeOnPacketConnections()->Contains(ip_addr2));
  EXPECT_TRUE(GetWakeOnPacketConnections()->Contains(ip_addr3));

  RemoveAllWakeOnPacketConnections(&e);
  EXPECT_EQ(GetWakeOnPacketConnections()->Count(), 0);
  EXPECT_FALSE(GetWakeOnPacketConnections()->Contains(ip_addr1));
  EXPECT_FALSE(GetWakeOnPacketConnections()->Contains(ip_addr2));
  EXPECT_FALSE(GetWakeOnPacketConnections()->Contains(ip_addr3));
}

TEST_F(WakeOnWiFiTestWithDispatcher, OnBeforeSuspend_ClearsEventHistory) {
  const int kNumEvents = WakeOnWiFi::kMaxDarkResumesPerPeriodShort - 1;
  vector<ByteString> whitelist;
  for (int i = 0; i < kNumEvents; ++i) {
    GetDarkResumeHistory()->RecordEvent();
  }
  EXPECT_EQ(kNumEvents, GetDarkResumeHistory()->Size());
  OnBeforeSuspend(true, whitelist, true, 0);
  EXPECT_TRUE(GetDarkResumeHistory()->Empty());
}

TEST_F(WakeOnWiFiTestWithDispatcher, OnBeforeSuspend_SetsWakeOnSSIDWhitelist) {
  vector<ByteString> whitelist;
  AddSSIDToWhitelist(kSSIDBytes1, sizeof(kSSIDBytes1), &whitelist);
  EXPECT_TRUE(GetWakeOnSSIDWhitelist()->empty());
  OnBeforeSuspend(true, whitelist, true, 0);
  EXPECT_FALSE(GetWakeOnSSIDWhitelist()->empty());
  EXPECT_EQ(1, GetWakeOnSSIDWhitelist()->size());
}

TEST_F(WakeOnWiFiTestWithDispatcher, OnBeforeSuspend_SetsDoneCallback) {
  vector<ByteString> whitelist;
  EXPECT_TRUE(SuspendActionsCallbackIsNull());
  OnBeforeSuspend(true, whitelist, true, 0);
  EXPECT_FALSE(SuspendActionsCallbackIsNull());
}

TEST_F(WakeOnWiFiTestWithMockDispatcher, OnBeforeSuspend_DHCPLeaseRenewal) {
  bool is_connected;
  bool have_dhcp_lease;
  vector<ByteString> whitelist;
  AddSSIDToWhitelist(kSSIDBytes1, sizeof(kSSIDBytes1), &whitelist);
  // If we are connected the time to next lease renewal is short enough, we will
  // initiate DHCP lease renewal immediately.
  is_connected = true;
  have_dhcp_lease = true;
  EXPECT_CALL(*this, RenewDHCPLeaseCallback()).Times(1);
  EXPECT_CALL(mock_dispatcher_, PostTask(_)).Times(1);
  OnBeforeSuspend(is_connected, whitelist, have_dhcp_lease,
                  kTimeToNextLeaseRenewalShort);

  // No immediate DHCP lease renewal because we are not connected.
  is_connected = false;
  have_dhcp_lease = true;
  EXPECT_CALL(*this, RenewDHCPLeaseCallback()).Times(0);
  EXPECT_CALL(mock_dispatcher_, PostTask(_)).Times(1);
  OnBeforeSuspend(is_connected, whitelist, have_dhcp_lease,
                  kTimeToNextLeaseRenewalShort);

  // No immediate DHCP lease renewal because the time to the next lease renewal
  // is longer than the threshold.
  is_connected = true;
  have_dhcp_lease = true;
  EXPECT_CALL(*this, RenewDHCPLeaseCallback()).Times(0);
  EXPECT_CALL(mock_dispatcher_, PostTask(_)).Times(1);
  OnBeforeSuspend(is_connected, whitelist, have_dhcp_lease,
                  kTimeToNextLeaseRenewalLong);

  // No immediate DHCP lease renewal because we do not have a DHCP lease that
  // needs to be renewed.
  is_connected = true;
  have_dhcp_lease = false;
  EXPECT_CALL(*this, RenewDHCPLeaseCallback()).Times(0);
  EXPECT_CALL(mock_dispatcher_, PostTask(_)).Times(1);
  OnBeforeSuspend(is_connected, whitelist, have_dhcp_lease,
                  kTimeToNextLeaseRenewalLong);
}

TEST_F(WakeOnWiFiTestWithDispatcher, OnDarkResume_ResetsDarkResumeScanRetries) {
  const bool is_connected = true;
  vector<ByteString> whitelist;
  SetDarkResumeScanRetriesLeft(3);
  EXPECT_EQ(3, GetDarkResumeScanRetriesLeft());
  OnDarkResume(is_connected, whitelist);
  EXPECT_EQ(0, GetDarkResumeScanRetriesLeft());
}

TEST_F(WakeOnWiFiTestWithDispatcher, OnDarkResume_SetsWakeOnSSIDWhitelist) {
  const bool is_connected = true;
  vector<ByteString> whitelist;
  AddSSIDToWhitelist(kSSIDBytes1, sizeof(kSSIDBytes1), &whitelist);
  EXPECT_TRUE(GetWakeOnSSIDWhitelist()->empty());
  OnDarkResume(is_connected, whitelist);
  EXPECT_FALSE(GetWakeOnSSIDWhitelist()->empty());
  EXPECT_EQ(1, GetWakeOnSSIDWhitelist()->size());
}

TEST_F(WakeOnWiFiTestWithDispatcher,
       OnDarkResume_WakeReasonUnsupported_Connected_Timeout) {
  // Test that correct actions are taken if we enter OnDarkResume on an
  // unsupported wake trigger while connected, then timeout on suspend actions
  // before suspending again.
  const bool is_connected = true;
  SetLastWakeReason(WakeOnWiFi::kWakeTriggerUnsupported);
  vector<ByteString> whitelist;
  AddSSIDToWhitelist(kSSIDBytes1, sizeof(kSSIDBytes1), &whitelist);
  InitStateForDarkResume();
  EXPECT_TRUE(DarkResumeActionsTimeOutCallbackIsCancelled());
  // Renew DHCP lease if we are connected in dark resume.
  EXPECT_CALL(*this, RenewDHCPLeaseCallback());
  EXPECT_CALL(metrics_, NotifyWakeOnWiFiOnDarkResume(
                            WakeOnWiFi::kWakeTriggerUnsupported));
  OnDarkResume(is_connected, whitelist);
  EXPECT_FALSE(DarkResumeActionsTimeOutCallbackIsCancelled());
  // Trigger timeout callback.
  // Since we timeout, we are disconnected before suspend.
  StartDHCPLeaseRenewalTimer();
  SetExpectationsDisconnectedBeforeSuspend();
  dispatcher_.DispatchPendingEvents();
  EXPECT_FALSE(DHCPLeaseRenewalTimerIsRunning());
  VerifyStateDisconnectedBeforeSuspend();
}

TEST_F(WakeOnWiFiTestWithDispatcher,
       OnDarkResume_WakeReasonUnsupported_Connected_NoAutoconnectableServices) {
  // Test that correct actions are taken if we enter OnDarkResume on an
  // unsupported wake trigger while connected, then go back to suspend because
  // we could not find any services available for autoconnect.
  const bool is_connected = true;
  SetLastWakeReason(WakeOnWiFi::kWakeTriggerUnsupported);
  vector<ByteString> whitelist;
  AddSSIDToWhitelist(kSSIDBytes1, sizeof(kSSIDBytes1), &whitelist);
  InitStateForDarkResume();
  EXPECT_TRUE(DarkResumeActionsTimeOutCallbackIsCancelled());
  // Renew DHCP lease if we are connected in dark resume.
  EXPECT_CALL(*this, RenewDHCPLeaseCallback());
  EXPECT_CALL(metrics_, NotifyWakeOnWiFiOnDarkResume(
                            WakeOnWiFi::kWakeTriggerUnsupported));
  OnDarkResume(is_connected, whitelist);
  EXPECT_FALSE(DarkResumeActionsTimeOutCallbackIsCancelled());

  StartDHCPLeaseRenewalTimer();
  SetExpectationsDisconnectedBeforeSuspend();
  OnNoAutoConnectableServicesAfterScan(whitelist);
  EXPECT_FALSE(DHCPLeaseRenewalTimerIsRunning());
  VerifyStateDisconnectedBeforeSuspend();
}

TEST_F(WakeOnWiFiTestWithDispatcher,
       OnDarkResume_WakeReasonUnsupported_Connected_LeaseObtained) {
  // Test that correct actions are taken if we enter OnDarkResume on an
  // unsupported wake trigger while connected, then connect and obtain a DHCP
  // lease before suspending again.
  const bool is_connected = true;
  const bool have_dhcp_lease = true;
  const uint32_t time_to_next_lease_renewal = 10;
  SetLastWakeReason(WakeOnWiFi::kWakeTriggerUnsupported);
  vector<ByteString> whitelist;
  AddSSIDToWhitelist(kSSIDBytes1, sizeof(kSSIDBytes1), &whitelist);
  InitStateForDarkResume();
  EXPECT_TRUE(DarkResumeActionsTimeOutCallbackIsCancelled());
  // Renew DHCP lease if we are connected in dark resume.
  EXPECT_CALL(*this, RenewDHCPLeaseCallback());
  EXPECT_CALL(metrics_, NotifyWakeOnWiFiOnDarkResume(
                            WakeOnWiFi::kWakeTriggerUnsupported));
  OnDarkResume(is_connected, whitelist);
  EXPECT_FALSE(DarkResumeActionsTimeOutCallbackIsCancelled());
  // Lease obtained.
  // Since a lease is obtained, we are connected before suspend.
  StopDHCPLeaseRenewalTimer();
  StartWakeToScanTimer();
  SetExpectationsConnectedBeforeSuspend();
  OnConnectedAndReachable(have_dhcp_lease, time_to_next_lease_renewal);
  EXPECT_TRUE(DHCPLeaseRenewalTimerIsRunning());
  EXPECT_FALSE(WakeToScanTimerIsRunning());
  VerifyStateConnectedBeforeSuspend();
}

TEST_F(WakeOnWiFiTestWithDispatcher,
       OnDarkResume_WakeReasonUnsupported_NotConnected_Timeout) {
  // Test that correct actions are taken if we enter OnDarkResume on an
  // unsupported wake trigger while not connected, then timeout on suspend
  // actions before suspending again.
  const bool is_connected = false;
  SetLastWakeReason(WakeOnWiFi::kWakeTriggerUnsupported);
  vector<ByteString> whitelist;
  AddSSIDToWhitelist(kSSIDBytes1, sizeof(kSSIDBytes1), &whitelist);
  InitStateForDarkResume();
  EXPECT_TRUE(DarkResumeActionsTimeOutCallbackIsCancelled());
  // Initiate scan if we are not connected in dark resume.
  EXPECT_CALL(*this, RemoveSupplicantNetworksCallback());
  EXPECT_CALL(metrics_, NotifyDarkResumeInitiateScan());
  EXPECT_CALL(*this, InitiateScanCallback(_));
  EXPECT_CALL(metrics_, NotifyWakeOnWiFiOnDarkResume(
                            WakeOnWiFi::kWakeTriggerUnsupported));
  OnDarkResume(is_connected, whitelist);
  EXPECT_FALSE(DarkResumeActionsTimeOutCallbackIsCancelled());
  // Trigger timeout callback.
  // Since we timeout, we are disconnected before suspend.
  StartDHCPLeaseRenewalTimer();
  SetExpectationsDisconnectedBeforeSuspend();
  dispatcher_.DispatchPendingEvents();
  EXPECT_FALSE(DHCPLeaseRenewalTimerIsRunning());
  VerifyStateDisconnectedBeforeSuspend();
}

TEST_F(
    WakeOnWiFiTestWithDispatcher,
    OnDarkResume_WakeReasonUnsupported_NotConnected_NoAutoconnectableServices) {
  // Test that correct actions are taken if we enter OnDarkResume on an
  // unsupported wake trigger while not connected, then go back to suspend
  // because we could not find any services available for autoconnect.
  const bool is_connected = false;
  SetLastWakeReason(WakeOnWiFi::kWakeTriggerUnsupported);
  vector<ByteString> whitelist;
  AddSSIDToWhitelist(kSSIDBytes1, sizeof(kSSIDBytes1), &whitelist);
  InitStateForDarkResume();
  EXPECT_TRUE(DarkResumeActionsTimeOutCallbackIsCancelled());
  // Initiate scan if we are not connected in dark resume.
  EXPECT_CALL(*this, RemoveSupplicantNetworksCallback());
  EXPECT_CALL(metrics_, NotifyDarkResumeInitiateScan());
  EXPECT_CALL(*this, InitiateScanCallback(_));
  EXPECT_CALL(metrics_, NotifyWakeOnWiFiOnDarkResume(
                            WakeOnWiFi::kWakeTriggerUnsupported));
  OnDarkResume(is_connected, whitelist);
  EXPECT_FALSE(DarkResumeActionsTimeOutCallbackIsCancelled());

  StartDHCPLeaseRenewalTimer();
  SetExpectationsDisconnectedBeforeSuspend();
  OnNoAutoConnectableServicesAfterScan(whitelist);
  EXPECT_FALSE(DHCPLeaseRenewalTimerIsRunning());
  VerifyStateDisconnectedBeforeSuspend();
}

TEST_F(WakeOnWiFiTestWithDispatcher,
       OnDarkResume_WakeReasonUnsupported_NotConnected_LeaseObtained) {
  // Test that correct actions are taken if we enter OnDarkResume on an
  // unsupported wake trigger while connected, then connect and obtain a DHCP
  // lease before suspending again.
  const bool is_connected = false;
  const bool have_dhcp_lease = true;
  const uint32_t time_to_next_lease_renewal = 10;
  SetLastWakeReason(WakeOnWiFi::kWakeTriggerUnsupported);
  vector<ByteString> whitelist;
  AddSSIDToWhitelist(kSSIDBytes1, sizeof(kSSIDBytes1), &whitelist);
  InitStateForDarkResume();
  EXPECT_TRUE(DarkResumeActionsTimeOutCallbackIsCancelled());
  // Initiate scan if we are not connected in dark resume.
  EXPECT_CALL(*this, RemoveSupplicantNetworksCallback());
  EXPECT_CALL(metrics_, NotifyDarkResumeInitiateScan());
  EXPECT_CALL(*this, InitiateScanCallback(_));
  EXPECT_CALL(metrics_, NotifyWakeOnWiFiOnDarkResume(
                            WakeOnWiFi::kWakeTriggerUnsupported));
  OnDarkResume(is_connected, whitelist);
  EXPECT_FALSE(DarkResumeActionsTimeOutCallbackIsCancelled());
  // Lease obtained.
  // Since a lease is obtained, we are connected before suspend.
  StopDHCPLeaseRenewalTimer();
  StartWakeToScanTimer();
  SetExpectationsConnectedBeforeSuspend();
  OnConnectedAndReachable(have_dhcp_lease, time_to_next_lease_renewal);
  EXPECT_TRUE(DHCPLeaseRenewalTimerIsRunning());
  EXPECT_FALSE(WakeToScanTimerIsRunning());
  VerifyStateConnectedBeforeSuspend();
}

TEST_F(WakeOnWiFiTestWithDispatcher, OnDarkResume_WakeReasonPattern) {
  // Test that correct actions are taken if we enter dark resume because the
  // system woke on a packet pattern match. We assume that we wake connected and
  // and go back to sleep connected if we wake on pattern.
  const bool is_connected = true;
  SetLastWakeReason(WakeOnWiFi::kWakeTriggerPattern);
  vector<ByteString> whitelist;
  AddSSIDToWhitelist(kSSIDBytes1, sizeof(kSSIDBytes1), &whitelist);

  InitStateForDarkResume();
  EXPECT_TRUE(DarkResumeActionsTimeOutCallbackIsCancelled());
  EXPECT_CALL(metrics_,
              NotifyWakeOnWiFiOnDarkResume(WakeOnWiFi::kWakeTriggerPattern));
  OnDarkResume(is_connected, whitelist);
  EXPECT_FALSE(DarkResumeActionsTimeOutCallbackIsCancelled());

  StartWakeToScanTimer();
  SetExpectationsConnectedBeforeSuspend();
  dispatcher_.DispatchPendingEvents();
  EXPECT_EQ(WakeOnWiFi::kWakeTriggerUnsupported, GetLastWakeReason());
  EXPECT_FALSE(WakeToScanTimerIsRunning());
  VerifyStateConnectedBeforeSuspend();
}

TEST_F(WakeOnWiFiTestWithDispatcher,
       OnDarkResume_WakeReasonDisconnect_NoAutoconnectableServices) {
  // Test that correct actions are taken if we enter dark resume because the
  // system woke on a disconnect, and go back to suspend because we could not
  // find any networks available for autoconnect.
  const bool is_connected = false;
  SetLastWakeReason(WakeOnWiFi::kWakeTriggerDisconnect);
  vector<ByteString> whitelist;
  AddSSIDToWhitelist(kSSIDBytes1, sizeof(kSSIDBytes1), &whitelist);

  InitStateForDarkResume();
  EXPECT_TRUE(DarkResumeActionsTimeOutCallbackIsCancelled());
  EXPECT_CALL(*this, RemoveSupplicantNetworksCallback());
  EXPECT_CALL(metrics_, NotifyDarkResumeInitiateScan());
  EXPECT_CALL(*this, InitiateScanCallback(_));
  EXPECT_CALL(metrics_,
              NotifyWakeOnWiFiOnDarkResume(WakeOnWiFi::kWakeTriggerDisconnect));
  OnDarkResume(is_connected, whitelist);
  EXPECT_FALSE(DarkResumeActionsTimeOutCallbackIsCancelled());

  StartDHCPLeaseRenewalTimer();
  SetExpectationsDisconnectedBeforeSuspend();
  OnNoAutoConnectableServicesAfterScan(whitelist);
  EXPECT_EQ(WakeOnWiFi::kWakeTriggerUnsupported, GetLastWakeReason());
  EXPECT_FALSE(DHCPLeaseRenewalTimerIsRunning());
  VerifyStateDisconnectedBeforeSuspend();
}

TEST_F(WakeOnWiFiTestWithDispatcher,
       OnDarkResume_WakeReasonDisconnect_Timeout) {
  // Test that correct actions are taken if we enter dark resume because the
  // system woke on a disconnect, then timeout on suspend actions before
  // suspending again.
  const bool is_connected = false;
  SetLastWakeReason(WakeOnWiFi::kWakeTriggerDisconnect);
  vector<ByteString> whitelist;
  AddSSIDToWhitelist(kSSIDBytes1, sizeof(kSSIDBytes1), &whitelist);

  InitStateForDarkResume();
  EXPECT_TRUE(DarkResumeActionsTimeOutCallbackIsCancelled());
  EXPECT_CALL(*this, RemoveSupplicantNetworksCallback());
  EXPECT_CALL(metrics_, NotifyDarkResumeInitiateScan());
  EXPECT_CALL(*this, InitiateScanCallback(_));
  EXPECT_CALL(metrics_,
              NotifyWakeOnWiFiOnDarkResume(WakeOnWiFi::kWakeTriggerDisconnect));
  OnDarkResume(is_connected, whitelist);
  EXPECT_FALSE(DarkResumeActionsTimeOutCallbackIsCancelled());

  StartDHCPLeaseRenewalTimer();
  SetExpectationsDisconnectedBeforeSuspend();
  dispatcher_.DispatchPendingEvents();
  EXPECT_EQ(WakeOnWiFi::kWakeTriggerUnsupported, GetLastWakeReason());
  EXPECT_FALSE(DHCPLeaseRenewalTimerIsRunning());
  VerifyStateDisconnectedBeforeSuspend();
}

TEST_F(WakeOnWiFiTestWithDispatcher,
       OnDarkResume_WakeReasonDisconnect_LeaseObtained) {
  // Test that correct actions are taken if we enter dark resume because the
  // system woke on a disconnect, then connect and obtain a DHCP lease before
  // suspending again.
  const bool is_connected = false;
  const bool have_dhcp_lease = true;
  const uint32_t time_to_next_lease_renewal = 10;
  SetLastWakeReason(WakeOnWiFi::kWakeTriggerDisconnect);
  vector<ByteString> whitelist;
  AddSSIDToWhitelist(kSSIDBytes1, sizeof(kSSIDBytes1), &whitelist);

  InitStateForDarkResume();
  EXPECT_TRUE(DarkResumeActionsTimeOutCallbackIsCancelled());
  EXPECT_CALL(*this, RemoveSupplicantNetworksCallback());
  EXPECT_CALL(metrics_, NotifyDarkResumeInitiateScan());
  EXPECT_CALL(*this, InitiateScanCallback(_));
  EXPECT_CALL(metrics_,
              NotifyWakeOnWiFiOnDarkResume(WakeOnWiFi::kWakeTriggerDisconnect));
  OnDarkResume(is_connected, whitelist);
  EXPECT_FALSE(DarkResumeActionsTimeOutCallbackIsCancelled());

  StopDHCPLeaseRenewalTimer();
  StartWakeToScanTimer();
  SetExpectationsConnectedBeforeSuspend();
  OnConnectedAndReachable(have_dhcp_lease, time_to_next_lease_renewal);
  EXPECT_EQ(WakeOnWiFi::kWakeTriggerUnsupported, GetLastWakeReason());
  EXPECT_TRUE(DHCPLeaseRenewalTimerIsRunning());
  EXPECT_FALSE(WakeToScanTimerIsRunning());
  VerifyStateConnectedBeforeSuspend();
}

TEST_F(WakeOnWiFiTestWithDispatcher,
       OnDarkResume_WakeReasonSSID_NoAutoconnectableServices) {
  // Test that correct actions are taken if we enter dark resume because the
  // system woke on SSID, and go back to suspend because we could not find any
  // networks available for autoconnect.
  const bool is_connected = false;
  SetLastWakeReason(WakeOnWiFi::kWakeTriggerSSID);
  vector<ByteString> whitelist;
  AddSSIDToWhitelist(kSSIDBytes1, sizeof(kSSIDBytes1), &whitelist);

  InitStateForDarkResume();
  EXPECT_TRUE(DarkResumeActionsTimeOutCallbackIsCancelled());
  EXPECT_CALL(*this, RemoveSupplicantNetworksCallback());
  EXPECT_CALL(metrics_, NotifyDarkResumeInitiateScan());
  EXPECT_CALL(*this, InitiateScanCallback(_));
  EXPECT_CALL(metrics_,
              NotifyWakeOnWiFiOnDarkResume(WakeOnWiFi::kWakeTriggerSSID));
  OnDarkResume(is_connected, whitelist);
  EXPECT_FALSE(DarkResumeActionsTimeOutCallbackIsCancelled());

  StartDHCPLeaseRenewalTimer();
  SetExpectationsDisconnectedBeforeSuspend();
  OnNoAutoConnectableServicesAfterScan(whitelist);
  EXPECT_EQ(WakeOnWiFi::kWakeTriggerUnsupported, GetLastWakeReason());
  EXPECT_FALSE(DHCPLeaseRenewalTimerIsRunning());
  VerifyStateDisconnectedBeforeSuspend();
}

TEST_F(WakeOnWiFiTestWithDispatcher, OnDarkResume_WakeReasonSSID_Timeout) {
  // Test that correct actions are taken if we enter dark resume because the
  // system woke on SSID, then timeout on suspend actions before suspending
  // again.
  const bool is_connected = false;
  SetLastWakeReason(WakeOnWiFi::kWakeTriggerSSID);
  vector<ByteString> whitelist;
  AddSSIDToWhitelist(kSSIDBytes1, sizeof(kSSIDBytes1), &whitelist);

  InitStateForDarkResume();
  EXPECT_TRUE(DarkResumeActionsTimeOutCallbackIsCancelled());
  EXPECT_CALL(*this, RemoveSupplicantNetworksCallback());
  EXPECT_CALL(metrics_, NotifyDarkResumeInitiateScan());
  EXPECT_CALL(*this, InitiateScanCallback(GetLastSSIDMatchFreqs()));
  EXPECT_CALL(metrics_,
              NotifyWakeOnWiFiOnDarkResume(WakeOnWiFi::kWakeTriggerSSID));
  OnDarkResume(is_connected, whitelist);
  EXPECT_FALSE(DarkResumeActionsTimeOutCallbackIsCancelled());

  StartDHCPLeaseRenewalTimer();
  SetExpectationsDisconnectedBeforeSuspend();
  dispatcher_.DispatchPendingEvents();
  EXPECT_EQ(WakeOnWiFi::kWakeTriggerUnsupported, GetLastWakeReason());
  EXPECT_FALSE(DHCPLeaseRenewalTimerIsRunning());
  VerifyStateDisconnectedBeforeSuspend();
}

TEST_F(WakeOnWiFiTestWithDispatcher,
       OnDarkResume_WakeReasonSSID_LeaseObtained) {
  // Test that correct actions are taken if we enter dark resume because the
  // system woke on SSID, then connect and obtain a DHCP lease before suspending
  // again.
  const bool is_connected = false;
  const bool have_dhcp_lease = true;
  const uint32_t time_to_next_lease_renewal = 10;
  SetLastWakeReason(WakeOnWiFi::kWakeTriggerSSID);
  vector<ByteString> whitelist;
  AddSSIDToWhitelist(kSSIDBytes1, sizeof(kSSIDBytes1), &whitelist);

  InitStateForDarkResume();
  EXPECT_TRUE(DarkResumeActionsTimeOutCallbackIsCancelled());
  EXPECT_CALL(*this, RemoveSupplicantNetworksCallback());
  EXPECT_CALL(metrics_, NotifyDarkResumeInitiateScan());
  EXPECT_CALL(*this, InitiateScanCallback(GetLastSSIDMatchFreqs()));
  EXPECT_CALL(metrics_,
              NotifyWakeOnWiFiOnDarkResume(WakeOnWiFi::kWakeTriggerSSID));
  OnDarkResume(is_connected, whitelist);
  EXPECT_FALSE(DarkResumeActionsTimeOutCallbackIsCancelled());

  StopDHCPLeaseRenewalTimer();
  StartWakeToScanTimer();
  SetExpectationsConnectedBeforeSuspend();
  OnConnectedAndReachable(have_dhcp_lease, time_to_next_lease_renewal);
  EXPECT_EQ(WakeOnWiFi::kWakeTriggerUnsupported, GetLastWakeReason());
  EXPECT_TRUE(DHCPLeaseRenewalTimerIsRunning());
  EXPECT_FALSE(WakeToScanTimerIsRunning());
  VerifyStateConnectedBeforeSuspend();
}

TEST_F(WakeOnWiFiTestWithDispatcher, OnDarkResume_Connected_DoNotRecordEvent) {
  const bool is_connected = true;
  vector<ByteString> whitelist;
  EXPECT_TRUE(GetDarkResumeHistory()->Empty());
  OnDarkResume(is_connected, whitelist);
  EXPECT_TRUE(GetDarkResumeHistory()->Empty());
}

TEST_F(WakeOnWiFiTestWithDispatcher, OnDarkResume_NotConnected_RecordEvent) {
  const bool is_connected = false;
  vector<ByteString> whitelist;
  EXPECT_TRUE(GetDarkResumeHistory()->Empty());
  OnDarkResume(is_connected, whitelist);
  EXPECT_EQ(1, GetDarkResumeHistory()->Size());
}

TEST_F(WakeOnWiFiTestWithDispatcher,
       OnDarkResume_NotConnected_MaxDarkResumes_ShortPeriod) {
  // These 3 dark resume timings are within a 1 minute interval, so as to
  // trigger the short throttling threshold (3 in 1 minute).
  const int kTimeSeconds[] = {10, 20, 30};
  CHECK_EQ(static_cast<const unsigned int>(
               WakeOnWiFi::kMaxDarkResumesPerPeriodShort),
           arraysize(kTimeSeconds));
  vector<ByteString> whitelist;

  // This test assumes that throttling takes place when 3 dark resumes have
  // been triggered in the last 1 minute.
  EXPECT_EQ(3, WakeOnWiFi::kMaxDarkResumesPerPeriodShort);
  EXPECT_EQ(1, WakeOnWiFi::kDarkResumeFrequencySamplingPeriodShortMinutes);

  // Wake on SSID dark resumes should be recorded in the dark resume history.
  const bool is_connected = false;
  SetLastWakeReason(WakeOnWiFi::kWakeTriggerSSID);
  EXPECT_TRUE(GetDarkResumeHistory()->Empty());

  // First two dark resumes take place at 10 and 20 seconds respectively. This
  // is still within the throttling threshold.
  for (int i = 0; i < WakeOnWiFi::kMaxDarkResumesPerPeriodShort - 1; ++i) {
    EXPECT_CALL(metrics_, NotifyWakeOnWiFiThrottled()).Times(0);
    EXPECT_CALL(time_, GetNow())
        .WillRepeatedly(Return(GetTimestampBootTime(kTimeSeconds[i])));
    OnDarkResume(is_connected, whitelist);
  }
  SetInDarkResume(false);  // this happens after BeforeSuspendActions
  EXPECT_EQ(WakeOnWiFi::kMaxDarkResumesPerPeriodShort - 1,
            GetDarkResumeHistory()->Size());

  // The 3rd dark resume takes place at 30 seconds, which makes 3 dark resumes
  // in the past minute. Disable wake on WiFi and start wake to scan timer.
  ResetSuspendActionsDoneCallback();
  StartDHCPLeaseRenewalTimer();
  StopWakeToScanTimer();
  EXPECT_TRUE(SuspendActionsCallbackIsNull());
  EXPECT_TRUE(DHCPLeaseRenewalTimerIsRunning());
  EXPECT_FALSE(WakeToScanTimerIsRunning());
  EXPECT_FALSE(GetDarkResumeHistory()->Empty());
  EXPECT_CALL(metrics_, NotifyWakeOnWiFiThrottled());
  EXPECT_CALL(time_, GetNow())
      .WillRepeatedly(Return(GetTimestampBootTime(
          kTimeSeconds[WakeOnWiFi::kMaxDarkResumesPerPeriodShort - 1])));
  OnDarkResume(is_connected, whitelist);
  EXPECT_FALSE(SuspendActionsCallbackIsNull());
  EXPECT_FALSE(DHCPLeaseRenewalTimerIsRunning());
  EXPECT_TRUE(WakeToScanTimerIsRunning());
  EXPECT_TRUE(GetDarkResumeHistory()->Empty());
  EXPECT_FALSE(GetInDarkResume());
}

TEST_F(WakeOnWiFiTestWithDispatcher,
       OnDarkResume_NotConnected_MaxDarkResumes_LongPeriod) {
  // These 10 dark resume timings are spaced 1 minute apart so as to trigger the
  // long throttling threshold (10 in 10 minute) without triggering the short
  // throttling threshold (3 in 1 minute).
  const int kTimeSeconds[] = {10, 70, 130, 190, 250, 310, 370, 430, 490, 550};
  CHECK_EQ(
      static_cast<const unsigned int>(WakeOnWiFi::kMaxDarkResumesPerPeriodLong),
      arraysize(kTimeSeconds));
  vector<ByteString> whitelist;

  // This test assumes that throttling takes place when 3 dark resumes have been
  // triggered in the last 1 minute, or when 10 dark resumes have been triggered
  // in the last 10 minutes.
  EXPECT_EQ(3, WakeOnWiFi::kMaxDarkResumesPerPeriodShort);
  EXPECT_EQ(1, WakeOnWiFi::kDarkResumeFrequencySamplingPeriodShortMinutes);
  EXPECT_EQ(10, WakeOnWiFi::kMaxDarkResumesPerPeriodLong);
  EXPECT_EQ(10, WakeOnWiFi::kDarkResumeFrequencySamplingPeriodLongMinutes);

  // Wake on SSID dark resumes should be recorded in the dark resume history.
  const bool is_connected = false;
  SetLastWakeReason(WakeOnWiFi::kWakeTriggerSSID);
  EXPECT_TRUE(GetDarkResumeHistory()->Empty());

  // The first 9 dark resumes happen once per minute. This is still within the
  // throttling threshold.
  for (int i = 0; i < WakeOnWiFi::kMaxDarkResumesPerPeriodLong - 1; ++i) {
    EXPECT_CALL(metrics_, NotifyWakeOnWiFiThrottled()).Times(0);
    EXPECT_CALL(time_, GetNow())
        .WillRepeatedly(Return(GetTimestampBootTime(kTimeSeconds[i])));
    OnDarkResume(is_connected, whitelist);
  }
  SetInDarkResume(false);  // this happens after BeforeSuspendActions
  EXPECT_EQ(WakeOnWiFi::kMaxDarkResumesPerPeriodLong - 1,
            GetDarkResumeHistory()->Size());

  // The occurrence of the 10th dark resume makes 10 dark resumes in the past 10
  // minutes. Disable wake on WiFi and start wake to scan timer.
  ResetSuspendActionsDoneCallback();
  StartDHCPLeaseRenewalTimer();
  StopWakeToScanTimer();
  EXPECT_TRUE(SuspendActionsCallbackIsNull());
  EXPECT_TRUE(DHCPLeaseRenewalTimerIsRunning());
  EXPECT_FALSE(WakeToScanTimerIsRunning());
  EXPECT_FALSE(GetDarkResumeHistory()->Empty());
  EXPECT_CALL(metrics_, NotifyWakeOnWiFiThrottled());
  EXPECT_CALL(time_, GetNow())
      .WillRepeatedly(Return(GetTimestampBootTime(
          kTimeSeconds[WakeOnWiFi::kMaxDarkResumesPerPeriodLong - 1])));
  OnDarkResume(is_connected, whitelist);
  EXPECT_FALSE(SuspendActionsCallbackIsNull());
  EXPECT_FALSE(DHCPLeaseRenewalTimerIsRunning());
  EXPECT_TRUE(WakeToScanTimerIsRunning());
  EXPECT_TRUE(GetDarkResumeHistory()->Empty());
  EXPECT_FALSE(GetInDarkResume());
  EXPECT_TRUE(GetLastSSIDMatchFreqs().empty());
}

TEST_F(WakeOnWiFiTestWithMockDispatcher, OnConnectedAndReachable) {
  const bool start_lease_renewal_timer = true;
  ScopedMockLog log;

  EXPECT_CALL(log, Log(_, _, _)).Times(AnyNumber());
  SetInDarkResume(true);
  ScopeLogger::GetInstance()->EnableScopesByName("wifi");
  ScopeLogger::GetInstance()->set_verbose_level(3);
  EXPECT_CALL(log, Log(_, _, HasSubstr("BeforeSuspendActions")));
  OnConnectedAndReachable(start_lease_renewal_timer,
                          kTimeToNextLeaseRenewalLong);

  SetInDarkResume(false);
  EXPECT_CALL(log, Log(_, _, HasSubstr("Not in dark resume, so do nothing")));
  OnConnectedAndReachable(start_lease_renewal_timer,
                          kTimeToNextLeaseRenewalLong);
  ScopeLogger::GetInstance()->EnableScopesByName("-wifi");
  ScopeLogger::GetInstance()->set_verbose_level(0);
}

TEST_F(WakeOnWiFiTestWithMockDispatcher, WakeOnWiFiDisabledAfterResume) {
  // At least one wake on WiFi trigger supported and Wake on WiFi features
  // are enabled, so disable Wake on WiFi on resume.]
  EnableWakeOnWiFiFeaturesPacketDarkConnect();
  GetWakeOnWiFiTriggers()->insert(WakeOnWiFi::kWakeTriggerPattern);
  EXPECT_CALL(netlink_manager_,
              SendNl80211Message(IsDisableWakeOnWiFiMsg(), _, _, _)).Times(1);
  EXPECT_CALL(metrics_, NotifySuspendWithWakeOnWiFiEnabledDone()).Times(1);
  OnAfterResume();

  // No wake no WiFi triggers supported, so do nothing.
  ClearWakeOnWiFiTriggersSupported();
  EXPECT_CALL(netlink_manager_,
              SendNl80211Message(IsDisableWakeOnWiFiMsg(), _, _, _)).Times(0);
  EXPECT_CALL(metrics_, NotifySuspendWithWakeOnWiFiEnabledDone()).Times(0);
  OnAfterResume();

  // Wake on WiFi features disabled, so do nothing.
  GetWakeOnWiFiTriggersSupported()->insert(WakeOnWiFi::kWakeTriggerPattern);
  DisableWakeOnWiFiFeatures();
  EXPECT_CALL(netlink_manager_,
              SendNl80211Message(IsDisableWakeOnWiFiMsg(), _, _, _)).Times(0);
  EXPECT_CALL(metrics_, NotifySuspendWithWakeOnWiFiEnabledDone()).Times(0);
  OnAfterResume();

  // Both WakeOnWiFi triggers are empty and Wake on WiFi features are disabled,
  // so do nothing.
  ClearWakeOnWiFiTriggersSupported();
  DisableWakeOnWiFiFeatures();
  EXPECT_CALL(netlink_manager_,
              SendNl80211Message(IsDisableWakeOnWiFiMsg(), _, _, _)).Times(0);
  EXPECT_CALL(metrics_, NotifySuspendWithWakeOnWiFiEnabledDone()).Times(0);
  OnAfterResume();
}

TEST_F(WakeOnWiFiTestWithMockDispatcher, SetWakeOnWiFiFeaturesEnabled) {
  const string bad_feature("blahblah");
  Error e;
  EnableWakeOnWiFiFeaturesPacketDarkConnect();
  EXPECT_STREQ(GetWakeOnWiFiFeaturesEnabled().c_str(),
               kWakeOnWiFiFeaturesEnabledPacketDarkConnect);
  EXPECT_FALSE(SetWakeOnWiFiFeaturesEnabled(
      kWakeOnWiFiFeaturesEnabledPacketDarkConnect, &e));
  EXPECT_STREQ(GetWakeOnWiFiFeaturesEnabled().c_str(),
               kWakeOnWiFiFeaturesEnabledPacketDarkConnect);

  EXPECT_FALSE(SetWakeOnWiFiFeaturesEnabled(bad_feature, &e));
  EXPECT_EQ(e.type(), Error::kInvalidArguments);
  EXPECT_STREQ(e.message().c_str(), "Invalid Wake on WiFi feature");
  EXPECT_STREQ(GetWakeOnWiFiFeaturesEnabled().c_str(),
               kWakeOnWiFiFeaturesEnabledPacketDarkConnect);

  EXPECT_TRUE(
      SetWakeOnWiFiFeaturesEnabled(kWakeOnWiFiFeaturesEnabledPacket, &e));
  EXPECT_STREQ(GetWakeOnWiFiFeaturesEnabled().c_str(),
               kWakeOnWiFiFeaturesEnabledPacket);
}

TEST_F(WakeOnWiFiTestWithMockDispatcher,
       ReportConnectedToServiceAfterWake_WakeOnDarkConnectEnabledAndConnected) {
  const bool is_connected = true;
  EnableWakeOnWiFiFeaturesPacketDarkConnect();
  EXPECT_CALL(
      metrics_,
      NotifyConnectedToServiceAfterWake(
          Metrics::kWiFiConnetionStatusAfterWakeOnWiFiEnabledWakeConnected));
  ReportConnectedToServiceAfterWake(is_connected);

  EnableWakeOnWiFiFeaturesDarkConnect();
  EXPECT_CALL(
      metrics_,
      NotifyConnectedToServiceAfterWake(
          Metrics::kWiFiConnetionStatusAfterWakeOnWiFiEnabledWakeConnected));
  ReportConnectedToServiceAfterWake(is_connected);
}

TEST_F(
    WakeOnWiFiTestWithMockDispatcher,
    ReportConnectedToServiceAfterWake_WakeOnDarkConnectEnabledAndNotConnected) {
  const bool is_connected = false;
  EnableWakeOnWiFiFeaturesPacketDarkConnect();
  EXPECT_CALL(
      metrics_,
      NotifyConnectedToServiceAfterWake(
          Metrics::kWiFiConnetionStatusAfterWakeOnWiFiEnabledWakeNotConnected));
  ReportConnectedToServiceAfterWake(is_connected);

  EnableWakeOnWiFiFeaturesDarkConnect();
  EXPECT_CALL(
      metrics_,
      NotifyConnectedToServiceAfterWake(
          Metrics::kWiFiConnetionStatusAfterWakeOnWiFiEnabledWakeNotConnected));
  ReportConnectedToServiceAfterWake(is_connected);
}

TEST_F(
    WakeOnWiFiTestWithMockDispatcher,
    ReportConnectedToServiceAfterWake_WakeOnDarkConnectDisabledAndConnected) {
  const bool is_connected = true;
  EnableWakeOnWiFiFeaturesPacket();
  EXPECT_CALL(
      metrics_,
      NotifyConnectedToServiceAfterWake(
          Metrics::kWiFiConnetionStatusAfterWakeOnWiFiDisabledWakeConnected));
  ReportConnectedToServiceAfterWake(is_connected);

  DisableWakeOnWiFiFeatures();
  EXPECT_CALL(
      metrics_,
      NotifyConnectedToServiceAfterWake(
          Metrics::kWiFiConnetionStatusAfterWakeOnWiFiDisabledWakeConnected));
  ReportConnectedToServiceAfterWake(is_connected);
}

TEST_F(
    WakeOnWiFiTestWithMockDispatcher,
    ReportConnectedToServiceAfterWake_WakeOnDarkConnectDisabledAndNotConnected)
{
  const bool is_connected = false;
  EnableWakeOnWiFiFeaturesPacket();
  EXPECT_CALL(
      metrics_,
      NotifyConnectedToServiceAfterWake(
          Metrics::
              kWiFiConnetionStatusAfterWakeOnWiFiDisabledWakeNotConnected));
  ReportConnectedToServiceAfterWake(is_connected);

  DisableWakeOnWiFiFeatures();
  EXPECT_CALL(
      metrics_,
      NotifyConnectedToServiceAfterWake(
          Metrics::
              kWiFiConnetionStatusAfterWakeOnWiFiDisabledWakeNotConnected));
  ReportConnectedToServiceAfterWake(is_connected);
}

TEST_F(WakeOnWiFiTestWithMockDispatcher,
       OnNoAutoConnectableServicesAfterScan_InDarkResume) {
  vector<ByteString> whitelist;
  AddSSIDToWhitelist(kSSIDBytes1, sizeof(kSSIDBytes1), &whitelist);
  EnableWakeOnWiFiFeaturesDarkConnect();
  SetInDarkResume(true);

  // Perform disconnect before suspend actions if we are in dark resume.
  GetWakeOnWiFiTriggers()->clear();
  StartDHCPLeaseRenewalTimer();
  StopWakeToScanTimer();
  OnNoAutoConnectableServicesAfterScan(whitelist);
  EXPECT_FALSE(DHCPLeaseRenewalTimerIsRunning());
  EXPECT_FALSE(WakeToScanTimerIsRunning());
  EXPECT_EQ(GetWakeOnWiFiTriggers()->size(), 1);
  EXPECT_TRUE(GetWakeOnWiFiTriggers()->find(WakeOnWiFi::kWakeTriggerSSID) !=
              GetWakeOnWiFiTriggers()->end());
}

TEST_F(WakeOnWiFiTestWithMockDispatcher,
       OnNoAutoConnectableServicesAfterScan_NotInDarkResume) {
  vector<ByteString> whitelist;
  AddSSIDToWhitelist(kSSIDBytes1, sizeof(kSSIDBytes1), &whitelist);
  EnableWakeOnWiFiFeaturesDarkConnect();
  SetInDarkResume(false);

  // If we are not in dark resume, do nothing.
  GetWakeOnWiFiTriggers()->clear();
  StartDHCPLeaseRenewalTimer();
  StopWakeToScanTimer();
  OnNoAutoConnectableServicesAfterScan(whitelist);
  EXPECT_TRUE(DHCPLeaseRenewalTimerIsRunning());
  EXPECT_EQ(GetWakeOnWiFiTriggers()->size(), 0);
}

TEST_F(WakeOnWiFiTestWithMockDispatcher,
       OnNoAutoConnectableServicesAfterScan_Retry) {
  vector<ByteString> whitelist;
  AddSSIDToWhitelist(kSSIDBytes1, sizeof(kSSIDBytes1), &whitelist);
  EnableWakeOnWiFiFeaturesDarkConnect();
  SetInDarkResume(true);
  SetDarkResumeScanRetriesLeft(1);

  // Perform a retry.
  EXPECT_EQ(1, GetDarkResumeScanRetriesLeft());
  EXPECT_CALL(metrics_, NotifyDarkResumeScanRetry());
  EXPECT_CALL(*this, InitiateScanCallback(GetLastSSIDMatchFreqs()));
  OnNoAutoConnectableServicesAfterScan(whitelist);
  EXPECT_EQ(0, GetDarkResumeScanRetriesLeft());

  // Still no auto-connectable services after retry. No more retries, so perform
  // disconnect before suspend actions.
  GetWakeOnWiFiTriggers()->clear();
  StartDHCPLeaseRenewalTimer();
  StopWakeToScanTimer();
  EXPECT_CALL(*this, InitiateScanCallback(GetLastSSIDMatchFreqs())).Times(0);
  OnNoAutoConnectableServicesAfterScan(whitelist);
  EXPECT_FALSE(DHCPLeaseRenewalTimerIsRunning());
  EXPECT_FALSE(WakeToScanTimerIsRunning());
  EXPECT_EQ(GetWakeOnWiFiTriggers()->size(), 1);
  EXPECT_TRUE(GetWakeOnWiFiTriggers()->find(WakeOnWiFi::kWakeTriggerSSID) !=
              GetWakeOnWiFiTriggers()->end());
}

TEST_F(WakeOnWiFiTestWithMockDispatcher, OnWakeupReasonReceived_Unsupported) {
  ScopedMockLog log;
  ScopeLogger::GetInstance()->EnableScopesByName("wifi");
  ScopeLogger::GetInstance()->set_verbose_level(3);
  SetWiphyIndex(kWakeReasonNlMsg_WiphyIndex);

  SetWakeOnPacketConnMessage msg;
  NetlinkPacket packet(
      kWakeReasonUnsupportedNlMsg, sizeof(kWakeReasonUnsupportedNlMsg));
  msg.InitFromPacket(&packet, GetWakeupReportMsgContext());
  EXPECT_CALL(log, Log(_, _, _)).Times(AnyNumber());
  EXPECT_CALL(log,
              Log(_, _, HasSubstr("Wakeup reason: Not wake on WiFi related")));
  EXPECT_CALL(metrics_, NotifyWakeupReasonReceived());
  EXPECT_CALL(*this, RecordDarkResumeWakeReasonCallback(_)).Times(0);
  OnWakeupReasonReceived(msg);
  EXPECT_EQ(WakeOnWiFi::kWakeTriggerUnsupported, GetLastWakeReason());

  ScopeLogger::GetInstance()->EnableScopesByName("-wifi");
  ScopeLogger::GetInstance()->set_verbose_level(0);
}

TEST_F(WakeOnWiFiTestWithMockDispatcher, OnWakeupReasonReceived_Disconnect) {
  ScopedMockLog log;
  ScopeLogger::GetInstance()->EnableScopesByName("wifi");
  ScopeLogger::GetInstance()->set_verbose_level(3);
  SetWiphyIndex(kWakeReasonNlMsg_WiphyIndex);

  SetWakeOnPacketConnMessage msg;
  NetlinkPacket packet(
      kWakeReasonDisconnectNlMsg, sizeof(kWakeReasonDisconnectNlMsg));
  msg.InitFromPacket(&packet, GetWakeupReportMsgContext());
  EXPECT_CALL(log, Log(_, _, _)).Times(AnyNumber());
  EXPECT_CALL(log, Log(_, _, HasSubstr("Wakeup reason: Disconnect")));
  EXPECT_CALL(metrics_, NotifyWakeupReasonReceived());
  EXPECT_CALL(*this, RecordDarkResumeWakeReasonCallback(
                         WakeOnWiFi::kWakeReasonStringDisconnect));
  OnWakeupReasonReceived(msg);
  EXPECT_EQ(WakeOnWiFi::kWakeTriggerDisconnect, GetLastWakeReason());

  ScopeLogger::GetInstance()->EnableScopesByName("-wifi");
  ScopeLogger::GetInstance()->set_verbose_level(0);
}

TEST_F(WakeOnWiFiTestWithMockDispatcher, OnWakeupReasonReceived_SSID) {
  ScopedMockLog log;
  ScopeLogger::GetInstance()->EnableScopesByName("wifi");
  ScopeLogger::GetInstance()->set_verbose_level(3);
  SetWiphyIndex(kWakeReasonNlMsg_WiphyIndex);

  SetWakeOnPacketConnMessage msg;
  NetlinkPacket packet(kWakeReasonSSIDNlMsg, sizeof(kWakeReasonSSIDNlMsg));
  msg.InitFromPacket(&packet, GetWakeupReportMsgContext());
  EXPECT_CALL(log, Log(_, _, _)).Times(AnyNumber());
  EXPECT_CALL(log, Log(_, _, HasSubstr("Wakeup reason: SSID")));
  EXPECT_CALL(metrics_, NotifyWakeupReasonReceived());
  EXPECT_CALL(*this, RecordDarkResumeWakeReasonCallback(
                         WakeOnWiFi::kWakeReasonStringSSID));
  OnWakeupReasonReceived(msg);
  EXPECT_EQ(WakeOnWiFi::kWakeTriggerSSID, GetLastWakeReason());
  EXPECT_EQ(arraysize(kSSID1FreqMatches), GetLastSSIDMatchFreqs().size());
  for (uint32_t freq : kSSID1FreqMatches) {
    EXPECT_TRUE(GetLastSSIDMatchFreqs().find(freq) !=
                GetLastSSIDMatchFreqs().end());
  }

  ScopeLogger::GetInstance()->EnableScopesByName("-wifi");
  ScopeLogger::GetInstance()->set_verbose_level(0);
}

TEST_F(WakeOnWiFiTestWithMockDispatcher, OnWakeupReasonReceived_Pattern) {
  ScopedMockLog log;
  ScopeLogger::GetInstance()->EnableScopesByName("wifi");
  ScopeLogger::GetInstance()->set_verbose_level(3);
  SetWiphyIndex(kWakeReasonNlMsg_WiphyIndex);

  SetWakeOnPacketConnMessage msg;
  NetlinkPacket packet(
      kWakeReasonPatternNlMsg, sizeof(kWakeReasonPatternNlMsg));
  msg.InitFromPacket(&packet, GetWakeupReportMsgContext());
  EXPECT_CALL(log, Log(_, _, _)).Times(AnyNumber());
  EXPECT_CALL(log, Log(_, _, HasSubstr("Wakeup reason: Pattern " +
                                       kWakeReasonPatternNlMsg_PattIndex)));
  EXPECT_CALL(metrics_, NotifyWakeupReasonReceived());
  EXPECT_CALL(*this, RecordDarkResumeWakeReasonCallback(
                         WakeOnWiFi::kWakeReasonStringPattern));
  OnWakeupReasonReceived(msg);
  EXPECT_EQ(WakeOnWiFi::kWakeTriggerPattern, GetLastWakeReason());

  ScopeLogger::GetInstance()->EnableScopesByName("-wifi");
  ScopeLogger::GetInstance()->set_verbose_level(0);
}

TEST_F(WakeOnWiFiTestWithMockDispatcher, OnWakeupReasonReceived_Error) {
  ScopedMockLog log;
  ScopeLogger::GetInstance()->EnableScopesByName("wifi");
  ScopeLogger::GetInstance()->set_verbose_level(7);
  SetWiphyIndex(kWakeReasonNlMsg_WiphyIndex);

  // kWrongMessageTypeNlMsg has an nlmsg_type of 0x16, which is different from
  // the 0x13 (i.e. kNl80211FamilyId) that we expect in these unittests.
  GetWakeOnPacketConnMessage msg0;
  NetlinkPacket packet0(kWrongMessageTypeNlMsg, sizeof(kWrongMessageTypeNlMsg));
  msg0.InitFromPacket(&packet0, GetWakeupReportMsgContext());
  EXPECT_CALL(log, Log(_, _, _)).Times(AnyNumber());
  EXPECT_CALL(log, Log(_, _, HasSubstr("Not a NL80211 Message")));
  EXPECT_CALL(metrics_, NotifyWakeupReasonReceived()).Times(0);
  EXPECT_CALL(*this, RecordDarkResumeWakeReasonCallback(_)).Times(0);
  OnWakeupReasonReceived(msg0);
  EXPECT_EQ(WakeOnWiFi::kWakeTriggerUnsupported, GetLastWakeReason());

  // This message has command NL80211_CMD_GET_WOWLAN, not a
  // NL80211_CMD_SET_WOWLAN.
  GetWakeOnPacketConnMessage msg1;
  NetlinkPacket packet1(kResponseNoIPAddresses, sizeof(kResponseNoIPAddresses));
  msg1.InitFromPacket(&packet1, GetWakeupReportMsgContext());
  EXPECT_CALL(log, Log(_, _, _)).Times(AnyNumber());
  EXPECT_CALL(log,
              Log(_, _, HasSubstr("Not a NL80211_CMD_SET_WOWLAN message")));
  EXPECT_CALL(metrics_, NotifyWakeupReasonReceived()).Times(0);
  EXPECT_CALL(*this, RecordDarkResumeWakeReasonCallback(_)).Times(0);
  OnWakeupReasonReceived(msg1);
  EXPECT_EQ(WakeOnWiFi::kWakeTriggerUnsupported, GetLastWakeReason());

  // Valid message, but wrong wiphy index.
  SetWiphyIndex(kWakeReasonNlMsg_WiphyIndex + 1);
  SetWakeOnPacketConnMessage msg2;
  NetlinkPacket packet(
      kWakeReasonDisconnectNlMsg, sizeof(kWakeReasonDisconnectNlMsg));
  msg2.InitFromPacket(&packet, GetWakeupReportMsgContext());
  EXPECT_CALL(log, Log(_, _, _)).Times(AnyNumber());
  EXPECT_CALL(
      log, Log(_, _, HasSubstr("Wakeup reason not meant for this interface")));
  EXPECT_CALL(metrics_, NotifyWakeupReasonReceived()).Times(0);
  EXPECT_CALL(*this, RecordDarkResumeWakeReasonCallback(_)).Times(0);
  OnWakeupReasonReceived(msg2);
  EXPECT_EQ(WakeOnWiFi::kWakeTriggerUnsupported, GetLastWakeReason());

  ScopeLogger::GetInstance()->EnableScopesByName("-wifi");
  ScopeLogger::GetInstance()->set_verbose_level(0);
}

#else  // DISABLE_WAKE_ON_WIFI

TEST_F(WakeOnWiFiTestWithMockDispatcher,
       WakeOnWiFiDisabled_AddWakeOnPacketConnection_ReturnsError) {
  DisableWakeOnWiFiFeatures();
  Error e;
  AddWakeOnPacketConnection("1.1.1.1", &e);
  EXPECT_EQ(e.type(), Error::kNotSupported);
  EXPECT_STREQ(e.message().c_str(), WakeOnWiFi::kWakeOnWiFiNotSupported);
}

TEST_F(WakeOnWiFiTestWithMockDispatcher,
       WakeOnWiFiDisabled_RemoveWakeOnPacketConnection_ReturnsError) {
  DisableWakeOnWiFiFeatures();
  Error e;
  RemoveWakeOnPacketConnection("1.1.1.1", &e);
  EXPECT_EQ(e.type(), Error::kNotSupported);
  EXPECT_STREQ(e.message().c_str(), WakeOnWiFi::kWakeOnWiFiNotSupported);
}

TEST_F(WakeOnWiFiTestWithMockDispatcher,
       WakeOnWiFiDisabled_RemoveAllWakeOnPacketConnections_ReturnsError) {
  DisableWakeOnWiFiFeatures();
  Error e;
  RemoveAllWakeOnPacketConnections(&e);
  EXPECT_EQ(e.type(), Error::kNotSupported);
  EXPECT_STREQ(e.message().c_str(), WakeOnWiFi::kWakeOnWiFiNotSupported);
}

TEST_F(WakeOnWiFiTestWithMockDispatcher,
       WakeOnWiFiDisabled_OnBeforeSuspend_ReportsDoneImmediately) {
  const bool is_connected = true;
  const bool have_dhcp_lease = true;
  vector<ByteString> whitelist;
  AddSSIDToWhitelist(kSSIDBytes1, sizeof(kSSIDBytes1), &whitelist);
  EXPECT_CALL(*this, DoneCallback(ErrorTypeIs(Error::kSuccess))).Times(1);
  EXPECT_CALL(*this, RenewDHCPLeaseCallback()).Times(0);
  OnBeforeSuspend(is_connected, whitelist, have_dhcp_lease,
                  kTimeToNextLeaseRenewalShort);

  EXPECT_CALL(*this, DoneCallback(ErrorTypeIs(Error::kSuccess))).Times(1);
  EXPECT_CALL(*this, RenewDHCPLeaseCallback()).Times(0);
  OnBeforeSuspend(is_connected, whitelist, have_dhcp_lease,
                  kTimeToNextLeaseRenewalLong);
}

TEST_F(WakeOnWiFiTestWithMockDispatcher,
       WakeOnWiFiDisabled_OnDarkResume_ReportsDoneImmediately) {
  const bool is_connected = true;
  vector<ByteString> whitelist;
  AddSSIDToWhitelist(kSSIDBytes1, sizeof(kSSIDBytes1), &whitelist);
  EXPECT_CALL(*this, DoneCallback(ErrorTypeIs(Error::kSuccess))).Times(1);
  EXPECT_CALL(mock_dispatcher_, PostDelayedTask(_, _)).Times(0);
  EXPECT_CALL(metrics_, NotifyWakeOnWiFiOnDarkResume(_)).Times(0);
  OnDarkResume(is_connected, whitelist);

  EXPECT_CALL(*this, DoneCallback(ErrorTypeIs(Error::kSuccess))).Times(1);
  EXPECT_CALL(mock_dispatcher_, PostDelayedTask(_, _)).Times(0);
  EXPECT_CALL(metrics_, NotifyWakeOnWiFiOnDarkResume(_)).Times(0);
  OnDarkResume(is_connected, whitelist);
}

TEST_F(WakeOnWiFiTestWithMockDispatcher,
       WakeOnWiFiDisabled_OnAfterResume_DoesNothing) {
  DisableWakeOnWiFiFeatures();
  EXPECT_CALL(netlink_manager_, SendNl80211Message(_, _, _, _)).Times(0);
  EXPECT_CALL(metrics_, NotifySuspendWithWakeOnWiFiEnabledDone()).Times(0);
  OnAfterResume();
}

TEST_F(WakeOnWiFiTestWithMockDispatcher,
       WakeOnWiFiDisabled_SetWakeOnWiFiFeaturesEnabled) {
  Error e;
  SetWakeOnWiFiFeaturesNotSupported();
  EXPECT_STREQ(GetWakeOnWiFiFeaturesEnabled().c_str(),
               kWakeOnWiFiFeaturesEnabledNotSupported);
  EXPECT_FALSE(
      SetWakeOnWiFiFeaturesEnabled(kWakeOnWiFiFeaturesEnabledNotSupported, &e));
  EXPECT_STREQ(GetWakeOnWiFiFeaturesEnabled().c_str(),
               kWakeOnWiFiFeaturesEnabledNotSupported);
  EXPECT_EQ(e.type(), Error::kNotSupported);
  EXPECT_STREQ(e.message().c_str(), WakeOnWiFi::kWakeOnWiFiNotSupported);

  EXPECT_FALSE(
      SetWakeOnWiFiFeaturesEnabled(kWakeOnWiFiFeaturesEnabledPacket, &e));
  EXPECT_STREQ(GetWakeOnWiFiFeaturesEnabled().c_str(),
               kWakeOnWiFiFeaturesEnabledNotSupported);
  EXPECT_EQ(e.type(), Error::kNotSupported);
  EXPECT_STREQ(e.message().c_str(), WakeOnWiFi::kWakeOnWiFiNotSupported);
}

TEST_F(WakeOnWiFiTestWithMockDispatcher,
       WakeOnWiFiDisabled_OnConnectedAndReachable) {
  ScopedMockLog log;
  const bool start_lease_renewal_timer = true;
  ScopeLogger::GetInstance()->EnableScopesByName("wifi");
  ScopeLogger::GetInstance()->set_verbose_level(3);

  EXPECT_CALL(log, Log(_, _, _)).Times(AnyNumber());
  SetInDarkResume(true);
  EXPECT_CALL(
      log, Log(_, _, HasSubstr("Wake on WiFi not supported, so do nothing")));
  OnConnectedAndReachable(start_lease_renewal_timer,
                          kTimeToNextLeaseRenewalLong);

  SetInDarkResume(false);
  EXPECT_CALL(log, Log(_, _, HasSubstr("Not in dark resume, so do nothing")));
  OnConnectedAndReachable(start_lease_renewal_timer,
                          kTimeToNextLeaseRenewalLong);
  ScopeLogger::GetInstance()->EnableScopesByName("-wifi");
  ScopeLogger::GetInstance()->set_verbose_level(0);
}

TEST_F(WakeOnWiFiTestWithMockDispatcher,
       WakeOnWiFiDisabled_ReportConnectedToServiceAfterWakeAndConnected) {
  const bool is_connected = true;
  EXPECT_CALL(
      metrics_,
      NotifyConnectedToServiceAfterWake(
          Metrics::kWiFiConnetionStatusAfterWakeOnWiFiDisabledWakeConnected));
  ReportConnectedToServiceAfterWake(is_connected);
}

TEST_F(WakeOnWiFiTestWithMockDispatcher,
       WakeOnWiFiDisabled_ReportConnectedToServiceAfterWakeAndNotConnected) {
  const bool is_connected = false;
  EXPECT_CALL(
      metrics_,
      NotifyConnectedToServiceAfterWake(
          Metrics::
              kWiFiConnetionStatusAfterWakeOnWiFiDisabledWakeNotConnected));
  ReportConnectedToServiceAfterWake(is_connected);
}

TEST_F(WakeOnWiFiTestWithMockDispatcher,
       WakeOnWiFiDisabled_OnNoAutoConnectableServicesAfterScan) {
  vector<ByteString> whitelist;
  AddSSIDToWhitelist(kSSIDBytes1, sizeof(kSSIDBytes1), &whitelist);

  // Do nothing (i.e. do not invoke WakeOnWiFi::BeforeSuspendActions) if wake
  // on WiFi is not supported, whether or not we are in dark resume.
  SetInDarkResume(true);
  GetWakeOnWiFiTriggers()->clear();
  StartDHCPLeaseRenewalTimer();
  StopWakeToScanTimer();
  OnNoAutoConnectableServicesAfterScan(whitelist);
  EXPECT_FALSE(WakeToScanTimerIsRunning());
  EXPECT_TRUE(DHCPLeaseRenewalTimerIsRunning());
  EXPECT_EQ(GetWakeOnWiFiTriggers()->size(), 0);

  SetInDarkResume(false);
  GetWakeOnWiFiTriggers()->clear();
  StartDHCPLeaseRenewalTimer();
  StopWakeToScanTimer();
  OnNoAutoConnectableServicesAfterScan(whitelist);
  EXPECT_FALSE(WakeToScanTimerIsRunning());
  EXPECT_TRUE(DHCPLeaseRenewalTimerIsRunning());
  EXPECT_EQ(GetWakeOnWiFiTriggers()->size(), 0);
}

TEST_F(WakeOnWiFiTestWithMockDispatcher,
       WakeOnWiFiDisabled_OnWakeupReasonReceived_DoesNothing) {
  ScopedMockLog log;
  ScopeLogger::GetInstance()->EnableScopesByName("wifi");
  ScopeLogger::GetInstance()->set_verbose_level(7);

  SetWakeOnPacketConnMessage msg;
  NetlinkPacket packet(kWakeReasonSSIDNlMsg, sizeof(kWakeReasonSSIDNlMsg));
  msg.InitFromPacket(&packet, GetWakeupReportMsgContext());
  EXPECT_CALL(log, Log(_, _, _)).Times(AnyNumber());
  EXPECT_CALL(
      log, Log(_, _, HasSubstr("Wake on WiFi not supported, so do nothing")));
  EXPECT_CALL(metrics_, NotifyWakeupReasonReceived()).Times(0);
  EXPECT_CALL(*this, RecordDarkResumeWakeReasonCallback(_)).Times(0);
  OnWakeupReasonReceived(msg);

  ScopeLogger::GetInstance()->EnableScopesByName("-wifi");
  ScopeLogger::GetInstance()->set_verbose_level(0);
}

TEST_F(WakeOnWiFiTestWithMockDispatcher,
       WakeOnWiFiDisabled_ConfigureSetWakeOnWiFiSettingsMessage_ReturnsFalse) {
  IPAddressStore no_addresses;
  IPAddressStore one_address;
  const char kIPv4AddrString[] = "1.1.1.1";
  one_address.AddUnique(
      IPAddress(string(kIPv4AddrString, sizeof(kIPv4AddrString))));
  set<WakeOnWiFi::WakeOnWiFiTrigger> no_trigs;
  set<WakeOnWiFi::WakeOnWiFiTrigger> one_trig;
  one_trig.insert(WakeOnWiFi::kWakeTriggerPattern);
  const int index = 1;  // wiphy device number
  vector<ByteString> whitelist;
  const uint32_t interval = kNetDetectScanIntervalSeconds;
  SetWakeOnPacketConnMessage msg;
  Error e;
  EXPECT_FALSE(ConfigureSetWakeOnWiFiSettingsMessage(
      &msg, no_trigs, no_addresses, index, interval, whitelist, &e));
  EXPECT_FALSE(ConfigureSetWakeOnWiFiSettingsMessage(
      &msg, one_trig, one_address, index, interval, whitelist, &e));
}

TEST_F(WakeOnWiFiTestWithMockDispatcher,
       WakeOnWiFiDisabled_WakeOnWiFiSettingsMatch_ReturnsFalse) {
  // Test that WakeOnWiFi::WakeOnWiFiSettingsMatch unconditionally returns false
  // when wake-on-WiFi is disabled by testing it against several cases where we
  // expect it to return true.
  IPAddressStore all_addresses;
  set<WakeOnWiFi::WakeOnWiFiTrigger> trigs;
  vector<ByteString> whitelist;
  const uint32_t interval = kNetDetectScanIntervalSeconds;

  GetWakeOnPacketConnMessage msg0;
  NetlinkPacket packet0(kResponseNoIPAddresses, sizeof(kResponseNoIPAddresses));
  msg0.InitFromPacket(&packet0, NetlinkMessage::MessageContext());
  EXPECT_FALSE(
      WakeOnWiFiSettingsMatch(msg0, trigs, all_addresses, interval, whitelist));

  trigs.insert(WakeOnWiFi::kWakeTriggerPattern);
  all_addresses.AddUnique(
      IPAddress(string(kIPV4Address0, sizeof(kIPV4Address0))));
  GetWakeOnPacketConnMessage msg1;
  NetlinkPacket packet1(kResponseIPV40, sizeof(kResponseIPV40));
  msg1.InitFromPacket(&packet1, NetlinkMessage::MessageContext());
  EXPECT_FALSE(
      WakeOnWiFiSettingsMatch(msg1, trigs, all_addresses, interval, whitelist));

  trigs.insert(WakeOnWiFi::kWakeTriggerDisconnect);
  GetWakeOnPacketConnMessage msg2;
  NetlinkPacket packet2(kResponseIPV40WakeOnDisconnect,
                        sizeof(kResponseIPV40WakeOnDisconnect));
  msg2.InitFromPacket(&packet2, NetlinkMessage::MessageContext());
  EXPECT_FALSE(
      WakeOnWiFiSettingsMatch(msg2, trigs, all_addresses, interval, whitelist));

  trigs.erase(WakeOnWiFi::kWakeTriggerDisconnect);
  all_addresses.AddUnique(
      IPAddress(string(kIPV4Address1, sizeof(kIPV4Address1))));
  GetWakeOnPacketConnMessage msg3;
  NetlinkPacket packet3(kResponseIPV401, sizeof(kResponseIPV401));
  msg3.InitFromPacket(&packet3, NetlinkMessage::MessageContext());
  EXPECT_FALSE(
      WakeOnWiFiSettingsMatch(msg3, trigs, all_addresses, interval, whitelist));

  all_addresses.AddUnique(
      IPAddress(string(kIPV6Address0, sizeof(kIPV6Address0))));
  GetWakeOnPacketConnMessage msg4;
  NetlinkPacket packet4(kResponseIPV401IPV60, sizeof(kResponseIPV401IPV60));
  msg4.InitFromPacket(&packet4, NetlinkMessage::MessageContext());
  EXPECT_FALSE(
      WakeOnWiFiSettingsMatch(msg4, trigs, all_addresses, interval, whitelist));

  all_addresses.AddUnique(
      IPAddress(string(kIPV6Address1, sizeof(kIPV6Address1))));
  GetWakeOnPacketConnMessage msg5;
  NetlinkPacket packet5(kResponseIPV401IPV601, sizeof(kResponseIPV401IPV601));
  msg5.InitFromPacket(&packet5, NetlinkMessage::MessageContext());
  EXPECT_FALSE(
      WakeOnWiFiSettingsMatch(msg5, trigs, all_addresses, interval, whitelist));

  all_addresses.Clear();
  trigs.clear();
  trigs.insert(WakeOnWiFi::kWakeTriggerSSID);
  AddSSIDToWhitelist(kSSIDBytes1, sizeof(kSSIDBytes1), &whitelist);
  AddSSIDToWhitelist(kSSIDBytes2, sizeof(kSSIDBytes2), &whitelist);
  GetWakeOnPacketConnMessage msg6;
  NetlinkPacket packet6(kResponseWakeOnSSID, sizeof(kResponseWakeOnSSID));
  msg6.InitFromPacket(&packet6, NetlinkMessage::MessageContext());
  EXPECT_FALSE(
      WakeOnWiFiSettingsMatch(msg6, trigs, all_addresses, interval, whitelist));
}

TEST_F(WakeOnWiFiTestWithMockDispatcher,
       WakeOnWiFiDisabled_ParseWakeOnWiFiCapabilities_DoesNothing) {
  // |kNewWiphyNlMsg| should indicate that the NIC supports wake on pattern
  // (on up to |kNewWiphyNlMsg_MaxPatterns| registered patterns),
  // supports wake on SSID (on up to |kNewWiphyNlMsg_MaxSSIDs| SSIDs), and
  // supports wake on disconnect. Test that
  // WakeOnWiFi::ParseWakeOnWiFiCapabilities does nothing and does not parse
  // these capabilities when wake-on-WiFi is disabled.
  ClearWakeOnWiFiTriggersSupported();
  SetWakeOnWiFiMaxSSIDs(0);
  NewWiphyMessage msg;
  NetlinkPacket packet(kNewWiphyNlMsg, sizeof(kNewWiphyNlMsg));
  msg.InitFromPacket(&packet, NetlinkMessage::MessageContext());
  ParseWakeOnWiFiCapabilities(msg);
  EXPECT_TRUE(GetWakeOnWiFiTriggersSupported()->empty());
  EXPECT_EQ(0, GetWakeOnWiFiMaxPatterns());
  EXPECT_EQ(0, GetWakeOnWiFiMaxSSIDs());
}

#endif  // DISABLE_WAKE_ON_WIFI

}  // namespace shill
