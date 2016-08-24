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

#ifndef SYSTEM_NATIVEPOWER_DAEMON_WAKE_LOCK_MANAGER_STUB_H_
#define SYSTEM_NATIVEPOWER_DAEMON_WAKE_LOCK_MANAGER_STUB_H_

#include <set>
#include <string>
#include <sys/types.h>

#include <base/macros.h>
#include <utils/StrongPointer.h>

#include "wake_lock_manager.h"

namespace android {

class IBinder;

// Stub implementation used by tests.
class WakeLockManagerStub : public WakeLockManagerInterface {
 public:
  WakeLockManagerStub();
  ~WakeLockManagerStub() override;

  // Constructs a string that can be compared with one returned by
  // GetRequestString().
  static std::string ConstructRequestString(const std::string& tag,
                                            const std::string& package,
                                            uid_t uid);

  int num_requests() const { return requests_.size(); }

  // Returns a string describing the request associated with |binder|, or an
  // empty string if no request is present.
  std::string GetRequestString(const sp<IBinder>& binder) const;

  // WakeLockManagerInterface:
  bool AddRequest(sp<IBinder> client_binder,
                  const std::string& tag,
                  const std::string& package,
                  uid_t uid) override;
  bool RemoveRequest(sp<IBinder> client_binder) override;

 private:
  // Currently-active requests, keyed by client binders.
  std::map<sp<IBinder>, Request> requests_;

  DISALLOW_COPY_AND_ASSIGN(WakeLockManagerStub);
};

}  // namespace android

#endif  // SYSTEM_NATIVEPOWER_DAEMON_WAKE_LOCK_MANAGER_STUB_H_
