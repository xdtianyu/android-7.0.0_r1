/*
 * Copyright (C) 2015 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "wake_lock_manager_stub.h"

#include <base/strings/stringprintf.h>
#include <binder/IBinder.h>

namespace android {

// static
std::string WakeLockManagerStub::ConstructRequestString(
    const std::string& tag,
    const std::string& package,
    uid_t uid) {
  return base::StringPrintf("%s,%s,%d", tag.c_str(), package.c_str(), uid);
}

WakeLockManagerStub::WakeLockManagerStub() = default;

WakeLockManagerStub::~WakeLockManagerStub() = default;

std::string WakeLockManagerStub::GetRequestString(
    const sp<IBinder>& binder) const {
  const auto it = requests_.find(binder);
  if (it == requests_.end())
    return std::string();

  const Request& req = it->second;
  return ConstructRequestString(req.tag, req.package, req.uid);
}

bool WakeLockManagerStub::AddRequest(sp<IBinder> client_binder,
                                     const std::string& tag,
                                     const std::string& package,
                                     uid_t uid) {
  requests_[client_binder] = Request(tag, package, uid);
  return true;
}

bool WakeLockManagerStub::RemoveRequest(sp<IBinder> client_binder) {
  if (!requests_.count(client_binder))
    return false;

  requests_.erase(client_binder);
  return true;
}

}  // namespace android
