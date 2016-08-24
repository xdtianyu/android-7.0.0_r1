//
// Copyright (C) 2014 The Android Open Source Project
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

#include "apmanager/service.h"

#include <signal.h>

#include <base/bind.h>
#include <base/strings/stringprintf.h>
#include <brillo/errors/error.h>

#if !defined(__ANDROID__)
#include <chromeos/dbus/service_constants.h>
#else
#include <dbus/apmanager/dbus-constants.h>
#endif  // __ANDROID__

#if defined(__BRILLO__)
#include "apmanager/event_dispatcher.h"
#endif  // __BRILLO__

#include "apmanager/control_interface.h"
#include "apmanager/manager.h"

using std::string;

namespace apmanager {

// static.
#if !defined(__ANDROID__)
const char Service::kHostapdPath[] = "/usr/sbin/hostapd";
const char Service::kHostapdConfigPathFormat[] =
    "/var/run/apmanager/hostapd/hostapd-%d.conf";
const char Service::kHostapdControlInterfacePath[] =
    "/var/run/apmanager/hostapd/ctrl_iface";
#else
const char Service::kHostapdPath[] = "/system/bin/hostapd";
const char Service::kHostapdConfigPathFormat[] =
    "/data/misc/apmanager/hostapd/hostapd-%d.conf";
const char Service::kHostapdControlInterfacePath[] =
    "/data/misc/apmanager/hostapd/ctrl_iface";
#endif  // __ANDROID__

#if defined(__BRILLO__)
const int Service::kAPInterfaceCheckIntervalMilliseconds = 200;
const int Service::kAPInterfaceCheckMaxAttempts = 5;
#endif  // __BRILLO__

const int Service::kTerminationTimeoutSeconds = 2;

// static. Service state definitions.
const char Service::kStateIdle[] = "Idle";
const char Service::kStateStarting[] = "Starting";
const char Service::kStateStarted[] = "Started";
const char Service::kStateFailed[] = "Failed";

Service::Service(Manager* manager, int service_identifier)
    : manager_(manager),
      identifier_(service_identifier),
      config_(new Config(manager, service_identifier)),
      adaptor_(manager->control_interface()->CreateServiceAdaptor(this)),
      dhcp_server_factory_(DHCPServerFactory::GetInstance()),
      file_writer_(FileWriter::GetInstance()),
      process_factory_(ProcessFactory::GetInstance()) {
  adaptor_->SetConfig(config_.get());
  adaptor_->SetState(kStateIdle);
  // TODO(zqiu): come up with better server address management. This is good
  // enough for now.
  config_->SetServerAddressIndex(identifier_ & 0xFF);

#if defined(__BRILLO__)
  event_dispatcher_ = EventDispatcher::GetInstance();
  start_in_progress_ = false;
#endif
}

Service::~Service() {
  // Stop hostapd process if still running.
  if (IsHostapdRunning()) {
    ReleaseResources();
  }
}

bool Service::StartInternal(Error* error) {
  if (IsHostapdRunning()) {
    Error::PopulateAndLog(
        error, Error::kInternalError, "Service already running", FROM_HERE);
    return false;
  }

  // Setup hostapd control interface path.
  config_->set_control_interface(kHostapdControlInterfacePath);

  // Generate hostapd configuration content.
  string config_str;
  if (!config_->GenerateConfigFile(error, &config_str)) {
    return false;
  }

  // Write configuration to a file.
  string config_file_name = base::StringPrintf(kHostapdConfigPathFormat,
                                               identifier_);
  if (!file_writer_->Write(config_file_name, config_str)) {
    Error::PopulateAndLog(error,
                          Error::kInternalError,
                          "Failed to write configuration to a file",
                          FROM_HERE);
    return false;
  }

  // Claim the device needed for this ap service.
  if (!config_->ClaimDevice()) {
    Error::PopulateAndLog(error,
                          Error::kInternalError,
                          "Failed to claim the device for this service",
                          FROM_HERE);
    return false;
  }

  // Start hostapd process.
  if (!StartHostapdProcess(config_file_name)) {
    Error::PopulateAndLog(
        error, Error::kInternalError, "Failed to start hostapd", FROM_HERE);
    // Release the device claimed for this service.
    config_->ReleaseDevice();
    return false;
  }

  // Start DHCP server if in server mode.
  if (config_->GetOperationMode() == kOperationModeServer) {
    dhcp_server_.reset(
        dhcp_server_factory_->CreateDHCPServer(config_->GetServerAddressIndex(),
                                               config_->selected_interface()));
    if (!dhcp_server_->Start()) {
      Error::PopulateAndLog(error,
                            Error::kInternalError,
                            "Failed to start DHCP server",
                            FROM_HERE);
      ReleaseResources();
      return false;
    }
    manager_->RequestDHCPPortAccess(config_->selected_interface());
  }

  // Start monitoring hostapd.
  if (!hostapd_monitor_) {
    hostapd_monitor_.reset(
        new HostapdMonitor(base::Bind(&Service::HostapdEventCallback,
                                      weak_factory_.GetWeakPtr()),
                           config_->control_interface(),
                           config_->selected_interface()));
  }
  hostapd_monitor_->Start();

  // Update service state.
  adaptor_->SetState(kStateStarting);

  return true;
}

void Service::Start(const base::Callback<void(const Error&)>& result_callback) {
  Error error;

#if !defined(__BRILLO__)
  StartInternal(&error);
  result_callback.Run(error);
#else
  if (start_in_progress_) {
    Error::PopulateAndLog(
        &error, Error::kInternalError, "Start already in progress", FROM_HERE);
    result_callback.Run(error);
    return;
  }

  string interface_name;
  if (!manager_->SetupApModeInterface(&interface_name)) {
    Error::PopulateAndLog(&error,
                          Error::kInternalError,
                          "Failed to setup AP mode interface",
                          FROM_HERE);
    result_callback.Run(error);
    return;
  }

  event_dispatcher_->PostDelayedTask(
      base::Bind(&Service::APInterfaceCheckTask,
                 weak_factory_.GetWeakPtr(),
                 interface_name,
                 0,    // Initial check count.
                 result_callback),
      kAPInterfaceCheckIntervalMilliseconds);
#endif
}

bool Service::Stop(Error* error) {
  if (!IsHostapdRunning()) {
    Error::PopulateAndLog(error,
                          Error::kInternalError,
                          "Service is not currently running", FROM_HERE);
    return false;
  }

  ReleaseResources();
  adaptor_->SetState(kStateIdle);
  return true;
}

#if defined(__BRILLO__)
void Service::HandleStartFailure() {
  // Restore station mode interface.
  string station_mode_interface;
  manager_->SetupStationModeInterface(&station_mode_interface);

  // Reset state variables.
  start_in_progress_ = false;
}

void Service::APInterfaceCheckTask(
    const string& interface_name,
    int check_count,
    const base::Callback<void(const Error&)>& result_callback) {
  Error error;

  // Check if the AP interface is enumerated.
  if (manager_->GetDeviceFromInterfaceName(interface_name)) {
    // Explicitly set the interface name to avoid picking other interface.
    config_->SetInterfaceName(interface_name);
    if (!StartInternal(&error)) {
      HandleStartFailure();
    }
    result_callback.Run(error);
    return;
  }

  check_count++;
  if (check_count >= kAPInterfaceCheckMaxAttempts) {
    Error::PopulateAndLog(&error,
                          Error::kInternalError,
                          "Timeout waiting for AP interface to be enumerated",
                          FROM_HERE);
    HandleStartFailure();
    result_callback.Run(error);
    return;
  }

  event_dispatcher_->PostDelayedTask(
      base::Bind(&Service::APInterfaceCheckTask,
                 weak_factory_.GetWeakPtr(),
                 interface_name,
                 check_count,
                 result_callback),
      kAPInterfaceCheckIntervalMilliseconds);
}
#endif  // __BRILLO__

bool Service::IsHostapdRunning() {
  return hostapd_process_ && hostapd_process_->pid() != 0 &&
         brillo::Process::ProcessExists(hostapd_process_->pid());
}

bool Service::StartHostapdProcess(const string& config_file_path) {
  hostapd_process_.reset(process_factory_->CreateProcess());
  hostapd_process_->AddArg(kHostapdPath);
  hostapd_process_->AddArg(config_file_path);
  if (!hostapd_process_->Start()) {
    hostapd_process_.reset();
    return false;
  }
  return true;
}

void Service::StopHostapdProcess() {
  if (!hostapd_process_->Kill(SIGTERM, kTerminationTimeoutSeconds)) {
    hostapd_process_->Kill(SIGKILL, kTerminationTimeoutSeconds);
  }
  hostapd_process_.reset();
}

void Service::ReleaseResources() {
  hostapd_monitor_.reset();
  StopHostapdProcess();
  dhcp_server_.reset();
  manager_->ReleaseDHCPPortAccess(config_->selected_interface());
#if defined(__BRILLO__)
  // Restore station mode interface.
  string station_mode_interface;
  manager_->SetupStationModeInterface(&station_mode_interface);
#endif  // __BRILLO__
  // Only release device after mode switching had completed, to
  // make sure the station mode interface gets enumerated by
  // shill.
  config_->ReleaseDevice();
}

void Service::HostapdEventCallback(HostapdMonitor::Event event,
                                   const std::string& data) {
  switch (event) {
    case HostapdMonitor::kHostapdFailed:
      adaptor_->SetState(kStateFailed);
      break;
    case HostapdMonitor::kHostapdStarted:
      adaptor_->SetState(kStateStarted);
      break;
    case HostapdMonitor::kStationConnected:
      LOG(INFO) << "Station connected: " << data;
      break;
    case HostapdMonitor::kStationDisconnected:
      LOG(INFO) << "Station disconnected: " << data;
      break;
    default:
      LOG(ERROR) << "Unknown event: " << event;
      break;
  }
}

}  // namespace apmanager
