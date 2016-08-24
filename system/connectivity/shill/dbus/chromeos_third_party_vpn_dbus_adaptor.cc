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

#include "shill/dbus/chromeos_third_party_vpn_dbus_adaptor.h"

#include <base/logging.h>
#if defined(__ANDROID__)
#include <dbus/service_constants.h>
#else
#include <chromeos/dbus/service_constants.h>
#endif  // __ANDROID__

#include "shill/logging.h"
#include "shill/service.h"
#include "shill/vpn/third_party_vpn_driver.h"

using brillo::dbus_utils::AsyncEventSequencer;
using brillo::dbus_utils::ExportedObjectManager;

namespace shill {

namespace Logging {

static auto kModuleLogScope = ScopeLogger::kVPN;
static std::string ObjectID(const ChromeosThirdPartyVpnDBusAdaptor* v) {
  return "(third_party_vpn_dbus_adaptor)";
}

}  // namespace Logging

namespace {

// The API converts external connection state to internal one
bool ConvertConnectState(
    ChromeosThirdPartyVpnDBusAdaptor::ExternalConnectState external_state,
    Service::ConnectState* internal_state) {
  switch (external_state) {
    case ChromeosThirdPartyVpnDBusAdaptor::kStateConnected:
      *internal_state = Service::kStateOnline;
      break;
    case ChromeosThirdPartyVpnDBusAdaptor::kStateFailure:
      *internal_state = Service::kStateFailure;
      break;
    default:
      return false;
  }
  return true;
}

}  // namespace

ChromeosThirdPartyVpnDBusAdaptor::ChromeosThirdPartyVpnDBusAdaptor(
    const scoped_refptr<dbus::Bus>& bus,
    ThirdPartyVpnDriver* client)
    : org::chromium::flimflam::ThirdPartyVpnAdaptor(this),
      ChromeosDBusAdaptor(bus,
                          kObjectPathBase + client->object_path_suffix()),
      client_(client) {
  // Register DBus object.
  RegisterWithDBusObject(dbus_object());
  dbus_object()->RegisterAndBlock();
}

ChromeosThirdPartyVpnDBusAdaptor::~ChromeosThirdPartyVpnDBusAdaptor() {
  dbus_object()->UnregisterAsync();
}

void ChromeosThirdPartyVpnDBusAdaptor::EmitPacketReceived(
    const std::vector<uint8_t>& packet) {
  SLOG(this, 2) << __func__;
  SendOnPacketReceivedSignal(packet);
}

void ChromeosThirdPartyVpnDBusAdaptor::EmitPlatformMessage(uint32_t message) {
  SLOG(this, 2) << __func__ << "(" << message << ")";
  SendOnPlatformMessageSignal(message);
}

bool ChromeosThirdPartyVpnDBusAdaptor::SetParameters(
    brillo::ErrorPtr* error,
    const std::map<std::string, std::string>& parameters,
    std::string* warning_message) {
  SLOG(this, 2) << __func__;
  std::string error_message;
  Error e;
  client_->SetParameters(parameters, &error_message, warning_message);
  if (!error_message.empty()) {
    e.Populate(Error::kInvalidArguments, error_message);
  }
  return !e.ToChromeosError(error);
}

bool ChromeosThirdPartyVpnDBusAdaptor::UpdateConnectionState(
    brillo::ErrorPtr* error, uint32_t connection_state) {
  SLOG(this, 2) << __func__ << "(" << connection_state << ")";
  // Externally supported states are from Service::kStateConnected to
  // Service::kStateOnline.
  Service::ConnectState internal_state;
  std::string error_message;
  Error e;
  if (ConvertConnectState(static_cast<ExternalConnectState>(connection_state),
                          &internal_state)) {
    client_->UpdateConnectionState(internal_state, &error_message);
    if (!error_message.empty()) {
      e.Populate(Error::kInvalidArguments, error_message);
    }
  } else {
    e.Populate(Error::kNotSupported, "Connection state is not supported");
  }
  return !e.ToChromeosError(error);
}

bool ChromeosThirdPartyVpnDBusAdaptor::SendPacket(
    brillo::ErrorPtr* error,
    const std::vector<uint8_t>& ip_packet) {
  SLOG(this, 2) << __func__;
  std::string error_message;
  client_->SendPacket(ip_packet, &error_message);
  Error e;
  if (!error_message.empty()) {
    e.Populate(Error::kWrongState, error_message);
  }
  return !e.ToChromeosError(error);
}

}  // namespace shill
