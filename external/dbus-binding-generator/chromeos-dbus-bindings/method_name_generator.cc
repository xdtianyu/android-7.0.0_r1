// Copyright 2014 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos-dbus-bindings/method_name_generator.h"

#include <base/files/file_path.h>
#include <base/strings/stringprintf.h>

#include "chromeos-dbus-bindings/indented_text.h"
#include "chromeos-dbus-bindings/interface.h"
#include "chromeos-dbus-bindings/name_parser.h"

using std::string;
using std::vector;

namespace chromeos_dbus_bindings {

// static
string MethodNameGenerator::GenerateMethodNameConstant(
    const string& method_name) {
  return "k" + method_name + "Method";
}

// static
bool MethodNameGenerator::GenerateMethodNames(
    const vector<Interface>& interfaces,
    const base::FilePath& output_file) {
  string contents;
  IndentedText text;
  for (const auto& interface : interfaces) {
    text.AddBlankLine();
    NameParser parser{interface.name};
    parser.AddOpenNamespaces(&text, true);
    for (const auto& method : interface.methods) {
      text.AddLine(
        base::StringPrintf("const char %s[] = \"%s\";",
                            GenerateMethodNameConstant(method.name).c_str(),
                            method.name.c_str()));
    }
    parser.AddCloseNamespaces(&text, true);
  }
  return HeaderGenerator::WriteTextToFile(output_file, text);
}

}  // namespace chromeos_dbus_bindings
