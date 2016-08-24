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

#include "shill/portal_detector.h"

#include <string>

#include <base/bind.h>
#include <base/strings/string_number_conversions.h>
#include <base/strings/string_util.h>
#include <base/strings/stringprintf.h>
#if defined(__ANDROID__)
#include <dbus/service_constants.h>
#else
#include <chromeos/dbus/service_constants.h>
#endif  // __ANDROID__

#include "shill/connection.h"
#include "shill/connectivity_trial.h"
#include "shill/logging.h"

using base::Bind;
using base::Callback;
using base::StringPrintf;
using std::string;

namespace shill {

namespace Logging {
static auto kModuleLogScope = ScopeLogger::kPortal;
static string ObjectID(Connection* c) { return c->interface_name(); }
}

const int PortalDetector::kDefaultCheckIntervalSeconds = 30;
const char PortalDetector::kDefaultCheckPortalList[] = "ethernet,wifi,cellular";

const int PortalDetector::kMaxRequestAttempts = 3;
const int PortalDetector::kMinTimeBetweenAttemptsSeconds = 3;
const int PortalDetector::kRequestTimeoutSeconds = 10;
const int PortalDetector::kMaxFailuresInContentPhase = 2;

PortalDetector::PortalDetector(
    ConnectionRefPtr connection,
    EventDispatcher* dispatcher,
    const Callback<void(const PortalDetector::Result&)>& callback)
    : attempt_count_(0),
      attempt_start_time_((struct timeval){0}),
      connection_(connection),
      dispatcher_(dispatcher),
      weak_ptr_factory_(this),
      portal_result_callback_(callback),
      connectivity_trial_callback_(Bind(&PortalDetector::CompleteAttempt,
                                        weak_ptr_factory_.GetWeakPtr())),
      time_(Time::GetInstance()),
      failures_in_content_phase_(0),
      connectivity_trial_(
          new ConnectivityTrial(connection_,
                                dispatcher_,
                                kRequestTimeoutSeconds,
                                connectivity_trial_callback_)) { }

PortalDetector::~PortalDetector() {
  Stop();
}

bool PortalDetector::Start(const string& url_string) {
  return StartAfterDelay(url_string, 0);
}

bool PortalDetector::StartAfterDelay(const string& url_string,
                                     int delay_seconds) {
  SLOG(connection_.get(), 3) << "In " << __func__;

  if (!connectivity_trial_->Start(url_string, delay_seconds * 1000)) {
    return false;
  }
  attempt_count_ = 1;
  // The attempt_start_time_ is calculated based on the current time and
  // |delay_seconds|.  This is used to determine if a portal detection attempt
  // is in progress.
  UpdateAttemptTime(delay_seconds);
  // If we're starting a new set of attempts, discard past failure history.
  failures_in_content_phase_ = 0;
  return true;
}

void PortalDetector::Stop() {
  SLOG(connection_.get(), 3) << "In " << __func__;

  attempt_count_ = 0;
  failures_in_content_phase_ = 0;
  if (connectivity_trial_.get())
    connectivity_trial_->Stop();
}

// IsInProgress returns true if a ConnectivityTrial is actively testing the
// connection.  If Start has been called, but the trial was delayed,
// IsInProgress will return false.  PortalDetector implements this by
// calculating the start time of the next ConnectivityTrial.  After an initial
// trial and in the case where multiple attempts may be tried, IsInProgress will
// return true.
bool PortalDetector::IsInProgress() {
  if (attempt_count_ > 1)
    return true;
  if (attempt_count_ == 1 && connectivity_trial_.get())
    return connectivity_trial_->IsActive();
  return false;
}

void PortalDetector::CompleteAttempt(ConnectivityTrial::Result trial_result) {
  Result result = Result(trial_result);
  if (trial_result.status == ConnectivityTrial::kStatusFailure &&
      trial_result.phase == ConnectivityTrial::kPhaseContent) {
    failures_in_content_phase_++;
  }

  LOG(INFO) << StringPrintf("Portal detection completed attempt %d with "
                            "phase==%s, status==%s, failures in content==%d",
                            attempt_count_,
                            ConnectivityTrial::PhaseToString(
                                trial_result.phase).c_str(),
                            ConnectivityTrial::StatusToString(
                                trial_result.status).c_str(),
                            failures_in_content_phase_);

  if (trial_result.status == ConnectivityTrial::kStatusSuccess ||
      attempt_count_ >= kMaxRequestAttempts ||
      failures_in_content_phase_ >= kMaxFailuresInContentPhase) {
    result.num_attempts = attempt_count_;
    result.final = true;
    Stop();
  } else {
    attempt_count_++;
    int retry_delay_seconds = AdjustStartDelay(0);
    connectivity_trial_->Retry(retry_delay_seconds * 1000);
    UpdateAttemptTime(retry_delay_seconds);
  }
  portal_result_callback_.Run(result);
}

void PortalDetector::UpdateAttemptTime(int delay_seconds) {
  time_->GetTimeMonotonic(&attempt_start_time_);
  struct timeval delay_timeval = { delay_seconds, 0 };
  timeradd(&attempt_start_time_, &delay_timeval, &attempt_start_time_);
}


int PortalDetector::AdjustStartDelay(int init_delay_seconds) {
  int next_attempt_delay_seconds = 0;
  if (attempt_count_ > 0) {
    // Ensure that attempts are spaced at least by a minimal interval.
    struct timeval now, elapsed_time;
    time_->GetTimeMonotonic(&now);
    timersub(&now, &attempt_start_time_, &elapsed_time);
    SLOG(connection_.get(), 4) << "Elapsed time from previous attempt is "
                               << elapsed_time.tv_sec << " seconds.";
    if (elapsed_time.tv_sec < kMinTimeBetweenAttemptsSeconds) {
      next_attempt_delay_seconds = kMinTimeBetweenAttemptsSeconds -
                                   elapsed_time.tv_sec;
    }
  } else {
    LOG(FATAL) << "AdjustStartDelay in PortalDetector called without "
                  "previous attempts";
  }
  SLOG(connection_.get(), 3) << "Adjusting trial start delay from "
                             << init_delay_seconds << " seconds to "
                             << next_attempt_delay_seconds << " seconds.";
  return next_attempt_delay_seconds;
}

}  // namespace shill
