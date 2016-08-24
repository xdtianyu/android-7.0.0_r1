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

#include "shill/ppp_daemon.h"

#include <stdint.h>

#include <map>
#include <string>
#include <vector>

#include <base/files/file_path.h>
#include <base/memory/weak_ptr.h>
#include <base/strings/string_number_conversions.h>

#include "shill/control_interface.h"
#include "shill/error.h"
#include "shill/external_task.h"
#include "shill/ppp_device.h"

namespace shill {

const char PPPDaemon::kDaemonPath[] = "/usr/sbin/pppd";
const char PPPDaemon::kShimPluginPath[] = SHIMDIR "/shill-pppd-plugin.so";
const char PPPDaemon::kPPPoEPluginPath[] = "rp-pppoe.so";
const uint32_t PPPDaemon::kUnspecifiedValue = UINT32_MAX;

std::unique_ptr<ExternalTask> PPPDaemon::Start(
    ControlInterface* control_interface,
    ProcessManager* process_manager,
    const base::WeakPtr<RPCTaskDelegate>& task_delegate,
    const PPPDaemon::Options& options,
    const std::string& device,
    const PPPDaemon::DeathCallback& death_callback,
    Error* error) {
  std::vector<std::string> arguments;
  if (options.debug) {
    arguments.push_back("debug");
  }
  if (options.no_detach) {
    arguments.push_back("nodetach");
  }
  if (options.no_default_route) {
    arguments.push_back("nodefaultroute");
  }
  if (options.use_peer_dns) {
    arguments.push_back("usepeerdns");
  }
  if (options.use_shim_plugin) {
    arguments.push_back("plugin");
    arguments.push_back(kShimPluginPath);
  }
  if (options.use_pppoe_plugin) {
    arguments.push_back("plugin");
    arguments.push_back(kPPPoEPluginPath);
  }
  if (options.lcp_echo_interval != kUnspecifiedValue) {
    arguments.push_back("lcp-echo-interval");
    arguments.push_back(base::UintToString(options.lcp_echo_interval));
  }
  if (options.lcp_echo_failure != kUnspecifiedValue) {
    arguments.push_back("lcp-echo-failure");
    arguments.push_back(base::UintToString(options.lcp_echo_failure));
  }
  if (options.max_fail != kUnspecifiedValue) {
    arguments.push_back("maxfail");
    arguments.push_back(base::UintToString(options.max_fail));
  }
  if (options.use_ipv6) {
    arguments.push_back("+ipv6");
    arguments.push_back("ipv6cp-use-ipaddr");
  }

  arguments.push_back(device);

  std::unique_ptr<ExternalTask> task(new ExternalTask(
      control_interface, process_manager, task_delegate, death_callback));

  std::map<std::string, std::string> environment;
  if (task->Start(base::FilePath(kDaemonPath), arguments, environment, true,
                  error)) {
    return task;
  }
  return nullptr;
}

}  // namespace shill
