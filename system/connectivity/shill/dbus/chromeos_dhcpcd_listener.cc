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

#include "shill/dbus/chromeos_dhcpcd_listener.h"

#include <string.h>

#include <base/bind.h>
#include <base/callback.h>
#include <base/strings/stringprintf.h>
#include <brillo/dbus/dbus_method_invoker.h>
#include <dbus/util.h>

#include "shill/dhcp/dhcp_config.h"
#include "shill/dhcp/dhcp_provider.h"
#include "shill/event_dispatcher.h"
#include "shill/logging.h"

using std::string;

namespace shill {

namespace Logging {
static auto kModuleLogScope = ScopeLogger::kDHCP;
static string ObjectID(ChromeosDHCPCDListener* d) {
  return "(dhcpcd_listener)";
}
}

const char ChromeosDHCPCDListener::kDBusInterfaceName[] = "org.chromium.dhcpcd";
const char ChromeosDHCPCDListener::kSignalEvent[] = "Event";
const char ChromeosDHCPCDListener::kSignalStatusChanged[] = "StatusChanged";

ChromeosDHCPCDListener::ChromeosDHCPCDListener(
    const scoped_refptr<dbus::Bus>& bus,
    EventDispatcher* dispatcher,
    DHCPProvider* provider)
    : bus_(bus),
      dispatcher_(dispatcher),
      provider_(provider),
      match_rule_(base::StringPrintf("type='signal', interface='%s'",
                                     kDBusInterfaceName)) {
  bus_->AssertOnDBusThread();
  CHECK(bus_->SetUpAsyncOperations());
  if (!bus_->is_connected()) {
    LOG(FATAL) << "DBus isn't connected.";
  }

  // Register filter function to the bus.  It will be called when incoming
  // messages are received.
  bus_->AddFilterFunction(&ChromeosDHCPCDListener::HandleMessageThunk, this);

  // Add match rule to the bus.
  dbus::ScopedDBusError error;
  bus_->AddMatch(match_rule_, error.get());
  if (error.is_set()) {
    LOG(FATAL) << "Failed to add match rule: " << error.name() << " "
               << error.message();
  }
}

ChromeosDHCPCDListener::~ChromeosDHCPCDListener() {
  bus_->RemoveFilterFunction(&ChromeosDHCPCDListener::HandleMessageThunk, this);
  dbus::ScopedDBusError error;
  bus_->RemoveMatch(match_rule_, error.get());
  if (error.is_set()) {
    LOG(FATAL) << "Failed to remove match rule: " << error.name() << " "
               << error.message();
  }
}

// static.
DBusHandlerResult ChromeosDHCPCDListener::HandleMessageThunk(
    DBusConnection* connection, DBusMessage* raw_message, void* user_data) {
  ChromeosDHCPCDListener* self =
      static_cast<ChromeosDHCPCDListener*>(user_data);
  return self->HandleMessage(connection, raw_message);
}

DBusHandlerResult ChromeosDHCPCDListener::HandleMessage(
    DBusConnection* connection, DBusMessage* raw_message) {
  bus_->AssertOnDBusThread();

  // Only interested in signal message.
  if (dbus_message_get_type(raw_message) != DBUS_MESSAGE_TYPE_SIGNAL) {
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  }

  // raw_message will be unrefed in Signal's parent class's (dbus::Message)
  // destructor. Increment the reference so we can use it in Signal.
  dbus_message_ref(raw_message);
  std::unique_ptr<dbus::Signal> signal(
      dbus::Signal::FromRawMessage(raw_message));

  // Verify the signal comes from the interface that we interested in.
  if (signal->GetInterface() != kDBusInterfaceName) {
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  }

  string sender = signal->GetSender();
  string member_name = signal->GetMember();
  dbus::MessageReader reader(signal.get());
  if (member_name == kSignalEvent) {
    uint32_t pid;
    string reason;
    brillo::VariantDictionary configurations;
    // ExtracMessageParameters will log the error if it failed.
    if (brillo::dbus_utils::ExtractMessageParameters(&reader,
                                                     nullptr,
                                                     &pid,
                                                     &reason,
                                                     &configurations)) {
      dispatcher_->PostTask(
          base::Bind(&ChromeosDHCPCDListener::EventSignal,
                     weak_factory_.GetWeakPtr(),
                     sender, pid, reason, configurations));
    }
  } else if (member_name == kSignalStatusChanged) {
    uint32_t pid;
    string status;
    // ExtracMessageParameters will log the error if it failed.
    if (brillo::dbus_utils::ExtractMessageParameters(&reader,
                                                     nullptr,
                                                     &pid,
                                                     &status)) {
      dispatcher_->PostTask(
          base::Bind(&ChromeosDHCPCDListener::StatusChangedSignal,
                     weak_factory_.GetWeakPtr(),
                     sender, pid, status));
    }
  } else {
    LOG(INFO) << "Ignore signal: " << member_name;
  }

  return DBUS_HANDLER_RESULT_HANDLED;
}

void ChromeosDHCPCDListener::EventSignal(
    const string& sender,
    uint32_t pid,
    const string& reason,
    const brillo::VariantDictionary& configuration) {
  DHCPConfigRefPtr config = provider_->GetConfig(pid);
  if (!config.get()) {
    if (provider_->IsRecentlyUnbound(pid)) {
      SLOG(nullptr, 3)
          << __func__ << ": ignoring message from recently unbound PID " << pid;
    } else {
      LOG(ERROR) << "Unknown DHCP client PID " << pid;
    }
    return;
  }
  config->InitProxy(sender);
  KeyValueStore configuration_store;
  KeyValueStore::ConvertFromVariantDictionary(configuration,
                                              &configuration_store);
  config->ProcessEventSignal(reason, configuration_store);
}

void ChromeosDHCPCDListener::StatusChangedSignal(const string& sender,
                                                 uint32_t pid,
                                                 const string& status) {
  DHCPConfigRefPtr config = provider_->GetConfig(pid);
  if (!config.get()) {
    if (provider_->IsRecentlyUnbound(pid)) {
      SLOG(nullptr, 3)
          << __func__ << ": ignoring message from recently unbound PID " << pid;
    } else {
      LOG(ERROR) << "Unknown DHCP client PID " << pid;
    }
    return;
  }
  config->InitProxy(sender);
  config->ProcessStatusChangeSignal(status);
}

}  // namespace shill
