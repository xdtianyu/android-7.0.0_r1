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

#ifndef SHILL_PORTAL_DETECTOR_H_
#define SHILL_PORTAL_DETECTOR_H_

#include <memory>
#include <string>
#include <vector>

#include <base/callback.h>
#include <base/cancelable_callback.h>
#include <base/memory/ref_counted.h>
#include <base/memory/weak_ptr.h>
#include <gtest/gtest_prod.h>  // for FRIEND_TEST

#include "shill/connectivity_trial.h"
#include "shill/net/shill_time.h"
#include "shill/refptr_types.h"

namespace shill {

class ByteString;
class PortalDetector;
class Time;

// The PortalDetector class implements the portal detection
// facility in shill, which is responsible for checking to see
// if a connection has "general internet connectivity".
//
// This information can be used for ranking one connection
// against another, or for informing UI and other components
// outside the connection manager whether the connection seems
// available for "general use" or if further user action may be
// necessary (e.g, click through of a WiFi Hotspot's splash
// page).
//
// This is achieved by using one or more ConnectivityTrial attempts
// to access a URL and expecting a specific response.  Any result
// that deviates from this result (DNS or HTTP errors, as well as
// deviations from the expected content) are considered failures.
class PortalDetector {
 public:
  struct Result {
    Result()
        : trial_result(ConnectivityTrial::Result()),
          num_attempts(0),
          final(false) {}
    explicit Result(ConnectivityTrial::Result result_in)
        : trial_result(result_in),
          num_attempts(0),
          final(false) {}
    Result(ConnectivityTrial::Result result_in,
           int num_attempts_in,
           int final_in)
        : trial_result(result_in),
          num_attempts(num_attempts_in),
          final(final_in) {}

    ConnectivityTrial::Result trial_result;

    // Total number of connectivity trials attempted.
    // This includes failure, timeout and successful attempts.
    // This only valid when |final| is true.
    int num_attempts;
    bool final;
  };

  static const int kDefaultCheckIntervalSeconds;
  static const char kDefaultCheckPortalList[];
  // Maximum number of times the PortalDetector will attempt a connection.
  static const int kMaxRequestAttempts;

  PortalDetector(ConnectionRefPtr connection,
                 EventDispatcher* dispatcher,
                 const base::Callback<void(const PortalDetector::Result&)>
                     &callback);
  virtual ~PortalDetector();

  // Start a portal detection test.  Returns true if |url_string| correctly
  // parses as a URL.  Returns false (and does not start) if the |url_string|
  // fails to parse.
  //
  // As each attempt completes the callback handed to the constructor will
  // be called.  The PortalDetector will try up to kMaxRequestAttempts times
  // to successfully retrieve the URL.  If the attempt is successful or
  // this is the last attempt, the "final" flag in the Result structure will
  // be true, otherwise it will be false, and the PortalDetector will
  // schedule the next attempt.
  virtual bool Start(const std::string& url_string);
  virtual bool StartAfterDelay(const std::string& url_string,
                               int delay_seconds);

  // End the current portal detection process if one exists, and do not call
  // the callback.
  virtual void Stop();

  // Returns whether portal request is "in progress": whether the underlying
  // ConnectivityTrial is in the progress of making attempts.  Returns true if
  // attempts are in progress, false otherwise.  Notably, this function
  // returns false during the period of time between calling "Start" or
  // "StartAfterDelay" and the actual start of the first attempt. In the case
  // where multiple attempts may be tried, IsInProgress will return true after
  // the first attempt has actively started testing the connection.
  virtual bool IsInProgress();

 private:
  friend class PortalDetectorTest;
  FRIEND_TEST(PortalDetectorTest, StartAttemptFailed);
  FRIEND_TEST(PortalDetectorTest, AdjustStartDelayImmediate);
  FRIEND_TEST(PortalDetectorTest, AdjustStartDelayAfterDelay);
  FRIEND_TEST(PortalDetectorTest, AttemptCount);
  FRIEND_TEST(PortalDetectorTest, ReadBadHeadersRetry);
  FRIEND_TEST(PortalDetectorTest, ReadBadHeader);
  FRIEND_TEST(PortalDetectorTest, RequestTimeout);
  FRIEND_TEST(PortalDetectorTest, ReadPartialHeaderTimeout);
  FRIEND_TEST(PortalDetectorTest, ReadCompleteHeader);
  FRIEND_TEST(PortalDetectorTest, ReadMatchingHeader);
  FRIEND_TEST(PortalDetectorTest, InvalidURL);

  // Minimum time between attempts to connect to server.
  static const int kMinTimeBetweenAttemptsSeconds;
  // Time to wait for request to complete.
  static const int kRequestTimeoutSeconds;
  // Maximum number of failures in content phase before we stop attempting
  // connections.
  static const int kMaxFailuresInContentPhase;

  // Internal method to update the start time of the next event.  This is used
  // to keep attempts spaced by at least kMinTimeBetweenAttemptsSeconds in the
  // event of a retry.
  void UpdateAttemptTime(int delay_seconds);

  // Internal method used to adjust the start delay in the event of a retry.
  // Calculates the elapsed time between the most recent attempt and the point
  // the retry is scheduled.  Adds an additional delay to meet the
  // kMinTimeBetweenAttemptsSeconds requirement.
  int AdjustStartDelay(int init_delay_seconds);

  // Callback used by ConnectivityTrial to return |result| after attempting to
  // determine connectivity status.
  void CompleteAttempt(ConnectivityTrial::Result result);

  int attempt_count_;
  struct timeval attempt_start_time_;
  ConnectionRefPtr connection_;
  EventDispatcher* dispatcher_;
  base::WeakPtrFactory<PortalDetector> weak_ptr_factory_;
  base::Callback<void(const Result&)> portal_result_callback_;
  base::Callback<void(ConnectivityTrial::Result)> connectivity_trial_callback_;
  Time* time_;
  int failures_in_content_phase_;
  std::unique_ptr<ConnectivityTrial> connectivity_trial_;


  DISALLOW_COPY_AND_ASSIGN(PortalDetector);
};

}  // namespace shill

#endif  // SHILL_PORTAL_DETECTOR_H_
