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

#ifndef SHILL_DBUS_CHROMEOS_POWER_MANAGER_PROXY_H_
#define SHILL_DBUS_CHROMEOS_POWER_MANAGER_PROXY_H_

// An implementation of PowerManagerProxyInterface.  It connects to the dbus and
// listens for events from the power manager.  When they occur, the delegate's
// member functions are called.

#include <stdint.h>

#include <string>
#include <vector>

#include <base/compiler_specific.h>
#include <power_manager/dbus-proxies.h>

#include "shill/power_manager_proxy_interface.h"

namespace shill {

class EventDispatcher;

class ChromeosPowerManagerProxy : public PowerManagerProxyInterface {
 public:
  // Constructs a PowerManager DBus object proxy with signals dispatched to
  // |delegate|.
  ChromeosPowerManagerProxy(
      EventDispatcher* dispatcher,
      const scoped_refptr<dbus::Bus>& bus,
      PowerManagerProxyDelegate* delegate,
      const base::Closure& service_appeared_callback,
      const base::Closure& service_vanished_callback);
  ~ChromeosPowerManagerProxy() override;

  // Inherited from PowerManagerProxyInterface.
  bool RegisterSuspendDelay(base::TimeDelta timeout,
                            const std::string& description,
                            int* delay_id_out) override;
  bool UnregisterSuspendDelay(int delay_id) override;
  bool ReportSuspendReadiness(int delay_id, int suspend_id) override;
  bool RegisterDarkSuspendDelay(base::TimeDelta timeout,
                                const std::string& description,
                                int* delay_id_out) override;
  bool UnregisterDarkSuspendDelay(int delay_id) override;
  bool ReportDarkSuspendReadiness(int delay_id, int suspend_id) override;
  bool RecordDarkResumeWakeReason(const std::string& wake_reason) override;

 private:
  // Signal handlers.
  void SuspendImminent(const std::vector<uint8_t>& serialized_proto);
  void SuspendDone(const std::vector<uint8_t>& serialized_proto);
  void DarkSuspendImminent(
      const std::vector<uint8_t>& serialized_proto);

  bool RegisterSuspendDelayInternal(bool is_dark,
                                    base::TimeDelta timeout,
                                    const std::string& description,
                                    int* delay_id_out);
  bool UnregisterSuspendDelayInternal(bool is_dark, int delay_id);
  bool ReportSuspendReadinessInternal(bool is_dark,
                                      int delay_id,
                                      int suspend_id);

  // Called when service appeared or vanished.
  void OnServiceAvailable(bool available);

  // Service name owner changed handler.
  void OnServiceOwnerChanged(const std::string& old_owner,
                             const std::string& new_owner);

  // Called when signal is connected to the ObjectProxy.
  void OnSignalConnected(const std::string& interface_name,
                         const std::string& signal_name,
                         bool success);

  std::unique_ptr<org::chromium::PowerManagerProxy> proxy_;
  EventDispatcher* dispatcher_;
  PowerManagerProxyDelegate* delegate_;
  base::Closure service_appeared_callback_;
  base::Closure service_vanished_callback_;
  bool service_available_;

  base::WeakPtrFactory<ChromeosPowerManagerProxy> weak_factory_{this};
  DISALLOW_COPY_AND_ASSIGN(ChromeosPowerManagerProxy);
};

}  // namespace shill

#endif  // SHILL_DBUS_CHROMEOS_POWER_MANAGER_PROXY_H_
