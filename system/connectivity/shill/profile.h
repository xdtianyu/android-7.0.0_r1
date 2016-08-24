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

#ifndef SHILL_PROFILE_H_
#define SHILL_PROFILE_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include <base/files/file_path.h>
#include <gtest/gtest_prod.h>  // for FRIEND_TEST

#include "shill/event_dispatcher.h"
#include "shill/property_store.h"
#include "shill/refptr_types.h"

namespace base {

class FilePath;

}  // namespace base

namespace shill {

class ControlInterface;
class Error;
class Manager;
class Metrics;
class ProfileAdaptorInterface;
class StoreInterface;

#if !defined(DISABLE_WIFI)
class WiFiProvider;
#endif  // DISABLE_WIFI

class Profile : public base::RefCounted<Profile> {
 public:
  enum InitStorageOption {
    kOpenExisting,
    kCreateNew,
    kCreateOrOpenExisting
  };
  struct Identifier {
    Identifier() {}
    explicit Identifier(const std::string& i) : identifier(i) {}
    Identifier(const std::string& u, const std::string& i)
        : user(u),
          identifier(i) {
    }
    std::string user;  // Empty for global.
    std::string identifier;
    std::string user_hash;
  };

  // Path to the cached list of inserted user profiles to be loaded at
  // startup.
  static const char kUserProfileListPathname[];

  Profile(ControlInterface* control_interface,
          Metrics* metrics,
          Manager* manager,
          const Identifier& name,
          const base::FilePath& storage_directory,
          bool connect_to_rpc);

  virtual ~Profile();

  // Set up persistent storage for this Profile.
  bool InitStorage(InitStorageOption storage_option,
                   Error* error);

  // Set up stub storage for this Profile. The data will NOT be
  // persisted. In most cases, you should prefer InitStorage.
  void InitStubStorage();

  // Remove the persistent storage for this Profile.  It is an error to
  // do so while the underlying storage is open via InitStorage() or
  // set_storage().
  bool RemoveStorage(Error* error);

  virtual std::string GetFriendlyName();

  virtual std::string GetRpcIdentifier();

  PropertyStore* mutable_store() { return &store_; }
  const PropertyStore& store() const { return store_; }

  // Set the storage inteface.  This is used for testing purposes.  It
  // takes ownership of |storage|.
  void set_storage(StoreInterface* storage);

  // Begin managing the persistence of |service|.
  // Returns true if |service| is new to this profile and was added,
  // false if the |service| already existed.
  virtual bool AdoptService(const ServiceRefPtr& service);

  // Cease managing the persistence of the Service |service|.
  // Returns true if |service| was found and abandoned, or not found.
  // Returns false if can't be abandoned.
  virtual bool AbandonService(const ServiceRefPtr& service);

  // Clobbers persisted notion of |service| with data from |service|.
  // Returns true if |service| was found and updated, false if not found.
  virtual bool UpdateService(const ServiceRefPtr& service);

  // Ask |service| if it can configure itself from the profile.  If it can,
  // ask |service| to perform the configuration and return true.  If not,
  // return false.
  virtual bool LoadService(const ServiceRefPtr& service);

  // Perform LoadService() on |service|.  If this succeeds, change
  // the service to point at this profile and return true.  If not, return
  // false.
  virtual bool ConfigureService(const ServiceRefPtr& service);

  // Allow the device to configure itself from this profile.  Returns
  // true if the device succeeded in finding its configuration.  If not,
  // return false.
  virtual bool ConfigureDevice(const DeviceRefPtr& device);

  // Remove a named entry from the profile.  This includes detaching
  // any service that uses this profile entry.
  virtual void DeleteEntry(const std::string& entry_name, Error* error);

  // Return a service configured from the given profile entry.
  // Callers must not register the returned service with the Manager or connect
  // it since it might not be in the provider's service list.
  virtual ServiceRefPtr GetServiceFromEntry(const std::string& entry_name,
                                            Error* error);

  // Return whether |service| can configure itself from the profile.
  bool ContainsService(const ServiceConstRefPtr& service);

  std::vector<std::string> EnumerateAvailableServices(Error* error);
  std::vector<std::string> EnumerateEntries(Error* error);

