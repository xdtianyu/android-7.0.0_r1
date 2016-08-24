// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBWEAVE_SRC_PRIVET_WIFI_DELEGATE_H_
#define LIBWEAVE_SRC_PRIVET_WIFI_DELEGATE_H_

#include <memory>
#include <set>
#include <string>

#include "src/privet/privet_types.h"

namespace weave {
namespace privet {

// Interface to provide WiFi functionality for PrivetHandler.
class WifiDelegate {
 public:
  WifiDelegate() = default;
  virtual ~WifiDelegate() {}

  // Returns status of the WiFi connection.
  virtual const ConnectionState& GetConnectionState() const = 0;

  // Returns status of the last WiFi setup.
  virtual const SetupState& GetSetupState() const = 0;

  // Starts WiFi setup. Device should try to connect to provided SSID and
  // password and store them on success. Result of setup should be available
  // using GetSetupState().
  // Final setup state can be retrieved with GetSetupState().
  virtual bool ConfigureCredentials(const std::string& ssid,
                                    const std::string& password,
                                    ErrorPtr* error) = 0;

  // Returns SSID of the currently configured WiFi network. Empty string, if
  // WiFi has not been configured yet.
  virtual std::string GetCurrentlyConnectedSsid() const = 0;

  // Returns SSID of the WiFi network hosted by this device. Empty if device is
  // not in setup or P2P modes.
  virtual std::string GetHostedSsid() const = 0;

  // Returns list of supported WiFi types. Currently it's just frequencies.
  virtual std::set<WifiType> GetTypes() const = 0;
};

}  // namespace privet
}  // namespace weave

#endif  // LIBWEAVE_SRC_PRIVET_WIFI_DELEGATE_H_
