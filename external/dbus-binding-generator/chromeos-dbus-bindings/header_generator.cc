// Copyright 2014 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos-dbus-bindings/header_generator.h"

#include <string>

#include <base/files/file_path.h>
#include <base/files/file_util.h>
#include <base/strings/string_util.h>
#include <base/strings/stringprintf.h>
#include <brillo/strings/string_utils.h>

#include "chromeos-dbus-bindings/indented_text.h"

using std::string;
using std::vector;

namespace chromeos_dbus_bindings {

// static
string HeaderGenerator::GenerateHeaderGuard(
    const base::FilePath& output_file) {
  string guard = base::StringPrintf("____chromeos_dbus_binding__%s",
                                    output_file.value().c_str());
  for (auto& c : guard) {
    if (base::IsAsciiAlpha(c)) {
      c = base::ToUpperASCII(c);
    } else if (!base::IsAsciiDigit(c)) {
      c = '_';
    }
  }
  return guard;
}

// static
bool HeaderGenerator::IsIntegralType(const string& type) {
  return type.find("::") == std::string::npos;
}

// static
void HeaderGenerator::MakeConstReferenceIfNeeded(std::string* type) {
  if (!IsIntegralType(*type)) {
    *type = base::StringPrintf("const %s&", type->c_str());
  }
}

// static
bool HeaderGenerator::WriteTextToFile(
    const base::FilePath& output_file, const IndentedText &text) {
  string contents = text.GetContents();
  int expected_write_return = contents.size();
  if (base::WriteFile(output_file, contents.c_str(), contents.size()) !=
      expected_write_return) {
    LOG(ERROR) << "Failed to write file " << output_file.value();
    return false;
  }
  return true;
}

// static
string HeaderGenerator::GetArgName(const char* prefix,
                                   const string& arg_name,
                                   int arg_index) {
  string name = arg_name.empty() ? std::to_string(arg_index) : arg_name;
  return base::StringPrintf("%s_%s", prefix, name.c_str());
}

}  // namespace chromeos_dbus_bindings
