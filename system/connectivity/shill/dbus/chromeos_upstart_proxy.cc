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

#include "shill/dbus/chromeos_upstart_proxy.h"

#include <base/bind.h>

#include "shill/logging.h"

using std::string;
using std::vector;

namespace shill {

// static.
const char ChromeosUpstartProxy::kServiceName[] = "com.ubuntu.Upstart";

ChromeosUpstartProxy::ChromeosUpstartProxy(const scoped_refptr<dbus::Bus>& bus)
    : upstart_proxy_(
        new com::ubuntu::Upstart0_6Proxy(bus, kServiceName)) {}

void ChromeosUpstartProxy::EmitEvent(
    const string& name, const vector<string>& env, bool wait) {
  upstart_proxy_->EmitEventAsync(
      name,
      env,
      wait,
      base::Bind(&ChromeosUpstartProxy::OnEmitEventSuccess,
                 weak_factory_.GetWeakPtr()),
      base::Bind(&ChromeosUpstartProxy::OnEmitEventFailure,
                 weak_factory_.GetWeakPtr()));
}

void ChromeosUpstartProxy::OnEmitEventSuccess() {
  VLOG(2) << "Event emitted successful";
}

void ChromeosUpstartProxy::OnEmitEventFailure(brillo::Error* error) {
  LOG(ERROR) << "Failed to emit event: " << error->GetCode()
      << " " << error->GetMessage();
}

}  // namespace shill
