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

#ifndef SHILL_CONNECTION_H_
#define SHILL_CONNECTION_H_

#include <deque>
#include <string>
#include <vector>

#include <base/memory/ref_counted.h>
#include <base/memory/weak_ptr.h>
#include <gtest/gtest_prod.h>  // for FRIEND_TEST

#include "shill/ipconfig.h"
#include "shill/net/ip_address.h"
#include "shill/refptr_types.h"
#include "shill/technology.h"

namespace shill {

class ControlInterface;
class DeviceInfo;
class FirewallProxyInterface;
class RTNLHandler;
#if !defined(__ANDROID__)
class Resolver;
#else
class DNSServerProxy;
class DNSServerProxyFactory;
#endif  // __ANDROID__
class RoutingTable;
struct RoutingTableEntry;

// The Conneciton maintains the implemented state of an IPConfig, e.g,
// the IP address, routing table and DNS table entries.
class Connection : public base::RefCounted<Connection> {
 public:
  // Clients can instantiate and use Binder to bind to a Connection and get
  // notified when the bound Connection disconnects. Note that the client's
  // disconnect callback will be executed at most once, and only if the bound
  // Connection is destroyed or signals disconnect. The Binder unbinds itself
  // from the underlying Connection when the Binder instance is destructed.
  class Binder {
   public:
    Binder(const std::string& name, const base::Closure& disconnect_callback);
    ~Binder();

    // Binds to |to_connection|. Unbinds the previous bound connection, if
    // any. Pass nullptr to just unbind this Binder.
    void Attach(const ConnectionRefPtr& to_connection);

    const std::string& name() const { return name_; }
    bool IsBound() const { return connection_ != nullptr; }
    ConnectionRefPtr connection() const { return connection_.get(); }

   private:
    friend class Connection;
    FRIEND_TEST(ConnectionTest, Binder);

    // Invoked by |connection_|.
    void OnDisconnect();

    const std::string name_;
    base::WeakPtr<Connection> connection_;
    const base::Closure client_disconnect_callback_;

    DISALLOW_COPY_AND_ASSIGN(Binder);
  };

  Connection(int interface_index,
             const std::string& interface_name,
             Technology::Identifier technology_,
             const DeviceInfo* device_info,
             ControlInterface* control_interface);

  // Add the contents of an IPConfig reference to the list of managed state.
  // This will replace all previous state for this address family.
  virtual void UpdateFromIPConfig(const IPConfigRefPtr& config);

  // Return the connection used by the lower binder.
  virtual ConnectionRefPtr GetLowerConnection() const {
    return lower_binder_.connection();
  }

  // Sets the current connection as "default", i.e., routes and DNS entries
  // should be used by all system components that don't select explicitly.
  virtual bool is_default() const { return is_default_; }
  virtual void SetIsDefault(bool is_default);

  // Update and apply the new DNS servers setting to this connection.
  virtual void UpdateDNSServers(const std::vector<std::string>& dns_servers);

  virtual const std::string& interface_name() const { return interface_name_; }
  virtual int interface_index() const { return interface_index_; }
  virtual const std::vector<std::string>& dns_servers() const {
    return dns_servers_;
  }
  virtual uint8_t table_id() const { return table_id_; }

  virtual const std::string& ipconfig_rpc_identifier() const {
    return ipconfig_rpc_identifier_;
  }

  virtual bool SetupIptableEntries();
  virtual bool TearDownIptableEntries();

  // Request to accept traffic routed to this connection even if it is not
  // the default.  This request is ref-counted so the caller must call
  // ReleaseRouting() when they no longer need this facility.
  virtual void RequestRouting();
  virtual void ReleaseRouting();

  // Request a host route through this connection.
  virtual bool RequestHostRoute(const IPAddress& destination);

  // Request a host route through this connection for a list of IPs in CIDR
  // notation (|excluded_ips_cidr_|).
  virtual bool PinPendingRoutes(int interface_index, RoutingTableEntry entry);

  // Return the subnet name for this connection.
  virtual std::string GetSubnetName() const;

  virtual const IPAddress& local() const { return local_; }
  virtual const IPAddress& gateway() const { return gateway_; }
  virtual Technology::Identifier technology() const { return technology_; }
  virtual const std::string& tethering() const { return tethering_; }
  void set_tethering(const std::string& tethering) { tethering_ = tethering; }

