// Copyright 2015 The Android Open Source Project
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

#include "libweaved/command.h"

#include "android/weave/IWeaveCommand.h"
#include "common/binder_utils.h"

using weaved::binder_utils::ParseDictionary;
using weaved::binder_utils::ToString;
using weaved::binder_utils::ToString16;
using weaved::binder_utils::StatusToError;

namespace weaved {

namespace {

// Converts binder exception code into a weave error code string.
std::string BinderExceptionString(int32_t exception_code) {
  if (exception_code == android::binder::Status::EX_NONE)
    return "_none";
  else if (exception_code == android::binder::Status::EX_SECURITY)
    return "_security";
  else if (exception_code == android::binder::Status::EX_BAD_PARCELABLE)
    return "_bad_parcelable";
  else if (exception_code == android::binder::Status::EX_ILLEGAL_ARGUMENT)
    return "_illegal_argument";
  else if (exception_code == android::binder::Status::EX_NULL_POINTER)
    return "_null_pointer";
  else if (exception_code == android::binder::Status::EX_ILLEGAL_STATE)
    return "_illegal_state";
  else if (exception_code == android::binder::Status::EX_NETWORK_MAIN_THREAD)
    return "_network_error";
  else if (exception_code == android::binder::Status::EX_UNSUPPORTED_OPERATION)
    return "_unsupported_operation";
  else if (exception_code == android::binder::Status::EX_SERVICE_SPECIFIC)
    return "_general_failure";

  return "_unknown";
}

}  // anonymous namespace

Command::Command(const android::sp<android::weave::IWeaveCommand>& proxy)
    : binder_proxy_{proxy} {}

Command::~Command() {}

std::string Command::GetID() const {
  std::string id;
  android::String16 id16;
  if (binder_proxy_->getId(&id16).isOk())
    id.assign(ToString(id16));
  return id;
}

std::string Command::GetName() const {
  std::string name;
  android::String16 name16;
  if (binder_proxy_->getId(&name16).isOk())
    name.assign(ToString(name16));
  return name;
}

std::string Command::GetComponent() const {
  std::string component;
  android::String16 component16;
  if (binder_proxy_->getId(&component16).isOk())
    component.assign(ToString(component16));
  return component;
}

Command::State Command::GetState() const {
  std::string state;
  android::String16 state16;
  if (binder_proxy_->getState(&state16).isOk())
    state.assign(ToString(state16));
  if (state == "queued")
    return Command::State::kQueued;
  else if (state == "inProgress")
    return Command::State::kInProgress;
  else if (state == "paused")
    return Command::State::kPaused;
  else if (state == "error")
    return Command::State::kError;
  else if (state == "done")
    return Command::State::kDone;
  else if (state == "cancelled")
    return Command::State::kCancelled;
  else if (state == "aborted")
    return Command::State::kAborted;
  else if (state == "expired")
    return Command::State::kExpired;
  LOG(WARNING) << "Unknown command state: " << state;
  return Command::State::kQueued;
}

Command::Origin Command::GetOrigin() const {
  std::string origin;
  android::String16 origin16;
  if (binder_proxy_->getState(&origin16).isOk())
    origin.assign(ToString(origin16));
  if (origin == "local")
    return Command::Origin::kLocal;
  else if (origin == "cloud")
    return Command::Origin::kCloud;
  LOG(WARNING) << "Unknown command origin: " << origin;
  return Command::Origin::kLocal;
}

const base::DictionaryValue& Command::GetParameters() const {
  if (!parameter_cache_) {
    android::String16 params_string16;
    if (!binder_proxy_->getParameters(&params_string16).isOk() ||
        !ParseDictionary(params_string16, &parameter_cache_).isOk()) {
      parameter_cache_.reset(new base::DictionaryValue);
    }
  }
  return *parameter_cache_;
}

bool Command::SetProgress(const base::DictionaryValue& progress,
                          brillo::ErrorPtr* error) {
  return StatusToError(binder_proxy_->setProgress(ToString16(progress)), error);
}

bool Command::Complete(const base::DictionaryValue& results,
                       brillo::ErrorPtr* error) {
  return StatusToError(binder_proxy_->complete(ToString16(results)), error);
}

bool Command::Abort(const std::string& error_code,
                    const std::string& error_message,
                    brillo::ErrorPtr* error) {
  return StatusToError(binder_proxy_->abort(ToString16(error_code),
                                            ToString16(error_message)),
                       error);
}

bool Command::AbortWithCustomError(const brillo::Error* command_error,
                                   brillo::ErrorPtr* error) {
  std::string error_code = "_" + command_error->GetCode();
  return Abort(error_code, command_error->GetMessage(), error);
}

bool Command::AbortWithCustomError(android::binder::Status status,
                                   brillo::ErrorPtr* error) {
  std::string error_code = BinderExceptionString(status.exceptionCode());
  return Abort(error_code, status.exceptionMessage().string(), error);
}

bool Command::Cancel(brillo::ErrorPtr* error) {
  return StatusToError(binder_proxy_->cancel(), error);
}

bool Command::Pause(brillo::ErrorPtr* error) {
  return StatusToError(binder_proxy_->pause(), error);
}

bool Command::SetError(const std::string& error_code,
                       const std::string& error_message,
                       brillo::ErrorPtr* error) {
  return StatusToError(binder_proxy_->setError(ToString16(error_code),
                                               ToString16(error_message)),
                       error);
}

bool Command::SetCustomError(const brillo::Error* command_error,
                             brillo::ErrorPtr* error) {
  std::string error_code = "_" + command_error->GetCode();
  return SetError(error_code, command_error->GetMessage(), error);
}

bool Command::SetCustomError(android::binder::Status status,
                             brillo::ErrorPtr* error) {
  std::string error_code = BinderExceptionString(status.exceptionCode());
  return SetError(error_code, status.exceptionMessage().string(), error);
}

}  // namespace weave
