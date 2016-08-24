// Copyright 2014 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_DBUS_BINDINGS_PROXY_GENERATOR_H_
#define CHROMEOS_DBUS_BINDINGS_PROXY_GENERATOR_H_

#include <string>
#include <vector>

#include <base/macros.h>

#include "chromeos-dbus-bindings/header_generator.h"
#include "chromeos-dbus-bindings/indented_text.h"
#include "chromeos-dbus-bindings/interface.h"

namespace base {

class FilePath;

}  // namespace base

namespace chromeos_dbus_bindings {

class IndentedText;
struct Interface;

class ProxyGenerator : public HeaderGenerator {
 public:
  static bool GenerateProxies(const ServiceConfig& config,
                              const std::vector<Interface>& interfaces,
                              const base::FilePath& output_file);

  static bool GenerateMocks(const ServiceConfig& config,
                            const std::vector<Interface>& interfaces,
                            const base::FilePath& mock_file,
                            const base::FilePath& proxy_file,
                            bool use_literal_proxy_file);

 private:
  friend class ProxyGeneratorTest;

  // Generates an abstract interface for one D-Bus interface proxy.
  static void GenerateInterfaceProxyInterface(const ServiceConfig& config,
                                              const Interface& interface,
                                              IndentedText* text);

  // Generates one interface proxy.
  static void GenerateInterfaceProxy(const ServiceConfig& config,
                                     const Interface& interface,
                                     IndentedText* text);

  // Generates one interface mock object.
  static void GenerateInterfaceMock(const ServiceConfig& config,
                                    const Interface& interface,
                                    IndentedText* text);

  // Generates the constructor and destructor for the proxy.
  static void AddConstructor(const ServiceConfig& config,
                             const Interface& interface,
                             const std::string& class_name,
                             IndentedText* text);
  static void AddDestructor(const std::string& class_name,
                            IndentedText* text);

  // Generates ReleaseObjectProxy() method to release ownership
  // of the object proxy.
  static void AddReleaseObjectProxy(IndentedText* text);

  // Generates AddGetObjectPath() method.
  static void AddGetObjectPath(IndentedText* text);

  // Generates GetObjectProxy() method.
  static void AddGetObjectProxy(IndentedText* text);

  // Generates SetPropertyChangedCallback/GetProperties() methods.
  static void AddPropertyPublicMethods(const std::string& class_name,
                                       bool declaration_only,
                                       IndentedText* text);

  // Generates OnPropertyChanged() method.
  static void AddOnPropertyChanged(IndentedText* text);

  // Generates logic permitting users to register handlers for signals.
  static void AddSignalHandlerRegistration(const Interface::Signal& signal,
                                           const std::string& interface_name,
                                           bool declaration_only,
                                           IndentedText* text);

  // Generates the property set class to contain interface properties.
  static void AddPropertySet(const ServiceConfig& config,
                             const Interface& interface,
                             IndentedText* text);

  // Generates the property accessors.
  static void AddProperties(const ServiceConfig& config,
                            const Interface& interface,
                            bool declaration_only,
                            IndentedText* text);

  // Generates a native C++ method which calls a D-Bus method on the proxy.
  static void AddMethodProxy(const Interface::Method& interface,
                             const std::string& interface_name,
                             bool declaration_only,
                             IndentedText* text);

  // Generates a native C++ method which calls a D-Bus method asynchronously.
  static void AddAsyncMethodProxy(const Interface::Method& interface,
                                  const std::string& interface_name,
                                  bool declaration_only,
                                  IndentedText* text);

  // Generates a mock for blocking D-Bus method.
  static void AddMethodMock(const Interface::Method& interface,
                            const std::string& interface_name,
                            IndentedText* text);

  // Generates a mock for asynchronous D-Bus method.
  static void AddAsyncMethodMock(const Interface::Method& interface,
                                 const std::string& interface_name,
                                 IndentedText* text);

  // Generates the MOCK_METHOD entry for the given arguments handling methods
  // with more than 10 arguments.
  static void AddMockMethodDeclaration(
      const std::string& method_name,
      const std::string& return_type,
      const std::vector<std::string>& arguments,
      IndentedText* text);

  // Generates a mock for the signal handler registration method.
  static void AddSignalHandlerRegistrationMock(
      const Interface::Signal& signal,
      IndentedText* text);

  // Generate the signal callback argument of a signal handler.
  static void AddSignalCallbackArg(const Interface::Signal& signal,
                                   bool comment_arg_name,
                                   IndentedText* block);

  // Generates the Object Manager proxy class.
  struct ObjectManager {
    // Generates the top-level class for Object Manager proxy.
    static void GenerateProxy(const ServiceConfig& config,
                              const std::vector<Interface>& interfaces,
                              IndentedText* text);

    // Generates Object Manager constructor.
    static void AddConstructor(const ServiceConfig& config,
                               const std::string& class_name,
                               const std::vector<Interface>& interfaces,
                               IndentedText* text);

    // Generates Object Manager destructor.
    static void AddDestructor(const std::string& class_name,
                              const std::vector<Interface>& interfaces,
                              IndentedText* text);

    // Generates GetObjectManagerProxy() method.
    static void AddGetObjectManagerProxy(IndentedText* text);

    // Generates code for interface-specific accessor methods
    static void AddInterfaceAccessors(const Interface& interface,
                                      IndentedText* text);

    // Generates OnPropertyChanged() method.
    static void AddOnPropertyChanged(const std::vector<Interface>& interfaces,
                                     IndentedText* text);

    // Generates ObjectAdded() method.
    static void AddObjectAdded(const ServiceConfig& config,
                               const std::vector<Interface>& interfaces,
                               IndentedText* text);

    // Generates ObjectRemoved() method.
    static void AddObjectRemoved(const std::vector<Interface>& interfaces,
                                 IndentedText* text);

    // Generates CreateProperties() method.
    static void AddCreateProperties(const std::vector<Interface>& interfaces,
                                    const std::string& class_name,
                                    IndentedText* text);

    // Generates data members of the class.
    static void AddDataMembers(const ServiceConfig& config,
                               const std::vector<Interface>& interfaces,
                               const std::string& class_name,
                               IndentedText* text);
  };
  // Generates the signal handler name for a given signal name.
  static std::string GetHandlerNameForSignal(const std::string& signal);

  DISALLOW_COPY_AND_ASSIGN(ProxyGenerator);
};

}  // namespace chromeos_dbus_bindings

#endif  // CHROMEOS_DBUS_BINDINGS_PROXY_GENERATOR_H_
