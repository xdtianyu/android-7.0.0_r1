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

#include "shill/result_aggregator.h"

#include "shill/event_dispatcher.h"
#include "shill/logging.h"

namespace shill {

ResultAggregator::ResultAggregator(const ResultCallback& callback)
    : ResultAggregator(callback, nullptr, -1) {}

ResultAggregator::ResultAggregator(const ResultCallback& callback,
                                   EventDispatcher* dispatcher,
                                   int timeout_milliseconds)
    : weak_ptr_factory_(this),
      callback_(callback),
      timeout_callback_(base::Bind(&ResultAggregator::Timeout,
                                   weak_ptr_factory_.GetWeakPtr())),
      got_result_(false),
      timed_out_(false) {
  CHECK(!callback.is_null());
  if (dispatcher && timeout_milliseconds >= 0) {
    dispatcher->PostDelayedTask(timeout_callback_.callback(),
                                timeout_milliseconds);
  }
}

ResultAggregator::~ResultAggregator() {
  if (got_result_ && !timed_out_) {
    callback_.Run(error_);
  }
  // timeout_callback_ will automatically be canceled when its destructor
  // is invoked.
}

void ResultAggregator::ReportResult(const Error& error) {
  LOG(INFO) << "Error type " << error << " reported";
  CHECK(!error.IsOngoing());  // We want the final result.
  got_result_ = true;
  if (error_.IsSuccess()) {  // Only copy first |error|.
    error_.CopyFrom(error);
  } else {
    LOG(WARNING) << "Dropping error type " << error;
  }
}

void ResultAggregator::Timeout() {
  LOG(WARNING) << "Results aggregator timed out";
  timed_out_ = true;
  error_.Populate(Error::kOperationTimeout);
  callback_.Run(error_);
}

}  // namespace shill
