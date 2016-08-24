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

#include "shill/wifi/callback80211_metrics.h"

#include <string>

#include "shill/logging.h"
#include "shill/metrics.h"
#include "shill/net/ieee80211.h"
#include "shill/net/netlink_manager.h"
#include "shill/net/nl80211_message.h"

using std::string;

namespace shill {

namespace Logging {
static auto kModuleLogScope = ScopeLogger::kWiFi;
static string ObjectID(const Callback80211Metrics* c) {
  return "(callback80211metrics)";
}
}

Callback80211Metrics::Callback80211Metrics(Metrics* metrics)
    : metrics_(metrics) {}

IEEE_80211::WiFiReasonCode Callback80211Metrics::WiFiReasonCodeFromUint16(
    uint16_t reason) const {
  IEEE_80211::WiFiReasonCode reason_enum = IEEE_80211::kReasonCodeInvalid;
  if (reason == IEEE_80211::kReasonCodeReserved0 ||
      reason == IEEE_80211::kReasonCodeReserved12 ||
      (reason >= IEEE_80211::kReasonCodeReservedBegin25 &&
       reason <= IEEE_80211::kReasonCodeReservedEnd31) ||
      (reason >= IEEE_80211::kReasonCodeReservedBegin40 &&
       reason <= IEEE_80211::kReasonCodeReservedEnd44) ||
      reason >= IEEE_80211::kReasonCodeMax) {
    SLOG(this, 1) << "Invalid reason code in disconnect message";
    reason_enum = IEEE_80211::kReasonCodeInvalid;
  } else {
    reason_enum = static_cast<IEEE_80211::WiFiReasonCode>(reason);
  }
  return reason_enum;
}

void Callback80211Metrics::CollectDisconnectStatistics(
    const NetlinkMessage& netlink_message) {
  if (!metrics_) {
    return;
  }
  // We only handle disconnect and deauthenticate messages, both of which are
  // nl80211 messages.
  if (netlink_message.message_type() != Nl80211Message::GetMessageType()) {
    return;
  }
  const Nl80211Message& message =
      * reinterpret_cast<const Nl80211Message*>(&netlink_message);

  // Station-instigated disconnects provide their information in the
  // deauthenticate message but AP-instigated disconnects provide it in the
  // disconnect message.
  uint16_t reason = IEEE_80211::kReasonCodeUnspecified;
  if (message.command() == DeauthenticateMessage::kCommand) {
    SLOG(this, 3) << "Handling Deauthenticate Message";
    message.Print(3, 3);
    // If there's no frame, this is probably an AP-caused disconnect and
    // there'll be a disconnect message to tell us about that.
    ByteString rawdata;
    if (!message.const_attributes()->GetRawAttributeValue(NL80211_ATTR_FRAME,
                                                          &rawdata)) {
      SLOG(this, 5) << "No frame in deauthenticate message, ignoring";
      return;
    }
    Nl80211Frame frame(rawdata);
    reason = frame.reason();
  } else if (message.command() == DisconnectMessage::kCommand) {
    SLOG(this, 3) << "Handling Disconnect Message";
    message.Print(3, 3);
    // If there's no reason code, this is probably a STA-caused disconnect and
    // there was be a disconnect message to tell us about that.
    if (!message.const_attributes()->GetU16AttributeValue(
            NL80211_ATTR_REASON_CODE, &reason)) {
      SLOG(this, 5) << "No reason code in disconnect message, ignoring";
      return;
    }
  } else {
    return;
  }

  IEEE_80211::WiFiReasonCode reason_enum = WiFiReasonCodeFromUint16(reason);

  Metrics::WiFiDisconnectByWhom by_whom =
      message.const_attributes()->IsFlagAttributeTrue(
          NL80211_ATTR_DISCONNECTED_BY_AP) ? Metrics::kDisconnectedByAp :
          Metrics::kDisconnectedNotByAp;
  SLOG(this, 1) << "Notify80211Disconnect by " << (by_whom ? "station" : "AP")
                << " because:" << reason_enum;
  metrics_->Notify80211Disconnect(by_whom, reason_enum);
}

}  // namespace shill.
