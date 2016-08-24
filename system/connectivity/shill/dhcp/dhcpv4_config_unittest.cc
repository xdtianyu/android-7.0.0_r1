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

#include "shill/dhcp/dhcpv4_config.h"

#include <memory>
#include <string>
#include <vector>

#include <base/bind.h>
#include <base/files/file_util.h>
#include <base/files/scoped_temp_dir.h>
#include <base/strings/stringprintf.h>
#if defined(__ANDROID__)
#include <dbus/service_constants.h>
#else
#include <chromeos/dbus/service_constants.h>
#endif  // __ANDROID__

#include "shill/dhcp/mock_dhcp_provider.h"
#include "shill/dhcp/mock_dhcp_proxy.h"
#include "shill/event_dispatcher.h"
#include "shill/mock_control.h"
#include "shill/mock_dhcp_properties.h"
#include "shill/mock_log.h"
#include "shill/mock_metrics.h"
#include "shill/mock_process_manager.h"
#include "shill/mock_store.h"
#include "shill/property_store_unittest.h"
#include "shill/testing.h"

using base::Bind;
using base::FilePath;
using base::ScopedTempDir;
using base::Unretained;
using std::string;
using std::unique_ptr;
using std::vector;
using testing::_;
using testing::AnyNumber;
using testing::ContainsRegex;
using testing::DoAll;
using testing::InvokeWithoutArgs;
using testing::Mock;
using testing::Return;
using testing::SetArgumentPointee;
using testing::Test;

namespace shill {

namespace {
const char kDeviceName[] = "eth0";
const char kHostName[] = "hostname";
const char kVendorClass[] = "vendorclass";
const char kLeaseFileSuffix[] = "leasefilesuffix";
const bool kArpGateway = true;
const bool kHasHostname = true;
const bool kHasVendorClass = true;
const bool kHasLeaseSuffix = true;
const char kStorageID[] = "dhcp_service_id";
}  // namespace

typedef scoped_refptr<DHCPv4Config> DHCPv4ConfigRefPtr;

class DHCPv4ConfigTest : public PropertyStoreTest {
 public:
  DHCPv4ConfigTest()
      : proxy_(new MockDHCPProxy()),
        metrics_(dispatcher()),
        config_(new DHCPv4Config(&control_,
                                 dispatcher(),
                                 &provider_,
                                 kDeviceName,
                                 kLeaseFileSuffix,
                                 kArpGateway,
                                 dhcp_props_,
                                 &metrics_)) {}

  virtual void SetUp() {
    config_->process_manager_ = &process_manager_;
  }

  bool StartInstance(DHCPv4ConfigRefPtr config) {
    return config->Start();
  }

  void StopInstance() {
    config_->Stop("In test");
  }

  DHCPv4ConfigRefPtr CreateMockMinijailConfig(const string& hostname,
                                              const string& vendorclass,
                                              const string& lease_suffix,
                                              bool arp_gateway);
  DHCPv4ConfigRefPtr CreateRunningConfig(const string& hostname,
                                         const string& vendorclass,
                                         const string& lease_suffix,
                                         bool arp_gateway);
  void StopRunningConfigAndExpect(DHCPv4ConfigRefPtr config,
                                  bool lease_file_exists);

 protected:
  static const int kPID;

