// Copyright 2015 The Android Open Source Project
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

#include "buffet/buffet_config.h"

#include <set>

#include <base/bind.h>
#include <brillo/data_encoding.h>
#include <gtest/gtest.h>

namespace buffet {

TEST(BuffetConfigTest, LoadConfig) {
  brillo::KeyValueStore config_store;
  config_store.SetString("client_id", "conf_client_id");
  config_store.SetString("client_secret", "conf_client_secret");
  config_store.SetString("api_key", "conf_api_key");
  config_store.SetString("oauth_url", "conf_oauth_url");
  config_store.SetString("service_url", "conf_service_url");
  config_store.SetString("oem_name", "conf_oem_name");
  config_store.SetString("model_name", "conf_model_name");
  config_store.SetString("model_id", "ABCDE");
  config_store.SetString("polling_period_ms", "12345");
  config_store.SetString("backup_polling_period_ms", "6589");
  config_store.SetBoolean("wifi_auto_setup_enabled", false);
  config_store.SetBoolean("ble_setup_enabled", true);
  config_store.SetString("pairing_modes", "pinCode,embeddedCode");
  config_store.SetString("embedded_code", "567");
  config_store.SetString("name", "conf_name");
  config_store.SetString("description", "conf_description");
  config_store.SetString("location", "conf_location");
  config_store.SetString("local_anonymous_access_role", "user");
  config_store.SetBoolean("local_pairing_enabled", false);
  config_store.SetBoolean("local_discovery_enabled", false);

  // Following will be ignored.
  config_store.SetString("device_kind", "conf_device_kind");
  config_store.SetString("device_id", "conf_device_id");
  config_store.SetString("refresh_token", "conf_refresh_token");
  config_store.SetString("robot_account", "conf_robot_account");
  config_store.SetString("last_configured_ssid", "conf_last_configured_ssid");

  weave::Settings settings;
  BuffetConfig config{{}};
  EXPECT_TRUE(config.LoadDefaults(config_store, &settings));

  EXPECT_EQ("conf_client_id", settings.client_id);
  EXPECT_EQ("conf_client_secret", settings.client_secret);
  EXPECT_EQ("conf_api_key", settings.api_key);
  EXPECT_EQ("conf_oauth_url", settings.oauth_url);
  EXPECT_EQ("conf_service_url", settings.service_url);
  EXPECT_EQ("conf_oem_name", settings.oem_name);
  EXPECT_EQ("conf_model_name", settings.model_name);
  EXPECT_EQ("ABCDE", settings.model_id);
  EXPECT_FALSE(settings.wifi_auto_setup_enabled);
  std::set<weave::PairingType> pairing_types{weave::PairingType::kPinCode,
                                             weave::PairingType::kEmbeddedCode};
  EXPECT_EQ(pairing_types, settings.pairing_modes);
  EXPECT_EQ("567", settings.embedded_code);
  EXPECT_EQ("conf_name", settings.name);
  EXPECT_EQ("conf_description", settings.description);
  EXPECT_EQ("conf_location", settings.location);
  EXPECT_EQ(weave::AuthScope::kUser, settings.local_anonymous_access_role);
  EXPECT_FALSE(settings.local_pairing_enabled);
  EXPECT_FALSE(settings.local_discovery_enabled);
}

class BuffetConfigTestWithFakes : public testing::Test,
                                  public BuffetConfig::FileIO,
                                  public Encryptor {
 public:
  void SetUp() {
    BuffetConfig::Options config_options;
    config_options.settings = base::FilePath{"settings_file"};
    config_.reset(new BuffetConfig{config_options});
    config_->SetEncryptor(this);
    config_->SetFileIO(this);
  };

  // buffet::Encryptor methods.
  bool EncryptWithAuthentication(const std::string& plaintext,
                                 std::string* ciphertext) override {
    *ciphertext = brillo::data_encoding::Base64Encode(plaintext);
    return encryptor_result_;
  };
  bool DecryptWithAuthentication(const std::string& ciphertext,
                                 std::string* plaintext) override {
    return encryptor_result_ &&
           brillo::data_encoding::Base64Decode(ciphertext, plaintext);
  };

  // buffet::BuffetConfig::FileIO methods.
  bool ReadFile(const base::FilePath& path, std::string* content) override {
    if (fake_file_content_.count(path.value()) == 0) {
      return false;
    }
    *content = fake_file_content_[path.value()];
    return io_result_;
  };
  bool WriteFile(const base::FilePath& path,
                 const std::string& content) override {
    if (io_result_) {
      fake_file_content_[path.value()] = content;
    }
    return io_result_;
  };

 protected:
  std::map<std::string, std::string> fake_file_content_;
  bool encryptor_result_ = true;
  bool io_result_ = true;
  std::unique_ptr<BuffetConfig> config_;
};

TEST_F(BuffetConfigTestWithFakes, EncryptionEnabled) {
  config_->SaveSettings("config", "test", {});
  ASSERT_NE("test", fake_file_content_["settings_file.config"]);
  ASSERT_EQ("test", config_->LoadSettings("config"));
}

TEST_F(BuffetConfigTestWithFakes, EncryptionFailure) {
  config_->SaveSettings("config", "test", {});
  ASSERT_FALSE(fake_file_content_["settings_file.config"].empty());
  encryptor_result_ = false;
  config_->SaveSettings("config", "test2", {});
  // Encryption fails -> file cleared.
  ASSERT_TRUE(fake_file_content_["settings_file.config"].empty());
}

TEST_F(BuffetConfigTestWithFakes, DecryptionFailure) {
  config_->SaveSettings("config", "test", {});
  ASSERT_FALSE(fake_file_content_["settings_file.config"].empty());
  encryptor_result_ = false;
  // Decryption fails -> empty settings loaded.
  ASSERT_TRUE(config_->LoadSettings("config").empty());
}

TEST_F(BuffetConfigTestWithFakes, SettingsIOFailure) {
  config_->SaveSettings("config", "test", {});
  std::string original = fake_file_content_["settings_file.config"];
  ASSERT_FALSE(original.empty());
  io_result_ = false;
  ASSERT_TRUE(config_->LoadSettings("config").empty());
  config_->SaveSettings("config2", "test", {});
  ASSERT_EQ(original, fake_file_content_["settings_file.config"]);
}

}  // namespace buffet
