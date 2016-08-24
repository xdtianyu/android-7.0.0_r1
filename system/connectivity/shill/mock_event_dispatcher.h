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

#ifndef SHILL_MOCK_EVENT_DISPATCHER_H_
#define SHILL_MOCK_EVENT_DISPATCHER_H_

#include <base/macros.h>
#include <gmock/gmock.h>

#include "shill/event_dispatcher.h"

namespace shill {

class MockEventDispatcher : public EventDispatcher {
 public:
  MockEventDispatcher();
  ~MockEventDispatcher() override;

  MOCK_METHOD0(DispatchForever, void());
  MOCK_METHOD0(DispatchPendingEvents, void());
  MOCK_METHOD1(PostTask, void(const base::Closure& task));
  MOCK_METHOD2(PostDelayedTask, void(const base::Closure& task,
                                     int64_t delay_ms));
  MOCK_METHOD3(CreateInputHandler, IOHandler*(
      int fd,
      const IOHandler::InputCallback& input_callback,
      const IOHandler::ErrorCallback& error_callback));

  MOCK_METHOD3(CreateReadyHandler,
               IOHandler*(int fd,
                          IOHandler::ReadyMode mode,
                          const base::Callback<void(int)>& callback));

 private:
  DISALLOW_COPY_AND_ASSIGN(MockEventDispatcher);
};

}  // namespace shill

#endif  // SHILL_MOCK_EVENT_DISPATCHER_H_
