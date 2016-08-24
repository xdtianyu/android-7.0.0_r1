//
// Copyright (C) 2015 The Android Open Source Project
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

#include "trunks/background_command_transceiver.h"

#include <base/bind.h>
#include <base/callback.h>
#include <base/location.h>
#include <base/logging.h>
#include <base/single_thread_task_runner.h>
#include <base/synchronization/waitable_event.h>
#include <base/thread_task_runner_handle.h>

namespace {

// A simple callback useful when waiting for an asynchronous call.
void AssignAndSignal(std::string* destination,
                     base::WaitableEvent* event,
                     const std::string& source) {
  *destination = source;
  event->Signal();
}

// A callback which posts another |callback| to a given |task_runner|.
void PostCallbackToTaskRunner(
    const trunks::CommandTransceiver::ResponseCallback& callback,
    scoped_refptr<base::SingleThreadTaskRunner> task_runner,
    const std::string& response) {
  base::Closure task = base::Bind(callback, response);
  task_runner->PostTask(FROM_HERE, task);
}

}  // namespace

namespace trunks {

BackgroundCommandTransceiver::BackgroundCommandTransceiver(
    CommandTransceiver* next_transceiver,
    const scoped_refptr<base::SequencedTaskRunner>& task_runner)
    : next_transceiver_(next_transceiver),
      task_runner_(task_runner),
      weak_factory_(this) {}

BackgroundCommandTransceiver::~BackgroundCommandTransceiver() {}

void BackgroundCommandTransceiver::SendCommand(
    const std::string& command,
    const ResponseCallback& callback) {
  if (task_runner_.get()) {
    ResponseCallback background_callback = base::Bind(
        PostCallbackToTaskRunner,
        callback,
        base::ThreadTaskRunnerHandle::Get());
    // Use SendCommandTask instead of binding to next_transceiver_ directly to
    // leverage weak pointer semantics.
    base::Closure task = base::Bind(
        &BackgroundCommandTransceiver::SendCommandTask,
        GetWeakPtr(),
        command,
        background_callback);
    task_runner_->PostNonNestableTask(FROM_HERE, task);
  } else {
    next_transceiver_->SendCommand(command, callback);
  }
}

std::string BackgroundCommandTransceiver::SendCommandAndWait(
    const std::string& command) {
  if (task_runner_.get()) {
    std::string response;
    base::WaitableEvent response_ready(true,    // manual_reset
                                       false);  // initially_signaled
    ResponseCallback callback =
        base::Bind(&AssignAndSignal, &response, &response_ready);
    // Use SendCommandTask instead of binding to next_transceiver_ directly to
    // leverage weak pointer semantics.
    base::Closure task = base::Bind(
        &BackgroundCommandTransceiver::SendCommandTask,
        GetWeakPtr(),
        command,
        callback);
    task_runner_->PostNonNestableTask(FROM_HERE, task);
    response_ready.Wait();
    return response;
  } else {
    return next_transceiver_->SendCommandAndWait(command);
  }
}

void BackgroundCommandTransceiver::SendCommandTask(
    const std::string& command,
    const ResponseCallback& callback) {
  next_transceiver_->SendCommand(command, callback);
}

}  // namespace trunks
