//
// Copyright (C) 2009 The Android Open Source Project
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

#include "update_engine/common/mock_http_fetcher.h"

#include <algorithm>

#include <base/bind.h>
#include <base/logging.h>
#include <base/strings/string_util.h>
#include <base/time/time.h>
#include <gtest/gtest.h>

// This is a mock implementation of HttpFetcher which is useful for testing.

using brillo::MessageLoop;
using std::min;

namespace chromeos_update_engine {

MockHttpFetcher::~MockHttpFetcher() {
  CHECK(timeout_id_ == MessageLoop::kTaskIdNull) <<
      "Call TerminateTransfer() before dtor.";
}

void MockHttpFetcher::BeginTransfer(const std::string& url) {
  EXPECT_FALSE(never_use_);
  if (fail_transfer_ || data_.empty()) {
    // No data to send, just notify of completion..
    SignalTransferComplete();
    return;
  }
  if (sent_size_ < data_.size())
    SendData(true);
}

// Returns false on one condition: If timeout_id_ was already set
// and it needs to be deleted by the caller. If timeout_id_ is null
// when this function is called, this function will always return true.
bool MockHttpFetcher::SendData(bool skip_delivery) {
  if (fail_transfer_) {
    SignalTransferComplete();
    return timeout_id_ != MessageLoop::kTaskIdNull;
  }

  CHECK_LT(sent_size_, data_.size());
  if (!skip_delivery) {
    const size_t chunk_size = min(kMockHttpFetcherChunkSize,
                                  data_.size() - sent_size_);
    CHECK(delegate_);
    delegate_->ReceivedBytes(this, &data_[sent_size_], chunk_size);
    // We may get terminated in the callback.
    if (sent_size_ == data_.size()) {
      LOG(INFO) << "Terminated in the ReceivedBytes callback.";
      return timeout_id_ != MessageLoop::kTaskIdNull;
    }
    sent_size_ += chunk_size;
    CHECK_LE(sent_size_, data_.size());
    if (sent_size_ == data_.size()) {
      // We've sent all the data. Notify of success.
      SignalTransferComplete();
    }
  }

  if (paused_) {
    // If we're paused, we should return true if timeout_id_ is set,
    // since we need the caller to delete it.
    return timeout_id_ != MessageLoop::kTaskIdNull;
  }

  if (timeout_id_ != MessageLoop::kTaskIdNull) {
    // we still need a timeout if there's more data to send
    return sent_size_ < data_.size();
  } else if (sent_size_ < data_.size()) {
    // we don't have a timeout source and we need one
    timeout_id_ = MessageLoop::current()->PostDelayedTask(
        FROM_HERE,
        base::Bind(&MockHttpFetcher::TimeoutCallback, base::Unretained(this)),
        base::TimeDelta::FromMilliseconds(10));
  }
  return true;
}

void MockHttpFetcher::TimeoutCallback() {
  CHECK(!paused_);
  if (SendData(false)) {
    // We need to re-schedule the timeout.
    timeout_id_ = MessageLoop::current()->PostDelayedTask(
        FROM_HERE,
        base::Bind(&MockHttpFetcher::TimeoutCallback, base::Unretained(this)),
        base::TimeDelta::FromMilliseconds(10));
  } else {
    timeout_id_ = MessageLoop::kTaskIdNull;
  }
}

// If the transfer is in progress, aborts the transfer early.
// The transfer cannot be resumed.
void MockHttpFetcher::TerminateTransfer() {
  LOG(INFO) << "Terminating transfer.";
  sent_size_ = data_.size();
  // Kill any timeout, it is ok to call with kTaskIdNull.
  MessageLoop::current()->CancelTask(timeout_id_);
  timeout_id_ = MessageLoop::kTaskIdNull;
  delegate_->TransferTerminated(this);
}

void MockHttpFetcher::SetHeader(const std::string& header_name,
                                const std::string& header_value) {
  extra_headers_[base::ToLowerASCII(header_name)] = header_value;
}

void MockHttpFetcher::Pause() {
  CHECK(!paused_);
  paused_ = true;
  MessageLoop::current()->CancelTask(timeout_id_);
  timeout_id_ = MessageLoop::kTaskIdNull;
}

void MockHttpFetcher::Unpause() {
  CHECK(paused_) << "You must pause before unpause.";
  paused_ = false;
  if (sent_size_ < data_.size()) {
    SendData(false);
  }
}

void MockHttpFetcher::FailTransfer(int http_response_code) {
  fail_transfer_ = true;
  http_response_code_ = http_response_code;
}

void MockHttpFetcher::SignalTransferComplete() {
  // If the transfer has been failed, the HTTP response code should be set
  // already.
  if (!fail_transfer_) {
    http_response_code_ = 200;
  }
  delegate_->TransferComplete(this, !fail_transfer_);
}

}  // namespace chromeos_update_engine
