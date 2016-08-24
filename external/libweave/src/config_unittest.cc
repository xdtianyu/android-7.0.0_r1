// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/config.h"

#include <set>

#include <base/bind.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <weave/provider/test/mock_config_store.h>
#include <weave/test/unittest_utils.h>

#include "src/data_encoding.h"

using testing::_;
using testing::Invoke;
using testing::Return;
using testing::WithArgs;

namespace weave {

const char kConfigName[] = "config";

class ConfigTest : public ::testing::Test {
 protected:
  void SetUp() override {
    Reload();
  }

  void Reload() {
    EXPECT_CALL(*this, OnConfigChanged(_)).Times(1);
    config_.reset(new Config{&config_store_});
    config_->AddOnChangedCallback(
        base::Bind(&ConfigTest::OnConfigChanged, base::Unretained(this)));
  }

  const Config::Settings& GetSettings() const { return config_->GetSettings(); }

  const Config::Settings& GetDefaultSettings() const {
    return default_.GetSettings();
  }

  MOCK_METHOD1(OnConfigChanged, void(const Settings&));

  provider::test::MockConfigStore config_store_;
  std::unique_ptr<Config> config_{new Config{nullptr}};
  Config default_{&config_store_};
};

TEST_F(ConfigTest, NoStorage) {
  Config config{nullptr};
  Config::Transaction change{&config};
  change.Commit();
}

TEST_F(ConfigTest, Defaults) {
  config_.reset(new Config{nullptr});
  EXPECT_EQ("", GetSettings().client_id);
  EXPECT_EQ("", GetSettings().client_secret);
  EXPECT_EQ("", GetSettings().api_key);
  EXPECT_EQ("https://accounts.google.com/o/oauth2/", GetSettings().oauth_url);
  EXPECT_EQ("https://www.googleapis.com/weave/v1/", GetSettings().service_url);
  EXPECT_EQ("talk.google.com:5223", GetSettings().xmpp_endpoint);
  EXPECT_EQ("", GetSettings().oem_name);
  EXPECT_EQ("", GetSettings().model_name);
  EXPECT_EQ("", GetSettings().model_id);
  EXPECT_FALSE(GetSettings().device_id.empty());
  EXPECT_EQ("", GetSettings().firmware_version);
  EXPECT_TRUE(GetSettings().wifi_auto_setup_enabled);
  EXPECT_EQ("", GetSettings().test_privet_ssid);
  EXPECT_EQ(std::set<PairingType>{PairingType::kPinCode},
            GetSettings().pairing_modes);
  EXPECT_EQ("", GetSettings().embedded_code);
  EXPECT_EQ("", GetSettings().name);
  EXPECT_EQ("", GetSettings().description);
  EXPECT_EQ("", GetSettings().location);
  EXPECT_EQ(AuthScope::kViewer, GetSettings().local_anonymous_access_role);
  EXPECT_TRUE(GetSettings().local_pairing_enabled);
  EXPECT_TRUE(GetSettings().local_discovery_enabled);
  EXPECT_EQ("", GetSettings().cloud_id);
  EXPECT_EQ("", GetSettings().refresh_token);
  EXPECT_EQ("", GetSettings().robot_account);
  EXPECT_EQ("", GetSettings().last_configured_ssid);
  EXPECT_EQ(std::vector<uint8_t>(), GetSettings().secret);
  EXPECT_EQ(RootClientTokenOwner::kNone, GetSettings().root_client_token_owner);
}

TEST_F(ConfigTest, LoadStateV0) {
  EXPECT_CALL(config_store_, LoadSettings(kConfigName))
      .WillOnce(Return(R"({
    "device_id": "state_device_id"
  })"));

  Reload();

  EXPECT_EQ("state_device_id", GetSettings().cloud_id);
  EXPECT_FALSE(GetSettings().device_id.empty());
  EXPECT_NE(GetSettings().cloud_id, GetSettings().device_id);

  EXPECT_CALL(config_store_, LoadSettings(kConfigName))
      .WillOnce(Return(R"({
    "device_id": "state_device_id",
    "cloud_id": "state_cloud_id"
  })"));

  Reload();

  EXPECT_EQ("state_cloud_id", GetSettings().cloud_id);
  EXPECT_EQ("state_device_id", GetSettings().device_id);
}

TEST_F(ConfigTest, LoadStateUnnamed) {
  EXPECT_CALL(config_store_, LoadSettings(kConfigName)).WillOnce(Return(""));

  EXPECT_CALL(config_store_, LoadSettings()).Times(1);

  Reload();
}

TEST_F(ConfigTest, LoadStateNamed) {
  EXPECT_CALL(config_store_, LoadSettings(kConfigName)).WillOnce(Return("{}"));

  EXPECT_CALL(config_store_, LoadSettings()).Times(0);

  Reload();
}

TEST_F(ConfigTest, LoadState) {
  auto state = R"({
    "version": 1,
    "api_key": "state_api_key",
    "client_id": "state_client_id",
    "client_secret": "state_client_secret",
    "cloud_id": "state_cloud_id",
    "description": "state_description",
    "device_id": "state_device_id",
    "last_configured_ssid": "state_last_configured_ssid",
    "local_anonymous_access_role": "user",
    "root_client_token_owner": "client",
    "local_discovery_enabled": false,
    "local_pairing_enabled": false,
    "location": "state_location",
    "name": "state_name",
    "oauth_url": "state_oauth_url",
    "refresh_token": "state_refresh_token",
    "robot_account": "state_robot_account",
    "secret": "c3RhdGVfc2VjcmV0",
    "service_url": "state_service_url",
    "xmpp_endpoint": "state_xmpp_endpoint"
  })";
  EXPECT_CALL(config_store_, LoadSettings(kConfigName)).WillOnce(Return(state));

  Reload();

  EXPECT_EQ("state_client_id", GetSettings().client_id);
  EXPECT_EQ("state_client_secret", GetSettings().client_secret);
  EXPECT_EQ("state_api_key", GetSettings().api_key);
  EXPECT_EQ("state_oauth_url", GetSettings().oauth_url);
  EXPECT_EQ("state_service_url", GetSettings().service_url);
  EXPECT_EQ("state_xmpp_endpoint", GetSettings().xmpp_endpoint);
  EXPECT_EQ(GetDefaultSettings().oem_name, GetSettings().oem_name);
  EXPECT_EQ(GetDefaultSettings().model_name, GetSettings().model_name);
  EXPECT_EQ(GetDefaultSettings().model_id, GetSettings().model_id);
  EXPECT_EQ("state_device_id", GetSettings().device_id);
  EXPECT_EQ(GetDefaultSettings().wifi_auto_setup_enabled,
            GetSettings().wifi_auto_setup_enabled);
  EXPECT_EQ(GetDefaultSettings().test_privet_ssid,
            GetSettings().test_privet_ssid);
  EXPECT_EQ(GetDefaultSettings().pairing_modes, GetSettings().pairing_modes);
  EXPECT_EQ(GetDefaultSettings().embedded_code, GetSettings().embedded_code);
  EXPECT_EQ("state_name", GetSettings().name);
  EXPECT_EQ("state_description", GetSettings().description);
  EXPECT_EQ("state_location", GetSettings().location);
  EXPECT_EQ(AuthScope::kUser, GetSettings().local_anonymous_access_role);
  EXPECT_FALSE(GetSettings().local_pairing_enabled);
  EXPECT_FALSE(GetSettings().local_discovery_enabled);
  EXPECT_EQ("state_cloud_id", GetSettings().cloud_id);
  EXPECT_EQ("state_refresh_token", GetSettings().refresh_token);
  EXPECT_EQ("state_robot_account", GetSettings().robot_account);
  EXPECT_EQ("state_last_configured_ssid", GetSettings().last_configured_ssid);
  EXPECT_EQ("c3RhdGVfc2VjcmV0", Base64Encode(GetSettings().secret));
  EXPECT_EQ(RootClientTokenOwner::kClient,
            GetSettings().root_client_token_owner);
}

