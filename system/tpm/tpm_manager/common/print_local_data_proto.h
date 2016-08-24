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

// THIS CODE IS GENERATED.

#ifndef TPM_MANAGER_COMMON_PRINT_LOCAL_DATA_PROTO_H_
#define TPM_MANAGER_COMMON_PRINT_LOCAL_DATA_PROTO_H_

#include <string>

#include "tpm_manager/common/local_data.pb.h"

namespace tpm_manager {

std::string GetProtoDebugStringWithIndent(const LocalData& value,
                                          int indent_size);
std::string GetProtoDebugString(const LocalData& value);

}  // namespace tpm_manager

#endif  // TPM_MANAGER_COMMON_PRINT_LOCAL_DATA_PROTO_H_
