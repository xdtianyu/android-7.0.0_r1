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

#include <map>
#include <set>

#include <base/files/file_util.h>
#include <base/files/important_file_writer.h>
#include <base/logging.h>
#include <base/message_loop/message_loop.h>
#include <base/strings/string_number_conversions.h>
#include <brillo/errors/error.h>
#include <brillo/errors/error_codes.h>
#include <brillo/osrelease_reader.h>
#include <brillo/strings/string_utils.h>
#include <weave/enum_to_string.h>

namespace buffet {

namespace {

const char kErrorDomain[] = "buffet";
const char kFileReadError[] = "file_read_error";
const char kProductVersionKey[] = "product_version";

class DefaultFileIO : public BuffetConfig::FileIO {
 public:
  bool ReadFile(const base::FilePath& path, std::string* content) override {
    return base::ReadFileToString(path, content);
  }
  bool WriteFile(const base::FilePath& path,
                 const std::string& content) override {
    return base::ImportantFileWriter::WriteFileAtomically(path, content);
  }
};

}  // namespace

namespace config_keys {

const char kClientId[] = "client_id";
const char kClientSecret[] = "client_secret";
const char kApiKey[] = "api_key";
const char kOAuthURL[] = "oauth_url";
const char kServiceURL[] = "service_url";
const char kName[] = "name";
const char kDescription[] = "description";
const char kLocation[] = "location";
const char kLocalAnonymousAccessRole[] = "local_anonymous_access_role";
const char kLocalDiscoveryEnabled[] = "local_discovery_enabled";
const char kLocalPairingEnabled[] = "local_pairing_enabled";
const char kOemName[] = "oem_name";
const char kModelName[] = "model_name";
const char kModelId[] = "model_id";
const char kWifiAutoSetupEnabled[] = "wifi_auto_setup_enabled";
const char kEmbeddedCode[] = "embedded_code";
const char kPairingModes[] = "pairing_modes";

}  // namespace config_keys

BuffetConfig::BuffetConfig(const Options& options)
    : options_(options),
      default_encryptor_(Encryptor::CreateDefaultEncryptor()),
      encryptor_(default_encryptor_.get()),
      default_file_io_(new DefaultFileIO),
      file_io_(default_file_io_.get()) {}

bool BuffetConfig::LoadDefaults(weave::Settings* settings) {
  // Keep this hardcoded default for sometime. This previously was set by
  // libweave. It should be set by overlay's buffet.conf.
  // Keys owners: avakulenko, gene, vitalybuka.
  settings->client_id =
      "338428340000-vkb4p6h40c7kja1k3l70kke8t615cjit.apps.googleusercontent."
      "com";
  settings->client_secret = "LS_iPYo_WIOE0m2VnLdduhnx";
  settings->api_key = "AIzaSyACK3oZtmIylUKXiTMqkZqfuRiCgQmQSAQ";

  settings->name = "Developer device";
  settings->oem_name = "Chromium";
  settings->model_name = "Brillo";
  settings->model_id = "AAAAA";

  if (!base::PathExists(options_.defaults))
    return true;  // Nothing to load.

  brillo::KeyValueStore store;
  if (!store.Load(options_.defaults))
    return false;
  bool result = LoadDefaults(store, settings);
  settings->test_privet_ssid = options_.test_privet_ssid;

  if (!options_.client_id.empty())
    settings->client_id = options_.client_id;
  if (!options_.client_secret.empty())
    settings->client_secret = options_.client_secret;
  if (!options_.api_key.empty())
    settings->api_key = options_.api_key;
  if (!options_.oauth_url.empty())
    settings->oauth_url = options_.oauth_url;
  if (!options_.service_url.empty())
    settings->service_url = options_.service_url;

  return result;
}

bool BuffetConfig::LoadDefaults(const brillo::KeyValueStore& store,
                                weave::Settings* settings) {
  store.GetString(config_keys::kClientId, &settings->client_id);
  store.GetString(config_keys::kClientSecret, &settings->client_secret);
  store.GetString(config_keys::kApiKey, &settings->api_key);
  store.GetString(config_keys::kOAuthURL, &settings->oauth_url);
  store.GetString(config_keys::kServiceURL, &settings->service_url);
  store.GetString(config_keys::kOemName, &settings->oem_name);
  store.GetString(config_keys::kModelName, &settings->model_name);
  store.GetString(config_keys::kModelId, &settings->model_id);

  brillo::OsReleaseReader reader;
  reader.Load();
  if (!reader.GetString(kProductVersionKey, &settings->firmware_version)) {
    LOG(ERROR) << "Could not read '" << kProductVersionKey << "' from OS";
  }

  store.GetBoolean(config_keys::kWifiAutoSetupEnabled,
                   &settings->wifi_auto_setup_enabled);
  store.GetString(config_keys::kEmbeddedCode, &settings->embedded_code);

  std::string modes_str;
  if (store.GetString(config_keys::kPairingModes, &modes_str)) {
    std::set<weave::PairingType> pairing_modes;
    for (const std::string& mode :
         brillo::string_utils::Split(modes_str, ",", true, true)) {
      weave::PairingType pairing_mode;
      if (!StringToEnum(mode, &pairing_mode))
        return false;
      pairing_modes.insert(pairing_mode);
    }
    settings->pairing_modes = std::move(pairing_modes);
  }

  store.GetString(config_keys::kName, &settings->name);
  store.GetString(config_keys::kDescription, &settings->description);
  store.GetString(config_keys::kLocation, &settings->location);

  std::string role_str;
  if (store.GetString(config_keys::kLocalAnonymousAccessRole, &role_str)) {
    if (!StringToEnum(role_str, &settings->local_anonymous_access_role))
      return false;
  }
  store.GetBoolean(config_keys::kLocalDiscoveryEnabled,
                   &settings->local_discovery_enabled);
  store.GetBoolean(config_keys::kLocalPairingEnabled,
                   &settings->local_pairing_enabled);
  return true;
}

std::string BuffetConfig::LoadSettings(const std::string& name) {
  std::string settings_blob;
  base::FilePath path = CreatePath(name);
  if (!file_io_->ReadFile(path, &settings_blob)) {
    LOG(WARNING) << "Failed to read \'" + path.value() +
                        "\', proceeding with empty settings.";
    return std::string();
  }
  std::string json_string;
  if (!encryptor_->DecryptWithAuthentication(settings_blob, &json_string)) {
    LOG(WARNING)
        << "Failed to decrypt settings, proceeding with empty settings.";
    SaveSettings(std::string(), name, {});
    return std::string();
  }
  return json_string;
}

std::string BuffetConfig::LoadSettings() {
  return LoadSettings("");
}

void BuffetConfig::SaveSettings(const std::string& name,
                                const std::string& settings,
                                const weave::DoneCallback& callback) {
  std::string encrypted_settings;
  weave::ErrorPtr error;
  base::FilePath path = CreatePath(name);
  if (!encryptor_->EncryptWithAuthentication(settings, &encrypted_settings)) {
    weave::Error::AddTo(&error, FROM_HERE, "file_write_error",
                        "Failed to encrypt settings.");
    encrypted_settings.clear();
  }
  if (!file_io_->WriteFile(path, encrypted_settings)) {
    weave::Error::AddTo(&error, FROM_HERE, "file_write_error",
                        "Failed to write \'" + path.value() +
                            "\', proceeding with empty settings.");
  }
  if (!callback.is_null()) {
    base::MessageLoop::current()->PostTask(
        FROM_HERE, base::Bind(callback, base::Passed(&error)));
  }
}

base::FilePath BuffetConfig::CreatePath(const std::string& name) const {
  return name.empty() ? options_.settings
                      : options_.settings.InsertBeforeExtension(
                            base::FilePath::kExtensionSeparator + name);
}

bool BuffetConfig::LoadFile(const base::FilePath& file_path,
                            std::string* data,
                            brillo::ErrorPtr* error) {
  if (!file_io_->ReadFile(file_path, data)) {
    brillo::errors::system::AddSystemError(error, FROM_HERE, errno);
    brillo::Error::AddToPrintf(error, FROM_HERE, kErrorDomain, kFileReadError,
                                 "Failed to read file '%s'",
                                 file_path.value().c_str());
    return false;
  }
  return true;
}

}  // namespace buffet
