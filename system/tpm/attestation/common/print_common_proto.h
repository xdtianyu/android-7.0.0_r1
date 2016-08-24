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

#ifndef ATTESTATION_COMMON_PRINT_COMMON_PROTO_H_
#define ATTESTATION_COMMON_PRINT_COMMON_PROTO_H_

#include <string>

#include "attestation/common/common.pb.h"

namespace attestation {

std::string GetProtoDebugStringWithIndent(KeyType value, int indent_size);
std::string GetProtoDebugString(KeyType value);
std::string GetProtoDebugStringWithIndent(KeyUsage value, int indent_size);
std::string GetProtoDebugString(KeyUsage value);
std::string GetProtoDebugStringWithIndent(CertificateProfile value,
                                          int indent_size);
std::string GetProtoDebugString(CertificateProfile value);
std::string GetProtoDebugStringWithIndent(const Quote& value, int indent_size);
std::string GetProtoDebugString(const Quote& value);
std::string GetProtoDebugStringWithIndent(const EncryptedData& value,
                                          int indent_size);
std::string GetProtoDebugString(const EncryptedData& value);
std::string GetProtoDebugStringWithIndent(const SignedData& value,
                                          int indent_size);
std::string GetProtoDebugString(const SignedData& value);
std::string GetProtoDebugStringWithIndent(
    const EncryptedIdentityCredential& value,
    int indent_size);
std::string GetProtoDebugString(const EncryptedIdentityCredential& value);

}  // namespace attestation

#endif  // ATTESTATION_COMMON_PRINT_COMMON_PROTO_H_
