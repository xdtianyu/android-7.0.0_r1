//
// Copyright (C) 2013 The Android Open Source Project
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

#include "shill/ppp_device.h"

#include <map>
#include <string>

#include <gtest/gtest.h>

#include "shill/metrics.h"
#include "shill/mock_control.h"
#include "shill/mock_metrics.h"
#include "shill/mock_ppp_device.h"

using std::map;
using std::string;

namespace shill {

// TODO(quiche): Add test for UpdateIPConfigFromPPP. crbug.com/266404

TEST(PPPDeviceTest, GetInterfaceName) {
  map<string, string> config;
  config[kPPPInterfaceName] = "ppp0";
  config["foo"] = "bar";
  EXPECT_EQ("ppp0", PPPDevice::GetInterfaceName(config));
}

TEST(PPPDeviceTest, ParseIPConfiguration) {
  MockControl control;
  MockMetrics metrics(nullptr);
  scoped_refptr<PPPDevice> device = new PPPDevice(&control, nullptr, &metrics,
                                                  nullptr, "test0", 0);

  map<string, string> config;
  config[kPPPInternalIP4Address] = "4.5.6.7";
  config[kPPPExternalIP4Address] = "33.44.55.66";
  config[kPPPGatewayAddress] = "192.168.1.1";
  config[kPPPDNS1] = "1.1.1.1";
  config[kPPPDNS2] = "2.2.2.2";
  config[kPPPInterfaceName] = "ppp0";
  config[kPPPLNSAddress] = "99.88.77.66";
  config[kPPPMRU] = "1492";
  config["foo"] = "bar";  // Unrecognized keys don't cause crash.
  EXPECT_CALL(metrics, SendSparseToUMA(Metrics::kMetricPPPMTUValue, 1492));
  IPConfig::Properties props = device->ParseIPConfiguration("in-test", config);
  EXPECT_EQ(IPAddress::kFamilyIPv4, props.address_family);
  EXPECT_EQ(IPAddress::GetMaxPrefixLength(IPAddress::kFamilyIPv4),
            props.subnet_prefix);
  EXPECT_EQ("4.5.6.7", props.address);
  EXPECT_EQ("33.44.55.66", props.peer_address);
  EXPECT_EQ("192.168.1.1", props.gateway);
  ASSERT_EQ(2, props.dns_servers.size());
  EXPECT_EQ("1.1.1.1", props.dns_servers[0]);
  EXPECT_EQ("2.2.2.2", props.dns_servers[1]);
  EXPECT_EQ("99.88.77.66/32", props.exclusion_list[0]);
  EXPECT_EQ(1, props.exclusion_list.size());
  EXPECT_EQ(1492, props.mtu);

  // No gateway specified.
  config.erase(kPPPGatewayAddress);
  EXPECT_CALL(metrics, SendSparseToUMA(Metrics::kMetricPPPMTUValue, 1492));
  IPConfig::Properties props2 = device->ParseIPConfiguration("in-test", config);
  EXPECT_EQ("33.44.55.66", props2.gateway);
}

}  // namespace shill
