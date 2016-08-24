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

#include "attestation/common/print_interface_proto.h"

#include <string>

#include <base/strings/string_number_conversions.h>
#include <base/strings/stringprintf.h>

#include "attestation/common/print_common_proto.h"

namespace attestation {

std::string GetProtoDebugString(AttestationStatus value) {
  return GetProtoDebugStringWithIndent(value, 0);
}

std::string GetProtoDebugStringWithIndent(AttestationStatus value,
                                          int indent_size) {
  if (value == STATUS_SUCCESS) {
    return "STATUS_SUCCESS";
  }
  if (value == STATUS_UNEXPECTED_DEVICE_ERROR) {
    return "STATUS_UNEXPECTED_DEVICE_ERROR";
  }
  if (value == STATUS_NOT_AVAILABLE) {
    return "STATUS_NOT_AVAILABLE";
  }
  if (value == STATUS_NOT_READY) {
    return "STATUS_NOT_READY";
  }
  if (value == STATUS_NOT_ALLOWED) {
    return "STATUS_NOT_ALLOWED";
  }
  if (value == STATUS_INVALID_PARAMETER) {
    return "STATUS_INVALID_PARAMETER";
  }
  if (value == STATUS_REQUEST_DENIED_BY_CA) {
    return "STATUS_REQUEST_DENIED_BY_CA";
  }
  if (value == STATUS_CA_NOT_AVAILABLE) {
    return "STATUS_CA_NOT_AVAILABLE";
  }
  return "<unknown>";
}

std::string GetProtoDebugString(const CreateGoogleAttestedKeyRequest& value) {
  return GetProtoDebugStringWithIndent(value, 0);
}

std::string GetProtoDebugStringWithIndent(
    const CreateGoogleAttestedKeyRequest& value,
    int indent_size) {
  std::string indent(indent_size, ' ');
  std::string output =
      base::StringPrintf("[%s] {\n", value.GetTypeName().c_str());

  if (value.has_key_label()) {
    output += indent + "  key_label: ";
    base::StringAppendF(&output, "%s", value.key_label().c_str());
    output += "\n";
  }
  if (value.has_key_type()) {
    output += indent + "  key_type: ";
    base::StringAppendF(&output, "%s",
                        GetProtoDebugStringWithIndent(value.key_type(),
                                                      indent_size + 2).c_str());
    output += "\n";
  }
  if (value.has_key_usage()) {
    output += indent + "  key_usage: ";
    base::StringAppendF(&output, "%s",
                        GetProtoDebugStringWithIndent(value.key_usage(),
                                                      indent_size + 2).c_str());
    output += "\n";
  }
  if (value.has_certificate_profile()) {
    output += indent + "  certificate_profile: ";
    base::StringAppendF(&output, "%s", GetProtoDebugStringWithIndent(
                                           value.certificate_profile(),
                                           indent_size + 2).c_str());
    output += "\n";
  }
  if (value.has_username()) {
    output += indent + "  username: ";
    base::StringAppendF(&output, "%s", value.username().c_str());
    output += "\n";
  }
  if (value.has_origin()) {
    output += indent + "  origin: ";
    base::StringAppendF(&output, "%s", value.origin().c_str());
    output += "\n";
  }
  output += indent + "}\n";
  return output;
}

std::string GetProtoDebugString(const CreateGoogleAttestedKeyReply& value) {
  return GetProtoDebugStringWithIndent(value, 0);
}

std::string GetProtoDebugStringWithIndent(
    const CreateGoogleAttestedKeyReply& value,
    int indent_size) {
  std::string indent(indent_size, ' ');
  std::string output =
      base::StringPrintf("[%s] {\n", value.GetTypeName().c_str());

  if (value.has_status()) {
    output += indent + "  status: ";
    base::StringAppendF(
        &output, "%s",
        GetProtoDebugStringWithIndent(value.status(), indent_size + 2).c_str());
    output += "\n";
  }
  if (value.has_server_error()) {
    output += indent + "  server_error: ";
    base::StringAppendF(&output, "%s", value.server_error().c_str());
    output += "\n";
  }
  if (value.has_certificate_chain()) {
    output += indent + "  certificate_chain: ";
    base::StringAppendF(&output, "%s", value.certificate_chain().c_str());
    output += "\n";
  }
  output += indent + "}\n";
  return output;
}

std::string GetProtoDebugString(const GetKeyInfoRequest& value) {
  return GetProtoDebugStringWithIndent(value, 0);
}

std::string GetProtoDebugStringWithIndent(const GetKeyInfoRequest& value,
                                          int indent_size) {
  std::string indent(indent_size, ' ');
  std::string output =
      base::StringPrintf("[%s] {\n", value.GetTypeName().c_str());

  if (value.has_key_label()) {
    output += indent + "  key_label: ";
    base::StringAppendF(&output, "%s", value.key_label().c_str());
    output += "\n";
  }
  if (value.has_username()) {
    output += indent + "  username: ";
    base::StringAppendF(&output, "%s", value.username().c_str());
    output += "\n";
  }
  output += indent + "}\n";
  return output;
}

std::string GetProtoDebugString(const GetKeyInfoReply& value) {
  return GetProtoDebugStringWithIndent(value, 0);
}

std::string GetProtoDebugStringWithIndent(const GetKeyInfoReply& value,
                                          int indent_size) {
  std::string indent(indent_size, ' ');
  std::string output =
      base::StringPrintf("[%s] {\n", value.GetTypeName().c_str());

  if (value.has_status()) {
    output += indent + "  status: ";
    base::StringAppendF(
        &output, "%s",
        GetProtoDebugStringWithIndent(value.status(), indent_size + 2).c_str());
    output += "\n";
  }
  if (value.has_key_type()) {
    output += indent + "  key_type: ";
    base::StringAppendF(&output, "%s",
                        GetProtoDebugStringWithIndent(value.key_type(),
                                                      indent_size + 2).c_str());
    output += "\n";
  }
  if (value.has_key_usage()) {
    output += indent + "  key_usage: ";
    base::StringAppendF(&output, "%s",
                        GetProtoDebugStringWithIndent(value.key_usage(),
                                                      indent_size + 2).c_str());
    output += "\n";
  }
  if (value.has_public_key()) {
    output += indent + "  public_key: ";
    base::StringAppendF(&output, "%s",
                        base::HexEncode(value.public_key().data(),
                                        value.public_key().size()).c_str());
    output += "\n";
  }
  if (value.has_certify_info()) {
    output += indent + "  certify_info: ";
    base::StringAppendF(&output, "%s",
                        base::HexEncode(value.certify_info().data(),
                                        value.certify_info().size()).c_str());
    output += "\n";
  }
  if (value.has_certify_info_signature()) {
    output += indent + "  certify_info_signature: ";
    base::StringAppendF(
        &output, "%s",
        base::HexEncode(value.certify_info_signature().data(),
                        value.certify_info_signature().size()).c_str());
    output += "\n";
  }
  if (value.has_certificate()) {
    output += indent + "  certificate: ";
    base::StringAppendF(&output, "%s",
                        base::HexEncode(value.certificate().data(),
                                        value.certificate().size()).c_str());
    output += "\n";
  }
  output += indent + "}\n";
  return output;
}

std::string GetProtoDebugString(const GetEndorsementInfoRequest& value) {
  return GetProtoDebugStringWithIndent(value, 0);
}

std::string GetProtoDebugStringWithIndent(
    const GetEndorsementInfoRequest& value,
    int indent_size) {
  std::string indent(indent_size, ' ');
  std::string output =
      base::StringPrintf("[%s] {\n", value.GetTypeName().c_str());

  if (value.has_key_type()) {
    output += indent + "  key_type: ";
    base::StringAppendF(&output, "%s",
                        GetProtoDebugStringWithIndent(value.key_type(),
                                                      indent_size + 2).c_str());
    output += "\n";
  }
  output += indent + "}\n";
  return output;
}

std::string GetProtoDebugString(const GetEndorsementInfoReply& value) {
  return GetProtoDebugStringWithIndent(value, 0);
}

std::string GetProtoDebugStringWithIndent(const GetEndorsementInfoReply& value,
                                          int indent_size) {
  std::string indent(indent_size, ' ');
  std::string output =
      base::StringPrintf("[%s] {\n", value.GetTypeName().c_str());

  if (value.has_status()) {
    output += indent + "  status: ";
    base::StringAppendF(
        &output, "%s",
        GetProtoDebugStringWithIndent(value.status(), indent_size + 2).c_str());
    output += "\n";
  }
  if (value.has_ek_public_key()) {
    output += indent + "  ek_public_key: ";
    base::StringAppendF(&output, "%s",
                        base::HexEncode(value.ek_public_key().data(),
                                        value.ek_public_key().size()).c_str());
    output += "\n";
  }
  if (value.has_ek_certificate()) {
    output += indent + "  ek_certificate: ";
    base::StringAppendF(&output, "%s",
                        base::HexEncode(value.ek_certificate().data(),
                                        value.ek_certificate().size()).c_str());
    output += "\n";
  }
  output += indent + "}\n";
  return output;
}

std::string GetProtoDebugString(const GetAttestationKeyInfoRequest& value) {
  return GetProtoDebugStringWithIndent(value, 0);
}

std::string GetProtoDebugStringWithIndent(
    const GetAttestationKeyInfoRequest& value,
    int indent_size) {
  std::string indent(indent_size, ' ');
  std::string output =
      base::StringPrintf("[%s] {\n", value.GetTypeName().c_str());

  if (value.has_key_type()) {
    output += indent + "  key_type: ";
    base::StringAppendF(&output, "%s",
                        GetProtoDebugStringWithIndent(value.key_type(),
                                                      indent_size + 2).c_str());
    output += "\n";
  }
  output += indent + "}\n";
  return output;
}

std::string GetProtoDebugString(const GetAttestationKeyInfoReply& value) {
  return GetProtoDebugStringWithIndent(value, 0);
}

std::string GetProtoDebugStringWithIndent(
    const GetAttestationKeyInfoReply& value,
    int indent_size) {
  std::string indent(indent_size, ' ');
  std::string output =
      base::StringPrintf("[%s] {\n", value.GetTypeName().c_str());

  if (value.has_status()) {
    output += indent + "  status: ";
    base::StringAppendF(
        &output, "%s",
        GetProtoDebugStringWithIndent(value.status(), indent_size + 2).c_str());
    output += "\n";
  }
  if (value.has_public_key()) {
    output += indent + "  public_key: ";
    base::StringAppendF(&output, "%s",
                        base::HexEncode(value.public_key().data(),
                                        value.public_key().size()).c_str());
    output += "\n";
  }
  if (value.has_public_key_tpm_format()) {
    output += indent + "  public_key_tpm_format: ";
    base::StringAppendF(
        &output, "%s",
        base::HexEncode(value.public_key_tpm_format().data(),
                        value.public_key_tpm_format().size()).c_str());
    output += "\n";
  }
  if (value.has_certificate()) {
    output += indent + "  certificate: ";
    base::StringAppendF(&output, "%s",
                        base::HexEncode(value.certificate().data(),
                                        value.certificate().size()).c_str());
    output += "\n";
  }
  if (value.has_pcr0_quote()) {
    output += indent + "  pcr0_quote: ";
    base::StringAppendF(&output, "%s",
                        GetProtoDebugStringWithIndent(value.pcr0_quote(),
                                                      indent_size + 2).c_str());
    output += "\n";
  }
  if (value.has_pcr1_quote()) {
    output += indent + "  pcr1_quote: ";
    base::StringAppendF(&output, "%s",
                        GetProtoDebugStringWithIndent(value.pcr1_quote(),
                                                      indent_size + 2).c_str());
    output += "\n";
  }
  output += indent + "}\n";
  return output;
}

std::string GetProtoDebugString(const ActivateAttestationKeyRequest& value) {
  return GetProtoDebugStringWithIndent(value, 0);
}

std::string GetProtoDebugStringWithIndent(
    const ActivateAttestationKeyRequest& value,
    int indent_size) {
  std::string indent(indent_size, ' ');
  std::string output =
      base::StringPrintf("[%s] {\n", value.GetTypeName().c_str());

  if (value.has_key_type()) {
    output += indent + "  key_type: ";
    base::StringAppendF(&output, "%s",
                        GetProtoDebugStringWithIndent(value.key_type(),
                                                      indent_size + 2).c_str());
    output += "\n";
  }
  if (value.has_encrypted_certificate()) {
    output += indent + "  encrypted_certificate: ";
    base::StringAppendF(&output, "%s", GetProtoDebugStringWithIndent(
                                           value.encrypted_certificate(),
                                           indent_size + 2).c_str());
    output += "\n";
  }
  if (value.has_save_certificate()) {
    output += indent + "  save_certificate: ";
    base::StringAppendF(&output, "%s",
                        value.save_certificate() ? "true" : "false");
    output += "\n";
  }
  output += indent + "}\n";
  return output;
}

std::string GetProtoDebugString(const ActivateAttestationKeyReply& value) {
  return GetProtoDebugStringWithIndent(value, 0);
}

std::string GetProtoDebugStringWithIndent(
    const ActivateAttestationKeyReply& value,
    int indent_size) {
  std::string indent(indent_size, ' ');
  std::string output =
      base::StringPrintf("[%s] {\n", value.GetTypeName().c_str());

  if (value.has_status()) {
    output += indent + "  status: ";
    base::StringAppendF(
        &output, "%s",
        GetProtoDebugStringWithIndent(value.status(), indent_size + 2).c_str());
    output += "\n";
  }
  if (value.has_certificate()) {
    output += indent + "  certificate: ";
    base::StringAppendF(&output, "%s",
                        base::HexEncode(value.certificate().data(),
                                        value.certificate().size()).c_str());
    output += "\n";
  }
  output += indent + "}\n";
  return output;
}

std::string GetProtoDebugString(const CreateCertifiableKeyRequest& value) {
  return GetProtoDebugStringWithIndent(value, 0);
}

std::string GetProtoDebugStringWithIndent(
    const CreateCertifiableKeyRequest& value,
    int indent_size) {
  std::string indent(indent_size, ' ');
  std::string output =
      base::StringPrintf("[%s] {\n", value.GetTypeName().c_str());

  if (value.has_key_label()) {
    output += indent + "  key_label: ";
    base::StringAppendF(&output, "%s", value.key_label().c_str());
    output += "\n";
  }
  if (value.has_username()) {
    output += indent + "  username: ";
    base::StringAppendF(&output, "%s", value.username().c_str());
    output += "\n";
  }
  if (value.has_key_type()) {
    output += indent + "  key_type: ";
    base::StringAppendF(&output, "%s",
                        GetProtoDebugStringWithIndent(value.key_type(),
                                                      indent_size + 2).c_str());
    output += "\n";
  }
  if (value.has_key_usage()) {
    output += indent + "  key_usage: ";
    base::StringAppendF(&output, "%s",
                        GetProtoDebugStringWithIndent(value.key_usage(),
                                                      indent_size + 2).c_str());
    output += "\n";
  }
  output += indent + "}\n";
  return output;
}

std::string GetProtoDebugString(const CreateCertifiableKeyReply& value) {
  return GetProtoDebugStringWithIndent(value, 0);
}

std::string GetProtoDebugStringWithIndent(
    const CreateCertifiableKeyReply& value,
    int indent_size) {
  std::string indent(indent_size, ' ');
  std::string output =
      base::StringPrintf("[%s] {\n", value.GetTypeName().c_str());

  if (value.has_status()) {
    output += indent + "  status: ";
    base::StringAppendF(
        &output, "%s",
        GetProtoDebugStringWithIndent(value.status(), indent_size + 2).c_str());
    output += "\n";
  }
  if (value.has_public_key()) {
    output += indent + "  public_key: ";
    base::StringAppendF(&output, "%s",
                        base::HexEncode(value.public_key().data(),
                                        value.public_key().size()).c_str());
    output += "\n";
  }
  if (value.has_certify_info()) {
    output += indent + "  certify_info: ";
    base::StringAppendF(&output, "%s",
                        base::HexEncode(value.certify_info().data(),
                                        value.certify_info().size()).c_str());
    output += "\n";
  }
  if (value.has_certify_info_signature()) {
    output += indent + "  certify_info_signature: ";
    base::StringAppendF(
        &output, "%s",
        base::HexEncode(value.certify_info_signature().data(),
                        value.certify_info_signature().size()).c_str());
    output += "\n";
  }
  output += indent + "}\n";
  return output;
}

std::string GetProtoDebugString(const DecryptRequest& value) {
  return GetProtoDebugStringWithIndent(value, 0);
}

std::string GetProtoDebugStringWithIndent(const DecryptRequest& value,
                                          int indent_size) {
  std::string indent(indent_size, ' ');
  std::string output =
      base::StringPrintf("[%s] {\n", value.GetTypeName().c_str());

  if (value.has_key_label()) {
    output += indent + "  key_label: ";
    base::StringAppendF(&output, "%s", value.key_label().c_str());
    output += "\n";
  }
  if (value.has_username()) {
    output += indent + "  username: ";
    base::StringAppendF(&output, "%s", value.username().c_str());
    output += "\n";
  }
  if (value.has_encrypted_data()) {
    output += indent + "  encrypted_data: ";
    base::StringAppendF(&output, "%s",
                        base::HexEncode(value.encrypted_data().data(),
                                        value.encrypted_data().size()).c_str());
    output += "\n";
  }
  output += indent + "}\n";
  return output;
}

std::string GetProtoDebugString(const DecryptReply& value) {
  return GetProtoDebugStringWithIndent(value, 0);
}

std::string GetProtoDebugStringWithIndent(const DecryptReply& value,
                                          int indent_size) {
  std::string indent(indent_size, ' ');
  std::string output =
      base::StringPrintf("[%s] {\n", value.GetTypeName().c_str());

  if (value.has_status()) {
    output += indent + "  status: ";
    base::StringAppendF(
        &output, "%s",
        GetProtoDebugStringWithIndent(value.status(), indent_size + 2).c_str());
    output += "\n";
  }
  if (value.has_decrypted_data()) {
    output += indent + "  decrypted_data: ";
    base::StringAppendF(&output, "%s",
                        base::HexEncode(value.decrypted_data().data(),
                                        value.decrypted_data().size()).c_str());
    output += "\n";
  }
  output += indent + "}\n";
  return output;
}

std::string GetProtoDebugString(const SignRequest& value) {
  return GetProtoDebugStringWithIndent(value, 0);
}

std::string GetProtoDebugStringWithIndent(const SignRequest& value,
                                          int indent_size) {
  std::string indent(indent_size, ' ');
  std::string output =
      base::StringPrintf("[%s] {\n", value.GetTypeName().c_str());

  if (value.has_key_label()) {
    output += indent + "  key_label: ";
    base::StringAppendF(&output, "%s", value.key_label().c_str());
    output += "\n";
  }
  if (value.has_username()) {
    output += indent + "  username: ";
    base::StringAppendF(&output, "%s", value.username().c_str());
    output += "\n";
  }
  if (value.has_data_to_sign()) {
    output += indent + "  data_to_sign: ";
    base::StringAppendF(&output, "%s",
                        base::HexEncode(value.data_to_sign().data(),
                                        value.data_to_sign().size()).c_str());
    output += "\n";
  }
  output += indent + "}\n";
  return output;
}

std::string GetProtoDebugString(const SignReply& value) {
  return GetProtoDebugStringWithIndent(value, 0);
}

std::string GetProtoDebugStringWithIndent(const SignReply& value,
                                          int indent_size) {
  std::string indent(indent_size, ' ');
  std::string output =
      base::StringPrintf("[%s] {\n", value.GetTypeName().c_str());

  if (value.has_status()) {
    output += indent + "  status: ";
    base::StringAppendF(
        &output, "%s",
        GetProtoDebugStringWithIndent(value.status(), indent_size + 2).c_str());
    output += "\n";
  }
  if (value.has_signature()) {
    output += indent + "  signature: ";
    base::StringAppendF(&output, "%s",
                        base::HexEncode(value.signature().data(),
                                        value.signature().size()).c_str());
    output += "\n";
  }
  output += indent + "}\n";
  return output;
}

std::string GetProtoDebugString(const RegisterKeyWithChapsTokenRequest& value) {
  return GetProtoDebugStringWithIndent(value, 0);
}

std::string GetProtoDebugStringWithIndent(
    const RegisterKeyWithChapsTokenRequest& value,
    int indent_size) {
  std::string indent(indent_size, ' ');
  std::string output =
      base::StringPrintf("[%s] {\n", value.GetTypeName().c_str());

  if (value.has_key_label()) {
    output += indent + "  key_label: ";
    base::StringAppendF(&output, "%s", value.key_label().c_str());
    output += "\n";
  }
  if (value.has_username()) {
    output += indent + "  username: ";
    base::StringAppendF(&output, "%s", value.username().c_str());
    output += "\n";
  }
  output += indent + "}\n";
  return output;
}

std::string GetProtoDebugString(const RegisterKeyWithChapsTokenReply& value) {
  return GetProtoDebugStringWithIndent(value, 0);
}

std::string GetProtoDebugStringWithIndent(
    const RegisterKeyWithChapsTokenReply& value,
    int indent_size) {
  std::string indent(indent_size, ' ');
  std::string output =
      base::StringPrintf("[%s] {\n", value.GetTypeName().c_str());

  if (value.has_status()) {
    output += indent + "  status: ";
    base::StringAppendF(
        &output, "%s",
        GetProtoDebugStringWithIndent(value.status(), indent_size + 2).c_str());
    output += "\n";
  }
  output += indent + "}\n";
  return output;
}

}  // namespace attestation
