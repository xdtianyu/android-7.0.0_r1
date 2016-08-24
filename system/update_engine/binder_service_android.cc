//
// Copyright (C) 2015 The Android Open Source Project
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

#include "update_engine/binder_service_android.h"

#include <base/bind.h>
#include <base/logging.h>
#include <binderwrapper/binder_wrapper.h>
#include <brillo/errors/error.h>
#include <utils/String8.h>

using android::binder::Status;
using android::os::IUpdateEngineCallback;

namespace {
Status ErrorPtrToStatus(const brillo::ErrorPtr& error) {
  return Status::fromServiceSpecificError(
      1, android::String8{error->GetMessage().c_str()});
}
}  // namespace

namespace chromeos_update_engine {

BinderUpdateEngineAndroidService::BinderUpdateEngineAndroidService(
    ServiceDelegateAndroidInterface* service_delegate)
    : service_delegate_(service_delegate) {
}

void BinderUpdateEngineAndroidService::SendStatusUpdate(
    int64_t /* last_checked_time */,
    double progress,
    update_engine::UpdateStatus status,
    const std::string& /* new_version  */,
    int64_t /* new_size */) {
  last_status_ = static_cast<int>(status);
  last_progress_ = progress;
  for (auto& callback : callbacks_) {
    callback->onStatusUpdate(last_status_, last_progress_);
  }
}

void BinderUpdateEngineAndroidService::SendPayloadApplicationComplete(
    ErrorCode error_code) {
  for (auto& callback : callbacks_) {
    callback->onPayloadApplicationComplete(static_cast<int>(error_code));
  }
}

Status BinderUpdateEngineAndroidService::bind(
    const android::sp<IUpdateEngineCallback>& callback, bool* return_value) {
  callbacks_.emplace_back(callback);

  auto binder_wrapper = android::BinderWrapper::Get();
  binder_wrapper->RegisterForDeathNotifications(
      IUpdateEngineCallback::asBinder(callback),
      base::Bind(&BinderUpdateEngineAndroidService::UnbindCallback,
                 base::Unretained(this),
                 base::Unretained(callback.get())));

  // Send an status update on connection (except when no update sent so far),
  // since the status update is oneway and we don't need to wait for the
  // response.
  if (last_status_ != -1)
    callback->onStatusUpdate(last_status_, last_progress_);

  *return_value = true;
  return Status::ok();
}

Status BinderUpdateEngineAndroidService::applyPayload(
    const android::String16& url,
    int64_t payload_offset,
    int64_t payload_size,
    const std::vector<android::String16>& header_kv_pairs) {
  const std::string payload_url{android::String8{url}.string()};
  std::vector<std::string> str_headers;
  str_headers.reserve(header_kv_pairs.size());
  for (const auto& header : header_kv_pairs) {
    str_headers.emplace_back(android::String8{header}.string());
  }

  brillo::ErrorPtr error;
  if (!service_delegate_->ApplyPayload(
          payload_url, payload_offset, payload_size, str_headers, &error)) {
    return ErrorPtrToStatus(error);
  }
  return Status::ok();
}

Status BinderUpdateEngineAndroidService::suspend() {
  brillo::ErrorPtr error;
  if (!service_delegate_->SuspendUpdate(&error))
    return ErrorPtrToStatus(error);
  return Status::ok();
}

Status BinderUpdateEngineAndroidService::resume() {
  brillo::ErrorPtr error;
  if (!service_delegate_->ResumeUpdate(&error))
    return ErrorPtrToStatus(error);
  return Status::ok();
}

Status BinderUpdateEngineAndroidService::cancel() {
  brillo::ErrorPtr error;
  if (!service_delegate_->CancelUpdate(&error))
    return ErrorPtrToStatus(error);
  return Status::ok();
}

Status BinderUpdateEngineAndroidService::resetStatus() {
  brillo::ErrorPtr error;
  if (!service_delegate_->ResetStatus(&error))
    return ErrorPtrToStatus(error);
  return Status::ok();
}

void BinderUpdateEngineAndroidService::UnbindCallback(
    IUpdateEngineCallback* callback) {
  auto it =
      std::find_if(callbacks_.begin(),
                   callbacks_.end(),
                   [&callback](const android::sp<IUpdateEngineCallback>& elem) {
                     return elem.get() == callback;
                   });
  if (it == callbacks_.end()) {
    LOG(ERROR) << "Got death notification for unknown callback.";
    return;
  }
  callbacks_.erase(it);
}

}  // namespace chromeos_update_engine
