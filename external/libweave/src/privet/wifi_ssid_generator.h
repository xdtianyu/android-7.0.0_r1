// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBWEAVE_SRC_PRIVET_WIFI_SSID_GENERATOR_H_
#define LIBWEAVE_SRC_PRIVET_WIFI_SSID_GENERATOR_H_

#include <string>

#include <base/callback.h>

namespace weave {
namespace privet {

class CloudDelegate;
class WifiDelegate;

class WifiSsidGenerator final {
 public:
  WifiSsidGenerator(const CloudDelegate* gcd, const WifiDelegate* wifi);
  ~WifiSsidGenerator() = default;

  std::string GenerateFlags() const;

  // Can return empty string if CloudDelegate is not ready.
  std::string GenerateSsid() const;

 private:
  friend class WifiSsidGeneratorTest;

  // Sets object to use |n| instead of random number for SSID generation.
  void SetRandomForTests(int n);
  std::string GenerateFlagsInternal() const;

  const CloudDelegate* gcd_{nullptr};
  const WifiDelegate* wifi_{nullptr};

  base::Callback<int(void)> get_random_;

  DISALLOW_COPY_AND_ASSIGN(WifiSsidGenerator);
};

}  // namespace privet
}  // namespace weave

#endif  // LIBWEAVE_SRC_PRIVET_WIFI_SSID_GENERATOR_H_
