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

#include "power_manager.h"

#include <base/files/file_util.h>
#include <base/logging.h>
#include <base/sys_info.h>
#include <binderwrapper/binder_wrapper.h>
#include <cutils/android_reboot.h>
#include <nativepower/constants.h>
#include <powermanager/IPowerManager.h>
#include <utils/Errors.h>
#include <utils/String8.h>

namespace android {
namespace {

// Path to real sysfs file that can be written to change the power state.
const char kDefaultPowerStatePath[] = "/sys/power/state";

}  // namespace

const char PowerManager::kRebootPrefix[] = "reboot,";
const char PowerManager::kShutdownPrefix[] = "shutdown,";
const char PowerManager::kPowerStateSuspend[] = "mem";

PowerManager::PowerManager()
    : power_state_path_(kDefaultPowerStatePath) {}

PowerManager::~PowerManager() = default;

bool PowerManager::Init() {
  if (!property_setter_)
    property_setter_.reset(new SystemPropertySetter());
  if (!wake_lock_manager_) {
    wake_lock_manager_.reset(new WakeLockManager());
    if (!static_cast<WakeLockManager*>(wake_lock_manager_.get())->Init())
      return false;
  }

  LOG(INFO) << "Registering with service manager as \""
            << kPowerManagerServiceName << "\"";
  return BinderWrapper::Get()->RegisterService(kPowerManagerServiceName, this);
}

status_t PowerManager::acquireWakeLock(int flags,
                                       const sp<IBinder>& lock,
                                       const String16& tag,
                                       const String16& packageName,
                                       bool isOneWay) {
  return AddWakeLockRequest(lock, tag, packageName,
                            BinderWrapper::Get()->GetCallingUid())
             ? OK
             : UNKNOWN_ERROR;
}

status_t PowerManager::acquireWakeLockWithUid(int flags,
                                              const sp<IBinder>& lock,
                                              const String16& tag,
                                              const String16& packageName,
                                              int uid,
                                              bool isOneWay) {
  return AddWakeLockRequest(lock, tag, packageName, static_cast<uid_t>(uid))
             ? OK
             : UNKNOWN_ERROR;
}

status_t PowerManager::releaseWakeLock(const sp<IBinder>& lock,
                                       int flags,
                                       bool isOneWay) {
  return wake_lock_manager_->RemoveRequest(lock) ? OK : UNKNOWN_ERROR;
}

status_t PowerManager::updateWakeLockUids(const sp<IBinder>& lock,
                                          int len,
                                          const int* uids,
                                          bool isOneWay) {
  NOTIMPLEMENTED() << "updateWakeLockUids: lock=" << lock.get()
                   << " len=" << len;
  return OK;
}

status_t PowerManager::powerHint(int hintId, int data) {
  NOTIMPLEMENTED() << "powerHint: hintId=" << hintId << " data=" << data;
  return OK;
}

status_t PowerManager::goToSleep(int64_t event_time_ms, int reason, int flags) {
  if (event_time_ms < last_resume_uptime_.InMilliseconds()) {
    LOG(WARNING) << "Ignoring request to suspend in response to event at "
                 << event_time_ms << " preceding last resume time "
                 << last_resume_uptime_.InMilliseconds();
    return BAD_VALUE;
  }

  LOG(INFO) << "Suspending immediately for event at " << event_time_ms
            << " (reason=" << reason << " flags=" << flags << ")";
  if (base::WriteFile(power_state_path_, kPowerStateSuspend,
                      strlen(kPowerStateSuspend)) !=
      static_cast<int>(strlen(kPowerStateSuspend))) {
    PLOG(ERROR) << "Failed to write \"" << kPowerStateSuspend << "\" to "
                << power_state_path_.value();
    return UNKNOWN_ERROR;
  }

  last_resume_uptime_ = base::SysInfo::Uptime();
  LOG(INFO) << "Resumed from suspend at "
            << last_resume_uptime_.InMilliseconds();
  return OK;
}

status_t PowerManager::reboot(bool confirm, const String16& reason, bool wait) {
  const std::string reason_str(String8(reason).string());
  if (!(reason_str.empty() || reason_str == kRebootReasonRecovery)) {
    LOG(WARNING) << "Ignoring reboot request with invalid reason \""
                 << reason_str << "\"";
    return BAD_VALUE;
  }

  LOG(INFO) << "Rebooting with reason \"" << reason_str << "\"";
  if (!property_setter_->SetProperty(ANDROID_RB_PROPERTY,
                                     kRebootPrefix + reason_str)) {
    return UNKNOWN_ERROR;
  }
  return OK;
}

status_t PowerManager::shutdown(bool confirm,
                                const String16& reason,
                                bool wait) {
  const std::string reason_str(String8(reason).string());
  if (!(reason_str.empty() || reason_str == kShutdownReasonUserRequested)) {
    LOG(WARNING) << "Ignoring shutdown request with invalid reason \""
                 << reason_str << "\"";
    return BAD_VALUE;
  }

  LOG(INFO) << "Shutting down with reason \"" << reason_str << "\"";
  if (!property_setter_->SetProperty(ANDROID_RB_PROPERTY,
                                     kShutdownPrefix + reason_str)) {
    return UNKNOWN_ERROR;
  }
  return OK;
}

status_t PowerManager::crash(const String16& message) {
  NOTIMPLEMENTED() << "crash: message=" << message;
  return OK;
}

bool PowerManager::AddWakeLockRequest(const sp<IBinder>& lock,
                                      const String16& tag,
                                      const String16& packageName,
                                      int uid) {
  return wake_lock_manager_->AddRequest(lock, String8(tag).string(),
                                        String8(packageName).string(), uid);
}

}  // namespace android
