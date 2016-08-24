// Copyright 2014 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_DBUS_BINDINGS_ADAPTOR_GENERATOR_H_
#define CHROMEOS_DBUS_BINDINGS_ADAPTOR_GENERATOR_H_

#include <string>
#include <vector>

#include <base/macros.h>

#include "chromeos-dbus-bindings/header_generator.h"
#include "chromeos-dbus-bindings/indented_text.h"

namespace base {

class FilePath;

}  // namespace base

namespace chromeos_dbus_bindings {

class IndentedText;
struct Interface;

class AdaptorGenerator : public HeaderGenerator {
 public:
  static bool GenerateAdaptors(const std::vector<Interface>& interfaces,
                               const base::FilePath& output_file);

 private:
  friend class AdaptorGeneratorTest;

  // Generates one interface adaptor.
  static void GenerateInterfaceAdaptor(const Interface& interface,
                                       IndentedText *text);

  // Generates the method prototypes for an interface declaration.
  static void AddInterfaceMethods(const Interface& interface,
                                  IndentedText *text);

  // Generates the constructor for the adaptor.
  static void AddConstructor(const std::string& class_name,
                             const std::string& itf_name,
                             IndentedText *text);

  // Generates RegisterWithDBusObject() method.
  static void AddRegisterWithDBusObject(const std::string& itf_name,
                                        const Interface& interface,
                                        IndentedText *text);

  // Generates the code to register the interface with a D-Bus object.
  static void RegisterInterface(const std::string& itf_name,
                                const Interface& interface,
                                IndentedText *text);

  // Generates adaptor methods to send the signals.
  static void AddSendSignalMethods(const Interface& interface,
                                   IndentedText *text);

  // Generates DBusSignal data members for the signals.
  static void AddSignalDataMembers(const Interface& interface,
                                   IndentedText *text);

  // Generates adaptor accessor methods for the properties.
  static void AddPropertyMethodImplementation(const Interface& interface,
                                              IndentedText *text);

  // Generate ExportProperty data members for the properties.
  static void AddPropertyDataMembers(const Interface& interface,
                                     IndentedText *text);

  DISALLOW_COPY_AND_ASSIGN(AdaptorGenerator);
};

}  // namespace chromeos_dbus_bindings

#endif  // CHROMEOS_DBUS_BINDINGS_ADAPTOR_GENERATOR_H_
