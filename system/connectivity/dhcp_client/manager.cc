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

#include "dhcp_client/manager.h"
#include "dhcp_client/service.h"

#include "dhcp_client/message_loop_event_dispatcher.h"

namespace dhcp_client {

Manager::Manager()
    : service_identifier_(0),
      event_dispatcher_(new MessageLoopEventDispatcher()) {
}

Manager::~Manager() {}

scoped_refptr<Service> Manager::StartService(
    const brillo::VariantDictionary& configs) {
  scoped_refptr<Service> service = new Service(this,
                                               service_identifier_++,
                                               event_dispatcher_.get(),
                                               configs);
  services_.push_back(service);
  return service;
}

bool Manager::StopService(const scoped_refptr<Service>& service) {
  for (auto it = services_.begin(); it != services_.end(); ++it) {
    if (*it == service) {
      services_.erase(it);
      return true;
    }
  }
  return false;
}

}  // namespace dhcp_client

