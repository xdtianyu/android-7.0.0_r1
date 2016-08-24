// Copyright 2014 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos-dbus-bindings/name_parser.h"

#include <string>

#include <base/strings/string_util.h>
#include <base/strings/stringprintf.h>
#include <brillo/strings/string_utils.h>

#include "chromeos-dbus-bindings/indented_text.h"

namespace chromeos_dbus_bindings {

namespace {

void AddOpenNamespace(IndentedText *text, const std::string& name) {
  text->AddLine(base::StringPrintf("namespace %s {", name.c_str()));
}

void AddCloseNamespace(IndentedText *text, const std::string& name) {
  text->AddLine(base::StringPrintf("}  // namespace %s", name.c_str()));
}

}  // anonymous namespace

NameParser::NameParser(const std::string& name)
    : namespaces{brillo::string_utils::Split(name, ".")} {
  CHECK(!namespaces.empty()) << "Empty name specified";
  type_name = namespaces.back();
  namespaces.pop_back();
}

std::string NameParser::MakeFullyQualified(const std::string& name) const {
  std::vector<std::string> parts = namespaces;
  parts.push_back(name);
  return brillo::string_utils::Join("::", parts);
}

std::string NameParser::MakeFullCppName() const {
  return MakeFullyQualified(type_name);
}

std::string NameParser::MakeVariableName() const {
  // Convert CamelCase name to google_style variable name.
  std::string result;
  bool last_upper = true;
  for (char c : type_name) {
    bool is_upper = isupper(c);
    if (is_upper) {
      if (!last_upper)
        result += '_';
      c = base::ToLowerASCII(c);
    }
    last_upper = is_upper;
    result.push_back(c);
  }
  return result;
}

std::string NameParser::MakeInterfaceName(bool fully_qualified) const {
  std::string interface_name = type_name + "Interface";
  return fully_qualified ? MakeFullyQualified(interface_name) : interface_name;
}

std::string NameParser::MakeProxyName(bool fully_qualified) const {
  std::string proxy_name = type_name + "Proxy";
  return fully_qualified ? MakeFullyQualified(proxy_name) : proxy_name;
}

std::string NameParser::MakeAdaptorName(bool fully_qualified) const {
  std::string adaptor_name = type_name + "Adaptor";
  return fully_qualified ? MakeFullyQualified(adaptor_name) : adaptor_name;
}

void NameParser::AddOpenNamespaces(IndentedText *text,
                                   bool add_main_type) const {
  for (const auto& ns : namespaces) {
    AddOpenNamespace(text, ns);
  }

  if (add_main_type)
    AddOpenNamespace(text, type_name);
}

void NameParser::AddCloseNamespaces(IndentedText *text,
                                    bool add_main_type) const {
  if (add_main_type)
    AddCloseNamespace(text, type_name);

  for (auto it = namespaces.rbegin(); it != namespaces.rend(); ++it) {
    AddCloseNamespace(text, *it);
  }
}

}  // namespace chromeos_dbus_bindings
