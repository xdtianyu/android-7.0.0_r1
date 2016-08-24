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

#include <base/bind.h>

#include "proxy_rpc_in_data_types.h"
#include "proxy_rpc_out_data_types.h"
#include "proxy_rpc_server.h"
#include "proxy_util.h"

namespace {
// XmlRpc library verbosity level.
static const int kDefaultXmlRpcVerbosity = 5;
// Profile name to be used for all the tests.
static const char kTestProfileName[] = "test";

bool ValidateNumOfElements(const XmlRpc::XmlRpcValue& value, int expected_num) {
  if (expected_num != 0) {
    return (value.valid() && value.size() == expected_num);
  } else {
    // |value| will be marked invalid when there are no elements.
    return !value.valid();
  }
}
}// namespace

/*************** RPC Method implementations **********/
XmlRpc::XmlRpcValue CreateProfile(
    XmlRpc::XmlRpcValue params_in,
    ProxyShillWifiClient* shill_wifi_client) {
  if (!ValidateNumOfElements(params_in, 1)) {
    return false;
  }
  const std::string& profile_name(params_in[0]);
  return shill_wifi_client->CreateProfile(profile_name);
}

XmlRpc::XmlRpcValue RemoveProfile(
    XmlRpc::XmlRpcValue params_in,
    ProxyShillWifiClient* shill_wifi_client) {
  if (!ValidateNumOfElements(params_in, 1)) {
    return false;
  }
  const std::string& profile_name(params_in[0]);
  return shill_wifi_client->RemoveProfile(profile_name);
}

XmlRpc::XmlRpcValue PushProfile(
    XmlRpc::XmlRpcValue params_in,
    ProxyShillWifiClient* shill_wifi_client) {
  if (!ValidateNumOfElements(params_in, 1)) {
    return false;
  }
  const std::string& profile_name(params_in[0]);
  return shill_wifi_client->PushProfile(profile_name);
}

XmlRpc::XmlRpcValue PopProfile(
    XmlRpc::XmlRpcValue params_in,
    ProxyShillWifiClient* shill_wifi_client) {
  if (!ValidateNumOfElements(params_in, 1)) {
    return false;
  }
  const std::string& profile_name(params_in[0]);
  return shill_wifi_client->PopProfile(profile_name);
}

XmlRpc::XmlRpcValue CleanProfiles(
    XmlRpc::XmlRpcValue params_in,
    ProxyShillWifiClient* shill_wifi_client) {
  if (!ValidateNumOfElements(params_in, 0)) {
    return false;
  }
  return shill_wifi_client->CleanProfiles();
}

XmlRpc::XmlRpcValue ConfigureServiceByGuid(
    XmlRpc::XmlRpcValue params_in,
    ProxyShillWifiClient* shill_wifi_client) {
  if (!ValidateNumOfElements(params_in, 1)) {
    return false;
  }
  ConfigureServiceParameters config_params(&params_in[0]);
  return shill_wifi_client->ConfigureServiceByGuid(
      config_params.guid_,
      config_params.autoconnect_type_,
      config_params.passphrase_);
}

XmlRpc::XmlRpcValue ConfigureWifiService(
    XmlRpc::XmlRpcValue params_in,
    ProxyShillWifiClient* shill_wifi_client) {
  if (!ValidateNumOfElements(params_in, 1)) {
    return false;
  }
  AssociationParameters assoc_params(&params_in[0]);
  brillo::VariantDictionary security_params;
  assoc_params.security_config_->GetServiceProperties(&security_params);
  return shill_wifi_client->ConfigureWifiService(
      assoc_params.ssid_,
      assoc_params.security_config_->security_,
      security_params,
      assoc_params.save_credentials_,
      assoc_params.station_type_,
      assoc_params.is_hidden_,
      assoc_params.guid_,
      assoc_params.autoconnect_type_);
}

