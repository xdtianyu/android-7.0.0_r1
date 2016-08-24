// Copyright 2014 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>
#include <string>

#include <base/command_line.h>
#include <base/files/file_path.h>
#include <base/files/file_util.h>
#include <base/json/json_reader.h>
#include <base/logging.h>
#include <base/strings/string_util.h>
#include <base/values.h>
#include <brillo/syslog_logging.h>

#include "chromeos-dbus-bindings/adaptor_generator.h"
#include "chromeos-dbus-bindings/method_name_generator.h"
#include "chromeos-dbus-bindings/proxy_generator.h"
#include "chromeos-dbus-bindings/xml_interface_parser.h"

using chromeos_dbus_bindings::AdaptorGenerator;
using chromeos_dbus_bindings::MethodNameGenerator;
using chromeos_dbus_bindings::ProxyGenerator;
using chromeos_dbus_bindings::ServiceConfig;

namespace switches {

static const char kHelp[] = "help";
static const char kMethodNames[] = "method-names";
static const char kAdaptor[] = "adaptor";
static const char kProxy[] = "proxy";
static const char kMock[] = "mock";
static const char kProxyPathForMocks[] = "proxy-path-in-mocks";
static const char kServiceConfig[] = "service-config";
static const char kHelpMessage[] = "\n"
    "generate-chromeos-dbus-bindings itf1.xml [itf2.xml...] [switches]\n"
    "    itf1.xml, ... = the input interface file(s) [mandatory].\n"
    "Available Switches: \n"
    "  --method-names=<method name header filename>\n"
    "    The output header file with string constants for each method name.\n"
    "  --adaptor=<adaptor header filename>\n"
    "    The output header file name containing the DBus adaptor class.\n"
    "  --proxy=<proxy header filename>\n"
    "    The output header file name containing the DBus proxy class.\n"
    "  --mock=<mock header filename>\n"
    "    The output header file name containing the DBus proxy mock class.\n"
    "  --service-config=<config.json>\n"
    "    The DBus service configuration file for the generator.\n";

}  // namespace switches

namespace {
// GYP sometimes enclosed the target file name in extra set of quotes like:
//    generate-chromeos-dbus-bindings in.xml "--adaptor=\"out.h\""
// So, this function helps us to remove them.
base::FilePath RemoveQuotes(const std::string& path) {
  std::string unquoted;
  base::TrimString(path, "\"'", &unquoted);
  return base::FilePath{unquoted};
}

// Makes a canonical path by making the path absolute and by removing any
// '..' which makes base::ReadFileToString() to fail.
base::FilePath SanitizeFilePath(const std::string& path) {
  base::FilePath path_in = RemoveQuotes(path);
  base::FilePath path_out = base::MakeAbsoluteFilePath(path_in);
  if (path_out.value().empty()) {
    LOG(WARNING) << "Failed to canonicalize '" << path << "'";
    path_out = path_in;
  }
  return path_out;
}


// Load the service configuration from the provided JSON file.
bool LoadConfig(const base::FilePath& path, ServiceConfig *config) {
  std::string contents;
  if (!base::ReadFileToString(path, &contents))
    return false;

  std::unique_ptr<base::Value> json{base::JSONReader::Read(contents).release()};
  if (!json)
    return false;

  base::DictionaryValue* dict = nullptr;  // Aliased with |json|.
  if (!json->GetAsDictionary(&dict))
    return false;

  dict->GetStringWithoutPathExpansion("service_name", &config->service_name);

  base::DictionaryValue* om_dict = nullptr;  // Owned by |dict|.
  if (dict->GetDictionaryWithoutPathExpansion("object_manager", &om_dict)) {
    if (!om_dict->GetStringWithoutPathExpansion("name",
                                                &config->object_manager.name) &&
        !config->service_name.empty()) {
      config->object_manager.name = config->service_name + ".ObjectManager";
    }
    om_dict->GetStringWithoutPathExpansion("object_path",
                                           &config->object_manager.object_path);
    if (config->object_manager.name.empty()) {
      LOG(ERROR) << "Object manager name is missing.";
      return false;
    }
  }

  base::ListValue* list = nullptr;  // Owned by |dict|.
  if (dict->GetListWithoutPathExpansion("ignore_interfaces", &list)) {
    config->ignore_interfaces.reserve(list->GetSize());
    for (base::Value* item : *list) {
      std::string interface_name;
      if (!item->GetAsString(&interface_name)) {
        LOG(ERROR) << "Invalid interface name in [ignore_interfaces] section";
        return false;
      }
      config->ignore_interfaces.push_back(interface_name);
    }
  }

  return true;
}

}   // anonymous namespace

