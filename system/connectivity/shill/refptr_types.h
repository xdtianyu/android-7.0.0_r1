//
// Copyright (C) 2012 The Android Open Source Project
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

#ifndef SHILL_REFPTR_TYPES_H_
#define SHILL_REFPTR_TYPES_H_

#include <base/memory/ref_counted.h>

namespace shill {

class Device;
typedef scoped_refptr<const Device> DeviceConstRefPtr;
typedef scoped_refptr<Device> DeviceRefPtr;

class Cellular;
typedef scoped_refptr<const Cellular> CellularConstRefPtr;
typedef scoped_refptr<Cellular> CellularRefPtr;

class Ethernet;
typedef scoped_refptr<const Ethernet> EthernetConstRefPtr;
typedef scoped_refptr<Ethernet> EthernetRefPtr;

class PPPDevice;
typedef scoped_refptr<const PPPDevice> PPPDeviceConstRefPtr;
typedef scoped_refptr<PPPDevice> PPPDeviceRefPtr;

class VirtualDevice;
typedef scoped_refptr<const VirtualDevice> VirtualDeviceConstRefPtr;
typedef scoped_refptr<VirtualDevice> VirtualDeviceRefPtr;

class WiFi;
typedef scoped_refptr<const WiFi> WiFiConstRefPtr;
typedef scoped_refptr<WiFi> WiFiRefPtr;

class WiMax;
typedef scoped_refptr<const WiMax> WiMaxConstRefPtr;
typedef scoped_refptr<WiMax> WiMaxRefPtr;

class WiFiEndpoint;
typedef scoped_refptr<const WiFiEndpoint> WiFiEndpointConstRefPtr;
typedef scoped_refptr<WiFiEndpoint> WiFiEndpointRefPtr;

class Service;
typedef scoped_refptr<const Service> ServiceConstRefPtr;
typedef scoped_refptr<Service> ServiceRefPtr;

class CellularService;
typedef scoped_refptr<const CellularService> CellularServiceConstRefPtr;
typedef scoped_refptr<CellularService> CellularServiceRefPtr;

class EthernetService;
typedef scoped_refptr<const EthernetService> EthernetServiceConstRefPtr;
typedef scoped_refptr<EthernetService> EthernetServiceRefPtr;

class VPNService;
typedef scoped_refptr<const VPNService> VPNServiceConstRefPtr;
typedef scoped_refptr<VPNService> VPNServiceRefPtr;

class WiFiService;
typedef scoped_refptr<const WiFiService> WiFiServiceConstRefPtr;
typedef scoped_refptr<WiFiService> WiFiServiceRefPtr;

class WiMaxService;
typedef scoped_refptr<const WiMaxService> WiMaxServiceConstRefPtr;
typedef scoped_refptr<WiMaxService> WiMaxServiceRefPtr;

class IPConfig;
typedef scoped_refptr<IPConfig> IPConfigRefPtr;

class DHCPConfig;
typedef scoped_refptr<DHCPConfig> DHCPConfigRefPtr;

class Profile;
typedef scoped_refptr<const Profile> ProfileConstRefPtr;
typedef scoped_refptr<Profile> ProfileRefPtr;

class Connection;
typedef scoped_refptr<Connection> ConnectionRefPtr;

}  // namespace shill

#endif  // SHILL_REFPTR_TYPES_H_
