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

#ifndef DHCP_CLIENT_EVENT_DISPATCHER_INTERFACE_H_
#define DHCP_CLIENT_EVENT_DISPATCHER_INTERFACE_H_

#include <base/callback.h>

namespace dhcp_client {

// Abstract class for dispatching tasks.
class EventDispatcherInterface {
 public:
  virtual ~EventDispatcherInterface() {}

  virtual bool PostTask(const base::Closure& task) = 0;
  virtual bool PostDelayedTask(const base::Closure& task,
                               int64_t delay_ms) = 0;
};

}  // namespace dhcp_client

#endif  // DHCP_CLIENT_EVENT_DISPATCHER_INTERFACE_H_
