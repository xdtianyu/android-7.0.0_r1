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

#ifndef APMANAGER_SERVICE_H_
#define APMANAGER_SERVICE_H_

#include <string>

#include <base/callback.h>
#include <base/macros.h>
#include <base/memory/ref_counted.h>
#include <base/memory/weak_ptr.h>
#include <brillo/process.h>

#include "apmanager/config.h"
#include "apmanager/dhcp_server_factory.h"
#include "apmanager/error.h"
#include "apmanager/file_writer.h"
#include "apmanager/hostapd_monitor.h"
#include "apmanager/process_factory.h"
#include "apmanager/service_adaptor_interface.h"

namespace apmanager {

class Manager;

#if defined(__BRILLO__)
class EventDispatcher;
#endif  // __BRILLO__

class Service : public base::RefCounted<Service> {
 public:
  Service(Manager* manager, int service_identifier);
  virtual ~Service();

  void Start(const base::Callback<void(const Error&)>& result_callback);
  bool Stop(Error* error);

  int identifier() const { return identifier_; }

  ServiceAdaptorInterface* adaptor() const { return adaptor_.get(); }

  Config* config() const { return config_.get(); }

 private:
  friend class ServiceTest;

  static const char kHostapdPath[];
  static const char kHostapdConfigPathFormat[];
  static const char kHostapdControlInterfacePath[];
  static const int kTerminationTimeoutSeconds;
  static const char kStateIdle[];
  static const char kStateStarting[];
  static const char kStateStarted[];
  static const char kStateFailed[];

#if defined(__BRILLO__)
  static const int kAPInterfaceCheckIntervalMilliseconds;
  static const int kAPInterfaceCheckMaxAttempts;

  // Task to check enumeration status of the specified AP interface
  // |interface_name|.
  void APInterfaceCheckTask(
      const std::string& interface_name,
      int check_count,
      const base::Callback<void(const Error&)>& result_callback);

  // Handle asynchronous service start failures.
  void HandleStartFailure();
#endif  // __BRILLO__

  bool StartInternal(Error* error);

  // Return true if hostapd process is currently running.
  bool IsHostapdRunning();

  // Start hostapd process. Return true if process is created/started
  // successfully, false otherwise.
  bool StartHostapdProcess(const std::string& config_file_path);

  // Stop the running hostapd process. Sending it a SIGTERM signal first, then
  // a SIGKILL if failed to terminated with SIGTERM.
  void StopHostapdProcess();

  // Release resources allocated to this service.
  void ReleaseResources();

  void HostapdEventCallback(HostapdMonitor::Event event,
                            const std::string& data);

  Manager* manager_;
  int identifier_;
  std::unique_ptr<Config> config_;
  std::unique_ptr<ServiceAdaptorInterface> adaptor_;
  std::unique_ptr<brillo::Process> hostapd_process_;
  std::unique_ptr<DHCPServer> dhcp_server_;
  DHCPServerFactory* dhcp_server_factory_;
  FileWriter* file_writer_;
  ProcessFactory* process_factory_;
  std::unique_ptr<HostapdMonitor> hostapd_monitor_;
#if defined(__BRILLO__)
  EventDispatcher* event_dispatcher_;
  bool start_in_progress_;
#endif  // __BRILLO__

  base::WeakPtrFactory<Service> weak_factory_{this};
  DISALLOW_COPY_AND_ASSIGN(Service);
};

}  // namespace apmanager

#endif  // APMANAGER_SERVICE_H_
