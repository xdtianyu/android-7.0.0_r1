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

#ifndef SHILL_CONNECTIVITY_TRIAL_H_
#define SHILL_CONNECTIVITY_TRIAL_H_

#include <memory>
#include <string>
#include <vector>

#include <base/callback.h>
#include <base/cancelable_callback.h>
#include <base/memory/ref_counted.h>
#include <base/memory/weak_ptr.h>
#include <gtest/gtest_prod.h>  // for FRIEND_TEST

#include "shill/http_request.h"
#include "shill/http_url.h"
#include "shill/net/shill_time.h"
#include "shill/net/sockets.h"
#include "shill/refptr_types.h"

namespace shill {

class ByteString;
class EventDispatcher;
class Time;

// The ConnectivityTrial class implements a single portal detection
// trial.  Each trial checks if a connection has "general internet
// connectivity."
//
// ConnectivityTrial is responsible for managing the callbacks between the
// calling class requesting a connectivity trial and the HTTPRequest that is
// used to test connectivity.  ConnectivityTrial maps between the HTTPRequest
// response codes to higher-level connection-oriented status.
//
// ConnectivityTrial tests the connection by attempting to parse and access a
// given URL.  Any result that deviates from the expected behavior (DNS or HTTP
// errors, as well as retrieved content errors, and timeouts) are considered
// failures.

class ConnectivityTrial {
 public:
  enum Phase {
    kPhaseConnection,
    kPhaseDNS,
    kPhaseHTTP,
    kPhaseContent,
    kPhaseUnknown
  };

  enum Status {
    kStatusFailure,
    kStatusSuccess,
    kStatusTimeout
  };

  struct Result {
    Result()
        : phase(kPhaseUnknown), status(kStatusFailure) {}
    Result(Phase phase_in, Status status_in)
        : phase(phase_in), status(status_in) {}
    Phase phase;
    Status status;
  };

  static const char kDefaultURL[];
  static const char kResponseExpected[];

  ConnectivityTrial(ConnectionRefPtr connection,
                    EventDispatcher* dispatcher,
                    int trial_timeout_seconds,
                    const base::Callback<void(Result)>& trial_callback);
  virtual ~ConnectivityTrial();

  // Static method used to map a portal detection phase tp a string.  This
  // includes the phases for connection, DNS, HTTP, returned content and
  // unknown.
  static const std::string PhaseToString(Phase phase);

  // Static method to map from the result of a portal detection phase to a
  // status string. This method supports success, timeout and failure.
  static const std::string StatusToString(Status status);

  // Static method mapping from HTTPRequest responses to ConntectivityTrial
  // phases for portal detection. For example, if the HTTPRequest result is
  // HTTPRequest::kResultDNSFailure, this method returns a
  // ConnectivityTrial::Result with the phase set to
  // ConnectivityTrial::kPhaseDNS and the status set to
  // ConnectivityTrial::kStatusFailure.
  static Result GetPortalResultForRequestResult(HTTPRequest::Result result);

  // Start a ConnectivityTrial with the supplied URL and starting delay (ms).
  // Returns trus if |url_string| correctly parses as a URL.  Returns false (and
  // does not start) if the |url_string| fails to parse.
  //
  // After a trial completes, the callback supplied in the constructor is
  // called.
  virtual bool Start(const std::string& url_string,
                     int start_delay_milliseconds);

  // After a trial completes, the calling class may call Retry on the trial.
  // This allows the underlying HTTPRequest object to be reused.  The URL is not
  // reparsed and the original URL supplied in the Start command is used.  The
  // |start_delay| is the time (ms) to wait before starting the trial.  Retry
  // returns true if the underlying HTTPRequest is still available.  If the
  // HTTPRequest was reset or never created, Retry will return false.
  virtual bool Retry(int start_delay_milliseconds);

  // End the current attempt if one is in progress.  Will not call the callback
  // with any intermediate results.
  // The ConnectivityTrial will cancel any existing scheduled tasks and destroy
  // the underlying HTTPRequest.
  virtual void Stop();

  // Method to return if the connection is being actively tested.
  virtual bool IsActive();

 private:
  friend class PortalDetectorTest;
  FRIEND_TEST(PortalDetectorTest, StartAttemptFailed);
  FRIEND_TEST(PortalDetectorTest, StartAttemptRepeated);
  FRIEND_TEST(PortalDetectorTest, StartAttemptAfterDelay);
  FRIEND_TEST(PortalDetectorTest, AttemptCount);
  FRIEND_TEST(PortalDetectorTest, ReadBadHeadersRetry);
  FRIEND_TEST(PortalDetectorTest, ReadBadHeader);
  friend class ConnectivityTrialTest;
  FRIEND_TEST(ConnectivityTrialTest, StartAttemptFailed);
  FRIEND_TEST(ConnectivityTrialTest, TrialRetry);
  FRIEND_TEST(ConnectivityTrialTest, ReadBadHeadersRetry);
  FRIEND_TEST(ConnectivityTrialTest, IsActive);

  // Start a ConnectivityTrial with the supplied delay in ms.
  void StartTrialAfterDelay(int start_delay_milliseconds);

  // Internal method used to start the actual connectivity trial, called after
  // the start delay completes.
  void StartTrialTask();

  // Callback used to return data read from the HTTPRequest.
  void RequestReadCallback(const ByteString& response_data);

  // Callback used to return the result of the HTTPRequest.
  void RequestResultCallback(HTTPRequest::Result result,
                             const ByteString& response_data);

  // Internal method used to clean up state and call the original caller that
  // created and triggered this ConnectivityTrial.
  void CompleteTrial(Result result);

  // Internal method used to cancel the timeout timer and stop an active
  // HTTPRequest.  If |reset_request| is true, this method resets the underlying
  // HTTPRequest object.
  void CleanupTrial(bool reset_request);

  // Callback used to cancel the underlying HTTPRequest in the event of a
  // timeout.
  void TimeoutTrialTask();

  ConnectionRefPtr connection_;
  EventDispatcher* dispatcher_;
  int trial_timeout_seconds_;
  base::Callback<void(Result)> trial_callback_;
  base::WeakPtrFactory<ConnectivityTrial> weak_ptr_factory_;
  base::Callback<void(const ByteString&)> request_read_callback_;
  base::Callback<void(HTTPRequest::Result, const ByteString&)>
        request_result_callback_;
  std::unique_ptr<HTTPRequest> request_;

  Sockets sockets_;
  HTTPURL url_;
  base::CancelableClosure trial_;
  base::CancelableClosure trial_timeout_;
  bool is_active_;
};

}  // namespace shill

#endif  // SHILL_CONNECTIVITY_TRIAL_H_
