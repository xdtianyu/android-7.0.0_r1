// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBWEAVE_SRC_CONFIG_H_
#define LIBWEAVE_SRC_CONFIG_H_

#include <set>
#include <string>
#include <vector>

#include <base/callback.h>
#include <base/gtest_prod_util.h>
#include <weave/error.h>
#include <weave/provider/config_store.h>

#include "src/privet/privet_types.h"

namespace weave {

class StorageInterface;

enum class RootClientTokenOwner {
  // Keep order as it's used with order comparison operators.
  kNone,
  kClient,
  kCloud,
};

// Handles reading buffet config and state files.
class Config final {
 public:
  struct Settings : public weave::Settings {
    std::string refresh_token;
    std::string robot_account;
    std::string last_configured_ssid;
    std::vector<uint8_t> secret;
    RootClientTokenOwner root_client_token_owner{RootClientTokenOwner::kNone};
  };

  using OnChangedCallback = base::Callback<void(const weave::Settings&)>;
  ~Config() = default;

  explicit Config(provider::ConfigStore* config_store);

  void AddOnChangedCallback(const OnChangedCallback& callback);
  const Config::Settings& GetSettings() const;

  // Allows editing of config. Makes sure that callbacks were called and changes
  // were saved.
  // User can commit changes by calling Commit method or by destroying the
  // object.
  class Transaction final {
   public:
    explicit Transaction(Config* config)
        : config_(config), settings_(&config->settings_) {
      CHECK(config_);
    }

    ~Transaction();

    void set_client_id(const std::string& id) { settings_->client_id = id; }
    void set_client_secret(const std::string& secret) {
      settings_->client_secret = secret;
    }
    void set_api_key(const std::string& key) { settings_->api_key = key; }
    void set_oauth_url(const std::string& url) { settings_->oauth_url = url; }
    void set_service_url(const std::string& url) {
      settings_->service_url = url;
    }
    void set_xmpp_endpoint(const std::string& endpoint) {
      settings_->xmpp_endpoint = endpoint;
    }
    void set_name(const std::string& name) { settings_->name = name; }
    void set_description(const std::string& description) {
      settings_->description = description;
    }
    void set_location(const std::string& location) {
      settings_->location = location;
    }
    void set_local_anonymous_access_role(AuthScope role) {
      settings_->local_anonymous_access_role = role;
    }
    void set_local_discovery_enabled(bool enabled) {
      settings_->local_discovery_enabled = enabled;
    }
    void set_local_pairing_enabled(bool enabled) {
      settings_->local_pairing_enabled = enabled;
    }
    void set_cloud_id(const std::string& id) { settings_->cloud_id = id; }
    void set_refresh_token(const std::string& token) {
      settings_->refresh_token = token;
    }
    void set_robot_account(const std::string& account) {
      settings_->robot_account = account;
    }
    void set_last_configured_ssid(const std::string& ssid) {
      settings_->last_configured_ssid = ssid;
    }
    void set_secret(const std::vector<uint8_t>& secret) {
      settings_->secret = secret;
    }
    void set_root_client_token_owner(
        RootClientTokenOwner root_client_token_owner) {
      settings_->root_client_token_owner = root_client_token_owner;
    }

    void Commit();

   private:
    FRIEND_TEST_ALL_PREFIXES(ConfigTest, Setters);
    void set_device_id(const std::string& id) {
      config_->settings_.device_id = id;
    }

    friend class Config;
    void LoadState();
    Config* config_;
    Settings* settings_;
    bool save_{true};
  };

 private:
  void Load();
  void Save();

  Settings settings_;
  provider::ConfigStore* config_store_{nullptr};
  std::vector<OnChangedCallback> on_changed_;

  DISALLOW_COPY_AND_ASSIGN(Config);
};

}  // namespace weave

#endif  // LIBWEAVE_SRC_CONFIG_H_
