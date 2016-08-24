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

#ifndef TPM_MANAGER_SERVER_MOCK_TPM_NVRAM_H_
#define TPM_MANAGER_SERVER_MOCK_TPM_NVRAM_H_

#include "tpm_manager/server/tpm_nvram.h"

#include <map>
#include <string>

#include <gmock/gmock.h>

namespace tpm_manager {

struct NvSpace {
  std::string data;
  bool written;
};

class MockTpmNvram : public TpmNvram {
 public:
  MockTpmNvram();
  ~MockTpmNvram() override;

  MOCK_METHOD2(DefineNvram, bool(uint32_t, size_t));
  MOCK_METHOD1(DestroyNvram, bool(uint32_t));
  MOCK_METHOD2(WriteNvram, bool(uint32_t, const std::string&));
  MOCK_METHOD2(ReadNvram, bool(uint32_t, std::string*));
  MOCK_METHOD2(IsNvramDefined, bool(uint32_t, bool*));
  MOCK_METHOD2(IsNvramLocked, bool(uint32_t, bool*));
  MOCK_METHOD2(GetNvramSize, bool(uint32_t, size_t*));

 private:
  bool FakeDefineNvram(uint32_t index, size_t length);
  bool FakeDestroyNvram(uint32_t index);
  bool FakeWriteNvram(uint32_t index, const std::string& data);
  bool FakeReadNvram(uint32_t index, std::string* data);
  bool FakeIsNvramDefined(uint32_t index, bool* defined);
  bool FakeIsNvramLocked(uint32_t index, bool* locked);
  bool FakeGetNvramSize(uint32_t index, size_t* size);

  std::map<uint32_t, NvSpace> nvram_map_;
};

}  // namespace tpm_manager

#endif  // TPM_MANAGER_SERVER_MOCK_TPM_NVRAM_H_