XmlRpc::XmlRpcValue ConnectWifi(
    XmlRpc::XmlRpcValue params_in,
    ProxyShillWifiClient* shill_wifi_client) {
  if (!ValidateNumOfElements(params_in, 1)) {
    return false;
  }

  AssociationParameters assoc_params(&params_in[0]);
  std::string wifi_interface = assoc_params.bgscan_config_->interface_;
  if (wifi_interface.empty()) {
    std::vector<std::string> interfaces;
    if (!shill_wifi_client->ListControlledWifiInterfaces(&interfaces) ||
        interfaces.empty()) {
      return false;
    }
    wifi_interface = interfaces[0];
  }
  shill_wifi_client->ConfigureBgScan(
      wifi_interface,
      assoc_params.bgscan_config_->method_,
      assoc_params.bgscan_config_->short_interval_,
      assoc_params.bgscan_config_->long_interval_,
      assoc_params.bgscan_config_->signal_threshold_);

  brillo::VariantDictionary security_params;
  assoc_params.security_config_->GetServiceProperties(&security_params);

  long discovery_time, association_time, configuration_time;
  std::string failure_reason;
  bool is_success = shill_wifi_client->ConnectToWifiNetwork(
      assoc_params.ssid_,
      assoc_params.security_config_->security_,
      security_params,
      assoc_params.save_credentials_,
      assoc_params.station_type_,
      assoc_params.is_hidden_,
      assoc_params.guid_,
      assoc_params.autoconnect_type_,
      GetMillisecondsFromSeconds(assoc_params.discovery_timeout_seconds_),
      GetMillisecondsFromSeconds(assoc_params.association_timeout_seconds_),
      GetMillisecondsFromSeconds(assoc_params.configuration_timeout_seconds_),
      &discovery_time,
      &association_time,
      &configuration_time,
      &failure_reason);

  AssociationResult association_result(
      is_success,
      GetSecondsFromMilliseconds(discovery_time),
      GetSecondsFromMilliseconds(association_time),
      GetSecondsFromMilliseconds(configuration_time),
      failure_reason);
  return association_result.ConvertToXmlRpcValue();
}

XmlRpc::XmlRpcValue DeleteEntriesForSsid(
    XmlRpc::XmlRpcValue params_in,
    ProxyShillWifiClient* shill_wifi_client) {
  if (!ValidateNumOfElements(params_in, 1)) {
    return false;
  }
  const std::string& ssid(params_in[0]);
  return shill_wifi_client->DeleteEntriesForSsid(ssid);
}

XmlRpc::XmlRpcValue InitTestNetworkState(
    XmlRpc::XmlRpcValue params_in,
    ProxyShillWifiClient* shill_wifi_client) {
  if (!ValidateNumOfElements(params_in, 0)) {
    return false;
  }
  shill_wifi_client->SetLogging();
  shill_wifi_client->CleanProfiles();
  shill_wifi_client->RemoveAllWifiEntries();
  shill_wifi_client->RemoveProfile(kTestProfileName);
  bool is_success = shill_wifi_client->CreateProfile(kTestProfileName);
  if (is_success) {
    shill_wifi_client->PushProfile(kTestProfileName);
  }
  return is_success;
}

XmlRpc::XmlRpcValue ListControlledWifiInterfaces(
    XmlRpc::XmlRpcValue params_in,
    ProxyShillWifiClient* shill_wifi_client) {
  if (!ValidateNumOfElements(params_in, 0)) {
    return false;
  }
  std::vector<std::string> interfaces;
  if (!shill_wifi_client->ListControlledWifiInterfaces(&interfaces)) {
    return false;
  }
  XmlRpc::XmlRpcValue result;
  int array_pos = 0;
  for (const auto& interface : interfaces) {
    result[array_pos++] = interface;
  }
  return result;
}

XmlRpc::XmlRpcValue Disconnect(
    XmlRpc::XmlRpcValue params_in,
    ProxyShillWifiClient* shill_wifi_client) {
  if (!ValidateNumOfElements(params_in, 1)) {
    return false;
  }
  const std::string& ssid = params_in[0];
  return shill_wifi_client->Disconnect(ssid);
}

XmlRpc::XmlRpcValue WaitForServiceStates(
    XmlRpc::XmlRpcValue params_in,
    ProxyShillWifiClient* shill_wifi_client) {
  if (!ValidateNumOfElements(params_in, 3)) {
    return false;
  }
  const std::string& ssid(params_in[0]);
  XmlRpc::XmlRpcValue states_as_xmlrpcvalue(params_in[1]);
  int timeout(params_in[2]);
  int num_states = states_as_xmlrpcvalue.size();
  std::vector<std::string> states;
  for (int array_pos = 0; array_pos < num_states; array_pos++) {
    states.emplace_back(std::string(states_as_xmlrpcvalue[array_pos]));
  }
  std::string final_state;
  long wait_time;
  bool is_success = shill_wifi_client->WaitForServiceStates(
    ssid, states, GetMillisecondsFromSeconds(timeout),
    &final_state, &wait_time);
  XmlRpc::XmlRpcValue result;
  result[0] = is_success;
  result[1] = final_state;
  result[2] = GetSecondsFromMilliseconds(wait_time);
  return result;
}

