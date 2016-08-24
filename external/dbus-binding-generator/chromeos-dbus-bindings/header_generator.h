// Copyright 2014 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_DBUS_BINDINGS_HEADER_GENERATOR_H_
#define CHROMEOS_DBUS_BINDINGS_HEADER_GENERATOR_H_

#include <string>
#include <vector>

#include <base/macros.h>

namespace base {

class FilePath;

};

namespace chromeos_dbus_bindings {

struct Interface;
class  IndentedText;

// General D-Bus service configuration settings used by Adaptor/Proxy code
// generators.
struct ServiceConfig {
  // D-Bus service name to be used when constructing proxy objects.
  // If omitted (empty), the service name parameter will be added to the
  // constructor of generated proxy class(es).
  std::string service_name;
  // Object Manager settings.
  struct {
    // The name of the Object Manager class to use. If empty, no object manager
    // is generated in the proxy code (this also disables property support on
    // proxy objects).
    // This is a "fake" name used to generate namespaces and the actual class
    // name for the object manager proxy. This name has no relationship to the
    // actual D-Bus properties of the actual object manager.
    std::string name;
    // The D-Bus path to Object Manager instance.
    std::string object_path;
  } object_manager;

  // A list of interfaces we should ignore and not generate any adaptors and
  // proxies for.
  std::vector<std::string> ignore_interfaces;
};

class HeaderGenerator {
 protected:
  // Create a unique header guard string to protect multiple includes of header.
  static std::string GenerateHeaderGuard(const base::FilePath& output_file);

  // Used to decide whether the argument should be a const reference.
  static bool IsIntegralType(const std::string& type);

  // If |type| is a non-integral type, converts it into a const reference.
  static void MakeConstReferenceIfNeeded(std::string* type);

  // Writes indented text to a file.
  static bool WriteTextToFile(const base::FilePath& output_file,
                              const IndentedText& text);

  // Generate a name of a method/signal argument based on the name provided in
  // the XML file. If |arg_name| is empty, it generates a name using
  // the |arg_index| counter.
  static std::string GetArgName(const char* prefix,
                                const std::string& arg_name,
                                int arg_index);

  static const int kScopeOffset = 1;
  static const int kBlockOffset = 2;
  static const int kLineContinuationOffset = 4;

 private:
  DISALLOW_COPY_AND_ASSIGN(HeaderGenerator);
};

}  // namespace chromeos_dbus_bindings

#endif  // CHROMEOS_DBUS_BINDINGS_HEADER_GENERATOR_H_