  // Clobbers persisted notion of |device| with data from |device|. Returns true
  // if |device| was found and updated, false otherwise. The base implementation
  // always returns false -- currently devices are persisted only in
  // DefaultProfile.
  virtual bool UpdateDevice(const DeviceRefPtr& device);

#if !defined(DISABLE_WIFI)
  // Clobbers persisted notion of |wifi_provider| with data from
  // |wifi_provider|. Returns true if |wifi_provider| was found and updated,
  // false otherwise. The base implementation always returns false -- currently
  // wifi_provider is persisted only in DefaultProfile.
  virtual bool UpdateWiFiProvider(const WiFiProvider& wifi_provider);
#endif  // DISABLE_WIFI

  // Write all in-memory state to disk via |storage_|.
  virtual bool Save();

  // Parses a profile identifier. There're two acceptable forms of the |raw|
  // identifier: "identifier" and "~user/identifier". Both "user" and
  // "identifier" must be suitable for use in a D-Bus object path. Returns true
  // on success.
  static bool ParseIdentifier(const std::string& raw, Identifier* parsed);

  // Returns the composite string identifier for a profile, as would have
  // been used in an argument to Manager::PushProfile() in creating this
  // profile.  It returns a string in the form "identifier", or
  // "~user/identifier" depending on whether this profile has a user
  // component.
  static std::string IdentifierToString(const Identifier& name);

  // Load a list of user profile identifiers from a cache file |path|.
  // The profiles themselves are not loaded.
  static std::vector<Identifier> LoadUserProfileList(
      const base::FilePath& path);

  // Save a list of user profile identifiers |profiles| to a cache file |path|.
  // Returns true if successful, false otherwise.
  static bool SaveUserProfileList(const base::FilePath& path,
                                  const std::vector<ProfileRefPtr>& profiles);

  // Returns whether |name| matches this Profile's |name_|.
  virtual bool MatchesIdentifier(const Identifier& name) const;

  // Returns the username component of the profile identifier.
  const std::string& GetUser() const { return name_.user; }

  // Returns the user_hash component of the profile identifier.
  const std::string& GetUserHash() const { return name_.user_hash; }

  virtual StoreInterface* GetStorage() {
    return storage_.get();
  }

  // Returns a read-only copy of the backing storage of the profile.
  virtual const StoreInterface* GetConstStorage() const {
    return storage_.get();
  }

  virtual bool IsDefault() const { return false; }

 protected:
  // Returns the persistent store file path for a Profile with the
  // given |storage_dir| and |profile_name|. Provided as a static
  // method, so that tests can use this logic without having to
  // instantiate a Profile.
  static base::FilePath GetFinalStoragePath(
      const base::FilePath& storage_dir,
      const Identifier& profile_name);

  Metrics* metrics() const { return metrics_; }
  Manager* manager() const { return manager_; }
  StoreInterface* storage() { return storage_.get(); }
  const base::FilePath& persistent_profile_path() {
    return persistent_profile_path_;
  }
  void set_persistent_profile_path(const base::FilePath& path) {
    persistent_profile_path_ = path;
  }

 private:
  friend class ManagerTest;
  friend class ProfileAdaptorInterface;
  FRIEND_TEST(ManagerTest, CreateDuplicateProfileWithMissingKeyfile);
  FRIEND_TEST(ManagerTest, RemoveProfile);
  FRIEND_TEST(ProfileTest, DeleteEntry);
  FRIEND_TEST(ProfileTest, GetStoragePath);
  FRIEND_TEST(ProfileTest, IsValidIdentifierToken);
  FRIEND_TEST(ProfileTest, GetServiceFromEntry);

  static bool IsValidIdentifierToken(const std::string& token);

  void HelpRegisterConstDerivedStrings(
      const std::string& name,
      Strings(Profile::*get)(Error* error));

  // Data members shared with subclasses via getter/setters above in the
  // protected: section
  Metrics* metrics_;
  Manager* manager_;
  ControlInterface* control_interface_;
  base::FilePath persistent_profile_path_;

  // Shared with |adaptor_| via public getter.
  PropertyStore store_;

  // Properties to be gotten via PropertyStore calls.
  Identifier name_;

  // Allows this profile to be backed with on-disk storage.
  std::unique_ptr<StoreInterface> storage_;

  std::unique_ptr<ProfileAdaptorInterface> adaptor_;

  DISALLOW_COPY_AND_ASSIGN(Profile);
};

}  // namespace shill

#endif  // SHILL_PROFILE_H_
