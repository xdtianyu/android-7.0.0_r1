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

#ifndef SHILL_CONTROL_INTERFACE_H_
#define SHILL_CONTROL_INTERFACE_H_

#include <algorithm>
#include <string>

#include <base/callback.h>

#include "shill/logging.h"

namespace shill {

class Device;
class DeviceAdaptorInterface;
class IPConfig;
class IPConfigAdaptorInterface;
class Manager;
class ManagerAdaptorInterface;
class Profile;
class ProfileAdaptorInterface;
class RPCTask;
class RPCTaskAdaptorInterface;
class Service;
class ServiceAdaptorInterface;
class ThirdPartyVpnDriver;
class ThirdPartyVpnAdaptorInterface;

class DBusObjectManagerProxyInterface;
class DBusPropertiesProxyInterface;
class DHCPCDListenerInterface;
class DHCPProvider;
class DHCPProxyInterface;
class FirewallProxyInterface;
class ModemCDMAProxyInterface;
class ModemGSMCardProxyInterface;
class ModemGSMNetworkProxyInterface;
class ModemGobiProxyInterface;
class ModemManagerClassic;
class ModemManagerProxyInterface;
class ModemProxyInterface;
class ModemSimpleProxyInterface;
class PowerManagerProxyDelegate;
class PowerManagerProxyInterface;
class UpstartProxyInterface;
class WiMaxDeviceProxyInterface;
class WiMaxManagerProxyInterface;
class WiMaxNetworkProxyInterface;

#if !defined(DISABLE_WIFI)
class SupplicantBSSProxyInterface;
class WiFiEndpoint;
#endif  // DISABLE_WIFI

#if !defined(DISABLE_WIFI) || !defined(DISABLE_WIRED_8021X)
class SupplicantEventDelegateInterface;
class SupplicantInterfaceProxyInterface;
class SupplicantNetworkProxyInterface;
class SupplicantProcessProxyInterface;
#endif  // DISABLE_WIFI || DISABLE_WIRED_8021X

namespace mm1 {

class ModemModem3gppProxyInterface;
class ModemModemCdmaProxyInterface;
class ModemProxyInterface;
class ModemSimpleProxyInterface;
class SimProxyInterface;

}  // namespace mm1

class RPCServiceWatcherInterface;

// This is the Interface for an object factory that creates adaptor/proxy
// objects
class ControlInterface {
 public:
  virtual ~ControlInterface() {}
  virtual void RegisterManagerObject(
      Manager* manager, const base::Closure& registration_done_callback) = 0;
  virtual DeviceAdaptorInterface* CreateDeviceAdaptor(Device* device) = 0;
  virtual IPConfigAdaptorInterface* CreateIPConfigAdaptor(
      IPConfig* ipconfig) = 0;
  virtual ManagerAdaptorInterface* CreateManagerAdaptor(Manager* manager) = 0;
  virtual ProfileAdaptorInterface* CreateProfileAdaptor(Profile* profile) = 0;
  virtual ServiceAdaptorInterface* CreateServiceAdaptor(Service* service) = 0;
  virtual RPCTaskAdaptorInterface* CreateRPCTaskAdaptor(RPCTask* task) = 0;
#ifndef DISABLE_VPN
  virtual ThirdPartyVpnAdaptorInterface* CreateThirdPartyVpnAdaptor(
      ThirdPartyVpnDriver* driver) = 0;
#endif

  virtual const std::string& NullRPCIdentifier() = 0;

  // The caller retains ownership of 'delegate'.  It must not be deleted before
  // the proxy.
  virtual PowerManagerProxyInterface* CreatePowerManagerProxy(
      PowerManagerProxyDelegate* delegate,
      const base::Closure& service_appeared_callback,
      const base::Closure& service_vanished_callback) = 0;

#if !defined(DISABLE_WIFI) || !defined(DISABLE_WIRED_8021X)
  virtual SupplicantProcessProxyInterface* CreateSupplicantProcessProxy(
      const base::Closure& service_appeared_callback,
      const base::Closure& service_vanished_callback) = 0;

  virtual SupplicantInterfaceProxyInterface* CreateSupplicantInterfaceProxy(
      SupplicantEventDelegateInterface* delegate,
      const std::string& object_path) = 0;

