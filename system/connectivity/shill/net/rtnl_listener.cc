//
// Copyright (C) 2011 The Android Open Source Project
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

#include "shill/net/rtnl_listener.h"

#include "shill/net/rtnl_handler.h"

using base::Callback;

namespace shill {

RTNLListener::RTNLListener(int listen_flags,
                           const Callback<void(const RTNLMessage&)>& callback)
    : RTNLListener{listen_flags, callback, RTNLHandler::GetInstance()} {}

RTNLListener::RTNLListener(int listen_flags,
                           const Callback<void(const RTNLMessage&)>& callback,
                           RTNLHandler* rtnl_handler)
    : listen_flags_(listen_flags),
      callback_(callback),
      rtnl_handler_(rtnl_handler) {
  rtnl_handler_->AddListener(this);
}

RTNLListener::~RTNLListener() {
  rtnl_handler_->RemoveListener(this);
}

void RTNLListener::NotifyEvent(int type, const RTNLMessage& msg) {
  if ((type & listen_flags_) != 0)
    callback_.Run(msg);
}

}  // namespace shill
