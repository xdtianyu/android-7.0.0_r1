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

#ifndef TPM_MANAGER_COMMON_MOCK_TPM_NVRAM_INTERFACE_H_
#define TPM_MANAGER_COMMON_MOCK_TPM_NVRAM_INTERFACE_H_

#include <gmock/gmock.h>

#include "tpm_manager/common/tpm_nvram_interface.h"

namespace tpm_manager {

class MockTpmNvramInterface : public TpmNvramInterface {
 public:
  MockTpmNvramInterface();
  ~MockTpmNvramInterface() override;

  MOCK_METHOD2(DefineNvram, void(const DefineNvramRequest& request,
                                 const DefineNvramCallback& callback));
  MOCK_METHOD2(DestroyNvram, void(const DestroyNvramRequest& request,
                                  const DestroyNvramCallback& callback));
  MOCK_METHOD2(WriteNvram, void(const WriteNvramRequest& request,
                                const WriteNvramCallback& callback));
  MOCK_METHOD2(ReadNvram, void(const ReadNvramRequest& request,
                               const ReadNvramCallback& callback));
  MOCK_METHOD2(IsNvramDefined, void(const IsNvramDefinedRequest& request,
                                    const IsNvramDefinedCallback& callback));
  MOCK_METHOD2(IsNvramLocked, void(const IsNvramLockedRequest& request,
                                   const IsNvramLockedCallback& callback));
  MOCK_METHOD2(GetNvramSize, void(const GetNvramSizeRequest& request,
                                  const GetNvramSizeCallback& callback));
};

}  // namespace tpm_manager

#endif  // TPM_MANAGER_COMMON_MOCK_TPM_NVRAM_INTERFACE_H_
