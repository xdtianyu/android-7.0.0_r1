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

#ifndef SYSTEM_NATIVEPOWER_DAEMON_WAKE_LOCK_MANAGER_H_
#define SYSTEM_NATIVEPOWER_DAEMON_WAKE_LOCK_MANAGER_H_

#include <sys/types.h>

#include <map>
#include <string>

#include <base/files/file_path.h>
#include <base/macros.h>
#include <base/time/time.h>
#include <utils/StrongPointer.h>

namespace android {

class IBinder;

class WakeLockManagerInterface {
 public:
  WakeLockManagerInterface() {}
  virtual ~WakeLockManagerInterface() {}

  virtual bool AddRequest(sp<IBinder> client_binder,
                          const std::string& tag,
                          const std::string& package,
                          uid_t uid) = 0;
  virtual bool RemoveRequest(sp<IBinder> client_binder) = 0;

 protected:
  // Information about a request from a client.
  struct Request {
    Request(const std::string& tag, const std::string& package, uid_t uid);
    Request(const Request& request);
    Request();

    std::string tag;
    std::string package;
    uid_t uid;
  };
};

class WakeLockManager : public WakeLockManagerInterface {
 public:
  // Name of the kernel wake lock created by this class.
  static const char kLockName[];

  WakeLockManager();
  ~WakeLockManager() override;

  void set_paths_for_testing(const base::FilePath& lock_path,
                             const base::FilePath& unlock_path) {
    lock_path_ = lock_path;
    unlock_path_ = unlock_path;
  }

  bool Init();

  // WakeLockManagerInterface:
  bool AddRequest(sp<IBinder> client_binder,
                  const std::string& tag,
                  const std::string& package,
                  uid_t uid) override;
  bool RemoveRequest(sp<IBinder> client_binder) override;

 private:
  void HandleBinderDeath(sp<IBinder> binder);

  base::FilePath lock_path_;
  base::FilePath unlock_path_;

  // Currently-active requests, keyed by client binders.
  std::map<sp<IBinder>, Request> requests_;

  DISALLOW_COPY_AND_ASSIGN(WakeLockManager);
};

}  // namespace android

#endif  // SYSTEM_NATIVEPOWER_DAEMON_WAKE_LOCK_MANAGER_H_
