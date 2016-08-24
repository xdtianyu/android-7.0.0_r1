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

#include "shill/upstart/upstart.h"

#include "shill/control_interface.h"
#include "shill/upstart/upstart_proxy_interface.h"

namespace shill {

// static
const char Upstart::kShillDisconnectEvent[] = "shill-disconnected";
const char Upstart::kShillConnectEvent[] = "shill-connected";

Upstart::Upstart(ControlInterface* control_interface)
    : upstart_proxy_(control_interface->CreateUpstartProxy()) {}

Upstart::~Upstart() {}

void Upstart::NotifyDisconnected() {
  upstart_proxy_->EmitEvent(kShillDisconnectEvent, {}, false);
}

void Upstart::NotifyConnected() {
  upstart_proxy_->EmitEvent(kShillConnectEvent, {}, false);
}

}  // namespace shill
