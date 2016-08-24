// Copyright 2014 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_DBUS_BINDINGS_INTERFACE_H_
#define CHROMEOS_DBUS_BINDINGS_INTERFACE_H_

#include <string>
#include <vector>

namespace chromeos_dbus_bindings {

struct Interface {
  struct Argument {
    Argument(const std::string& name_in,
             const std::string& type_in) : name(name_in), type(type_in) {}
    std::string name;
    std::string type;
  };
  struct Method {
    enum class Kind {
      kSimple,
      kNormal,
      kAsync,
      kRaw
    };
    Method(const std::string& name_in,
           const std::vector<Argument>& input_arguments_in,
           const std::vector<Argument>& output_arguments_in)
        : name(name_in),
          input_arguments(input_arguments_in),
          output_arguments(output_arguments_in) {}
    Method(const std::string& name_in,
           const std::vector<Argument>& input_arguments_in)
        : name(name_in),
          input_arguments(input_arguments_in) {}
    explicit Method(const std::string& name_in) : name(name_in) {}
    std::string name;
    std::vector<Argument> input_arguments;
    std::vector<Argument> output_arguments;
    std::string doc_string;
    Kind kind{Kind::kNormal};
    bool is_const{false};
    bool include_dbus_message{false};
  };
  struct Signal {
    Signal(const std::string& name_in,
           const std::vector<Argument>& arguments_in)
        : name(name_in), arguments(arguments_in) {}
    explicit Signal(const std::string& name_in) : name(name_in) {}
    std::string name;
    std::vector<Argument> arguments;
    std::string doc_string;
  };
  struct Property {
    Property(const std::string& name_in,
             const std::string& type_in,
             const std::string& access_in)
        : name(name_in), type(type_in), access(access_in) {}
    std::string name;
    std::string type;
    std::string access;
    std::string doc_string;
  };

  Interface() = default;
  Interface(const std::string& name_in,
            const std::vector<Method>& methods_in,
            const std::vector<Signal>& signals_in,
            const std::vector<Property>& properties_in)
      : name(name_in), methods(methods_in), signals(signals_in),
        properties(properties_in) {}
  std::string name;
  std::string path;
  std::vector<Method> methods;
  std::vector<Signal> signals;
  std::vector<Property> properties;
  std::string doc_string;
};

}  // namespace chromeos_dbus_bindings

#endif  // CHROMEOS_DBUS_BINDINGS_INTERFACE_H_
