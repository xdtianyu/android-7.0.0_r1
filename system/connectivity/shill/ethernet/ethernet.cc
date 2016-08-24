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

#include "shill/ethernet/ethernet.h"

#include <linux/ethtool.h>
#include <netinet/ether.h>
#include <netinet/in.h>
#include <linux/if.h>  // NOLINT - Needs definitions from netinet/ether.h
#include <linux/sockios.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <map>
#include <string>
#include <vector>

#include <base/bind.h>

#include "shill/adaptor_interfaces.h"
#include "shill/control_interface.h"
#include "shill/device.h"
#include "shill/device_info.h"
#include "shill/ethernet/ethernet_service.h"
#include "shill/event_dispatcher.h"
#include "shill/logging.h"
#include "shill/manager.h"
#include "shill/net/rtnl_handler.h"
#include "shill/pppoe/pppoe_service.h"
#include "shill/profile.h"
#include "shill/property_accessor.h"
#include "shill/refptr_types.h"
#include "shill/store_interface.h"

#if !defined(DISABLE_WIRED_8021X)
#include "shill/eap_credentials.h"
#include "shill/eap_listener.h"
#include "shill/ethernet/ethernet_eap_provider.h"
#include "shill/supplicant/supplicant_interface_proxy_interface.h"
#include "shill/supplicant/supplicant_process_proxy_interface.h"
#include "shill/supplicant/wpa_supplicant.h"
#endif  // DISABLE_WIRED_8021X

using std::map;
using std::string;
using std::vector;

