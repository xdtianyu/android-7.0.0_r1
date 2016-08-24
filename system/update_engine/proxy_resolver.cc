//
// Copyright (C) 2010 The Android Open Source Project
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

#include "update_engine/proxy_resolver.h"

#include <base/bind.h>
#include <base/location.h>

using brillo::MessageLoop;
using std::deque;
using std::string;

namespace chromeos_update_engine {

const char kNoProxy[] = "direct://";

DirectProxyResolver::~DirectProxyResolver() {
  if (idle_callback_id_ != MessageLoop::kTaskIdNull) {
    // The DirectProxyResolver is instantiated as part of the UpdateAttempter
    // which is also instantiated by default by the FakeSystemState, even when
    // it is not used. We check the manage_shares_id_ before calling the
    // MessageLoop::current() since the unit test using a FakeSystemState may
    // have not define a MessageLoop for the current thread.
    MessageLoop::current()->CancelTask(idle_callback_id_);
    idle_callback_id_ = MessageLoop::kTaskIdNull;
  }
}

bool DirectProxyResolver::GetProxiesForUrl(const string& url,
                                           ProxiesResolvedFn callback,
                                           void* data) {
  idle_callback_id_ = MessageLoop::current()->PostTask(
      FROM_HERE,
      base::Bind(
            &DirectProxyResolver::ReturnCallback,
            base::Unretained(this),
            callback,
            data));
  return true;
}

void DirectProxyResolver::ReturnCallback(ProxiesResolvedFn callback,
                                         void* data) {
  idle_callback_id_ = MessageLoop::kTaskIdNull;

  // Initialize proxy pool with as many proxies as indicated (all identical).
  deque<string> proxies(num_proxies_, kNoProxy);

  (*callback)(proxies, data);
}


}  // namespace chromeos_update_engine
