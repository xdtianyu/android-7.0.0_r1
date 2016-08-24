// Copyright 2016 The Android Open Source Project
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

#include "buffet/binder_command_proxy.h"

#include <weave/enum_to_string.h>

#include "buffet/weave_error_conversion.h"
#include "common/binder_utils.h"

using weaved::binder_utils::ParseDictionary;
using weaved::binder_utils::ToStatus;
using weaved::binder_utils::ToString;
using weaved::binder_utils::ToString16;

namespace buffet {

namespace {

android::binder::Status ReportDestroyedError() {
  return android::binder::Status::fromServiceSpecificError(
      1, android::String8{"Command has been destroyed"});
}

}  // anonymous namespace

BinderCommandProxy::BinderCommandProxy(
    const std::weak_ptr<weave::Command>& command) : command_{command} {}

android::binder::Status BinderCommandProxy::getId(android::String16* id) {
  auto command = command_.lock();
  if (!command)
    return ReportDestroyedError();
  *id = ToString16(command->GetID());
  return android::binder::Status::ok();
}

android::binder::Status BinderCommandProxy::getName(android::String16* name) {
  auto command = command_.lock();
  if (!command)
    return ReportDestroyedError();
  *name = ToString16(command->GetName());
  return android::binder::Status::ok();
}

android::binder::Status BinderCommandProxy::getComponent(
    android::String16* component) {
  auto command = command_.lock();
  if (!command)
    return ReportDestroyedError();
  *component = ToString16(command->GetComponent());
  return android::binder::Status::ok();
}

android::binder::Status BinderCommandProxy::getState(android::String16* state) {
  auto command = command_.lock();
  if (!command)
    return ReportDestroyedError();
  *state = ToString16(EnumToString(command->GetState()));
  return android::binder::Status::ok();
}

android::binder::Status BinderCommandProxy::getOrigin(
    android::String16* origin) {
  auto command = command_.lock();
  if (!command)
    return ReportDestroyedError();
  *origin = ToString16(EnumToString(command->GetOrigin()));
  return android::binder::Status::ok();
}

android::binder::Status BinderCommandProxy::getParameters(
    android::String16* parameters) {
  auto command = command_.lock();
  if (!command)
    return ReportDestroyedError();
  *parameters = ToString16(command->GetParameters());
  return android::binder::Status::ok();
}

android::binder::Status BinderCommandProxy::getProgress(
    android::String16* progress) {
  auto command = command_.lock();
  if (!command)
    return ReportDestroyedError();
  *progress = ToString16(command->GetProgress());
  return android::binder::Status::ok();
}

android::binder::Status BinderCommandProxy::getResults(
    android::String16* results) {
  auto command = command_.lock();
  if (!command)
    return ReportDestroyedError();
  *results = ToString16(command->GetResults());
  return android::binder::Status::ok();
}

android::binder::Status BinderCommandProxy::setProgress(
    const android::String16& progress) {
  auto command = command_.lock();
  if (!command)
    return ReportDestroyedError();
  std::unique_ptr<base::DictionaryValue> dict;
  auto status = ParseDictionary(progress, &dict);
  if (status.isOk()) {
    weave::ErrorPtr error;
    status = ToStatus(command->SetProgress(*dict, &error), &error);
  }
  return status;
}

android::binder::Status BinderCommandProxy::complete(
    const android::String16& results) {
  auto command = command_.lock();
  if (!command)
    return ReportDestroyedError();
  std::unique_ptr<base::DictionaryValue> dict;
  auto status = ParseDictionary(results, &dict);
  if (status.isOk()) {
    weave::ErrorPtr error;
    status = ToStatus(command->Complete(*dict, &error), &error);
  }
  return status;
}

android::binder::Status BinderCommandProxy::abort(
    const android::String16& errorCode,
    const android::String16& errorMessage) {
  auto command = command_.lock();
  if (!command)
    return ReportDestroyedError();
  weave::ErrorPtr command_error;
  weave::Error::AddTo(&command_error, FROM_HERE, ToString(errorCode),
                      ToString(errorMessage));
  weave::ErrorPtr error;
  return ToStatus(command->Abort(command_error.get(), &error), &error);
}

android::binder::Status BinderCommandProxy::cancel() {
  auto command = command_.lock();
  if (!command)
    return ReportDestroyedError();
  weave::ErrorPtr error;
  return ToStatus(command->Cancel(&error), &error);
}

android::binder::Status BinderCommandProxy::pause() {
  auto command = command_.lock();
  if (!command)
    return ReportDestroyedError();
  weave::ErrorPtr error;
  return ToStatus(command->Pause(&error), &error);
}

android::binder::Status BinderCommandProxy::setError(
    const android::String16& errorCode,
    const android::String16& errorMessage) {
  auto command = command_.lock();
  if (!command)
    return ReportDestroyedError();
  weave::ErrorPtr command_error;
  weave::Error::AddTo(&command_error, FROM_HERE, ToString(errorCode),
                      ToString(errorMessage));
  weave::ErrorPtr error;
  return ToStatus(command->SetError(command_error.get(), &error), &error);
}

}  // namespace buffet
