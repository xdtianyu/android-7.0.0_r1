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

#include "shill/technology.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "shill/error.h"

using std::string;
using std::vector;
using testing::ElementsAre;
using testing::Test;

namespace shill {

class TechnologyTest : public Test {};

TEST_F(TechnologyTest, IdentifierFromName) {
  EXPECT_EQ(Technology::kEthernet, Technology::IdentifierFromName("ethernet"));
  EXPECT_EQ(Technology::kEthernetEap,
            Technology::IdentifierFromName("etherneteap"));
  EXPECT_EQ(Technology::kWifi, Technology::IdentifierFromName("wifi"));
  EXPECT_EQ(Technology::kWiMax, Technology::IdentifierFromName("wimax"));
  EXPECT_EQ(Technology::kCellular, Technology::IdentifierFromName("cellular"));
  EXPECT_EQ(Technology::kTunnel, Technology::IdentifierFromName("tunnel"));
  EXPECT_EQ(Technology::kLoopback, Technology::IdentifierFromName("loopback"));
  EXPECT_EQ(Technology::kVPN, Technology::IdentifierFromName("vpn"));
  EXPECT_EQ(Technology::kPPP, Technology::IdentifierFromName("ppp"));
  EXPECT_EQ(Technology::kUnknown, Technology::IdentifierFromName("bluetooth"));
  EXPECT_EQ(Technology::kUnknown, Technology::IdentifierFromName("foo"));
  EXPECT_EQ(Technology::kUnknown, Technology::IdentifierFromName(""));
}

TEST_F(TechnologyTest, NameFromIdentifier) {
  EXPECT_EQ("ethernet", Technology::NameFromIdentifier(Technology::kEthernet));
  EXPECT_EQ("etherneteap",
            Technology::NameFromIdentifier(Technology::kEthernetEap));
  EXPECT_EQ("wifi", Technology::NameFromIdentifier(Technology::kWifi));
  EXPECT_EQ("wimax", Technology::NameFromIdentifier(Technology::kWiMax));
  EXPECT_EQ("cellular", Technology::NameFromIdentifier(Technology::kCellular));
  EXPECT_EQ("tunnel", Technology::NameFromIdentifier(Technology::kTunnel));
  EXPECT_EQ("loopback", Technology::NameFromIdentifier(Technology::kLoopback));
  EXPECT_EQ("vpn", Technology::NameFromIdentifier(Technology::kVPN));
  EXPECT_EQ("ppp", Technology::NameFromIdentifier(Technology::kPPP));
  EXPECT_EQ("pppoe", Technology::NameFromIdentifier(Technology::kPPPoE));
  EXPECT_EQ("unknown", Technology::NameFromIdentifier(Technology::kUnknown));
}

TEST_F(TechnologyTest, IdentifierFromStorageGroup) {
  EXPECT_EQ(Technology::kVPN, Technology::IdentifierFromStorageGroup("vpn"));
  EXPECT_EQ(Technology::kVPN, Technology::IdentifierFromStorageGroup("vpn_a"));
  EXPECT_EQ(Technology::kVPN, Technology::IdentifierFromStorageGroup("vpn__a"));
  EXPECT_EQ(Technology::kVPN,
            Technology::IdentifierFromStorageGroup("vpn_a_1"));
  EXPECT_EQ(Technology::kUnknown,
            Technology::IdentifierFromStorageGroup("_vpn"));
  EXPECT_EQ(Technology::kUnknown, Technology::IdentifierFromStorageGroup("_"));
  EXPECT_EQ(Technology::kUnknown, Technology::IdentifierFromStorageGroup(""));
}

TEST_F(TechnologyTest, GetTechnologyVectorFromStringWithValidTechnologyNames) {
  vector<Technology::Identifier> technologies;
  Error error;

  EXPECT_TRUE(Technology::GetTechnologyVectorFromString(
      "", &technologies, &error));
  EXPECT_THAT(technologies, ElementsAre());
  EXPECT_TRUE(error.IsSuccess());

  EXPECT_TRUE(Technology::GetTechnologyVectorFromString(
      "ethernet", &technologies, &error));
  EXPECT_THAT(technologies, ElementsAre(Technology::kEthernet));
  EXPECT_TRUE(error.IsSuccess());

  EXPECT_TRUE(Technology::GetTechnologyVectorFromString(
      "ethernet,vpn", &technologies, &error));
  EXPECT_THAT(technologies, ElementsAre(Technology::kEthernet,
                                        Technology::kVPN));
  EXPECT_TRUE(error.IsSuccess());

  EXPECT_TRUE(Technology::GetTechnologyVectorFromString(
      "wifi,ethernet,vpn", &technologies, &error));
  EXPECT_THAT(technologies, ElementsAre(Technology::kWifi,
                                        Technology::kEthernet,
                                        Technology::kVPN));
  EXPECT_TRUE(error.IsSuccess());
}

TEST_F(TechnologyTest,
       GetTechnologyVectorFromStringWithInvalidTechnologyNames) {
  vector<Technology::Identifier> technologies;
  Error error;

  EXPECT_FALSE(Technology::GetTechnologyVectorFromString(
      "foo", &technologies, &error));
  EXPECT_EQ(Error::kInvalidArguments, error.type());
  EXPECT_EQ("foo is an unknown technology name", error.message());

  EXPECT_FALSE(Technology::GetTechnologyVectorFromString(
      "ethernet,bar", &technologies, &error));
  EXPECT_EQ(Error::kInvalidArguments, error.type());
  EXPECT_EQ("bar is an unknown technology name", error.message());

  EXPECT_FALSE(Technology::GetTechnologyVectorFromString(
      "ethernet,foo,vpn", &technologies, &error));
  EXPECT_EQ(Error::kInvalidArguments, error.type());
  EXPECT_EQ("foo is an unknown technology name", error.message());
}

TEST_F(TechnologyTest,
       GetTechnologyVectorFromStringWithDuplicateTechnologyNames) {
  vector<Technology::Identifier> technologies;
  Error error;

  EXPECT_FALSE(Technology::GetTechnologyVectorFromString(
      "ethernet,vpn,ethernet", &technologies, &error));
  EXPECT_EQ(Error::kInvalidArguments, error.type());
  EXPECT_EQ("ethernet is duplicated in the list", error.message());
}

}  // namespace shill
