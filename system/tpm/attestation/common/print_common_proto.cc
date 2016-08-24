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

#include "attestation/common/print_common_proto.h"

#include <string>

#include <base/strings/string_number_conversions.h>
#include <base/strings/stringprintf.h>

namespace attestation {

std::string GetProtoDebugString(KeyType value) {
  return GetProtoDebugStringWithIndent(value, 0);
}

std::string GetProtoDebugStringWithIndent(KeyType value, int indent_size) {
  if (value == KEY_TYPE_RSA) {
    return "KEY_TYPE_RSA";
  }
  if (value == KEY_TYPE_ECC) {
    return "KEY_TYPE_ECC";
  }
  return "<unknown>";
}

std::string GetProtoDebugString(KeyUsage value) {
  return GetProtoDebugStringWithIndent(value, 0);
}

std::string GetProtoDebugStringWithIndent(KeyUsage value, int indent_size) {
  if (value == KEY_USAGE_SIGN) {
    return "KEY_USAGE_SIGN";
  }
  if (value == KEY_USAGE_DECRYPT) {
    return "KEY_USAGE_DECRYPT";
  }
  return "<unknown>";
}

std::string GetProtoDebugString(CertificateProfile value) {
  return GetProtoDebugStringWithIndent(value, 0);
}

std::string GetProtoDebugStringWithIndent(CertificateProfile value,
                                          int indent_size) {
  if (value == ENTERPRISE_MACHINE_CERTIFICATE) {
    return "ENTERPRISE_MACHINE_CERTIFICATE";
  }
  if (value == ENTERPRISE_USER_CERTIFICATE) {
    return "ENTERPRISE_USER_CERTIFICATE";
  }
  if (value == CONTENT_PROTECTION_CERTIFICATE) {
    return "CONTENT_PROTECTION_CERTIFICATE";
  }
  if (value == CONTENT_PROTECTION_CERTIFICATE_WITH_STABLE_ID) {
    return "CONTENT_PROTECTION_CERTIFICATE_WITH_STABLE_ID";
  }
  if (value == CAST_CERTIFICATE) {
    return "CAST_CERTIFICATE";
  }
  if (value == GFSC_CERTIFICATE) {
    return "GFSC_CERTIFICATE";
  }
  return "<unknown>";
}

std::string GetProtoDebugString(const Quote& value) {
  return GetProtoDebugStringWithIndent(value, 0);
}

std::string GetProtoDebugStringWithIndent(const Quote& value, int indent_size) {
  std::string indent(indent_size, ' ');
  std::string output =
      base::StringPrintf("[%s] {\n", value.GetTypeName().c_str());

  if (value.has_quote()) {
    output += indent + "  quote: ";
    base::StringAppendF(
        &output, "%s",
        base::HexEncode(value.quote().data(), value.quote().size()).c_str());
    output += "\n";
  }
  if (value.has_quoted_data()) {
    output += indent + "  quoted_data: ";
    base::StringAppendF(&output, "%s",
                        base::HexEncode(value.quoted_data().data(),
                                        value.quoted_data().size()).c_str());
    output += "\n";
  }
  if (value.has_quoted_pcr_value()) {
    output += indent + "  quoted_pcr_value: ";
    base::StringAppendF(
        &output, "%s",
        base::HexEncode(value.quoted_pcr_value().data(),
                        value.quoted_pcr_value().size()).c_str());
    output += "\n";
  }
  if (value.has_pcr_source_hint()) {
    output += indent + "  pcr_source_hint: ";
    base::StringAppendF(
        &output, "%s", base::HexEncode(value.pcr_source_hint().data(),
                                       value.pcr_source_hint().size()).c_str());
    output += "\n";
  }
  output += indent + "}\n";
  return output;
}

std::string GetProtoDebugString(const EncryptedData& value) {
  return GetProtoDebugStringWithIndent(value, 0);
}

std::string GetProtoDebugStringWithIndent(const EncryptedData& value,
                                          int indent_size) {
  std::string indent(indent_size, ' ');
  std::string output =
      base::StringPrintf("[%s] {\n", value.GetTypeName().c_str());

  if (value.has_wrapped_key()) {
    output += indent + "  wrapped_key: ";
    base::StringAppendF(&output, "%s",
                        base::HexEncode(value.wrapped_key().data(),
                                        value.wrapped_key().size()).c_str());
    output += "\n";
  }
  if (value.has_iv()) {
    output += indent + "  iv: ";
    base::StringAppendF(
        &output, "%s",
        base::HexEncode(value.iv().data(), value.iv().size()).c_str());
    output += "\n";
  }
  if (value.has_mac()) {
    output += indent + "  mac: ";
    base::StringAppendF(
        &output, "%s",
        base::HexEncode(value.mac().data(), value.mac().size()).c_str());
    output += "\n";
  }
  if (value.has_encrypted_data()) {
    output += indent + "  encrypted_data: ";
    base::StringAppendF(&output, "%s",
                        base::HexEncode(value.encrypted_data().data(),
                                        value.encrypted_data().size()).c_str());
    output += "\n";
  }
  if (value.has_wrapping_key_id()) {
    output += indent + "  wrapping_key_id: ";
    base::StringAppendF(
        &output, "%s", base::HexEncode(value.wrapping_key_id().data(),
                                       value.wrapping_key_id().size()).c_str());
    output += "\n";
  }
  output += indent + "}\n";
  return output;
}

std::string GetProtoDebugString(const SignedData& value) {
  return GetProtoDebugStringWithIndent(value, 0);
}

std::string GetProtoDebugStringWithIndent(const SignedData& value,
                                          int indent_size) {
  std::string indent(indent_size, ' ');
  std::string output =
      base::StringPrintf("[%s] {\n", value.GetTypeName().c_str());

  if (value.has_data()) {
    output += indent + "  data: ";
    base::StringAppendF(
        &output, "%s",
        base::HexEncode(value.data().data(), value.data().size()).c_str());
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

std::string GetProtoDebugString(const EncryptedIdentityCredential& value) {
  return GetProtoDebugStringWithIndent(value, 0);
}

std::string GetProtoDebugStringWithIndent(
    const EncryptedIdentityCredential& value,
    int indent_size) {
  std::string indent(indent_size, ' ');
  std::string output =
      base::StringPrintf("[%s] {\n", value.GetTypeName().c_str());

  if (value.has_asym_ca_contents()) {
    output += indent + "  asym_ca_contents: ";
    base::StringAppendF(
        &output, "%s",
        base::HexEncode(value.asym_ca_contents().data(),
                        value.asym_ca_contents().size()).c_str());
    output += "\n";
  }
  if (value.has_sym_ca_attestation()) {
    output += indent + "  sym_ca_attestation: ";
    base::StringAppendF(
        &output, "%s",
        base::HexEncode(value.sym_ca_attestation().data(),
                        value.sym_ca_attestation().size()).c_str());
    output += "\n";
  }
  output += indent + "}\n";
  return output;
}

}  // namespace attestation
