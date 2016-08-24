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

#include "tpm_manager/common/print_local_data_proto.h"

#include <string>

#include <base/strings/string_number_conversions.h>
#include <base/strings/stringprintf.h>

namespace tpm_manager {

std::string GetProtoDebugString(const LocalData& value) {
  return GetProtoDebugStringWithIndent(value, 0);
}

std::string GetProtoDebugStringWithIndent(const LocalData& value,
                                          int indent_size) {
  std::string indent(indent_size, ' ');
  std::string output =
      base::StringPrintf("[%s] {\n", value.GetTypeName().c_str());

  if (value.has_owner_password()) {
    output += indent + "  owner_password: ";
    base::StringAppendF(&output, "%s",
                        base::HexEncode(value.owner_password().data(),
                                        value.owner_password().size())
                            .c_str());
    output += "\n";
  }
  output += indent + "  owner_dependency: {";
  for (int i = 0; i < value.owner_dependency_size(); ++i) {
    base::StringAppendF(&output, "%s", value.owner_dependency(i).c_str());
  }
  output += "}\n";
  if (value.has_endorsement_password()) {
    output += indent + "  endorsement_password: ";
    base::StringAppendF(&output, "%s",
                        base::HexEncode(value.endorsement_password().data(),
                                        value.endorsement_password().size())
                            .c_str());
    output += "\n";
  }
  if (value.has_lockout_password()) {
    output += indent + "  lockout_password: ";
    base::StringAppendF(&output, "%s",
                        base::HexEncode(value.lockout_password().data(),
                                        value.lockout_password().size())
                            .c_str());
    output += "\n";
  }
  output += indent + "}\n";
  return output;
}

}  // namespace tpm_manager
