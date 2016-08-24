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

#ifndef TPM_MANAGER_COMMON_PRINT_TPM_NVRAM_INTERFACE_PROTO_H_
#define TPM_MANAGER_COMMON_PRINT_TPM_NVRAM_INTERFACE_PROTO_H_

#include <string>

#include "tpm_manager/common/tpm_nvram_interface.pb.h"

namespace tpm_manager {

std::string GetProtoDebugStringWithIndent(const DefineNvramRequest& value,
                                          int indent_size);
std::string GetProtoDebugString(const DefineNvramRequest& value);
std::string GetProtoDebugStringWithIndent(const DefineNvramReply& value,
                                          int indent_size);
std::string GetProtoDebugString(const DefineNvramReply& value);
std::string GetProtoDebugStringWithIndent(const DestroyNvramRequest& value,
                                          int indent_size);
std::string GetProtoDebugString(const DestroyNvramRequest& value);
std::string GetProtoDebugStringWithIndent(const DestroyNvramReply& value,
                                          int indent_size);
std::string GetProtoDebugString(const DestroyNvramReply& value);
std::string GetProtoDebugStringWithIndent(const WriteNvramRequest& value,
                                          int indent_size);
std::string GetProtoDebugString(const WriteNvramRequest& value);
std::string GetProtoDebugStringWithIndent(const WriteNvramReply& value,
                                          int indent_size);
std::string GetProtoDebugString(const WriteNvramReply& value);
std::string GetProtoDebugStringWithIndent(const ReadNvramRequest& value,
                                          int indent_size);
std::string GetProtoDebugString(const ReadNvramRequest& value);
std::string GetProtoDebugStringWithIndent(const ReadNvramReply& value,
                                          int indent_size);
std::string GetProtoDebugString(const ReadNvramReply& value);
std::string GetProtoDebugStringWithIndent(const IsNvramDefinedRequest& value,
                                          int indent_size);
std::string GetProtoDebugString(const IsNvramDefinedRequest& value);
std::string GetProtoDebugStringWithIndent(const IsNvramDefinedReply& value,
                                          int indent_size);
std::string GetProtoDebugString(const IsNvramDefinedReply& value);
std::string GetProtoDebugStringWithIndent(const IsNvramLockedRequest& value,
                                          int indent_size);
std::string GetProtoDebugString(const IsNvramLockedRequest& value);
std::string GetProtoDebugStringWithIndent(const IsNvramLockedReply& value,
                                          int indent_size);
std::string GetProtoDebugString(const IsNvramLockedReply& value);
std::string GetProtoDebugStringWithIndent(const GetNvramSizeRequest& value,
                                          int indent_size);
std::string GetProtoDebugString(const GetNvramSizeRequest& value);
std::string GetProtoDebugStringWithIndent(const GetNvramSizeReply& value,
                                          int indent_size);
std::string GetProtoDebugString(const GetNvramSizeReply& value);

}  // namespace tpm_manager

#endif  // TPM_MANAGER_COMMON_PRINT_TPM_NVRAM_INTERFACE_PROTO_H_
