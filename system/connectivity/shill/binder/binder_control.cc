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

#include "shill/binder/binder_control.h"

#include <base/bind.h>
#include <binder/IServiceManager.h>
#include <binderwrapper/binder_wrapper.h>
#include <brillo/binder_watcher.h>

// TODO(samueltan): remove when shill is no longer dependent on DBus proxies.
#include <dbus/service_constants.h>

#include "shill/manager.h"

#include "shill/binder/device_binder_adaptor.h"
#include "shill/binder/manager_binder_adaptor.h"
#include "shill/binder/service_binder_adaptor.h"
#include "shill/dbus/chromeos_dhcpcd_listener.h"
#include "shill/dbus/chromeos_dhcpcd_proxy.h"
#include "shill/ipconfig_adaptor_stub.h"
#include "shill/profile_adaptor_stub.h"
#include "shill/rpc_task_adaptor_stub.h"
#include "shill/third_party_vpn_adaptor_stub.h"
#include "shill/dbus/chromeos_firewalld_proxy.h"
#include "shill/power_manager_proxy_stub.h"
#include "shill/upstart/upstart_proxy_stub.h"
#if !defined(DISABLE_WIFI)
#include "shill/dbus/chromeos_supplicant_bss_proxy.h"
#endif  // DISABLE_WIFI
#if !defined(DISABLE_WIFI) || !defined(DISABLE_WIRED_8021X)
#include "shill/dbus/chromeos_supplicant_interface_proxy.h"
#include "shill/dbus/chromeos_supplicant_network_proxy.h"
#include "shill/dbus/chromeos_supplicant_process_proxy.h"
#endif  // DISABLE_WIFI || DISABLE_WIRED_8021X

using android::BinderWrapper;
using android::defaultServiceManager;
using std::string;
using std::to_string;

namespace shill {

// static.
const char BinderControl::kNullRpcIdentifier[] = "-1";

BinderControl::BinderControl(EventDispatcher* dispatcher)
    : next_unique_binder_adaptor_id_(0),
      dispatcher_(dispatcher),
      null_identifier_(kNullRpcIdentifier) {
  BinderWrapper::Create();
  // Watch Binder events in the main loop
  brillo::BinderWatcher binder_watcher;
  CHECK(binder_watcher.Init()) << "Binder FD watcher init failed";

  // Also initialize D-Bus, which we will use alongside Binder for IPC with
  // daemons that do not yet support Binder.
  // TODO(samueltan): remove when shill is no longer dependent on DBus proxies.
  dbus::Bus::Options options;
  options.bus_type = dbus::Bus::SYSTEM;
  proxy_bus_ = new dbus::Bus(options);
  CHECK(proxy_bus_->Connect());
}

BinderControl::~BinderControl() {
  // TODO(samueltan): remove when shill is no longer dependent on DBus proxies.
  if (proxy_bus_) {
    proxy_bus_->ShutdownAndBlock();
  }
}

const string& BinderControl::NullRPCIdentifier() { return null_identifier_; }

void BinderControl::RegisterManagerObject(
    Manager* manager, const base::Closure& registration_done_callback) {
  // Binder manager object registration is performed synchronously, and
  // ManagerBinderAdaptor::RegisterAsync does not
  // actually use the callback passed to it. However, since the caller of this
  // function expects |registration_done_callback| to be called asynchronously,
  // post the callback to the message loop ourselves.
  manager->RegisterAsync(base::Callback<void(bool)>());
  dispatcher_->PostTask(registration_done_callback);
}

DeviceAdaptorInterface* BinderControl::CreateDeviceAdaptor(Device* device) {
  return CreateAdaptor<Device, DeviceAdaptorInterface, DeviceBinderAdaptor>(
      device);
}

IPConfigAdaptorInterface* BinderControl::CreateIPConfigAdaptor(
    IPConfig* config) {
  return new IPConfigAdaptorStub(to_string(next_unique_binder_adaptor_id_++));
}

ManagerAdaptorInterface* BinderControl::CreateManagerAdaptor(Manager* manager) {
  return CreateAdaptor<Manager, ManagerAdaptorInterface, ManagerBinderAdaptor>(
      manager);
}

ProfileAdaptorInterface* BinderControl::CreateProfileAdaptor(Profile* profile) {
  return new ProfileAdaptorStub(to_string(next_unique_binder_adaptor_id_++));
}

RPCTaskAdaptorInterface* BinderControl::CreateRPCTaskAdaptor(RPCTask* task) {
  return new RPCTaskAdaptorStub(to_string(next_unique_binder_adaptor_id_++));
}

ServiceAdaptorInterface* BinderControl::CreateServiceAdaptor(Service* service) {
  return CreateAdaptor<Service, ServiceAdaptorInterface, ServiceBinderAdaptor>(
      service);
}

#ifndef DISABLE_VPN
ThirdPartyVpnAdaptorInterface* BinderControl::CreateThirdPartyVpnAdaptor(
    ThirdPartyVpnDriver* driver) {
  return new ThirdPartyVpnAdaptorStub(
      to_string(next_unique_binder_adaptor_id_++));
}
#endif

PowerManagerProxyInterface* BinderControl::CreatePowerManagerProxy(
    PowerManagerProxyDelegate* delegate,
    const base::Closure& service_appeared_callback,
    const base::Closure& service_vanished_callback) {
  return new PowerManagerProxyStub();
}

#if !defined(DISABLE_WIFI) || !defined(DISABLE_WIRED_8021X)
SupplicantProcessProxyInterface* BinderControl::CreateSupplicantProcessProxy(
    const base::Closure& service_appeared_callback,
    const base::Closure& service_vanished_callback) {
  return new ChromeosSupplicantProcessProxy(dispatcher_, proxy_bus_,
                                            service_appeared_callback,
                                            service_vanished_callback);
}

SupplicantInterfaceProxyInterface*
BinderControl::CreateSupplicantInterfaceProxy(
    SupplicantEventDelegateInterface* delegate, const string& object_path) {
  return new ChromeosSupplicantInterfaceProxy(proxy_bus_, object_path,
                                              delegate);
}

SupplicantNetworkProxyInterface* BinderControl::CreateSupplicantNetworkProxy(
    const string& object_path) {
  return new ChromeosSupplicantNetworkProxy(proxy_bus_, object_path);
}
#endif  // DISABLE_WIFI || DISABLE_WIRED_8021X

#if !defined(DISABLE_WIFI)
SupplicantBSSProxyInterface* BinderControl::CreateSupplicantBSSProxy(
    WiFiEndpoint* wifi_endpoint, const string& object_path) {
  return new ChromeosSupplicantBSSProxy(proxy_bus_, object_path, wifi_endpoint);
}
#endif  // DISABLE_WIFI

DHCPCDListenerInterface* BinderControl::CreateDHCPCDListener(
    DHCPProvider* provider) {
  return new ChromeosDHCPCDListener(proxy_bus_, dispatcher_, provider);
}

DHCPProxyInterface* BinderControl::CreateDHCPProxy(const string& service) {
  return new ChromeosDHCPCDProxy(proxy_bus_, service);
}

UpstartProxyInterface* BinderControl::CreateUpstartProxy() {
  return new UpstartProxyStub();
}

FirewallProxyInterface* BinderControl::CreateFirewallProxy() {
  return new ChromeosFirewalldProxy(proxy_bus_);
}

template <typename Object, typename AdaptorInterface, typename Adaptor>
AdaptorInterface* BinderControl::CreateAdaptor(Object* object) {
  return new Adaptor(object, to_string(next_unique_binder_adaptor_id_++));
}

}  // namespace shill
