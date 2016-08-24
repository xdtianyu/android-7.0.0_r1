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

#include <memory>
#include <string>
#include <vector>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "shill/ipconfig.h"
#include "shill/mock_connection.h"
#include "shill/mock_control.h"
#include "shill/mock_device.h"
#include "shill/mock_device_info.h"
#include "shill/mock_firewall_proxy.h"
#if !defined(__ANDROID__)
#include "shill/mock_resolver.h"
#else
#include "shill/mock_dns_server_proxy.h"
#include "shill/mock_dns_server_proxy_factory.h"
#endif  // __ANDROID__
#include "shill/mock_routing_table.h"
#include "shill/net/mock_rtnl_handler.h"
#include "shill/routing_table_entry.h"

using std::string;
using std::vector;
using testing::_;
using testing::Mock;
using testing::NiceMock;
using testing::Return;
using testing::ReturnRef;
using testing::StrictMock;
using testing::Test;

namespace shill {

namespace {
const char kTestDeviceName0[] = "netdev0";
const int kTestDeviceInterfaceIndex0 = 123;
const char kTestDeviceName1[] = "netdev1";
const int kTestDeviceInterfaceIndex1 = 321;
const char kIPAddress0[] = "192.168.1.1";
const char kGatewayAddress0[] = "192.168.1.254";
const char kBroadcastAddress0[] = "192.168.1.255";
const char kNameServer0[] = "8.8.8.8";
const char kNameServer1[] = "8.8.9.9";
const int32_t kPrefix0 = 24;
const int32_t kPrefix1 = 31;
const char kSearchDomain0[] = "chromium.org";
const char kSearchDomain1[] = "google.com";
const char kIPv6Address[] = "2001:db8::1";
const char kIPv6NameServer0[] = "2001:db9::1";
const char kIPv6NameServer1[] = "2001:db9::2";
}  // namespace

class ConnectionTest : public Test {
 public:
  ConnectionTest()
      : device_info_(new StrictMock<MockDeviceInfo>(
            &control_,
            nullptr,
            nullptr,
            nullptr)),
        connection_(new Connection(
            kTestDeviceInterfaceIndex0,
            kTestDeviceName0,
            Technology::kUnknown,
            device_info_.get(),
            &control_)),
        ipconfig_(new IPConfig(&control_, kTestDeviceName0)),
        ip6config_(new IPConfig(&control_, kTestDeviceName0)),
        local_address_(IPAddress::kFamilyIPv4),
        broadcast_address_(IPAddress::kFamilyIPv4),
        gateway_address_(IPAddress::kFamilyIPv4),
        default_address_(IPAddress::kFamilyIPv4),
        local_ipv6_address_(IPAddress::kFamilyIPv6) {}

  virtual void SetUp() {
    ReplaceSingletons(connection_);
    properties_.address = kIPAddress0;
    properties_.subnet_prefix = kPrefix0;
    properties_.gateway = kGatewayAddress0;
    properties_.broadcast_address = kBroadcastAddress0;
    properties_.dns_servers.push_back(kNameServer0);
    properties_.dns_servers.push_back(kNameServer1);
    properties_.domain_search.push_back(kSearchDomain0);
    properties_.domain_search.push_back(kSearchDomain1);
    properties_.address_family = IPAddress::kFamilyIPv4;
    UpdateProperties();
    ipv6_properties_.address = kIPv6Address;
    ipv6_properties_.dns_servers.push_back(kIPv6NameServer0);
    ipv6_properties_.dns_servers.push_back(kIPv6NameServer1);
    ipv6_properties_.address_family = IPAddress::kFamilyIPv6;
    UpdateIPv6Properties();
    EXPECT_TRUE(local_address_.SetAddressFromString(kIPAddress0));
    EXPECT_TRUE(broadcast_address_.SetAddressFromString(kBroadcastAddress0));
    EXPECT_TRUE(gateway_address_.SetAddressFromString(kGatewayAddress0));
    EXPECT_TRUE(local_ipv6_address_.SetAddressFromString(kIPv6Address));
  }

  virtual void TearDown() {
    AddDestructorExpectations();
    connection_ = nullptr;
  }

  void ReplaceSingletons(ConnectionRefPtr connection) {
#if !defined(__ANDROID__)
    connection->resolver_ = &resolver_;
#else
    connection->dns_server_proxy_factory_ = &dns_server_proxy_factory_;
#endif  // __ANDROID__
    connection->routing_table_ = &routing_table_;
    connection->rtnl_handler_ = &rtnl_handler_;
  }

  void UpdateProperties() {
    ipconfig_->UpdateProperties(properties_, true);
  }

  void UpdateIPv6Properties() {
    ip6config_->UpdateProperties(ipv6_properties_, true);
  }

  bool PinHostRoute(ConnectionRefPtr connection,
                    const IPAddress trusted_ip,
                    const IPAddress gateway) {
    return connection->PinHostRoute(trusted_ip, gateway);
  }

  const IPAddress& GetLocalAddress(ConnectionRefPtr connection) {
    return connection->local_;
  }

  const IPAddress& GetGatewayAddress(ConnectionRefPtr connection) {
    return connection->gateway_;
  }

  bool GetHasBroadcastDomain(ConnectionRefPtr connection) {
    return connection->has_broadcast_domain_;
  }

  uint32_t GetDefaultMetric() {
      return Connection::kDefaultMetric;
  }

  uint32_t GetNonDefaultMetricBase() {
      return Connection::kNonDefaultMetricBase;
  }

  void SetLocal(const IPAddress& local) {
    connection_->local_ = local;
  }

#if defined(__ANDROID__)
  // Setting expectations for creating/starting DNS server proxy.
  void ExpectDNSServerProxyCreation(const vector<string>& dns_servers,
                                    bool verify_servers) {
    MockDNSServerProxy* dns_server_proxy = new MockDNSServerProxy();
    if (verify_servers) {
      EXPECT_CALL(dns_server_proxy_factory_,
                  CreateDNSServerProxy(dns_servers))
          .WillOnce(Return(dns_server_proxy));
    } else {
      EXPECT_CALL(dns_server_proxy_factory_, CreateDNSServerProxy(_))
          .WillOnce(Return(dns_server_proxy));
    }
    EXPECT_CALL(*dns_server_proxy, Start());
  }
#endif  // __ANDROID__

 protected:
  class DisconnectCallbackTarget {
   public:
    DisconnectCallbackTarget()
        : callback_(base::Bind(&DisconnectCallbackTarget::CallTarget,
                               base::Unretained(this))) {}

    MOCK_METHOD0(CallTarget, void());
    const base::Closure& callback() { return callback_; }

   private:
    base::Closure callback_;
  };

  void AddDestructorExpectations() {
    EXPECT_CALL(routing_table_, FlushRoutes(kTestDeviceInterfaceIndex0));
    EXPECT_CALL(routing_table_, FlushRoutesWithTag(kTestDeviceInterfaceIndex0));
    EXPECT_CALL(*device_info_.get(),
                FlushAddresses(kTestDeviceInterfaceIndex0));
  }

