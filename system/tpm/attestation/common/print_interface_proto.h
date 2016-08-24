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

#ifndef ATTESTATION_COMMON_PRINT_INTERFACE_PROTO_H_
#define ATTESTATION_COMMON_PRINT_INTERFACE_PROTO_H_

#include <string>

#include "attestation/common/interface.pb.h"

namespace attestation {

std::string GetProtoDebugStringWithIndent(AttestationStatus value,
                                          int indent_size);
std::string GetProtoDebugString(AttestationStatus value);
std::string GetProtoDebugStringWithIndent(
    const CreateGoogleAttestedKeyRequest& value,
    int indent_size);
std::string GetProtoDebugString(const CreateGoogleAttestedKeyRequest& value);
std::string GetProtoDebugStringWithIndent(
    const CreateGoogleAttestedKeyReply& value,
    int indent_size);
std::string GetProtoDebugString(const CreateGoogleAttestedKeyReply& value);
std::string GetProtoDebugStringWithIndent(const GetKeyInfoRequest& value,
                                          int indent_size);
std::string GetProtoDebugString(const GetKeyInfoRequest& value);
std::string GetProtoDebugStringWithIndent(const GetKeyInfoReply& value,
                                          int indent_size);
std::string GetProtoDebugString(const GetKeyInfoReply& value);
std::string GetProtoDebugStringWithIndent(
    const GetEndorsementInfoRequest& value,
    int indent_size);
std::string GetProtoDebugString(const GetEndorsementInfoRequest& value);
std::string GetProtoDebugStringWithIndent(const GetEndorsementInfoReply& value,
                                          int indent_size);
std::string GetProtoDebugString(const GetEndorsementInfoReply& value);
std::string GetProtoDebugStringWithIndent(
    const GetAttestationKeyInfoRequest& value,
    int indent_size);
std::string GetProtoDebugString(const GetAttestationKeyInfoRequest& value);
std::string GetProtoDebugStringWithIndent(
    const GetAttestationKeyInfoReply& value,
    int indent_size);
std::string GetProtoDebugString(const GetAttestationKeyInfoReply& value);
std::string GetProtoDebugStringWithIndent(
    const ActivateAttestationKeyRequest& value,
    int indent_size);
std::string GetProtoDebugString(const ActivateAttestationKeyRequest& value);
std::string GetProtoDebugStringWithIndent(
    const ActivateAttestationKeyReply& value,
    int indent_size);
std::string GetProtoDebugString(const ActivateAttestationKeyReply& value);
std::string GetProtoDebugStringWithIndent(
    const CreateCertifiableKeyRequest& value,
    int indent_size);
std::string GetProtoDebugString(const CreateCertifiableKeyRequest& value);
std::string GetProtoDebugStringWithIndent(
    const CreateCertifiableKeyReply& value,
    int indent_size);
std::string GetProtoDebugString(const CreateCertifiableKeyReply& value);
std::string GetProtoDebugStringWithIndent(const DecryptRequest& value,
                                          int indent_size);
std::string GetProtoDebugString(const DecryptRequest& value);
std::string GetProtoDebugStringWithIndent(const DecryptReply& value,
                                          int indent_size);
std::string GetProtoDebugString(const DecryptReply& value);
std::string GetProtoDebugStringWithIndent(const SignRequest& value,
                                          int indent_size);
std::string GetProtoDebugString(const SignRequest& value);
std::string GetProtoDebugStringWithIndent(const SignReply& value,
                                          int indent_size);
std::string GetProtoDebugString(const SignReply& value);
std::string GetProtoDebugStringWithIndent(
    const RegisterKeyWithChapsTokenRequest& value,
    int indent_size);
std::string GetProtoDebugString(const RegisterKeyWithChapsTokenRequest& value);
std::string GetProtoDebugStringWithIndent(
    const RegisterKeyWithChapsTokenReply& value,
    int indent_size);
std::string GetProtoDebugString(const RegisterKeyWithChapsTokenReply& value);

}  // namespace attestation

#endif  // ATTESTATION_COMMON_PRINT_INTERFACE_PROTO_H_
