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

#include "update_engine/dbus_service.h"

#include "update_engine/dbus-constants.h"
#include "update_engine/update_status_utils.h"

namespace chromeos_update_engine {

using brillo::ErrorPtr;
using chromeos_update_engine::UpdateEngineService;
using std::string;

DBusUpdateEngineService::DBusUpdateEngineService(SystemState* system_state)
    : common_(new UpdateEngineService{system_state}) {
}

// org::chromium::UpdateEngineInterfaceInterface methods implementation.

bool DBusUpdateEngineService::AttemptUpdate(ErrorPtr* error,
                                            const string& in_app_version,
                                            const string& in_omaha_url) {
  return AttemptUpdateWithFlags(
      error, in_app_version, in_omaha_url, 0 /* no flags */);
}

bool DBusUpdateEngineService::AttemptUpdateWithFlags(
    ErrorPtr* error,
    const string& in_app_version,
    const string& in_omaha_url,
    int32_t in_flags_as_int) {
  update_engine::AttemptUpdateFlags flags =
      static_cast<update_engine::AttemptUpdateFlags>(in_flags_as_int);
  bool interactive = !(flags &
      update_engine::kAttemptUpdateFlagNonInteractive);

  return common_->AttemptUpdate(
      error, in_app_version, in_omaha_url,
      interactive ? 0 : UpdateEngineService::kAttemptUpdateFlagNonInteractive);
}

bool DBusUpdateEngineService::AttemptRollback(ErrorPtr* error,
                                              bool in_powerwash) {
  return common_->AttemptRollback(error, in_powerwash);
}

bool DBusUpdateEngineService::CanRollback(ErrorPtr* error,
                                          bool* out_can_rollback) {
  return common_->CanRollback(error, out_can_rollback);
}

bool DBusUpdateEngineService::ResetStatus(ErrorPtr* error) {
  return common_->ResetStatus(error);
}

bool DBusUpdateEngineService::GetStatus(ErrorPtr* error,
                                        int64_t* out_last_checked_time,
                                        double* out_progress,
                                        string* out_current_operation,
                                        string* out_new_version,
                                        int64_t* out_new_size) {
  return common_->GetStatus(error,
                            out_last_checked_time,
                            out_progress,
                            out_current_operation,
                            out_new_version,
                            out_new_size);
}

bool DBusUpdateEngineService::RebootIfNeeded(ErrorPtr* error) {
  return common_->RebootIfNeeded(error);
}

bool DBusUpdateEngineService::SetChannel(ErrorPtr* error,
                                         const string& in_target_channel,
                                         bool in_is_powerwash_allowed) {
  return common_->SetChannel(error, in_target_channel, in_is_powerwash_allowed);
}

bool DBusUpdateEngineService::GetChannel(ErrorPtr* error,
                                         bool in_get_current_channel,
                                         string* out_channel) {
  return common_->GetChannel(error, in_get_current_channel, out_channel);
}

bool DBusUpdateEngineService::SetP2PUpdatePermission(ErrorPtr* error,
                                                     bool in_enabled) {
  return common_->SetP2PUpdatePermission(error, in_enabled);
}

bool DBusUpdateEngineService::GetP2PUpdatePermission(ErrorPtr* error,
                                                     bool* out_enabled) {
  return common_->GetP2PUpdatePermission(error, out_enabled);
}

bool DBusUpdateEngineService::SetUpdateOverCellularPermission(ErrorPtr* error,
                                                              bool in_allowed) {
  return common_->SetUpdateOverCellularPermission(error, in_allowed);
}

bool DBusUpdateEngineService::GetUpdateOverCellularPermission(
    ErrorPtr* error, bool* out_allowed) {
  return common_->GetUpdateOverCellularPermission(error, out_allowed);
}

bool DBusUpdateEngineService::GetDurationSinceUpdate(
    ErrorPtr* error, int64_t* out_usec_wallclock) {
  return common_->GetDurationSinceUpdate(error, out_usec_wallclock);
}

bool DBusUpdateEngineService::GetPrevVersion(ErrorPtr* error,
                                             string* out_prev_version) {
  return common_->GetPrevVersion(error, out_prev_version);
}

bool DBusUpdateEngineService::GetRollbackPartition(
    ErrorPtr* error, string* out_rollback_partition_name) {
  return common_->GetRollbackPartition(error, out_rollback_partition_name);
}

bool DBusUpdateEngineService::GetLastAttemptError(
    ErrorPtr* error, int32_t* out_last_attempt_error){
 return common_->GetLastAttemptError(error, out_last_attempt_error);
}

UpdateEngineAdaptor::UpdateEngineAdaptor(SystemState* system_state,
                                         const scoped_refptr<dbus::Bus>& bus)
    : org::chromium::UpdateEngineInterfaceAdaptor(&dbus_service_),
    bus_(bus),
    dbus_service_(system_state),
    dbus_object_(nullptr,
                 bus,
                 dbus::ObjectPath(update_engine::kUpdateEngineServicePath)) {}

void UpdateEngineAdaptor::RegisterAsync(
    const base::Callback<void(bool)>& completion_callback) {
  RegisterWithDBusObject(&dbus_object_);
  dbus_object_.RegisterAsync(completion_callback);
}

bool UpdateEngineAdaptor::RequestOwnership() {
  return bus_->RequestOwnershipAndBlock(update_engine::kUpdateEngineServiceName,
                                        dbus::Bus::REQUIRE_PRIMARY);
}

void UpdateEngineAdaptor::SendStatusUpdate(int64_t last_checked_time,
                                           double progress,
                                           update_engine::UpdateStatus status,
                                           const string& new_version,
                                           int64_t new_size) {
  const string str_status = UpdateStatusToString(status);
  SendStatusUpdateSignal(
      last_checked_time, progress, str_status, new_version, new_size);
}

}  // namespace chromeos_update_engine
