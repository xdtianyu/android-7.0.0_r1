//
// Copyright (C) 2012 The Android Open Source Project
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

#include "shill/dhcp/dhcp_provider.h"

#include <signal.h>

#include <base/bind.h>
#include <base/files/file_util.h>
#include <base/process/process.h>
#include <base/process/process_iterator.h>
#include <base/stl_util.h>
#include <base/strings/stringprintf.h>

#include "shill/control_interface.h"
#include "shill/dhcp/dhcpcd_listener_interface.h"
#include "shill/dhcp/dhcpv4_config.h"
#ifndef DISABLE_DHCPV6
#include "shill/dhcp/dhcpv6_config.h"
#endif
#include "shill/dhcp_properties.h"
#include "shill/event_dispatcher.h"
#include "shill/logging.h"

using base::FilePath;
using std::string;

namespace shill {

namespace Logging {
static auto kModuleLogScope = ScopeLogger::kDHCP;
static string ObjectID(DHCPProvider* d) { return "(dhcp_provider)"; }
}

namespace {
base::LazyInstance<DHCPProvider> g_dhcp_provider = LAZY_INSTANCE_INITIALIZER;
static const int kUnbindDelayMilliseconds = 2000;

#if defined(__ANDROID__)
const char kDHCPCDExecutableName[] = "dhcpcd-6.8.2";
#else
const char kDHCPCDExecutableName[] = "dhcpcd";
#endif  // __ANDROID__

}  // namespace

constexpr char DHCPProvider::kDHCPCDPathFormatLease[];
#ifndef DISABLE_DHCPV6
constexpr char DHCPProvider::kDHCPCDPathFormatLease6[];
#endif  // DISABLE_DHCPV6

DHCPProvider::DHCPProvider()
    : root_("/"),
      control_interface_(nullptr),
      dispatcher_(nullptr),
      metrics_(nullptr) {
  SLOG(this, 2) << __func__;
}

DHCPProvider::~DHCPProvider() {
  SLOG(this, 2) << __func__;
}

DHCPProvider* DHCPProvider::GetInstance() {
  return g_dhcp_provider.Pointer();
}

void DHCPProvider::Init(ControlInterface* control_interface,
                        EventDispatcher* dispatcher,
                        Metrics* metrics) {
  SLOG(this, 2) << __func__;
  listener_.reset(control_interface->CreateDHCPCDListener(this));
  control_interface_ = control_interface;
  dispatcher_ = dispatcher;
  metrics_ = metrics;

  // Kill the dhcpcd processes accidentally left by previous run.
  base::NamedProcessIterator iter(kDHCPCDExecutableName, NULL);
  while (const base::ProcessEntry* entry = iter.NextProcessEntry()) {
    base::Process process = base::Process::Open(entry->pid());
    kill(process.Pid(),SIGKILL);
  }
}

void DHCPProvider::Stop() {
  listener_.reset();
}

DHCPConfigRefPtr DHCPProvider::CreateIPv4Config(
    const string& device_name,
    const string& lease_file_suffix,
    bool arp_gateway,
    const DhcpProperties& dhcp_props) {
  SLOG(this, 2) << __func__ << " device: " << device_name;
  return new DHCPv4Config(control_interface_,
                          dispatcher_,
                          this,
                          device_name,
                          lease_file_suffix,
                          arp_gateway,
                          dhcp_props,
                          metrics_);
}

#ifndef DISABLE_DHCPV6
DHCPConfigRefPtr DHCPProvider::CreateIPv6Config(
    const string& device_name, const string& lease_file_suffix) {
  SLOG(this, 2) << __func__ << " device: " << device_name;
  return new DHCPv6Config(control_interface_,
                          dispatcher_,
                          this,
                          device_name,
                          lease_file_suffix);
}
#endif

DHCPConfigRefPtr DHCPProvider::GetConfig(int pid) {
  SLOG(this, 2) << __func__ << " pid: " << pid;
  PIDConfigMap::const_iterator it = configs_.find(pid);
  if (it == configs_.end()) {
    return nullptr;
  }
  return it->second;
}

void DHCPProvider::BindPID(int pid, const DHCPConfigRefPtr& config) {
  SLOG(this, 2) << __func__ << " pid: " << pid;
  configs_[pid] = config;
}

void DHCPProvider::UnbindPID(int pid) {
  SLOG(this, 2) << __func__ << " pid: " << pid;
  configs_.erase(pid);
  recently_unbound_pids_.insert(pid);
  dispatcher_->PostDelayedTask(base::Bind(&DHCPProvider::RetireUnboundPID,
                                          base::Unretained(this),
                                          pid), kUnbindDelayMilliseconds);
}

void DHCPProvider::RetireUnboundPID(int pid) {
  recently_unbound_pids_.erase(pid);
}

bool DHCPProvider::IsRecentlyUnbound(int pid) {
  return ContainsValue(recently_unbound_pids_, pid);
}

void DHCPProvider::DestroyLease(const string& name) {
  SLOG(this, 2) << __func__ << " name: " << name;
  base::DeleteFile(root_.Append(
      base::StringPrintf(kDHCPCDPathFormatLease,
                         name.c_str())), false);
#ifndef DISABLE_DHCPV6
  base::DeleteFile(root_.Append(
      base::StringPrintf(kDHCPCDPathFormatLease6,
                         name.c_str())), false);
#endif  // DISABLE_DHCPV6
}

}  // namespace shill
