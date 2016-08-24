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

#include "shill/vpn/third_party_vpn_driver.h"

#include <gtest/gtest.h>

#include "shill/mock_adaptors.h"
#include "shill/mock_device_info.h"
#include "shill/mock_event_dispatcher.h"
#include "shill/mock_file_io.h"
#include "shill/mock_manager.h"
#include "shill/mock_metrics.h"
#include "shill/mock_service.h"
#include "shill/mock_store.h"
#include "shill/mock_virtual_device.h"
#include "shill/nice_mock_control.h"
#include "shill/vpn/mock_vpn_service.h"

using testing::_;
using testing::Mock;
using testing::NiceMock;
using testing::Return;
using testing::SetArgumentPointee;

namespace shill {

class ThirdPartyVpnDriverTest : public testing::Test {
 public:
  ThirdPartyVpnDriverTest()
      : device_info_(&control_, &dispatcher_, &metrics_, &manager_),
        metrics_(&dispatcher_),
        manager_(&control_, &dispatcher_, &metrics_),
        driver_(new ThirdPartyVpnDriver(&control_, &dispatcher_, &metrics_,
                                        &manager_, &device_info_)),
        adaptor_interface_(new ThirdPartyVpnMockAdaptor()),
        service_(new MockVPNService(&control_, &dispatcher_, &metrics_,
                                    &manager_, driver_)),
        device_(new MockVirtualDevice(&control_, &dispatcher_, &metrics_,
                                      &manager_, kInterfaceName,
                                      kInterfaceIndex, Technology::kVPN)) {}

  virtual ~ThirdPartyVpnDriverTest() {}

  virtual void SetUp() {
    driver_->adaptor_interface_.reset(adaptor_interface_);
    driver_->file_io_ = &mock_file_io_;
  }

  virtual void TearDown() {
    driver_->device_ = nullptr;
    driver_->service_ = nullptr;
    driver_->file_io_ = nullptr;
  }

 protected:
  static const char kConfigName[];
  static const char kInterfaceName[];
  static const int kInterfaceIndex;

