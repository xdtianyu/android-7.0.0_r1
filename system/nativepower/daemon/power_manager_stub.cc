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

#include <base/format_macros.h>
#include <base/logging.h>
#include <base/strings/stringprintf.h>
#include <binderwrapper/binder_wrapper.h>
#include <nativepower/power_manager_stub.h>
#include <utils/String8.h>

#include "wake_lock_manager_stub.h"

namespace android {

PowerManagerStub::SuspendRequest::SuspendRequest(int64_t event_time_ms,
                                                 int reason,
                                                 int flags)
    : event_time_ms(event_time_ms),
      reason(reason),
      flags(flags) {}

// static
std::string PowerManagerStub::ConstructWakeLockString(
    const std::string& tag,
    const std::string& package,
    uid_t uid) {
  return WakeLockManagerStub::ConstructRequestString(tag, package, uid);
}

// static
std::string PowerManagerStub::ConstructSuspendRequestString(
    int64_t event_time_ms,
    int reason,
    int flags) {
  return base::StringPrintf("%" PRId64 ",%d,%d", event_time_ms, reason, flags);
}

PowerManagerStub::PowerManagerStub()
    : wake_lock_manager_(new WakeLockManagerStub()) {}

PowerManagerStub::~PowerManagerStub() = default;

int PowerManagerStub::GetNumWakeLocks() const {
  return wake_lock_manager_->num_requests();
}

std::string PowerManagerStub::GetWakeLockString(
    const sp<IBinder>& binder) const {
  return wake_lock_manager_->GetRequestString(binder);
}

std::string PowerManagerStub::GetSuspendRequestString(size_t index) const {
  if (index >= suspend_requests_.size())
    return std::string();

  const SuspendRequest& request = suspend_requests_[index];
  return ConstructSuspendRequestString(request.event_time_ms, request.reason,
                                       request.flags);
}

status_t PowerManagerStub::acquireWakeLock(int flags,
                                           const sp<IBinder>& lock,
                                           const String16& tag,
                                           const String16& packageName,
                                           bool isOneWay) {
  CHECK(wake_lock_manager_->AddRequest(lock, String8(tag).string(),
                                       String8(packageName).string(),
                                       BinderWrapper::Get()->GetCallingUid()));
  return OK;
}

status_t PowerManagerStub::acquireWakeLockWithUid(int flags,
                                                  const sp<IBinder>& lock,
                                                  const String16& tag,
                                                  const String16& packageName,
                                                  int uid,
                                                  bool isOneWay) {
  CHECK(wake_lock_manager_->AddRequest(lock, String8(tag).string(),
                                       String8(packageName).string(),
                                       static_cast<uid_t>(uid)));
  return OK;
}

status_t PowerManagerStub::releaseWakeLock(const sp<IBinder>& lock,
                                           int flags,
                                           bool isOneWay) {
  CHECK(wake_lock_manager_->RemoveRequest(lock));
  return OK;
}

status_t PowerManagerStub::updateWakeLockUids(const sp<IBinder>& lock,
                                              int len,
                                              const int* uids,
                                              bool isOneWay) {
  return OK;
}

status_t PowerManagerStub::powerHint(int hintId, int data) {
  return OK;
}

status_t PowerManagerStub::goToSleep(int64_t event_time_ms,
                                     int reason,
                                     int flags) {
  suspend_requests_.emplace_back(event_time_ms, reason, flags);
  return OK;
}

status_t PowerManagerStub::reboot(bool confirm,
                                  const String16& reason,
                                  bool wait) {
  reboot_reasons_.push_back(String8(reason).string());
  return OK;
}

status_t PowerManagerStub::shutdown(bool confirm,
                                    const String16& reason,
                                    bool wait) {
  shutdown_reasons_.push_back(String8(reason).string());
  return OK;
}

status_t PowerManagerStub::crash(const String16& message) {
  return OK;
}

}  // namespace android
