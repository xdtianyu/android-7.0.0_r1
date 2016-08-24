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

#include "shill/connection.h"

#include <arpa/inet.h>
#include <linux/rtnetlink.h>

#include <set>

#include <base/strings/stringprintf.h>

#include "shill/control_interface.h"
#include "shill/device_info.h"
#include "shill/firewall_proxy_interface.h"
#include "shill/logging.h"
#include "shill/net/rtnl_handler.h"
#include "shill/routing_table.h"

#if !defined(__ANDROID__)
#include "shill/resolver.h"
#else
#include "shill/dns_server_proxy.h"
#include "shill/dns_server_proxy_factory.h"
#endif  // __ANDROID__

using base::Bind;
using base::Closure;
using base::Unretained;
using std::deque;
using std::set;
using std::string;
using std::vector;

namespace shill {

namespace Logging {
static auto kModuleLogScope = ScopeLogger::kConnection;
static string ObjectID(Connection* c) {
  if (c == nullptr)
    return "(connection)";
  return c->interface_name();
}
}

#if defined(__ANDROID__)
namespace {
const char* kGoogleDNSServers[] = {
    "8.8.4.4",
    "8.8.8.8"
};
}  // namespace
#endif  // __ANDROID__

// static
const uint32_t Connection::kDefaultMetric = 1;
// static
const uint32_t Connection::kNonDefaultMetricBase = 10;
// static
const uint32_t Connection::kMarkForUserTraffic = 0x1;
// static
const uint8_t Connection::kSecondaryTableId = 0x1;

Connection::Binder::Binder(const string& name,
                           const Closure& disconnect_callback)
    : name_(name),
      client_disconnect_callback_(disconnect_callback) {}

Connection::Binder::~Binder() {
  Attach(nullptr);
}

void Connection::Binder::Attach(const ConnectionRefPtr& to_connection) {
  if (connection_) {
    connection_->DetachBinder(this);
    LOG(INFO) << name_ << ": unbound from connection: "
              << connection_->interface_name();
    connection_.reset();
  }
  if (to_connection) {
    connection_ = to_connection->weak_ptr_factory_.GetWeakPtr();
    connection_->AttachBinder(this);
    LOG(INFO) << name_ << ": bound to connection: "
              << connection_->interface_name();
  }
}

void Connection::Binder::OnDisconnect() {
  LOG(INFO) << name_ << ": bound connection disconnected: "
            << connection_->interface_name();
  connection_.reset();
  if (!client_disconnect_callback_.is_null()) {
    SLOG(connection_.get(), 2) << "Running client disconnect callback.";
    client_disconnect_callback_.Run();
  }
}

Connection::Connection(int interface_index,
                       const std::string& interface_name,
                       Technology::Identifier technology,
                       const DeviceInfo* device_info,
                       ControlInterface* control_interface)
    : weak_ptr_factory_(this),
      is_default_(false),
      has_broadcast_domain_(false),
      routing_request_count_(0),
      interface_index_(interface_index),
      interface_name_(interface_name),
      technology_(technology),
      user_traffic_only_(false),
      table_id_(RT_TABLE_MAIN),
      local_(IPAddress::kFamilyUnknown),
      gateway_(IPAddress::kFamilyUnknown),
      lower_binder_(
          interface_name_,
          // Connection owns a single instance of |lower_binder_| so it's safe
          // to use an Unretained callback.
          Bind(&Connection::OnLowerDisconnect, Unretained(this))),
      device_info_(device_info),
#if !defined(__ANDROID__)
      resolver_(Resolver::GetInstance()),
#else
      dns_server_proxy_factory_(DNSServerProxyFactory::GetInstance()),
#endif  // __ANDROID__
      routing_table_(RoutingTable::GetInstance()),
      rtnl_handler_(RTNLHandler::GetInstance()),
      control_interface_(control_interface) {
  SLOG(this, 2) << __func__ << "(" << interface_index << ", "
                << interface_name << ", "
                << Technology::NameFromIdentifier(technology) << ")";
}

Connection::~Connection() {
  SLOG(this, 2) << __func__ << " " << interface_name_;

  NotifyBindersOnDisconnect();

  DCHECK(!routing_request_count_);
  routing_table_->FlushRoutes(interface_index_);
  routing_table_->FlushRoutesWithTag(interface_index_);
  device_info_->FlushAddresses(interface_index_);
  TearDownIptableEntries();
}

void Connection::UpdateFromIPConfig(const IPConfigRefPtr& config) {
  SLOG(this, 2) << __func__ << " " << interface_name_;

  const IPConfig::Properties& properties = config->properties();
  user_traffic_only_ = properties.user_traffic_only;
  table_id_ = user_traffic_only_ ? kSecondaryTableId : (uint8_t)RT_TABLE_MAIN;

  IPAddress gateway(properties.address_family);
  if (!properties.gateway.empty() &&
      !gateway.SetAddressFromString(properties.gateway)) {
    LOG(ERROR) << "Gateway address " << properties.gateway << " is invalid";
    return;
  }

  excluded_ips_cidr_ = properties.exclusion_list;

  IPAddress trusted_ip(properties.address_family);
  if (!excluded_ips_cidr_.empty()) {
    const std::string first_excluded_ip = excluded_ips_cidr_[0];
    excluded_ips_cidr_.erase(excluded_ips_cidr_.begin());
    // A VPN connection can currently be bound to exactly one lower connection
    // such as eth0 or wan0. The excluded IPs are pinned to the gateway of
    // that connection. Setting up the routing table this way ensures that when
    // the lower connection goes offline, the associated entries in the routing
    // table are removed. On the flip side, when there are multiple connections
    // such as eth0 and wan0 and some IPs can be reached quickly over one
    // connection and the others over a different connection, all routes are
    // still pinned to a connection.
    //
    // The optimal connection to reach the first excluded IP is found below.
    // When this is found the route for the remaining excluded IPs are pinned in
    // the method PinPendingRoutes below.
    if (!trusted_ip.SetAddressAndPrefixFromString(first_excluded_ip)) {
      LOG(ERROR) << "Trusted IP address "
                 << first_excluded_ip << " is invalid";
      return;
    }
    if (!PinHostRoute(trusted_ip, gateway)) {
      LOG(ERROR) << "Unable to pin host route to " << first_excluded_ip;
      return;
    }
  }

  IPAddress local(properties.address_family);
  if (!local.SetAddressFromString(properties.address)) {
    LOG(ERROR) << "Local address " << properties.address << " is invalid";
    return;
  }
  local.set_prefix(properties.subnet_prefix);

  IPAddress broadcast(properties.address_family);
  if (properties.broadcast_address.empty()) {
    if (properties.peer_address.empty()) {
      LOG(WARNING) << "Broadcast address is not set.  Using default.";
      broadcast = local.GetDefaultBroadcast();
    }
  } else if (!broadcast.SetAddressFromString(properties.broadcast_address)) {
    LOG(ERROR) << "Broadcast address " << properties.broadcast_address
               << " is invalid";
    return;
  }

  IPAddress peer(properties.address_family);
  if (!properties.peer_address.empty() &&
      !peer.SetAddressFromString(properties.peer_address)) {
    LOG(ERROR) << "Peer address " << properties.peer_address
               << " is invalid";
    return;
  }

  if (!FixGatewayReachability(local, &peer, &gateway, trusted_ip)) {
    LOG(WARNING) << "Expect limited network connectivity.";
  }

  if (device_info_->HasOtherAddress(interface_index_, local)) {
    // The address has changed for this interface.  We need to flush
    // everything and start over.
    LOG(INFO) << __func__ << ": Flushing old addresses and routes.";
    routing_table_->FlushRoutes(interface_index_);
    device_info_->FlushAddresses(interface_index_);
  }

  LOG(INFO) << __func__ << ": Installing with parameters:"
            << " local=" << local.ToString()
            << " broadcast=" << broadcast.ToString()
            << " peer=" << peer.ToString()
            << " gateway=" << gateway.ToString();
  rtnl_handler_->AddInterfaceAddress(interface_index_, local, broadcast, peer);

  if (gateway.IsValid() && properties.default_route) {
    routing_table_->SetDefaultRoute(interface_index_, gateway,
                                    GetMetric(is_default_),
                                    table_id_);
  }

  if (user_traffic_only_) {
    SetupIptableEntries();
  }

  // Install any explicitly configured routes at the default metric.
  routing_table_->ConfigureRoutes(interface_index_, config, kDefaultMetric,
                                  table_id_);

  SetMTU(properties.mtu);

  if (properties.blackhole_ipv6) {
    routing_table_->CreateBlackholeRoute(interface_index_,
                                         IPAddress::kFamilyIPv6,
                                         kDefaultMetric,
                                         table_id_);
  }

  // Save a copy of the last non-null DNS config.
  if (!config->properties().dns_servers.empty()) {
    dns_servers_ = config->properties().dns_servers;
  }

#if defined(__ANDROID__)
  // Default to Google's DNS server if it is not provided through DHCP.
  if (config->properties().dns_servers.empty()) {
    LOG(INFO) << "Default to use Google DNS servers";
    dns_servers_ =
        vector<string>(std::begin(kGoogleDNSServers),
                       std::end(kGoogleDNSServers));
  }
#endif  // __ANDROID__

  if (!config->properties().domain_search.empty()) {
    dns_domain_search_ = config->properties().domain_search;
  }

  if (!config->properties().domain_name.empty()) {
    dns_domain_name_ = config->properties().domain_name;
  }

  ipconfig_rpc_identifier_ = config->GetRpcIdentifier();

  PushDNSConfig();

  local_ = local;
  gateway_ = gateway;
  has_broadcast_domain_ = !peer.IsValid();
}

bool Connection::SetupIptableEntries() {
  if (!firewall_proxy_) {
    firewall_proxy_.reset(control_interface_->CreateFirewallProxy());
  }

  std::vector<std::string> user_names;
  user_names.push_back("chronos");
  user_names.push_back("debugd");

  if (!firewall_proxy_->RequestVpnSetup(user_names, interface_name_)) {
    LOG(ERROR) << "VPN iptables setup request failed.";
    return false;
  }

  return true;
}

bool Connection::TearDownIptableEntries() {
  return firewall_proxy_ ? firewall_proxy_->RemoveVpnSetup() : true;
}

void Connection::SetIsDefault(bool is_default) {
  SLOG(this, 2) << __func__ << " " << interface_name_
                << " (index " << interface_index_ << ") "
                << is_default_ << " -> " << is_default;
  if (is_default == is_default_) {
    return;
  }

  routing_table_->SetDefaultMetric(interface_index_, GetMetric(is_default));

  is_default_ = is_default;

  PushDNSConfig();
  if (is_default) {
    DeviceRefPtr device = device_info_->GetDevice(interface_index_);
    if (device) {
      device->RequestPortalDetection();
    }
  }
  routing_table_->FlushCache();
}

void Connection::UpdateDNSServers(const vector<string>& dns_servers) {
  dns_servers_ = dns_servers;
  PushDNSConfig();
}

void Connection::PushDNSConfig() {
  if (!is_default_) {
#if defined(__ANDROID__)
    // Stop DNS server proxy to avoid having multiple instances of it running.
    // Only run DNS server proxy for the current default connection.
    dns_server_proxy_.reset();
#endif  // __ANDROID__
    return;
  }

  vector<string> domain_search = dns_domain_search_;
  if (domain_search.empty() && !dns_domain_name_.empty()) {
    SLOG(this, 2) << "Setting domain search to domain name "
                  << dns_domain_name_;
    domain_search.push_back(dns_domain_name_ + ".");
  }
#if !defined(__ANDROID__)
  resolver_->SetDNSFromLists(dns_servers_, domain_search);
#else
  dns_server_proxy_.reset(
      dns_server_proxy_factory_->CreateDNSServerProxy(dns_servers_));
  dns_server_proxy_->Start();
#endif  // __ANDROID__
}

void Connection::RequestRouting() {
  if (routing_request_count_++ == 0) {
    DeviceRefPtr device = device_info_->GetDevice(interface_index_);
    DCHECK(device.get());
    if (!device.get()) {
      LOG(ERROR) << "Device is NULL!";
      return;
    }
    device->SetLooseRouting(true);
  }
}

void Connection::ReleaseRouting() {
  DCHECK_GT(routing_request_count_, 0);
  if (--routing_request_count_ == 0) {
    DeviceRefPtr device = device_info_->GetDevice(interface_index_);
    DCHECK(device.get());
    if (!device.get()) {
      LOG(ERROR) << "Device is NULL!";
      return;
    }
    device->SetLooseRouting(false);

    // Clear any cached routes that might have accumulated while reverse-path
    // filtering was disabled.
    routing_table_->FlushCache();
  }
}

bool Connection::RequestHostRoute(const IPAddress& address) {
  // Do not set interface_index_ since this may not be the default route through
  // which this destination can be found.  However, we should tag the created
  // route with our interface index so we can clean this route up when this
  // connection closes.  Also, add route query callback to determine the lower
  // connection and bind to it.
  if (!routing_table_->RequestRouteToHost(
          address,
          -1,
          interface_index_,
          Bind(&Connection::OnRouteQueryResponse,
               weak_ptr_factory_.GetWeakPtr()),
          table_id_)) {
    LOG(ERROR) << "Could not request route to " << address.ToString();
    return false;
  }

  return true;
}

bool Connection::PinPendingRoutes(int interface_index,
                                  RoutingTableEntry entry) {
  // The variable entry is locally modified, hence is passed by value in the
  // second argument above.
  for (auto excluded_ip = excluded_ips_cidr_.begin();
       excluded_ip != excluded_ips_cidr_.end(); ++excluded_ip) {
    if (!entry.dst.SetAddressAndPrefixFromString(*excluded_ip) ||
        !entry.dst.IsValid() ||
        !routing_table_->AddRoute(interface_index, entry)) {
      LOG(ERROR) << "Unable to setup route for " << *excluded_ip << ".";
    }
  }

  return true;
}

string Connection::GetSubnetName() const {
  if (!local().IsValid()) {
    return "";
  }
  return base::StringPrintf("%s/%d",
                            local().GetNetworkPart().ToString().c_str(),
                            local().prefix());
}

bool Connection::FixGatewayReachability(const IPAddress& local,
                                        IPAddress* peer,
                                        IPAddress* gateway,
                                        const IPAddress& trusted_ip) {
  SLOG(nullptr, 2) << __func__
      << " local " << local.ToString()
      << ", peer " << peer->ToString()
      << ", gateway " << gateway->ToString()
      << ", trusted_ip " << trusted_ip.ToString();
  if (!gateway->IsValid()) {
    LOG(WARNING) << "No gateway address was provided for this connection.";
    return false;
  }

  if (peer->IsValid()) {
    if (!gateway->HasSameAddressAs(*peer)) {
      LOG(WARNING) << "Gateway address "
                   << gateway->ToString()
                   << " does not match peer address "
                   << peer->ToString();
      return false;
    }
    if (gateway->HasSameAddressAs(trusted_ip)) {
      // In order to send outgoing traffic in a point-to-point network,
      // the gateway IP address isn't of significance.  As opposed to
      // broadcast networks, we never ARP for the gateway IP address,
      // but just send the IP packet addressed to the recipient.  As
      // such, since using the external trusted IP address as the
      // gateway or peer wreaks havoc on the routing rules, we choose
      // not to supply a gateway address.  Here's an example:
      //
      //     Client    <->  Internet  <->  VPN Gateway  <->  Internal Network
      //   192.168.1.2                      10.0.1.25         172.16.5.0/24
      //
      // In this example, a client connects to a VPN gateway on its
      // public IP address 10.0.1.25.  It gets issued an IP address
      // from the VPN internal pool.  For some VPN gateways, this
      // results in a pushed-down PPP configuration which specifies:
      //
      //    Client local address:   172.16.5.13
      //    Client peer address:    10.0.1.25
      //    Client default gateway: 10.0.1.25
      //
      // If we take this literally, we need to resolve the fact that
      // 10.0.1.25 is now listed as the default gateway and interface
      // peer address for the point-to-point interface.  However, in
      // order to route tunneled packets to the VPN gateway we must
      // use the external route through the physical interface and
      // not the tunnel, or else we end up in an infinite loop
      // re-entering the tunnel trying to route towards the VPN server.
      //
      // We can do this by pinning a route, but we would need to wait
      // for the pinning process to complete before assigning this
      // address.  Currently this process is asynchronous and will
      // complete only after returning to the event loop.  Additionally,
      // since there's no metric associated with assigning an address
      // to an interface, it's always possible that having the peer
      // address of the interface might still trump a host route.
      //
      // To solve this problem, we reset the peer and gateway
      // addresses.  Neither is required in order to perform the
      // underlying routing task.  A gateway route can be specified
      // without an IP endpoint on point-to-point links, and simply
      // specify the outbound interface index.  Similarly, a peer
      // IP address is not necessary either, and will be assigned
      // the same IP address as the local IP.  This approach
      // simplifies routing and doesn't change the desired
      // functional behavior.
      //
      LOG(INFO) << "Removing gateway and peer addresses to preserve "
                << "routability to trusted IP address.";
      peer->SetAddressToDefault();
      gateway->SetAddressToDefault();
    }
    return true;
  }

  if (local.CanReachAddress(*gateway)) {
    return true;
  }

  LOG(WARNING) << "Gateway "
               << gateway->ToString()
               << " is unreachable from local address/prefix "
               << local.ToString() << "/" << local.prefix();

  IPAddress gateway_with_max_prefix(*gateway);
  gateway_with_max_prefix.set_prefix(
      IPAddress::GetMaxPrefixLength(gateway_with_max_prefix.family()));
  IPAddress default_address(gateway->family());
  RoutingTableEntry entry(gateway_with_max_prefix,
                          default_address,
                          default_address,
                          0,
                          RT_SCOPE_LINK,
                          false,
                          table_id_,
                          RoutingTableEntry::kDefaultTag);

  if (!routing_table_->AddRoute(interface_index_, entry)) {
    LOG(ERROR) << "Unable to add link-scoped route to gateway.";
    return false;
  }

  LOG(WARNING) << "Mitigating this by creating a link route to the gateway.";

  return true;
}

uint32_t Connection::GetMetric(bool is_default) {
  // If this is not the default route, assign a metric based on the interface
  // index.  This way all non-default routes (even to the same gateway IP) end
  // up with unique metrics so they do not collide.
  return is_default ? kDefaultMetric : kNonDefaultMetricBase + interface_index_;
}

bool Connection::PinHostRoute(const IPAddress& trusted_ip,
                              const IPAddress& gateway) {
  SLOG(this, 2) << __func__;
  if (!trusted_ip.IsValid()) {
    LOG(ERROR) << "No trusted IP -- unable to pin host route.";
    return false;
  }

  if (!gateway.IsValid()) {
    // Although we cannot pin a host route, we are also not going to create
    // a gateway route that will interfere with our primary connection, so
    // it is okay to return success here.
    LOG(WARNING) << "No gateway -- unable to pin host route.";
    return true;
  }

  return RequestHostRoute(trusted_ip);
}

void Connection::SetMTU(int32_t mtu) {
  SLOG(this, 2) << __func__ << " " << mtu;
  // Make sure the MTU value is valid.
  if (mtu == IPConfig::kUndefinedMTU) {
    mtu = IPConfig::kDefaultMTU;
  } else {
    int min_mtu = IsIPv6() ? IPConfig::kMinIPv6MTU : IPConfig::kMinIPv4MTU;
    if (mtu < min_mtu) {
      SLOG(this, 2) << __func__ << " MTU " << mtu
                    << " is too small; adjusting up to " << min_mtu;
      mtu = min_mtu;
    }
  }

  rtnl_handler_->SetInterfaceMTU(interface_index_, mtu);
}

void Connection::OnRouteQueryResponse(int interface_index,
                                      const RoutingTableEntry& entry) {
  SLOG(this, 2) << __func__ << "(" << interface_index << ", "
                << entry.tag << ")" << " @ " << interface_name_;
  lower_binder_.Attach(nullptr);
  DeviceRefPtr device = device_info_->GetDevice(interface_index);
  if (!device) {
    LOG(ERROR) << "Unable to lookup device for index " << interface_index;
    return;
  }
  ConnectionRefPtr connection = device->connection();
  if (!connection) {
    LOG(ERROR) << "Device " << interface_index << " has no connection.";
    return;
  }
  if (connection == this) {
    LOG(ERROR) << "Avoiding a connection bind loop for " << interface_name();
    return;
  }
  lower_binder_.Attach(connection);
  connection->CreateGatewayRoute();
  device->OnConnectionUpdated();
  PinPendingRoutes(interface_index, entry);
}

bool Connection::CreateGatewayRoute() {
  // Ensure that the gateway for the lower connection remains reachable,
  // since we may create routes that conflict with it.
  if (!has_broadcast_domain_) {
    return false;
  }

  // If there is no gateway, don't try to create a route to it.
  if (!gateway_.IsValid()) {
    return false;
  }

  // It is not worth keeping track of this route, since it is benign,
  // and only pins persistent state that was already true of the connection.
  // If DHCP parameters change later (without the connection having been
  // destroyed and recreated), the binding processes will likely terminate
  // and restart, causing a new link route to be created.
  return routing_table_->CreateLinkRoute(interface_index_, local_, gateway_,
                                         table_id_);
}

void Connection::OnLowerDisconnect() {
  SLOG(this, 2) << __func__ << " @ " << interface_name_;
  // Ensures that |this| instance doesn't get destroyed in the middle of
  // notifying the binders. This method needs to be separate from
  // NotifyBindersOnDisconnect because the latter may be invoked by Connection's
  // destructor when |this| instance's reference count is already 0.
  ConnectionRefPtr connection(this);
  connection->NotifyBindersOnDisconnect();
}

void Connection::NotifyBindersOnDisconnect() {
  // Note that this method may be invoked by the destructor.
  SLOG(this, 2) << __func__ << " @ " << interface_name_;

  // Unbinds the lower connection before notifying the binders. This ensures
  // correct behavior in case of circular binding.
  lower_binder_.Attach(nullptr);
  while (!binders_.empty()) {
    // Pop the binder first and then notify it to ensure that each binder is
    // notified only once.
    Binder* binder = binders_.front();
    binders_.pop_front();
    binder->OnDisconnect();
  }
}

void Connection::AttachBinder(Binder* binder) {
  SLOG(this, 2) << __func__ << "(" << binder->name() << ")" << " @ "
                            << interface_name_;
  binders_.push_back(binder);
}

void Connection::DetachBinder(Binder* binder) {
  SLOG(this, 2) << __func__ << "(" << binder->name() << ")" << " @ "
                            << interface_name_;
  for (auto it = binders_.begin(); it != binders_.end(); ++it) {
    if (binder == *it) {
      binders_.erase(it);
      return;
    }
  }
}

ConnectionRefPtr Connection::GetCarrierConnection() {
  SLOG(this, 2) << __func__ << " @ " << interface_name_;
  set<Connection*> visited;
  ConnectionRefPtr carrier = this;
  while (carrier->GetLowerConnection()) {
    if (ContainsKey(visited, carrier.get())) {
      LOG(ERROR) << "Circular connection chain starting at: "
                 << carrier->interface_name();
      // If a loop is detected return a NULL value to signal that the carrier
      // connection is unknown.
      return nullptr;
    }
    visited.insert(carrier.get());
    carrier = carrier->GetLowerConnection();
  }
  SLOG(this, 2) << "Carrier connection: " << carrier->interface_name()
                << " @ " << interface_name_;
  return carrier;
}

bool Connection::IsIPv6() {
  return local_.family() == IPAddress::kFamilyIPv6;
}

}  // namespace shill
