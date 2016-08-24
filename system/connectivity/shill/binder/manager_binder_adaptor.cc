//
// Copyright (C) 2016 The Android Open Source Project
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

#include "shill/binder/manager_binder_adaptor.h"

#include <binder/Status.h>
#include <binderwrapper/binder_wrapper.h>
#include <utils/String16.h>
#include <utils/String8.h>

#include "shill/logging.h"
#include "shill/manager.h"

using android::binder::Status;
using android::BinderWrapper;
using android::IBinder;
using android::sp;
using android::String16;
using android::String8;
using android::system::connectivity::shill::IPropertyChangedCallback;
using std::string;
using std::vector;

namespace shill {

namespace Logging {
static auto kModuleLogScope = ScopeLogger::kBinder;
static string ObjectID(ManagerBinderAdaptor* m) {
  return "Manager binder adaptor (id " + m->GetRpcIdentifier() + ")";
}
}  // namespace Logging

ManagerBinderAdaptor::ManagerBinderAdaptor(Manager* manager,
                                           const std::string& id)
    : BinderAdaptor(id), manager_(manager) {}

ManagerBinderAdaptor::~ManagerBinderAdaptor() { manager_ = nullptr; }

void ManagerBinderAdaptor::RegisterAsync(
    const base::Callback<void(bool)>& /*completion_callback*/) {
  // Registration is performed synchronously in Binder.
  BinderWrapper::Get()->RegisterService(
      String8(getInterfaceDescriptor()).string(), this);
}

void ManagerBinderAdaptor::EmitBoolChanged(const string& name, bool /*value*/) {
  SLOG(this, 2) << __func__ << ": " << name;
  SendPropertyChangedSignal(name);
}

void ManagerBinderAdaptor::EmitUintChanged(const string& name,
                                           uint32_t /*value*/) {
  SLOG(this, 2) << __func__ << ": " << name;
  SendPropertyChangedSignal(name);
}

void ManagerBinderAdaptor::EmitIntChanged(const string& name, int /*value*/) {
  SLOG(this, 2) << __func__ << ": " << name;
  SendPropertyChangedSignal(name);
}

void ManagerBinderAdaptor::EmitStringChanged(const string& name,
                                             const string& /*value*/) {
  SLOG(this, 2) << __func__ << ": " << name;
  SendPropertyChangedSignal(name);
}

void ManagerBinderAdaptor::EmitStringsChanged(const string& name,
                                              const vector<string>& /*value*/) {
  SLOG(this, 2) << __func__ << ": " << name;
  SendPropertyChangedSignal(name);
}

void ManagerBinderAdaptor::EmitRpcIdentifierChanged(const string& name,
                                                    const string& /*value*/) {
  SLOG(this, 2) << __func__ << ": " << name;
  SendPropertyChangedSignal(name);
}

void ManagerBinderAdaptor::EmitRpcIdentifierArrayChanged(
    const string& name, const vector<string>& /*value*/) {
  SLOG(this, 2) << __func__ << ": " << name;
  SendPropertyChangedSignal(name);
}

Status ManagerBinderAdaptor::SetupApModeInterface(String16* _aidl_return) {
  // STUB IMPLEMENTATION.
  return Status::ok();
}

Status ManagerBinderAdaptor::SetupStationModeInterface(String16* _aidl_return) {
  // STUB IMPLEMENTATION.
  // TODO(samueltan): replace this with proper implementation.
  return Status::ok();
}

Status ManagerBinderAdaptor::ClaimInterface(const String16& claimer_name,
                                            const String16& interface_name) {
  // STUB IMPLEMENTATION.
  // TODO(samueltan): replace this with proper implementation.
  return Status::ok();
}

Status ManagerBinderAdaptor::ReleaseInterface(const String16& claimer_name,
                                              const String16& interface_name) {
  // STUB IMPLEMENTATION.
  // TODO(samueltan): replace this with proper implementation.
  return Status::ok();
}

Status ManagerBinderAdaptor::ConfigureService(
    const android::os::PersistableBundle& properties,
    sp<IBinder>* _aidl_return) {
  // STUB IMPLEMENTATION.
  // TODO(samueltan): replace this with proper implementation.
  return Status::ok();
}

Status ManagerBinderAdaptor::RequestScan(int32_t type) {
  // STUB IMPLEMENTATION.
  // TODO(samueltan): replace this with proper implementation.
  return Status::ok();
}

Status ManagerBinderAdaptor::GetDevices(vector<sp<IBinder>>* _aidl_return) {
  // STUB IMPLEMENTATION.
  // TODO(samueltan): replace this with proper implementation.
  return Status::ok();
}

Status ManagerBinderAdaptor::RegisterPropertyChangedSignalHandler(
    const sp<IPropertyChangedCallback>& callback) {
  AddPropertyChangedSignalHandler(callback);
  return Status::ok();
}

}  // namespace shill
