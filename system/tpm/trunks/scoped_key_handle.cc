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

#include "trunks/scoped_key_handle.h"

#include <base/logging.h>

#include "trunks/error_codes.h"

namespace {

const trunks::TPM_HANDLE kInvalidHandle = 0;

}  // namespace

namespace trunks {

ScopedKeyHandle::ScopedKeyHandle(const TrunksFactory& factory)
    : factory_(factory),
      handle_(kInvalidHandle) {}

ScopedKeyHandle::ScopedKeyHandle(const TrunksFactory& factory,
                                 TPM_HANDLE handle)
    : factory_(factory),
      handle_(handle) {}

ScopedKeyHandle::~ScopedKeyHandle() {
  if (handle_ != kInvalidHandle) {
    FlushHandleContext(handle_);
  }
}

TPM_HANDLE ScopedKeyHandle::release() {
  TPM_HANDLE tmp_handle = handle_;
  handle_ = kInvalidHandle;
  return tmp_handle;
}

void ScopedKeyHandle::reset(TPM_HANDLE new_handle) {
  TPM_HANDLE tmp_handle = handle_;
  handle_ = new_handle;
  if (tmp_handle != kInvalidHandle) {
    FlushHandleContext(tmp_handle);
  }
}

void ScopedKeyHandle::reset() {
  reset(kInvalidHandle);
}

TPM_HANDLE* ScopedKeyHandle::ptr() {
  return &handle_;
}

TPM_HANDLE ScopedKeyHandle::get() const {
  return handle_;
}

void ScopedKeyHandle::FlushHandleContext(TPM_HANDLE handle) {
  TPM_RC result = TPM_RC_SUCCESS;
  result = factory_.GetTpm()->FlushContextSync(handle, nullptr);
  if (result) {
    LOG(WARNING) << "Error closing handle: " << handle << " : "
                 << GetErrorString(result);
  }
}

}  // namespace trunks
