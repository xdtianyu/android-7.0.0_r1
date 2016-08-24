// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBWEAVE_INCLUDE_WEAVE_PROVIDER_TEST_MOCK_WIFI_H_
#define LIBWEAVE_INCLUDE_WEAVE_PROVIDER_TEST_MOCK_WIFI_H_

#include <weave/provider/network.h>

#include <string>

#include <gmock/gmock.h>

namespace weave {
namespace provider {
namespace test {

class MockWifi : public Wifi {
 public:
  MOCK_METHOD3(Connect,
               void(const std::string&,
                    const std::string&,
                    const DoneCallback&));
  MOCK_METHOD1(StartAccessPoint, void(const std::string&));
  MOCK_METHOD0(StopAccessPoint, void());
  MOCK_CONST_METHOD0(IsWifi24Supported, bool());
  MOCK_CONST_METHOD0(IsWifi50Supported, bool());
};

}  // namespace test
}  // namespace provider
}  // namespace weave

#endif  // LIBWEAVE_INCLUDE_WEAVE_PROVIDER_TEST_MOCK_WIFI_H_
