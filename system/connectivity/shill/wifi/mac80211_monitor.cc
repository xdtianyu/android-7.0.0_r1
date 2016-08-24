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

#include "shill/wifi/mac80211_monitor.h"

#include <algorithm>

#include <base/files/file_util.h>
#include <base/strings/string_number_conversions.h>
#include <base/strings/string_split.h>
#include <base/strings/stringprintf.h>

#include "shill/logging.h"
#include "shill/metrics.h"
#include "shill/net/shill_time.h"

namespace shill {

using std::string;
using std::vector;

namespace Logging {
static auto kModuleLogScope = ScopeLogger::kWiFi;
static string ObjectID(Mac80211Monitor* m) { return m->link_name(); }
}

// statics
// At 17-25 bytes per queue, this accommodates 80 queues.
// ath9k has 4 queues, and WP2 has 16 queues.
const size_t Mac80211Monitor::kMaxQueueStateSizeBytes = 2048;
const char Mac80211Monitor::kQueueStatusPathFormat[] =
    "/sys/kernel/debug/ieee80211/%s/queues";
const char Mac80211Monitor::kWakeQueuesPathFormat[] =
    "/sys/kernel/debug/ieee80211/%s/wake_queues";
const time_t Mac80211Monitor::kQueueStatePollIntervalSeconds = 30;
const time_t Mac80211Monitor::kMinimumTimeBetweenWakesSeconds = 60;

Mac80211Monitor::Mac80211Monitor(
    EventDispatcher* dispatcher,
    const string& link_name,
    size_t queue_length_limit,
    const base::Closure& on_repair_callback,
    Metrics* metrics)
    : time_(Time::GetInstance()),
      dispatcher_(dispatcher),
      link_name_(link_name),
      queue_length_limit_(queue_length_limit),
      on_repair_callback_(on_repair_callback),
      metrics_(metrics),
      phy_name_("phy-unknown"),
      last_woke_queues_monotonic_seconds_(0),
      is_running_(false),
      have_ever_read_queue_state_file_(false),
      is_device_connected_(false),
      weak_ptr_factory_(this) {
  CHECK(time_);
  CHECK(dispatcher_);
  CHECK(metrics_);
}

Mac80211Monitor::~Mac80211Monitor() {
  Stop();
}

void Mac80211Monitor::Start(const string& phy_name) {
  SLOG(this, 2) << __func__ << " on " << link_name_ << " (" << phy_name << ")";
  CHECK(!is_running_);
  phy_name_ = phy_name;
  queue_state_file_path_ = base::FilePath(
      base::StringPrintf(kQueueStatusPathFormat, phy_name.c_str()));
  wake_queues_file_path_ = base::FilePath(
      base::StringPrintf(kWakeQueuesPathFormat, phy_name.c_str()));
  last_woke_queues_monotonic_seconds_ = 0;
  StartTimer();
  is_running_ = true;
}

void Mac80211Monitor::Stop() {
  SLOG(this, 2) << __func__ << " on " << link_name_ << " (" << phy_name_ << ")";
  StopTimer();
  is_running_ = false;
}

void Mac80211Monitor::UpdateConnectedState(bool new_state) {
  SLOG(this, 2) << __func__ << " (new_state=" << new_state << ")";
  is_device_connected_ = new_state;
}

void Mac80211Monitor::StartTimer() {
  SLOG(this, 2) << __func__;
  if (check_queues_callback_.IsCancelled()) {
    check_queues_callback_.Reset(
        Bind(&Mac80211Monitor::WakeQueuesIfNeeded,
             weak_ptr_factory_.GetWeakPtr()));
  }
  dispatcher_->PostDelayedTask(check_queues_callback_.callback(),
                                kQueueStatePollIntervalSeconds * 1000);
}

void Mac80211Monitor::StopTimer() {
  SLOG(this, 2) << __func__;
  check_queues_callback_.Cancel();
}

void Mac80211Monitor::WakeQueuesIfNeeded() {
  SLOG(this, 2) << __func__ << " on " << link_name_ << " (" << phy_name_ << ")";
  CHECK(is_running_);
  StartTimer();  // Always re-arm timer.

  if (is_device_connected_) {
    SLOG(this, 5) << "Skipping queue check: device is connected.";
    return;
  }

  string queue_state_string;
  if (!base::ReadFileToString(queue_state_file_path_, &queue_state_string,
                              kMaxQueueStateSizeBytes)) {
    if (have_ever_read_queue_state_file_) {
      LOG(WARNING) << __func__ << ": incomplete read on "
                   << queue_state_file_path_.value();
    }
    return;
  }
  have_ever_read_queue_state_file_ = true;

  uint32_t stuck_flags =
      CheckAreQueuesStuck(ParseQueueState(queue_state_string));
  SLOG(this, 2) << __func__ << " stuck_flags=" << stuck_flags;
  if (!(stuck_flags & kStopFlagPowerSave)) {
    if (stuck_flags) {
      LOG(INFO) << "Skipping wake: stuck_flags is "
                << std::showbase << std::hex << stuck_flags
                << " (require " << kStopFlagPowerSave << " to wake)."
                << std::dec << std::noshowbase;
    }
    return;
  }

  time_t now_monotonic_seconds = 0;
  if (!time_->GetSecondsMonotonic(&now_monotonic_seconds)) {
    LOG(WARNING) << "Skipping reset: failed to get monotonic time";
    return;
  }

  time_t elapsed = now_monotonic_seconds - last_woke_queues_monotonic_seconds_;
  if (elapsed < kMinimumTimeBetweenWakesSeconds) {
    LOG(WARNING) << "Skipping reset "
                 << "(min interval=" << kMinimumTimeBetweenWakesSeconds
                 << ", elapsed=" << elapsed << ")";
    return;
  }

  LOG(WARNING) << "Queues appear stuck; waking.";
  if (base::WriteFile(wake_queues_file_path_, "", sizeof("")) < 0) {
    PLOG(ERROR) << "Failed to write to " << wake_queues_file_path_.value();
    return;
  }

  if (!on_repair_callback_.is_null()) {
    on_repair_callback_.Run();
  }

  last_woke_queues_monotonic_seconds_ = now_monotonic_seconds;
}

uint32_t Mac80211Monitor::CheckAreQueuesStuck(
    const vector<QueueState>& queue_states) {
  size_t max_stuck_queue_len = 0;
  uint32_t stuck_flags = 0;
  for (const auto& queue_state : queue_states) {
    if (queue_state.queue_length < queue_length_limit_) {
      SLOG(this, 5) << __func__
                    << " skipping queue of length " << queue_state.queue_length
                    << " (threshold is " << queue_length_limit_ << ")";
      continue;
    }
    if (!queue_state.stop_flags) {
      SLOG(this, 5) << __func__
                    << " skipping queue of length " << queue_state.queue_length
                    << " (not stopped)";
      continue;
    }
    stuck_flags = stuck_flags | queue_state.stop_flags;
    max_stuck_queue_len =
        std::max(max_stuck_queue_len, queue_state.queue_length);
  }

  if (max_stuck_queue_len >= queue_length_limit_) {
    LOG(WARNING) << "max queue length is " << max_stuck_queue_len;
  }

  if (stuck_flags) {
    for (unsigned int i = 0; i < kStopReasonMax; ++i) {
      auto stop_reason = static_cast<QueueStopReason>(i);
      if (stuck_flags & GetFlagForReason(stop_reason)) {
        metrics_->SendEnumToUMA(Metrics::kMetricWifiStoppedTxQueueReason,
                                stop_reason,
                                kStopReasonMax);
      }
    }

    metrics_->SendToUMA(Metrics::kMetricWifiStoppedTxQueueLength,
                        max_stuck_queue_len,
                        Metrics::kMetricWifiStoppedTxQueueLengthMin,
                        Metrics::kMetricWifiStoppedTxQueueLengthMax,
                        Metrics::kMetricWifiStoppedTxQueueLengthNumBuckets);
  }

  return stuck_flags;
}

// example input:
// 01: 0x00000000/0
// ...
// 04: 0x00000000/0
//
// static
vector<Mac80211Monitor::QueueState>
Mac80211Monitor::ParseQueueState(const string& state_string) {
  vector<QueueState> queue_states;
  vector<string> queue_state_strings = base::SplitString(
      state_string, "\n", base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);

  if (queue_state_strings.empty()) {
    return queue_states;
  }

  // Trailing newline generates empty string as last element.
  // Discard that empty string if present.
  if (queue_state_strings.back().empty()) {
    queue_state_strings.pop_back();
  }

  for (const auto& queue_state : queue_state_strings) {
    // Example |queue_state|: "00: 0x00000000/10".
    vector<string> queuenum_and_state = base::SplitString(
        queue_state, ":", base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);
    if (queuenum_and_state.size() != 2) {
      LOG(WARNING) << __func__ << ": parse error on " << queue_state;
      continue;
    }

    // Example |queuenum_and_state|: {"00", "0x00000000/10"}.
    vector<string> stopflags_and_length = base::SplitString(
        queuenum_and_state[1], "/", base::TRIM_WHITESPACE,
        base::SPLIT_WANT_ALL);
    if (stopflags_and_length.size() != 2) {
      LOG(WARNING) << __func__ << ": parse error on " << queue_state;
      continue;
    }

    // Example |stopflags_and_length|: {"0x00000000", "10"}.
    size_t queue_number;
    uint32_t stop_flags;
    size_t queue_length;
    if (!base::StringToSizeT(queuenum_and_state[0], &queue_number)) {
      LOG(WARNING) << __func__ << ": parse error on " << queue_state;
      continue;
    }
    if (!base::HexStringToUInt(stopflags_and_length[0], &stop_flags)) {
      LOG(WARNING) << __func__ << ": parse error on " << queue_state;
      continue;
    }
    if (!base::StringToSizeT(stopflags_and_length[1], &queue_length)) {
      LOG(WARNING) << __func__ << ": parse error on " << queue_state;
      continue;
    }
    queue_states.emplace_back(queue_number, stop_flags, queue_length);
  }

  return queue_states;
}

// static
Mac80211Monitor::QueueStopFlag Mac80211Monitor::GetFlagForReason(
    QueueStopReason reason) {
  switch (reason) {
    case kStopReasonDriver:
      return kStopFlagDriver;
    case kStopReasonPowerSave:
      return kStopFlagPowerSave;
    case kStopReasonChannelSwitch:
      return kStopFlagChannelSwitch;
    case kStopReasonAggregation:
      return kStopFlagAggregation;
    case kStopReasonSuspend:
      return kStopFlagSuspend;
    case kStopReasonBufferAdd:
      return kStopFlagBufferAdd;
    case kStopReasonChannelTypeChange:
      return kStopFlagChannelTypeChange;
  }

  // The switch statement above is exhaustive (-Wswitch will complain
  // if it is not). But |reason| might be outside the defined range for
  // the enum, so we need this to keep the compiler happy.
  return kStopFlagInvalid;
}

}  // namespace shill
