//
// Copyright (C) 2015 The Android Open Source Project
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

#include "shill/dbus/chromeos_dbus_control.h"

#include <brillo/dbus/async_event_sequencer.h>

#if defined(__ANDROID__)
#include <dbus/service_constants.h>
#else
#include <chromeos/dbus/service_constants.h>
#endif  // __ANDROID__

#include "shill/dbus/chromeos_device_dbus_adaptor.h"
#include "shill/dbus/chromeos_ipconfig_dbus_adaptor.h"
#include "shill/dbus/chromeos_manager_dbus_adaptor.h"
#include "shill/dbus/chromeos_profile_dbus_adaptor.h"
#include "shill/dbus/chromeos_rpc_task_dbus_adaptor.h"
#include "shill/dbus/chromeos_service_dbus_adaptor.h"
#include "shill/dbus/chromeos_third_party_vpn_dbus_adaptor.h"

#include "shill/dbus/chromeos_dhcpcd_listener.h"
#include "shill/dbus/chromeos_dhcpcd_proxy.h"
#if defined(__ANDROID__)
#include "shill/dbus/chromeos_firewalld_proxy.h"
#include "shill/power_manager_proxy_stub.h"
#include "shill/upstart/upstart_proxy_stub.h"
#else
#include "shill/dbus/chromeos_permission_broker_proxy.h"
#include "shill/dbus/chromeos_power_manager_proxy.h"
#include "shill/dbus/chromeos_upstart_proxy.h"
#endif  // __ANDROID__
#include "shill/dbus/chromeos_dbus_service_watcher.h"

#if !defined(DISABLE_CELLULAR)
#include "shill/dbus/chromeos_dbus_objectmanager_proxy.h"
#include "shill/dbus/chromeos_dbus_properties_proxy.h"
#include "shill/dbus/chromeos_mm1_modem_modem3gpp_proxy.h"
#include "shill/dbus/chromeos_mm1_modem_modemcdma_proxy.h"
#include "shill/dbus/chromeos_mm1_modem_proxy.h"
#include "shill/dbus/chromeos_mm1_modem_simple_proxy.h"
#include "shill/dbus/chromeos_mm1_sim_proxy.h"
#include "shill/dbus/chromeos_modem_cdma_proxy.h"
#include "shill/dbus/chromeos_modem_gobi_proxy.h"
#include "shill/dbus/chromeos_modem_gsm_card_proxy.h"
#include "shill/dbus/chromeos_modem_gsm_network_proxy.h"
#include "shill/dbus/chromeos_modem_manager_proxy.h"
#include "shill/dbus/chromeos_modem_proxy.h"
#include "shill/dbus/chromeos_modem_simple_proxy.h"
#endif  // DISABLE_CELLULAR

#if !defined(DISABLE_WIFI)
#include "shill/dbus/chromeos_supplicant_bss_proxy.h"
#endif  // DISABLE_WIFI

#if !defined(DISABLE_WIFI) || !defined(DISABLE_WIRED_8021X)
#include "shill/dbus/chromeos_supplicant_interface_proxy.h"
#include "shill/dbus/chromeos_supplicant_network_proxy.h"
#include "shill/dbus/chromeos_supplicant_process_proxy.h"
#endif  // DISABLE_WIFI || DISABLE_WIRED_8021X

#if !defined(DISABLE_WIMAX)
#include "shill/dbus/chromeos_wimax_device_proxy.h"
#include "shill/dbus/chromeos_wimax_manager_proxy.h"
#include "shill/dbus/chromeos_wimax_network_proxy.h"
#endif  // DISABLE_WIMAX

#include "shill/manager.h"

using brillo::dbus_utils::AsyncEventSequencer;
using std::string;

