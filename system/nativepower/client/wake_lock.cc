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

#include <nativepower/wake_lock.h>

#include <base/logging.h>
#include <binderwrapper/binder_wrapper.h>
#include <nativepower/power_manager_client.h>
#include <powermanager/IPowerManager.h>
#include <powermanager/PowerManager.h>

namespace android {

WakeLock::WakeLock(const std::string& tag,
                   const std::string& package,
                   PowerManagerClient* client)
    : acquired_lock_(false),
      tag_(tag),
      package_(package),
      client_(client) {
  DCHECK(client_);
}

WakeLock::~WakeLock() {
  sp<IPowerManager> power_manager = client_->power_manager();
  if (acquired_lock_ && power_manager.get()) {
    status_t status =
        power_manager->releaseWakeLock(lock_binder_, 0 /* flags */);
    if (status != OK) {
      LOG(ERROR) << "Wake lock release request for \"" << tag_ << "\" failed "
                 << "with status " << status;
    }
  }
}

bool WakeLock::Init() {
  sp<IPowerManager> power_manager = client_->power_manager();
  if (!power_manager.get()) {
    LOG(ERROR) << "Can't acquire wake lock for \"" << tag_ << "\"; no "
               << "connection to power manager";
    return false;
  }

  lock_binder_ = BinderWrapper::Get()->CreateLocalBinder();
  status_t status = power_manager->acquireWakeLock(
      POWERMANAGER_PARTIAL_WAKE_LOCK,
      lock_binder_, String16(tag_.c_str()), String16(package_.c_str()));
  if (status != OK) {
    LOG(ERROR) << "Wake lock acquire request for \"" << tag_ << "\" failed "
               << "with status " << status;
    return false;
  }

  acquired_lock_ = true;
  return true;
}

}  // namespace android