  // Returns a new test connection object. The caller usually needs to call
  // AddDestructorExpectations before destroying the object.
  ConnectionRefPtr GetNewConnection() {
    ConnectionRefPtr connection(new Connection(kTestDeviceInterfaceIndex0,
                                               kTestDeviceName0,
                                               Technology::kUnknown,
                                               device_info_.get(),
                                               &control_));
    ReplaceSingletons(connection);
    return connection;
  }

  std::unique_ptr<StrictMock<MockDeviceInfo>> device_info_;
  ConnectionRefPtr connection_;
  MockControl control_;
  IPConfigRefPtr ipconfig_;
  IPConfigRefPtr ip6config_;
  IPConfig::Properties properties_;
  IPConfig::Properties ipv6_properties_;
  IPAddress local_address_;
  IPAddress broadcast_address_;
  IPAddress gateway_address_;
  IPAddress default_address_;
  IPAddress local_ipv6_address_;
#if !defined(__ANDROID__)
  StrictMock<MockResolver> resolver_;
#else
  StrictMock<MockDNSServerProxyFactory> dns_server_proxy_factory_;
#endif  // __ANDROID__
  StrictMock<MockRoutingTable> routing_table_;
  StrictMock<MockRTNLHandler> rtnl_handler_;
};

namespace {

MATCHER_P2(IsIPAddress, address, prefix, "") {
  IPAddress match_address(address);
  match_address.set_prefix(prefix);
  return match_address.Equals(arg);
}

MATCHER_P(IsIPv6Address, address, "") {
  IPAddress match_address(address);
  return match_address.Equals(arg);
}

MATCHER(IsNonNullCallback, "") {
  return !arg.is_null();
}

MATCHER_P(IsValidRoutingTableEntry, dst, "") {
  return dst.Equals(arg.dst);
}

MATCHER_P(IsLinkRouteTo, dst, "") {
  return dst.HasSameAddressAs(arg.dst) &&
      arg.dst.prefix() ==
          IPAddress::GetMaxPrefixLength(IPAddress::kFamilyIPv4) &&
      !arg.src.IsValid() && !arg.gateway.IsValid() &&
      arg.scope == RT_SCOPE_LINK && !arg.from_rtnl;
}

}  // namespace

TEST_F(ConnectionTest, InitState) {
  EXPECT_EQ(kTestDeviceInterfaceIndex0, connection_->interface_index_);
  EXPECT_EQ(kTestDeviceName0, connection_->interface_name_);
  EXPECT_FALSE(connection_->is_default());
  EXPECT_FALSE(connection_->routing_request_count_);
}

TEST_F(ConnectionTest, AddConfig) {
  EXPECT_CALL(*device_info_,
              HasOtherAddress(kTestDeviceInterfaceIndex0,
                              IsIPAddress(local_address_, kPrefix0)))
      .WillOnce(Return(false));
  EXPECT_CALL(rtnl_handler_,
              AddInterfaceAddress(kTestDeviceInterfaceIndex0,
                                  IsIPAddress(local_address_, kPrefix0),
                                  IsIPAddress(broadcast_address_, 0),
                                  IsIPAddress(default_address_, 0)));
  EXPECT_CALL(routing_table_,
              SetDefaultRoute(kTestDeviceInterfaceIndex0,
                              IsIPAddress(gateway_address_, 0),
                              GetNonDefaultMetricBase() +
                              kTestDeviceInterfaceIndex0,
                              RT_TABLE_MAIN));
  EXPECT_CALL(routing_table_,
              ConfigureRoutes(kTestDeviceInterfaceIndex0,
                              ipconfig_,
                              GetDefaultMetric(),
                              RT_TABLE_MAIN));
  EXPECT_CALL(rtnl_handler_, SetInterfaceMTU(kTestDeviceInterfaceIndex0,
                                             IPConfig::kDefaultMTU));
  connection_->UpdateFromIPConfig(ipconfig_);
  IPAddress test_local_address(local_address_);
  test_local_address.set_prefix(kPrefix0);
  EXPECT_TRUE(test_local_address.Equals(GetLocalAddress(connection_)));
  EXPECT_TRUE(gateway_address_.Equals(GetGatewayAddress(connection_)));
  EXPECT_TRUE(GetHasBroadcastDomain(connection_));
  EXPECT_FALSE(connection_->IsIPv6());

  EXPECT_CALL(routing_table_,
              CreateLinkRoute(kTestDeviceInterfaceIndex0,
                              IsIPAddress(local_address_, kPrefix0),
                              IsIPAddress(gateway_address_, 0),
                              RT_TABLE_MAIN))
      .WillOnce(Return(true))
      .WillOnce(Return(false));
  EXPECT_TRUE(connection_->CreateGatewayRoute());
  EXPECT_FALSE(connection_->CreateGatewayRoute());
  connection_->has_broadcast_domain_ = false;
  EXPECT_FALSE(connection_->CreateGatewayRoute());

  EXPECT_CALL(routing_table_, SetDefaultMetric(kTestDeviceInterfaceIndex0,
                                               GetDefaultMetric()));
#if !defined(__ANDROID__)
  EXPECT_CALL(resolver_, SetDNSFromLists(
      ipconfig_->properties().dns_servers,
      ipconfig_->properties().domain_search));
#else
  ExpectDNSServerProxyCreation(ipconfig_->properties().dns_servers, true);
#endif  // __ANDROID__
  scoped_refptr<MockDevice> device(new StrictMock<MockDevice>(
      &control_,
      nullptr,
      nullptr,
      nullptr,
      kTestDeviceName0,
      string(),
      kTestDeviceInterfaceIndex0));
  EXPECT_CALL(*device_info_, GetDevice(kTestDeviceInterfaceIndex0))
      .WillOnce(Return(device));
  EXPECT_CALL(*device.get(), RequestPortalDetection())
      .WillOnce(Return(true));
  EXPECT_CALL(routing_table_, FlushCache())
      .WillOnce(Return(true));
  connection_->SetIsDefault(true);
  Mock::VerifyAndClearExpectations(&routing_table_);
  EXPECT_TRUE(connection_->is_default());

  EXPECT_CALL(routing_table_,
              SetDefaultMetric(kTestDeviceInterfaceIndex0,
                               GetNonDefaultMetricBase() +
                               kTestDeviceInterfaceIndex0));
  EXPECT_CALL(routing_table_, FlushCache())
      .WillOnce(Return(true));
  connection_->SetIsDefault(false);
  EXPECT_FALSE(connection_->is_default());
}

TEST_F(ConnectionTest, AddConfigUserTrafficOnly) {
  ConnectionRefPtr connection = GetNewConnection();
  const std::string kExcludeAddress1 = "192.0.1.0/24";
  const std::string kExcludeAddress2 = "192.0.2.0/24";
  IPAddress address1(IPAddress::kFamilyIPv4);
  IPAddress address2(IPAddress::kFamilyIPv4);
  EXPECT_TRUE(address1.SetAddressAndPrefixFromString(kExcludeAddress1));
  EXPECT_TRUE(address2.SetAddressAndPrefixFromString(kExcludeAddress2));
  EXPECT_CALL(*device_info_,
              HasOtherAddress(kTestDeviceInterfaceIndex0,
                              IsIPAddress(local_address_, kPrefix0)))
      .WillOnce(Return(false));
  EXPECT_CALL(rtnl_handler_,
              AddInterfaceAddress(kTestDeviceInterfaceIndex0,
                                  IsIPAddress(local_address_, kPrefix0),
                                  IsIPAddress(broadcast_address_, 0),
                                  IsIPAddress(default_address_, 0)));
  EXPECT_CALL(routing_table_,
              ConfigureRoutes(kTestDeviceInterfaceIndex0, ipconfig_,
                              GetDefaultMetric(), 1));
  EXPECT_CALL(
      routing_table_,
      RequestRouteToHost(IsIPAddress(address1, address1.prefix()), -1,
                         kTestDeviceInterfaceIndex0, IsNonNullCallback(), 1))
      .WillOnce(Return(true));
  EXPECT_CALL(rtnl_handler_, SetInterfaceMTU(kTestDeviceInterfaceIndex0,
                                             IPConfig::kDefaultMTU));

  MockFirewallProxy* firewall_proxy = new MockFirewallProxy();
  connection->firewall_proxy_.reset(firewall_proxy);
  EXPECT_CALL(*firewall_proxy, RequestVpnSetup(_, _));
  properties_.user_traffic_only = true;
  properties_.default_route = false;
  properties_.exclusion_list.push_back(kExcludeAddress1);
  properties_.exclusion_list.push_back(kExcludeAddress2);
  UpdateProperties();
  connection->UpdateFromIPConfig(ipconfig_);

  scoped_refptr<MockDevice> device1(
      new MockDevice(&control_, nullptr, nullptr, nullptr, kTestDeviceName1,
                     string(), kTestDeviceInterfaceIndex1));
  scoped_refptr<MockConnection> mock_connection(
      new MockConnection(device_info_.get()));
  ConnectionRefPtr device_connection = mock_connection.get();

  EXPECT_CALL(*device_info_.get(),
              FlushAddresses(mock_connection->interface_index()));
  const string kInterfaceName(kTestDeviceName1);
  EXPECT_CALL(*mock_connection, interface_name())
      .WillRepeatedly(ReturnRef(kInterfaceName));
  EXPECT_CALL(*device1, connection())
      .WillRepeatedly(testing::ReturnRef(device_connection));
  EXPECT_CALL(*device_info_, GetDevice(kTestDeviceInterfaceIndex1))
      .WillOnce(Return(device1));

  EXPECT_CALL(routing_table_, AddRoute(kTestDeviceInterfaceIndex1,
                                       IsValidRoutingTableEntry(address2)))
      .WillOnce(Return(true));

  connection->OnRouteQueryResponse(
      kTestDeviceInterfaceIndex1,
      RoutingTableEntry(default_address_, default_address_, default_address_, 1,
                        1, false));

  IPAddress test_local_address(local_address_);
  test_local_address.set_prefix(kPrefix0);
  EXPECT_TRUE(test_local_address.Equals(GetLocalAddress(connection)));
  EXPECT_TRUE(gateway_address_.Equals(GetGatewayAddress(connection)));
  EXPECT_TRUE(GetHasBroadcastDomain(connection));
  EXPECT_FALSE(connection->IsIPv6());

  EXPECT_CALL(routing_table_,
              CreateLinkRoute(kTestDeviceInterfaceIndex0,
                              IsIPAddress(local_address_, kPrefix0),
                              IsIPAddress(gateway_address_, 0), 1))
      .WillOnce(Return(true))
      .WillOnce(Return(false));
  EXPECT_TRUE(connection->CreateGatewayRoute());
  EXPECT_FALSE(connection->CreateGatewayRoute());
  connection->has_broadcast_domain_ = false;
  EXPECT_FALSE(connection->CreateGatewayRoute());

  EXPECT_CALL(routing_table_,
              SetDefaultMetric(kTestDeviceInterfaceIndex0, GetDefaultMetric()));
#if !defined(__ANDROID__)
  EXPECT_CALL(resolver_,
              SetDNSFromLists(ipconfig_->properties().dns_servers,
                              ipconfig_->properties().domain_search));
#else
  ExpectDNSServerProxyCreation(ipconfig_->properties().dns_servers, true);
#endif  // __ANDROID__
  scoped_refptr<MockDevice> device(new StrictMock<MockDevice>(
      &control_, nullptr, nullptr, nullptr, kTestDeviceName0, string(),
      kTestDeviceInterfaceIndex0));
  EXPECT_CALL(*device_info_, GetDevice(kTestDeviceInterfaceIndex0))
      .WillOnce(Return(device));
  EXPECT_CALL(*device.get(), RequestPortalDetection()).WillOnce(Return(true));
  EXPECT_CALL(routing_table_, FlushCache()).WillOnce(Return(true));
  connection->SetIsDefault(true);
  Mock::VerifyAndClearExpectations(&routing_table_);
  EXPECT_TRUE(connection->is_default());

  EXPECT_CALL(routing_table_, SetDefaultMetric(kTestDeviceInterfaceIndex0,
                                               GetNonDefaultMetricBase() +
                                                   kTestDeviceInterfaceIndex0));
  EXPECT_CALL(routing_table_, FlushCache()).WillOnce(Return(true));
  connection->SetIsDefault(false);
  EXPECT_FALSE(connection->is_default());
  AddDestructorExpectations();
  EXPECT_CALL(*firewall_proxy, RemoveVpnSetup());
}

TEST_F(ConnectionTest, AddConfigIPv6) {
  EXPECT_CALL(*device_info_,
              HasOtherAddress(kTestDeviceInterfaceIndex0,
                              IsIPv6Address(local_ipv6_address_)))
      .WillOnce(Return(false));
  EXPECT_CALL(rtnl_handler_,
              AddInterfaceAddress(kTestDeviceInterfaceIndex0,
                                  IsIPv6Address(local_ipv6_address_),
                                  _,
                                  _));
  EXPECT_CALL(routing_table_,
              ConfigureRoutes(kTestDeviceInterfaceIndex0,
                              ip6config_,
                              GetDefaultMetric(),
                              RT_TABLE_MAIN));
  EXPECT_CALL(rtnl_handler_, SetInterfaceMTU(kTestDeviceInterfaceIndex0,
                                             IPConfig::kDefaultMTU));
  connection_->UpdateFromIPConfig(ip6config_);
  IPAddress test_local_address(local_ipv6_address_);
  EXPECT_TRUE(test_local_address.Equals(GetLocalAddress(connection_)));
  EXPECT_TRUE(connection_->IsIPv6());
}

TEST_F(ConnectionTest, AddConfigWithPeer) {
  const string kPeerAddress("192.168.1.222");
  IPAddress peer_address(IPAddress::kFamilyIPv4);
  EXPECT_TRUE(peer_address.SetAddressFromString(kPeerAddress));
  properties_.peer_address = kPeerAddress;
  properties_.gateway = string();
  UpdateProperties();
  EXPECT_CALL(*device_info_,
              HasOtherAddress(kTestDeviceInterfaceIndex0,
                              IsIPAddress(local_address_, kPrefix0)))
      .WillOnce(Return(false));
  EXPECT_CALL(rtnl_handler_,
              AddInterfaceAddress(kTestDeviceInterfaceIndex0,
                                  IsIPAddress(local_address_, kPrefix0),
                                  IsIPAddress(broadcast_address_, 0),
                                  IsIPAddress(peer_address, 0)));
  EXPECT_CALL(routing_table_, SetDefaultRoute(_, _, _, _)).Times(0);
  EXPECT_CALL(routing_table_,
              ConfigureRoutes(kTestDeviceInterfaceIndex0,
                              ipconfig_,
                              GetDefaultMetric(),
                              RT_TABLE_MAIN));
  EXPECT_CALL(rtnl_handler_, SetInterfaceMTU(kTestDeviceInterfaceIndex0,
                                             IPConfig::kDefaultMTU));
  connection_->UpdateFromIPConfig(ipconfig_);
  EXPECT_FALSE(GetHasBroadcastDomain(connection_));
}

TEST_F(ConnectionTest, AddConfigWithBrokenNetmask) {
  // Assign a prefix that makes the gateway unreachable.
  properties_.subnet_prefix = kPrefix1;
  UpdateProperties();

  // Connection should add a link route which will allow the
  // gateway to be reachable.
  IPAddress gateway_address(IPAddress::kFamilyIPv4);
  EXPECT_TRUE(gateway_address.SetAddressFromString(kGatewayAddress0));
  EXPECT_CALL(routing_table_, AddRoute(kTestDeviceInterfaceIndex0,
                                       IsLinkRouteTo(gateway_address)))
      .WillOnce(Return(true));
  EXPECT_CALL(*device_info_,
              HasOtherAddress(kTestDeviceInterfaceIndex0,
                              IsIPAddress(local_address_, kPrefix1)))
      .WillOnce(Return(false));
  EXPECT_CALL(rtnl_handler_,
              AddInterfaceAddress(kTestDeviceInterfaceIndex0,
                                  IsIPAddress(local_address_, kPrefix1),
                                  IsIPAddress(broadcast_address_, 0),
                                  IsIPAddress(default_address_, 0)));
  EXPECT_CALL(routing_table_,
              SetDefaultRoute(kTestDeviceInterfaceIndex0,
                              IsIPAddress(gateway_address_, 0),
                              GetNonDefaultMetricBase() +
                              kTestDeviceInterfaceIndex0,
                              RT_TABLE_MAIN));
  EXPECT_CALL(routing_table_,
              ConfigureRoutes(kTestDeviceInterfaceIndex0,
                              ipconfig_,
                              GetDefaultMetric(),
                              RT_TABLE_MAIN));
  EXPECT_CALL(rtnl_handler_, SetInterfaceMTU(kTestDeviceInterfaceIndex0,
                                             IPConfig::kDefaultMTU));
  connection_->UpdateFromIPConfig(ipconfig_);
}

TEST_F(ConnectionTest, AddConfigReverse) {
  EXPECT_CALL(routing_table_, SetDefaultMetric(kTestDeviceInterfaceIndex0,
                                               GetDefaultMetric()));
  vector<string> empty_list;
#if !defined(__ANDROID__)
  EXPECT_CALL(resolver_, SetDNSFromLists(empty_list, empty_list));
#else
  ExpectDNSServerProxyCreation(empty_list, true);
#endif  // __ANDROID__
  scoped_refptr<MockDevice> device(new StrictMock<MockDevice>(
      &control_,
      nullptr,
      nullptr,
      nullptr,
      kTestDeviceName0,
      string(),
      kTestDeviceInterfaceIndex0));
  EXPECT_CALL(*device_info_, GetDevice(kTestDeviceInterfaceIndex0))
      .WillOnce(Return(device));
  EXPECT_CALL(*device.get(), RequestPortalDetection())
      .WillOnce(Return(true));
  EXPECT_CALL(routing_table_, FlushCache())
      .WillOnce(Return(true));
  connection_->SetIsDefault(true);
  Mock::VerifyAndClearExpectations(&routing_table_);

  EXPECT_CALL(*device_info_,
              HasOtherAddress(kTestDeviceInterfaceIndex0,
                              IsIPAddress(local_address_, kPrefix0)))
      .WillOnce(Return(false));
  EXPECT_CALL(rtnl_handler_,
              AddInterfaceAddress(kTestDeviceInterfaceIndex0,
                                  IsIPAddress(local_address_, kPrefix0),
                                  IsIPAddress(broadcast_address_, 0),
                                  IsIPAddress(default_address_, 0)));
  EXPECT_CALL(routing_table_, SetDefaultRoute(kTestDeviceInterfaceIndex0,
                                              IsIPAddress(gateway_address_, 0),
                                              GetDefaultMetric(),
                                              RT_TABLE_MAIN));
  EXPECT_CALL(routing_table_,
              ConfigureRoutes(kTestDeviceInterfaceIndex0,
                              ipconfig_,
                              GetDefaultMetric(),
                              RT_TABLE_MAIN));
#if !defined(__ANDROID__)
  EXPECT_CALL(resolver_,
              SetDNSFromLists(ipconfig_->properties().dns_servers,
                              ipconfig_->properties().domain_search));
#else
  ExpectDNSServerProxyCreation(ipconfig_->properties().dns_servers, true);
#endif  // __ANDROID__
  EXPECT_CALL(rtnl_handler_, SetInterfaceMTU(kTestDeviceInterfaceIndex0,
                                             IPConfig::kDefaultMTU));
  connection_->UpdateFromIPConfig(ipconfig_);
}

TEST_F(ConnectionTest, AddConfigWithDNSDomain) {
  const string kDomainName("chromium.org");
  properties_.domain_search.clear();
  properties_.domain_name = kDomainName;
  UpdateProperties();
  EXPECT_CALL(*device_info_, HasOtherAddress(_, _))
      .WillOnce(Return(false));
  EXPECT_CALL(rtnl_handler_, AddInterfaceAddress(_, _, _, _));
  EXPECT_CALL(routing_table_, SetDefaultRoute(_, _, _, _));
  EXPECT_CALL(routing_table_, ConfigureRoutes(_, _, _, _));
  EXPECT_CALL(rtnl_handler_, SetInterfaceMTU(_, _));
  connection_->UpdateFromIPConfig(ipconfig_);

  EXPECT_CALL(routing_table_, SetDefaultMetric(_, _));
  vector<string> domain_search_list;
  domain_search_list.push_back(kDomainName + ".");
#if !defined(__ANDROID__)
  EXPECT_CALL(resolver_, SetDNSFromLists(_, domain_search_list));
#else
  ExpectDNSServerProxyCreation(vector<string>(), false);
#endif  // __ANDROID__
  DeviceRefPtr device;
  EXPECT_CALL(*device_info_, GetDevice(_)).WillOnce(Return(device));
  EXPECT_CALL(routing_table_, FlushCache()).WillOnce(Return(true));
  connection_->SetIsDefault(true);
}

TEST_F(ConnectionTest, HasOtherAddress) {
  EXPECT_CALL(*device_info_,
              HasOtherAddress(kTestDeviceInterfaceIndex0,
                              IsIPAddress(local_address_, kPrefix0)))
      .WillOnce(Return(true));
  EXPECT_CALL(routing_table_, FlushRoutes(kTestDeviceInterfaceIndex0));
  EXPECT_CALL(*device_info_, FlushAddresses(kTestDeviceInterfaceIndex0));
  EXPECT_CALL(rtnl_handler_,
              AddInterfaceAddress(kTestDeviceInterfaceIndex0,
                                  IsIPAddress(local_address_, kPrefix0),
                                  IsIPAddress(broadcast_address_, 0),
                                  IsIPAddress(default_address_, 0)));
  EXPECT_CALL(routing_table_,
              SetDefaultRoute(kTestDeviceInterfaceIndex0,
                              IsIPAddress(gateway_address_, 0),
                              GetNonDefaultMetricBase() +
                              kTestDeviceInterfaceIndex0,
                              RT_TABLE_MAIN));
  EXPECT_CALL(routing_table_,
              ConfigureRoutes(kTestDeviceInterfaceIndex0,
                              ipconfig_,
                              GetDefaultMetric(),
                              RT_TABLE_MAIN));
  EXPECT_CALL(rtnl_handler_, SetInterfaceMTU(kTestDeviceInterfaceIndex0,
                                             IPConfig::kDefaultMTU));
  connection_->UpdateFromIPConfig(ipconfig_);
}

TEST_F(ConnectionTest, UpdateDNSServers) {
  const char* kDnsServers[] = {"1.1.1.1", "1.1.1.2"};
  vector<string> dns_servers(kDnsServers, std::end(kDnsServers));

  // Non-default connection.
  connection_->is_default_ = false;
#if !defined(__ANDROID__)
  EXPECT_CALL(resolver_, SetDNSFromLists(_, _)).Times(0);
#else
  EXPECT_CALL(dns_server_proxy_factory_, CreateDNSServerProxy(_)).Times(0);
#endif  // __ANDROID__
  connection_->UpdateDNSServers(dns_servers);
#if !defined(__ANDROID__)
  Mock::VerifyAndClearExpectations(&resolver_);
#else
  Mock::VerifyAndClearExpectations(&dns_server_proxy_factory_);
#endif  // __ANDROID__

  // Default connection.
  connection_->is_default_ = true;
#if !defined(__ANDROID__)
  EXPECT_CALL(resolver_, SetDNSFromLists(dns_servers, _));
#else
  ExpectDNSServerProxyCreation(dns_servers, true);
#endif  // __ANDROID__
  connection_->UpdateDNSServers(dns_servers);
#if !defined(__ANDROID__)
  Mock::VerifyAndClearExpectations(&resolver_);
#else
  Mock::VerifyAndClearExpectations(&dns_server_proxy_factory_);
#endif  // __ANDROID__
}

TEST_F(ConnectionTest, RouteRequest) {
  ConnectionRefPtr connection = GetNewConnection();
  scoped_refptr<MockDevice> device(new StrictMock<MockDevice>(
      &control_,
      nullptr,
      nullptr,
      nullptr,
      kTestDeviceName0,
      string(),
      kTestDeviceInterfaceIndex0));
  EXPECT_CALL(*device_info_, GetDevice(kTestDeviceInterfaceIndex0))
      .WillRepeatedly(Return(device));
  EXPECT_CALL(*device.get(), SetLooseRouting(true)).Times(1);
  connection->RequestRouting();
  connection->RequestRouting();

  // The first release should only decrement the reference counter.
  connection->ReleaseRouting();

  // Another release will re-enable reverse-path filter.
  EXPECT_CALL(*device.get(), SetLooseRouting(false));
  EXPECT_CALL(routing_table_, FlushCache());
  connection->ReleaseRouting();

  // The destructor will remove the routes and addresses.
  AddDestructorExpectations();
}

TEST_F(ConnectionTest, Destructor) {
  ConnectionRefPtr connection(new Connection(kTestDeviceInterfaceIndex1,
                                             kTestDeviceName1,
                                             Technology::kUnknown,
                                             device_info_.get(),
                                             &control_));
#if !defined(__ANDROID__)
  connection->resolver_ = &resolver_;
#else
  connection->dns_server_proxy_factory_ = &dns_server_proxy_factory_;
#endif  // __ANDROID__
  connection->routing_table_ = &routing_table_;
  connection->rtnl_handler_ = &rtnl_handler_;
  EXPECT_CALL(routing_table_, FlushRoutes(kTestDeviceInterfaceIndex1));
  EXPECT_CALL(routing_table_, FlushRoutesWithTag(kTestDeviceInterfaceIndex1));
  EXPECT_CALL(*device_info_, FlushAddresses(kTestDeviceInterfaceIndex1));
  connection = nullptr;
}

TEST_F(ConnectionTest, RequestHostRoute) {
  ConnectionRefPtr connection = GetNewConnection();
  IPAddress address(IPAddress::kFamilyIPv4);
  ASSERT_TRUE(address.SetAddressFromString(kIPAddress0));
  size_t prefix_len = 16;
  address.set_prefix(prefix_len);
  EXPECT_CALL(routing_table_,
              RequestRouteToHost(IsIPAddress(address, prefix_len),
                                 -1,
                                 kTestDeviceInterfaceIndex0,
                                 IsNonNullCallback(),
                                 RT_TABLE_MAIN))
      .WillOnce(Return(true));
  EXPECT_TRUE(connection->RequestHostRoute(address));

  // The destructor will remove the routes and addresses.
  AddDestructorExpectations();
}

TEST_F(ConnectionTest, BlackholeIPv6) {
  properties_.blackhole_ipv6 = true;
  UpdateProperties();
  EXPECT_CALL(*device_info_, HasOtherAddress(_, _))
      .WillOnce(Return(false));
  EXPECT_CALL(rtnl_handler_, AddInterfaceAddress(_, _, _, _));
  EXPECT_CALL(routing_table_, SetDefaultRoute(_, _, _, _));
  EXPECT_CALL(routing_table_, ConfigureRoutes(_, _, _, _));
  EXPECT_CALL(routing_table_,
              CreateBlackholeRoute(kTestDeviceInterfaceIndex0,
                                   IPAddress::kFamilyIPv6,
                                   Connection::kDefaultMetric,
                                   RT_TABLE_MAIN))
      .WillOnce(Return(true));
  EXPECT_CALL(rtnl_handler_, SetInterfaceMTU(kTestDeviceInterfaceIndex0,
                                             IPConfig::kDefaultMTU));
  connection_->UpdateFromIPConfig(ipconfig_);
}

TEST_F(ConnectionTest, PinHostRoute) {
  ConnectionRefPtr connection = GetNewConnection();

  IPAddress gateway(IPAddress::kFamilyIPv4);
  IPAddress trusted_ip(IPAddress::kFamilyIPv4);

  // Should fail because neither IP address is set.
  EXPECT_FALSE(PinHostRoute(connection, trusted_ip, gateway));

  static const char kGateway[] = "10.242.2.13";
  ASSERT_TRUE(gateway.SetAddressFromString(kGateway));

  // Should fail because trusted IP is not set.
  EXPECT_FALSE(PinHostRoute(connection, trusted_ip, gateway));

  static const char kTrustedIP[] = "10.0.1.1/8";
  ASSERT_TRUE(trusted_ip.SetAddressAndPrefixFromString(kTrustedIP));

  // Should pass without calling RequestRouteToHost since if the gateway
  // is not set, there is no work to be done.
  EXPECT_CALL(routing_table_, RequestRouteToHost(_, _, _, _, _)).Times(0);
  EXPECT_TRUE(PinHostRoute(connection, trusted_ip,
                           IPAddress(gateway.family())));
  Mock::VerifyAndClearExpectations(&routing_table_);

  EXPECT_CALL(routing_table_,
              RequestRouteToHost(IsIPAddress(trusted_ip, trusted_ip.prefix()),
                                 -1, kTestDeviceInterfaceIndex0, _,
                                 RT_TABLE_MAIN)).WillOnce(Return(false));
  EXPECT_FALSE(PinHostRoute(connection, trusted_ip, gateway));

  EXPECT_CALL(routing_table_,
              RequestRouteToHost(IsIPAddress(trusted_ip, trusted_ip.prefix()),
                                 -1, kTestDeviceInterfaceIndex0, _,
                                 RT_TABLE_MAIN)).WillOnce(Return(true));
  EXPECT_TRUE(PinHostRoute(connection, trusted_ip, gateway));

  // The destructor will remove the routes and addresses.
  AddDestructorExpectations();
}

TEST_F(ConnectionTest, FixGatewayReachability) {
  ConnectionRefPtr connection = GetNewConnection();
  static const char kLocal[] = "10.242.2.13";
  IPAddress local(IPAddress::kFamilyIPv4);
  ASSERT_TRUE(local.SetAddressFromString(kLocal));
  const int kPrefix = 24;
  local.set_prefix(kPrefix);
  IPAddress gateway(IPAddress::kFamilyIPv4);
  IPAddress peer(IPAddress::kFamilyIPv4);
  IPAddress trusted_ip(IPAddress::kFamilyIPv4);

  // Should fail because no gateway is set.
  EXPECT_FALSE(connection->FixGatewayReachability(
      local, &peer, &gateway, trusted_ip));
  EXPECT_EQ(kPrefix, local.prefix());
  EXPECT_FALSE(peer.IsValid());
  EXPECT_FALSE(gateway.IsValid());

  // Should succeed because with the given prefix, this gateway is reachable.
  static const char kReachableGateway[] = "10.242.2.14";
  ASSERT_TRUE(gateway.SetAddressFromString(kReachableGateway));
  IPAddress gateway_backup(gateway);
  peer = IPAddress(IPAddress::kFamilyIPv4);
  EXPECT_TRUE(connection->FixGatewayReachability(
      local, &peer, &gateway, trusted_ip));
  // Prefix should remain unchanged.
  EXPECT_EQ(kPrefix, local.prefix());
  // Peer should remain unchanged.
  EXPECT_FALSE(peer.IsValid());
  // Gateway should remain unchanged.
  EXPECT_TRUE(gateway_backup.Equals(gateway));

  // Should succeed because we created a link route to the gateway.
  static const char kRemoteGateway[] = "10.242.3.14";
  ASSERT_TRUE(gateway.SetAddressFromString(kRemoteGateway));
  gateway_backup = gateway;
  peer = IPAddress(IPAddress::kFamilyIPv4);
  EXPECT_CALL(routing_table_, AddRoute(kTestDeviceInterfaceIndex0,
                                       IsLinkRouteTo(gateway)))
      .WillOnce(Return(true));
  EXPECT_TRUE(connection->FixGatewayReachability(
      local, &peer, &gateway, trusted_ip));

  // Peer should remain unchanged.
  EXPECT_FALSE(peer.IsValid());
  // Gateway should remain unchanged.
  EXPECT_TRUE(gateway_backup.Equals(gateway));

  // Should fail if AddRoute() fails.
  EXPECT_CALL(routing_table_, AddRoute(kTestDeviceInterfaceIndex0,
                                       IsLinkRouteTo(gateway)))
      .WillOnce(Return(false));
  EXPECT_FALSE(connection->FixGatewayReachability(
      local, &peer, &gateway, trusted_ip));

  // If this is a peer-to-peer interface and the peer matches the gateway,
  // we should succeed.
  local.set_prefix(kPrefix);
  static const char kUnreachableGateway[] = "11.242.2.14";
  ASSERT_TRUE(gateway.SetAddressFromString(kUnreachableGateway));
  gateway_backup = gateway;
  ASSERT_TRUE(peer.SetAddressFromString(kUnreachableGateway));
  EXPECT_TRUE(connection->FixGatewayReachability(
      local, &peer, &gateway, trusted_ip));
  EXPECT_EQ(kPrefix, local.prefix());
  EXPECT_TRUE(peer.Equals(gateway));
  EXPECT_TRUE(gateway_backup.Equals(gateway));

  // If there is a peer specified and it does not match the gateway (even
  // if it was reachable via netmask), we should fail.
  ASSERT_TRUE(gateway.SetAddressFromString(kReachableGateway));
  EXPECT_FALSE(connection->FixGatewayReachability(
      local, &peer, &gateway, trusted_ip));
  EXPECT_EQ(kPrefix, local.prefix());
  EXPECT_FALSE(peer.Equals(gateway));

  // If this is a peer-to-peer interface and the peer matches the gateway,
  // but it also matches the trusted IP address, the gateway and peer address
  // should be modified to allow routing to work correctly.
  ASSERT_TRUE(gateway.SetAddressFromString(kUnreachableGateway));
  ASSERT_TRUE(peer.SetAddressFromString(kUnreachableGateway));
  ASSERT_TRUE(trusted_ip.SetAddressAndPrefixFromString(
      string(kUnreachableGateway) + "/32"));
  EXPECT_TRUE(connection->FixGatewayReachability(
      local, &peer, &gateway, trusted_ip));
  EXPECT_TRUE(peer.IsDefault());
  EXPECT_TRUE(gateway.IsDefault());

  // The destructor will remove the routes and addresses.
  AddDestructorExpectations();
}

TEST_F(ConnectionTest, Binders) {
  EXPECT_TRUE(connection_->binders_.empty());
  DisconnectCallbackTarget target0;
  DisconnectCallbackTarget target1;
  DisconnectCallbackTarget target2;
  DisconnectCallbackTarget target3;
  Connection::Binder binder0("binder0", target0.callback());
  Connection::Binder binder1("binder1", target1.callback());
  Connection::Binder binder2("binder2", target2.callback());
  Connection::Binder binder3("binder3", target3.callback());

  binder0.Attach(connection_);
  binder1.Attach(connection_);

  EXPECT_CALL(target1, CallTarget()).Times(0);
  binder1.Attach(connection_);

  binder3.Attach(connection_);
  binder2.Attach(connection_);

  EXPECT_CALL(target3, CallTarget()).Times(0);
  binder3.Attach(nullptr);

  ASSERT_EQ(3, connection_->binders_.size());
  EXPECT_TRUE(connection_->binders_.at(0) == &binder0);
  EXPECT_TRUE(connection_->binders_.at(1) == &binder1);
  EXPECT_TRUE(connection_->binders_.at(2) == &binder2);

  EXPECT_CALL(target0, CallTarget()).Times(1);
  EXPECT_CALL(target1, CallTarget()).Times(1);
  EXPECT_CALL(target2, CallTarget()).Times(1);
  connection_->NotifyBindersOnDisconnect();
  EXPECT_TRUE(connection_->binders_.empty());

  // Should be a no-op.
  connection_->NotifyBindersOnDisconnect();
}

TEST_F(ConnectionTest, Binder) {
  // No connection should be bound initially.
  Connection::Binder* binder = &connection_->lower_binder_;
  EXPECT_EQ(connection_->interface_name(), binder->name_);
  EXPECT_FALSE(binder->client_disconnect_callback_.is_null());
  EXPECT_FALSE(binder->IsBound());

  ConnectionRefPtr connection1 = GetNewConnection();
  EXPECT_TRUE(connection1->binders_.empty());

  // Bind lower |connection1| and check if it's bound.
  binder->Attach(connection1);
  EXPECT_TRUE(binder->IsBound());
  EXPECT_EQ(connection1.get(), binder->connection().get());
  ASSERT_FALSE(connection1->binders_.empty());
  EXPECT_TRUE(binder == connection1->binders_.at(0));

  // Unbind lower |connection1| and check if it's unbound.
  binder->Attach(nullptr);
  EXPECT_FALSE(binder->IsBound());
  EXPECT_TRUE(connection1->binders_.empty());

  ConnectionRefPtr connection2 = GetNewConnection();

  // Bind lower |connection1| to upper |connection2| and destroy the upper
  // |connection2|. Make sure lower |connection1| is unbound (i.e., the
  // disconnect callback is deregistered).
  connection2->lower_binder_.Attach(connection1);
  EXPECT_FALSE(connection1->binders_.empty());
  AddDestructorExpectations();
  connection2 = nullptr;
  EXPECT_TRUE(connection1->binders_.empty());

  // Bind lower |connection1| to upper |connection_| and destroy lower
  // |connection1|. Make sure lower |connection1| is unbound from upper
  // |connection_| and upper |connection_|'s registered disconnect callbacks are
  // run.
  binder->Attach(connection1);
  DisconnectCallbackTarget target;
  Connection::Binder test_binder("from_test", target.callback());
  test_binder.Attach(connection_);
  EXPECT_CALL(target, CallTarget()).Times(1);
  ASSERT_FALSE(connection_->binders_.empty());
  AddDestructorExpectations();
  connection1 = nullptr;
  EXPECT_FALSE(binder->IsBound());
  EXPECT_FALSE(test_binder.IsBound());
  EXPECT_TRUE(connection_->binders_.empty());

  {
    // Binding a connection to itself should be safe.
    ConnectionRefPtr connection = GetNewConnection();

    connection->lower_binder_.Attach(connection);

    EXPECT_FALSE(connection->binders_.empty());

    DisconnectCallbackTarget target;
    Connection::Binder binder("test", target.callback());
    binder.Attach(connection);

    AddDestructorExpectations();
    EXPECT_CALL(target, CallTarget()).Times(1);
    connection = nullptr;
  }
  {
    // Circular binding of multiple connections should be safe.
    ConnectionRefPtr connection_a = GetNewConnection();
    ConnectionRefPtr connection_b = GetNewConnection();

    connection_a->lower_binder_.Attach(connection_b);
    connection_b->lower_binder_.Attach(connection_a);

    EXPECT_FALSE(connection_a->binders_.empty());
    EXPECT_FALSE(connection_b->binders_.empty());

    DisconnectCallbackTarget target_a;
    DisconnectCallbackTarget target_b;
    Connection::Binder binder_a("test_a", target_a.callback());
    Connection::Binder binder_b("test_b", target_b.callback());
    binder_a.Attach(connection_a);
    binder_b.Attach(connection_b);

    AddDestructorExpectations();
    EXPECT_CALL(target_a, CallTarget()).Times(1);
    EXPECT_CALL(target_b, CallTarget()).Times(1);
    connection_b = nullptr;

    EXPECT_TRUE(connection_a->binders_.empty());

    AddDestructorExpectations();
    connection_a = nullptr;
  }
  {
    // Test the weak pointer to the bound Connection. This is not a case that
    // should occur but the weak pointer should handle it gracefully.
    DisconnectCallbackTarget target;
    Connection::Binder binder("test_weak", target.callback());
    ConnectionRefPtr connection = GetNewConnection();
    binder.Attach(connection);

    // Make sure the connection doesn't notify the binder on destruction.
    connection->binders_.clear();
    AddDestructorExpectations();
    EXPECT_CALL(target, CallTarget()).Times(0);
    connection = nullptr;

    // Ensure no crash -- the weak pointer to connection should be nullptr.
    EXPECT_FALSE(binder.connection());
    binder.Attach(nullptr);
  }
}

TEST_F(ConnectionTest, OnRouteQueryResponse) {
  Connection::Binder* binder = &connection_->lower_binder_;
  ConnectionRefPtr connection = GetNewConnection();
  scoped_refptr<MockDevice> device(new StrictMock<MockDevice>(
      &control_,
      nullptr,
      nullptr,
      nullptr,
      kTestDeviceName1,
      string(),
      kTestDeviceInterfaceIndex1));

  // Make sure we unbind the old lower connection even if we can't lookup the
  // lower connection device.
  binder->Attach(connection);
  scoped_refptr<MockDevice> null_device;
  EXPECT_CALL(*device_info_, GetDevice(kTestDeviceInterfaceIndex1))
      .WillOnce(Return(null_device));
  connection_->OnRouteQueryResponse(
      kTestDeviceInterfaceIndex1, RoutingTableEntry());
  EXPECT_FALSE(binder->IsBound());

  // Check for graceful handling of a connection loop.
  EXPECT_CALL(*device, connection())
      .WillRepeatedly(testing::ReturnRef(connection_));
  EXPECT_CALL(*device_info_, GetDevice(kTestDeviceInterfaceIndex1))
      .WillOnce(Return(device));
  connection_->OnRouteQueryResponse(
      kTestDeviceInterfaceIndex1, RoutingTableEntry());
  EXPECT_FALSE(binder->IsBound());

  // Check for graceful handling of a device with no connection.
  ConnectionRefPtr device_connection;
  EXPECT_CALL(*device, connection())
      .WillRepeatedly(testing::ReturnRef(device_connection));
  EXPECT_CALL(*device_info_, GetDevice(kTestDeviceInterfaceIndex1))
      .WillOnce(Return(device));
  connection_->OnRouteQueryResponse(
      kTestDeviceInterfaceIndex1, RoutingTableEntry());
  EXPECT_FALSE(binder->IsBound());

  // Create a mock connection that will be used for binding.
  scoped_refptr<MockConnection> mock_connection(
      new StrictMock<MockConnection>(device_info_.get()));
  EXPECT_CALL(*device_info_.get(),
      FlushAddresses(mock_connection->interface_index()));
  const string kInterfaceName(kTestDeviceName0);
  EXPECT_CALL(*mock_connection, interface_name())
      .WillRepeatedly(ReturnRef(kInterfaceName));
  device_connection = mock_connection.get();
  EXPECT_CALL(*device, connection())
      .WillRepeatedly(testing::ReturnRef(device_connection));
  EXPECT_CALL(*device_info_, GetDevice(kTestDeviceInterfaceIndex1))
      .WillOnce(Return(device));

  // Check that the binding process completes, causing its upper
  // connection to create a gateway route.
  EXPECT_CALL(*mock_connection, CreateGatewayRoute())
      .WillOnce(Return(true));

  // Ensure that the Device is notified of the change to the connection.
  EXPECT_CALL(*device, OnConnectionUpdated()).Times(1);
  connection_->OnRouteQueryResponse(
      kTestDeviceInterfaceIndex1, RoutingTableEntry());

  // Check that the upper connection is bound to the lower connection.
  EXPECT_TRUE(binder->IsBound());
  EXPECT_EQ(mock_connection.get(), binder->connection().get());

  AddDestructorExpectations();
  connection = nullptr;
}

TEST_F(ConnectionTest, GetCarrierConnection) {
  EXPECT_EQ(connection_.get(), connection_->GetCarrierConnection().get());

  ConnectionRefPtr connection1 = GetNewConnection();
  ConnectionRefPtr connection2 = GetNewConnection();
  ConnectionRefPtr connection3 = GetNewConnection();

  connection_->lower_binder_.Attach(connection1);
  EXPECT_EQ(connection1.get(), connection_->GetCarrierConnection().get());

  connection1->lower_binder_.Attach(connection2);
  EXPECT_EQ(connection2.get(), connection_->GetCarrierConnection().get());

  connection2->lower_binder_.Attach(connection3);
  EXPECT_EQ(connection3.get(), connection_->GetCarrierConnection().get());

  // Create a cycle back to |connection1|.
  connection3->lower_binder_.Attach(connection1);
  EXPECT_EQ(nullptr, connection_->GetCarrierConnection().get());

  AddDestructorExpectations();
  connection3 = nullptr;

  AddDestructorExpectations();
  connection2 = nullptr;

  AddDestructorExpectations();
  connection1 = nullptr;
}

TEST_F(ConnectionTest, GetSubnetName) {
  EXPECT_EQ("", connection_->GetSubnetName());
  IPAddress local("1.2.3.4");
  local.set_prefix(24);
  SetLocal(local);
  EXPECT_EQ("1.2.3.0/24", connection_->GetSubnetName());
}

TEST_F(ConnectionTest, SetMTU) {
  testing::InSequence seq;
  EXPECT_CALL(rtnl_handler_, SetInterfaceMTU(
      kTestDeviceInterfaceIndex0, IPConfig::kDefaultMTU));
  connection_->SetMTU(0);

  EXPECT_CALL(rtnl_handler_, SetInterfaceMTU(
      kTestDeviceInterfaceIndex0, IPConfig::kDefaultMTU));
  connection_->SetMTU(IPConfig::kUndefinedMTU);

  // Test IPv4 minimum MTU.
  SetLocal(local_address_);
  EXPECT_CALL(rtnl_handler_, SetInterfaceMTU(
      kTestDeviceInterfaceIndex0, IPConfig::kMinIPv4MTU));
  connection_->SetMTU(1);

  EXPECT_CALL(rtnl_handler_, SetInterfaceMTU(
      kTestDeviceInterfaceIndex0, IPConfig::kMinIPv4MTU));
  connection_->SetMTU(IPConfig::kMinIPv4MTU - 1);

  EXPECT_CALL(rtnl_handler_, SetInterfaceMTU(
      kTestDeviceInterfaceIndex0, IPConfig::kMinIPv4MTU));
  connection_->SetMTU(IPConfig::kMinIPv4MTU);

  EXPECT_CALL(rtnl_handler_, SetInterfaceMTU(
      kTestDeviceInterfaceIndex0, IPConfig::kMinIPv4MTU + 1));
  connection_->SetMTU(IPConfig::kMinIPv4MTU + 1);

  // Test IPv6 minimum MTU.
  SetLocal(local_ipv6_address_);
  EXPECT_CALL(rtnl_handler_, SetInterfaceMTU(
      kTestDeviceInterfaceIndex0, IPConfig::kMinIPv6MTU));
  connection_->SetMTU(1);

  EXPECT_CALL(rtnl_handler_, SetInterfaceMTU(
      kTestDeviceInterfaceIndex0, IPConfig::kMinIPv6MTU));
  connection_->SetMTU(IPConfig::kMinIPv6MTU - 1);

  EXPECT_CALL(rtnl_handler_, SetInterfaceMTU(
      kTestDeviceInterfaceIndex0, IPConfig::kMinIPv6MTU));
  connection_->SetMTU(IPConfig::kMinIPv6MTU);

  EXPECT_CALL(rtnl_handler_, SetInterfaceMTU(
      kTestDeviceInterfaceIndex0, IPConfig::kMinIPv6MTU + 1));
  connection_->SetMTU(IPConfig::kMinIPv6MTU + 1);
}

}  // namespace shill
