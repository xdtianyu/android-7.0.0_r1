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

#ifndef SHILL_DBUS_CHROMEOS_DHCPCD_LISTENER_H_
#define SHILL_DBUS_CHROMEOS_DHCPCD_LISTENER_H_

#include <string>

#include <dbus/dbus.h>

#include <base/macros.h>
#include <base/memory/ref_counted.h>
#include <base/memory/weak_ptr.h>
#include <dbus/bus.h>
#include <dbus/message.h>

#include <brillo/variant_dictionary.h>

#include "shill/dhcp/dhcpcd_listener_interface.h"

namespace shill {

class DHCPProvider;
class EventDispatcher;

// The DHCPCD listener is a singleton proxy that listens to signals from all
// DHCP clients and dispatches them through the DHCP provider to the appropriate
// client based on the PID.
class ChromeosDHCPCDListener final : public DHCPCDListenerInterface {
 public:
  ChromeosDHCPCDListener(const scoped_refptr<dbus::Bus>& bus,
                         EventDispatcher* dispatcher,
                         DHCPProvider* provider);
  ~ChromeosDHCPCDListener() override;

 private:
  static const char kDBusInterfaceName[];
  static const char kSignalEvent[];
  static const char kSignalStatusChanged[];

  // Redirects the function call to HandleMessage
  static DBusHandlerResult HandleMessageThunk(DBusConnection* connection,
                                              DBusMessage* raw_message,
                                              void* user_data);

  // Handles incoming messages.
  DBusHandlerResult HandleMessage(DBusConnection* connection,
                                  DBusMessage* raw_message);

  // Signal handlers.
  void EventSignal(const std::string& sender,
                   uint32_t pid,
                   const std::string& reason,
                   const brillo::VariantDictionary& configurations);
  void StatusChangedSignal(const std::string& sender,
                           uint32_t pid,
                           const std::string& status);

  scoped_refptr<dbus::Bus> bus_;
  EventDispatcher* dispatcher_;
  DHCPProvider* provider_;
  const std::string match_rule_;

  base::WeakPtrFactory<ChromeosDHCPCDListener> weak_factory_{this};
  DISALLOW_COPY_AND_ASSIGN(ChromeosDHCPCDListener);
};

}  // namespace shill

#endif  // SHILL_DBUS_CHROMEOS_DHCPCD_LISTENER_H_
