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

#include "shill/binder/service_binder_adaptor.h"

#include <binder/Status.h>

#include "shill/logging.h"
#include "shill/service.h"

using android::binder::Status;
using android::IBinder;
using android::sp;
using android::system::connectivity::shill::IPropertyChangedCallback;
using std::string;

namespace shill {

namespace Logging {
static auto kModuleLogScope = ScopeLogger::kBinder;
static string ObjectID(ServiceBinderAdaptor* s) {
  return "Service binder adaptor (id " + s->GetRpcIdentifier() + ", " +
         s->service()->unique_name() + ")";
}
}  // namespace Logging

ServiceBinderAdaptor::ServiceBinderAdaptor(Service* service,
                                           const std::string& id)
    : BinderAdaptor(id), service_(service) {}

ServiceBinderAdaptor::~ServiceBinderAdaptor() { service_ = nullptr; }

void ServiceBinderAdaptor::EmitBoolChanged(const string& name, bool /*value*/) {
  SLOG(this, 2) << __func__ << ": " << name;
  SendPropertyChangedSignal(name);
}

void ServiceBinderAdaptor::EmitUint8Changed(const string& name,
                                            uint8_t /*value*/) {
  SLOG(this, 2) << __func__ << ": " << name;
  SendPropertyChangedSignal(name);
}

void ServiceBinderAdaptor::EmitUint16Changed(const string& name,
                                             uint16_t /*value*/) {
  SLOG(this, 2) << __func__ << ": " << name;
  SendPropertyChangedSignal(name);
}

void ServiceBinderAdaptor::EmitUint16sChanged(const string& name,
                                              const Uint16s& /*value*/) {
  SLOG(this, 2) << __func__ << ": " << name;
  SendPropertyChangedSignal(name);
}

void ServiceBinderAdaptor::EmitUintChanged(const string& name,
                                           uint32_t /*value*/) {
  SLOG(this, 2) << __func__ << ": " << name;
  SendPropertyChangedSignal(name);
}

void ServiceBinderAdaptor::EmitIntChanged(const string& name, int /*value*/) {
  SLOG(this, 2) << __func__ << ": " << name;
  SendPropertyChangedSignal(name);
}

void ServiceBinderAdaptor::EmitRpcIdentifierChanged(const string& name,
                                                    const string& /*value*/) {
  SLOG(this, 2) << __func__ << ": " << name;
  SendPropertyChangedSignal(name);
}

void ServiceBinderAdaptor::EmitStringChanged(const string& name,
                                             const string& /*value*/) {
  SLOG(this, 2) << __func__ << ": " << name;
  SendPropertyChangedSignal(name);
}

void ServiceBinderAdaptor::EmitStringmapChanged(const string& name,
                                                const Stringmap& /*value*/) {
  SLOG(this, 2) << __func__ << ": " << name;
  SendPropertyChangedSignal(name);
}

Status ServiceBinderAdaptor::Connect() {
  // STUB IMPLEMENTATION.
  // TODO(samueltan): replace this with proper implementation.
  return Status::ok();
}

Status ServiceBinderAdaptor::GetState(int32_t* _aidl_return) {
  // STUB IMPLEMENTATION.
  // TODO(samueltan): replace this with proper implementation.
  return Status::ok();
}

Status ServiceBinderAdaptor::GetStrength(int8_t* _aidl_return) {
  // STUB IMPLEMENTATION.
  // TODO(samueltan): replace this with proper implementation.
  return Status::ok();
}

Status ServiceBinderAdaptor::GetError(int32_t* _aidl_return) {
  // STUB IMPLEMENTATION.
  // TODO(samueltan): replace this with proper implementation.
  return Status::ok();
}

Status ServiceBinderAdaptor::RegisterPropertyChangedSignalHandler(
    const sp<IPropertyChangedCallback>& callback) {
  AddPropertyChangedSignalHandler(callback);
  return Status::ok();
}

}  // namespace shill
