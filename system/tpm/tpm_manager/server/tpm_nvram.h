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

#ifndef TPM_MANAGER_SERVER_TPM_NVRAM_H_
#define TPM_MANAGER_SERVER_TPM_NVRAM_H_

#include <string>

namespace tpm_manager {

// TpmNvram is an interface for accessing Nvram functionality on a Tpm.
class TpmNvram {
 public:
  TpmNvram() = default;
  virtual ~TpmNvram() = default;

  // This method creates a NVRAM space in the TPM. Returns true iff
  // the space was successfully created.
  virtual bool DefineNvram(uint32_t index, size_t length) = 0;

  // This method destroys a defined NVRAM space. Returns true iff the NVRAM
  // space was successfully destroyed.
  virtual bool DestroyNvram(uint32_t index) = 0;

  // This method writes |data| to the NVRAM space defined by |index|. The size
  // of |data| must be equal or less than the size of the NVRAM space. Returns
  // true on success. Once written to, the NVRAM space is locked and cannot be
  // written to again.
  virtual bool WriteNvram(uint32_t index, const std::string& data) = 0;

  // This method reads all the contents of the NVRAM space at |index| and writes
  // it into |data|. Returns true on success.
  virtual bool ReadNvram(uint32_t index, std::string* data) = 0;

  // This method sets the out argument |defined| to true iff the NVRAM space
  // referred to by |index| is defined. Returns true on success.
  virtual bool IsNvramDefined(uint32_t index, bool* defined) = 0;

  // This method sets the out argument |locked| to true iff the NVRAM space
  // referred to by |index| is locked. Returns true on success.
  virtual bool IsNvramLocked(uint32_t index, bool* locked) = 0;

  // This method sets the out argument |size| to the size of the NVRAM space
  // referred to by |index|. Returns true on success.
  virtual bool GetNvramSize(uint32_t index, size_t* size) = 0;
};

}  // namespace tpm_manager

#endif  // TPM_MANAGER_SERVER_TPM_NVRAM_H_
