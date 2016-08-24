//
// Copyright (C) 2016 The Android Open Source Project
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

#ifndef SHILL_BINDER_BINDER_CONTROL_H_
#define SHILL_BINDER_BINDER_CONTROL_H_

#include <map>
#include <string>

#include <binder/IBinder.h>
#include <utils/StrongPointer.h>

#include "shill/control_interface.h"

namespace shill {

class EventDispatcher;
class Manager;

class BinderControl : public ControlInterface {
 public:
  BinderControl(EventDispatcher* dispatcher);
  ~BinderControl() override;

  void RegisterManagerObject(
      Manager* manager,
      const base::Closure& registration_done_callback) override;
  DeviceAdaptorInterface* CreateDeviceAdaptor(Device* device) override;
  IPConfigAdaptorInterface* CreateIPConfigAdaptor(IPConfig* ipconfig) override;
  ManagerAdaptorInterface* CreateManagerAdaptor(Manager* manager) override;
  ProfileAdaptorInterface* CreateProfileAdaptor(Profile* profile) override;
  RPCTaskAdaptorInterface* CreateRPCTaskAdaptor(RPCTask* task) override;
  ServiceAdaptorInterface* CreateServiceAdaptor(Service* service) override;

  const std::string& NullRPCIdentifier() override;

  // The caller retains ownership of 'delegate'.  It must not be deleted before
  // the proxy.
  PowerManagerProxyInterface* CreatePowerManagerProxy(
      PowerManagerProxyDelegate* delegate,
      const base::Closure& service_appeared_callback,
      const base::Closure& service_vanished_callback) override;

#if !defined(DISABLE_WIFI) || !defined(DISABLE_WIRED_8021X)
  SupplicantProcessProxyInterface* CreateSupplicantProcessProxy(
      const base::Closure& service_appeared_callback,
      const base::Closure& service_vanished_callback) override;

  SupplicantInterfaceProxyInterface* CreateSupplicantInterfaceProxy(
      SupplicantEventDelegateInterface* delegate,
      const std::string& object_path) override;

  SupplicantNetworkProxyInterface* CreateSupplicantNetworkProxy(
      const std::string& object_path) override;
#endif  // DISABLE_WIFI || DISABLE_WIRED_8021X

#if !defined(DISABLE_WIFI)
  // See comment in supplicant_bss_proxy.h, about bare pointer.
  SupplicantBSSProxyInterface* CreateSupplicantBSSProxy(
      WiFiEndpoint* wifi_endpoint, const std::string& object_path) override;
#endif  // DISABLE_WIFI

  UpstartProxyInterface* CreateUpstartProxy() override;

  DHCPCDListenerInterface* CreateDHCPCDListener(
      DHCPProvider* provider) override;

  DHCPProxyInterface* CreateDHCPProxy(const std::string& service) override;

  FirewallProxyInterface* CreateFirewallProxy() override;

 private:
  static const char kNullRpcIdentifier[];

  template <typename Object, typename AdaptorInterface, typename Adaptor>
  AdaptorInterface* CreateAdaptor(Object* object);

  // This counter is used to assign unique IDs to binder adaptors. The string
  // representation of this integer will assigned to as a unique ID to the next
  // binder adaptor created. This unique ID will then be used as the Binder
  // adaptor's RPC identifier.
  uint32_t next_unique_binder_adaptor_id_;
  std::map<std::string, android::sp<android::IBinder>> rpc_id_to_binder_map_;
  EventDispatcher* dispatcher_;
  std::string null_identifier_;

  // TODO(samueltan): remove when shill is no longer dependent on DBus proxies.
  scoped_refptr<dbus::Bus> proxy_bus_;
};

}  // namespace shill

#endif  // SHILL_BINDER_BINDER_CONTROL_H_
