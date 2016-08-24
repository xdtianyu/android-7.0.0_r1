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

#include "shill/ethernet/ethernet_service.h"

#include <netinet/ether.h>
#if defined(__ANDROID__)
#include <net/if.h>
#else
#include <linux/if.h>  // NOLINT - Needs definitions from netinet/ether.h
#endif  // __ANDROID__
#include <stdio.h>
#include <time.h>

#include <string>

#include <base/strings/stringprintf.h>
#if defined(__ANDROID__)
#include <dbus/service_constants.h>
#else
#include <chromeos/dbus/service_constants.h>
#endif  // __ANDROID__

#include "shill/control_interface.h"
#include "shill/device.h"
#include "shill/device_info.h"
#include "shill/eap_credentials.h"
#include "shill/ethernet/ethernet.h"
#include "shill/event_dispatcher.h"
#include "shill/manager.h"
#include "shill/profile.h"

using std::string;

namespace shill {

// static
const char EthernetService::kAutoConnNoCarrier[] = "no carrier";
const char EthernetService::kServiceType[] = "ethernet";

EthernetService::EthernetService(ControlInterface* control_interface,
                                 EventDispatcher* dispatcher,
                                 Metrics* metrics,
                                 Manager* manager,
                                 base::WeakPtr<Ethernet> ethernet)
    : EthernetService(control_interface, dispatcher, metrics, manager,
                      Technology::kEthernet, ethernet) {
  SetConnectable(true);
  SetAutoConnect(true);
  set_friendly_name("Ethernet");
  SetStrength(kStrengthMax);

  // Now that |this| is a fully constructed EthernetService, synchronize
  // observers with our current state, and emit the appropriate change
  // notifications. (Initial observer state may have been set in our base
  // class.)
  NotifyPropertyChanges();
}

EthernetService::EthernetService(ControlInterface* control_interface,
                                 EventDispatcher* dispatcher,
                                 Metrics* metrics,
                                 Manager* manager,
                                 Technology::Identifier technology,
                                 base::WeakPtr<Ethernet> ethernet)
  : Service(control_interface, dispatcher, metrics, manager, technology),
    ethernet_(ethernet) {}

EthernetService::~EthernetService() { }

void EthernetService::Connect(Error* error, const char* reason) {
  Service::Connect(error, reason);
  CHECK(ethernet_);
  ethernet_->ConnectTo(this);
}

void EthernetService::Disconnect(Error* error, const char* reason) {
  Service::Disconnect(error, reason);
  CHECK(ethernet_);
  ethernet_->DisconnectFrom(this);
}

std::string EthernetService::GetDeviceRpcId(Error* /*error*/) const {
  CHECK(ethernet_);
  return ethernet_->GetRpcIdentifier();
}

string EthernetService::GetStorageIdentifier() const {
  CHECK(ethernet_);
  return base::StringPrintf(
      "%s_%s", Technology::NameFromIdentifier(technology()).c_str(),
      ethernet_->address().c_str());
}

bool EthernetService::IsAutoConnectByDefault() const {
  return true;
}

bool EthernetService::SetAutoConnectFull(const bool& connect,
                                         Error* error) {
  if (!connect) {
    Error::PopulateAndLog(
        FROM_HERE, error, Error::kInvalidArguments,
        "Auto-connect on Ethernet services must not be disabled.");
    return false;
  }
  return Service::SetAutoConnectFull(connect, error);
}

void EthernetService::Remove(Error* error) {
  error->Populate(Error::kNotSupported);
}

bool EthernetService::IsVisible() const {
  CHECK(ethernet_);
  return ethernet_->link_up();
}

bool EthernetService::IsAutoConnectable(const char** reason) const {
  if (!Service::IsAutoConnectable(reason)) {
    return false;
  }
  CHECK(ethernet_);
  if (!ethernet_->link_up()) {
    *reason = kAutoConnNoCarrier;
    return false;
  }
  return true;
}

void EthernetService::OnVisibilityChanged() {
  NotifyPropertyChanges();
}

string EthernetService::GetTethering(Error* /*error*/) const {
  CHECK(ethernet_);
  return ethernet_->IsConnectedViaTether() ? kTetheringConfirmedState :
      kTetheringNotDetectedState;
}

}  // namespace shill
