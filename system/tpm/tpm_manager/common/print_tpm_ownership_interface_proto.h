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

#ifndef TPM_MANAGER_COMMON_PRINT_TPM_OWNERSHIP_INTERFACE_PROTO_H_
#define TPM_MANAGER_COMMON_PRINT_TPM_OWNERSHIP_INTERFACE_PROTO_H_

#include <string>

#include "tpm_manager/common/tpm_ownership_interface.pb.h"

namespace tpm_manager {

std::string GetProtoDebugStringWithIndent(const GetTpmStatusRequest& value,
                                          int indent_size);
std::string GetProtoDebugString(const GetTpmStatusRequest& value);
std::string GetProtoDebugStringWithIndent(const GetTpmStatusReply& value,
                                          int indent_size);
std::string GetProtoDebugString(const GetTpmStatusReply& value);
std::string GetProtoDebugStringWithIndent(const TakeOwnershipRequest& value,
                                          int indent_size);
std::string GetProtoDebugString(const TakeOwnershipRequest& value);
std::string GetProtoDebugStringWithIndent(const TakeOwnershipReply& value,
                                          int indent_size);
std::string GetProtoDebugString(const TakeOwnershipReply& value);
std::string GetProtoDebugStringWithIndent(
    const RemoveOwnerDependencyRequest& value,
    int indent_size);
std::string GetProtoDebugString(const RemoveOwnerDependencyRequest& value);
std::string GetProtoDebugStringWithIndent(
    const RemoveOwnerDependencyReply& value,
    int indent_size);
std::string GetProtoDebugString(const RemoveOwnerDependencyReply& value);

}  // namespace tpm_manager

#endif  // TPM_MANAGER_COMMON_PRINT_TPM_OWNERSHIP_INTERFACE_PROTO_H_