  virtual SupplicantNetworkProxyInterface* CreateSupplicantNetworkProxy(
      const std::string& object_path) = 0;
#endif  // DISABLE_WIFI || DISABLE_WIRED_8021X

#if !defined(DISABLE_WIFI)
  // See comment in supplicant_bss_proxy.h, about bare pointer.
  virtual SupplicantBSSProxyInterface* CreateSupplicantBSSProxy(
      WiFiEndpoint* wifi_endpoint,
      const std::string& object_path) = 0;
#endif  // DISABLE_WIFI

  virtual UpstartProxyInterface* CreateUpstartProxy() = 0;

  virtual DHCPCDListenerInterface* CreateDHCPCDListener(
      DHCPProvider* provider) = 0;

  virtual DHCPProxyInterface* CreateDHCPProxy(const std::string& service) = 0;

  virtual FirewallProxyInterface* CreateFirewallProxy() = 0;

#if !defined(DISABLE_CELLULAR)
  virtual DBusPropertiesProxyInterface* CreateDBusPropertiesProxy(
      const std::string& path,
      const std::string& service) = 0;

  virtual DBusObjectManagerProxyInterface* CreateDBusObjectManagerProxy(
      const std::string& path,
      const std::string& service,
      const base::Closure& service_appeared_callback,
      const base::Closure& service_vanished_callback) = 0;

  virtual ModemManagerProxyInterface* CreateModemManagerProxy(
      ModemManagerClassic* manager,
      const std::string& path,
      const std::string& service,
      const base::Closure& service_appeared_callback,
      const base::Closure& service_vanished_callback) = 0;

  virtual ModemProxyInterface* CreateModemProxy(const std::string& path,
                                                const std::string& service) = 0;

  virtual ModemSimpleProxyInterface* CreateModemSimpleProxy(
      const std::string& path,
      const std::string& service) = 0;

  virtual ModemCDMAProxyInterface* CreateModemCDMAProxy(
      const std::string& path,
      const std::string& service) = 0;

  virtual ModemGSMCardProxyInterface* CreateModemGSMCardProxy(
      const std::string& path,
      const std::string& service) = 0;

  virtual ModemGSMNetworkProxyInterface* CreateModemGSMNetworkProxy(
      const std::string& path,
      const std::string& service) = 0;

  virtual ModemGobiProxyInterface* CreateModemGobiProxy(
      const std::string& path,
      const std::string& service) = 0;

  // Proxies for ModemManager1 interfaces
  virtual mm1::ModemModem3gppProxyInterface* CreateMM1ModemModem3gppProxy(
      const std::string& path,
      const std::string& service) = 0;

  virtual mm1::ModemModemCdmaProxyInterface* CreateMM1ModemModemCdmaProxy(
      const std::string& path,
      const std::string& service) = 0;

  virtual mm1::ModemProxyInterface* CreateMM1ModemProxy(
      const std::string& path,
      const std::string& service) = 0;

  virtual mm1::ModemSimpleProxyInterface* CreateMM1ModemSimpleProxy(
      const std::string& path,
      const std::string& service) = 0;

  virtual mm1::SimProxyInterface* CreateSimProxy(
      const std::string& path,
      const std::string& service) = 0;
#endif  // DISABLE_CELLULAR

#if !defined(DISABLE_WIMAX)
  virtual WiMaxDeviceProxyInterface* CreateWiMaxDeviceProxy(
      const std::string& path) = 0;
  virtual WiMaxManagerProxyInterface* CreateWiMaxManagerProxy(
      const base::Closure& service_appeared_callback,
      const base::Closure& service_vanished_callback) = 0;
  virtual WiMaxNetworkProxyInterface* CreateWiMaxNetworkProxy(
      const std::string& path) = 0;
#endif  // DISABLE_WIMAX

  static void RpcIdToStorageId(std::string* rpc_id) {
    CHECK(rpc_id);
    DCHECK_EQ(rpc_id->at(0), '/');
    rpc_id->erase(0, 1);
    std::replace(rpc_id->begin(), rpc_id->end(), '/', '_');
  }
};

}  // namespace shill

#endif  // SHILL_CONTROL_INTERFACE_H_
