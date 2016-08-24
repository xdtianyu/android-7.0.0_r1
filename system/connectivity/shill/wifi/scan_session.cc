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

#include "shill/wifi/scan_session.h"

#include <algorithm>
#include <set>
#include <string>
#include <vector>

#include <base/bind.h>
#include <base/memory/weak_ptr.h>
#include <base/stl_util.h>
#include <base/strings/stringprintf.h>

#include "shill/event_dispatcher.h"
#include "shill/logging.h"
#include "shill/metrics.h"
#include "shill/net/netlink_manager.h"
#include "shill/net/netlink_message.h"
#include "shill/net/nl80211_attribute.h"
#include "shill/net/nl80211_message.h"

using base::Bind;
using base::StringPrintf;
using std::set;
using std::string;
using std::vector;

namespace shill {

namespace Logging {
static auto kModuleLogScope = ScopeLogger::kWiFi;
static string ObjectID(ScanSession* s) { return "(scan_session)"; }
}

const float ScanSession::kAllFrequencies = 1.1;
const uint64_t ScanSession::kScanRetryDelayMilliseconds = 200;  // Arbitrary.
const size_t ScanSession::kScanRetryCount = 50;

ScanSession::ScanSession(
    NetlinkManager* netlink_manager,
    EventDispatcher* dispatcher,
    const WiFiProvider::FrequencyCountList& previous_frequencies,
    const set<uint16_t>& available_frequencies,
    uint32_t ifindex,
    const FractionList& fractions,
    size_t min_frequencies,
    size_t max_frequencies,
    OnScanFailed on_scan_failed,
    Metrics* metrics)
    : weak_ptr_factory_(this),
      netlink_manager_(netlink_manager),
      dispatcher_(dispatcher),
      frequency_list_(previous_frequencies),
      total_connections_(0),
      total_connects_provided_(0),
      total_fraction_wanted_(0.0),
      wifi_interface_index_(ifindex),
      ssids_(ByteString::IsLessThan),
      fractions_(fractions),
      min_frequencies_(min_frequencies),
      max_frequencies_(max_frequencies),
      on_scan_failed_(on_scan_failed),
      scan_tries_left_(kScanRetryCount),
      found_error_(false),
      metrics_(metrics) {
  sort(frequency_list_.begin(), frequency_list_.end(),
       &ScanSession::CompareFrequencyCount);
  // Add to |frequency_list_| all the frequencies from |available_frequencies|
  // that aren't in |previous_frequencies|.
  set<uint16_t> seen_frequencies;
  for (const auto& freq_conn : frequency_list_) {
    seen_frequencies.insert(freq_conn.frequency);
    total_connections_ += freq_conn.connection_count;
  }
  for (const auto freq : available_frequencies) {
    if (!ContainsKey(seen_frequencies, freq)) {
      frequency_list_.push_back(WiFiProvider::FrequencyCount(freq, 0));
    }
  }

  SLOG(this, 6) << "Frequency connections vector:";
  for (const auto& freq_conn : frequency_list_) {
    SLOG(this, 6) << "    freq[" << freq_conn.frequency << "] = "
                  << freq_conn.connection_count;
  }

  original_frequency_count_ = frequency_list_.size();
  ebusy_timer_.Pause();
}

ScanSession::~ScanSession() {
  const int kLogLevel = 6;
  ReportResults(kLogLevel);
}

bool ScanSession::HasMoreFrequencies() const {
  return !frequency_list_.empty();
}

vector<uint16_t> ScanSession::GetScanFrequencies(float fraction_wanted,
                                                 size_t min_frequencies,
                                                 size_t max_frequencies) {
  DCHECK_GE(fraction_wanted, 0);
  total_fraction_wanted_ += fraction_wanted;
  float total_connects_wanted = total_fraction_wanted_ * total_connections_;

  vector<uint16_t> frequencies;
  WiFiProvider::FrequencyCountList::iterator freq_connect =
      frequency_list_.begin();
  SLOG(this, 7) << "Scanning for frequencies:";
  while (freq_connect != frequency_list_.end()) {
    if (frequencies.size() >= min_frequencies) {
      if (total_connects_provided_ >= total_connects_wanted)
        break;
      if (frequencies.size() >= max_frequencies)
        break;
    }
    uint16_t frequency = freq_connect->frequency;
    size_t connection_count = freq_connect->connection_count;
    total_connects_provided_ += connection_count;
    frequencies.push_back(frequency);
    SLOG(this, 7) << "    freq[" << frequency << "] = " << connection_count;

    freq_connect = frequency_list_.erase(freq_connect);
  }
  return frequencies;
}

void ScanSession::InitiateScan() {
  float fraction_wanted = kAllFrequencies;
  if (!fractions_.empty()) {
    fraction_wanted = fractions_.front();
    fractions_.pop_front();
  }
  current_scan_frequencies_ = GetScanFrequencies(fraction_wanted,
                                                 min_frequencies_,
                                                 max_frequencies_);
  DoScan(current_scan_frequencies_);
}

void ScanSession::ReInitiateScan() {
  ebusy_timer_.Pause();
  DoScan(current_scan_frequencies_);
}

void ScanSession::DoScan(const vector<uint16_t>& scan_frequencies) {
  if (scan_frequencies.empty()) {
    LOG(INFO) << "Not sending empty frequency list";
    return;
  }
  TriggerScanMessage trigger_scan;
  trigger_scan.attributes()->CreateNl80211Attribute(
      NL80211_ATTR_SCAN_FREQUENCIES, NetlinkMessage::MessageContext());
  trigger_scan.attributes()->CreateNl80211Attribute(
      NL80211_ATTR_SCAN_SSIDS, NetlinkMessage::MessageContext());
  trigger_scan.attributes()->SetU32AttributeValue(NL80211_ATTR_IFINDEX,
                                                  wifi_interface_index_);
  AttributeListRefPtr frequency_list;
  if (!trigger_scan.attributes()->GetNestedAttributeList(
      NL80211_ATTR_SCAN_FREQUENCIES, &frequency_list) || !frequency_list) {
    LOG(FATAL) << "Couldn't get NL80211_ATTR_SCAN_FREQUENCIES.";
  }
  trigger_scan.attributes()->SetNestedAttributeHasAValue(
      NL80211_ATTR_SCAN_FREQUENCIES);

  SLOG(this, 6) << "We have requested scan frequencies:";
  string attribute_name;
  int i = 0;
  for (const auto freq : scan_frequencies) {
    SLOG(this, 6) << "  " << freq;
    attribute_name = StringPrintf("Frequency-%d", i);
    frequency_list->CreateU32Attribute(i, attribute_name.c_str());
    frequency_list->SetU32AttributeValue(i, freq);
    ++i;
  }

  if (!ssids_.empty()) {
    AttributeListRefPtr ssid_list;
    if (!trigger_scan.attributes()->GetNestedAttributeList(
        NL80211_ATTR_SCAN_SSIDS, &ssid_list) || !ssid_list) {
      LOG(FATAL) << "Couldn't get NL80211_ATTR_SCAN_SSIDS attribute.";
    }
    trigger_scan.attributes()->SetNestedAttributeHasAValue(
        NL80211_ATTR_SCAN_SSIDS);
    int i = 0;
    string attribute_name;
    for (const auto& ssid : ssids_) {
      attribute_name = StringPrintf("NL80211_ATTR_SSID_%d", i);
      ssid_list->CreateRawAttribute(i, attribute_name.c_str());
      ssid_list->SetRawAttributeValue(i, ssid);
      ++i;
    }
    // Add an empty one at the end so we ask for a broadcast in addition to
    // the specific SSIDs.
    attribute_name = StringPrintf("NL80211_ATTR_SSID_%d", i);
    ssid_list->CreateRawAttribute(i, attribute_name.c_str());
    ssid_list->SetRawAttributeValue(i, ByteString());
  }
  netlink_manager_->SendNl80211Message(
      &trigger_scan,
      Bind(&ScanSession::OnTriggerScanResponse,
           weak_ptr_factory_.GetWeakPtr()),
      Bind(&NetlinkManager::OnAckDoNothing),
      Bind(&ScanSession::OnTriggerScanErrorResponse,
           weak_ptr_factory_.GetWeakPtr()));
}

void ScanSession::OnTriggerScanResponse(const Nl80211Message& netlink_message) {
  LOG(WARNING) << "Didn't expect _this_ netlink message, here:";
  netlink_message.Print(0, 0);
  on_scan_failed_.Run();
  return;
}

void ScanSession::OnTriggerScanErrorResponse(
    NetlinkManager::AuxilliaryMessageType type,
    const NetlinkMessage* netlink_message) {
  switch (type) {
    case NetlinkManager::kErrorFromKernel: {
        if (!netlink_message) {
          LOG(ERROR) << __func__ << ": Message failed: NetlinkManager Error.";
          found_error_ = true;
          on_scan_failed_.Run();
          break;
        }
        if (netlink_message->message_type() !=
            ErrorAckMessage::GetMessageType()) {
          LOG(ERROR) << __func__ << ": Message failed: Not an error.";
          found_error_ = true;
          on_scan_failed_.Run();
          break;
        }
        const ErrorAckMessage* error_ack_message =
            static_cast<const ErrorAckMessage*>(netlink_message);
        if (error_ack_message->error()) {
          LOG(ERROR) << __func__ << ": Message failed: "
                     << error_ack_message->ToString();
          if (error_ack_message->error() == EBUSY) {
            if (scan_tries_left_ == 0) {
              LOG(ERROR) << "Retried progressive scan " << kScanRetryCount
                         << " times and failed each time.  Giving up.";
              found_error_ = true;
              on_scan_failed_.Run();
              scan_tries_left_ = kScanRetryCount;
              return;
            }
            --scan_tries_left_;
            SLOG(this, 3) << __func__ << " - trying again (" << scan_tries_left_
                          << " remaining after this)";
            ebusy_timer_.Resume();
            dispatcher_->PostDelayedTask(Bind(&ScanSession::ReInitiateScan,
                                              weak_ptr_factory_.GetWeakPtr()),
                                         kScanRetryDelayMilliseconds);
            break;
          }
          found_error_ = true;
          on_scan_failed_.Run();
        } else {
          SLOG(this, 6) << __func__ << ": Message ACKed";
        }
      }
      break;

    case NetlinkManager::kUnexpectedResponseType:
      LOG(ERROR) << "Message not handled by regular message handler:";
      if (netlink_message) {
        netlink_message->Print(0, 0);
      }
      found_error_ = true;
      on_scan_failed_.Run();
      break;

    case NetlinkManager::kTimeoutWaitingForResponse:
      // This is actually expected since, in the working case, a trigger scan
      // message gets its responses broadcast rather than unicast.
      break;

    default:
      LOG(ERROR) << "Unexpected auxiliary message type: " << type;
      found_error_ = true;
      on_scan_failed_.Run();
      break;
  }
}

void ScanSession::ReportResults(int log_level) {
  SLOG(this, log_level) << "------ ScanSession finished ------";
  SLOG(this, log_level) << "Scanned "
                        << original_frequency_count_ - frequency_list_.size()
                        << " frequencies (" << frequency_list_.size()
                        << " remaining)";
  if (found_error_) {
    SLOG(this, log_level) << "ERROR encountered during scan ("
                          << current_scan_frequencies_.size() << " frequencies"
                          << " dangling - counted as scanned but, really, not)";
  } else {
    SLOG(this, log_level) << "No error encountered during scan.";
  }

  base::TimeDelta elapsed_time;
  ebusy_timer_.GetElapsedTime(&elapsed_time);
  if (metrics_) {
    metrics_->SendToUMA(Metrics::kMetricWiFiScanTimeInEbusyMilliseconds,
                        elapsed_time.InMilliseconds(),
                        Metrics::kMetricTimeToScanMillisecondsMin,
                        Metrics::kMetricTimeToScanMillisecondsMax,
                        Metrics::kMetricTimeToScanMillisecondsNumBuckets);
  }
  SLOG(this, log_level) << "Spent " << elapsed_time.InMillisecondsRoundedUp()
                        << " milliseconds waiting for EBUSY.";
}

void ScanSession::AddSsid(const ByteString& ssid) {
  ssids_.insert(ssid);
}

// static
bool ScanSession::CompareFrequencyCount(
    const WiFiProvider::FrequencyCount& first,
    const WiFiProvider::FrequencyCount& second) {
  return first.connection_count > second.connection_count;
}

}  // namespace shill

