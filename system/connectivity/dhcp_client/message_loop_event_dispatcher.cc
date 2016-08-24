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

#include "dhcp_client/message_loop_event_dispatcher.h"

#include <base/location.h>
#include <base/message_loop/message_loop.h>
#include <base/time/time.h>

namespace dhcp_client {

MessageLoopEventDispatcher::MessageLoopEventDispatcher() {}
MessageLoopEventDispatcher::~MessageLoopEventDispatcher() {}

bool MessageLoopEventDispatcher::PostTask(const base::Closure& task) {
  if (!base::MessageLoop::current())
    return false;
  base::MessageLoop::current()->PostTask(FROM_HERE, task);
  return true;
}

bool MessageLoopEventDispatcher::PostDelayedTask(const base::Closure& task,
                                                 int64_t delay_ms) {
  if (!base::MessageLoop::current())
    return false;
  base::MessageLoop::current()->PostDelayedTask(
      FROM_HERE, task, base::TimeDelta::FromMilliseconds(delay_ms));
  return true;
}

}  // namespace dhcp_client