  FilePath lease_file_;
  FilePath pid_file_;
  ScopedTempDir temp_dir_;
  unique_ptr<MockDHCPProxy> proxy_;
  MockControl control_;
  MockProcessManager process_manager_;
  MockMetrics metrics_;
  MockDHCPProvider provider_;
  MockDhcpProperties dhcp_props_;
  DHCPv4ConfigRefPtr config_;
};

const int DHCPv4ConfigTest::kPID = 123456;

DHCPv4ConfigRefPtr DHCPv4ConfigTest::CreateMockMinijailConfig(
    const string& hostname,
    const string& vendorclass,
    const string& lease_suffix,
    bool arp_gateway) {
  MockStore storage;
  DhcpProperties dhcp_props;
  if (!hostname.empty()) {
    EXPECT_CALL(storage, GetString(kStorageID, "DHCPProperty.Hostname", _))
        .WillOnce(DoAll(SetArgumentPointee<2>(string(kHostName)),
                        Return(true)));
  } else {
    EXPECT_CALL(storage, GetString(kStorageID, "DHCPProperty.Hostname", _))
        .WillOnce(Return(false));
  }
  if (!vendorclass.empty()) {
    EXPECT_CALL(storage, GetString(kStorageID, "DHCPProperty.VendorClass", _))
        .WillOnce(DoAll(SetArgumentPointee<2>(string(kVendorClass)),
                        Return(true)));
  } else {
    EXPECT_CALL(storage, GetString(kStorageID, "DHCPProperty.VendorClass", _))
        .WillOnce(Return(false));
  }
  dhcp_props.Load(&storage, kStorageID);
  DHCPv4ConfigRefPtr config(new DHCPv4Config(&control_,
                                             dispatcher(),
                                             &provider_,
                                             kDeviceName,
                                             lease_suffix,
                                             arp_gateway,
                                             dhcp_props,
                                             &metrics_));
  config->process_manager_ = &process_manager_;

  return config;
}

DHCPv4ConfigRefPtr DHCPv4ConfigTest::CreateRunningConfig(
    const string& hostname, const string& vendorclass,
    const string& lease_suffix, bool arp_gateway) {
  MockStore storage;
  DhcpProperties dhcp_props;
  if (!hostname.empty()) {
    EXPECT_CALL(storage, GetString(kStorageID, "DHCPProperty.Hostname", _))
        .WillOnce(DoAll(SetArgumentPointee<2>(string(kHostName)),
                        Return(true)));
  } else {
    EXPECT_CALL(storage, GetString(kStorageID, "DHCPProperty.Hostname", _))
        .WillOnce(Return(false));
  }
  if (!vendorclass.empty()) {
    EXPECT_CALL(storage, GetString(kStorageID, "DHCPProperty.VendorClass", _))
        .WillOnce(DoAll(SetArgumentPointee<2>(string(kVendorClass)),
                        Return(true)));
  } else {
    EXPECT_CALL(storage, GetString(kStorageID, "DHCPProperty.VendorClass", _))
        .WillOnce(Return(false));
  }
  dhcp_props.Load(&storage, kStorageID);
  DHCPv4ConfigRefPtr config(new DHCPv4Config(&control_,
                                             dispatcher(),
                                             &provider_,
                                             kDeviceName,
                                             lease_suffix,
                                             arp_gateway,
                                             dhcp_props,
                                             &metrics_));
  config->process_manager_ = &process_manager_;
  EXPECT_CALL(process_manager_, StartProcessInMinijail(_, _, _, _, _, _, _))
      .WillOnce(Return(kPID));
  EXPECT_CALL(provider_, BindPID(kPID, IsRefPtrTo(config)));
  EXPECT_TRUE(config->Start());
  EXPECT_EQ(kPID, config->pid_);
  EXPECT_EQ(config->hostname_, hostname);
  EXPECT_EQ(config->vendor_class_, vendorclass);

  EXPECT_TRUE(temp_dir_.CreateUniqueTempDir());
  config->root_ = temp_dir_.path();
  FilePath varrun = temp_dir_.path().Append("var/run/dhcpcd");
  EXPECT_TRUE(base::CreateDirectory(varrun));
  pid_file_ = varrun.Append(base::StringPrintf("dhcpcd-%s-4.pid", kDeviceName));
  FilePath varlib = temp_dir_.path().Append("var/lib/dhcpcd");
  EXPECT_TRUE(base::CreateDirectory(varlib));
  lease_file_ =
      varlib.Append(base::StringPrintf("dhcpcd-%s.lease", kDeviceName));
  EXPECT_EQ(0, base::WriteFile(pid_file_, "", 0));
  EXPECT_EQ(0, base::WriteFile(lease_file_, "", 0));
  EXPECT_TRUE(base::PathExists(pid_file_));
  EXPECT_TRUE(base::PathExists(lease_file_));

  return config;
}

void DHCPv4ConfigTest::StopRunningConfigAndExpect(DHCPv4ConfigRefPtr config,
                                                  bool lease_file_exists) {
  ScopedMockLog log;
  // We use a non-zero exit status so that we get the log message.
  EXPECT_CALL(log, Log(_, _, ::testing::EndsWith("status 10")));
  EXPECT_CALL(provider_, UnbindPID(kPID));
  config->OnProcessExited(10);

  EXPECT_FALSE(base::PathExists(pid_file_));
  EXPECT_EQ(lease_file_exists, base::PathExists(lease_file_));
}

TEST_F(DHCPv4ConfigTest, GetIPv4AddressString) {
  EXPECT_EQ("255.255.255.255", config_->GetIPv4AddressString(0xffffffff));
  EXPECT_EQ("0.0.0.0", config_->GetIPv4AddressString(0));
  EXPECT_EQ("1.2.3.4", config_->GetIPv4AddressString(0x04030201));
}

TEST_F(DHCPv4ConfigTest, ParseClasslessStaticRoutes) {
  const string kDefaultAddress = "0.0.0.0";
  const string kDefaultDestination = kDefaultAddress + "/0";
  const string kRouter0 = "10.0.0.254";
  const string kAddress1 = "192.168.1.0";
  const string kDestination1 = kAddress1 + "/24";
  // Last gateway missing, leaving an odd number of parameters.
  const string kBrokenClasslessRoutes0 = kDefaultDestination + " " + kRouter0 +
      " " + kDestination1;
  IPConfig::Properties properties;
  EXPECT_FALSE(DHCPv4Config::ParseClasslessStaticRoutes(kBrokenClasslessRoutes0,
                                                        &properties));
  EXPECT_TRUE(properties.routes.empty());
  EXPECT_TRUE(properties.gateway.empty());

  // Gateway argument for the second route is malformed, but we were able
  // to salvage a default gateway.
  const string kBrokenRouter1 = "10.0.0";
  const string kBrokenClasslessRoutes1 = kBrokenClasslessRoutes0 + " " +
      kBrokenRouter1;
  EXPECT_FALSE(DHCPv4Config::ParseClasslessStaticRoutes(kBrokenClasslessRoutes1,
                                                        &properties));
  EXPECT_TRUE(properties.routes.empty());
  EXPECT_EQ(kRouter0, properties.gateway);

  const string kRouter1 = "10.0.0.253";
  const string kRouter2 = "10.0.0.252";
  const string kClasslessRoutes0 = kDefaultDestination + " " + kRouter2 + " " +
      kDestination1 + " " + kRouter1;
  EXPECT_TRUE(DHCPv4Config::ParseClasslessStaticRoutes(kClasslessRoutes0,
                                                       &properties));
  // The old default route is preserved.
  EXPECT_EQ(kRouter0, properties.gateway);

  // The two routes (including the one which would have otherwise been
  // classified as a default route) are added to the routing table.
  EXPECT_EQ(2, properties.routes.size());
  const IPConfig::Route& route0 = properties.routes[0];
  EXPECT_EQ(kDefaultAddress, route0.host);
  EXPECT_EQ("0.0.0.0", route0.netmask);
  EXPECT_EQ(kRouter2, route0.gateway);

  const IPConfig::Route& route1 = properties.routes[1];
  EXPECT_EQ(kAddress1, route1.host);
  EXPECT_EQ("255.255.255.0", route1.netmask);
  EXPECT_EQ(kRouter1, route1.gateway);

  // A malformed routing table should not affect the current table.
  EXPECT_FALSE(DHCPv4Config::ParseClasslessStaticRoutes(kBrokenClasslessRoutes1,
                                                        &properties));
  EXPECT_EQ(2, properties.routes.size());
  EXPECT_EQ(kRouter0, properties.gateway);
}

TEST_F(DHCPv4ConfigTest, ParseConfiguration) {
  KeyValueStore conf;
  conf.SetUint(DHCPv4Config::kConfigurationKeyIPAddress, 0x01020304);
  conf.SetUint8(DHCPv4Config::kConfigurationKeySubnetCIDR, 16);
  conf.SetUint(DHCPv4Config::kConfigurationKeyBroadcastAddress, 0x10203040);
  {
    vector<uint32_t> routers;
    routers.push_back(0x02040608);
    routers.push_back(0x03050709);
    conf.SetUint32s(DHCPv4Config::kConfigurationKeyRouters, routers);
  }
  {
    vector<uint32_t> dns;
    dns.push_back(0x09070503);
    dns.push_back(0x08060402);
    conf.SetUint32s(DHCPv4Config::kConfigurationKeyDNS, dns);
  }
  conf.SetString(DHCPv4Config::kConfigurationKeyDomainName, "domain-name");
  {
    vector<string> search;
    search.push_back("foo.com");
    search.push_back("bar.com");
    conf.SetStrings(DHCPv4Config::kConfigurationKeyDomainSearch, search);
  }
  conf.SetUint16(DHCPv4Config::kConfigurationKeyMTU, 600);
  conf.SetString(DHCPv4Config::kConfigurationKeyHostname, "hostname");
  conf.SetString("UnknownKey", "UnknownValue");

  EXPECT_CALL(metrics_,
              SendSparseToUMA(Metrics::kMetricDhcpClientMTUValue, 600));
  IPConfig::Properties properties;
  ASSERT_TRUE(config_->ParseConfiguration(conf, &properties));
  EXPECT_EQ("4.3.2.1", properties.address);
  EXPECT_EQ(16, properties.subnet_prefix);
  EXPECT_EQ("64.48.32.16", properties.broadcast_address);
  EXPECT_EQ("8.6.4.2", properties.gateway);
  ASSERT_EQ(2, properties.dns_servers.size());
  EXPECT_EQ("3.5.7.9", properties.dns_servers[0]);
  EXPECT_EQ("2.4.6.8", properties.dns_servers[1]);
  EXPECT_EQ("domain-name", properties.domain_name);
  ASSERT_EQ(2, properties.domain_search.size());
  EXPECT_EQ("foo.com", properties.domain_search[0]);
  EXPECT_EQ("bar.com", properties.domain_search[1]);
  EXPECT_EQ(600, properties.mtu);
  EXPECT_EQ("hostname", properties.accepted_hostname);
}

TEST_F(DHCPv4ConfigTest, ParseConfigurationWithMinimumMTU) {
  // Even without a minimum MTU set, we should ignore a 576 value.
  KeyValueStore conf;
  conf.SetUint16(DHCPv4Config::kConfigurationKeyMTU, 576);

  IPConfig::Properties properties;
  EXPECT_CALL(metrics_,
              SendSparseToUMA(Metrics::kMetricDhcpClientMTUValue, 576));
  ASSERT_TRUE(config_->ParseConfiguration(conf, &properties));
  EXPECT_EQ(IPConfig::kUndefinedMTU, properties.mtu);
  Mock::VerifyAndClearExpectations(&metrics_);

  // With a minimum MTU set, values below the minimum should be ignored.
  config_->set_minimum_mtu(1500);
  conf.RemoveUint16(DHCPv4Config::kConfigurationKeyMTU);
  conf.SetUint16(DHCPv4Config::kConfigurationKeyMTU, 1499);
  EXPECT_CALL(metrics_,
              SendSparseToUMA(Metrics::kMetricDhcpClientMTUValue, 1499));
  ASSERT_TRUE(config_->ParseConfiguration(conf, &properties));
  EXPECT_EQ(IPConfig::kUndefinedMTU, properties.mtu);
  Mock::VerifyAndClearExpectations(&metrics_);

  // A value (other than 576) should be accepted if it is >= mimimum_mtu.
  config_->set_minimum_mtu(577);
  conf.RemoveUint16(DHCPv4Config::kConfigurationKeyMTU);
  conf.SetUint16(DHCPv4Config::kConfigurationKeyMTU, 577);
  EXPECT_CALL(metrics_,
              SendSparseToUMA(Metrics::kMetricDhcpClientMTUValue, 577));
  ASSERT_TRUE(config_->ParseConfiguration(conf, &properties));
  EXPECT_EQ(577, properties.mtu);
}

MATCHER_P4(IsDHCPCDArgs,
           has_hostname,
           has_vendorclass,
           has_arp_gateway,
           has_lease_suffix, "") {
  if (arg[0] != "-B" ||
      arg[1] != "-q" ||
      arg[2] != "-4") {
    return false;
  }

  int end_offset = 3;
  if (has_hostname) {
    if (arg[end_offset] != "-h" ||
        arg[end_offset + 1] != kHostName) {
      return false;
    }
    end_offset += 2;
  }

  if (has_vendorclass){
    if (arg[end_offset] != "-i" ||
        arg[end_offset + 1] != kVendorClass) {
      return false;
    }
    end_offset += 2;
  }

  if (has_arp_gateway) {
    if (arg[end_offset] != "-R" ||
        arg[end_offset + 1] != "-P") {
      return false;
    }
    end_offset += 2;
  }

  string device_arg = has_lease_suffix ?
      string(kDeviceName) + "=" + string(kLeaseFileSuffix) : kDeviceName;
  return arg[end_offset] == device_arg;
}

TEST_F(DHCPv4ConfigTest, StartWithHostname) {
  config_->hostname_ = kHostName;
  EXPECT_CALL(process_manager_,
              StartProcessInMinijail(_, _,
                                     IsDHCPCDArgs(kHasHostname,
                                                  !kHasVendorClass,
                                                  kArpGateway,
                                                  kHasLeaseSuffix), _, _, _, _))
      .WillOnce(Return(-1));
  EXPECT_FALSE(StartInstance(config_));
}

TEST_F(DHCPv4ConfigTest, StartWithoutHostname) {
  DHCPv4ConfigRefPtr config = CreateMockMinijailConfig("",
                                                       "",
                                                       kLeaseFileSuffix,
                                                       kArpGateway);
  EXPECT_CALL(process_manager_,
              StartProcessInMinijail(_, _,
                                     IsDHCPCDArgs(!kHasHostname,
                                                  !kHasVendorClass,
                                                  kArpGateway,
                                                  kHasLeaseSuffix), _, _, _, _))
      .WillOnce(Return(-1));
  EXPECT_FALSE(StartInstance(config));
}

TEST_F(DHCPv4ConfigTest, StartWithEmptyHostname) {
  DHCPv4ConfigRefPtr config = CreateMockMinijailConfig("",
                                                       "",
                                                       kLeaseFileSuffix,
                                                       kArpGateway);
  EXPECT_CALL(process_manager_,
              StartProcessInMinijail(_, _,
                                     IsDHCPCDArgs(!kHasHostname,
                                                  !kHasVendorClass,
                                                  kArpGateway,
                                                  kHasLeaseSuffix), _, _, _, _))
      .WillOnce(Return(-1));
  EXPECT_FALSE(StartInstance(config));
}

TEST_F(DHCPv4ConfigTest, StartWithVendorClass) {
  config_->hostname_ = kHostName;
  config_->vendor_class_ = kVendorClass;
  EXPECT_CALL(process_manager_,
              StartProcessInMinijail(_, _,
                                     IsDHCPCDArgs(kHasHostname,
                                                  kHasVendorClass,
                                                  kArpGateway,
                                                  kHasLeaseSuffix), _, _, _, _))
      .WillOnce(Return(-1));
  EXPECT_FALSE(StartInstance(config_));
}

TEST_F(DHCPv4ConfigTest, StartWithoutVendorClass) {
  DHCPv4ConfigRefPtr config = CreateMockMinijailConfig(kHostName,
                                                       "",
                                                       kLeaseFileSuffix,
                                                       kArpGateway);
  EXPECT_CALL(process_manager_,
              StartProcessInMinijail(_, _,
                                     IsDHCPCDArgs(kHasHostname,
                                                  !kHasVendorClass,
                                                  kArpGateway,
                                                  kHasLeaseSuffix), _, _, _, _))
      .WillOnce(Return(-1));
  EXPECT_FALSE(StartInstance(config));
}


TEST_F(DHCPv4ConfigTest, StartWithoutArpGateway) {
  DHCPv4ConfigRefPtr config = CreateMockMinijailConfig(kHostName,
                                                       "",
                                                       kLeaseFileSuffix,
                                                       !kArpGateway);
  EXPECT_CALL(process_manager_,
              StartProcessInMinijail(_, _,
                                     IsDHCPCDArgs(kHasHostname,
                                                  !kHasVendorClass,
                                                  !kArpGateway,
                                                  kHasLeaseSuffix), _, _, _, _))
      .WillOnce(Return(-1));
  EXPECT_FALSE(StartInstance(config));
}

namespace {

class DHCPv4ConfigCallbackTest : public DHCPv4ConfigTest {
 public:
  virtual void SetUp() {
    DHCPv4ConfigTest::SetUp();
    config_->RegisterUpdateCallback(
        Bind(&DHCPv4ConfigCallbackTest::SuccessCallback, Unretained(this)));
    config_->RegisterFailureCallback(
        Bind(&DHCPv4ConfigCallbackTest::FailureCallback, Unretained(this)));
    ip_config_ = config_;
  }