namespace shill {

namespace Logging {
static auto kModuleLogScope = ScopeLogger::kEthernet;
static string ObjectID(Ethernet* e) { return e->GetRpcIdentifier(); }
}

Ethernet::Ethernet(ControlInterface* control_interface,
                   EventDispatcher* dispatcher,
                   Metrics* metrics,
                   Manager* manager,
                   const string& link_name,
                   const string& address,
                   int interface_index)
    : Device(control_interface,
             dispatcher,
             metrics,
             manager,
             link_name,
             address,
             interface_index,
             Technology::kEthernet),
      control_interface_(control_interface),
      link_up_(false),
#if !defined(DISABLE_WIRED_8021X)
      is_eap_authenticated_(false),
      is_eap_detected_(false),
      eap_listener_(new EapListener(dispatcher, interface_index)),
      supplicant_process_proxy_(
          control_interface_->CreateSupplicantProcessProxy(
              base::Closure(), base::Closure())),
#endif  // DISABLE_WIRED_8021X
      sockets_(new Sockets()),
      weak_ptr_factory_(this) {
  PropertyStore* store = this->mutable_store();
#if !defined(DISABLE_WIRED_8021X)
  store->RegisterConstBool(kEapAuthenticationCompletedProperty,
                           &is_eap_authenticated_);
  store->RegisterConstBool(kEapAuthenticatorDetectedProperty,
                           &is_eap_detected_);
#endif  // DISABLE_WIRED_8021X
  store->RegisterConstBool(kLinkUpProperty, &link_up_);
  store->RegisterDerivedBool(kPPPoEProperty, BoolAccessor(
      new CustomAccessor<Ethernet, bool>(this,
                                         &Ethernet::GetPPPoEMode,
                                         &Ethernet::ConfigurePPPoEMode,
                                         &Ethernet::ClearPPPoEMode)));

#if !defined(DISABLE_WIRED_8021X)
  eap_listener_->set_request_received_callback(
      base::Bind(&Ethernet::OnEapDetected, weak_ptr_factory_.GetWeakPtr()));
#endif  // DISABLE_WIRED_8021X
  service_ = CreateEthernetService();
  SLOG(this, 2) << "Ethernet device " << link_name << " initialized.";
}

Ethernet::~Ethernet() {
}

void Ethernet::Start(Error* error,
                     const EnabledStateChangedCallback& /*callback*/) {
  rtnl_handler()->SetInterfaceFlags(interface_index(), IFF_UP, IFF_UP);
  OnEnabledStateChanged(EnabledStateChangedCallback(), Error());
  LOG(INFO) << "Registering " << link_name() << " with manager.";
  if (!manager()->HasService(service_)) {
    manager()->RegisterService(service_);
  }
  if (error)
    error->Reset();       // indicate immediate completion
}

void Ethernet::Stop(Error* error,
                    const EnabledStateChangedCallback& /*callback*/) {
  manager()->DeregisterService(service_);
#if !defined(DISABLE_WIRED_8021X)
  StopSupplicant();
#endif  // DISABLE_WIRED_8021X
  OnEnabledStateChanged(EnabledStateChangedCallback(), Error());
  if (error)
    error->Reset();       // indicate immediate completion
}

void Ethernet::LinkEvent(unsigned int flags, unsigned int change) {
  Device::LinkEvent(flags, change);
  if ((flags & IFF_LOWER_UP) != 0 && !link_up_) {
    link_up_ = true;
    adaptor()->EmitBoolChanged(kLinkUpProperty, link_up_);
    // We SetupWakeOnLan() here, instead of in Start(), because with
    // r8139, "ethtool -s eth0 wol g" fails when no cable is plugged
    // in.
    manager()->UpdateService(service_);
    service_->OnVisibilityChanged();
    SetupWakeOnLan();
#if !defined(DISABLE_WIRED_8021X)
    eap_listener_->Start();
#endif  // DISABLE_WIRED_8021X
  } else if ((flags & IFF_LOWER_UP) == 0 && link_up_) {
    link_up_ = false;
    adaptor()->EmitBoolChanged(kLinkUpProperty, link_up_);
    DestroyIPConfig();
    SelectService(nullptr);
    manager()->UpdateService(service_);
    service_->OnVisibilityChanged();
#if !defined(DISABLE_WIRED_8021X)
    is_eap_detected_ = false;
    GetEapProvider()->ClearCredentialChangeCallback(this);
    SetIsEapAuthenticated(false);
    StopSupplicant();
    eap_listener_->Stop();
#endif  // DISABLE_WIRED_8021X
  }
}

bool Ethernet::Load(StoreInterface* storage) {
  const string id = GetStorageIdentifier();
  if (!storage->ContainsGroup(id)) {
    SLOG(this, 2) << "Device is not available in the persistent store: " << id;
    return false;
  }

  bool pppoe = false;
  storage->GetBool(id, kPPPoEProperty, &pppoe);

  Error error;
  ConfigurePPPoEMode(pppoe, &error);
  if (!error.IsSuccess()) {
    LOG(WARNING) << "Error configuring PPPoE mode.  Ignoring!";
  }

  return Device::Load(storage);
}

bool Ethernet::Save(StoreInterface* storage) {
  const string id = GetStorageIdentifier();
  storage->SetBool(id, kPPPoEProperty, GetPPPoEMode(nullptr));
  return true;
}

void Ethernet::ConnectTo(EthernetService* service) {
  CHECK(service == service_.get()) << "Ethernet was asked to connect the "
                                   << "wrong service?";
  CHECK(!GetPPPoEMode(nullptr)) << "We should never connect in PPPoE mode!";
  if (!link_up_) {
    return;
  }
  SelectService(service);
  if (AcquireIPConfigWithLeaseName(service->GetStorageIdentifier())) {
    SetServiceState(Service::kStateConfiguring);
  } else {
    LOG(ERROR) << "Unable to acquire DHCP config.";
    SetServiceState(Service::kStateFailure);
    DestroyIPConfig();
  }
}

void Ethernet::DisconnectFrom(EthernetService* service) {
  CHECK(service == service_.get()) << "Ethernet was asked to disconnect the "
                                   << "wrong service?";
  DropConnection();
}

#if !defined(DISABLE_WIRED_8021X)
void Ethernet::TryEapAuthentication() {
  try_eap_authentication_callback_.Reset(
      Bind(&Ethernet::TryEapAuthenticationTask,
           weak_ptr_factory_.GetWeakPtr()));
  dispatcher()->PostTask(try_eap_authentication_callback_.callback());
}

void Ethernet::BSSAdded(const string& path, const KeyValueStore& properties) {
  NOTREACHED() << __func__ << " is not implemented for Ethernet";
}

void Ethernet::BSSRemoved(const string& path) {
  NOTREACHED() << __func__ << " is not implemented for Ethernet";
}

void Ethernet::Certification(const KeyValueStore& properties) {
  string subject;
  uint32_t depth;
  if (WPASupplicant::ExtractRemoteCertification(properties, &subject, &depth)) {
    dispatcher()->PostTask(Bind(&Ethernet::CertificationTask,
                                weak_ptr_factory_.GetWeakPtr(),
                                subject, depth));
  }
}

void Ethernet::EAPEvent(const string& status, const string& parameter) {
  dispatcher()->PostTask(Bind(&Ethernet::EAPEventTask,
                              weak_ptr_factory_.GetWeakPtr(),
                              status,
                              parameter));
}

void Ethernet::PropertiesChanged(const KeyValueStore& properties) {
  if (!properties.ContainsString(WPASupplicant::kInterfacePropertyState)) {
    return;
  }
  dispatcher()->PostTask(
      Bind(&Ethernet::SupplicantStateChangedTask,
           weak_ptr_factory_.GetWeakPtr(),
           properties.GetString(WPASupplicant::kInterfacePropertyState)));
}

void Ethernet::ScanDone(const bool& /*success*/) {
  NOTREACHED() << __func__ << " is not implented for Ethernet";
}

void Ethernet::TDLSDiscoverResponse(const std::string& peer_address) {
  NOTREACHED() << __func__ << " is not implented for Ethernet";
}

EthernetEapProvider* Ethernet::GetEapProvider() {
  EthernetEapProvider* eap_provider = manager()->ethernet_eap_provider();
  CHECK(eap_provider);
  return eap_provider;
}

ServiceConstRefPtr Ethernet::GetEapService() {
  ServiceConstRefPtr eap_service = GetEapProvider()->service();
  CHECK(eap_service);
  return eap_service;
}

void Ethernet::OnEapDetected() {
  is_eap_detected_ = true;
  eap_listener_->Stop();
  GetEapProvider()->SetCredentialChangeCallback(
      this,
      base::Bind(&Ethernet::TryEapAuthentication,
                 weak_ptr_factory_.GetWeakPtr()));
  TryEapAuthentication();
}

bool Ethernet::StartSupplicant() {
  if (supplicant_interface_proxy_.get()) {
    return true;
  }

  string interface_path;
  KeyValueStore create_interface_args;
  create_interface_args.SetString(WPASupplicant::kInterfacePropertyName,
                                  link_name());
  create_interface_args.SetString(WPASupplicant::kInterfacePropertyDriver,
                                  WPASupplicant::kDriverWired);
  create_interface_args.SetString(WPASupplicant::kInterfacePropertyConfigFile,
                                  WPASupplicant::kSupplicantConfPath);
  if (!supplicant_process_proxy_->CreateInterface(create_interface_args,
                                                  &interface_path)) {
    // Interface might've already been created, try to retrieve it.
    if (!supplicant_process_proxy_->GetInterface(link_name(),
                                                 &interface_path)) {
      LOG(ERROR) << __func__ << ": Failed to create interface with supplicant.";
      StopSupplicant();
      return false;
    }
  }

  supplicant_interface_proxy_.reset(
      control_interface_->CreateSupplicantInterfaceProxy(this, interface_path));
  supplicant_interface_path_ = interface_path;
  return true;
}

bool Ethernet::StartEapAuthentication() {
  KeyValueStore params;
  GetEapService()->eap()->PopulateSupplicantProperties(
      &certificate_file_, &params);
  params.SetString(WPASupplicant::kNetworkPropertyEapKeyManagement,
                   WPASupplicant::kKeyManagementIeee8021X);
  params.SetUint(WPASupplicant::kNetworkPropertyEapolFlags, 0);
  params.SetUint(WPASupplicant::kNetworkPropertyScanSSID, 0);

  service_->ClearEAPCertification();
  eap_state_handler_.Reset();

  if (!supplicant_network_path_.empty()) {
    if (!supplicant_interface_proxy_->RemoveNetwork(supplicant_network_path_)) {
      LOG(ERROR) << "Failed to remove network: " << supplicant_network_path_;
      return false;
    }
  }
  if (!supplicant_interface_proxy_->AddNetwork(params,
                                               &supplicant_network_path_)) {
    LOG(ERROR) << "Failed to add network";
    return false;
  }
  CHECK(!supplicant_network_path_.empty());

  supplicant_interface_proxy_->SelectNetwork(supplicant_network_path_);
  supplicant_interface_proxy_->EAPLogon();
  return true;
}

void Ethernet::StopSupplicant() {
  if (supplicant_interface_proxy_.get()) {
    supplicant_interface_proxy_->EAPLogoff();
  }
  supplicant_interface_proxy_.reset();
  if (!supplicant_interface_path_.empty()) {
    if (!supplicant_process_proxy_->RemoveInterface(
        supplicant_interface_path_)) {
      LOG(ERROR) << __func__ << ": Failed to remove interface from supplicant.";
    }
  }
  supplicant_network_path_ = "";
  supplicant_interface_path_ = "";
  SetIsEapAuthenticated(false);
}

void Ethernet::SetIsEapAuthenticated(bool is_eap_authenticated) {
  if (is_eap_authenticated == is_eap_authenticated_) {
    return;
  }

  // If our EAP authentication state changes, we have now joined a different
  // network.  Restart the DHCP process and any other connection state.
  DisconnectFrom(service_.get());
  ConnectTo(service_.get());
  is_eap_authenticated_ = is_eap_authenticated;
  adaptor()->EmitBoolChanged(kEapAuthenticationCompletedProperty,
                             is_eap_authenticated_);
}

void Ethernet::CertificationTask(const string& subject, uint32_t depth) {
  CHECK(service_) << "Ethernet " << link_name() << " " << __func__
                  << " with no service.";
  service_->AddEAPCertification(subject, depth);
}

void Ethernet::EAPEventTask(const string& status, const string& parameter) {
  LOG(INFO) << "In " << __func__ << " with status " << status
            << ", parameter " << parameter;
  Service::ConnectFailure failure = Service::kFailureUnknown;
  if (eap_state_handler_.ParseStatus(status, parameter, &failure)) {
    LOG(INFO) << "EAP authentication succeeded!";
    SetIsEapAuthenticated(true);
  } else if (failure != Service::Service::kFailureUnknown) {
    LOG(INFO) << "EAP authentication failed!";
    SetIsEapAuthenticated(false);
  }
}

void Ethernet::SupplicantStateChangedTask(const string& state) {
  LOG(INFO) << "Supplicant state changed to " << state;
}

void Ethernet::TryEapAuthenticationTask() {
  if (!GetEapService()->Is8021xConnectable()) {
    if (is_eap_authenticated_) {
      LOG(INFO) << "EAP Service lost 802.1X credentials; "
                << "terminating EAP authentication.";
    } else {
      LOG(INFO) << "EAP Service lacks 802.1X credentials; "
                << "not doing EAP authentication.";
    }
    StopSupplicant();
    return;
  }

  if (!is_eap_detected_) {
    LOG(WARNING) << "EAP authenticator not detected; "
                 << "not doing EAP authentication.";
    return;
  }
  if (!StartSupplicant()) {
    LOG(ERROR) << "Failed to start supplicant.";
    return;
  }
  StartEapAuthentication();
}
#endif  // DISABLE_WIRED_8021X

void Ethernet::SetupWakeOnLan() {
  int sock;
  struct ifreq interface_command;
  struct ethtool_wolinfo wake_on_lan_command;

  if (link_name().length() >= sizeof(interface_command.ifr_name)) {
    LOG(WARNING) << "Interface name " << link_name() << " too long: "
                 << link_name().size() << " >= "
                 << sizeof(interface_command.ifr_name);
    return;
  }

  sock = sockets_->Socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);
  if (sock < 0) {
    LOG(WARNING) << "Failed to allocate socket: "
                 << sockets_->ErrorString() << ".";
    return;
  }
  ScopedSocketCloser socket_closer(sockets_.get(), sock);

