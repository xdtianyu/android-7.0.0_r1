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

#ifndef SHILL_UPSTART_UPSTART_H_
#define SHILL_UPSTART_UPSTART_H_

#include <memory>

#include <base/macros.h>

#include "shill/upstart/upstart_proxy_interface.h"

namespace shill {

class ControlInterface;

class Upstart {
 public:
  // |control_interface| creates the UpstartProxy. Use a fake for testing.
  explicit Upstart(ControlInterface* control_interface);
  virtual ~Upstart();

  // Report an event to upstart indicating that the system has disconnected.
  virtual void NotifyDisconnected();
  // Report an event to upstart indicating that the system has connected.
  virtual void NotifyConnected();

 private:
  // Event string to be provided to upstart to indicate we have disconnected.
  static const char kShillDisconnectEvent[];
  // Event string to be provided to upstart to indicate we have connected.
  static const char kShillConnectEvent[];

  // The upstart proxy created by this class.
  const std::unique_ptr<UpstartProxyInterface> upstart_proxy_;

  DISALLOW_COPY_AND_ASSIGN(Upstart);
};

}  // namespace shill

#endif  // SHILL_UPSTART_UPSTART_H_