  MOCK_METHOD2(SuccessCallback,
               void(const IPConfigRefPtr& ipconfig, bool new_lease_acquired));
  MOCK_METHOD1(FailureCallback, void(const IPConfigRefPtr& ipconfig));

  // The mock methods above take IPConfigRefPtr because this is the type
  // that the registered callbacks take.  This conversion of the DHCP
  // config ref pointer eases our work in setting up expectations.
  const IPConfigRefPtr& ConfigRef() { return ip_config_; }

 private:
  IPConfigRefPtr ip_config_;
};

}  // namespace

TEST_F(DHCPv4ConfigCallbackTest, ProcessEventSignalFail) {
  KeyValueStore conf;
  conf.SetUint(DHCPv4Config::kConfigurationKeyIPAddress, 0x01020304);
  EXPECT_CALL(*this, SuccessCallback(_, _)).Times(0);
  EXPECT_CALL(*this, FailureCallback(ConfigRef()));
  config_->ProcessEventSignal(DHCPv4Config::kReasonFail, conf);
  Mock::VerifyAndClearExpectations(this);
  EXPECT_TRUE(config_->properties().address.empty());
}

TEST_F(DHCPv4ConfigCallbackTest, ProcessEventSignalSuccess) {
  for (const auto& reason : { DHCPv4Config::kReasonBound,
                              DHCPv4Config::kReasonRebind,
                              DHCPv4Config::kReasonReboot,
                              DHCPv4Config::kReasonRenew }) {
    int address_octet = 0;
    for (const auto lease_time_given : { false, true }) {
      KeyValueStore conf;
      conf.SetUint(DHCPv4Config::kConfigurationKeyIPAddress, ++address_octet);
      if (lease_time_given) {
        const uint32_t kLeaseTime = 1;
        conf.SetUint(DHCPv4Config::kConfigurationKeyLeaseTime, kLeaseTime);
      }
      EXPECT_CALL(*this, SuccessCallback(ConfigRef(), true));
      EXPECT_CALL(*this, FailureCallback(_)).Times(0);
      config_->ProcessEventSignal(reason, conf);
      string failure_message = string(reason) + " failed with lease time " +
          (lease_time_given ? "given" : "not given");
      EXPECT_TRUE(Mock::VerifyAndClearExpectations(this)) << failure_message;
      EXPECT_EQ(base::StringPrintf("%d.0.0.0", address_octet),
                config_->properties().address) << failure_message;
    }
  }
}

TEST_F(DHCPv4ConfigCallbackTest, StoppedDuringFailureCallback) {
  KeyValueStore conf;
  conf.SetUint(DHCPv4Config::kConfigurationKeyIPAddress, 0x01020304);
  // Stop the DHCP config while it is calling the failure callback.  We
  // need to ensure that no callbacks are left running inadvertently as
  // a result.
  EXPECT_CALL(*this, FailureCallback(ConfigRef()))
      .WillOnce(InvokeWithoutArgs(this, &DHCPv4ConfigTest::StopInstance));
  config_->ProcessEventSignal(DHCPv4Config::kReasonFail, conf);
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(this));
}

