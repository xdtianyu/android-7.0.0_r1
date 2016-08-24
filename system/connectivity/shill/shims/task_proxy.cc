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

#include "shill/shims/task_proxy.h"

#include <base/logging.h>

using std::map;
using std::string;

namespace shill {

namespace shims {

TaskProxy::TaskProxy(scoped_refptr<dbus::Bus> bus,
                     const string& path,
                     const string& service)
      : proxy_(bus, dbus::ObjectPath(path)) {}

TaskProxy::~TaskProxy() {}

void TaskProxy::Notify(const string& reason, const map<string, string>& dict) {
  LOG(INFO) << __func__ << "(" << reason
            << ", argcount: " << dict.size() << ")";
  brillo::ErrorPtr error;
  if (!proxy_.notify(reason, dict, &error)) {
    LOG(ERROR) << "DBus error: " << error->GetCode() << ": "
               << error->GetMessage();
  }
}

bool TaskProxy::GetSecret(string* username, string* password) {
  LOG(INFO) << __func__;
  brillo::ErrorPtr error;
  if (!proxy_.getsec(username, password, &error)) {
    LOG(ERROR) << "DBus error: " << error->GetCode() << ": "
               << error->GetMessage();
    return false;
  }
  return true;
}

}  // namespace shims

}  // namespace shill
