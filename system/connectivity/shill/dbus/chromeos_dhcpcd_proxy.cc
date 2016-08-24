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

#include "shill/dbus/chromeos_dhcpcd_proxy.h"

#include "shill/logging.h"

using std::string;
namespace shill {

namespace Logging {
static auto kModuleLogScope = ScopeLogger::kDHCP;
static string ObjectID(ChromeosDHCPCDProxy* d) { return "(dhcpcd_proxy)"; }
}

ChromeosDHCPCDProxy::ChromeosDHCPCDProxy(const scoped_refptr<dbus::Bus>& bus,
                                         const std::string& service_name)
    : dhcpcd_proxy_(
        new org::chromium::dhcpcdProxy(bus, service_name)) {
  SLOG(this, 2) << "DHCPCDProxy(service=" << service_name << ").";
  // Do not register signal handlers, signals are processed by
  // ChromeosDHCPCDListener.
}

ChromeosDHCPCDProxy::~ChromeosDHCPCDProxy() {
  dhcpcd_proxy_->ReleaseObjectProxy(base::Bind(&base::DoNothing));
}

void ChromeosDHCPCDProxy::Rebind(const string& interface) {
  SLOG(DBus, nullptr, 2) << __func__;
  brillo::ErrorPtr error;
  if (!dhcpcd_proxy_->Rebind(interface, &error)) {
    LogDBusError(error, __func__, interface);
  }
}

void ChromeosDHCPCDProxy::Release(const string& interface) {
  SLOG(DBus, nullptr, 2) << __func__;
  brillo::ErrorPtr error;
  if (!dhcpcd_proxy_->Release(interface, &error)) {
    LogDBusError(error, __func__, interface);
  }
}

void ChromeosDHCPCDProxy::LogDBusError(const brillo::ErrorPtr& error,
                                       const string& method,
                                       const string& interface) {
  if (error->GetCode() == DBUS_ERROR_SERVICE_UNKNOWN ||
      error->GetCode() == DBUS_ERROR_NO_REPLY) {
    LOG(INFO) << method << ": dhcpcd daemon appears to have exited.";
  } else {
    LOG(FATAL) << "DBus error: " << method << " " << interface << ": "
               << error->GetCode() << ": " << error->GetMessage();
  }
}

}  // namespace shill
