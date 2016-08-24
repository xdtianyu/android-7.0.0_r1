// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/privet/wifi_ssid_generator.h"

#include <gtest/gtest.h>

#include "src/privet/mock_delegates.h"
#include "src/privet/openssl_utils.h"

namespace weave {
namespace privet {

class WifiSsidGeneratorTest : public testing::Test {
 protected:
  void SetRandomForTests(int n) { ssid_generator_.SetRandomForTests(n); }

  testing::StrictMock<MockCloudDelegate> gcd_;
  testing::StrictMock<MockWifiDelegate> wifi_;

  WifiSsidGenerator ssid_generator_{&gcd_, &wifi_};
};

TEST_F(WifiSsidGeneratorTest, GenerateFlagsWithWifi24) {
  EXPECT_CALL(wifi_, GetTypes())
      .WillRepeatedly(Return(std::set<WifiType>{WifiType::kWifi24}));

  EXPECT_EQ(ssid_generator_.GenerateFlags().size(), 2u);

  wifi_.connection_state_ = ConnectionState{ConnectionState::kUnconfigured};
  gcd_.connection_state_ = ConnectionState{ConnectionState::kUnconfigured};
  EXPECT_EQ("DB", ssid_generator_.GenerateFlags());

  wifi_.connection_state_ = ConnectionState{ConnectionState::kOnline};
  EXPECT_EQ("CB", ssid_generator_.GenerateFlags());

  gcd_.connection_state_ = ConnectionState{ConnectionState::kOffline};
  EXPECT_EQ("AB", ssid_generator_.GenerateFlags());

  wifi_.connection_state_ = ConnectionState{ConnectionState::kUnconfigured};
  EXPECT_EQ("BB", ssid_generator_.GenerateFlags());
}

TEST_F(WifiSsidGeneratorTest, GenerateFlagsWithWifi50) {
  EXPECT_CALL(wifi_, GetTypes())
      .WillRepeatedly(Return(std::set<WifiType>{WifiType::kWifi50}));

  EXPECT_EQ(ssid_generator_.GenerateFlags().size(), 2u);

  wifi_.connection_state_ = ConnectionState{ConnectionState::kUnconfigured};
  gcd_.connection_state_ = ConnectionState{ConnectionState::kUnconfigured};
  EXPECT_EQ("DC", ssid_generator_.GenerateFlags());

  wifi_.connection_state_ = ConnectionState{ConnectionState::kOnline};
  EXPECT_EQ("CC", ssid_generator_.GenerateFlags());

  gcd_.connection_state_ = ConnectionState{ConnectionState::kOffline};
  EXPECT_EQ("AC", ssid_generator_.GenerateFlags());

  wifi_.connection_state_ = ConnectionState{ConnectionState::kUnconfigured};
  EXPECT_EQ("BC", ssid_generator_.GenerateFlags());
}

TEST_F(WifiSsidGeneratorTest, GenerateSsid31orLess) {
  EXPECT_LE(ssid_generator_.GenerateSsid().size(), 31u);
}

TEST_F(WifiSsidGeneratorTest, GenerateSsidValue) {
  SetRandomForTests(47);
  EXPECT_EQ("TestDevice 47.ABMIDABprv", ssid_generator_.GenerateSsid());

  SetRandomForTests(9);
  EXPECT_EQ("TestDevice 9.ABMIDABprv", ssid_generator_.GenerateSsid());
}

TEST_F(WifiSsidGeneratorTest, GenerateSsidLongName) {
  SetRandomForTests(99);
  EXPECT_CALL(gcd_, GetName()).WillRepeatedly(Return("Very Long Device Name"));
  EXPECT_EQ("Very Long Device  99.ABMIDABprv", ssid_generator_.GenerateSsid());
}

}  // namespace privet
}  // namespace weave
