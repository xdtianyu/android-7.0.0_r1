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

#include "shill/dhcp/dhcpv6_config.h"

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
#include "shill/mock_log.h"
#include "shill/mock_metrics.h"
#include "shill/mock_process_manager.h"
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
using testing::InvokeWithoutArgs;
using testing::Mock;
using testing::Return;
using testing::SetArgumentPointee;
using testing::Test;

namespace shill {

namespace {
const char kDeviceName[] = "eth0";
const char kLeaseFileSuffix[] = "leasefilesuffix";
const bool kHasLeaseSuffix = true;
const char kIPAddress[] = "2001:db8:0:1::1";
const char kDelegatedPrefix[] = "2001:db8:0:100::";
}  // namespace

typedef scoped_refptr<DHCPv6Config> DHCPv6ConfigRefPtr;

class DHCPv6ConfigTest : public PropertyStoreTest {
 public:
  DHCPv6ConfigTest()
      : proxy_(new MockDHCPProxy()),
        config_(new DHCPv6Config(&control_,
                                 dispatcher(),
                                 &provider_,
                                 kDeviceName,
                                 kLeaseFileSuffix)) {}

  virtual void SetUp() {
    config_->process_manager_ = &process_manager_;
  }

  bool StartInstance(DHCPv6ConfigRefPtr config) {
    return config->Start();
  }

  void StopInstance() {
    config_->Stop("In test");
  }

  DHCPv6ConfigRefPtr CreateMockMinijailConfig(const string& lease_suffix);
  DHCPv6ConfigRefPtr CreateRunningConfig(const string& lease_suffix);
  void StopRunningConfigAndExpect(DHCPv6ConfigRefPtr config,
                                  bool lease_file_exists);

 protected:
  static const int kPID;
  static const unsigned int kTag;