TEST_F(DHCPv4ConfigCallbackTest, StoppedDuringSuccessCallback) {
  KeyValueStore conf;
  conf.SetUint(DHCPv4Config::kConfigurationKeyIPAddress, 0x01020304);
  const uint32_t kLeaseTime = 1;
  conf.SetUint(DHCPv4Config::kConfigurationKeyLeaseTime, kLeaseTime);
  // Stop the DHCP config while it is calling the success callback.  This
  // can happen if the device has a static IP configuration and releases
  // the lease after accepting other network parameters from the DHCP
  // IPConfig properties.  We need to ensure that no callbacks are left
  // running inadvertently as a result.
  EXPECT_CALL(*this, SuccessCallback(ConfigRef(), true))
      .WillOnce(InvokeWithoutArgs(this, &DHCPv4ConfigTest::StopInstance));
  config_->ProcessEventSignal(DHCPv4Config::kReasonBound, conf);
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(this));
}

TEST_F(DHCPv4ConfigCallbackTest, ProcessEventSignalUnknown) {
  KeyValueStore conf;
  conf.SetUint(DHCPv4Config::kConfigurationKeyIPAddress, 0x01020304);
  static const char kReasonUnknown[] = "UNKNOWN_REASON";
  EXPECT_CALL(*this, SuccessCallback(_, _)).Times(0);
  EXPECT_CALL(*this, FailureCallback(_)).Times(0);
  config_->ProcessEventSignal(kReasonUnknown, conf);
  Mock::VerifyAndClearExpectations(this);
  EXPECT_TRUE(config_->properties().address.empty());
}

