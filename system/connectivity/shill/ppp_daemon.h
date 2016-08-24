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

#ifndef SHILL_PPP_DAEMON_H_
#define SHILL_PPP_DAEMON_H_

#include <string>

#include <base/callback.h>
#include <base/memory/weak_ptr.h>
#include <gtest/gtest_prod.h>

#include "shill/external_task.h"

namespace shill {

class ControlInterface;
class Error;
class ProcessManager;

// PPPDaemon provides control over the configuration and instantiation of pppd
// processes.  All pppd instances created through PPPDaemon will use shill's
// pppd plugin.
class PPPDaemon {
 public:
  // The type of callback invoked when an ExternalTask wrapping a pppd instance
  // dies.  The first argument is the pid of the process, the second is the exit
  // code.
  typedef base::Callback<void(pid_t, int)> DeathCallback;

  // Provides options used when preparing a pppd task for execution.  These map
  // to pppd command-line options.  Refer to https://ppp.samba.org/pppd.html for
  // more details about the meaning of each.
  struct Options {
    Options()
        : debug(false),
          no_detach(false),
          no_default_route(false),
          use_peer_dns(false),
          use_shim_plugin(true),
          use_pppoe_plugin(false),
          lcp_echo_interval(kUnspecifiedValue),
          lcp_echo_failure(kUnspecifiedValue),
          max_fail(kUnspecifiedValue),
          use_ipv6(false) {}

    // Causes pppd to emit log messages useful for debugging connectivity.
    bool debug;

    // Causes pppd to not fork and daemonize, remaining attached to the
    // controlling terminal that spawned it.
    bool no_detach;

    // Stops pppd from modifying the routing table.
    bool no_default_route;

    // Instructs pppd to request DNS servers from the remote server.
    bool use_peer_dns;

    // If set, will cause the shill pppd plugin to be used at the creation of
    // the pppd instace.  This will result in connectivity events being plumbed
    // over D-Bus to the RPCTaskDelegate provided during PPPDaemon::Start.
    bool use_shim_plugin;

    // If set, enables the rp-pppoe plugin which allows pppd to be used over
    // ethernet devices.
    bool use_pppoe_plugin;

    // The number of seconds between sending LCP echo requests.
    uint32_t lcp_echo_interval;

    // The number of missed LCP echo responses tolerated before disconnecting.
    uint32_t lcp_echo_failure;

    // The number of allowed failed consecutive connection attempts before
    // giving up.  A value of 0 means there is no limit.
    uint32_t max_fail;

    // Instructs pppd to request an IPv6 address from the remote server.
    bool use_ipv6;
  };

  // The path to the pppd plugin provided by shill.
  static const char kShimPluginPath[];

  // Starts a pppd instance.  |options| provides the configuration for the
  // instance to be started, |device| specifies which device the PPP connection
  // is to be established on, |death_callback| will be invoked when the
  // underlying pppd process dies.  |error| is populated if the task cannot be
  // started, and nullptr is returned.
  static std::unique_ptr<ExternalTask> Start(
      ControlInterface* control_interface,
      ProcessManager* process_manager,
      const base::WeakPtr<RPCTaskDelegate>& task_delegate,
      const Options& options,
      const std::string& device,
      const DeathCallback& death_callback,
      Error* error);

 private:
  FRIEND_TEST(PPPDaemonTest, PluginUsed);

  static const char kDaemonPath[];
  static const char kPPPoEPluginPath[];
  static const uint32_t kUnspecifiedValue;

  PPPDaemon();
  ~PPPDaemon();

  DISALLOW_COPY_AND_ASSIGN(PPPDaemon);
};

}  // namespace shill

#endif  // SHILL_PPP_DAEMON_H_
