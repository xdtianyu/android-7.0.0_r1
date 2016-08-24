// Copyright 2015 The Android Open Source Project
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

#include <signal.h>
#include <sysexits.h>

#include <string>

#include <base/command_line.h>
#include <base/files/file_util.h>
#include <brillo/dbus/async_event_sequencer.h>
#include <brillo/dbus/exported_object_manager.h>
#include <brillo/daemons/dbus_daemon.h>
#include <brillo/flag_helper.h>
#if !defined(__ANDROID__)
#include <brillo/minijail/minijail.h>
#endif  // !defined(__ANDROID__)
#include <brillo/syslog_logging.h>

#include "webservd/config.h"
#include "webservd/log_manager.h"
#include "webservd/server.h"
#include "webservd/utils.h"

#if defined(__ANDROID__)
#include "webservd/firewalld_firewall.h"
using FirewallImpl = webservd::FirewalldFirewall;
#else
#include "webservd/permission_broker_firewall.h"
using FirewallImpl = webservd::PermissionBrokerFirewall;
#endif  // defined(__ANDROID__)

using brillo::dbus_utils::AsyncEventSequencer;

namespace {

const char kDefaultConfigFilePath[] = "/etc/webservd/config";
const char kServiceName[] = "org.chromium.WebServer";
const char kRootServicePath[] = "/org/chromium/WebServer";
#if !defined(__ANDROID__)
const char kWebServerUserName[] = "webservd";
const char kWebServerGroupName[] = "webservd";
#endif  // !defined(__ANDROID__)

class Daemon final : public brillo::DBusServiceDaemon {
 public:
  explicit Daemon(webservd::Config config)
      : DBusServiceDaemon{kServiceName, kRootServicePath},
        config_{std::move(config)} {}

 protected:
  void RegisterDBusObjectsAsync(AsyncEventSequencer* sequencer) override {
    webservd::LogManager::Init(base::FilePath{config_.log_directory});
    server_.reset(new webservd::Server{
        object_manager_.get(), config_,
        std::unique_ptr<webservd::FirewallInterface>{new FirewallImpl()}});
    server_->RegisterAsync(
        sequencer->GetHandler("Server.RegisterAsync() failed.", true));
  }

  void OnShutdown(int* /* return_code */) override {
    server_.reset();
  }

 private:
  webservd::Config config_;
  std::unique_ptr<webservd::Server> server_;

  DISALLOW_COPY_AND_ASSIGN(Daemon);
};

}  // namespace

int main(int argc, char* argv[]) {
  DEFINE_bool(log_to_stderr, false, "log trace messages to stderr as well");
  DEFINE_string(config_path, "",
                "path to a file containing server configuration");
  DEFINE_bool(debug, false,
              "return debug error information in web requests");
  DEFINE_bool(ipv6, true, "enable IPv6 support");
  brillo::FlagHelper::Init(argc, argv, "Brillo web server daemon");

  // From libmicrohttpd documentation, section 1.5 SIGPIPE:
  // ... portable code using MHD must install a SIGPIPE handler or explicitly
  // block the SIGPIPE signal.
  // This also applies to using pipes over D-Bus to pass request/response data
  // to/from remote request handlers. We handle errors from write operations on
  // sockets/pipes correctly, so SIGPIPE is just a pest.
  signal(SIGPIPE, SIG_IGN);

  int flags = brillo::kLogToSyslog;
  if (FLAGS_log_to_stderr)
    flags |= brillo::kLogToStderr;
  brillo::InitLog(flags | brillo::kLogHeader);

  webservd::Config config;
  config.use_ipv6 = FLAGS_ipv6;
  base::FilePath default_file_path{kDefaultConfigFilePath};
  if (!FLAGS_config_path.empty()) {
    // In tests, we'll override the board specific and default configurations
    // with a test specific configuration.
    webservd::LoadConfigFromFile(base::FilePath{FLAGS_config_path}, &config);
  } else if (base::PathExists(default_file_path)) {
    // Some boards have a configuration they will want to use to override
    // our defaults.  Part of our interface is to look for this in a
    // standard location.
    CHECK(webservd::LoadConfigFromFile(default_file_path, &config));
  } else {
    webservd::LoadDefaultConfig(&config);
  }

  // For protocol handlers bound to specific network interfaces, we need root
  // access to create those bound sockets. Do that here before we drop
  // privileges.
  for (auto& handler_config : config.protocol_handlers) {
    if (!handler_config.interface_name.empty()) {
      int socket_fd =
          webservd::CreateNetworkInterfaceSocket(handler_config.interface_name);
      if (socket_fd < 0) {
        LOG(ERROR) << "Failed to create a socket for network interface "
                   << handler_config.interface_name;
        return EX_SOFTWARE;
      }
      handler_config.socket_fd = socket_fd;
    }
  }

  config.use_debug = FLAGS_debug;
  Daemon daemon{std::move(config)};

  // TODO: Re-enable this for Android once minijail works with libcap-ng.
#if !defined(__ANDROID__)
  // Drop privileges and use 'webservd' user. We need to do this after Daemon
  // object is constructed since it creates an instance of base::AtExitManager
  // which is required for brillo::Minijail::GetInstance() to work.
  brillo::Minijail* minijail_instance = brillo::Minijail::GetInstance();
  minijail* jail = minijail_instance->New();
  minijail_instance->DropRoot(jail, kWebServerUserName, kWebServerGroupName);
  // Permissions needed for the daemon to allow it to bind to ports like TCP
  // 80.
  minijail_instance->UseCapabilities(jail, CAP_TO_MASK(CAP_NET_BIND_SERVICE));
  minijail_enter(jail);
  minijail_instance->Destroy(jail);
#endif  // !defined(__ANDROID__)

  return daemon.Run();
}
