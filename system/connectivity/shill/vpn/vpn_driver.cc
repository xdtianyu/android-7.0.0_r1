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

#include "shill/vpn/vpn_driver.h"

#include <string>
#include <vector>

#include <base/strings/string_util.h>
#if defined(__ANDROID__)
#include <dbus/service_constants.h>
#else
#include <chromeos/dbus/service_constants.h>
#endif  // __ANDROID__

#include "shill/connection.h"
#include "shill/event_dispatcher.h"
#include "shill/logging.h"
#include "shill/manager.h"
#include "shill/property_accessor.h"
#include "shill/property_store.h"
#include "shill/store_interface.h"

using std::string;
using std::vector;

namespace shill {

namespace Logging {
static auto kModuleLogScope = ScopeLogger::kVPN;
static string ObjectID(VPNDriver* v) { return "(vpn_driver)"; }
}

// static
const int VPNDriver::kDefaultConnectTimeoutSeconds = 60;

VPNDriver::VPNDriver(EventDispatcher* dispatcher,
                     Manager* manager,
                     const Property* properties,
                     size_t property_count)
    : weak_ptr_factory_(this),
      dispatcher_(dispatcher),
      manager_(manager),
      properties_(properties),
      property_count_(property_count),
      connect_timeout_seconds_(0) {}

VPNDriver::~VPNDriver() {}

bool VPNDriver::Load(StoreInterface* storage, const string& storage_id) {
  SLOG(this, 2) << __func__;
  for (size_t i = 0; i < property_count_; i++) {
    if ((properties_[i].flags & Property::kEphemeral)) {
      continue;
    }
    const string property = properties_[i].property;
    if (properties_[i].flags & Property::kArray) {
      CHECK(!(properties_[i].flags & Property::kCredential))
          << "Property cannot be both an array and a credential";
      vector<string> value;
      if (storage->GetStringList(storage_id, property, &value)) {
        args_.SetStrings(property, value);
      } else {
        args_.RemoveStrings(property);
      }
    } else {
      string value;
      bool loaded = (properties_[i].flags & Property::kCredential) ?
          storage->GetCryptedString(storage_id, property, &value) :
          storage->GetString(storage_id, property, &value);
      if (loaded) {
        args_.SetString(property, value);
      } else {
        args_.RemoveString(property);
      }
    }
  }
  return true;
}

bool VPNDriver::Save(StoreInterface* storage,
                     const string& storage_id,
                     bool save_credentials) {
  SLOG(this, 2) << __func__;
  for (size_t i = 0; i < property_count_; i++) {
    if ((properties_[i].flags & Property::kEphemeral)) {
      continue;
    }
    bool credential = (properties_[i].flags & Property::kCredential);
    const string property = properties_[i].property;
    if (properties_[i].flags & Property::kArray) {
      CHECK(!credential)
          << "Property cannot be both an array and a credential";
      if (!args_.ContainsStrings(property)) {
        storage->DeleteKey(storage_id, property);
        continue;
      }
      Strings value = args_.GetStrings(property);
      storage->SetStringList(storage_id, property, value);
    } else {
      if (!args_.ContainsString(property) ||
          (credential && !save_credentials)) {
        storage->DeleteKey(storage_id, property);
        continue;
      }
      string value = args_.GetString(property);
      if (credential) {
        storage->SetCryptedString(storage_id, property, value);
      } else {
        storage->SetString(storage_id, property, value);
      }
    }
  }
  return true;
}

void VPNDriver::UnloadCredentials() {
  SLOG(this, 2) << __func__;
  for (size_t i = 0; i < property_count_; i++) {
    if ((properties_[i].flags &
         (Property::kEphemeral | Property::kCredential))) {
      args_.RemoveString(properties_[i].property);
    }
  }
}

void VPNDriver::InitPropertyStore(PropertyStore* store) {
  SLOG(this, 2) << __func__;
  for (size_t i = 0; i < property_count_; i++) {
    if (properties_[i].flags & Property::kArray) {
      store->RegisterDerivedStrings(
          properties_[i].property,
          StringsAccessor(
              new CustomMappedAccessor<VPNDriver, Strings, size_t>(
                  this,
                  &VPNDriver::ClearMappedStringsProperty,
                  &VPNDriver::GetMappedStringsProperty,
                  &VPNDriver::SetMappedStringsProperty,
                  i)));
    } else {
      store->RegisterDerivedString(
          properties_[i].property,
          StringAccessor(
              new CustomMappedAccessor<VPNDriver, string, size_t>(
                  this,
                  &VPNDriver::ClearMappedStringProperty,
                  &VPNDriver::GetMappedStringProperty,
                  &VPNDriver::SetMappedStringProperty,
                  i)));
    }
  }

  store->RegisterDerivedKeyValueStore(
      kProviderProperty,
      KeyValueStoreAccessor(
          new CustomAccessor<VPNDriver, KeyValueStore>(
              this, &VPNDriver::GetProvider, nullptr)));
}

void VPNDriver::ClearMappedStringProperty(const size_t& index, Error* error) {
  CHECK(index < property_count_);
  if (args_.ContainsString(properties_[index].property)) {
    args_.RemoveString(properties_[index].property);
  } else {
    error->Populate(Error::kNotFound, "Property is not set");
  }
}

void VPNDriver::ClearMappedStringsProperty(const size_t& index, Error* error) {
  CHECK(index < property_count_);
  if (args_.ContainsStrings(properties_[index].property)) {
    args_.RemoveStrings(properties_[index].property);
  } else {
    error->Populate(Error::kNotFound, "Property is not set");
  }
}

string VPNDriver::GetMappedStringProperty(const size_t& index, Error* error) {
  // Provider properties are set via SetProperty calls to "Provider.XXX",
  // however, they are retrieved via a GetProperty call, which returns all
  // properties in a single "Provider" dict.  Therefore, none of the individual
  // properties in the kProperties are available for enumeration in
  // GetProperties.  Instead, they are retrieved via GetProvider below.
  error->Populate(Error::kInvalidArguments,
                  "Provider properties are not read back in this manner");
  return string();
}

Strings VPNDriver::GetMappedStringsProperty(const size_t& index, Error* error) {
  // Provider properties are set via SetProperty calls to "Provider.XXX",
  // however, they are retrieved via a GetProperty call, which returns all
  // properties in a single "Provider" dict.  Therefore, none of the individual
  // properties in the kProperties are available for enumeration in
  // GetProperties.  Instead, they are retrieved via GetProvider below.
  error->Populate(Error::kInvalidArguments,
                  "Provider properties are not read back in this manner");
  return Strings();
}

bool VPNDriver::SetMappedStringProperty(
    const size_t& index, const string& value, Error* error) {
  CHECK(index < property_count_);
  if (args_.ContainsString(properties_[index].property) &&
      args_.GetString(properties_[index].property) == value) {
    return false;
  }
  args_.SetString(properties_[index].property, value);
  return true;
}

bool VPNDriver::SetMappedStringsProperty(
    const size_t& index, const Strings& value, Error* error) {
  CHECK(index < property_count_);
  if (args_.ContainsStrings(properties_[index].property) &&
      args_.GetStrings(properties_[index].property) == value) {
    return false;
  }
  args_.SetStrings(properties_[index].property, value);
  return true;
}

KeyValueStore VPNDriver::GetProvider(Error* error) {
  SLOG(this, 2) << __func__;
  string provider_prefix = string(kProviderProperty) + ".";
  KeyValueStore provider_properties;

  for (size_t i = 0; i < property_count_; i++) {
    if ((properties_[i].flags & Property::kWriteOnly)) {
      continue;
    }
    string prop = properties_[i].property;

    // Chomp off leading "Provider." from properties that have this prefix.
    string chopped_prop;
    if (base::StartsWith(prop, provider_prefix,
                         base::CompareCase::INSENSITIVE_ASCII)) {
      chopped_prop = prop.substr(provider_prefix.length());
    } else {
      chopped_prop = prop;
    }

    if (properties_[i].flags & Property::kArray) {
      if (!args_.ContainsStrings(prop)) {
        continue;
      }
      Strings value = args_.GetStrings(prop);
      provider_properties.SetStrings(chopped_prop, value);
    } else {
      if (!args_.ContainsString(prop)) {
        continue;
      }
      string value = args_.GetString(prop);
      provider_properties.SetString(chopped_prop, value);
    }
  }

  return provider_properties;
}

void VPNDriver::StartConnectTimeout(int timeout_seconds) {
  if (IsConnectTimeoutStarted()) {
    return;
  }
  LOG(INFO) << "Schedule VPN connect timeout: "
            << timeout_seconds << " seconds.";
  connect_timeout_seconds_ = timeout_seconds;
  connect_timeout_callback_.Reset(
      Bind(&VPNDriver::OnConnectTimeout, weak_ptr_factory_.GetWeakPtr()));
  dispatcher_->PostDelayedTask(
      connect_timeout_callback_.callback(), timeout_seconds * 1000);
}

void VPNDriver::StopConnectTimeout() {
  SLOG(this, 2) << __func__;
  connect_timeout_callback_.Cancel();
  connect_timeout_seconds_ = 0;
}

bool VPNDriver::IsConnectTimeoutStarted() const {
  return !connect_timeout_callback_.IsCancelled();
}

void VPNDriver::OnConnectTimeout() {
  LOG(INFO) << "VPN connect timeout.";
  StopConnectTimeout();
}

string VPNDriver::GetHost() const {
  return args_.LookupString(kProviderHostProperty, "");
}

}  // namespace shill
