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

#ifndef SYSTEM_NATIVEPOWER_INCLUDE_NATIVEPOWER_WAKE_LOCK_H_
#define SYSTEM_NATIVEPOWER_INCLUDE_NATIVEPOWER_WAKE_LOCK_H_

#include <string>

#include <base/macros.h>
#include <utils/StrongPointer.h>

namespace android {

class IBinder;
class PowerManagerClient;

// RAII-style class that prevents the system from suspending.
//
// Instantiate by calling PowerManagerClient::CreateWakeLock().
class WakeLock {
 public:
  ~WakeLock();

 private:
  friend class PowerManagerClient;

  // Ownership of |client| remains with the caller.
  WakeLock(const std::string& tag,
           const std::string& package,
           PowerManagerClient* client);

  // Initializes the object and acquires the lock, returning true on success.
  bool Init();

  // Was a lock successfully acquired from the power manager?
  bool acquired_lock_;

  std::string tag_;
  std::string package_;

  // Weak pointer to the client that created this wake lock.
  PowerManagerClient* client_;

  // Locally-created binder passed to the power manager.
  sp<IBinder> lock_binder_;

  DISALLOW_COPY_AND_ASSIGN(WakeLock);
};

}  // namespace android

#endif  // SYSTEM_NATIVEPOWER_INCLUDE_NATIVEPOWER_WAKE_LOCK_H_
