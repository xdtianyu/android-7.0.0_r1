// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBWEAVE_INCLUDE_WEAVE_PROVIDER_WIFI_H_
#define LIBWEAVE_INCLUDE_WEAVE_PROVIDER_WIFI_H_

#include <string>

#include <base/callback.h>
#include <weave/error.h>

namespace weave {
namespace provider {

// Interface with methods to control WiFi capability of the device.
class Wifi {
 public:
  // Connects to the given network with the given pass-phrase. Implementation
  // should post either of callbacks.
  virtual void Connect(const std::string& ssid,
                       const std::string& passphrase,
                       const DoneCallback& callback) = 0;

  // Starts WiFi access point for wifi setup.
  virtual void StartAccessPoint(const std::string& ssid) = 0;

  // Stops WiFi access point.
  virtual void StopAccessPoint() = 0;

  virtual bool IsWifi24Supported() const = 0;
  virtual bool IsWifi50Supported() const = 0;

 protected:
  virtual ~Wifi() {}
};

}  // namespace provider
}  // namespace weave

#endif  // LIBWEAVE_INCLUDE_WEAVE_PROVIDER_WIFI_H_
