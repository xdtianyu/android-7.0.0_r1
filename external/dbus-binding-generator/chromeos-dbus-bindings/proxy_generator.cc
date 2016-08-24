// Copyright 2014 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos-dbus-bindings/proxy_generator.h"

#include <utility>

#include <base/files/file_path.h>
#include <base/format_macros.h>
#include <base/logging.h>
#include <base/strings/stringprintf.h>
#include <brillo/strings/string_utils.h>

#include "chromeos-dbus-bindings/dbus_signature.h"
#include "chromeos-dbus-bindings/indented_text.h"
#include "chromeos-dbus-bindings/name_parser.h"

using base::StringPrintf;
using std::pair;
using std::string;
using std::vector;

namespace chromeos_dbus_bindings {

namespace {
// Helper struct to encapsulate information about method call parameter during
// code generation.
struct ParamDef {
  ParamDef(const string& param_type, const string& param_name, bool param_ref)
      : type(param_type), name(param_name), is_const_ref(param_ref) {}

  string type;
  string name;
  bool is_const_ref;
};

string GetParamString(const ParamDef& param_def) {
  return StringPrintf(param_def.is_const_ref ? "const %s& %s" : "%s* %s",
                      param_def.type.c_str(), param_def.name.c_str());
}
}  // anonymous namespace

// static
bool ProxyGenerator::GenerateProxies(
    const ServiceConfig& config,
    const std::vector<Interface>& interfaces,
    const base::FilePath& output_file) {
  IndentedText text;

  text.AddLine("// Automatic generation of D-Bus interfaces:");
  for (const auto& interface : interfaces) {
    text.AddLine(StringPrintf("//  - %s", interface.name.c_str()));
  }
  string header_guard = GenerateHeaderGuard(output_file);
  text.AddLine(StringPrintf("#ifndef %s", header_guard.c_str()));
  text.AddLine(StringPrintf("#define %s", header_guard.c_str()));
  text.AddLine("#include <memory>");
  text.AddLine("#include <string>");
  text.AddLine("#include <vector>");
  text.AddBlankLine();
  text.AddLine("#include <base/bind.h>");
  text.AddLine("#include <base/callback.h>");
  text.AddLine("#include <base/logging.h>");
  text.AddLine("#include <base/macros.h>");
  text.AddLine("#include <base/memory/ref_counted.h>");
  text.AddLine("#include <brillo/any.h>");
  text.AddLine("#include <brillo/dbus/dbus_method_invoker.h>");
  text.AddLine("#include <brillo/dbus/dbus_property.h>");
  text.AddLine("#include <brillo/dbus/dbus_signal_handler.h>");
  text.AddLine("#include <brillo/errors/error.h>");
  text.AddLine("#include <brillo/variant_dictionary.h>");
  text.AddLine("#include <dbus/bus.h>");
  text.AddLine("#include <dbus/message.h>");
  text.AddLine("#include <dbus/object_manager.h>");
  text.AddLine("#include <dbus/object_path.h>");
  text.AddLine("#include <dbus/object_proxy.h>");
  text.AddBlankLine();

  if (!config.object_manager.name.empty()) {
    // Add forward-declaration for Object Manager proxy class.
    NameParser parser{config.object_manager.name};
    parser.AddOpenNamespaces(&text, false);
    text.AddLine(StringPrintf("class %s;",
                              parser.MakeProxyName(false).c_str()));
    parser.AddCloseNamespaces(&text, false);
    text.AddBlankLine();
  }

  for (const auto& interface : interfaces) {
    GenerateInterfaceProxyInterface(config, interface, &text);
    GenerateInterfaceProxy(config, interface, &text);
  }

  ObjectManager::GenerateProxy(config, interfaces, &text);

  text.AddLine(StringPrintf("#endif  // %s", header_guard.c_str()));
  return WriteTextToFile(output_file, text);
}

// static
bool ProxyGenerator::GenerateMocks(const ServiceConfig& config,
                                   const std::vector<Interface>& interfaces,
                                   const base::FilePath& mock_file,
                                   const base::FilePath& proxy_file,
                                   bool use_literal_proxy_file) {
  IndentedText text;

  text.AddLine("// Automatic generation of D-Bus interface mock proxies for:");
  for (const auto& interface : interfaces) {
    text.AddLine(StringPrintf("//  - %s", interface.name.c_str()));
  }
  string header_guard = GenerateHeaderGuard(mock_file);
  text.AddLine(StringPrintf("#ifndef %s", header_guard.c_str()));
  text.AddLine(StringPrintf("#define %s", header_guard.c_str()));
  text.AddLine("#include <string>");
  text.AddLine("#include <vector>");
  text.AddBlankLine();
  text.AddLine("#include <base/callback_forward.h>");
  text.AddLine("#include <base/logging.h>");
  text.AddLine("#include <base/macros.h>");
  text.AddLine("#include <brillo/any.h>");
  text.AddLine("#include <brillo/errors/error.h>");
  text.AddLine("#include <brillo/variant_dictionary.h>");
  text.AddLine("#include <gmock/gmock.h>");
  text.AddBlankLine();

  if (!proxy_file.empty()) {
    // If we have a proxy header file, it would have the proxy interfaces we
    // need to base our mocks on, so we need to include that header file.
    base::FilePath relative_path;
    if (use_literal_proxy_file) {
      relative_path = proxy_file;
    } else {
      // Generate a relative path from |mock_file| to |proxy_file|.

      // First, get the path components for both source and destination paths.
      std::vector<base::FilePath::StringType> src_components;
      mock_file.DirName().GetComponents(&src_components);
      std::vector<base::FilePath::StringType> dest_components;
      proxy_file.DirName().GetComponents(&dest_components);

      // Find the common root.

      // I wish we had C++14 and its 4-parameter version of std::mismatch()...
      auto src_end = src_components.end();
      if (src_components.size() > dest_components.size())
        src_end = src_components.begin() + dest_components.size();

      auto mismatch_pair = std::mismatch(src_components.begin(), src_end,
                                         dest_components.begin());

      // For each remaining components in the |src_components|, generate the
      // parent directory references ("..").
      size_t src_count = std::distance(mismatch_pair.first,
                                       src_components.end());
      std::vector<base::FilePath::StringType> components{
          src_count, base::FilePath::kParentDirectory};
      // Append the remaining components from |dest_components|.
      components.insert(components.end(),
                        mismatch_pair.second, dest_components.end());
      // Finally, add the base name of the target file name.
      components.push_back(proxy_file.BaseName().value());
      // Now reconstruct the relative path.
      relative_path = base::FilePath{base::FilePath::kCurrentDirectory};
      for (const auto& component : components)
        relative_path = relative_path.Append(component);
    }
    text.AddLine(StringPrintf("#include \"%s\"",
                              relative_path.value().c_str()));
    text.AddBlankLine();
  }

  for (const auto& interface : interfaces) {
    // If we have no proxy file, we need the abstract interfaces generated here.
    if (proxy_file.empty())
      GenerateInterfaceProxyInterface(config, interface, &text);
    GenerateInterfaceMock(config, interface, &text);
  }

  text.AddLine(StringPrintf("#endif  // %s", header_guard.c_str()));
  return WriteTextToFile(mock_file, text);
}

// static
void ProxyGenerator::GenerateInterfaceProxyInterface(
    const ServiceConfig& config,
    const Interface& interface,
    IndentedText* text) {
  NameParser parser{interface.name};
  string proxy_name = parser.MakeProxyName(false);
  string base_interface_name = proxy_name + "Interface";

  parser.AddOpenNamespaces(text, false);
  text->AddBlankLine();

  text->AddLine(StringPrintf("// Abstract interface proxy for %s.",
                             parser.MakeFullCppName().c_str()));
  text->AddComments(interface.doc_string);
  text->AddLine(StringPrintf("class %s {", base_interface_name.c_str()));
  text->AddLineWithOffset("public:", kScopeOffset);
  text->PushOffset(kBlockOffset);
  text->AddLine(
      StringPrintf("virtual ~%s() = default;", base_interface_name.c_str()));

  for (const auto& method : interface.methods) {
    AddMethodProxy(method, interface.name, true, text);
    AddAsyncMethodProxy(method, interface.name, true, text);
  }
  for (const auto& signal : interface.signals) {
    AddSignalHandlerRegistration(signal, interface.name, true, text);
  }
  AddProperties(config, interface, true, text);
  text->AddBlankLine();
  text->AddLine("virtual const dbus::ObjectPath& GetObjectPath() const = 0;");
  if (!config.object_manager.name.empty() && !interface.properties.empty())
    AddPropertyPublicMethods(proxy_name, true, text);

  text->PopOffset();
  text->AddLine("};");
  text->AddBlankLine();

  parser.AddCloseNamespaces(text, false);
  text->AddBlankLine();
}

// static
void ProxyGenerator::GenerateInterfaceProxy(const ServiceConfig& config,
                                            const Interface& interface,
                                            IndentedText* text) {
  NameParser parser{interface.name};
  string proxy_name = parser.MakeProxyName(false);
  string base_interface_name = proxy_name + "Interface";

  parser.AddOpenNamespaces(text, false);
  text->AddBlankLine();

  text->AddLine(StringPrintf("// Interface proxy for %s.",
                             parser.MakeFullCppName().c_str()));
  text->AddComments(interface.doc_string);
  text->AddLine(StringPrintf("class %s final : public %s {",
                             proxy_name.c_str(), base_interface_name.c_str()));
  text->AddLineWithOffset("public:", kScopeOffset);
  text->PushOffset(kBlockOffset);
  AddPropertySet(config, interface, text);
  AddConstructor(config, interface, proxy_name, text);
  AddDestructor(proxy_name, text);
  for (const auto& signal : interface.signals) {
    AddSignalHandlerRegistration(signal, interface.name, false, text);
  }
  AddReleaseObjectProxy(text);
  AddGetObjectPath(text);
  AddGetObjectProxy(text);
  if (!config.object_manager.name.empty() && !interface.properties.empty())
    AddPropertyPublicMethods(proxy_name, false, text);
  for (const auto& method : interface.methods) {
    AddMethodProxy(method, interface.name, false, text);
    AddAsyncMethodProxy(method, interface.name, false, text);
  }
  AddProperties(config, interface, false, text);

  text->PopOffset();
  text->AddBlankLine();
  text->AddLineWithOffset("private:", kScopeOffset);

  text->PushOffset(kBlockOffset);
  if (!config.object_manager.name.empty() && !interface.properties.empty())
    AddOnPropertyChanged(text);
  text->AddLine("scoped_refptr<dbus::Bus> bus_;");
  if (config.service_name.empty()) {
    text->AddLine("std::string service_name_;");
  } else {
    text->AddLine(StringPrintf("const std::string service_name_{\"%s\"};",
                               config.service_name.c_str()));
  }
  if (interface.path.empty()) {
    text->AddLine("dbus::ObjectPath object_path_;");
  } else {
    text->AddLine(StringPrintf("const dbus::ObjectPath object_path_{\"%s\"};",
                               interface.path.c_str()));
  }
  if (!config.object_manager.name.empty() && !interface.properties.empty()) {
    text->AddLine("PropertySet* property_set_;");
    text->AddLine(
        StringPrintf("base::Callback<void(%sInterface*, const std::string&)> "
                     "on_property_changed_;",
                     proxy_name.c_str()));
  }
  text->AddLine("dbus::ObjectProxy* dbus_object_proxy_;");
  text->AddBlankLine();

  if (!config.object_manager.name.empty() && !interface.properties.empty()) {
    text->AddLine(StringPrintf(
        "friend class %s;",
        NameParser{config.object_manager.name}.MakeProxyName(true).c_str()));
  }
  text->AddLine(StringPrintf("DISALLOW_COPY_AND_ASSIGN(%s);",
                             proxy_name.c_str()));
  text->PopOffset();
  text->AddLine("};");

  text->AddBlankLine();

  parser.AddCloseNamespaces(text, false);

  text->AddBlankLine();
}

// static
void ProxyGenerator::GenerateInterfaceMock(const ServiceConfig& config,
                                           const Interface& interface,
                                           IndentedText* text) {
  NameParser parser{interface.name};
  string proxy_name = parser.MakeProxyName(false);
  string base_interface_name = proxy_name + "Interface";
  string mock_name = proxy_name + "Mock";

  parser.AddOpenNamespaces(text, false);
  text->AddBlankLine();

  text->AddLine(StringPrintf("// Mock object for %s.",
                             base_interface_name.c_str()));
  text->AddLine(StringPrintf("class %s : public %s {",
                             mock_name.c_str(), base_interface_name.c_str()));
  text->AddLineWithOffset("public:", kScopeOffset);
  text->PushOffset(kBlockOffset);
  text->AddLine(StringPrintf("%s() = default;", mock_name.c_str()));
  text->AddBlankLine();

  for (const auto& method : interface.methods) {
    AddMethodMock(method, interface.name, text);
    AddAsyncMethodMock(method, interface.name, text);
  }
  for (const auto& signal : interface.signals) {
    AddSignalHandlerRegistrationMock(signal, text);
  }

  DbusSignature signature;
  for (const auto& prop : interface.properties) {
    string type;
    CHECK(signature.Parse(prop.type, &type));
    MakeConstReferenceIfNeeded(&type);
    string name = NameParser{prop.name}.MakeVariableName();
    text->AddLine(StringPrintf("MOCK_CONST_METHOD0(%s, %s());",
                               name.c_str(), type.c_str()));
    if (prop.access == "readwrite") {
      text->AddLine(StringPrintf("MOCK_METHOD2(set_%s, void(%s, "
                                 "const base::Callback<bool>&));",
                                 name.c_str(), type.c_str()));
    }
  }
  text->AddLine(
      "MOCK_CONST_METHOD0(GetObjectPath, const dbus::ObjectPath&());");
  if (!config.object_manager.name.empty() && !interface.properties.empty()) {
    text->AddLineAndPushOffsetTo(
        "MOCK_METHOD1(SetPropertyChangedCallback,", 1, '(');
    text->AddLine(StringPrintf(
        "void(const base::Callback<void(%sInterface*, const std::string&)>&));",
        proxy_name.c_str()));
    text->PopOffset();
  }

  text->PopOffset();
  text->AddBlankLine();
  text->AddLineWithOffset("private:", kScopeOffset);
  text->AddLineWithOffset(StringPrintf("DISALLOW_COPY_AND_ASSIGN(%s);",
                                       mock_name.c_str()),
                          kBlockOffset);
  text->AddLine("};");

  parser.AddCloseNamespaces(text, false);
  text->AddBlankLine();
}

// static
void ProxyGenerator::AddConstructor(const ServiceConfig& config,
                                    const Interface& interface,
                                    const string& class_name,
                                    IndentedText* text) {
  IndentedText block;
  vector<ParamDef> args{{"scoped_refptr<dbus::Bus>", "bus", true}};
  if (config.service_name.empty())
    args.emplace_back("std::string", "service_name", true);
  if (interface.path.empty())
    args.emplace_back("dbus::ObjectPath", "object_path", true);
  if (!config.object_manager.name.empty() && !interface.properties.empty())
    args.emplace_back("PropertySet", "property_set", false);

  if (args.size() == 1) {
    block.AddLine(StringPrintf("%s(%s) :", class_name.c_str(),
                               GetParamString(args.front()).c_str()));
  } else {
    block.AddLine(StringPrintf("%s(", class_name.c_str()));
    block.PushOffset(kLineContinuationOffset);
    for (size_t i = 0; i < args.size() - 1; i++) {
      block.AddLine(StringPrintf("%s,", GetParamString(args[i]).c_str()));
    }
    block.AddLine(StringPrintf("%s) :", GetParamString(args.back()).c_str()));
  }
  block.PushOffset(kLineContinuationOffset);
  for (const auto& arg : args) {
    block.AddLine(StringPrintf("%s_{%s},", arg.name.c_str(),
                               arg.name.c_str()));
  }
  block.AddLine("dbus_object_proxy_{");
  block.AddLineWithOffset(
      "bus_->GetObjectProxy(service_name_, object_path_)} {",
      kLineContinuationOffset);
  block.PopOffset();
  if (args.size() > 1)
    block.PopOffset();
  block.AddLine("}");
  block.AddBlankLine();
  text->AddBlock(block);
}

// static
void ProxyGenerator::AddDestructor(const string& class_name,
                                   IndentedText* text) {
  IndentedText block;
  block.AddLine(StringPrintf("~%s() override {", class_name.c_str()));
  block.AddLine("}");
  text->AddBlock(block);
}

// static
void ProxyGenerator::AddReleaseObjectProxy(IndentedText* text) {
  text->AddBlankLine();
  text->AddLine("void ReleaseObjectProxy(const base::Closure& callback) {");
  text->AddLineWithOffset(
      "bus_->RemoveObjectProxy(service_name_, object_path_, callback);",
      kBlockOffset);
  text->AddLine("}");
}

// static
void ProxyGenerator::AddGetObjectPath(IndentedText* text) {
  text->AddBlankLine();
  text->AddLine("const dbus::ObjectPath& GetObjectPath() const override {");
  text->AddLineWithOffset("return object_path_;", kBlockOffset);
  text->AddLine("}");
}

// static
void ProxyGenerator::AddGetObjectProxy(IndentedText* text) {
  text->AddBlankLine();
  text->AddLine("dbus::ObjectProxy* GetObjectProxy() const { "
                "return dbus_object_proxy_; }");
}

// static
void ProxyGenerator::AddPropertyPublicMethods(const string& class_name,
                                              bool declaration_only,
                                              IndentedText* text) {
  text->AddBlankLine();
  text->AddLine(StringPrintf("%svoid SetPropertyChangedCallback(",
                             declaration_only ? "virtual " : ""));
  text->AddLineWithOffset(
      StringPrintf("const base::Callback<void(%sInterface*, "
                   "const std::string&)>& callback) %s",
                   class_name.c_str(),
                   declaration_only ? "= 0;" : "override {"),
      kLineContinuationOffset);
  if (!declaration_only) {
    text->AddLineWithOffset("on_property_changed_ = callback;", kBlockOffset);
    text->AddLine("}");
    text->AddBlankLine();

    text->AddLine(
        "const PropertySet* GetProperties() const { return property_set_; }");
    text->AddLine("PropertySet* GetProperties() { return property_set_; }");
  }
}

// static
void ProxyGenerator::AddOnPropertyChanged(IndentedText* text) {
  text->AddLine("void OnPropertyChanged(const std::string& property_name) {");
  text->PushOffset(kBlockOffset);
  text->AddLine("if (!on_property_changed_.is_null())");
  text->PushOffset(kBlockOffset);
  text->AddLine("on_property_changed_.Run(this, property_name);");
  text->PopOffset();
  text->PopOffset();
  text->AddLine("}");
  text->AddBlankLine();
}

void ProxyGenerator::AddSignalHandlerRegistration(
      const Interface::Signal& signal,
      const string& interface_name,
      bool declaration_only,
      IndentedText* text) {
  IndentedText block;
  block.AddBlankLine();
  block.AddLine(StringPrintf("%svoid Register%sSignalHandler(",
                             declaration_only ? "virtual " : "",
                             signal.name.c_str()));
  block.PushOffset(kLineContinuationOffset);
  AddSignalCallbackArg(signal, false, &block);
  block.AddLine(StringPrintf(
      "dbus::ObjectProxy::OnConnectedCallback on_connected_callback)%s",
      declaration_only ? " = 0;" : " override {"));
  if (!declaration_only) {
    block.PopOffset();  // Method signature arguments
    block.PushOffset(kBlockOffset);
    block.AddLine("brillo::dbus_utils::ConnectToSignal(");
    block.PushOffset(kLineContinuationOffset);
    block.AddLine("dbus_object_proxy_,");
    block.AddLine(StringPrintf("\"%s\",", interface_name.c_str()));
    block.AddLine(StringPrintf("\"%s\",", signal.name.c_str()));
    block.AddLine("signal_callback,");
    block.AddLine("on_connected_callback);");
    block.PopOffset();  // Function call line continuation
    block.PopOffset();  // Method body
    block.AddLine("}");
  }
  text->AddBlock(block);
}

// static
void ProxyGenerator::AddPropertySet(const ServiceConfig& config,
                                    const Interface& interface,
                                    IndentedText* text) {
  // Must have ObjectManager in order for property system to work correctly.
  if (config.object_manager.name.empty())
    return;

  IndentedText block;
  block.AddLine("class PropertySet : public dbus::PropertySet {");
  block.AddLineWithOffset("public:", kScopeOffset);
  block.PushOffset(kBlockOffset);
  block.AddLineAndPushOffsetTo("PropertySet(dbus::ObjectProxy* object_proxy,",
                               1, '(');
  block.AddLine("const PropertyChangedCallback& callback)");
  block.PopOffset();
  block.PushOffset(kLineContinuationOffset);
  block.AddLineAndPushOffsetTo(": dbus::PropertySet{object_proxy,", 1, '{');
  block.AddLine(StringPrintf("\"%s\",", interface.name.c_str()));
  block.AddLine("callback} {");
  block.PopOffset();
  block.PopOffset();
  block.PushOffset(kBlockOffset);
  for (const auto& prop : interface.properties) {
    block.AddLine(
        StringPrintf("RegisterProperty(%sName(), &%s);",
                     prop.name.c_str(),
                     NameParser{prop.name}.MakeVariableName().c_str()));
  }
  block.PopOffset();
  block.AddLine("}");
  block.AddBlankLine();

  DbusSignature signature;
  for (const auto& prop : interface.properties) {
    string type;
    CHECK(signature.Parse(prop.type, &type));
    block.AddLine(
        StringPrintf("brillo::dbus_utils::Property<%s> %s;",
                     type.c_str(),
                     NameParser{prop.name}.MakeVariableName().c_str()));
  }
  block.AddBlankLine();

  block.PopOffset();
  block.AddLineWithOffset("private:", kScopeOffset);
  block.AddLineWithOffset("DISALLOW_COPY_AND_ASSIGN(PropertySet);",
                          kBlockOffset);
  block.AddLine("};");
  block.AddBlankLine();

  text->AddBlock(block);
}

// static
void ProxyGenerator::AddProperties(const ServiceConfig& config,
                                   const Interface& interface,
                                   bool declaration_only,
                                   IndentedText* text) {
  // Must have ObjectManager in order for property system to work correctly.
  if (config.object_manager.name.empty())
    return;

  if (declaration_only && !interface.properties.empty())
    text->AddBlankLine();

  DbusSignature signature;
  for (const auto& prop : interface.properties) {
    if (declaration_only) {
      text->AddLine(
          StringPrintf("static const char* %sName() { return \"%s\"; }",
                       prop.name.c_str(),
                       prop.name.c_str()));
    }
    string type;
    CHECK(signature.Parse(prop.type, &type));
    MakeConstReferenceIfNeeded(&type);
    string name = NameParser{prop.name}.MakeVariableName();
    if (!declaration_only)
      text->AddBlankLine();
    text->AddLine(
        StringPrintf("%s%s %s() const%s",
                     declaration_only ? "virtual " : "",
                     type.c_str(),
                     name.c_str(),
                     declaration_only ? " = 0;" : " override {"));
    if (!declaration_only) {
      text->AddLineWithOffset(
          StringPrintf("return property_set_->%s.value();", name.c_str()),
          kBlockOffset);
      text->AddLine("}");
    }
    if (prop.access == "readwrite") {
      if (!declaration_only)
        text->AddBlankLine();
      text->AddLineAndPushOffsetTo(
          StringPrintf("%svoid set_%s(%s value,",
                       declaration_only ? "virtual " : "",
                       name.c_str(),
                       type.c_str()),
          1, '(');
      text->AddLine(
          StringPrintf("const base::Callback<void(bool)>& callback)%s",
                       declaration_only ? " = 0;" : " override {"));
      text->PopOffset();
      if (!declaration_only) {
        text->AddLineWithOffset(
            StringPrintf("property_set_->%s.Set(value, callback);", name.c_str()),
            kBlockOffset);
        text->AddLine("}");
      }
    }
  }
}

// static
void ProxyGenerator::AddMethodProxy(const Interface::Method& method,
                                    const string& interface_name,
                                    bool declaration_only,
                                    IndentedText* text) {
  IndentedText block;
  DbusSignature signature;
  block.AddBlankLine();
  block.AddComments(method.doc_string);
  block.AddLine(StringPrintf("%sbool %s(",
                             declaration_only ? "virtual " : "",
                             method.name.c_str()));
  block.PushOffset(kLineContinuationOffset);
  vector<string> argument_names;
  int argument_number = 0;
  for (const auto& argument : method.input_arguments) {
    string argument_type;
    CHECK(signature.Parse(argument.type, &argument_type));
    MakeConstReferenceIfNeeded(&argument_type);
    string argument_name = GetArgName("in", argument.name, ++argument_number);
    argument_names.push_back(argument_name);
    block.AddLine(StringPrintf(
        "%s %s,", argument_type.c_str(), argument_name.c_str()));
  }
  vector<string> out_param_names{"response.get()", "error"};
  for (const auto& argument : method.output_arguments) {
    string argument_type;
    CHECK(signature.Parse(argument.type, &argument_type));
    string argument_name = GetArgName("out", argument.name, ++argument_number);
    out_param_names.push_back(argument_name);
    block.AddLine(StringPrintf(
        "%s* %s,", argument_type.c_str(), argument_name.c_str()));
  }
  block.AddLine("brillo::ErrorPtr* error,");
  block.AddLine(
      StringPrintf("int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT)%s",
                   declaration_only ? " = 0;" : " override {"));
  block.PopOffset();
  if (!declaration_only) {
    block.PushOffset(kBlockOffset);

    block.AddLine(
        "auto response = brillo::dbus_utils::CallMethodAndBlockWithTimeout(");
    block.PushOffset(kLineContinuationOffset);
    block.AddLine("timeout_ms,");
    block.AddLine("dbus_object_proxy_,");
    block.AddLine(StringPrintf("\"%s\",", interface_name.c_str()));
    block.AddLine(StringPrintf("\"%s\",", method.name.c_str()));
    string last_argument = "error";
    for (const auto& argument_name : argument_names) {
      block.AddLine(StringPrintf("%s,", last_argument.c_str()));
      last_argument = argument_name;
    }
    block.AddLine(StringPrintf("%s);", last_argument.c_str()));
    block.PopOffset();

    block.AddLine("return response && "
                  "brillo::dbus_utils::ExtractMethodCallResults(");
    block.PushOffset(kLineContinuationOffset);
    block.AddLine(brillo::string_utils::Join(", ", out_param_names) + ");");
    block.PopOffset();
    block.PopOffset();
    block.AddLine("}");
  }
  text->AddBlock(block);
}

// static
void ProxyGenerator::AddAsyncMethodProxy(const Interface::Method& method,
                                         const string& interface_name,
                                         bool declaration_only,
                                         IndentedText* text) {
  IndentedText block;
  DbusSignature signature;
  block.AddBlankLine();
  block.AddComments(method.doc_string);
  block.AddLine(StringPrintf("%svoid %sAsync(",
                             declaration_only ? "virtual " : "",
                             method.name.c_str()));
  block.PushOffset(kLineContinuationOffset);
  vector<string> argument_names;
  int argument_number = 0;
  for (const auto& argument : method.input_arguments) {
    string argument_type;
    CHECK(signature.Parse(argument.type, &argument_type));
    MakeConstReferenceIfNeeded(&argument_type);
    string argument_name = GetArgName("in", argument.name, ++argument_number);
    argument_names.push_back(argument_name);
    block.AddLine(StringPrintf(
        "%s %s,", argument_type.c_str(), argument_name.c_str()));
  }
  vector<string> out_params;
  for (const auto& argument : method.output_arguments) {
    string argument_type;
    CHECK(signature.Parse(argument.type, &argument_type));
    MakeConstReferenceIfNeeded(&argument_type);
    if (!argument.name.empty())
      base::StringAppendF(&argument_type, " /*%s*/", argument.name.c_str());
    out_params.push_back(argument_type);
  }
  block.AddLine(StringPrintf(
      "const base::Callback<void(%s)>& success_callback,",
      brillo::string_utils::Join(", ", out_params).c_str()));
  block.AddLine(
      "const base::Callback<void(brillo::Error*)>& error_callback,");
  block.AddLine(
      StringPrintf("int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT)%s",
                   declaration_only ? " = 0;" : " override {"));
  block.PopOffset();
  if (!declaration_only) {
    block.PushOffset(kBlockOffset);

    block.AddLine("brillo::dbus_utils::CallMethodWithTimeout(");
    block.PushOffset(kLineContinuationOffset);
    block.AddLine("timeout_ms,");
    block.AddLine("dbus_object_proxy_,");
    block.AddLine(StringPrintf("\"%s\",", interface_name.c_str()));
    block.AddLine(StringPrintf("\"%s\",", method.name.c_str()));
    block.AddLine("success_callback,");
    string last_argument = "error_callback";
    for (const auto& argument_name : argument_names) {
      block.AddLine(StringPrintf("%s,", last_argument.c_str()));
      last_argument = argument_name;
    }
    block.AddLine(StringPrintf("%s);", last_argument.c_str()));
    block.PopOffset();

    block.PopOffset();
    block.AddLine("}");
  }
  text->AddBlock(block);
}

// static
void ProxyGenerator::AddMethodMock(const Interface::Method& method,
                                   const string& /* interface_name */,
                                   IndentedText* text) {
  DbusSignature signature;
  vector<string> arguments;
  for (const auto& argument : method.input_arguments) {
    string argument_type;
    CHECK(signature.Parse(argument.type, &argument_type));
    MakeConstReferenceIfNeeded(&argument_type);
    if (!argument.name.empty())
      base::StringAppendF(&argument_type, " /*in_%s*/", argument.name.c_str());
    arguments.push_back(argument_type);
  }
  for (const auto& argument : method.output_arguments) {
    string argument_type;
    CHECK(signature.Parse(argument.type, &argument_type));
    argument_type += '*';
    if (!argument.name.empty())
      base::StringAppendF(&argument_type, " /*out_%s*/", argument.name.c_str());
    arguments.push_back(argument_type);
  }
  arguments.push_back("brillo::ErrorPtr* /*error*/");
  arguments.push_back("int /*timeout_ms*/");
  AddMockMethodDeclaration(method.name, "bool", arguments, text);
}

// static
void ProxyGenerator::AddAsyncMethodMock(const Interface::Method& method,
                                        const string& /* interface_name */,
                                        IndentedText* text) {
  DbusSignature signature;
  vector<string> arguments;
  for (const auto& argument : method.input_arguments) {
    string argument_type;
    CHECK(signature.Parse(argument.type, &argument_type));
    MakeConstReferenceIfNeeded(&argument_type);
    if (!argument.name.empty())
      base::StringAppendF(&argument_type, " /*in_%s*/", argument.name.c_str());
    arguments.push_back(argument_type);
  }
  vector<string> out_params;
  for (const auto& argument : method.output_arguments) {
    string argument_type;
    CHECK(signature.Parse(argument.type, &argument_type));
    MakeConstReferenceIfNeeded(&argument_type);
    if (!argument.name.empty())
      base::StringAppendF(&argument_type, " /*%s*/", argument.name.c_str());
    out_params.push_back(argument_type);
  }
  arguments.push_back(StringPrintf(
      "const base::Callback<void(%s)>& /*success_callback*/",
      brillo::string_utils::Join(", ", out_params).c_str()));
  arguments.push_back(
      "const base::Callback<void(brillo::Error*)>& /*error_callback*/");
  arguments.push_back("int /*timeout_ms*/");
  AddMockMethodDeclaration(method.name + "Async", "void", arguments, text);
}

void ProxyGenerator::AddMockMethodDeclaration(const string& method_name,
                                              const string& return_type,
                                              const vector<string>& arguments,
                                              IndentedText* text) {
  IndentedText block;
  // GMOCK doesn't go all the way up to 11, so we need to handle methods with
  // 11 arguments or more in a different way.
  if (arguments.size() >= 11) {
    block.AddLineAndPushOffsetTo(
        StringPrintf("%s %s(%s,",
                     return_type.c_str(),
                     method_name.c_str(),
                     arguments.front().c_str()),
        1, '(');
    for (size_t i = 1; i < arguments.size() - 1; i++)
      block.AddLine(StringPrintf("%s,", arguments[i].c_str()));
    block.AddLine(StringPrintf("%s) override {", arguments.back().c_str()));
    block.PopOffset();
    block.PushOffset(kBlockOffset);
    block.AddLine(StringPrintf(
        "LOG(WARNING) << \"%s(): gmock can't handle methods with %" PRIuS
        " arguments. You can override this method in a subclass if you need"
        " to.\";",
        method_name.c_str(), arguments.size()));
    if (return_type == "void") {
      // No return added here.
    } else if (return_type == "bool") {
      block.AddLine("return false;");
    } else {
      LOG(FATAL) << "The return type is not supported.";
    }
    block.PopOffset();
    block.AddLine("}");
  } else {
    block.AddLineAndPushOffsetTo(
        StringPrintf("MOCK_METHOD%zu(%s,",
                     arguments.size(), method_name.c_str()),
        1, '(');
    block.AddLineAndPushOffsetTo(
        StringPrintf("%s(%s,", return_type.c_str(), arguments.front().c_str()),
        1, '(');
    for (size_t i = 1; i < arguments.size() - 1; i++)
      block.AddLine(StringPrintf("%s,", arguments[i].c_str()));
    block.AddLine(StringPrintf("%s));", arguments.back().c_str()));
    block.PopOffset();
    block.PopOffset();
  }
  text->AddBlock(block);
}

// static
void ProxyGenerator::AddSignalHandlerRegistrationMock(
    const Interface::Signal& signal,
    IndentedText* text) {
  IndentedText callback_arg_text;
  AddSignalCallbackArg(signal, true, &callback_arg_text);
  vector<string> arg_lines = callback_arg_text.GetLines();

  IndentedText block;
  block.AddLineAndPushOffsetTo(
      StringPrintf("MOCK_METHOD2(Register%sSignalHandler,",
                   signal.name.c_str()),
      1, '(');
  for (size_t i = 0; i < arg_lines.size(); ++i) {
    if (i == 0)
      block.AddLineAndPushOffsetTo("void(" + arg_lines[i], 1, '(');
    else
      block.AddLine(arg_lines[i]);
  }
  block.AddLine(
      "dbus::ObjectProxy::OnConnectedCallback /*on_connected_callback*/));");
  text->AddBlock(block);
}

// static
void ProxyGenerator::AddSignalCallbackArg(const Interface::Signal& signal,
                                          bool comment_arg_name,
                                          IndentedText* block) {
  DbusSignature signature;
  string signal_callback = StringPrintf("%ssignal_callback%s",
                                        comment_arg_name ? "/*" : "",
                                        comment_arg_name ? "*/" : "");
  if (signal.arguments.empty()) {
    block->AddLine(StringPrintf("const base::Closure& %s,",
                                signal_callback.c_str()));
  } else {
    string last_argument;
    string prefix{"const base::Callback<void("};
    for (const auto argument : signal.arguments) {
      if (!last_argument.empty()) {
        if (!prefix.empty()) {
          block->AddLineAndPushOffsetTo(
              StringPrintf("%s%s,", prefix.c_str(), last_argument.c_str()),
              1, '(');
          prefix.clear();
        } else {
          block->AddLine(StringPrintf("%s,", last_argument.c_str()));
        }
      }
      CHECK(signature.Parse(argument.type, &last_argument));
      MakeConstReferenceIfNeeded(&last_argument);
    }
    block->AddLine(StringPrintf("%s%s)>& %s,",
                                prefix.c_str(),
                                last_argument.c_str(),
                                signal_callback.c_str()));
    if (prefix.empty()) {
      block->PopOffset();
    }
  }
}

// static
void ProxyGenerator::ObjectManager::GenerateProxy(
    const ServiceConfig& config,
    const std::vector<Interface>& interfaces,
    IndentedText* text) {
  if (config.object_manager.name.empty())
    return;

  NameParser object_manager{config.object_manager.name};
  object_manager.AddOpenNamespaces(text, false);
  text->AddBlankLine();

  string class_name = object_manager.type_name + "Proxy";
  text->AddLine(StringPrintf("class %s : "
                             "public dbus::ObjectManager::Interface {",
                             class_name.c_str()));
  text->AddLineWithOffset("public:", kScopeOffset);
  text->PushOffset(kBlockOffset);

  AddConstructor(config, class_name, interfaces, text);
  AddDestructor(class_name, interfaces, text);
  AddGetObjectManagerProxy(text);
  for (const auto& itf : interfaces) {
    AddInterfaceAccessors(itf, text);
  }
  text->PopOffset();

  text->AddLineWithOffset("private:", kScopeOffset);
  text->PushOffset(kBlockOffset);
  AddOnPropertyChanged(interfaces, text);
  AddObjectAdded(config, interfaces, text);
  AddObjectRemoved(interfaces, text);
  AddCreateProperties(interfaces, class_name, text);
  AddDataMembers(config, interfaces, class_name, text);

  text->AddLine(StringPrintf("DISALLOW_COPY_AND_ASSIGN(%s);",
                              class_name.c_str()));
  text->PopOffset();
  text->AddLine("};");
  text->AddBlankLine();
  object_manager.AddCloseNamespaces(text, false);
  text->AddBlankLine();
}

void ProxyGenerator::ObjectManager::AddConstructor(
    const ServiceConfig& config,
    const std::string& class_name,
    const std::vector<Interface>& interfaces,
    IndentedText* text) {
  if (config.service_name.empty()) {
    text->AddLineAndPushOffsetTo(
        StringPrintf("%s(const scoped_refptr<dbus::Bus>& bus,",
                     class_name.c_str()),
        1, '(');
    text->AddLine("const std::string& service_name)");
    text->PopOffset();
  } else {
    text->AddLine(StringPrintf("%s(const scoped_refptr<dbus::Bus>& bus)",
                               class_name.c_str()));
  }
  text->PushOffset(kLineContinuationOffset);
  text->AddLine(": bus_{bus},");
  text->PushOffset(kBlockOffset);
  if (config.service_name.empty()) {
    text->AddLine("service_name_{service_name},");
  }
  text->AddLine("dbus_object_manager_{bus->GetObjectManager(");
  text->PushOffset(kLineContinuationOffset);
  if (config.service_name.empty()) {
    text->AddLine("service_name,");
  } else {
    text->AddLine(StringPrintf("\"%s\",", config.service_name.c_str()));
  }
  text->AddLine(StringPrintf("dbus::ObjectPath{\"%s\"})} {",
                             config.object_manager.object_path.c_str()));
  text->PopOffset();
  text->PopOffset();
  text->PopOffset();
  text->PushOffset(kBlockOffset);
  for (const auto& itf : interfaces) {
    text->AddLine(
        StringPrintf("dbus_object_manager_->RegisterInterface(\"%s\", this);",
                     itf.name.c_str()));
  }
  text->PopOffset();
  text->AddLine("}");
  text->AddBlankLine();
}

void ProxyGenerator::ObjectManager::AddDestructor(
    const std::string& class_name,
    const std::vector<Interface>& interfaces,
    IndentedText* text) {
  text->AddLine(StringPrintf("~%s() override {", class_name.c_str()));
  text->PushOffset(kBlockOffset);
  for (const auto& itf : interfaces) {
    text->AddLine(
        StringPrintf("dbus_object_manager_->UnregisterInterface(\"%s\");",
                     itf.name.c_str()));
  }
  text->PopOffset();
  text->AddLine("}");
  text->AddBlankLine();
}

void ProxyGenerator::ObjectManager::AddGetObjectManagerProxy(
    IndentedText* text) {
  text->AddLine("dbus::ObjectManager* GetObjectManagerProxy() const {");
  text->AddLineWithOffset("return dbus_object_manager_;", kBlockOffset);
  text->AddLine("}");
  text->AddBlankLine();
}

void ProxyGenerator::ObjectManager::AddInterfaceAccessors(
    const Interface& interface,
    IndentedText* text) {
  NameParser itf_name{interface.name};
  string map_name = itf_name.MakeVariableName() + "_instances_";

  // GetProxy().
  if (interface.path.empty()) {
    // We have no fixed path, so there could be multiple instances of this itf.
    text->AddLine(StringPrintf("%sInterface* Get%s(",
                                itf_name.MakeProxyName(true).c_str(),
                                itf_name.MakeProxyName(false).c_str()));
    text->PushOffset(kLineContinuationOffset);
    text->AddLine("const dbus::ObjectPath& object_path) {");
    text->PopOffset();
    text->PushOffset(kBlockOffset);
    text->AddLine(StringPrintf("auto p = %s.find(object_path);",
                                map_name.c_str()));
    text->AddLine(StringPrintf("if (p != %s.end())", map_name.c_str()));
    text->PushOffset(kBlockOffset);
    text->AddLine("return p->second.get();");
    text->PopOffset();
    text->AddLine("return nullptr;");
    text->PopOffset();
    text->AddLine("}");
  } else {
    // We have a fixed path, so the object could be considered a "singleton".
    // Skip the object_path parameter and return the first available instance.
    text->AddLine(StringPrintf("%sInterface* Get%s() {",
                                itf_name.MakeProxyName(true).c_str(),
                                itf_name.MakeProxyName(false).c_str()));
    text->PushOffset(kBlockOffset);
    text->AddLine(StringPrintf("if (%s.empty())", map_name.c_str()));
    text->AddLineWithOffset("return nullptr;", kBlockOffset);
    text->AddLine(StringPrintf("return %s.begin()->second.get();",
                               map_name.c_str()));
    text->PopOffset();
    text->AddLine("}");
  }

  // GetInstances().
  text->AddLine(
      StringPrintf("std::vector<%sInterface*> Get%sInstances() const {",
                   itf_name.MakeProxyName(true).c_str(),
                   itf_name.type_name.c_str()));
  text->PushOffset(kBlockOffset);
  text->AddLine(StringPrintf("std::vector<%sInterface*> values;",
                             itf_name.MakeProxyName(true).c_str()));
  text->AddLine(StringPrintf("values.reserve(%s.size());", map_name.c_str()));
  text->AddLine(StringPrintf("for (const auto& pair : %s)", map_name.c_str()));
  text->AddLineWithOffset("values.push_back(pair.second.get());", kBlockOffset);
  text->AddLine("return values;");
  text->PopOffset();
  text->AddLine("}");

  // SetAddedCallback().
  text->AddLine(StringPrintf("void Set%sAddedCallback(",
                              itf_name.type_name.c_str()));
  text->PushOffset(kLineContinuationOffset);
  text->AddLine(
      StringPrintf("const base::Callback<void(%sInterface*)>& callback) {",
                   itf_name.MakeProxyName(true).c_str()));
  text->PopOffset();
  text->PushOffset(kBlockOffset);
  text->AddLine(StringPrintf("on_%s_added_ = callback;",
                             itf_name.MakeVariableName().c_str()));
  text->PopOffset();
  text->AddLine("}");

  // SetRemovedCallback().
  text->AddLine(StringPrintf("void Set%sRemovedCallback(",
                             itf_name.type_name.c_str()));
  text->PushOffset(kLineContinuationOffset);
  text->AddLine("const base::Callback<void(const dbus::ObjectPath&)>& "
                "callback) {");
  text->PopOffset();
  text->PushOffset(kBlockOffset);
  text->AddLine(StringPrintf("on_%s_removed_ = callback;",
                              itf_name.MakeVariableName().c_str()));
  text->PopOffset();
  text->AddLine("}");

  text->AddBlankLine();
}

void ProxyGenerator::ObjectManager::AddOnPropertyChanged(
    const std::vector<Interface>& interfaces,
    IndentedText* text) {
  // If there are no interfaces with properties, comment out parameter
  // names for OnPropertyChanged() to prevent compiler warnings on unused
  // function parameters.
  auto has_props = [](const Interface& itf) { return !itf.properties.empty(); };
  auto itf_with_props = std::find_if(interfaces.begin(), interfaces.end(),
                                     has_props);
  if (itf_with_props == interfaces.end()) {
    text->AddLineAndPushOffsetTo("void OnPropertyChanged("
                                 "const dbus::ObjectPath& /* object_path */,",
                                 1, '(');
    text->AddLine("const std::string& /* interface_name */,");
    text->AddLine("const std::string& /* property_name */) {}");
    text->PopOffset();
    text->AddBlankLine();
    return;
  }
  text->AddLineAndPushOffsetTo("void OnPropertyChanged("
                               "const dbus::ObjectPath& object_path,",
                               1, '(');
  text->AddLine("const std::string& interface_name,");
  text->AddLine("const std::string& property_name) {");
  text->PopOffset();
  text->PushOffset(kBlockOffset);
  for (const auto& itf : interfaces) {
    if (itf.properties.empty())
      continue;

    NameParser itf_name{itf.name};
    text->AddLine(StringPrintf("if (interface_name == \"%s\") {",
                               itf.name.c_str()));
    text->PushOffset(kBlockOffset);
    string map_name = itf_name.MakeVariableName() + "_instances_";
    text->AddLine(StringPrintf("auto p = %s.find(object_path);",
                               map_name.c_str()));
    text->AddLine(StringPrintf("if (p == %s.end())", map_name.c_str()));
    text->PushOffset(kBlockOffset);
    text->AddLine("return;");
    text->PopOffset();
    text->AddLine("p->second->OnPropertyChanged(property_name);");
    text->AddLine("return;");
    text->PopOffset();
    text->AddLine("}");
  }
  text->PopOffset();
  text->AddLine("}");
  text->AddBlankLine();
}

void ProxyGenerator::ObjectManager::AddObjectAdded(
    const ServiceConfig& config,
    const std::vector<Interface>& interfaces,
    IndentedText* text) {
  text->AddLine("void ObjectAdded(");
  text->PushOffset(kLineContinuationOffset);
  text->AddLine("const dbus::ObjectPath& object_path,");
  text->AddLine("const std::string& interface_name) override {");
  text->PopOffset();
  text->PushOffset(kBlockOffset);
  for (const auto& itf : interfaces) {
    NameParser itf_name{itf.name};
    string var_name = itf_name.MakeVariableName();
    text->AddLine(StringPrintf("if (interface_name == \"%s\") {",
                               itf.name.c_str()));
    text->PushOffset(kBlockOffset);
    if (!itf.properties.empty()) {
      text->AddLine("auto property_set =");
      text->PushOffset(kLineContinuationOffset);
      text->AddLine(StringPrintf("static_cast<%s::PropertySet*>(",
                                 itf_name.MakeProxyName(true).c_str()));
      text->PushOffset(kLineContinuationOffset);
      text->AddLine("dbus_object_manager_->GetProperties(object_path, "
                    "interface_name));");
      text->PopOffset();
      text->PopOffset();
    }
    text->AddLine(StringPrintf("std::unique_ptr<%s> %s_proxy{",
                               itf_name.MakeProxyName(true).c_str(),
                               var_name.c_str()));
    text->PushOffset(kBlockOffset);
    string new_instance = StringPrintf("new %s{bus_",
                                       itf_name.MakeProxyName(true).c_str());
    if (config.service_name.empty()) {
      new_instance += ", service_name_";
    }
    if (itf.path.empty())
      new_instance += ", object_path";
    if (!itf.properties.empty())
      new_instance += ", property_set";
    new_instance += "}";
    text->AddLine(new_instance);
    text->PopOffset();
    text->AddLine("};");
    text->AddLine(StringPrintf("auto p = %s_instances_.emplace(object_path, "
                               "std::move(%s_proxy));",
                               var_name.c_str(), var_name.c_str()));
    text->AddLine(StringPrintf("if (!on_%s_added_.is_null())",
                               var_name.c_str()));
    text->PushOffset(kBlockOffset);
    text->AddLine(StringPrintf("on_%s_added_.Run(p.first->second.get());",
                               var_name.c_str()));
    text->PopOffset();
    text->AddLine("return;");
    text->PopOffset();
    text->AddLine("}");
  }
  text->PopOffset();
  text->AddLine("}");
  text->AddBlankLine();
}

void ProxyGenerator::ObjectManager::AddObjectRemoved(
    const std::vector<Interface>& interfaces,
    IndentedText* text) {
  text->AddLine("void ObjectRemoved(");
  text->PushOffset(kLineContinuationOffset);
  text->AddLine("const dbus::ObjectPath& object_path,");
  text->AddLine("const std::string& interface_name) override {");
  text->PopOffset();
  text->PushOffset(kBlockOffset);
  for (const auto& itf : interfaces) {
    NameParser itf_name{itf.name};
    string var_name = itf_name.MakeVariableName();
    text->AddLine(StringPrintf("if (interface_name == \"%s\") {",
                               itf.name.c_str()));
    text->PushOffset(kBlockOffset);
    text->AddLine(StringPrintf("auto p = %s_instances_.find(object_path);",
                               var_name.c_str()));
    text->AddLine(StringPrintf("if (p != %s_instances_.end()) {",
                               var_name.c_str()));
    text->PushOffset(kBlockOffset);
    text->AddLine(StringPrintf("if (!on_%s_removed_.is_null())",
                               var_name.c_str()));
    text->PushOffset(kBlockOffset);
    text->AddLine(StringPrintf("on_%s_removed_.Run(object_path);",
                               var_name.c_str()));
    text->PopOffset();
    text->AddLine(StringPrintf("%s_instances_.erase(p);",
                               var_name.c_str()));
    text->PopOffset();
    text->AddLine("}");
    text->AddLine("return;");
    text->PopOffset();
    text->AddLine("}");
  }
  text->PopOffset();
  text->AddLine("}");
  text->AddBlankLine();
}

void ProxyGenerator::ObjectManager::AddCreateProperties(
    const std::vector<Interface>& interfaces,
    const std::string& class_name,
    IndentedText* text) {
  text->AddLine("dbus::PropertySet* CreateProperties(");
  text->PushOffset(kLineContinuationOffset);
  text->AddLine("dbus::ObjectProxy* object_proxy,");
  text->AddLine("const dbus::ObjectPath& object_path,");
  text->AddLine("const std::string& interface_name) override {");
  text->PopOffset();
  text->PushOffset(kBlockOffset);
  for (const auto& itf : interfaces) {
    NameParser itf_name{itf.name};
    text->AddLine(StringPrintf("if (interface_name == \"%s\") {",
                               itf.name.c_str()));
    text->PushOffset(kBlockOffset);
    text->AddLine(StringPrintf("return new %s::PropertySet{",
                               itf_name.MakeProxyName(true).c_str()));
    text->PushOffset(kLineContinuationOffset);
    text->AddLine("object_proxy,");
    text->AddLineAndPushOffsetTo(
        StringPrintf("base::Bind(&%s::OnPropertyChanged,",
                     class_name.c_str()),
        1, '(');
    text->AddLine("weak_ptr_factory_.GetWeakPtr(),");
    text->AddLine("object_path,");
    text->AddLine("interface_name)");
    text->PopOffset();
    text->PopOffset();
    text->AddLine("};");
    text->PopOffset();
    text->AddLine("}");
  }
  text->AddLineAndPushOffsetTo("LOG(FATAL) << \"Creating properties for "
                               "unsupported interface \"", 1, ' ');
  text->AddLine("<< interface_name;");
  text->PopOffset();
  text->AddLine("return nullptr;");
  text->PopOffset();
  text->AddLine("}");
  text->AddBlankLine();
}

void ProxyGenerator::ObjectManager::AddDataMembers(
    const ServiceConfig& config,
    const std::vector<Interface>& interfaces,
    const std::string& class_name,
    IndentedText* text) {
  text->AddLine("scoped_refptr<dbus::Bus> bus_;");
  if (config.service_name.empty()) {
    text->AddLine("std::string service_name_;");
  }
  text->AddLine("dbus::ObjectManager* dbus_object_manager_;");
  for (const auto& itf : interfaces) {
    NameParser itf_name{itf.name};
    string var_name = itf_name.MakeVariableName();
    text->AddLineAndPushOffsetTo("std::map<dbus::ObjectPath,", 1, '<');
    text->AddLine(StringPrintf("std::unique_ptr<%s>> %s_instances_;",
                               itf_name.MakeProxyName(true).c_str(),
                               var_name.c_str()));
    text->PopOffset();
    text->AddLine(
        StringPrintf("base::Callback<void(%sInterface*)> on_%s_added_;",
                     itf_name.MakeProxyName(true).c_str(),
                     var_name.c_str()));
    text->AddLine(StringPrintf("base::Callback<void(const dbus::ObjectPath&)> "
                               "on_%s_removed_;",
                               var_name.c_str()));
  }
  text->AddLine(
      StringPrintf("base::WeakPtrFactory<%s> weak_ptr_factory_{this};",
                   class_name.c_str()));
  text->AddBlankLine();
}

// static
string ProxyGenerator::GetHandlerNameForSignal(const string& signal) {
  return StringPrintf("On%sSignal", signal.c_str());
}

}  // namespace chromeos_dbus_bindings
