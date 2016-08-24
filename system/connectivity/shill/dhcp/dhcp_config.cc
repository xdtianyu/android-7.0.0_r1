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

#include "shill/dhcp/dhcp_config.h"

#include <vector>

#include <arpa/inet.h>
#include <stdlib.h>
#include <sys/wait.h>

#include <base/files/file_util.h>
#include <base/strings/string_split.h>
#include <base/strings/stringprintf.h>
#if defined(__ANDROID__)
#include <dbus/service_constants.h>
#else
#include <chromeos/dbus/service_constants.h>
#endif  // __ANDROID__
#include <brillo/minijail/minijail.h>

#include "shill/control_interface.h"
#include "shill/dhcp/dhcp_provider.h"
#include "shill/dhcp/dhcp_proxy_interface.h"
#include "shill/event_dispatcher.h"
#include "shill/logging.h"
#include "shill/metrics.h"
#include "shill/net/ip_address.h"
#include "shill/process_manager.h"

using std::string;
using std::vector;

namespace shill {

namespace Logging {
static auto kModuleLogScope = ScopeLogger::kDHCP;
static string ObjectID(DHCPConfig* d) {
  if (d == nullptr)
    return "(dhcp_config)";
  else
    return d->device_name();
}
}

// static
const int DHCPConfig::kAcquisitionTimeoutSeconds = 30;
const int DHCPConfig::kDHCPCDExitPollMilliseconds = 50;
const int DHCPConfig::kDHCPCDExitWaitMilliseconds = 3000;
#if defined(__ANDROID__)
const char DHCPConfig::kDHCPCDPath[] = "/system/bin/dhcpcd-6.8.2";
const char DHCPConfig::kDHCPCDUser[] = "dhcp";
const char DHCPConfig::kDHCPCDGroup[] = "dbus";
#else
const char DHCPConfig::kDHCPCDPath[] = "/sbin/dhcpcd";
const char DHCPConfig::kDHCPCDUser[] = "dhcp";
const char DHCPConfig::kDHCPCDGroup[] = "dhcp";
#endif  // __ANDROID__

DHCPConfig::DHCPConfig(ControlInterface* control_interface,
                       EventDispatcher* dispatcher,
                       DHCPProvider* provider,
                       const string& device_name,
                       const string& type,
                       const string& lease_file_suffix)
    : IPConfig(control_interface, device_name, type),
      control_interface_(control_interface),
      provider_(provider),
      lease_file_suffix_(lease_file_suffix),
      pid_(0),
      is_lease_active_(false),
      lease_acquisition_timeout_seconds_(kAcquisitionTimeoutSeconds),
      minimum_mtu_(kMinIPv4MTU),
      root_("/"),
      weak_ptr_factory_(this),
      dispatcher_(dispatcher),
      process_manager_(ProcessManager::GetInstance()) {
  SLOG(this, 2) << __func__ << ": " << device_name;
  if (lease_file_suffix_.empty()) {
    lease_file_suffix_ = device_name;
  }
}

DHCPConfig::~DHCPConfig() {
  SLOG(this, 2) << __func__ << ": " << device_name();

  // Don't leave behind dhcpcd running.
  Stop(__func__);
}

bool DHCPConfig::RequestIP() {
  SLOG(this, 2) << __func__ << ": " << device_name();
  if (!pid_) {
    return Start();
  }
  if (!proxy_.get()) {
    LOG(ERROR) << "Unable to request IP before acquiring destination.";
    return Restart();
  }
  return RenewIP();
}

bool DHCPConfig::RenewIP() {
  SLOG(this, 2) << __func__ << ": " << device_name();
  if (!pid_) {
    return Start();
  }
  if (!proxy_.get()) {
    LOG(ERROR) << "Unable to renew IP before acquiring destination.";
    return false;
  }
  StopExpirationTimeout();
  proxy_->Rebind(device_name());
  StartAcquisitionTimeout();
  return true;
}

bool DHCPConfig::ReleaseIP(ReleaseReason reason) {
  SLOG(this, 2) << __func__ << ": " << device_name();
  if (!pid_) {
    return true;
  }

  // If we are using static IP and haven't retrieved a lease yet, we should
  // allow the DHCP process to continue until we have a lease.
  if (!is_lease_active_ && reason == IPConfig::kReleaseReasonStaticIP) {
    return true;
  }

  // If we are using gateway unicast ARP to speed up re-connect, don't
  // give up our leases when we disconnect.
  bool should_keep_lease =
      reason == IPConfig::kReleaseReasonDisconnect &&
                ShouldKeepLeaseOnDisconnect();

  if (!should_keep_lease && proxy_.get()) {
    proxy_->Release(device_name());
  }
  Stop(__func__);
  return true;
}

void DHCPConfig::InitProxy(const string& service) {
  if (!proxy_.get()) {
    LOG(INFO) << "Init DHCP Proxy: " << device_name() << " at " << service;
    proxy_.reset(control_interface_->CreateDHCPProxy(service));
  }
}

void DHCPConfig::UpdateProperties(const Properties& properties,
                                  bool new_lease_acquired) {
  StopAcquisitionTimeout();
  if (properties.lease_duration_seconds) {
    UpdateLeaseExpirationTime(properties.lease_duration_seconds);
    StartExpirationTimeout(properties.lease_duration_seconds);
  } else {
    LOG(WARNING) << "Lease duration is zero; not starting an expiration timer.";
    ResetLeaseExpirationTime();
    StopExpirationTimeout();
  }
  IPConfig::UpdateProperties(properties, new_lease_acquired);
}

void DHCPConfig::NotifyFailure() {
  StopAcquisitionTimeout();
  StopExpirationTimeout();
  IPConfig::NotifyFailure();
}

bool DHCPConfig::IsEphemeralLease() const {
  return lease_file_suffix_ == device_name();
}

bool DHCPConfig::Start() {
  SLOG(this, 2) << __func__ << ": " << device_name();

  // Setup program arguments.
  vector<string> args = GetFlags();
  string interface_arg(device_name());
  if (lease_file_suffix_ != device_name()) {
    interface_arg = base::StringPrintf("%s=%s", device_name().c_str(),
                                       lease_file_suffix_.c_str());
  }
  args.push_back(interface_arg);

  uint64_t capmask = CAP_TO_MASK(CAP_NET_BIND_SERVICE) |
                     CAP_TO_MASK(CAP_NET_BROADCAST) |
                     CAP_TO_MASK(CAP_NET_ADMIN) |
                     CAP_TO_MASK(CAP_NET_RAW);
  pid_t pid = process_manager_->StartProcessInMinijail(
      FROM_HERE,
      base::FilePath(kDHCPCDPath),
      args,
      kDHCPCDUser,
      kDHCPCDGroup,
      capmask,
      base::Bind(&DHCPConfig::OnProcessExited,
                 weak_ptr_factory_.GetWeakPtr()));
  if (pid < 0) {
    return false;
  }
  pid_ = pid;
  LOG(INFO) << "Spawned " << kDHCPCDPath << " with pid: " << pid_;
  provider_->BindPID(pid_, this);
  StartAcquisitionTimeout();
  return true;
}

void DHCPConfig::Stop(const char* reason) {
  LOG_IF(INFO, pid_) << "Stopping " << pid_ << " (" << reason << ")";
  KillClient();
  // KillClient waits for the client to terminate so it's safe to cleanup the
  // state.
  CleanupClientState();
}

void DHCPConfig::KillClient() {
  if (!pid_) {
    return;
  }

  // Pass the termination responsibility to ProcessManager.
  // ProcessManager will try to terminate the process using SIGTERM, then
  // SIGKill signals.  It will log an error message if it is not able to
  // terminate the process in a timely manner.
  process_manager_->StopProcessAndBlock(pid_);
}

bool DHCPConfig::Restart() {
  // Take a reference of this instance to make sure we don't get destroyed in
  // the middle of this call.
  DHCPConfigRefPtr me = this;
  me->Stop(__func__);
  return me->Start();
}

void DHCPConfig::OnProcessExited(int exit_status) {
  CHECK(pid_);
  if (exit_status == EXIT_SUCCESS) {
    SLOG(nullptr, 2) << "pid " << pid_ << " exit status " << exit_status;
  } else {
    LOG(WARNING) << "pid " << pid_ << " exit status " << exit_status;
  }
  CleanupClientState();
}

void DHCPConfig::CleanupClientState() {
  SLOG(this, 2) << __func__ << ": " << device_name();
  StopAcquisitionTimeout();
  StopExpirationTimeout();

  proxy_.reset();
  if (pid_) {
    int pid = pid_;
    pid_ = 0;
    // |this| instance may be destroyed after this call.
    provider_->UnbindPID(pid);
  }
  is_lease_active_ = false;
}

vector<string> DHCPConfig::GetFlags() {
  vector<string> flags;
  flags.push_back("-B");  // Run in foreground.
  flags.push_back("-q");  // Only warnings+errors to stderr.
  return flags;
}

void DHCPConfig::StartAcquisitionTimeout() {
  CHECK(lease_expiration_callback_.IsCancelled());
  lease_acquisition_timeout_callback_.Reset(
      Bind(&DHCPConfig::ProcessAcquisitionTimeout,
           weak_ptr_factory_.GetWeakPtr()));
  dispatcher_->PostDelayedTask(
      lease_acquisition_timeout_callback_.callback(),
      lease_acquisition_timeout_seconds_ * 1000);
}

void DHCPConfig::StopAcquisitionTimeout() {
  lease_acquisition_timeout_callback_.Cancel();
}

void DHCPConfig::ProcessAcquisitionTimeout() {
  LOG(ERROR) << "Timed out waiting for DHCP lease on " << device_name() << " "
             << "(after " << lease_acquisition_timeout_seconds_ << " seconds).";
  if (!ShouldFailOnAcquisitionTimeout()) {
    LOG(INFO) << "Continuing to use our previous lease, due to gateway-ARP.";
  } else {
    NotifyFailure();
  }
}

void DHCPConfig::StartExpirationTimeout(uint32_t lease_duration_seconds) {
  CHECK(lease_acquisition_timeout_callback_.IsCancelled());
  SLOG(this, 2) << __func__ << ": " << device_name()
                << ": " << "Lease timeout is " << lease_duration_seconds
                << " seconds.";
  lease_expiration_callback_.Reset(
      Bind(&DHCPConfig::ProcessExpirationTimeout,
           weak_ptr_factory_.GetWeakPtr()));
  dispatcher_->PostDelayedTask(
      lease_expiration_callback_.callback(),
      lease_duration_seconds * 1000);
}

void DHCPConfig::StopExpirationTimeout() {
  lease_expiration_callback_.Cancel();
}

void DHCPConfig::ProcessExpirationTimeout() {
  LOG(ERROR) << "DHCP lease expired on " << device_name()
             << "; restarting DHCP client instance.";
  NotifyExpiry();
  if (!Restart()) {
    NotifyFailure();
  }
}

}  // namespace shill
