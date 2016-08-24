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

#include "shill/connectivity_trial.h"

#include <string>

#include <base/bind.h>
#include <base/strings/pattern.h>
#include <base/strings/string_number_conversions.h>
#include <base/strings/string_util.h>
#include <base/strings/stringprintf.h>
#if defined(__ANDROID__)
#include <dbus/service_constants.h>
#else
#include <chromeos/dbus/service_constants.h>
#endif  // __ANDROID__

#include "shill/async_connection.h"
#include "shill/connection.h"
#include "shill/dns_client.h"
#include "shill/event_dispatcher.h"
#include "shill/http_request.h"
#include "shill/http_url.h"
#include "shill/logging.h"
#include "shill/net/ip_address.h"
#include "shill/net/sockets.h"

using base::Bind;
using base::Callback;
using base::StringPrintf;
using std::string;

namespace shill {

namespace Logging {
static auto kModuleLogScope = ScopeLogger::kPortal;
static string ObjectID(Connection* c) { return c->interface_name(); }
}

const char ConnectivityTrial::kDefaultURL[] =
    "http://www.gstatic.com/generate_204";
const char ConnectivityTrial::kResponseExpected[] = "HTTP/?.? 204";

ConnectivityTrial::ConnectivityTrial(
    ConnectionRefPtr connection,
    EventDispatcher* dispatcher,
    int trial_timeout_seconds,
    const Callback<void(Result)>& callback)
    : connection_(connection),
      dispatcher_(dispatcher),
      trial_timeout_seconds_(trial_timeout_seconds),
      trial_callback_(callback),
      weak_ptr_factory_(this),
      request_read_callback_(
          Bind(&ConnectivityTrial::RequestReadCallback,
               weak_ptr_factory_.GetWeakPtr())),
      request_result_callback_(
          Bind(&ConnectivityTrial::RequestResultCallback,
               weak_ptr_factory_.GetWeakPtr())),
      is_active_(false) { }

ConnectivityTrial::~ConnectivityTrial() {
  Stop();
}

bool ConnectivityTrial::Retry(int start_delay_milliseconds) {
  SLOG(connection_.get(), 3) << "In " << __func__;
  if (request_.get())
    CleanupTrial(false);
  else
    return false;
  StartTrialAfterDelay(start_delay_milliseconds);
  return true;
}

bool ConnectivityTrial::Start(const string& url_string,
                              int start_delay_milliseconds) {
  SLOG(connection_.get(), 3) << "In " << __func__;

  if (!url_.ParseFromString(url_string)) {
    LOG(ERROR) << "Failed to parse URL string: " << url_string;
    return false;
  }
  if (request_.get()) {
    CleanupTrial(false);
  } else {
    request_.reset(new HTTPRequest(connection_, dispatcher_, &sockets_));
  }
  StartTrialAfterDelay(start_delay_milliseconds);
  return true;
}

void ConnectivityTrial::Stop() {
  SLOG(connection_.get(), 3) << "In " << __func__;

  if (!request_.get()) {
    return;
  }

  CleanupTrial(true);
}

void ConnectivityTrial::StartTrialAfterDelay(int start_delay_milliseconds) {
  SLOG(connection_.get(), 4) << "In " << __func__
                             << " delay = " << start_delay_milliseconds
                             << "ms.";
  trial_.Reset(Bind(&ConnectivityTrial::StartTrialTask,
                    weak_ptr_factory_.GetWeakPtr()));
  dispatcher_->PostDelayedTask(trial_.callback(), start_delay_milliseconds);
}

void ConnectivityTrial::StartTrialTask() {
  HTTPRequest::Result result =
      request_->Start(url_, request_read_callback_, request_result_callback_);
  if (result != HTTPRequest::kResultInProgress) {
    CompleteTrial(ConnectivityTrial::GetPortalResultForRequestResult(result));
    return;
  }
  is_active_ = true;

  trial_timeout_.Reset(Bind(&ConnectivityTrial::TimeoutTrialTask,
                            weak_ptr_factory_.GetWeakPtr()));
  dispatcher_->PostDelayedTask(trial_timeout_.callback(),
                               trial_timeout_seconds_ * 1000);
}

bool ConnectivityTrial::IsActive() {
  return is_active_;
}

void ConnectivityTrial::RequestReadCallback(const ByteString& response_data) {
  const string response_expected(kResponseExpected);
  bool expected_length_received = false;
  int compare_length = 0;
  if (response_data.GetLength() < response_expected.length()) {
    // There isn't enough data yet for a final decision, but we can still
    // test to see if the partial string matches so far.
    expected_length_received = false;
    compare_length = response_data.GetLength();
  } else {
    expected_length_received = true;
    compare_length = response_expected.length();
  }

  if (base::MatchPattern(
          string(reinterpret_cast<const char*>(response_data.GetConstData()),
                 compare_length),
          response_expected.substr(0, compare_length))) {
    if (expected_length_received) {
      CompleteTrial(Result(kPhaseContent, kStatusSuccess));
    }
    // Otherwise, we wait for more data from the server.
  } else {
    CompleteTrial(Result(kPhaseContent, kStatusFailure));
  }
}

void ConnectivityTrial::RequestResultCallback(
    HTTPRequest::Result result, const ByteString& /*response_data*/) {
  CompleteTrial(GetPortalResultForRequestResult(result));
}

void ConnectivityTrial::CompleteTrial(Result result) {
  SLOG(connection_.get(), 3)
      << StringPrintf("Connectivity Trial completed with phase==%s, status==%s",
                      PhaseToString(result.phase).c_str(),
                      StatusToString(result.status).c_str());
  CleanupTrial(false);
  trial_callback_.Run(result);
}

void ConnectivityTrial::CleanupTrial(bool reset_request) {
  trial_timeout_.Cancel();

  if (request_.get())
    request_->Stop();

  is_active_ = false;

  if (!reset_request || !request_.get())
    return;

  request_.reset();
}

void ConnectivityTrial::TimeoutTrialTask() {
  LOG(ERROR) << "Connectivity Trial - Request timed out";
  if (request_->response_data().GetLength()) {
    CompleteTrial(ConnectivityTrial::Result(ConnectivityTrial::kPhaseContent,
                                            ConnectivityTrial::kStatusTimeout));
  } else {
    CompleteTrial(ConnectivityTrial::Result(ConnectivityTrial::kPhaseUnknown,
                                            ConnectivityTrial::kStatusTimeout));
  }
}

// statiic
const string ConnectivityTrial::PhaseToString(Phase phase) {
  switch (phase) {
    case kPhaseConnection:
      return kPortalDetectionPhaseConnection;
    case kPhaseDNS:
      return kPortalDetectionPhaseDns;
    case kPhaseHTTP:
      return kPortalDetectionPhaseHttp;
    case kPhaseContent:
      return kPortalDetectionPhaseContent;
    case kPhaseUnknown:
    default:
      return kPortalDetectionPhaseUnknown;
  }
}

// static
const string ConnectivityTrial::StatusToString(Status status) {
  switch (status) {
    case kStatusSuccess:
      return kPortalDetectionStatusSuccess;
    case kStatusTimeout:
      return kPortalDetectionStatusTimeout;
    case kStatusFailure:
    default:
      return kPortalDetectionStatusFailure;
  }
}

ConnectivityTrial::Result ConnectivityTrial::GetPortalResultForRequestResult(
    HTTPRequest::Result result) {
  switch (result) {
    case HTTPRequest::kResultSuccess:
      // The request completed without receiving the expected payload.
      return Result(kPhaseContent, kStatusFailure);
    case HTTPRequest::kResultDNSFailure:
      return Result(kPhaseDNS, kStatusFailure);
    case HTTPRequest::kResultDNSTimeout:
      return Result(kPhaseDNS, kStatusTimeout);
    case HTTPRequest::kResultConnectionFailure:
      return Result(kPhaseConnection, kStatusFailure);
    case HTTPRequest::kResultConnectionTimeout:
      return Result(kPhaseConnection, kStatusTimeout);
    case HTTPRequest::kResultRequestFailure:
    case HTTPRequest::kResultResponseFailure:
      return Result(kPhaseHTTP, kStatusFailure);
    case HTTPRequest::kResultRequestTimeout:
    case HTTPRequest::kResultResponseTimeout:
      return Result(kPhaseHTTP, kStatusTimeout);
    case HTTPRequest::kResultUnknown:
    default:
      return Result(kPhaseUnknown, kStatusFailure);
  }
}

}  // namespace shill