namespace shill {

// static.
const char ChromeosDBusControl::kNullPath[] = "/";

ChromeosDBusControl::ChromeosDBusControl(EventDispatcher* dispatcher)
    : dispatcher_(dispatcher),
      null_identifier_(kNullPath) {
  dbus::Bus::Options options;
  options.bus_type = dbus::Bus::SYSTEM;

  adaptor_bus_ = new dbus::Bus(options);
  proxy_bus_ = new dbus::Bus(options);
  CHECK(adaptor_bus_->Connect());
  CHECK(proxy_bus_->Connect());
}

ChromeosDBusControl::~ChromeosDBusControl() {
  if (adaptor_bus_) {
    adaptor_bus_->ShutdownAndBlock();
  }
  if (proxy_bus_) {
    proxy_bus_->ShutdownAndBlock();
  }
}

const string& ChromeosDBusControl::NullRPCIdentifier() {
  return null_identifier_;
}

void ChromeosDBusControl::RegisterManagerObject(
    Manager* manager, const base::Closure& registration_done_callback) {
  registration_done_callback_ = registration_done_callback;
  scoped_refptr<AsyncEventSequencer> sequencer(new AsyncEventSequencer());
  manager->RegisterAsync(
      base::Bind(
          &ChromeosDBusControl::OnDBusServiceRegistered,
          base::Unretained(this),
          sequencer->GetHandler("Manager.RegisterAsync() failed.", true)));
  sequencer->OnAllTasksCompletedCall({
      base::Bind(&ChromeosDBusControl::TakeServiceOwnership,
                 base::Unretained(this))
  });
}

template <typename Object, typename AdaptorInterface, typename Adaptor>
AdaptorInterface* ChromeosDBusControl::CreateAdaptor(Object* object) {
  return new Adaptor(adaptor_bus_, object);
}

void ChromeosDBusControl::OnDBusServiceRegistered(
    const base::Callback<void(bool)>& completion_action, bool success) {
  // The DBus control interface will take over the ownership of the DBus service
  // in this callback.  The daemon will crash if registration failed.
  completion_action.Run(success);

  // We can start the manager now that we have ownership of the D-Bus service.
  // Doing so earlier would allow the manager to emit signals before service
  // ownership was acquired.
  registration_done_callback_.Run();
}

void ChromeosDBusControl::TakeServiceOwnership(bool success) {
  // Success should always be true since we've said that failures are fatal.
  CHECK(success) << "Init of one or more objects has failed.";
  CHECK(adaptor_bus_->RequestOwnershipAndBlock(kFlimflamServiceName,
                                       dbus::Bus::REQUIRE_PRIMARY))
      << "Unable to take ownership of " << kFlimflamServiceName;
}

DeviceAdaptorInterface* ChromeosDBusControl::CreateDeviceAdaptor(
    Device* device) {
  return
      CreateAdaptor<Device, DeviceAdaptorInterface, ChromeosDeviceDBusAdaptor>(
          device);
}

IPConfigAdaptorInterface* ChromeosDBusControl::CreateIPConfigAdaptor(
    IPConfig* config) {
  return
      CreateAdaptor<IPConfig, IPConfigAdaptorInterface,
                    ChromeosIPConfigDBusAdaptor>(config);
}

ManagerAdaptorInterface* ChromeosDBusControl::CreateManagerAdaptor(
    Manager* manager) {
  return new ChromeosManagerDBusAdaptor(adaptor_bus_, proxy_bus_, manager);
}

ProfileAdaptorInterface* ChromeosDBusControl::CreateProfileAdaptor(
    Profile* profile) {
  return
      CreateAdaptor<Profile, ProfileAdaptorInterface,
                    ChromeosProfileDBusAdaptor>(profile);
}

RPCTaskAdaptorInterface* ChromeosDBusControl::CreateRPCTaskAdaptor(
    RPCTask* task) {
  return
      CreateAdaptor<RPCTask, RPCTaskAdaptorInterface,
                    ChromeosRPCTaskDBusAdaptor>(task);
}

ServiceAdaptorInterface* ChromeosDBusControl::CreateServiceAdaptor(
    Service* service) {
  return
      CreateAdaptor<Service, ServiceAdaptorInterface,
                    ChromeosServiceDBusAdaptor>(service);
}

#ifndef DISABLE_VPN
ThirdPartyVpnAdaptorInterface* ChromeosDBusControl::CreateThirdPartyVpnAdaptor(
    ThirdPartyVpnDriver* driver) {
  return
      CreateAdaptor<ThirdPartyVpnDriver, ThirdPartyVpnAdaptorInterface,
                    ChromeosThirdPartyVpnDBusAdaptor>(driver);
}
#endif

PowerManagerProxyInterface* ChromeosDBusControl::CreatePowerManagerProxy(
    PowerManagerProxyDelegate* delegate,
    const base::Closure& service_appeared_callback,
    const base::Closure& service_vanished_callback) {
#if defined(__ANDROID__)
  return new PowerManagerProxyStub();
#else
  return new ChromeosPowerManagerProxy(dispatcher_,
                                       proxy_bus_,
                                       delegate,
                                       service_appeared_callback,
                                       service_vanished_callback);
#endif  // __ANDROID__
}

#if !defined(DISABLE_WIFI) || !defined(DISABLE_WIRED_8021X)
SupplicantProcessProxyInterface*
    ChromeosDBusControl::CreateSupplicantProcessProxy(
        const base::Closure& service_appeared_callback,
        const base::Closure& service_vanished_callback) {
  return new ChromeosSupplicantProcessProxy(dispatcher_,
                                            proxy_bus_,
                                            service_appeared_callback,
                                            service_vanished_callback);
}

SupplicantInterfaceProxyInterface*
    ChromeosDBusControl::CreateSupplicantInterfaceProxy(
        SupplicantEventDelegateInterface* delegate,
        const string& object_path) {
  return new ChromeosSupplicantInterfaceProxy(
      proxy_bus_, object_path, delegate);
}

SupplicantNetworkProxyInterface*
    ChromeosDBusControl::CreateSupplicantNetworkProxy(
        const string& object_path) {
  return new ChromeosSupplicantNetworkProxy(proxy_bus_, object_path);
}
#endif  // DISABLE_WIFI || DISABLE_WIRED_8021X

#if !defined(DISABLE_WIFI)
SupplicantBSSProxyInterface* ChromeosDBusControl::CreateSupplicantBSSProxy(
    WiFiEndpoint* wifi_endpoint,
    const string& object_path) {
  return new ChromeosSupplicantBSSProxy(proxy_bus_, object_path, wifi_endpoint);
}
#endif  // DISABLE_WIFI

DHCPCDListenerInterface* ChromeosDBusControl::CreateDHCPCDListener(
    DHCPProvider* provider) {
  return new ChromeosDHCPCDListener(proxy_bus_, dispatcher_, provider);
}

DHCPProxyInterface* ChromeosDBusControl::CreateDHCPProxy(
    const string& service) {
  return new ChromeosDHCPCDProxy(proxy_bus_, service);
}

UpstartProxyInterface* ChromeosDBusControl::CreateUpstartProxy() {
#if defined(__ANDROID__)
  return new UpstartProxyStub();
#else
  return new ChromeosUpstartProxy(proxy_bus_);
#endif  // __ANDROID__
}

FirewallProxyInterface* ChromeosDBusControl::CreateFirewallProxy() {
#if defined(__ANDROID__)
  return new ChromeosFirewalldProxy(proxy_bus_);
#else
  return new ChromeosPermissionBrokerProxy(proxy_bus_);
#endif  // __ANDROID__
}

#if !defined(DISABLE_CELLULAR)
DBusPropertiesProxyInterface* ChromeosDBusControl::CreateDBusPropertiesProxy(
    const string& path,
    const string& service) {
  return new ChromeosDBusPropertiesProxy(proxy_bus_, path, service);
}

DBusObjectManagerProxyInterface*
    ChromeosDBusControl::CreateDBusObjectManagerProxy(
        const string& path,
        const string& service,
        const base::Closure& service_appeared_callback,
        const base::Closure& service_vanished_callback) {
  return new ChromeosDBusObjectManagerProxy(dispatcher_,
                                            proxy_bus_,
                                            path,
                                            service,
                                            service_appeared_callback,
                                            service_vanished_callback);
}

ModemManagerProxyInterface*
    ChromeosDBusControl::CreateModemManagerProxy(
        ModemManagerClassic* manager,
        const string& path,
        const string& service,
        const base::Closure& service_appeared_callback,
        const base::Closure& service_vanished_callback) {
  return new ChromeosModemManagerProxy(dispatcher_,
                                       proxy_bus_,
                                       manager,
                                       path,
                                       service,
                                       service_appeared_callback,
                                       service_vanished_callback);
}

ModemProxyInterface* ChromeosDBusControl::CreateModemProxy(
    const string& path,
    const string& service) {
  return new ChromeosModemProxy(proxy_bus_, path, service);
}

ModemSimpleProxyInterface* ChromeosDBusControl::CreateModemSimpleProxy(
    const string& path,
    const string& service) {
  return new ChromeosModemSimpleProxy(proxy_bus_, path, service);
}

ModemCDMAProxyInterface* ChromeosDBusControl::CreateModemCDMAProxy(
    const string& path,
    const string& service) {
  return new ChromeosModemCDMAProxy(proxy_bus_, path, service);
}

ModemGSMCardProxyInterface* ChromeosDBusControl::CreateModemGSMCardProxy(
    const string& path,
    const string& service) {
  return new ChromeosModemGSMCardProxy(proxy_bus_, path, service);
}

ModemGSMNetworkProxyInterface* ChromeosDBusControl::CreateModemGSMNetworkProxy(
    const string& path,
    const string& service) {
  return new ChromeosModemGSMNetworkProxy(proxy_bus_, path, service);
}

ModemGobiProxyInterface* ChromeosDBusControl::CreateModemGobiProxy(
    const string& path,
    const string& service) {
  return new ChromeosModemGobiProxy(proxy_bus_, path, service);
}

// Proxies for ModemManager1 interfaces
mm1::ModemModem3gppProxyInterface*
    ChromeosDBusControl::CreateMM1ModemModem3gppProxy(
        const string& path,
        const string& service) {
  return new mm1::ChromeosModemModem3gppProxy(proxy_bus_, path, service);
}

mm1::ModemModemCdmaProxyInterface*
    ChromeosDBusControl::CreateMM1ModemModemCdmaProxy(
        const string& path,
        const string& service) {
  return new mm1::ChromeosModemModemCdmaProxy(proxy_bus_, path, service);
}

mm1::ModemProxyInterface* ChromeosDBusControl::CreateMM1ModemProxy(
    const string& path,
    const string& service) {
  return new mm1::ChromeosModemProxy(proxy_bus_, path, service);
}

mm1::ModemSimpleProxyInterface* ChromeosDBusControl::CreateMM1ModemSimpleProxy(
    const string& path,
    const string& service) {
  return new mm1::ChromeosModemSimpleProxy(proxy_bus_, path, service);
}

mm1::SimProxyInterface* ChromeosDBusControl::CreateSimProxy(
    const string& path,
    const string& service) {
  return new mm1::ChromeosSimProxy(proxy_bus_, path, service);
}
#endif  // DISABLE_CELLULAR

#if !defined(DISABLE_WIMAX)
WiMaxDeviceProxyInterface* ChromeosDBusControl::CreateWiMaxDeviceProxy(
    const string& path) {
  return new ChromeosWiMaxDeviceProxy(proxy_bus_, path);
}

WiMaxManagerProxyInterface* ChromeosDBusControl::CreateWiMaxManagerProxy(
    const base::Closure& service_appeared_callback,
    const base::Closure& service_vanished_callback) {
  return new ChromeosWiMaxManagerProxy(dispatcher_,
                                       proxy_bus_,
                                       service_appeared_callback,
                                       service_vanished_callback);
}

WiMaxNetworkProxyInterface* ChromeosDBusControl::CreateWiMaxNetworkProxy(
    const string& path) {
  return new ChromeosWiMaxNetworkProxy(proxy_bus_, path);
}
#endif  // DISABLE_WIMAX

}  // namespace shill
