// Copyright 2014 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_DBUS_BINDINGS_NAME_PARSER_H_
#define CHROMEOS_DBUS_BINDINGS_NAME_PARSER_H_

#include <string>
#include <vector>

#include <base/macros.h>

namespace chromeos_dbus_bindings {

struct Interface;
class  IndentedText;

// A helper class that allows to decompose D-Bus name strings such as
// "org.chromium.TestInterface" into components and be able to construct the
// corresponding C++ identifiers, namespaces, variable names, etc.
class NameParser {
 public:
  explicit NameParser(const std::string& name);

  // Returns fully-qualified C++ type name for the current D-Bus name
  // for example "org::chromium::TestInterface".
  std::string MakeFullCppName() const;

  // Returns a variable name suitable for object of this type.
  // For example "test_interface".
  std::string MakeVariableName() const;

  // Returns a name of an interface for the given type, optionally qualifying
  // it with the C++ namespaces.
  std::string MakeInterfaceName(bool fully_qualified) const;

  // Returns a name of a proxy class for the given type, optionally qualifying
  // it with the C++ namespaces.
  std::string MakeProxyName(bool fully_qualified) const;

  // Returns a name of an adaptor class for the given type, optionally
  // qualifying it with the C++ namespaces.
  std::string MakeAdaptorName(bool fully_qualified) const;

  // Adds opening "namespace ... {" statements to |text|.
  // If |add_main_type| is true, adds the main type name as a namespace as well.
  void AddOpenNamespaces(IndentedText *text, bool add_main_type) const;

  // Adds closing "}  // namespace ..." statements to |text|.
  // If |add_main_type| is true, adds the main type name as a namespace as well.
  void AddCloseNamespaces(IndentedText *text, bool add_main_type) const;

  std::string type_name;  // e.g. "TestInterface".
  std::vector<std::string> namespaces;  // e.g. {"org", "chromium"}.

 private:
  // Helper function to prepend the C++ namespaces to the |name|.
  std::string MakeFullyQualified(const std::string& name) const;
};

}  // namespace chromeos_dbus_bindings

#endif  // CHROMEOS_DBUS_BINDINGS_NAME_PARSER_H_
