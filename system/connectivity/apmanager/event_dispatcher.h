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

#ifndef APMANAGER_EVENT_DISPATCHER_H_
#define APMANAGER_EVENT_DISPATCHER_H_

#include <base/callback.h>
#include <base/lazy_instance.h>

namespace apmanager {

// Singleton class for dispatching tasks to current message loop.
class EventDispatcher {
 public:
  virtual ~EventDispatcher();

  // This is a singleton. Use EventDispatcher::GetInstance()->Foo().
  static EventDispatcher* GetInstance();

  // These are thin wrappers around calls of the same name in
  // <base/message_loop_proxy.h>
  virtual bool PostTask(const base::Closure& task);
  virtual bool PostDelayedTask(const base::Closure& task,
                               int64_t delay_ms);

 protected:
  EventDispatcher();

 private:
  friend struct base::DefaultLazyInstanceTraits<EventDispatcher>;

  DISALLOW_COPY_AND_ASSIGN(EventDispatcher);
};

}  // namespace apmanager

#endif  // APMANAGER_EVENT_DISPATCHER_H_
