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
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include <iostream>

#include <base/command_line.h>
#include <base/logging.h>
#include <base/message_loop/message_loop.h>

#include "proxy_dbus_shill_wifi_client.h"
#include "proxy_shill_wifi_client.h"
#include "proxy_rpc_server.h"

namespace {
namespace switches {
static const char kHelp[] = "help";
static const char kPort[] = "port";
static const char kHelpMessage[] = "\n"
    "Available Switches: \n"
    "  --port=<port>\n"
    "    Set the RPC server to listen on this TCP port(mandatory).\n";
}  // namespace switches
}  // namespace

int main(int argc, char* argv[]) {
  base::CommandLine::Init(argc, argv);
  base::CommandLine* cl = base::CommandLine::ForCurrentProcess();

  if (cl->HasSwitch(switches::kHelp)) {
    LOG(INFO) << switches::kHelpMessage;
    return EXIT_SUCCESS;
  }

  int xml_rpc_port;
  if (!cl->HasSwitch(switches::kPort)) {
    LOG(ERROR) << "port switch is mandatory.";
    LOG(ERROR) << switches::kHelpMessage;
    return EXIT_FAILURE;
  }
  xml_rpc_port = std::stoi(cl->GetSwitchValueASCII(switches::kPort));

  // Create and instantiate a message loop so that we can use it
  // to block for asynchronous dbus signal callbacks. This needs
  // to be instantiated before we connect to dbus.
  base::MessageLoopForIO message_loop;

  // Connect to dbus's system bus.
  dbus::Bus::Options options;
  options.bus_type = dbus::Bus::SYSTEM;
  scoped_refptr<dbus::Bus> dbus_bus = new dbus::Bus(options);
  CHECK(dbus_bus->Connect());

  // We're creating the Dbus version of the Shill Wifi Client for now.
  std::unique_ptr<ProxyShillWifiClient> shill_wifi_client(
      new ProxyDbusShillWifiClient(dbus_bus));

  // Create the RPC server object
  std::unique_ptr<ProxyRpcServer> rpc_server(
      new ProxyRpcServer(xml_rpc_port, std::move(shill_wifi_client)));

  // Run indefinitely
  rpc_server->Run();

  return 0;
}