int main(int argc, char** argv) {
  base::CommandLine::Init(argc, argv);
  base::CommandLine* cl = base::CommandLine::ForCurrentProcess();

  // Setup logging to stderr. This also parses some implicit flags using the
  // CommandLine singleton.
  brillo::InitLog(brillo::kLogToStderr | brillo::kLogHeader);

  if (cl->HasSwitch(switches::kHelp)) {
    LOG(INFO) << switches::kHelpMessage;
    return 0;
  }

  auto input_files = cl->GetArgs();
  if (input_files.empty()) {
    LOG(ERROR) << "At least one file must be specified.";
    LOG(ERROR) << switches::kHelpMessage;
    return 1;
  }

  ServiceConfig config;
  if (cl->HasSwitch(switches::kServiceConfig)) {
    std::string config_file = cl->GetSwitchValueASCII(switches::kServiceConfig);
    if (!config_file.empty() &&
        !LoadConfig(SanitizeFilePath(config_file), &config)) {
      LOG(ERROR) << "Failed to load DBus service config file " << config_file;
      return 1;
    }
  }

  chromeos_dbus_bindings::XmlInterfaceParser parser;
  for (const auto& input : input_files) {
    std::string contents;
    if (!base::ReadFileToString(SanitizeFilePath(input), &contents)) {
      LOG(ERROR) << "Failed to read file " << input;
      return 1;
    }
    if (!parser.ParseXmlInterfaceFile(contents, config.ignore_interfaces)) {
      LOG(ERROR) << "Failed to parse interface file " << input;
      return 1;
    }
  }

  if (cl->HasSwitch(switches::kMethodNames)) {
    std::string method_name_file =
        cl->GetSwitchValueASCII(switches::kMethodNames);
    VLOG(1) << "Outputting method names to " << method_name_file;
    if (!MethodNameGenerator::GenerateMethodNames(
            parser.interfaces(),
            RemoveQuotes(method_name_file))) {
      LOG(ERROR) << "Failed to output method names.";
      return 1;
    }
  }

  if (cl->HasSwitch(switches::kAdaptor)) {
    std::string adaptor_file = cl->GetSwitchValueASCII(switches::kAdaptor);
    VLOG(1) << "Outputting adaptor to " << adaptor_file;
    if (!AdaptorGenerator::GenerateAdaptors(parser.interfaces(),
                                            RemoveQuotes(adaptor_file))) {
      LOG(ERROR) << "Failed to output adaptor.";
      return 1;
     }
  }

  base::FilePath proxy_path;  // Used by both Proxy and Mock generation.
  if (cl->HasSwitch(switches::kProxy)) {
    std::string proxy_file = cl->GetSwitchValueASCII(switches::kProxy);
    proxy_path = RemoveQuotes(proxy_file);
    base::NormalizeFilePath(proxy_path, &proxy_path);
    VLOG(1) << "Outputting proxy to " << proxy_path.value();
    if (!ProxyGenerator::GenerateProxies(config, parser.interfaces(),
                                         proxy_path)) {
      LOG(ERROR) << "Failed to output proxy.";
      return 1;
     }
  }

  base::FilePath proxy_include_path = proxy_path;
  bool use_literal_include_path = false;
  if (cl->HasSwitch(switches::kProxyPathForMocks)) {
    std::string proxy_file_in_mocks =
        cl->GetSwitchValueASCII(switches::kProxyPathForMocks);
    proxy_include_path = RemoveQuotes(proxy_file_in_mocks);
    use_literal_include_path = true;
  }

  if (cl->HasSwitch(switches::kMock)) {
    std::string mock_file = cl->GetSwitchValueASCII(switches::kMock);
    base::FilePath mock_path = RemoveQuotes(mock_file);
    base::NormalizeFilePath(mock_path, &mock_path);
    VLOG(1) << "Outputting mock to " << mock_path.value();
    if (!ProxyGenerator::GenerateMocks(config, parser.interfaces(), mock_path,
                                       proxy_include_path,
                                       use_literal_include_path)) {
      LOG(ERROR) << "Failed to output mock.";
      return 1;
     }
  }

  return 0;
}
