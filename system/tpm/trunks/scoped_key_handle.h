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

#ifndef TRUNKS_SCOPED_KEY_HANDLE_H_
#define TRUNKS_SCOPED_KEY_HANDLE_H_

#include "trunks/tpm_generated.h"
#include "trunks/trunks_export.h"
#include "trunks/trunks_factory.h"

namespace trunks {

// This class is used to wrap a Key or NV ram handle given by the TPM.
// It provides a destructor that cleans up TPM resources associated with
// that handle.
class TRUNKS_EXPORT ScopedKeyHandle {
 public:
  // We provide a factory to the constructor so that we can later free
  // resources associated with the handle.
  explicit ScopedKeyHandle(const TrunksFactory& factory);
  ScopedKeyHandle(const TrunksFactory& factory, TPM_HANDLE handle);
  virtual ~ScopedKeyHandle();

  // This method releases the TPM_HANDLE associated with this class.
  // It returns the handle that was previously wrapped, and returns
  // INVALID_HANDLE if the previous handle was unset.
  virtual TPM_HANDLE release();

  // This method flushes all context associated with the current handle,
  // and has the class wrap |new_handle|
  virtual void reset(TPM_HANDLE new_handle);

  // This method flushes all context associated with the current handle,
  // and resets the internal handle of the class to the uninitialized value.
  // Note: After reset() this class should not be used again till a new handle
  // is injected.
  virtual void reset();

  // This method returns a pointer to the handle associated with this class.
  // This method does not transfer ownership.
  virtual TPM_HANDLE* ptr();

  // This method returns the handle currectly associated with the class.
  // This method does not transfer ownership, therefore the handle returned
  // might be stale.
  virtual TPM_HANDLE get() const;

 private:
  const TrunksFactory& factory_;
  TPM_HANDLE handle_;
  void FlushHandleContext(TPM_HANDLE handle);

  DISALLOW_COPY_AND_ASSIGN(ScopedKeyHandle);
};

}  // namespace trunks

#endif  // TRUNKS_SCOPED_KEY_HANDLE_H_