  memset(&interface_command, 0, sizeof(interface_command));
  memset(&wake_on_lan_command, 0, sizeof(wake_on_lan_command));
  wake_on_lan_command.cmd = ETHTOOL_SWOL;
  if (manager()->IsWakeOnLanEnabled()) {
    wake_on_lan_command.wolopts = WAKE_MAGIC;
  }
  interface_command.ifr_data = &wake_on_lan_command;
  memcpy(interface_command.ifr_name,
         link_name().data(), link_name().length());

  int res = sockets_->Ioctl(sock, SIOCETHTOOL, &interface_command);
  if (res < 0) {
    LOG(WARNING) << "Failed to enable wake-on-lan: "
                 << sockets_->ErrorString() << ".";
    return;
  }
}

bool Ethernet::ConfigurePPPoEMode(const bool& enable, Error* error) {
#if defined(DISABLE_PPPOE)
  if (enable) {
    LOG(WARNING) << "PPPoE support is not implemented.  Ignoring attempt "
                 << "to configure " << link_name();
    error->Populate(Error::kNotSupported);
  }
  return false;
#else
  CHECK(service_);

  EthernetServiceRefPtr service = nullptr;
  if (enable && service_->technology() != Technology::kPPPoE) {
    service = CreatePPPoEService();
  } else if (!enable && service_->technology() == Technology::kPPPoE) {
    service = CreateEthernetService();
  } else {
    return false;
  }

  CHECK(service);
  service_->Disconnect(error, nullptr);
  manager()->DeregisterService(service_);
  service_ = service;
  manager()->RegisterService(service_);

  return true;
#endif  // DISABLE_PPPOE
}

bool Ethernet::GetPPPoEMode(Error* error) {
  if (service_ == nullptr) {
    return false;
  }
  return service_->technology() == Technology::kPPPoE;
}

void Ethernet::ClearPPPoEMode(Error* error) {
  ConfigurePPPoEMode(false, error);
}

EthernetServiceRefPtr Ethernet::CreateEthernetService() {
  return new EthernetService(control_interface_,
                             dispatcher(),
                             metrics(),
                             manager(),
                             weak_ptr_factory_.GetWeakPtr());
}

EthernetServiceRefPtr Ethernet::CreatePPPoEService() {
  return new PPPoEService(control_interface_,
                          dispatcher(),
                          metrics(),
                          manager(),
                          weak_ptr_factory_.GetWeakPtr());
}

}  // namespace shill
