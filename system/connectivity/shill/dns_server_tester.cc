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

#include "shill/dns_server_tester.h"

#include <string>

#include <base/bind.h>
#include <base/strings/string_number_conversions.h>
#include <base/strings/string_util.h>
#include <base/strings/stringprintf.h>

#include "shill/connection.h"
#include "shill/dns_client.h"
#include "shill/dns_client_factory.h"
#include "shill/error.h"
#include "shill/event_dispatcher.h"

using base::Bind;
using base::Callback;
using std::vector;
using std::string;

namespace shill {

// static
const char DNSServerTester::kDNSTestHostname[] = "www.gstatic.com";
// static
const int DNSServerTester::kDNSTestRetryIntervalMilliseconds = 60000;
// static
const int DNSServerTester::kDNSTimeoutMilliseconds = 5000;

DNSServerTester::DNSServerTester(ConnectionRefPtr connection,
                                 EventDispatcher* dispatcher,
                                 const vector<string>& dns_servers,
                                 const bool retry_until_success,
                                 const Callback<void(const Status)>& callback)
    : connection_(connection),
      dispatcher_(dispatcher),
      retry_until_success_(retry_until_success),
      weak_ptr_factory_(this),
      dns_result_callback_(callback),
      dns_client_callback_(Bind(&DNSServerTester::DNSClientCallback,
                                weak_ptr_factory_.GetWeakPtr())),
      dns_test_client_(DNSClientFactory::GetInstance()->CreateDNSClient(
          IPAddress::kFamilyIPv4,
          connection_->interface_name(),
          dns_servers,
          kDNSTimeoutMilliseconds,
          dispatcher_,
          dns_client_callback_)) {}

DNSServerTester::~DNSServerTester() {
  Stop();
}

void DNSServerTester::Start() {
  // Stop existing attempt.
  Stop();
  // Schedule the test to start immediately.
  StartAttempt(0);
}

void DNSServerTester::StartAttempt(int delay_ms) {
  start_attempt_.Reset(Bind(&DNSServerTester::StartAttemptTask,
                            weak_ptr_factory_.GetWeakPtr()));
  dispatcher_->PostDelayedTask(start_attempt_.callback(), delay_ms);
}

void DNSServerTester::StartAttemptTask() {
  Error error;
  if (!dns_test_client_->Start(kDNSTestHostname, &error)) {
    LOG(ERROR) << __func__ << ": Failed to start DNS client "
                                << error.message();
    CompleteAttempt(kStatusFailure);
  }
}

void DNSServerTester::Stop() {
  start_attempt_.Cancel();
  StopAttempt();
}

void DNSServerTester::StopAttempt() {
  if (dns_test_client_.get()) {
    dns_test_client_->Stop();
  }
}

void DNSServerTester::CompleteAttempt(Status status) {
  if (status == kStatusFailure && retry_until_success_) {
    // Schedule the test to restart after retry timeout interval.
    StartAttempt(kDNSTestRetryIntervalMilliseconds);
    return;
  }

  dns_result_callback_.Run(status);
}

void DNSServerTester::DNSClientCallback(const Error& error,
                                        const IPAddress& ip) {
  Status status = kStatusSuccess;
  if (!error.IsSuccess()) {
    status = kStatusFailure;
  }

  CompleteAttempt(status);
}

}  // namespace shill
