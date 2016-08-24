// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/config.h"

#include <set>

#include <base/bind.h>
#include <base/guid.h>
#include <base/json/json_reader.h>
#include <base/json/json_writer.h>
#include <base/logging.h>
#include <base/strings/string_number_conversions.h>
#include <base/values.h>
#include <weave/enum_to_string.h>

#include "src/data_encoding.h"
#include "src/privet/privet_types.h"
#include "src/string_utils.h"
#include "src/bind_lambda.h"

namespace weave {

const char kConfigName[] = "config";

namespace config_keys {

const char kVersion[] = "version";

const char kClientId[] = "client_id";
const char kClientSecret[] = "client_secret";
const char kApiKey[] = "api_key";
const char kOAuthURL[] = "oauth_url";
const char kServiceURL[] = "service_url";
const char kXmppEndpoint[] = "xmpp_endpoint";
const char kName[] = "name";
const char kDescription[] = "description";
const char kLocation[] = "location";
const char kLocalAnonymousAccessRole[] = "local_anonymous_access_role";
const char kLocalDiscoveryEnabled[] = "local_discovery_enabled";
const char kLocalPairingEnabled[] = "local_pairing_enabled";
const char kRefreshToken[] = "refresh_token";
const char kCloudId[] = "cloud_id";
const char kDeviceId[] = "device_id";
const char kRobotAccount[] = "robot_account";
const char kLastConfiguredSsid[] = "last_configured_ssid";
const char kSecret[] = "secret";
const char kRootClientTokenOwner[] = "root_client_token_owner";

}  // namespace config_keys

const char kWeaveUrl[] = "https://www.googleapis.com/weave/v1/";
const char kDeprecatedUrl[] = "https://www.googleapis.com/clouddevices/v1/";
const char kXmppEndpoint[] = "talk.google.com:5223";

namespace {

const int kCurrentConfigVersion = 1;

void MigrateFromV0(base::DictionaryValue* dict) {
  std::string cloud_id;
  if (dict->GetString(config_keys::kCloudId, &cloud_id) && !cloud_id.empty())
    return;
  scoped_ptr<base::Value> tmp;
  if (dict->Remove(config_keys::kDeviceId, &tmp))
    dict->Set(config_keys::kCloudId, std::move(tmp));
}

Config::Settings CreateDefaultSettings() {
  Config::Settings result;
  result.oauth_url = "https://accounts.google.com/o/oauth2/";
  result.service_url = kWeaveUrl;
  result.xmpp_endpoint = kXmppEndpoint;
  result.local_anonymous_access_role = AuthScope::kViewer;
  result.pairing_modes.insert(PairingType::kPinCode);
  result.device_id = base::GenerateGUID();
  return result;
}

const EnumToStringMap<RootClientTokenOwner>::Map kRootClientTokenOwnerMap[] = {
    {RootClientTokenOwner::kNone, "none"},
    {RootClientTokenOwner::kClient, "client"},
    {RootClientTokenOwner::kCloud, "cloud"},
};

}  // namespace

template <>
LIBWEAVE_EXPORT EnumToStringMap<RootClientTokenOwner>::EnumToStringMap()
    : EnumToStringMap(kRootClientTokenOwnerMap) {}

Config::Config(provider::ConfigStore* config_store)
    : settings_{CreateDefaultSettings()}, config_store_{config_store} {
  Load();
}

void Config::AddOnChangedCallback(const OnChangedCallback& callback) {
  on_changed_.push_back(callback);
  // Force to read current state.
  callback.Run(settings_);
}

const Config::Settings& Config::GetSettings() const {
  return settings_;
}

void Config::Load() {
  Transaction change{this};
  change.save_ = false;

  settings_ = CreateDefaultSettings();

  if (!config_store_)
    return;

  // Crash on any mistakes in defaults.
  CHECK(config_store_->LoadDefaults(&settings_));

  CHECK(!settings_.client_id.empty());
  CHECK(!settings_.client_secret.empty());
  CHECK(!settings_.api_key.empty());
  CHECK(!settings_.oauth_url.empty());
  CHECK(!settings_.service_url.empty());
  CHECK(!settings_.xmpp_endpoint.empty());
  CHECK(!settings_.oem_name.empty());
  CHECK(!settings_.model_name.empty());
  CHECK(!settings_.model_id.empty());
  CHECK(!settings_.name.empty());
  CHECK(!settings_.device_id.empty());
  CHECK_EQ(settings_.embedded_code.empty(),
           (settings_.pairing_modes.find(PairingType::kEmbeddedCode) ==
              settings_.pairing_modes.end()));

  // Values below will be generated at runtime.
  CHECK(settings_.cloud_id.empty());
  CHECK(settings_.refresh_token.empty());
  CHECK(settings_.robot_account.empty());
  CHECK(settings_.last_configured_ssid.empty());
  CHECK(settings_.secret.empty());
  CHECK(settings_.root_client_token_owner == RootClientTokenOwner::kNone);

  change.LoadState();
}

void Config::Transaction::LoadState() {
  if (!config_->config_store_)
    return;
  std::string json_string = config_->config_store_->LoadSettings(kConfigName);
  if (json_string.empty()) {
    json_string = config_->config_store_->LoadSettings();
    if (json_string.empty())
      return;
  }

  auto value = base::JSONReader::Read(json_string);
  base::DictionaryValue* dict = nullptr;
  if (!value || !value->GetAsDictionary(&dict)) {
    LOG(ERROR) << "Failed to parse settings.";
    return;
  }

  int loaded_version = 0;
  dict->GetInteger(config_keys::kVersion, &loaded_version);

  if (loaded_version != kCurrentConfigVersion) {
    LOG(INFO) << "State version mismatch. expected: " << kCurrentConfigVersion
              << ", loaded: " << loaded_version;
    save_ = true;
  }

  if (loaded_version == 0) {
    MigrateFromV0(dict);
  }

  std::string tmp;
  bool tmp_bool{false};

  if (dict->GetString(config_keys::kClientId, &tmp))
    set_client_id(tmp);

  if (dict->GetString(config_keys::kClientSecret, &tmp))
    set_client_secret(tmp);

  if (dict->GetString(config_keys::kApiKey, &tmp))
    set_api_key(tmp);

  if (dict->GetString(config_keys::kOAuthURL, &tmp))
    set_oauth_url(tmp);

  if (dict->GetString(config_keys::kServiceURL, &tmp)) {
    if (tmp == kDeprecatedUrl)
      tmp = kWeaveUrl;
    set_service_url(tmp);
  }

  if (dict->GetString(config_keys::kXmppEndpoint, &tmp)) {
    set_xmpp_endpoint(tmp);
  }

  if (dict->GetString(config_keys::kName, &tmp))
    set_name(tmp);

  if (dict->GetString(config_keys::kDescription, &tmp))
    set_description(tmp);

  if (dict->GetString(config_keys::kLocation, &tmp))
    set_location(tmp);

  AuthScope scope{AuthScope::kNone};
  if (dict->GetString(config_keys::kLocalAnonymousAccessRole, &tmp) &&
      StringToEnum(tmp, &scope)) {
    set_local_anonymous_access_role(scope);
  }

  if (dict->GetBoolean(config_keys::kLocalDiscoveryEnabled, &tmp_bool))
    set_local_discovery_enabled(tmp_bool);

  if (dict->GetBoolean(config_keys::kLocalPairingEnabled, &tmp_bool))
    set_local_pairing_enabled(tmp_bool);

  if (dict->GetString(config_keys::kCloudId, &tmp))
    set_cloud_id(tmp);

  if (dict->GetString(config_keys::kDeviceId, &tmp))
    set_device_id(tmp);

  if (dict->GetString(config_keys::kRefreshToken, &tmp))
    set_refresh_token(tmp);

  if (dict->GetString(config_keys::kRobotAccount, &tmp))
    set_robot_account(tmp);

  if (dict->GetString(config_keys::kLastConfiguredSsid, &tmp))
    set_last_configured_ssid(tmp);

  std::vector<uint8_t> secret;
  if (dict->GetString(config_keys::kSecret, &tmp) && Base64Decode(tmp, &secret))
    set_secret(secret);

  RootClientTokenOwner token_owner{RootClientTokenOwner::kNone};
  if (dict->GetString(config_keys::kRootClientTokenOwner, &tmp) &&
      StringToEnum(tmp, &token_owner)) {
    set_root_client_token_owner(token_owner);
  }
}

void Config::Save() {
  if (!config_store_)
    return;

  base::DictionaryValue dict;
  dict.SetInteger(config_keys::kVersion, kCurrentConfigVersion);

  dict.SetString(config_keys::kClientId, settings_.client_id);
  dict.SetString(config_keys::kClientSecret, settings_.client_secret);
  dict.SetString(config_keys::kApiKey, settings_.api_key);
  dict.SetString(config_keys::kOAuthURL, settings_.oauth_url);
  dict.SetString(config_keys::kServiceURL, settings_.service_url);
  dict.SetString(config_keys::kXmppEndpoint, settings_.xmpp_endpoint);
  dict.SetString(config_keys::kRefreshToken, settings_.refresh_token);
  dict.SetString(config_keys::kCloudId, settings_.cloud_id);
  dict.SetString(config_keys::kDeviceId, settings_.device_id);
  dict.SetString(config_keys::kRobotAccount, settings_.robot_account);
  dict.SetString(config_keys::kLastConfiguredSsid,
                 settings_.last_configured_ssid);
  dict.SetString(config_keys::kSecret, Base64Encode(settings_.secret));
  dict.SetString(config_keys::kRootClientTokenOwner,
                 EnumToString(settings_.root_client_token_owner));
  dict.SetString(config_keys::kName, settings_.name);
  dict.SetString(config_keys::kDescription, settings_.description);
  dict.SetString(config_keys::kLocation, settings_.location);
  dict.SetString(config_keys::kLocalAnonymousAccessRole,
                 EnumToString(settings_.local_anonymous_access_role));
  dict.SetBoolean(config_keys::kLocalDiscoveryEnabled,
                  settings_.local_discovery_enabled);
  dict.SetBoolean(config_keys::kLocalPairingEnabled,
                  settings_.local_pairing_enabled);

  std::string json_string;
  base::JSONWriter::WriteWithOptions(
      dict, base::JSONWriter::OPTIONS_PRETTY_PRINT, &json_string);

  config_store_->SaveSettings(
      kConfigName, json_string,
      base::Bind([](ErrorPtr error) { CHECK(!error); }));
}

Config::Transaction::~Transaction() {
  Commit();
}

void Config::Transaction::Commit() {
  if (!config_)
    return;
  if (save_)
    config_->Save();
  for (const auto& cb : config_->on_changed_)
    cb.Run(*settings_);
  config_ = nullptr;
}

}  // namespace weave
