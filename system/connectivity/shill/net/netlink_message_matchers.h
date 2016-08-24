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

#ifndef SHILL_NET_NETLINK_MESSAGE_MATCHERS_H_
#define SHILL_NET_NETLINK_MESSAGE_MATCHERS_H_

#include <base/logging.h>
#include <gmock/gmock.h>

#include "shill/net/netlink_message.h"
#include "shill/net/nl80211_message.h"

namespace shill {

// Given a netlink message, verifies that it is an Nl80211Message and verifies,
// further that it is the specified command.
MATCHER_P2(IsNl80211Command, nl80211_message_type, command, "") {
  if (!arg) {
    LOG(INFO) << "Null message";
    return false;
  }
  if (arg->message_type() != nl80211_message_type) {
    LOG(INFO) << "Not an nl80211 message";
    return false;
  }
  const Nl80211Message* msg = static_cast<const Nl80211Message*>(arg);
  if (msg->command() != command) {
    LOG(INFO) << "Not a message of type " << command
               << " (it's a " << +msg->command() << ")";
    return false;
  }
  return true;
}

// Given a netlink message, verifies that it is configured to disable
// wake on WiFi functionality of the NIC.
MATCHER(IsDisableWakeOnWiFiMsg, "") {
  if (!arg) {
    LOG(INFO) << "Null message";
    return false;
  }
  const Nl80211Message* msg = static_cast<const Nl80211Message*>(arg);
  if (msg->command() != NL80211_CMD_SET_WOWLAN) {
    LOG(INFO) << "Not a NL80211_CMD_SET_WOWLAN message";
    return false;
  }
  uint32_t wiphy;
  if (!msg->const_attributes()->GetU32AttributeValue(NL80211_ATTR_WIPHY,
                                                     &wiphy)) {
    LOG(INFO) << "Wiphy index not set";
    return false;
  }
  AttributeListConstRefPtr triggers;
  if (msg->const_attributes()->ConstGetNestedAttributeList(
          NL80211_ATTR_WOWLAN_TRIGGERS, &triggers)) {
    LOG(INFO) << "Message contains NL80211_ATTR_WOWLAN_TRIGGERS";
    return false;
  }
  return true;
}

// Verifies that a NetlinkMessage is an NL80211_CMD_TRIGGER_SCAN message that
// contains exactly one SSID along with the requisite empty one.
MATCHER_P(HasHiddenSSID, nl80211_message_type, "") {
  if (!arg) {
    LOG(INFO) << "Null message";
    return false;
  }
  if (arg->message_type() != nl80211_message_type) {
    LOG(INFO) << "Not an nl80211 message";
    return false;
  }
  const Nl80211Message* msg = reinterpret_cast<const Nl80211Message*>(arg);
  if (msg->command() != NL80211_CMD_TRIGGER_SCAN) {
    LOG(INFO) << "Not a NL80211_CMD_TRIGGER_SCAN message";
    return false;
  }
  AttributeListConstRefPtr ssids;
  if (!msg->const_attributes()->ConstGetNestedAttributeList(
      NL80211_ATTR_SCAN_SSIDS, &ssids)) {
    LOG(INFO) << "No SSID list in message";
    return false;
  }
  ByteString ssid;
  AttributeIdIterator ssid_iter(*ssids);
  if (!ssids->GetRawAttributeValue(ssid_iter.GetId(), &ssid)) {
    LOG(INFO) << "SSID list contains no (hidden) SSIDs";
    return false;
  }

  // A valid Scan containing a single hidden SSID should contain
  // two SSID entries: one containing the SSID we are looking for,
  // and an empty entry, signifying that we also want to do a
  // broadcast probe request for all non-hidden APs as well.
  ByteString empty_ssid;
  if (ssid_iter.AtEnd()) {
    LOG(INFO) << "SSID list doesn't contain an empty SSIDs (but should)";
    return false;
  }
  ssid_iter.Advance();
  if (!ssids->GetRawAttributeValue(ssid_iter.GetId(), &empty_ssid) ||
      !empty_ssid.IsEmpty()) {
    LOG(INFO) << "SSID list doesn't contain an empty SSID (but should)";
    return false;
  }

  return true;
}

// Verifies that a NetlinkMessage is an NL80211_CMD_TRIGGER_SCAN message that
// contains no SSIDs.
MATCHER_P(HasNoHiddenSSID, nl80211_message_type, "") {
  if (!arg) {
    LOG(INFO) << "Null message";
    return false;
  }
  if (arg->message_type() != nl80211_message_type) {
    LOG(INFO) << "Not an nl80211 message";
    return false;
  }
  const Nl80211Message* msg = reinterpret_cast<const Nl80211Message*>(arg);
  if (msg->command() != NL80211_CMD_TRIGGER_SCAN) {
    LOG(INFO) << "Not a NL80211_CMD_TRIGGER_SCAN message";
    return false;
  }
  AttributeListConstRefPtr ssids;
  if (!msg->const_attributes()->ConstGetNestedAttributeList(
      NL80211_ATTR_SCAN_SSIDS, &ssids)) {
    return true;
  }
  AttributeIdIterator ssid_iter(*ssids);
  if (ssid_iter.AtEnd()) {
    return true;
  }

  LOG(INFO) << "SSID list contains at least one (hidden) SSID";
  return false;
}

}  // namespace shill

#endif  // SHILL_NET_NETLINK_MESSAGE_MATCHERS_H_
