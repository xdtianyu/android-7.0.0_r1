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

#include <nativepower/power_manager_client.h>

#include <base/bind.h>
#include <base/logging.h>
#include <binder/IBinder.h>
#include <binderwrapper/binder_wrapper.h>
#include <nativepower/constants.h>
#include <nativepower/wake_lock.h>
#include <powermanager/PowerManager.h>

namespace android {
namespace {

// Returns the string corresponding to |reason|. Values are hardcoded in
// core/java/android/os/PowerManager.java.
String16 ShutdownReasonToString16(ShutdownReason reason) {
  switch (reason) {
    case ShutdownReason::DEFAULT:
      return String16();
    case ShutdownReason::USER_REQUESTED:
      return String16(kShutdownReasonUserRequested);
    default:
      LOG(ERROR) << "Unknown shutdown reason " << static_cast<int>(reason);
      return String16();
  }
}

// Returns the string corresponding to |reason|. Values are hardcoded in
// core/java/android/os/PowerManager.java.
String16 RebootReasonToString16(RebootReason reason) {
  switch (reason) {
    case RebootReason::DEFAULT:
      return String16();
    case RebootReason::RECOVERY:
      return String16(kRebootReasonRecovery);
    default:
      LOG(ERROR) << "Unknown reboot reason " << static_cast<int>(reason);
      return String16();
  }
}

}  // namespace

PowerManagerClient::PowerManagerClient()
    : weak_ptr_factory_(this) {}

PowerManagerClient::~PowerManagerClient() {
  if (power_manager_.get()) {
    BinderWrapper::Get()->UnregisterForDeathNotifications(
        IInterface::asBinder(power_manager_));
  }
}

bool PowerManagerClient::Init() {
  sp<IBinder> power_manager_binder =
      BinderWrapper::Get()->GetService(kPowerManagerServiceName);
  if (!power_manager_binder.get()) {
    LOG(ERROR) << "Didn't get " << kPowerManagerServiceName << " service";
    return false;
  }

  BinderWrapper::Get()->RegisterForDeathNotifications(
      power_manager_binder,
      base::Bind(&PowerManagerClient::OnPowerManagerDied,
                 weak_ptr_factory_.GetWeakPtr()));
  power_manager_ = interface_cast<IPowerManager>(power_manager_binder);

  return true;
}

std::unique_ptr<WakeLock> PowerManagerClient::CreateWakeLock(
    const std::string& tag,
    const std::string& package) {
  std::unique_ptr<WakeLock> lock(new WakeLock(tag, package, this));
  if (!lock->Init())
    lock.reset();
  return lock;
}

bool PowerManagerClient::Suspend(base::TimeDelta event_uptime,
                                 SuspendReason reason,
                                 int flags) {
  DCHECK(power_manager_.get());
  status_t status = power_manager_->goToSleep(
      event_uptime.InMilliseconds(), static_cast<int>(reason), flags);
  if (status != OK) {
    LOG(ERROR) << "Suspend request failed with status " << status;
    return false;
  }
  return true;
}

bool PowerManagerClient::ShutDown(ShutdownReason reason) {
  DCHECK(power_manager_.get());
  status_t status = power_manager_->shutdown(false /* confirm */,
                                             ShutdownReasonToString16(reason),
                                             false /* wait */);
  if (status != OK) {
    LOG(ERROR) << "Shutdown request failed with status " << status;
    return false;
  }
  return true;
}

bool PowerManagerClient::Reboot(RebootReason reason) {
  DCHECK(power_manager_.get());
  status_t status = power_manager_->reboot(false /* confirm */,
                                           RebootReasonToString16(reason),
                                           false /* wait */);
  if (status != OK) {
    LOG(ERROR) << "Reboot request failed with status " << status;
    return false;
  }
  return true;
}

void PowerManagerClient::OnPowerManagerDied() {
  LOG(WARNING) << "Power manager died";
  power_manager_.clear();
  // TODO: Try to get a new handle periodically; also consider notifying
  // previously-created WakeLock objects so they can reacquire locks.
}

}  // namespace android
