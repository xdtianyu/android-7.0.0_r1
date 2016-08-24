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

#include "shill/power_manager.h"

#include <map>
#include <string>

#include <base/bind.h>
#include <base/stl_util.h>
#if defined(__ANDROID__)
#include <dbus/service_constants.h>
#else
#include <chromeos/dbus/service_constants.h>
#endif  // __ANDROID__

#include "shill/control_interface.h"
#include "shill/event_dispatcher.h"
#include "shill/logging.h"
#include "shill/power_manager_proxy_interface.h"

using base::Bind;
using base::TimeDelta;
using base::Unretained;
using std::map;
using std::string;

namespace shill {

// static
const int PowerManager::kInvalidSuspendId = -1;
const char PowerManager::kSuspendDelayDescription[] = "shill";
const char PowerManager::kDarkSuspendDelayDescription[] = "shill";
const int PowerManager::kSuspendTimeoutMilliseconds = 15 * 1000;

PowerManager::PowerManager(EventDispatcher* dispatcher,
                           ControlInterface* control_interface)
    : dispatcher_(dispatcher),
      control_interface_(control_interface),
      suspend_delay_registered_(false),
      suspend_delay_id_(0),
      dark_suspend_delay_registered_(false),
      dark_suspend_delay_id_(0),
      suspending_(false),
      in_dark_resume_(false),
      current_suspend_id_(0),
      current_dark_suspend_id_(0) {}

PowerManager::~PowerManager() {}

void PowerManager::Start(
    TimeDelta suspend_delay,
    const SuspendImminentCallback& suspend_imminent_callback,
    const SuspendDoneCallback& suspend_done_callback,
    const DarkSuspendImminentCallback& dark_suspend_imminent_callback) {
  power_manager_proxy_.reset(
      control_interface_->CreatePowerManagerProxy(
          this,
          Bind(&PowerManager::OnPowerManagerAppeared, Unretained(this)),
          Bind(&PowerManager::OnPowerManagerVanished, Unretained(this))));
  suspend_delay_ = suspend_delay;
  suspend_imminent_callback_ = suspend_imminent_callback;
  suspend_done_callback_ = suspend_done_callback;
  dark_suspend_imminent_callback_ = dark_suspend_imminent_callback;
}

void PowerManager::Stop() {
  LOG(INFO) << __func__;
  // We may attempt to unregister with a stale |suspend_delay_id_| if powerd
  // reappeared behind our back. It is safe to do so.
  if (suspend_delay_registered_)
    power_manager_proxy_->UnregisterSuspendDelay(suspend_delay_id_);
  if (dark_suspend_delay_registered_)
    power_manager_proxy_->UnregisterDarkSuspendDelay(dark_suspend_delay_id_);

  suspend_delay_registered_ = false;
  dark_suspend_delay_registered_ = false;
  power_manager_proxy_.reset();
}

bool PowerManager::ReportSuspendReadiness() {
  if (!suspending_) {
    LOG(INFO) << __func__ << ": Suspend attempt ("
              << current_suspend_id_ << ") not active. Ignoring signal.";
    return false;
  }
  return power_manager_proxy_->ReportSuspendReadiness(suspend_delay_id_,
                                                      current_suspend_id_);
}

bool PowerManager::ReportDarkSuspendReadiness() {
  return power_manager_proxy_->ReportDarkSuspendReadiness(
      dark_suspend_delay_id_,
      current_dark_suspend_id_);
}

bool PowerManager::RecordDarkResumeWakeReason(const string& wake_reason) {
  return power_manager_proxy_->RecordDarkResumeWakeReason(wake_reason);
}

void PowerManager::OnSuspendImminent(int suspend_id) {
  LOG(INFO) << __func__ << "(" << suspend_id << ")";
  current_suspend_id_ = suspend_id;

  // If we're already suspending, don't call the |suspend_imminent_callback_|
  // again.
  if (!suspending_) {
    // Change the power state to suspending as soon as this signal is received
    // so that the manager can suppress auto-connect, for example.
    // Also, we must set this before running the callback below, because the
    // callback may synchronously report suspend readiness.
    suspending_ = true;
    suspend_imminent_callback_.Run();
  }
}

void PowerManager::OnSuspendDone(int suspend_id) {
  // NB: |suspend_id| could be -1. See OnPowerManagerVanished.
  LOG(INFO) << __func__ << "(" << suspend_id << ")";
  if (!suspending_) {
    LOG(WARNING) << "Recieved unexpected SuspendDone ("
                 << suspend_id << "). Ignoring.";
    return;
  }

  suspending_ = false;
  in_dark_resume_ = false;
  suspend_done_callback_.Run();
}

void PowerManager::OnDarkSuspendImminent(int suspend_id) {
  LOG(INFO) << __func__ << "(" << suspend_id << ")";
  if (!dark_suspend_delay_registered_) {
    LOG(WARNING) << "Ignoring DarkSuspendImminent signal from powerd. shill "
                 << "does not have a dark suspend delay registered. This "
                 << "means that shill is not guaranteed any time before a "
                 << "resuspend.";
    return;
  }
  in_dark_resume_ = true;
  current_dark_suspend_id_ = suspend_id;
  dark_suspend_imminent_callback_.Run();
}

void PowerManager::OnPowerManagerAppeared() {
  LOG(INFO) << __func__;
  CHECK(!suspend_delay_registered_);
  if (power_manager_proxy_->RegisterSuspendDelay(suspend_delay_,
                                                 kSuspendDelayDescription,
                                                 &suspend_delay_id_))
    suspend_delay_registered_ = true;

  if (power_manager_proxy_->RegisterDarkSuspendDelay(
      suspend_delay_,
      kDarkSuspendDelayDescription,
      &dark_suspend_delay_id_))
    dark_suspend_delay_registered_ = true;
}

void PowerManager::OnPowerManagerVanished() {
  LOG(INFO) << __func__;
  // If powerd vanished during a suspend, we need to wake ourselves up.
  if (suspending_)
    OnSuspendDone(kInvalidSuspendId);
  suspend_delay_registered_ = false;
  dark_suspend_delay_registered_ = false;
}

}  // namespace shill