  FilePath lease_file_;
  FilePath pid_file_;
  ScopedTempDir temp_dir_;
  unique_ptr<MockDHCPProxy> proxy_;
  MockControl control_;
  MockProcessManager process_manager_;
  MockDHCPProvider provider_;
  DHCPv6ConfigRefPtr config_;
};

const int DHCPv6ConfigTest::kPID = 123456;
const unsigned int DHCPv6ConfigTest::kTag = 77;

DHCPv6ConfigRefPtr DHCPv6ConfigTest::CreateMockMinijailConfig(
    const string& lease_suffix) {
  DHCPv6ConfigRefPtr config(new DHCPv6Config(&control_,
                                             dispatcher(),
                                             &provider_,
                                             kDeviceName,
                                             lease_suffix));
  config->process_manager_ = &process_manager_;

  return config;
}

DHCPv6ConfigRefPtr DHCPv6ConfigTest::CreateRunningConfig(
    const string& lease_suffix) {
  DHCPv6ConfigRefPtr config(new DHCPv6Config(&control_,
                                             dispatcher(),
                                             &provider_,
                                             kDeviceName,
                                             lease_suffix));
  config->process_manager_ = &process_manager_;
  EXPECT_CALL(process_manager_, StartProcessInMinijail(_, _, _, _, _, _, _))
      .WillOnce(Return(kPID));
  EXPECT_CALL(provider_, BindPID(kPID, IsRefPtrTo(config)));
  EXPECT_TRUE(config->Start());
  EXPECT_EQ(kPID, config->pid_);

  EXPECT_TRUE(temp_dir_.CreateUniqueTempDir());
  config->root_ = temp_dir_.path();
  FilePath varrun = temp_dir_.path().Append("var/run/dhcpcd");
  EXPECT_TRUE(base::CreateDirectory(varrun));
  pid_file_ = varrun.Append(base::StringPrintf("dhcpcd-%s-6.pid", kDeviceName));
  FilePath varlib = temp_dir_.path().Append("var/lib/dhcpcd");
  EXPECT_TRUE(base::CreateDirectory(varlib));
  lease_file_ =
      varlib.Append(base::StringPrintf("dhcpcd-%s.lease6", kDeviceName));
  EXPECT_EQ(0, base::WriteFile(pid_file_, "", 0));
  EXPECT_EQ(0, base::WriteFile(lease_file_, "", 0));
  EXPECT_TRUE(base::PathExists(pid_file_));
  EXPECT_TRUE(base::PathExists(lease_file_));

  return config;
}

void DHCPv6ConfigTest::StopRunningConfigAndExpect(DHCPv6ConfigRefPtr config,
                                                  bool lease_file_exists) {
  ScopedMockLog log;
  // We use a non-zero exit status so that we get the log message.
  EXPECT_CALL(log, Log(_, _, ::testing::EndsWith("status 10")));
  EXPECT_CALL(provider_, UnbindPID(kPID));
  config->OnProcessExited(10);

  EXPECT_FALSE(base::PathExists(pid_file_));
  EXPECT_EQ(lease_file_exists, base::PathExists(lease_file_));
}

TEST_F(DHCPv6ConfigTest, ParseConfiguration) {
  const char kConfigIPAddress[] = "2001:db8:0:1::129";
  const char kConfigDelegatedPrefix[] = "2001:db8:1:100::";
  const char kConfigNameServer[] = "fec8:0::1";
  const char kConfigDomainSearch[] = "example.domain";
  const uint32_t kConfigDelegatedPrefixLength = 56;
  const uint32_t kConfigIPAddressLeaseTime = 5;
  const uint32_t kConfigDelegatedPrefixLeaseTime = 10;

  KeyValueStore conf;
  conf.SetString(DHCPv6Config::kConfigurationKeyIPAddress, kConfigIPAddress);
  conf.SetUint(DHCPv6Config::kConfigurationKeyIPAddressLeaseTime,
               kConfigIPAddressLeaseTime);
  conf.SetString(DHCPv6Config::kConfigurationKeyDelegatedPrefix,
                 kConfigDelegatedPrefix);
  conf.SetUint(DHCPv6Config::kConfigurationKeyDelegatedPrefixLength,
               kConfigDelegatedPrefixLength);
  conf.SetUint(DHCPv6Config::kConfigurationKeyDelegatedPrefixLeaseTime,
               kConfigDelegatedPrefixLeaseTime);
  {
    vector<string> dns;
    dns.push_back(kConfigNameServer);
    conf.SetStrings(DHCPv6Config::kConfigurationKeyDNS, dns);
  }
  {
    vector<string> domain_search;
    domain_search.push_back(kConfigDomainSearch);
    conf.SetStrings(DHCPv6Config::kConfigurationKeyDomainSearch, domain_search);
  }
  conf.SetString("UnknownKey", "UnknownValue");

  ASSERT_TRUE(config_->ParseConfiguration(conf));
  EXPECT_EQ(kConfigIPAddress, config_->properties_.address);
  EXPECT_EQ(kConfigDelegatedPrefix, config_->properties_.delegated_prefix);
  EXPECT_EQ(kConfigDelegatedPrefixLength,
            config_->properties_.delegated_prefix_length);
  ASSERT_EQ(1, config_->properties_.dns_servers.size());
  EXPECT_EQ(kConfigNameServer, config_->properties_.dns_servers[0]);
  ASSERT_EQ(1, config_->properties_.domain_search.size());
  EXPECT_EQ(kConfigDomainSearch, config_->properties_.domain_search[0]);
  // Use IP address lease time since it is shorter.
  EXPECT_EQ(kConfigIPAddressLeaseTime,
            config_->properties_.lease_duration_seconds);
}

MATCHER_P(IsDHCPCDv6Args, has_lease_suffix, "") {
  if (arg[0] != "-B" ||
      arg[1] != "-q" ||
      arg[2] != "-6" ||
      arg[3] != "-a") {
    return false;
  }

  int end_offset = 4;

  string device_arg = has_lease_suffix ?
      string(kDeviceName) + "=" + string(kLeaseFileSuffix) : kDeviceName;
  return arg[end_offset] == device_arg;
}

TEST_F(DHCPv6ConfigTest, StartDhcpcd) {
  EXPECT_CALL(process_manager_,
              StartProcessInMinijail(_, _, IsDHCPCDv6Args(kHasLeaseSuffix),
                                     _, _, _, _))
      .WillOnce(Return(-1));
  EXPECT_FALSE(StartInstance(config_));
}


namespace {

class DHCPv6ConfigCallbackTest : public DHCPv6ConfigTest {
 public:
  virtual void SetUp() {
    DHCPv6ConfigTest::SetUp();
    config_->RegisterUpdateCallback(
        Bind(&DHCPv6ConfigCallbackTest::SuccessCallback, Unretained(this)));
    config_->RegisterFailureCallback(
        Bind(&DHCPv6ConfigCallbackTest::FailureCallback, Unretained(this)));
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

TEST_F(DHCPv6ConfigCallbackTest, ProcessEventSignalFail) {
  KeyValueStore conf;
  conf.SetString(DHCPv6Config::kConfigurationKeyIPAddress, kIPAddress);
  conf.SetString(DHCPv6Config::kConfigurationKeyDelegatedPrefix,
                 kDelegatedPrefix);
  EXPECT_CALL(*this, SuccessCallback(_, _)).Times(0);
  EXPECT_CALL(*this, FailureCallback(ConfigRef()));
  config_->ProcessEventSignal(DHCPv6Config::kReasonFail, conf);
  Mock::VerifyAndClearExpectations(this);
  EXPECT_TRUE(config_->properties().address.empty());
}

TEST_F(DHCPv6ConfigCallbackTest, ProcessEventSignalSuccess) {
  for (const auto& reason : { DHCPv6Config::kReasonBound,
                              DHCPv6Config::kReasonRebind,
                              DHCPv6Config::kReasonReboot,
                              DHCPv6Config::kReasonRenew }) {
    for (const auto lease_time_given : { false, true }) {
      KeyValueStore conf;
      conf.SetString(DHCPv6Config::kConfigurationKeyIPAddress, kIPAddress);
      conf.SetString(DHCPv6Config::kConfigurationKeyDelegatedPrefix,
                     kDelegatedPrefix);
      if (lease_time_given) {
        const uint32_t kLeaseTime = 1;
        conf.SetUint(DHCPv6Config::kConfigurationKeyIPAddressLeaseTime,
                     kLeaseTime);
      }
      EXPECT_CALL(*this, SuccessCallback(ConfigRef(), true));
      EXPECT_CALL(*this, FailureCallback(_)).Times(0);
      config_->ProcessEventSignal(reason, conf);
      string failure_message = string(reason) + " failed with lease time " +
          (lease_time_given ? "given" : "not given");
      EXPECT_TRUE(Mock::VerifyAndClearExpectations(this)) << failure_message;
      EXPECT_EQ("2001:db8:0:1::1", config_->properties().address)
          << failure_message;
    }
  }
}

TEST_F(DHCPv6ConfigCallbackTest, StoppedDuringFailureCallback) {
  KeyValueStore conf;
  conf.SetString(DHCPv6Config::kConfigurationKeyIPAddress, kIPAddress);
  conf.SetString(DHCPv6Config::kConfigurationKeyDelegatedPrefix,
                 kDelegatedPrefix);
  // Stop the DHCP config while it is calling the failure callback.  We
  // need to ensure that no callbacks are left running inadvertently as
  // a result.
  EXPECT_CALL(*this, FailureCallback(ConfigRef()))
      .WillOnce(InvokeWithoutArgs(this, &DHCPv6ConfigTest::StopInstance));
  config_->ProcessEventSignal(DHCPv6Config::kReasonFail, conf);
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(this));
}

TEST_F(DHCPv6ConfigCallbackTest, StoppedDuringSuccessCallback) {
  KeyValueStore conf;
  conf.SetString(DHCPv6Config::kConfigurationKeyIPAddress, kIPAddress);
  conf.SetString(DHCPv6Config::kConfigurationKeyDelegatedPrefix,
                 kDelegatedPrefix);
  const uint32_t kLeaseTime = 1;
  conf.SetUint(DHCPv6Config::kConfigurationKeyIPAddressLeaseTime, kLeaseTime);
  // Stop the DHCP config while it is calling the success callback.  This
  // can happen if the device has a static IP configuration and releases
  // the lease after accepting other network parameters from the DHCP
  // IPConfig properties.  We need to ensure that no callbacks are left
  // running inadvertently as a result.
  EXPECT_CALL(*this, SuccessCallback(ConfigRef(), true))
      .WillOnce(InvokeWithoutArgs(this, &DHCPv6ConfigTest::StopInstance));
  config_->ProcessEventSignal(DHCPv6Config::kReasonBound, conf);
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(this));
}

TEST_F(DHCPv6ConfigCallbackTest, ProcessEventSignalUnknown) {
  KeyValueStore conf;
  conf.SetString(DHCPv6Config::kConfigurationKeyIPAddress, kIPAddress);
  conf.SetString(DHCPv6Config::kConfigurationKeyDelegatedPrefix,
                 kDelegatedPrefix);
  static const char kReasonUnknown[] = "UNKNOWN_REASON";
  EXPECT_CALL(*this, SuccessCallback(_, _)).Times(0);
  EXPECT_CALL(*this, FailureCallback(_)).Times(0);
  config_->ProcessEventSignal(kReasonUnknown, conf);
  Mock::VerifyAndClearExpectations(this);
  EXPECT_TRUE(config_->properties().address.empty());
}

TEST_F(DHCPv6ConfigTest, StartSuccessEphemeral) {
  DHCPv6ConfigRefPtr config =
      CreateRunningConfig(kDeviceName);
  StopRunningConfigAndExpect(config, false);
}

TEST_F(DHCPv6ConfigTest, StartSuccessPersistent) {
  DHCPv6ConfigRefPtr config =
      CreateRunningConfig(kLeaseFileSuffix);
  StopRunningConfigAndExpect(config, true);
}

}  // namespace shill