  // Return the lowest connection on which this connection depends. In case of
  // error, a nullptr is returned.
  virtual ConnectionRefPtr GetCarrierConnection();

  // Return true if this is an IPv6 connection.
  virtual bool IsIPv6();

 protected:
  friend class base::RefCounted<Connection>;

  virtual ~Connection();
  virtual bool CreateGatewayRoute();

 private:
  friend class ConnectionTest;
  FRIEND_TEST(ConnectionTest, AddConfig);
  FRIEND_TEST(ConnectionTest, AddConfigUserTrafficOnly);
  FRIEND_TEST(ConnectionTest, Binder);
  FRIEND_TEST(ConnectionTest, Binders);
  FRIEND_TEST(ConnectionTest, BlackholeIPv6);
  FRIEND_TEST(ConnectionTest, Destructor);
  FRIEND_TEST(ConnectionTest, FixGatewayReachability);
  FRIEND_TEST(ConnectionTest, GetCarrierConnection);
  FRIEND_TEST(ConnectionTest, InitState);
  FRIEND_TEST(ConnectionTest, OnRouteQueryResponse);
  FRIEND_TEST(ConnectionTest, RequestHostRoute);
  FRIEND_TEST(ConnectionTest, SetMTU);
  FRIEND_TEST(ConnectionTest, UpdateDNSServers);
  FRIEND_TEST(VPNServiceTest, OnConnectionDisconnected);

  static const uint32_t kDefaultMetric;
  static const uint32_t kNonDefaultMetricBase;
  static const uint32_t kMarkForUserTraffic;
  static const uint8_t kSecondaryTableId;

  // Work around misconfigured servers which provide a gateway address that
  // is unreachable with the provided netmask.
  bool FixGatewayReachability(const IPAddress& local,
                              IPAddress* peer,
                              IPAddress* gateway,
                              const IPAddress& trusted_ip);
  uint32_t GetMetric(bool is_default);
  bool PinHostRoute(const IPAddress& trusted_ip, const IPAddress& gateway);
  void SetMTU(int32_t mtu);

  void OnRouteQueryResponse(int interface_index,
                            const RoutingTableEntry& entry);

  void AttachBinder(Binder* binder);
  void DetachBinder(Binder* binder);
  void NotifyBindersOnDisconnect();

  void OnLowerDisconnect();

  // Send our DNS configuration to the resolver.
  void PushDNSConfig();

  base::WeakPtrFactory<Connection> weak_ptr_factory_;

  bool is_default_;
  bool has_broadcast_domain_;
  int routing_request_count_;
  int interface_index_;
  const std::string interface_name_;
  Technology::Identifier technology_;
  std::vector<std::string> dns_servers_;
  std::vector<std::string> dns_domain_search_;
  std::vector<std::string> excluded_ips_cidr_;
  std::string dns_domain_name_;
  std::string ipconfig_rpc_identifier_;
  bool user_traffic_only_;
  uint8_t table_id_;
  IPAddress local_;
  IPAddress gateway_;

  // Track the tethering status of the Service associated with this connection.
  // This property is set by a service as it takes ownership of a connection,
  // and is read by services that are bound through this connection.
  std::string tethering_;

  // A binder to a lower Connection that this Connection depends on, if any.
  Binder lower_binder_;

  // Binders to clients -- usually to upper connections or related services and
  // devices.
  std::deque<Binder*> binders_;

  // Store cached copies of singletons for speed/ease of testing
  const DeviceInfo* device_info_;
#if !defined(__ANDROID__)
  Resolver* resolver_;
#else
  DNSServerProxyFactory* dns_server_proxy_factory_;
  std::unique_ptr<DNSServerProxy> dns_server_proxy_;
#endif  // __ANDROID__;
  RoutingTable* routing_table_;
  RTNLHandler* rtnl_handler_;

  ControlInterface* control_interface_;
  std::unique_ptr<FirewallProxyInterface> firewall_proxy_;

  DISALLOW_COPY_AND_ASSIGN(Connection);
};

}  // namespace shill

#endif  // SHILL_CONNECTION_H_
