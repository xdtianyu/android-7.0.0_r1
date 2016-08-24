//
// Copyright (C) 2014 The Android Open Source Project
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

#include "apmanager/dhcp_server.h"

#include <string>

#include <net/if.h>

#include <base/strings/string_util.h>
#include <base/strings/stringprintf.h>
#include <brillo/process_mock.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <shill/net/mock_rtnl_handler.h>

#include "apmanager/mock_file_writer.h"
#include "apmanager/mock_process_factory.h"

using brillo::ProcessMock;
using ::testing::_;
using ::testing::Mock;
using ::testing::Return;
using std::string;

namespace {
  const uint16_t kServerAddressIndex = 1;
  const char kTestInterfaceName[] = "test_interface";
  const char kExpectedDnsmasqConfigFile[] =
      "port=0\n"
      "bind-interfaces\n"
      "log-dhcp\n"
      "keep-in-foreground\n"
      "dhcp-range=192.168.1.1,192.168.1.128\n"
      "interface=test_interface\n"
#if !defined(__ANDROID__)
      "user=apmanager\n"
      "dhcp-leasefile=/var/run/apmanager/dnsmasq/dhcpd-1.leases\n";
#else
      "user=system\n"
      "dhcp-leasefile=/data/misc/apmanager/dnsmasq/dhcpd-1.leases\n";
#endif  // __ANDROID__

#if !defined(__ANDROID__)
  const char kBinSleep[] = "/bin/sleep";
  const char kDnsmasqConfigFilePath[] =
      "/var/run/apmanager/dnsmasq/dhcpd-1.conf";
#else
  const char kBinSleep[] = "/system/bin/sleep";
  const char kDnsmasqConfigFilePath[] =
      "/data/misc/apmanager/dnsmasq/dhcpd-1.conf";
#endif  // __ANDROID__
}  // namespace

namespace apmanager {

class DHCPServerTest : public testing::Test {
 public:
  DHCPServerTest()
      : dhcp_server_(new DHCPServer(kServerAddressIndex, kTestInterfaceName)),
        rtnl_handler_(new shill::MockRTNLHandler()) {}
  virtual ~DHCPServerTest() {}

  virtual void SetUp() {
    dhcp_server_->rtnl_handler_ = rtnl_handler_.get();
    dhcp_server_->file_writer_ = &file_writer_;
    dhcp_server_->process_factory_ = &process_factory_;
  }

  virtual void TearDown() {
    // Reset DHCP server now while RTNLHandler is still valid.
    dhcp_server_.reset();
  }

  void StartDummyProcess() {
    dhcp_server_->dnsmasq_process_.reset(new brillo::ProcessImpl);
    dhcp_server_->dnsmasq_process_->AddArg(kBinSleep);
    dhcp_server_->dnsmasq_process_->AddArg("12345");
    CHECK(dhcp_server_->dnsmasq_process_->Start());
  }

  string GenerateConfigFile() {
    return dhcp_server_->GenerateConfigFile();
  }

 protected:
  std::unique_ptr<DHCPServer> dhcp_server_;
  std::unique_ptr<shill::MockRTNLHandler> rtnl_handler_;
  MockFileWriter file_writer_;
  MockProcessFactory process_factory_;
};


TEST_F(DHCPServerTest, GenerateConfigFile) {
  string config_content = GenerateConfigFile();
  EXPECT_STREQ(kExpectedDnsmasqConfigFile, config_content.c_str())
      << "Expected to find the following config...\n"
      << kExpectedDnsmasqConfigFile << ".....\n"
      << config_content;
}

TEST_F(DHCPServerTest, StartWhenServerAlreadyStarted) {
  StartDummyProcess();

  EXPECT_FALSE(dhcp_server_->Start());
}

TEST_F(DHCPServerTest, StartSuccess) {
  ProcessMock* process = new ProcessMock();

  const int kInterfaceIndex = 1;
  EXPECT_CALL(file_writer_,
              Write(kDnsmasqConfigFilePath, kExpectedDnsmasqConfigFile))
      .WillOnce(Return(true));
  EXPECT_CALL(*rtnl_handler_.get(), GetInterfaceIndex(kTestInterfaceName))
      .WillOnce(Return(kInterfaceIndex));
  EXPECT_CALL(*rtnl_handler_.get(),
      AddInterfaceAddress(kInterfaceIndex, _, _, _)).Times(1);
  EXPECT_CALL(*rtnl_handler_.get(),
      SetInterfaceFlags(kInterfaceIndex, IFF_UP, IFF_UP)).Times(1);
  EXPECT_CALL(process_factory_, CreateProcess()).WillOnce(Return(process));
  EXPECT_CALL(*process, Start()).WillOnce(Return(true));
  EXPECT_TRUE(dhcp_server_->Start());
  Mock::VerifyAndClearExpectations(rtnl_handler_.get());
}

}  // namespace apmanager
