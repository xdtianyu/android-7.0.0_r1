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

#ifndef SHILL_TEST_EVENT_DISPATCHER_H_
#define SHILL_TEST_EVENT_DISPATCHER_H_

#include <base/macros.h>
#include <base/message_loop/message_loop.h>
#include <brillo/message_loops/base_message_loop.h>

#include "shill/event_dispatcher.h"

namespace shill {

// Event dispatcher with message loop for testing.
class EventDispatcherForTest : public EventDispatcher {
 public:
  EventDispatcherForTest() {
    chromeos_message_loop_.SetAsCurrent();
  }
  ~EventDispatcherForTest() override {}

 private:
  // Message loop for testing.
  base::MessageLoopForIO message_loop_;
  // The chromeos wrapper for the main message loop.
  brillo::BaseMessageLoop chromeos_message_loop_{&message_loop_};

  DISALLOW_COPY_AND_ASSIGN(EventDispatcherForTest);
};

}  // namespace shill

#endif  // SHILL_TEST_EVENT_DISPATCHER_H_