XmlRpc::XmlRpcValue GetServiceOrder(
    XmlRpc::XmlRpcValue params_in,
    ProxyShillWifiClient* shill_wifi_client) {
  if (!ValidateNumOfElements(params_in, 0)) {
    return false;
  }
  std::string order;
  if (!shill_wifi_client->GetServiceOrder(&order)) {
    return false;
  }
  return order;
}

XmlRpc::XmlRpcValue SetServiceOrder(
    XmlRpc::XmlRpcValue params_in,
    ProxyShillWifiClient* shill_wifi_client) {
  if (!ValidateNumOfElements(params_in, 1)) {
    return false;
  }
  const std::string& order(params_in[0]);
  return shill_wifi_client->SetServiceOrder(order);
}

XmlRpc::XmlRpcValue GetServiceProperties(
    XmlRpc::XmlRpcValue params_in,
    ProxyShillWifiClient* shill_wifi_client) {
  if (!ValidateNumOfElements(params_in, 1)) {
    return false;
  }
  const std::string& ssid(params_in[0]);
  brillo::VariantDictionary properties;
  if (!shill_wifi_client->GetServiceProperties(ssid, &properties)) {
    return false;
  }
  XmlRpc::XmlRpcValue result;
  GetXmlRpcValueFromBrilloAnyValue(properties, &result);
  return result;
}

XmlRpc::XmlRpcValue GetActiveWifiSsids(
    XmlRpc::XmlRpcValue params_in,
    ProxyShillWifiClient* shill_wifi_client) {
  if (!ValidateNumOfElements(params_in, 0)) {
    return false;
  }
  std::vector<std::string> ssids;
  if (!shill_wifi_client->GetActiveWifiSsids(&ssids)) {
    return false;
  }
  XmlRpc::XmlRpcValue result;
  int array_pos = 0;
  for (const auto& ssid : ssids) {
    result[array_pos++] = ssid;
  }
  return result;
}

XmlRpc::XmlRpcValue SetSchedScan(
    XmlRpc::XmlRpcValue params_in,
    ProxyShillWifiClient* shill_wifi_client) {
  if (!ValidateNumOfElements(params_in, 1)) {
    return false;
  }
  bool enable(params_in[0]);
  return shill_wifi_client->SetSchedScan(enable);
}

XmlRpc::XmlRpcValue GetDbusPropertyOnDevice(
    XmlRpc::XmlRpcValue params_in,
    ProxyShillWifiClient* shill_wifi_client) {
  if (!ValidateNumOfElements(params_in, 2)) {
    return false;
  }
  const std::string& interface_name(params_in[0]);
  const std::string& property_name(params_in[1]);
  brillo::Any property_value;
  if (!shill_wifi_client->GetPropertyOnDevice(
          interface_name, property_name, &property_value)) {
    return false;
  }
  XmlRpc::XmlRpcValue result;
  GetXmlRpcValueFromBrilloAnyValue(property_value, &result);
  return result;
}

XmlRpc::XmlRpcValue SetDbusPropertyOnDevice(
    XmlRpc::XmlRpcValue params_in,
    ProxyShillWifiClient* shill_wifi_client) {
  if (!ValidateNumOfElements(params_in, 3)) {
    return false;
  }
  const std::string& interface_name(params_in[0]);
  const std::string& property_name(params_in[1]);
  brillo::Any property_value;
  GetBrilloAnyValueFromXmlRpcValue(&params_in[2], &property_value);
  return shill_wifi_client->SetPropertyOnDevice(
      interface_name, property_name, property_value);
}

XmlRpc::XmlRpcValue RequestRoamDbus(
    XmlRpc::XmlRpcValue params_in,
    ProxyShillWifiClient* shill_wifi_client) {
  if (!ValidateNumOfElements(params_in, 2)) {
    return false;
  }
  const std::string& bssid(params_in[0]);
  const std::string& interface_name(params_in[1]);
  // |interface_name| is the first argument in ProxyShillWifiClient method
  // to keep it symmetric with other methods defined in the interface even
  // though it is reversed in the RPC call.
  return shill_wifi_client->RequestRoam(interface_name, bssid);
}

XmlRpc::XmlRpcValue SetDeviceEnabled(
    XmlRpc::XmlRpcValue params_in,
    ProxyShillWifiClient* shill_wifi_client) {
  if (!ValidateNumOfElements(params_in, 2)) {
    return false;
  }
  const std::string& interface_name(params_in[0]);
  bool enable(params_in[1]);
  return shill_wifi_client->SetDeviceEnabled(interface_name, enable);
}