TEST_F(DHCPv4ConfigCallbackTest, ProcessEventSignalGatewayArp) {
  KeyValueStore conf;
  conf.SetUint(DHCPv4Config::kConfigurationKeyIPAddress, 0x01020304);
  EXPECT_CALL(*this, SuccessCallback(ConfigRef(), false));
  EXPECT_CALL(*this, FailureCallback(_)).Times(0);
  EXPECT_CALL(process_manager_, StartProcessInMinijail(_, _, _, _, _, _, _))
      .WillOnce(Return(0));
  StartInstance(config_);
  config_->ProcessEventSignal(DHCPv4Config::kReasonGatewayArp, conf);
  Mock::VerifyAndClearExpectations(this);
  EXPECT_EQ("4.3.2.1", config_->properties().address);
  EXPECT_TRUE(config_->is_gateway_arp_active_);
  // Will not fail on acquisition timeout since Gateway ARP is active.
  EXPECT_FALSE(config_->ShouldFailOnAcquisitionTimeout());

  // An official reply from a DHCP server should reset our GatewayArp state.
  EXPECT_CALL(*this, SuccessCallback(ConfigRef(), true));
  EXPECT_CALL(*this, FailureCallback(_)).Times(0);
  config_->ProcessEventSignal(DHCPv4Config::kReasonRenew, conf);
  Mock::VerifyAndClearExpectations(this);
  EXPECT_FALSE(config_->is_gateway_arp_active_);
  // Will fail on acquisition timeout since Gateway ARP is not active.
  EXPECT_TRUE(config_->ShouldFailOnAcquisitionTimeout());
}

