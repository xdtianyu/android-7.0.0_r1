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

#include "wake_lock_manager.h"

#include <base/bind.h>
#include <base/files/file_util.h>
#include <base/format_macros.h>
#include <base/logging.h>
#include <base/strings/stringprintf.h>
#include <binder/IBinder.h>
#include <binderwrapper/binder_wrapper.h>

namespace android {
namespace {

// Paths to the sysfs lock and unlock files.
const char kLockPath[] = "/sys/power/wake_lock";
const char kUnlockPath[] = "/sys/power/wake_unlock";

// Writes |data| to |path|, returning true on success or logging an error and
// returning false otherwise.
bool WriteToFile(const base::FilePath& path, const std::string& data) {
  // This are sysfs "files" in real life, so it doesn't matter if we overwrite
  // them or append to them, but appending makes it easier for tests to detect
  // multiple writes when using real temporary files.
  VLOG(1) << "Writing \"" << data << "\" to " << path.value();
  if (!base::AppendToFile(path, data.data(), data.size())) {
    PLOG(ERROR) << "Failed to write \"" << data << "\" to " << path.value();
    return false;
  }
  return true;
}

}  // namespace

const char WakeLockManager::kLockName[] = "nativepowerman";

WakeLockManager::Request::Request(const std::string& tag,
                                  const std::string& package,
                                  uid_t uid)
    : tag(tag),
      package(package),
      uid(uid) {}

WakeLockManager::Request::Request(const Request& request) = default;

WakeLockManager::Request::Request() : uid(-1) {}

WakeLockManager::WakeLockManager()
    : lock_path_(kLockPath),
      unlock_path_(kUnlockPath) {}

WakeLockManager::~WakeLockManager() {
  while (!requests_.empty())
    RemoveRequest(requests_.begin()->first);
}

bool WakeLockManager::Init() {
  if (!base::PathIsWritable(lock_path_) ||
      !base::PathIsWritable(unlock_path_)) {
    LOG(ERROR) << lock_path_.value() << " and/or " << unlock_path_.value()
               << " are not writable";
    return false;
  }
  return true;
}

bool WakeLockManager::AddRequest(sp<IBinder> client_binder,
                                 const std::string& tag,
                                 const std::string& package,
                                 uid_t uid) {
  const bool new_request = !requests_.count(client_binder);
  LOG(INFO) << (new_request ? "Adding" : "Updating") << " request for binder "
            << client_binder.get() << ": tag=\"" << tag << "\""
            << " package=\"" << package << "\" uid=" << uid;

  const bool first_request = requests_.empty();

  if (new_request) {
    if (!BinderWrapper::Get()->RegisterForDeathNotifications(
            client_binder,
            base::Bind(&WakeLockManager::HandleBinderDeath,
                       base::Unretained(this), client_binder))) {
      return false;
    }
  }
  requests_[client_binder] = Request(tag, package, uid);

  if (first_request && !WriteToFile(lock_path_, kLockName))
    return false;

  return true;
}

bool WakeLockManager::RemoveRequest(sp<IBinder> client_binder) {
  LOG(INFO) << "Removing request for binder " << client_binder.get();

  if (!requests_.erase(client_binder)) {
    LOG(WARNING) << "Ignoring removal request for unknown binder "
                 << client_binder.get();
    return false;
  }
  BinderWrapper::Get()->UnregisterForDeathNotifications(client_binder);

  if (requests_.empty() && !WriteToFile(unlock_path_, kLockName))
    return false;

  return true;
}

void WakeLockManager::HandleBinderDeath(sp<IBinder> binder) {
  LOG(INFO) << "Received death notification for binder " << binder.get();
  RemoveRequest(binder);
}

}  // namespace android
