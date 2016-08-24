//
// Copyright (C) 2014 The Android Open Source Project
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

#include "update_engine/update_manager/real_device_policy_provider.h"

#include <stdint.h>

#include <base/location.h>
#include <base/logging.h>
#include <base/time/time.h>
#include <policy/device_policy.h>

#include "update_engine/common/utils.h"
#include "update_engine/update_manager/generic_variables.h"
#include "update_engine/update_manager/real_shill_provider.h"

using base::TimeDelta;
using brillo::MessageLoop;
using policy::DevicePolicy;
using std::set;
using std::string;

namespace {

const int kDevicePolicyRefreshRateInMinutes = 60;

}  // namespace

namespace chromeos_update_manager {

RealDevicePolicyProvider::~RealDevicePolicyProvider() {
  MessageLoop::current()->CancelTask(scheduled_refresh_);
}

bool RealDevicePolicyProvider::Init() {
  CHECK(policy_provider_ != nullptr);

  // On Init() we try to get the device policy and keep updating it.
  RefreshDevicePolicyAndReschedule();

  // We also listen for signals from the session manager to force a device
  // policy refresh.
  session_manager_proxy_->RegisterPropertyChangeCompleteSignalHandler(
      base::Bind(&RealDevicePolicyProvider::OnPropertyChangedCompletedSignal,
                 base::Unretained(this)),
      base::Bind(&RealDevicePolicyProvider::OnSignalConnected,
                 base::Unretained(this)));
  return true;
}

void RealDevicePolicyProvider::OnPropertyChangedCompletedSignal(
    const string& success) {
  if (success != "success") {
    LOG(WARNING) << "Received device policy updated signal with a failure.";
  }
  // We refresh the policy file even if the payload string is kSignalFailure.
  LOG(INFO) << "Reloading and re-scheduling device policy due to signal "
               "received.";
  MessageLoop::current()->CancelTask(scheduled_refresh_);
  scheduled_refresh_ = MessageLoop::kTaskIdNull;
  RefreshDevicePolicyAndReschedule();
}

void RealDevicePolicyProvider::OnSignalConnected(const string& interface_name,
                                                 const string& signal_name,
                                                 bool successful) {
  if (!successful) {
    LOG(WARNING) << "We couldn't connect to SessionManager signal for updates "
                    "on the device policy blob. We will reload the policy file "
                    "periodically.";
  }
  // We do a one-time refresh of the DevicePolicy just in case we missed a
  // signal between the first refresh and the time the signal handler was
  // actually connected.
  RefreshDevicePolicy();
}

void RealDevicePolicyProvider::RefreshDevicePolicyAndReschedule() {
  RefreshDevicePolicy();
  scheduled_refresh_ = MessageLoop::current()->PostDelayedTask(
      FROM_HERE,
      base::Bind(&RealDevicePolicyProvider::RefreshDevicePolicyAndReschedule,
                 base::Unretained(this)),
      TimeDelta::FromMinutes(kDevicePolicyRefreshRateInMinutes));
}

template<typename T>
void RealDevicePolicyProvider::UpdateVariable(
    AsyncCopyVariable<T>* var,
    bool (DevicePolicy::*getter_method)(T*) const) {
  T new_value;
  if (policy_provider_->device_policy_is_loaded() &&
      (policy_provider_->GetDevicePolicy().*getter_method)(&new_value)) {
    var->SetValue(new_value);
  } else {
    var->UnsetValue();
  }
}

template<typename T>
void RealDevicePolicyProvider::UpdateVariable(
    AsyncCopyVariable<T>* var,
    bool (RealDevicePolicyProvider::*getter_method)(T*) const) {
  T new_value;
  if (policy_provider_->device_policy_is_loaded() &&
      (this->*getter_method)(&new_value)) {
    var->SetValue(new_value);
  } else {
    var->UnsetValue();
  }
}

bool RealDevicePolicyProvider::ConvertAllowedConnectionTypesForUpdate(
      set<ConnectionType>* allowed_types) const {
  set<string> allowed_types_str;
  if (!policy_provider_->GetDevicePolicy()
      .GetAllowedConnectionTypesForUpdate(&allowed_types_str)) {
    return false;
  }
  allowed_types->clear();
  for (auto& type_str : allowed_types_str) {
    ConnectionType type =
        RealShillProvider::ParseConnectionType(type_str.c_str());
    if (type != ConnectionType::kUnknown) {
      allowed_types->insert(type);
    } else {
      LOG(WARNING) << "Policy includes unknown connection type: " << type_str;
    }
  }
  return true;
}

bool RealDevicePolicyProvider::ConvertScatterFactor(
    TimeDelta* scatter_factor) const {
  int64_t scatter_factor_in_seconds;
  if (!policy_provider_->GetDevicePolicy().GetScatterFactorInSeconds(
      &scatter_factor_in_seconds)) {
    return false;
  }
  if (scatter_factor_in_seconds < 0) {
    LOG(WARNING) << "Ignoring negative scatter factor: "
                 << scatter_factor_in_seconds;
    return false;
  }
  *scatter_factor = TimeDelta::FromSeconds(scatter_factor_in_seconds);
  return true;
}

void RealDevicePolicyProvider::RefreshDevicePolicy() {
  if (!policy_provider_->Reload()) {
    LOG(INFO) << "No device policies/settings present.";
  }

  var_device_policy_is_loaded_.SetValue(
      policy_provider_->device_policy_is_loaded());

  UpdateVariable(&var_release_channel_, &DevicePolicy::GetReleaseChannel);
  UpdateVariable(&var_release_channel_delegated_,
                 &DevicePolicy::GetReleaseChannelDelegated);
  UpdateVariable(&var_update_disabled_, &DevicePolicy::GetUpdateDisabled);
  UpdateVariable(&var_target_version_prefix_,
                 &DevicePolicy::GetTargetVersionPrefix);
  UpdateVariable(&var_scatter_factor_,
                 &RealDevicePolicyProvider::ConvertScatterFactor);
  UpdateVariable(
      &var_allowed_connection_types_for_update_,
      &RealDevicePolicyProvider::ConvertAllowedConnectionTypesForUpdate);
  UpdateVariable(&var_owner_, &DevicePolicy::GetOwner);
  UpdateVariable(&var_http_downloads_enabled_,
                 &DevicePolicy::GetHttpDownloadsEnabled);
  UpdateVariable(&var_au_p2p_enabled_, &DevicePolicy::GetAuP2PEnabled);
}

}  // namespace chromeos_update_manager
