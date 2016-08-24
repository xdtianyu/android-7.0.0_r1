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

#include "shill/event_dispatcher.h"

#include <stdio.h>

#include <base/callback.h>
#include <base/location.h>
#include <base/message_loop/message_loop.h>
#include <base/run_loop.h>
#include <base/time/time.h>

using base::Callback;
using base::Closure;

namespace shill {

EventDispatcher::EventDispatcher()
    : io_handler_factory_(
          IOHandlerFactoryContainer::GetInstance()->GetIOHandlerFactory()) {
}

EventDispatcher::~EventDispatcher() {}

void EventDispatcher::DispatchForever() {
  base::MessageLoop::current()->Run();
}

void EventDispatcher::DispatchPendingEvents() {
  base::RunLoop().RunUntilIdle();
}

void EventDispatcher::PostTask(const Closure& task) {
  base::MessageLoop::current()->PostTask(FROM_HERE, task);
}

void EventDispatcher::PostDelayedTask(const Closure& task, int64_t delay_ms) {
  base::MessageLoop::current()->PostDelayedTask(
      FROM_HERE, task, base::TimeDelta::FromMilliseconds(delay_ms));
}

// TODO(zqiu): Remove all reference to this function and use the
// IOHandlerFactory function directly. Delete this function once
// all references are removed.
IOHandler* EventDispatcher::CreateInputHandler(
    int fd,
    const IOHandler::InputCallback& input_callback,
    const IOHandler::ErrorCallback& error_callback) {
  return io_handler_factory_->CreateIOInputHandler(
          fd, input_callback, error_callback);
}

// TODO(zqiu): Remove all reference to this function and use the
// IOHandlerFactory function directly. Delete this function once
// all references are removed.
IOHandler* EventDispatcher::CreateReadyHandler(
    int fd,
    IOHandler::ReadyMode mode,
    const Callback<void(int)>& ready_callback) {
  return io_handler_factory_->CreateIOReadyHandler(
          fd, mode, ready_callback);
}

}  // namespace shill