TEST_F(DHCPv4ConfigCallbackTest, ProcessEventSignalGatewayArpNak) {
  KeyValueStore conf;
  conf.SetUint(DHCPv4Config::kConfigurationKeyIPAddress, 0x01020304);
  EXPECT_CALL(process_manager_, StartProcessInMinijail(_, _, _, _, _, _, _))
      .WillOnce(Return(0));
  StartInstance(config_);
  config_->ProcessEventSignal(DHCPv4Config::kReasonGatewayArp, conf);
  EXPECT_TRUE(config_->is_gateway_arp_active_);

  // Sending a NAK should clear is_gateway_arp_active_.
  config_->ProcessEventSignal(DHCPv4Config::kReasonNak, conf);
  EXPECT_FALSE(config_->is_gateway_arp_active_);
  // Will fail on acquisition timeout since Gateway ARP is not active.
  EXPECT_TRUE(config_->ShouldFailOnAcquisitionTimeout());
  Mock::VerifyAndClearExpectations(this);
}

TEST_F(DHCPv4ConfigTest, ProcessStatusChangeSingal) {
  EXPECT_CALL(metrics_, NotifyDhcpClientStatus(
      Metrics::kDhcpClientStatusBound));
  config_->ProcessStatusChangeSignal(DHCPv4Config::kStatusBound);
}

TEST_F(DHCPv4ConfigTest, StartSuccessEphemeral) {
  DHCPv4ConfigRefPtr config =
      CreateRunningConfig(kHostName, kVendorClass, kDeviceName, kArpGateway);
  StopRunningConfigAndExpect(config, false);
}

TEST_F(DHCPv4ConfigTest, StartSuccessPersistent) {
  DHCPv4ConfigRefPtr config =
      CreateRunningConfig(kHostName, kVendorClass,
                          kLeaseFileSuffix, kArpGateway);
  StopRunningConfigAndExpect(config, true);
}

}  // namespace shill
