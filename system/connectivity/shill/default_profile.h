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

#ifndef SHILL_DEFAULT_PROFILE_H_
#define SHILL_DEFAULT_PROFILE_H_

#include <random>
#include <string>
#include <vector>

#include <gtest/gtest_prod.h>  // for FRIEND_TEST

#include "shill/event_dispatcher.h"
#include "shill/manager.h"
#include "shill/profile.h"
#include "shill/property_store.h"
#include "shill/refptr_types.h"

namespace shill {

class ControlInterface;
#if !defined(DISABLE_WIFI)
class WiFiProvider;
#endif  // DISABLE_WIFI

class DefaultProfile : public Profile {
 public:
  static const char kDefaultId[];

  DefaultProfile(ControlInterface* control,
                 Metrics* metrics,
                 Manager* manager,
                 const base::FilePath& storage_directory,
                 const std::string& profile_id,
                 const Manager::Properties& manager_props);
  ~DefaultProfile() override;

  // Loads global configuration into manager properties.  This should
  // only be called by the Manager.
  virtual void LoadManagerProperties(Manager::Properties* manager_props,
                                     DhcpProperties* dhcp_properties);

  // Override the Profile superclass implementation to accept all Ethernet
  // services, since these should have an affinity for the default profile.
  bool ConfigureService(const ServiceRefPtr& service) override;

  // Persists profile information, as well as that of discovered devices
  // and bound services, to disk.
  // Returns true on success, false on failure.
  bool Save() override;

  // Inherited from Profile.
  bool UpdateDevice(const DeviceRefPtr& device) override;

#if !defined(DISABLE_WIFI)
  // Inherited from Profile.
  bool UpdateWiFiProvider(const WiFiProvider& wifi_provider) override;
#endif  // DISABLE_WIFI

  bool IsDefault() const override { return true; }

 private:
  friend class DefaultProfileTest;
  FRIEND_TEST(DefaultProfileTest, GetStoragePath);
  FRIEND_TEST(DefaultProfileTest, LoadManagerDefaultProperties);
  FRIEND_TEST(DefaultProfileTest, LoadManagerProperties);
  FRIEND_TEST(DefaultProfileTest, Save);

  static const char kStorageId[];
  static const char kStorageArpGateway[];
  static const char kStorageCheckPortalList[];
  static const char kStorageConnectionIdSalt[];
  static const char kStorageHostName[];
  static const char kStorageIgnoredDNSSearchPaths[];
  static const char kStorageLinkMonitorTechnologies[];
  static const char kStorageName[];
  static const char kStorageNoAutoConnectTechnologies[];
  static const char kStorageOfflineMode[];
  static const char kStoragePortalCheckInterval[];
  static const char kStoragePortalURL[];
  static const char kStorageProhibitedTechnologies[];

  const std::string profile_id_;
  const Manager::Properties& props_;
  std::default_random_engine random_engine_;

  DISALLOW_COPY_AND_ASSIGN(DefaultProfile);
};

}  // namespace shill

#endif  // SHILL_DEFAULT_PROFILE_H_
