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

#ifndef SHILL_PPP_DEVICE_H_
#define SHILL_PPP_DEVICE_H_

#include <base/macros.h>

#include <map>
#include <string>

#include "shill/ipconfig.h"
#include "shill/virtual_device.h"

namespace shill {

// Declared in the header to avoid linking unused code into shims.
static const char kPPPDNS1[] = "DNS1";
static const char kPPPDNS2[] = "DNS2";
static const char kPPPExternalIP4Address[] = "EXTERNAL_IP4_ADDRESS";
static const char kPPPGatewayAddress[] = "GATEWAY_ADDRESS";
static const char kPPPInterfaceName[] = "INTERNAL_IFNAME";
static const char kPPPInternalIP4Address[] = "INTERNAL_IP4_ADDRESS";
static const char kPPPLNSAddress[] = "LNS_ADDRESS";
static const char kPPPMRU[] = "MRU";
static const char kPPPReasonAuthenticated[] = "authenticated";
static const char kPPPReasonAuthenticating[] = "authenticating";
static const char kPPPReasonConnect[] = "connect";
static const char kPPPReasonDisconnect[] = "disconnect";

class PPPDevice : public VirtualDevice {
 public:
  PPPDevice(ControlInterface* control,
            EventDispatcher* dispatcher,
            Metrics* metrics,
            Manager* manager,
            const std::string& link_name,
            int interface_index);
  ~PPPDevice() override;

  // Set IPConfig for this device, based on the dictionary of
  // configuration strings received from our PPP plugin.
  virtual void UpdateIPConfigFromPPP(
      const std::map<std::string, std::string>& configuration,
      bool blackhole_ipv6);

  // Same as UpdateIPConfigFromPPP except overriding the default MTU
  // in the IPConfig.
  virtual void UpdateIPConfigFromPPPWithMTU(
      const std::map<std::string, std::string>& configuration,
      bool blackhole_ipv6,
      int32_t mtu);

#ifndef DISABLE_DHCPV6
  // Start a DHCPv6 configuration client for this device.  The generic
  // file name (based on the device name) will be used for the acquired
  // lease, so that the lease file will be removed when the DHCPv6 client
  // terminates.  For PPP devices, there is no correlation between
  // the service name and the network that it connected to.
  virtual bool AcquireIPv6Config();
#endif  // DISABLE_DHCPV6

  // Get the network device name (e.g. "ppp0") from the dictionary of
  // configuration strings received from our PPP plugin.
  static std::string GetInterfaceName(
      const std::map<std::string, std::string>& configuration);

 private:
  FRIEND_TEST(PPPDeviceTest, GetInterfaceName);
  FRIEND_TEST(PPPDeviceTest, ParseIPConfiguration);

  IPConfig::Properties ParseIPConfiguration(
      const std::string& link_name,
      const std::map<std::string, std::string>& configuration);

  DISALLOW_COPY_AND_ASSIGN(PPPDevice);
};

}  // namespace shill

#endif  // SHILL_PPP_DEVICE_H_