TEST_F(ConfigTest, Setters) {
  Config::Transaction change{config_.get()};

  change.set_client_id("set_client_id");
  EXPECT_EQ("set_client_id", GetSettings().client_id);

  change.set_client_secret("set_client_secret");
  EXPECT_EQ("set_client_secret", GetSettings().client_secret);

  change.set_api_key("set_api_key");
  EXPECT_EQ("set_api_key", GetSettings().api_key);

  change.set_oauth_url("set_oauth_url");
  EXPECT_EQ("set_oauth_url", GetSettings().oauth_url);

  change.set_service_url("set_service_url");
  EXPECT_EQ("set_service_url", GetSettings().service_url);

  change.set_xmpp_endpoint("set_xmpp_endpoint");
  EXPECT_EQ("set_xmpp_endpoint", GetSettings().xmpp_endpoint);

  change.set_name("set_name");
  EXPECT_EQ("set_name", GetSettings().name);

  change.set_description("set_description");
  EXPECT_EQ("set_description", GetSettings().description);

  change.set_location("set_location");
  EXPECT_EQ("set_location", GetSettings().location);

  change.set_local_anonymous_access_role(AuthScope::kViewer);
  EXPECT_EQ(AuthScope::kViewer, GetSettings().local_anonymous_access_role);

  change.set_local_anonymous_access_role(AuthScope::kNone);
  EXPECT_EQ(AuthScope::kNone, GetSettings().local_anonymous_access_role);

  change.set_local_anonymous_access_role(AuthScope::kUser);
  EXPECT_EQ(AuthScope::kUser, GetSettings().local_anonymous_access_role);

  change.set_local_discovery_enabled(false);
  EXPECT_FALSE(GetSettings().local_discovery_enabled);

  change.set_local_pairing_enabled(false);
  EXPECT_FALSE(GetSettings().local_pairing_enabled);

  change.set_local_discovery_enabled(true);
  EXPECT_TRUE(GetSettings().local_discovery_enabled);

  change.set_local_pairing_enabled(true);
  EXPECT_TRUE(GetSettings().local_pairing_enabled);

  change.set_cloud_id("set_cloud_id");
  EXPECT_EQ("set_cloud_id", GetSettings().cloud_id);

  change.set_device_id("set_device_id");
  EXPECT_EQ("set_device_id", GetSettings().device_id);

  change.set_refresh_token("set_token");
  EXPECT_EQ("set_token", GetSettings().refresh_token);

  change.set_robot_account("set_account");
  EXPECT_EQ("set_account", GetSettings().robot_account);

  change.set_last_configured_ssid("set_last_configured_ssid");
  EXPECT_EQ("set_last_configured_ssid", GetSettings().last_configured_ssid);

  const std::vector<uint8_t> secret{1, 2, 3, 4, 5};
  change.set_secret(secret);
  EXPECT_EQ(secret, GetSettings().secret);

  change.set_root_client_token_owner(RootClientTokenOwner::kCloud);
  EXPECT_EQ(RootClientTokenOwner::kCloud,
            GetSettings().root_client_token_owner);

  EXPECT_CALL(*this, OnConfigChanged(_)).Times(1);

  EXPECT_CALL(config_store_, SaveSettings(kConfigName, _, _))
      .WillOnce(WithArgs<1, 2>(
          Invoke([](const std::string& json, const DoneCallback& callback) {
            auto expected = R"({
          'version': 1,
          'api_key': 'set_api_key',
          'client_id': 'set_client_id',
          'client_secret': 'set_client_secret',
          'cloud_id': 'set_cloud_id',
          'description': 'set_description',
          'device_id': 'set_device_id',
          'last_configured_ssid': 'set_last_configured_ssid',
          'local_anonymous_access_role': 'user',
          'root_client_token_owner': 'cloud',
          'local_discovery_enabled': true,
          'local_pairing_enabled': true,
          'location': 'set_location',
          'name': 'set_name',
          'oauth_url': 'set_oauth_url',
          'refresh_token': 'set_token',
          'robot_account': 'set_account',
          'secret': 'AQIDBAU=',
          'service_url': 'set_service_url',
          'xmpp_endpoint': 'set_xmpp_endpoint'
        })";
            EXPECT_JSON_EQ(expected, *test::CreateValue(json));
            callback.Run(nullptr);
          })));

  change.Commit();
}

}  // namespace weave
