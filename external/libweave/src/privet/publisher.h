// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBWEAVE_SRC_PRIVET_PUBLISHER_H_
#define LIBWEAVE_SRC_PRIVET_PUBLISHER_H_

#include <memory>
#include <string>
#include <vector>

#include <base/macros.h>

namespace weave {

namespace provider {
class DnsServiceDiscovery;
}

namespace privet {

class CloudDelegate;
class DeviceDelegate;
class WifiDelegate;

// Publishes privet service on DNS-SD.
class Publisher {
 public:
  Publisher(const DeviceDelegate* device,
            const CloudDelegate* cloud,
            const WifiDelegate* wifi,
            provider::DnsServiceDiscovery* dns_sd);
  ~Publisher();

  // Updates published information.  Removes service if HTTP is not alive.
  void Update();

 private:
  void ExposeService();
  void RemoveService();

  provider::DnsServiceDiscovery* dns_sd_{nullptr};

  const DeviceDelegate* device_{nullptr};
  const CloudDelegate* cloud_{nullptr};
  const WifiDelegate* wifi_{nullptr};

  std::pair<uint16_t, std::vector<std::string>> published_;

  DISALLOW_COPY_AND_ASSIGN(Publisher);
};

}  // namespace privet
}  // namespace weave

#endif  // LIBWEAVE_SRC_PRIVET_PUBLISHER_H_