  NiceMockControl control_;
  NiceMock<MockDeviceInfo> device_info_;
  MockEventDispatcher dispatcher_;
  MockMetrics metrics_;
  MockFileIO mock_file_io_;
  MockManager manager_;
  ThirdPartyVpnDriver* driver_;                  // Owned by |service_|
  ThirdPartyVpnMockAdaptor* adaptor_interface_;  // Owned by |driver_|
  scoped_refptr<MockVPNService> service_;
  scoped_refptr<MockVirtualDevice> device_;
};

const char ThirdPartyVpnDriverTest::kConfigName[] = "default-1";
const char ThirdPartyVpnDriverTest::kInterfaceName[] = "tun0";
const int ThirdPartyVpnDriverTest::kInterfaceIndex = 123;

TEST_F(ThirdPartyVpnDriverTest, ConnectAndDisconnect) {
  const std::string interface = kInterfaceName;
  IOHandler* io_handler = new IOHandler();  // Owned by |driver_|
  int fd = 1;

  EXPECT_CALL(*service_, SetState(Service::kStateConfiguring)).Times(1);
  EXPECT_CALL(device_info_, CreateTunnelInterface(_))
      .WillOnce(DoAll(SetArgumentPointee<0>(interface), Return(true)));
  Error error;
  driver_->Connect(service_, &error);
  EXPECT_TRUE(error.IsSuccess());
  EXPECT_EQ(kInterfaceName, driver_->tunnel_interface_);
  EXPECT_TRUE(driver_->IsConnectTimeoutStarted());

  EXPECT_CALL(device_info_, OpenTunnelInterface(interface))
      .WillOnce(Return(fd));
  EXPECT_CALL(dispatcher_, CreateInputHandler(fd, _, _))
      .WillOnce(Return(io_handler));
  EXPECT_CALL(*adaptor_interface_, EmitPlatformMessage(static_cast<uint32_t>(
                                       ThirdPartyVpnDriver::kConnected)));
  EXPECT_FALSE(driver_->ClaimInterface("eth1", kInterfaceIndex));
  EXPECT_TRUE(driver_->ClaimInterface(interface, kInterfaceIndex));
  EXPECT_EQ(driver_->active_client_, driver_);
  EXPECT_TRUE(driver_->parameters_expected_);
  EXPECT_EQ(driver_->io_handler_.get(), io_handler);
  ASSERT_TRUE(driver_->device_);
  EXPECT_EQ(kInterfaceIndex, driver_->device_->interface_index());

  EXPECT_CALL(*service_, SetState(Service::kStateIdle)).Times(1);
  EXPECT_CALL(*adaptor_interface_, EmitPlatformMessage(static_cast<uint32_t>(
                                       ThirdPartyVpnDriver::kDisconnected)));
  EXPECT_CALL(mock_file_io_, Close(fd));
  driver_->Disconnect();
  EXPECT_EQ(driver_->io_handler_.get(), nullptr);
}

TEST_F(ThirdPartyVpnDriverTest, SendPacket) {
  int fd = 1;
  std::string error;
  std::vector<uint8_t> ip_packet(5, 0);
  driver_->SendPacket(ip_packet, &error);
  EXPECT_EQ(error, "Unexpected call");

  error.clear();
  ThirdPartyVpnDriver::active_client_ = driver_;
  driver_->SendPacket(ip_packet, &error);
  EXPECT_EQ(error, "Device not open");

  driver_->tun_fd_ = fd;
  error.clear();
  EXPECT_CALL(mock_file_io_, Write(fd, ip_packet.data(), ip_packet.size()))
      .WillOnce(Return(ip_packet.size() - 1));
  EXPECT_CALL(
      *adaptor_interface_,
      EmitPlatformMessage(static_cast<uint32_t>(ThirdPartyVpnDriver::kError)));
  driver_->SendPacket(ip_packet, &error);
  EXPECT_EQ(error, "Partial write");

  error.clear();
  EXPECT_CALL(mock_file_io_, Write(fd, ip_packet.data(), ip_packet.size()))
      .WillOnce(Return(ip_packet.size()));
  driver_->SendPacket(ip_packet, &error);
  EXPECT_TRUE(error.empty());

  driver_->tun_fd_ = -1;

  EXPECT_CALL(*adaptor_interface_, EmitPlatformMessage(static_cast<uint32_t>(
                                       ThirdPartyVpnDriver::kDisconnected)));
}

TEST_F(ThirdPartyVpnDriverTest, UpdateConnectionState) {
  std::string error;
  driver_->UpdateConnectionState(Service::kStateConfiguring, &error);
  EXPECT_EQ(error, "Unexpected call");

  error.clear();
  ThirdPartyVpnDriver::active_client_ = driver_;
  driver_->UpdateConnectionState(Service::kStateConfiguring, &error);
  EXPECT_EQ(error, "Invalid argument");

  error.clear();
  driver_->service_ = service_;
  EXPECT_CALL(*service_, SetState(_)).Times(0);
  driver_->UpdateConnectionState(Service::kStateOnline, &error);
  EXPECT_TRUE(error.empty());
  Mock::VerifyAndClearExpectations(service_.get());

  EXPECT_CALL(*service_, SetState(Service::kStateFailure)).Times(1);
  EXPECT_CALL(*adaptor_interface_, EmitPlatformMessage(static_cast<uint32_t>(
                                       ThirdPartyVpnDriver::kDisconnected)))
      .Times(1);
  driver_->UpdateConnectionState(Service::kStateFailure, &error);
  EXPECT_TRUE(error.empty());
  Mock::VerifyAndClearExpectations(service_.get());
  Mock::VerifyAndClearExpectations(adaptor_interface_);
}

TEST_F(ThirdPartyVpnDriverTest, SetParameters) {
  std::map<std::string, std::string> parameters;
  std::string error;
  std::string warning;
  driver_->SetParameters(parameters, &error, &warning);
  EXPECT_EQ(error, "Unexpected call");

  error.clear();
  ThirdPartyVpnDriver::active_client_ = driver_;
  driver_->parameters_expected_ = true;
  driver_->SetParameters(parameters, &error, &warning);
  EXPECT_EQ(error,
            "address is missing;subnet_prefix is missing;"
            "dns_servers is missing;"
            "exclusion_list is missing;inclusion_list is missing;");
  EXPECT_TRUE(warning.empty());

  error.clear();
  parameters["address"] = "1234.1.1.1";
  driver_->SetParameters(parameters, &error, &warning);
  EXPECT_EQ(error,
            "address is not a valid IP;subnet_prefix is missing;"
            "dns_servers is missing;"
            "exclusion_list is missing;inclusion_list is missing;");
  EXPECT_TRUE(warning.empty());

  error.clear();
  parameters["address"] = "123.211.21.18";
  driver_->SetParameters(parameters, &error, &warning);
  EXPECT_EQ(error,
            "subnet_prefix is missing;dns_servers is missing;"
            "exclusion_list is missing;inclusion_list is missing;");
  EXPECT_TRUE(warning.empty());

  error.clear();
  parameters["subnet_prefix"] = "123";
  driver_->SetParameters(parameters, &error, &warning);
  EXPECT_EQ(error,
            "subnet_prefix not in expected range;dns_servers is missing;"
            "exclusion_list is missing;inclusion_list is missing;");
  EXPECT_TRUE(warning.empty());

  error.clear();
  parameters["subnet_prefix"] = "12";
  driver_->SetParameters(parameters, &error, &warning);
  EXPECT_EQ(error, "dns_servers is missing;"
                   "exclusion_list is missing;inclusion_list is missing;");
  EXPECT_TRUE(warning.empty());

  error.clear();
  parameters["dns_servers"] = "12 123123 43902374";
  driver_->SetParameters(parameters, &error, &warning);
  EXPECT_EQ(error, "dns_servers has no valid values or is empty;"
                   "exclusion_list is missing;inclusion_list is missing;");
  EXPECT_EQ(warning, "12 for dns_servers is invalid;"
                     "123123 for dns_servers is invalid;"
                     "43902374 for dns_servers is invalid;");

  driver_->device_ =
      new MockVirtualDevice(&control_, &dispatcher_, &metrics_, &manager_,
                            kInterfaceName, kInterfaceIndex, Technology::kVPN);
  error.clear();
  warning.clear();
  parameters["exclusion_list"] =
      "400.400.400.400/12 1.1.1.1/44 1.1.1.1/-1 "
      "123.211.21.0/23 123.211.21.1/23 123.211.21.0/25 "
      "1.1.1.1.1/12 1.1.1/13";
  parameters["dns_servers"] = "";
  driver_->SetParameters(parameters, &error, &warning);
  EXPECT_EQ(error, "dns_servers has no valid values or is empty;"
                   "inclusion_list is missing;");
  EXPECT_EQ(warning,
            "400.400.400.400/12 for exclusion_list is invalid;"
            "1.1.1.1/44 for exclusion_list is invalid;"
            "1.1.1.1/-1 for exclusion_list is invalid;"
            "Duplicate entry for 123.211.21.1/23 in exclusion_list found;"
            "1.1.1.1.1/12 for exclusion_list is invalid;"
            "1.1.1/13 for exclusion_list is invalid;");

  error.clear();
  warning.clear();
  parameters["exclusion_list"] = "0.0.0.0/0 123.211.21.29/31 123.211.21.1/24";
  parameters["inclusion_list"] =
      "400.400.400.400/12 1.1.1.1/44 1.1.1.1/-1 "
      "123.211.22.0/24 123.211.22.1/24 "
      "1.1.1.1.1/12 1.1.1/13 123.211.21.0/24";
  driver_->SetParameters(parameters, &error, &warning);
  EXPECT_EQ(error, "dns_servers has no valid values or is empty;");
  EXPECT_EQ(warning,
            "400.400.400.400/12 for inclusion_list is invalid;"
            "1.1.1.1/44 for inclusion_list is invalid;"
            "1.1.1.1/-1 for inclusion_list is invalid;"
            "Duplicate entry for 123.211.22.1/24 in inclusion_list found;"
            "1.1.1.1.1/12 for inclusion_list is invalid;"
            "1.1.1/13 for inclusion_list is invalid;"
            "Duplicate entry for 123.211.21.0/24 in inclusion_list found;");

  error.clear();
  warning.clear();
  parameters["dns_servers"] = "123.211.21.18 123.211.21.19";
  parameters["inclusion_list"] = "123.211.61.29/7 123.211.42.29/17";
  driver_->SetParameters(parameters, &error, &warning);
  EXPECT_EQ(driver_->ip_properties_.exclusion_list.size(), 3);
  EXPECT_EQ(driver_->ip_properties_.exclusion_list[0], "123.211.21.29/31");
  EXPECT_EQ(driver_->ip_properties_.exclusion_list[1], "0.0.0.0/0");
  EXPECT_EQ(driver_->ip_properties_.exclusion_list[2], "123.211.21.1/24");
  EXPECT_EQ(driver_->ip_properties_.routes.size(), 2);
  EXPECT_EQ(driver_->ip_properties_.routes[0].host, "123.211.61.29");
  EXPECT_EQ(driver_->ip_properties_.routes[1].host, "123.211.42.29");
  EXPECT_EQ(driver_->ip_properties_.routes[0].netmask, "254.0.0.0");
  EXPECT_EQ(driver_->ip_properties_.routes[1].netmask, "255.255.128.0");
  EXPECT_EQ(driver_->ip_properties_.routes[0].gateway, parameters["address"]);
  EXPECT_EQ(driver_->ip_properties_.routes[1].gateway, parameters["address"]);
  EXPECT_TRUE(error.empty());
  EXPECT_TRUE(warning.empty());
  EXPECT_FALSE(driver_->parameters_expected_);
  driver_->device_ = nullptr;
}

}  // namespace shill