XmlRpc::XmlRpcValue DiscoverTdlsLink(
    XmlRpc::XmlRpcValue params_in,
    ProxyShillWifiClient* shill_wifi_client) {
  if (!ValidateNumOfElements(params_in, 2)) {
    return false;
  }
  const std::string& interface_name(params_in[0]);
  const std::string& peer_mac_address(params_in[1]);
  return shill_wifi_client->DiscoverTdlsLink(interface_name, peer_mac_address);
}

XmlRpc::XmlRpcValue EstablishTdlsLink(
    XmlRpc::XmlRpcValue params_in,
    ProxyShillWifiClient* shill_wifi_client) {
  if (!ValidateNumOfElements(params_in, 2)) {
    return false;
  }
  const std::string& interface_name(params_in[0]);
  const std::string& peer_mac_address(params_in[1]);
  return shill_wifi_client->EstablishTdlsLink(interface_name, peer_mac_address);
}

XmlRpc::XmlRpcValue QueryTdlsLink(
    XmlRpc::XmlRpcValue params_in,
    ProxyShillWifiClient* shill_wifi_client) {
  if (!ValidateNumOfElements(params_in, 2)) {
    return false;
  }
  const std::string& interface_name(params_in[0]);
  const std::string& peer_mac_address(params_in[1]);
  std::string status;
  if (!shill_wifi_client->QueryTdlsLink(
          interface_name, peer_mac_address, &status)) {
    return false;
  }
  return status;
}

XmlRpc::XmlRpcValue AddWakePacketSource(
    XmlRpc::XmlRpcValue params_in,
    ProxyShillWifiClient* shill_wifi_client) {
  if (!ValidateNumOfElements(params_in, 2)) {
    return false;
  }
  const std::string& interface_name(params_in[0]);
  const std::string& source_ip(params_in[1]);
  return shill_wifi_client->AddWakePacketSource(interface_name, source_ip);
}

XmlRpc::XmlRpcValue RemoveWakePacketSource(
    XmlRpc::XmlRpcValue params_in,
    ProxyShillWifiClient* shill_wifi_client) {
  if (!ValidateNumOfElements(params_in, 2)) {
    return false;
  }
  const std::string& interface_name(params_in[0]);
  const std::string& source_ip(params_in[1]);
  return shill_wifi_client->RemoveWakePacketSource(interface_name, source_ip);
}

XmlRpc::XmlRpcValue RemoveAllWakePacketSources(
    XmlRpc::XmlRpcValue params_in,
    ProxyShillWifiClient* shill_wifi_client) {
  if (!ValidateNumOfElements(params_in, 1)) {
    return false;
  }
  const std::string& interface_name(params_in[0]);
  return shill_wifi_client->RemoveAllWakePacketSources(interface_name);
}

XmlRpc::XmlRpcValue SyncTimeTo(
    XmlRpc::XmlRpcValue params_in,
    ProxyShillWifiClient* shill_wifi_client) {
  if (!ValidateNumOfElements(params_in, 1)) {
    return false;
  }
  double epoch_seconds(params_in[0]);
  double seconds;
  double microseconds = modf(epoch_seconds, &seconds) * 1000000;
  struct timeval tv;
  tv.tv_sec = seconds;
  tv.tv_usec = microseconds;
  return settimeofday(&tv, nullptr);
}

// Dummy method to be used for rpc methods not implemented yet.
XmlRpc::XmlRpcValue NotImplementedRpcMethod(
    XmlRpc::XmlRpcValue params_in,
    ProxyShillWifiClient* shill_wifi_client) {
  LOG(ERROR) << "RPC Method not implemented.";
  return true;
}

ProxyRpcServerMethod::ProxyRpcServerMethod(
    const std::string& method_name,
    const RpcServerMethodHandler& handler,
    ProxyShillWifiClient* shill_wifi_client,
    ProxyRpcServer* server)
  : XmlRpcServerMethod(method_name, server),
    handler_(handler),
    shill_wifi_client_(shill_wifi_client) {
}

void ProxyRpcServerMethod::execute(
    XmlRpc::XmlRpcValue& params_in,
    XmlRpc::XmlRpcValue& value_out) {
  value_out = handler_.Run(params_in, shill_wifi_client_);
}

std::string ProxyRpcServerMethod::help(void) {
  // TODO: Lookup the method help using the |method_name| from
  // a text file.
  return "Shill Test Proxy RPC methods help.";
}

