// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBWEAVE_INCLUDE_WEAVE_PROVIDER_TEST_MOCK_DNS_SERVICE_DISCOVERY_H_
#define LIBWEAVE_INCLUDE_WEAVE_PROVIDER_TEST_MOCK_DNS_SERVICE_DISCOVERY_H_

#include <weave/provider/dns_service_discovery.h>

#include <string>
#include <vector>

#include <gmock/gmock.h>

namespace weave {
namespace provider {
namespace test {

class MockDnsServiceDiscovery : public DnsServiceDiscovery {
 public:
  MOCK_METHOD3(PublishService,
               void(const std::string&,
                    uint16_t,
                    const std::vector<std::string>&));
  MOCK_METHOD1(StopPublishing, void(const std::string&));
};

}  // namespace test
}  // namespace provider
}  // namespace weave

#endif  // LIBWEAVE_INCLUDE_WEAVE_PROVIDER_TEST_MOCK_DNS_SERVICE_DISCOVERY_H_
