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

#include "shill/profile.h"

#include <set>
#include <string>
#include <vector>

#include <base/files/file_util.h>
#include <base/stl_util.h>
#include <base/strings/string_split.h>
#include <base/strings/string_util.h>
#include <base/strings/stringprintf.h>
#if defined(__ANDROID__)
#include <dbus/service_constants.h>
#else
#include <chromeos/dbus/service_constants.h>
#endif  // __ANDROID__

#include "shill/adaptor_interfaces.h"
#include "shill/control_interface.h"
#include "shill/logging.h"
#include "shill/manager.h"
#include "shill/property_accessor.h"
#include "shill/service.h"
#include "shill/store_factory.h"
#include "shill/store_interface.h"
#include "shill/stub_storage.h"

using base::FilePath;
using std::set;
using std::string;
using std::vector;

namespace shill {

#if defined(ENABLE_JSON_STORE)
namespace {
const char kFileExtensionJson[] = "json";
}
#endif

// static
const char Profile::kUserProfileListPathname[] =
    RUNDIR "/loaded_profile_list";

Profile::Profile(ControlInterface* control_interface,
                 Metrics* metrics,
                 Manager* manager,
                 const Identifier& name,
                 const base::FilePath& storage_directory,
                 bool connect_to_rpc)
    : metrics_(metrics),
      manager_(manager),
      control_interface_(control_interface),
      name_(name) {
  if (connect_to_rpc)
    adaptor_.reset(control_interface->CreateProfileAdaptor(this));

  // kCheckPortalListProperty: Registered in DefaultProfile
  // kCountryProperty: Registered in DefaultProfile
  store_.RegisterConstString(kNameProperty, &name_.identifier);
  store_.RegisterConstString(kUserHashProperty, &name_.user_hash);

  // kOfflineModeProperty: Registered in DefaultProfile
  // kPortalURLProperty: Registered in DefaultProfile

  HelpRegisterConstDerivedStrings(kServicesProperty,
                                  &Profile::EnumerateAvailableServices);
  HelpRegisterConstDerivedStrings(kEntriesProperty, &Profile::EnumerateEntries);

  if (name.user.empty()) {
    // Subtle: Profile is only directly instantiated for user
    // profiles. And user profiles must have a non-empty
    // |name.user|. So we want to CHECK here. But Profile is also the
    // base class for DefaultProfile. So a CHECK here would cause us
    // to abort whenever we attempt to instantiate a DefaultProfile.
    //
    // Instead, we leave |persistent_profile_path_| unintialized. One
    // of two things will happen: a) we become a DefaultProfile, and
    // the DefaultProfile ctor sets |persistent_profile_path_|, or b)
    // we really are destined to be a user Profile. In the latter
    // case, our |name| argument was invalid,
    // |persistent_profile_path_| is never set, and we CHECK for an
    // empty |persistent_profile_path_| in InitStorage().
    //
    // TODO(quiche): Clean this up. crbug.com/527553
  } else {
    persistent_profile_path_ = GetFinalStoragePath(storage_directory, name);
  }
}

Profile::~Profile() {}

bool Profile::InitStorage(InitStorageOption storage_option, Error* error) {
  CHECK(!persistent_profile_path_.empty());
  std::unique_ptr<StoreInterface> storage(
      StoreFactory::GetInstance()->CreateStore(persistent_profile_path_));
  bool already_exists = storage->IsNonEmpty();
  if (!already_exists && storage_option != kCreateNew &&
      storage_option != kCreateOrOpenExisting) {
    Error::PopulateAndLog(
        FROM_HERE, error, Error::kNotFound,
        base::StringPrintf("Profile storage for %s:%s does not already exist",
                           name_.user.c_str(), name_.identifier.c_str()));
    return false;
  } else if (already_exists && storage_option != kOpenExisting &&
             storage_option != kCreateOrOpenExisting) {
    Error::PopulateAndLog(
        FROM_HERE, error, Error::kAlreadyExists,
        base::StringPrintf("Profile storage for %s:%s already exists",
                           name_.user.c_str(), name_.identifier.c_str()));
    return false;
  }
  if (!storage->Open()) {
    Error::PopulateAndLog(
        FROM_HERE, error, Error::kInternalError,
        base::StringPrintf("Could not open profile storage for %s:%s",
                           name_.user.c_str(), name_.identifier.c_str()));
    if (already_exists) {
      // The profile contents are corrupt, or we do not have access to
      // this file.  Move this file out of the way so a future open attempt
      // will succeed, assuming the failure reason was the former.
      storage->MarkAsCorrupted();
      metrics_->NotifyCorruptedProfile();
    }
    return false;
  }
  if (!already_exists) {
    // Add a descriptive header to the profile so even if nothing is stored
    // to it, it still has some content.  Completely empty keyfiles are not
    // valid for reading.
    storage->SetHeader(
        base::StringPrintf("Profile %s:%s", name_.user.c_str(),
                           name_.identifier.c_str()));
  }
  set_storage(storage.release());
  manager_->OnProfileStorageInitialized(this);
  return true;
}

void Profile::InitStubStorage() {
  set_storage(new StubStorage());
}

bool Profile::RemoveStorage(Error* error) {
  CHECK(!storage_.get());
  CHECK(!persistent_profile_path_.empty());

  if (!base::DeleteFile(persistent_profile_path_, false)) {
    Error::PopulateAndLog(
        FROM_HERE, error, Error::kOperationFailed,
        base::StringPrintf("Could not remove path %s",
                           persistent_profile_path_.value().c_str()));
    return false;
  }

  return true;
}

string Profile::GetFriendlyName() {
  return (name_.user.empty() ? "" : name_.user + "/") + name_.identifier;
}

string Profile::GetRpcIdentifier() {
  if (!adaptor_.get()) {
    return string();
  }
  return adaptor_->GetRpcIdentifier();
}

void Profile::set_storage(StoreInterface* storage) {
  storage_.reset(storage);
}

bool Profile::AdoptService(const ServiceRefPtr& service) {
  if (service->profile() == this) {
    return false;
  }
  service->SetProfile(this);
  return service->Save(storage_.get()) && storage_->Flush();
}

bool Profile::AbandonService(const ServiceRefPtr& service) {
  if (service->profile() == this)
    service->SetProfile(nullptr);
  return storage_->DeleteGroup(service->GetStorageIdentifier()) &&
      storage_->Flush();
}

bool Profile::UpdateService(const ServiceRefPtr& service) {
  return service->Save(storage_.get()) && storage_->Flush();
}

bool Profile::LoadService(const ServiceRefPtr& service) {
  if (!ContainsService(service))
    return false;
  return service->Load(storage_.get());
}

bool Profile::ConfigureService(const ServiceRefPtr& service) {
  if (!LoadService(service))
    return false;
  service->SetProfile(this);
  return true;
}

bool Profile::ConfigureDevice(const DeviceRefPtr& device) {
  return device->Load(storage_.get());
}

bool Profile::ContainsService(const ServiceConstRefPtr& service) {
  return service->IsLoadableFrom(*storage_.get());
}

void Profile::DeleteEntry(const std::string& entry_name, Error* error) {
  if (!storage_->ContainsGroup(entry_name)) {
    Error::PopulateAndLog(
        FROM_HERE, error, Error::kNotFound,
        base::StringPrintf("Entry %s does not exist in profile",
                           entry_name.c_str()));
    return;
  }
  if (!manager_->HandleProfileEntryDeletion(this, entry_name)) {
    // If HandleProfileEntryDeletion() returns succeeds, DeleteGroup()
    // has already been called when AbandonService was called.
    // Otherwise, we need to delete the group ourselves.
    storage_->DeleteGroup(entry_name);
  }
  Save();
}

ServiceRefPtr Profile::GetServiceFromEntry(const std::string& entry_name,
                                           Error* error) {
  if (!storage_->ContainsGroup(entry_name)) {
    Error::PopulateAndLog(
        FROM_HERE, error, Error::kNotFound,
        base::StringPrintf("Entry %s does not exist in profile",
                           entry_name.c_str()));
    return nullptr;
  }

  // Lookup the service entry from the registered services.
  ServiceRefPtr service =
      manager_->GetServiceWithStorageIdentifier(this, entry_name, error);
  if (service) {
    return service;
  }

  // Load the service entry to a temporary service.
  return manager_->CreateTemporaryServiceFromProfile(this, entry_name, error);
}

bool Profile::IsValidIdentifierToken(const string& token) {
  if (token.empty()) {
    return false;
  }
  for (auto chr : token) {
    if (!base::IsAsciiAlpha(chr) && !base::IsAsciiDigit(chr)) {
      return false;
    }
  }
  return true;
}

// static
bool Profile::ParseIdentifier(const string& raw, Identifier* parsed) {
  if (raw.empty()) {
    return false;
  }
  if (raw[0] == '~') {
    // Format: "~user/identifier".
    size_t slash = raw.find('/');
    if (slash == string::npos) {
      return false;
    }
    string user(raw.begin() + 1, raw.begin() + slash);
    string identifier(raw.begin() + slash + 1, raw.end());
    if (!IsValidIdentifierToken(user) || !IsValidIdentifierToken(identifier)) {
      return false;
    }
    parsed->user = user;
    parsed->identifier = identifier;
    return true;
  }

  // Format: "identifier".
  if (!IsValidIdentifierToken(raw)) {
    return false;
  }
  parsed->user = "";
  parsed->identifier = raw;
  return true;
}

// static
string Profile::IdentifierToString(const Identifier& name) {
  if (name.user.empty()) {
    // Format: "identifier".
    return name.identifier;
  }

  // Format: "~user/identifier".
  return base::StringPrintf(
      "~%s/%s", name.user.c_str(), name.identifier.c_str());
}

// static
vector<Profile::Identifier> Profile::LoadUserProfileList(const FilePath& path) {
  vector<Identifier> profile_identifiers;
  string profile_data;
  if (!base::ReadFileToString(path, &profile_data)) {
    return profile_identifiers;
  }

  vector<string> profile_lines =
      base::SplitString(profile_data, "\n", base::KEEP_WHITESPACE,
                        base::SPLIT_WANT_ALL);
  for (const auto& line : profile_lines) {
    if (line.empty()) {
      // This will be the case on the last line, so let's not complain about it.
      continue;
    }
    size_t space = line.find(' ');
    if (space == string::npos || space == 0) {
      LOG(ERROR) << "Invalid line found in " << path.value()
                 << ": " << line;
      continue;
    }
    string name(line.begin(), line.begin() + space);
    Identifier identifier;
    if (!ParseIdentifier(name, &identifier) || identifier.user.empty()) {
      LOG(ERROR) << "Invalid profile name found in " << path.value()
                 << ": " << name;
      continue;
    }
    identifier.user_hash = string(line.begin() + space + 1, line.end());
    profile_identifiers.push_back(identifier);
  }

  return profile_identifiers;
}

// static
bool Profile::SaveUserProfileList(const FilePath& path,
                                  const vector<ProfileRefPtr>& profiles) {
  vector<string> lines;
  for (const auto& profile : profiles) {
    Identifier& id = profile->name_;
    if (id.user.empty()) {
      continue;
    }
    lines.push_back(base::StringPrintf("%s %s\n",
                                       IdentifierToString(id).c_str(),
                                       id.user_hash.c_str()));
  }
  string content = base::JoinString(lines, "");
  size_t ret = base::WriteFile(path, content.c_str(), content.length());
  return ret == content.length();
}

bool Profile::MatchesIdentifier(const Identifier& name) const {
  return name.user == name_.user && name.identifier == name_.identifier;
}

bool Profile::Save() {
  return storage_->Flush();
}

vector<string> Profile::EnumerateAvailableServices(Error* error) {
  // We should return the Manager's service list if this is the active profile.
  if (manager_->IsActiveProfile(this)) {
    return manager_->EnumerateAvailableServices(error);
  } else {
    return vector<string>();
  }
}

vector<string> Profile::EnumerateEntries(Error* /*error*/) {
  vector<string> service_groups;

  // Filter this list down to only entries that correspond
  // to a technology.  (wifi_*, etc)
  for (const auto& group : storage_->GetGroups()) {
    if (Technology::IdentifierFromStorageGroup(group) != Technology::kUnknown)
      service_groups.push_back(group);
  }

  return service_groups;
}

bool Profile::UpdateDevice(const DeviceRefPtr& device) {
  return false;
}

#if !defined(DISABLE_WIFI)
bool Profile::UpdateWiFiProvider(const WiFiProvider& wifi_provider) {
  return false;
}
#endif  // DISABLE_WIFI

void Profile::HelpRegisterConstDerivedStrings(
    const string& name,
    Strings(Profile::*get)(Error*)) {
  store_.RegisterDerivedStrings(
      name, StringsAccessor(
                new CustomAccessor<Profile, Strings>(this, get, nullptr)));
}

// static
FilePath Profile::GetFinalStoragePath(
    const FilePath& storage_dir,
    const Identifier& profile_name) {
  FilePath base_path;
  if (profile_name.user.empty()) {  // True for DefaultProfiles.
    base_path = storage_dir.Append(
        base::StringPrintf("%s.profile", profile_name.identifier.c_str()));
  } else {
    base_path = storage_dir.Append(
        base::StringPrintf("%s/%s.profile",
                           profile_name.user.c_str(),
                           profile_name.identifier.c_str()));
  }

  // TODO(petkov): Validate the directory permissions, etc.

#if defined(ENABLE_JSON_STORE)
  return base_path.AddExtension(kFileExtensionJson);
#else
  return base_path;
#endif
}

}  // namespace shill
