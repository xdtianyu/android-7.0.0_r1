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

#ifndef SHILL_EVENT_DISPATCHER_H_
#define SHILL_EVENT_DISPATCHER_H_

#include <memory>

#include <base/callback.h>
#include <base/macros.h>
#include <base/memory/ref_counted.h>
#include <base/message_loop/message_loop.h>

#include "shill/net/io_handler_factory_container.h"

namespace base {
class MessageLoopProxy;
}  // namespace base


namespace shill {

// This is the main event dispatcher.  It contains a central instance, and is
// the entity responsible for dispatching events out of all queues to their
// listeners during the idle loop.
class EventDispatcher {
 public:
  EventDispatcher();
  virtual ~EventDispatcher();

  virtual void DispatchForever();

  // Processes all pending events that can run and returns.
  virtual void DispatchPendingEvents();

  // These are thin wrappers around calls of the same name in
  // <base/message_loop_proxy.h>
  virtual void PostTask(const base::Closure& task);
  virtual void PostDelayedTask(const base::Closure& task, int64_t delay_ms);

  virtual IOHandler* CreateInputHandler(
      int fd,
      const IOHandler::InputCallback& input_callback,
      const IOHandler::ErrorCallback& error_callback);

  virtual IOHandler* CreateReadyHandler(
      int fd,
      IOHandler::ReadyMode mode,
      const IOHandler::ReadyCallback& ready_callback);

 private:
  IOHandlerFactory* io_handler_factory_;

  DISALLOW_COPY_AND_ASSIGN(EventDispatcher);
};

}  // namespace shill

#endif  // SHILL_EVENT_DISPATCHER_H_