ProxyRpcServer::ProxyRpcServer(
    int server_port,
    std::unique_ptr<ProxyShillWifiClient> shill_wifi_client)
  : XmlRpcServer(),
    server_port_(server_port),
    shill_wifi_client_(std::move(shill_wifi_client)) {
}

void ProxyRpcServer::RegisterRpcMethod(
    const std::string& method_name,
    const RpcServerMethodHandler& handler) {
  methods_.emplace_back(
      new ProxyRpcServerMethod(
          method_name, handler, shill_wifi_client_.get(), this));
}

void ProxyRpcServer::Run() {
  XmlRpc::setVerbosity(kDefaultXmlRpcVerbosity);
  if (!XmlRpc::XmlRpcServer::bindAndListen(server_port_)) {
    LOG(ERROR) << "Failed to bind to port " << server_port_ << ".";
    return;
  }
  XmlRpc::XmlRpcServer::enableIntrospection(true);

  RegisterRpcMethod("create_profile", base::Bind(&CreateProfile));
  RegisterRpcMethod("remove_profile", base::Bind(&RemoveProfile));
  RegisterRpcMethod("push_profile", base::Bind(&PushProfile));
  RegisterRpcMethod("pop_profile", base::Bind(&PopProfile));
  RegisterRpcMethod("clean_profiles", base::Bind(&CleanProfiles));
  RegisterRpcMethod("configure_service_by_guid",
                    base::Bind(&ConfigureServiceByGuid));
  RegisterRpcMethod("configure_wifi_service", base::Bind(&ConfigureWifiService));
  RegisterRpcMethod("connect_wifi", base::Bind(&ConnectWifi));
  RegisterRpcMethod("delete_entries_for_ssid", base::Bind(&DeleteEntriesForSsid));
  RegisterRpcMethod("init_test_network_state", base::Bind(&InitTestNetworkState));
  RegisterRpcMethod("list_controlled_wifi_interfaces",
                    base::Bind(&ListControlledWifiInterfaces));
  RegisterRpcMethod("disconnect", base::Bind(&Disconnect));
  RegisterRpcMethod("wait_for_service_states",
                    base::Bind(&WaitForServiceStates));
  RegisterRpcMethod("get_service_order", base::Bind(&GetServiceOrder));
  RegisterRpcMethod("set_service_order", base::Bind(&SetServiceOrder));
  RegisterRpcMethod("get_service_properties", base::Bind(&GetServiceProperties));
  RegisterRpcMethod("get_active_wifi_SSIDs", base::Bind(&GetActiveWifiSsids));
  RegisterRpcMethod("set_sched_scan", base::Bind(&SetSchedScan));
  RegisterRpcMethod("get_dbus_property_on_device",
                    base::Bind(&GetDbusPropertyOnDevice));
  RegisterRpcMethod("set_dbus_property_on_device",
                    base::Bind(&SetDbusPropertyOnDevice));
  RegisterRpcMethod("request_roam_dbus", base::Bind(&RequestRoamDbus));
  RegisterRpcMethod("set_device_enabled", base::Bind(&SetDeviceEnabled));
  RegisterRpcMethod("discover_tdls_link", base::Bind(&DiscoverTdlsLink));
  RegisterRpcMethod("establish_tdls_link", base::Bind(&EstablishTdlsLink));
  RegisterRpcMethod("query_tdls_link", base::Bind(&QueryTdlsLink));
  RegisterRpcMethod("add_wake_packet_source", base::Bind(&AddWakePacketSource));
  RegisterRpcMethod("remove_wake_packet_source",
                    base::Bind(&RemoveWakePacketSource));
  RegisterRpcMethod("remove_all_wake_packet_sources",
                    base::Bind(&RemoveAllWakePacketSources));
  RegisterRpcMethod("sync_time_to",
                    base::Bind(&SyncTimeTo));
  RegisterRpcMethod("request_roam",
                    base::Bind(&NotImplementedRpcMethod));
  RegisterRpcMethod("enable_ui",
                    base::Bind(&NotImplementedRpcMethod));
  RegisterRpcMethod("do_suspend",
                    base::Bind(&NotImplementedRpcMethod));
  RegisterRpcMethod("do_suspend_bg",
                    base::Bind(&NotImplementedRpcMethod));
  RegisterRpcMethod("clear_supplicant_blacklist",
                    base::Bind(&NotImplementedRpcMethod));
  RegisterRpcMethod("ready",
                    base::Bind(&NotImplementedRpcMethod));

  XmlRpc::XmlRpcServer::work(-1.0);
}
