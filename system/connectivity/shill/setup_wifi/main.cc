//
// Copyright (C) 2015 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#include <sysexits.h>

#include <map>
#include <memory>
#include <string>

#include <base/command_line.h>
#include <base/logging.h>
#include <brillo/any.h>
#include <brillo/daemons/dbus_daemon.h>
#if defined(__ANDROID__)
#include <dbus/service_constants.h>
#else
#include <chromeos/dbus/service_constants.h>
#endif  // __ANDROID__
#include <shill/dbus-proxies.h>

namespace {

namespace switches {
static const char kHelp[] = "help";
static const char kPassphrase[] = "passphrase";
static const char kHexSsid[] = "hex-ssid";
static const char kSSID[] = "ssid";
static const char kHelpMessage[] = "\n"
    "Available Switches: \n"
    "  --ssid=<ssid>\n"
    "    Set the SSID to configure (mandatory).\n"
    "  --hex-ssid\n"
    "    SSID is provided in hexadecimal\n"
    "  --passphrase=<passprhase>\n"
    "    Set the passphrase for PSK networks\n";
}  // namespace switches

}  // namespace

class MyClient : public brillo::DBusDaemon {
 public:
  MyClient(std::string ssid, bool is_hex_ssid, std::string psk)
      : ssid_(ssid), is_hex_ssid_(is_hex_ssid), psk_(psk) {}
  ~MyClient() override = default;

 protected:
  int OnInit() override {
    int ret = DBusDaemon::OnInit();
    if (ret != EX_OK) {
      return ret;
    }
    ConfigureAndConnect();
    Quit();
    return EX_OK;
  }

  bool ConfigureAndConnect() {
    std::unique_ptr<org::chromium::flimflam::ManagerProxy> shill_manager_proxy(
        new org::chromium::flimflam::ManagerProxy(bus_));

    dbus::ObjectPath created_service;
    brillo::ErrorPtr configure_error;
    if (!shill_manager_proxy->ConfigureService(
            GetServiceConfig(), &created_service, &configure_error)) {
      LOG(ERROR) << "Configure service failed";
      return false;
    }

    brillo::ErrorPtr connect_error;
    std::unique_ptr<org::chromium::flimflam::ServiceProxy> shill_service_proxy(
        new org::chromium::flimflam::ServiceProxy(bus_,
                                                  created_service));
    if (!shill_service_proxy->Connect(&connect_error)) {
      LOG(ERROR) << "Connect service failed";
      return false;
    }

    // TODO(pstew): Monitor service as it attempts to connect.

    return true;
  }

  std::map<std::string, brillo::Any> GetServiceConfig() {
    std::map<std::string, brillo::Any> configure_dict;
    configure_dict[shill::kTypeProperty] = shill::kTypeWifi;
    if (is_hex_ssid_) {
      configure_dict[shill::kWifiHexSsid] = ssid_;
    } else {
      configure_dict[shill::kSSIDProperty] = ssid_;
    }
    if (!psk_.empty()) {
      configure_dict[shill::kPassphraseProperty] = psk_;
      configure_dict[shill::kSecurityProperty] = shill::kSecurityPsk;
    }
    return configure_dict;
  }

  std::string ssid_;
  bool is_hex_ssid_;
  std::string psk_;
};

int main(int argc, char** argv) {
  base::CommandLine::Init(argc, argv);
  base::CommandLine* cl = base::CommandLine::ForCurrentProcess();

  if (cl->HasSwitch(switches::kHelp)) {
    LOG(INFO) << switches::kHelpMessage;
    return EXIT_SUCCESS;
  }

  if (!cl->HasSwitch(switches::kSSID)) {
    LOG(ERROR) << "ssid switch is mandatory.";
    LOG(ERROR) << switches::kHelpMessage;
    return EXIT_FAILURE;
  }

  std::string ssid = cl->GetSwitchValueASCII(switches::kSSID);
  std::string psk;
  if (cl->HasSwitch(switches::kPassphrase)) {
    psk = cl->GetSwitchValueASCII(switches::kPassphrase);
  }
  bool hex_ssid = cl->HasSwitch(switches::kHexSsid);

  MyClient client(ssid, hex_ssid, psk);
  client.Run();
  LOG(INFO) << "Process exiting.";

  return EXIT_SUCCESS;
}

