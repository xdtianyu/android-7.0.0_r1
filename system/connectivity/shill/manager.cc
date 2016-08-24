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

#include "shill/manager.h"

#include <stdio.h>
#include <time.h>

#include <algorithm>
#include <set>
#include <string>
#include <vector>

#include <base/bind.h>
#include <base/callback.h>
#include <base/files/file_util.h>
#include <base/memory/ref_counted.h>
#include <base/strings/pattern.h>
#include <base/strings/stringprintf.h>
#include <base/strings/string_split.h>
#include <base/strings/string_util.h>
#if defined(__ANDROID__)
#include <dbus/service_constants.h>
#else
#include <chromeos/dbus/service_constants.h>
#endif  // __ANDROID__

#include "shill/adaptor_interfaces.h"
#include "shill/callbacks.h"
#include "shill/connection.h"
#include "shill/control_interface.h"
#include "shill/default_profile.h"
#include "shill/device.h"
#include "shill/device_claimer.h"
#include "shill/device_info.h"
#include "shill/ephemeral_profile.h"
#include "shill/error.h"
#include "shill/ethernet/ethernet_temporary_service.h"
#include "shill/event_dispatcher.h"
#include "shill/geolocation_info.h"
#include "shill/hook_table.h"
#include "shill/ip_address_store.h"
#include "shill/logging.h"
#include "shill/profile.h"
#include "shill/property_accessor.h"
#include "shill/resolver.h"
#include "shill/result_aggregator.h"
#include "shill/service.h"
#include "shill/service_sorter.h"
#include "shill/store_factory.h"
#include "shill/vpn/vpn_provider.h"
#include "shill/vpn/vpn_service.h"
#include "shill/wimax/wimax_service.h"

#if defined(__BRILLO__)
#include "shill/wifi/wifi_driver_hal.h"
#endif  // __BRILLO__

#if !defined(DISABLE_WIFI)
#include "shill/wifi/wifi.h"
#include "shill/wifi/wifi_provider.h"
#include "shill/wifi/wifi_service.h"
#endif  // DISABLE_WIFI

#if !defined(DISABLE_WIRED_8021X)
#include "shill/ethernet/ethernet_eap_provider.h"
#include "shill/ethernet/ethernet_eap_service.h"
#endif  // DISABLE_WIRED_8021X

using base::Bind;
using base::Callback;
using base::FilePath;
using base::StringPrintf;
using base::Unretained;
using std::map;
using std::set;
using std::string;
using std::vector;

namespace shill {

namespace Logging {
static auto kModuleLogScope = ScopeLogger::kManager;
static string ObjectID(const Manager* m) { return "manager"; }
}


// statics
const char Manager::kErrorNoDevice[] = "no wifi devices available";
const char Manager::kErrorTypeRequired[] = "must specify service type";
const char Manager::kErrorUnsupportedServiceType[] =
    "service type is unsupported";
// This timeout should be less than the upstart job timeout, otherwise
// stats for termination actions might be lost.
const int Manager::kTerminationActionsTimeoutMilliseconds = 19500;

// Device status check interval (every 3 minutes).
const int Manager::kDeviceStatusCheckIntervalMilliseconds = 180000;

// static
const char* Manager::kProbeTechnologies[] = {
    kTypeEthernet,
    kTypeWifi,
    kTypeWimax,
    kTypeCellular
};

// static
const char Manager::kDefaultClaimerName[] = "";

Manager::Manager(ControlInterface* control_interface,
                 EventDispatcher* dispatcher,
                 Metrics* metrics,
                 const string& run_directory,
                 const string& storage_directory,
                 const string& user_storage_directory)
    : dispatcher_(dispatcher),
      run_path_(FilePath(run_directory)),
      storage_path_(FilePath(storage_directory)),
      user_storage_path_(user_storage_directory),
      user_profile_list_path_(FilePath(Profile::kUserProfileListPathname)),
      adaptor_(control_interface->CreateManagerAdaptor(this)),
      device_info_(control_interface, dispatcher, metrics, this),
#if !defined(DISABLE_CELLULAR)
      modem_info_(control_interface, dispatcher, metrics, this),
#endif  // DISABLE_CELLULAR
#if !defined(DISABLE_WIRED_8021X)
      ethernet_eap_provider_(
          new EthernetEapProvider(
              control_interface, dispatcher, metrics, this)),
#endif  // DISABLE_WIRED_8021X
      vpn_provider_(
          new VPNProvider(control_interface, dispatcher, metrics, this)),
#if !defined(DISABLE_WIFI)
      wifi_provider_(
          new WiFiProvider(control_interface, dispatcher, metrics, this)),
#if defined(__BRILLO__)
      wifi_driver_hal_(WiFiDriverHal::GetInstance()),
#endif  // __BRILLO__
#endif  // DISABLE_WIFI
#if !defined(DISABLE_WIMAX)
      wimax_provider_(
          new WiMaxProvider(control_interface, dispatcher, metrics, this)),
#endif  // DISABLE_WIMAX
      resolver_(Resolver::GetInstance()),
      running_(false),
      connect_profiles_to_rpc_(true),
      ephemeral_profile_(
          new EphemeralProfile(control_interface, metrics, this)),
      control_interface_(control_interface),
      metrics_(metrics),
      use_startup_portal_list_(false),
      device_status_check_task_(Bind(&Manager::DeviceStatusCheckTask,
                                     base::Unretained(this))),
      termination_actions_(dispatcher),
      suspend_delay_registered_(false),
      is_wake_on_lan_enabled_(true),
      ignore_unknown_ethernet_(false),
      default_service_callback_tag_(0),
      crypto_util_proxy_(new CryptoUtilProxy(dispatcher)),
      health_checker_remote_ips_(new IPAddressStore()),
      suppress_autoconnect_(false),
      is_connected_state_(false),
      dhcp_properties_(new DhcpProperties()) {
  HelpRegisterDerivedString(kActiveProfileProperty,
                            &Manager::GetActiveProfileRpcIdentifier,
                            nullptr);
  store_.RegisterBool(kArpGatewayProperty, &props_.arp_gateway);
  HelpRegisterConstDerivedStrings(kAvailableTechnologiesProperty,
                                  &Manager::AvailableTechnologies);
  HelpRegisterDerivedString(kCheckPortalListProperty,
                            &Manager::GetCheckPortalList,
                            &Manager::SetCheckPortalList);
  HelpRegisterConstDerivedStrings(kConnectedTechnologiesProperty,
                                  &Manager::ConnectedTechnologies);
  store_.RegisterConstString(kConnectionStateProperty, &connection_state_);
  store_.RegisterString(kCountryProperty, &props_.country);
  HelpRegisterDerivedString(kDefaultTechnologyProperty,
                            &Manager::DefaultTechnology,
                            nullptr);
  HelpRegisterConstDerivedRpcIdentifier(
      kDefaultServiceProperty, &Manager::GetDefaultServiceRpcIdentifier);
  HelpRegisterConstDerivedRpcIdentifiers(kDevicesProperty,
                                         &Manager::EnumerateDevices);
#if !defined(DISABLE_WIFI)
  HelpRegisterDerivedBool(kDisableWiFiVHTProperty,
                          &Manager::GetDisableWiFiVHT,
                          &Manager::SetDisableWiFiVHT);
#endif  // DISABLE_WIFI
  HelpRegisterConstDerivedStrings(kEnabledTechnologiesProperty,
                                  &Manager::EnabledTechnologies);
  HelpRegisterDerivedString(kIgnoredDNSSearchPathsProperty,
                            &Manager::GetIgnoredDNSSearchPaths,
                            &Manager::SetIgnoredDNSSearchPaths);
  store_.RegisterString(kHostNameProperty, &props_.host_name);
  store_.RegisterString(kLinkMonitorTechnologiesProperty,
                        &props_.link_monitor_technologies);
  store_.RegisterString(kNoAutoConnectTechnologiesProperty,
                        &props_.no_auto_connect_technologies);
  store_.RegisterBool(kOfflineModeProperty, &props_.offline_mode);
  store_.RegisterString(kPortalURLProperty, &props_.portal_url);
  store_.RegisterInt32(kPortalCheckIntervalProperty,
                       &props_.portal_check_interval_seconds);
  HelpRegisterConstDerivedRpcIdentifiers(kProfilesProperty,
                                         &Manager::EnumerateProfiles);
  HelpRegisterDerivedString(kProhibitedTechnologiesProperty,
                            &Manager::GetProhibitedTechnologies,
                            &Manager::SetProhibitedTechnologies);
  HelpRegisterDerivedString(kStateProperty,
                            &Manager::CalculateState,
                            nullptr);
  HelpRegisterConstDerivedRpcIdentifiers(kServicesProperty,
                                         &Manager::EnumerateAvailableServices);
  HelpRegisterConstDerivedRpcIdentifiers(kServiceCompleteListProperty,
                                         &Manager::EnumerateCompleteServices);
  HelpRegisterConstDerivedRpcIdentifiers(kServiceWatchListProperty,
                                         &Manager::EnumerateWatchedServices);
  HelpRegisterConstDerivedStrings(kUninitializedTechnologiesProperty,
                                  &Manager::UninitializedTechnologies);
  store_.RegisterBool(kWakeOnLanEnabledProperty, &is_wake_on_lan_enabled_);
  HelpRegisterConstDerivedStrings(kClaimedDevicesProperty,
                                  &Manager::ClaimedDevices);

  UpdateProviderMapping();

  dhcp_properties_->InitPropertyStore(&store_);

  SLOG(this, 2) << "Manager initialized.";
}

Manager::~Manager() {}

void Manager::RegisterAsync(const Callback<void(bool)>& completion_callback) {
  adaptor_->RegisterAsync(completion_callback);
}

void Manager::SetBlacklistedDevices(const vector<string>& blacklisted_devices) {
  blacklisted_devices_ = blacklisted_devices;
}

void Manager::SetWhitelistedDevices(const vector<string>& whitelisted_devices) {
  whitelisted_devices_ = whitelisted_devices;
}

void Manager::Start() {
  LOG(INFO) << "Manager started.";

  power_manager_.reset(
      new PowerManager(dispatcher_, control_interface_));
  power_manager_->Start(base::TimeDelta::FromMilliseconds(
                            kTerminationActionsTimeoutMilliseconds),
                        Bind(&Manager::OnSuspendImminent, AsWeakPtr()),
                        Bind(&Manager::OnSuspendDone, AsWeakPtr()),
                        Bind(&Manager::OnDarkSuspendImminent, AsWeakPtr()));
  upstart_.reset(new Upstart(control_interface_));

  CHECK(base::CreateDirectory(run_path_)) << run_path_.value();
  resolver_->set_path(run_path_.Append("resolv.conf"));

  InitializeProfiles();
  running_ = true;
  device_info_.Start();
#if !defined(DISABLE_CELLULAR)
  modem_info_.Start();
#endif  // DISABLE_CELLULAR
  for (const auto& provider_mapping : providers_) {
    provider_mapping.second->Start();
  }

  // Start task for checking connection status.
  dispatcher_->PostDelayedTask(device_status_check_task_.callback(),
                               kDeviceStatusCheckIntervalMilliseconds);
}

void Manager::Stop() {
  running_ = false;
  // Persist device information to disk;
  for (const auto& device : devices_) {
    UpdateDevice(device);
  }

#if !defined(DISABLE_WIFI)
  UpdateWiFiProvider();
#endif  // DISABLE_WIFI

  // Persist profile, service information to disk.
  for (const auto& profile : profiles_) {
    // Since this happens in a loop, the current manager state is stored to
    // all default profiles in the stack.  This is acceptable because the
    // only time multiple default profiles are loaded are during autotests.
    profile->Save();
  }

  Error e;
  for (const auto& service : services_) {
    service->Disconnect(&e, __func__);
  }

  for (const auto& device : devices_) {
    device->SetEnabled(false);
  }

  for (const auto& provider_mapping : providers_) {
    provider_mapping.second->Stop();
  }
#if !defined(DISABLE_CELLULAR)
  modem_info_.Stop();
#endif  // DISABLE_CELLULAR
  device_info_.Stop();
  device_status_check_task_.Cancel();
  sort_services_task_.Cancel();
  power_manager_->Stop();
  power_manager_.reset();
}

void Manager::InitializeProfiles() {
  DCHECK(profiles_.empty());  // The default profile must go first on stack.
  CHECK(base::CreateDirectory(storage_path_)) << storage_path_.value();

  // Ensure that we have storage for the default profile, and that
  // the persistent copy of the default profile is not corrupt.
  scoped_refptr<DefaultProfile>
      default_profile(new DefaultProfile(control_interface_,
                                         metrics_,
                                         this,
                                         storage_path_,
                                         DefaultProfile::kDefaultId,
                                         props_));
  // The default profile may fail to initialize if it's corrupted.
  // If so, recreate the default profile.
  if (!default_profile->InitStorage(Profile::kCreateOrOpenExisting, nullptr))
    CHECK(default_profile->InitStorage(Profile::kCreateNew, nullptr));
  // In case we created a new profile, initialize its default values,
  // and then save. This is required for properties such as
  // PortalDetector::kDefaultCheckPortalList to be initialized correctly.
  LoadProperties(default_profile);
  default_profile->Save();
  default_profile = nullptr;  // PushProfileInternal will re-create.

  // Read list of user profiles. This must be done before pushing the
  // default profile, because modifying the profile stack updates the
  // user profile list.
  vector<Profile::Identifier> identifiers =
      Profile::LoadUserProfileList(user_profile_list_path_);

  // Push the default profile onto the stack.
  Error error;
  string path;
  Profile::Identifier default_profile_id;
  CHECK(Profile::ParseIdentifier(
      DefaultProfile::kDefaultId, &default_profile_id));
  PushProfileInternal(default_profile_id, &path, &error);
  CHECK(!profiles_.empty());  // Must have a default profile.

  // Push user profiles onto the stack.
  for (const auto& profile_id : identifiers)  {
    PushProfileInternal(profile_id, &path, &error);
  }
}

void Manager::CreateProfile(const string& name, string* path, Error* error) {
  SLOG(this, 2) << __func__ << " " << name;
  Profile::Identifier ident;
  if (!Profile::ParseIdentifier(name, &ident)) {
    Error::PopulateAndLog(FROM_HERE, error, Error::kInvalidArguments,
                          "Invalid profile name " + name);
    return;
  }

  if (HasProfile(ident)) {
    Error::PopulateAndLog(FROM_HERE, error, Error::kAlreadyExists,
                          "Profile name " + name + " is already on stack");
    return;
  }

  ProfileRefPtr profile;
  if (ident.user.empty()) {
    profile = new DefaultProfile(control_interface_,
                                 metrics_,
                                 this,
                                 storage_path_,
                                 ident.identifier,
                                 props_);
  } else {
    profile = new Profile(control_interface_,
                          metrics_,
                          this,
                          ident,
                          user_storage_path_,
                          true);
  }

  if (!profile->InitStorage(Profile::kCreateNew, error)) {
    // |error| will have been populated by InitStorage().
    return;
  }

  // Save profile data out, and then let the scoped pointer fall out of scope.
  if (!profile->Save()) {
    Error::PopulateAndLog(FROM_HERE, error, Error::kInternalError,
                          "Profile name " + name + " could not be saved");
    return;
  }

  *path = profile->GetRpcIdentifier();
}

bool Manager::HasProfile(const Profile::Identifier& ident) {
  for (const auto& profile : profiles_) {
    if (profile->MatchesIdentifier(ident)) {
      return true;
    }
  }
  return false;
}

void Manager::PushProfileInternal(
    const Profile::Identifier& ident, string* path, Error* error) {
  if (HasProfile(ident)) {
    Error::PopulateAndLog(FROM_HERE, error, Error::kAlreadyExists,
                          "Profile name " + Profile::IdentifierToString(ident) +
                          " is already on stack");
    return;
  }

  ProfileRefPtr profile;
  if (ident.user.empty()) {
    // Allow a machine-wide-profile to be pushed on the stack only if the
    // profile stack is empty, or if the topmost profile on the stack is
    // also a machine-wide (non-user) profile.
    if (!profiles_.empty() && !profiles_.back()->GetUser().empty()) {
      Error::PopulateAndLog(FROM_HERE, error, Error::kInvalidArguments,
                            "Cannot load non-default global profile " +
                            Profile::IdentifierToString(ident) +
                            " on top of a user profile");
      return;
    }

    scoped_refptr<DefaultProfile>
        default_profile(new DefaultProfile(control_interface_,
                                           metrics_,
                                           this,
                                           storage_path_,
                                           ident.identifier,
                                           props_));
    if (!default_profile->InitStorage(Profile::kOpenExisting, nullptr)) {
      LOG(ERROR) << "Failed to open default profile.";
      // Try to continue anyway, so that we can be useful in cases
      // where the disk is full.
      default_profile->InitStubStorage();
    }

    LoadProperties(default_profile);
    profile = default_profile;
  } else {
    profile = new Profile(control_interface_,
                          metrics_,
                          this,
                          ident,
                          user_storage_path_,
                          connect_profiles_to_rpc_);
    if (!profile->InitStorage(Profile::kOpenExisting, error)) {
      // |error| will have been populated by InitStorage().
      return;
    }
  }

  profiles_.push_back(profile);

  for (ServiceRefPtr& service : services_) {
    service->ClearExplicitlyDisconnected();

    // Offer each registered Service the opportunity to join this new Profile.
    if (profile->ConfigureService(service)) {
      LOG(INFO) << "(Re-)configured service " << service->unique_name()
                << " from new profile.";
    }
  }

  // Shop the Profile contents around to Devices which may have configuration
  // stored in these profiles.
  for (DeviceRefPtr& device : devices_) {
    profile->ConfigureDevice(device);
  }

  // Offer the Profile contents to the service providers which will
  // create new services if necessary.
  for (const auto& provider_mapping : providers_) {
    provider_mapping.second->CreateServicesFromProfile(profile);
  }

  *path = profile->GetRpcIdentifier();
  SortServices();
  OnProfilesChanged();
  LOG(INFO) << __func__ << " finished; " << profiles_.size()
            << " profile(s) now present.";
}

void Manager::PushProfile(const string& name, string* path, Error* error) {
  SLOG(this, 2) << __func__ << " " << name;
  Profile::Identifier ident;
  if (!Profile::ParseIdentifier(name, &ident)) {
    Error::PopulateAndLog(FROM_HERE, error, Error::kInvalidArguments,
                          "Invalid profile name " + name);
    return;
  }
  PushProfileInternal(ident, path, error);
}

void Manager::InsertUserProfile(const string& name,
                                const string& user_hash,
                                string* path,
                                Error* error) {
  SLOG(this, 2) << __func__ << " " << name;
  Profile::Identifier ident;
  if (!Profile::ParseIdentifier(name, &ident) ||
      ident.user.empty()) {
    Error::PopulateAndLog(FROM_HERE, error, Error::kInvalidArguments,
                          "Invalid user profile name " + name);
    return;
  }
  ident.user_hash = user_hash;
  PushProfileInternal(ident, path, error);
}

void Manager::PopProfileInternal() {
  CHECK(!profiles_.empty());
  ProfileRefPtr active_profile = profiles_.back();
  profiles_.pop_back();
  for (auto it = services_.begin(); it != services_.end();) {
    (*it)->ClearExplicitlyDisconnected();
    if (IsServiceEphemeral(*it)) {
      // Not affected, since the EphemeralProfile isn't on the stack.
      // Not logged, since ephemeral services aren't that interesting.
      ++it;
      continue;
    }

    if ((*it)->profile().get() != active_profile.get()) {
      LOG(INFO) << "Skipping unload of service " << (*it)->unique_name()
                << ": wasn't using this profile.";
      ++it;
      continue;
    }

    if (MatchProfileWithService(*it)) {
      LOG(INFO) << "Skipping unload of service " << (*it)->unique_name()
                << ": re-configured from another profile.";
      ++it;
      continue;
    }

    if (!UnloadService(&it)) {
      LOG(INFO) << "Service " << (*it)->unique_name()
                << " not completely unloaded.";
      ++it;
      continue;
    }

    // Service was totally unloaded. No advance of iterator in this
    // case, as UnloadService has updated the iterator for us.
  }
  SortServices();
  OnProfilesChanged();
  LOG(INFO) << __func__ << " finished; " << profiles_.size()
            << " profile(s) still present.";
}

void Manager::OnProfilesChanged() {
  Error unused_error;

  adaptor_->EmitStringsChanged(kProfilesProperty,
                               EnumerateProfiles(&unused_error));
  Profile::SaveUserProfileList(user_profile_list_path_, profiles_);
}

void Manager::PopProfile(const string& name, Error* error) {
  SLOG(this, 2) << __func__ << " " << name;
  Profile::Identifier ident;
  if (profiles_.empty()) {
    Error::PopulateAndLog(
        FROM_HERE, error, Error::kNotFound, "Profile stack is empty");
    return;
  }
  ProfileRefPtr active_profile = profiles_.back();
  if (!Profile::ParseIdentifier(name, &ident)) {
    Error::PopulateAndLog(FROM_HERE, error, Error::kInvalidArguments,
                          "Invalid profile name " + name);
    return;
  }
  if (!active_profile->MatchesIdentifier(ident)) {
    Error::PopulateAndLog(FROM_HERE, error, Error::kNotSupported,
                          name + " is not the active profile");
    return;
  }
  PopProfileInternal();
}

void Manager::PopAnyProfile(Error* error) {
  SLOG(this, 2) << __func__;
  Profile::Identifier ident;
  if (profiles_.empty()) {
    Error::PopulateAndLog(
        FROM_HERE, error, Error::kNotFound, "Profile stack is empty");
    return;
  }
  PopProfileInternal();
}

void Manager::PopAllUserProfiles(Error* /*error*/) {
  SLOG(this, 2) << __func__;
  while (!profiles_.empty() && !profiles_.back()->GetUser().empty()) {
    PopProfileInternal();
  }
}

void Manager::RemoveProfile(const string& name, Error* error) {
  Profile::Identifier ident;
  if (!Profile::ParseIdentifier(name, &ident)) {
    Error::PopulateAndLog(FROM_HERE, error, Error::kInvalidArguments,
                          "Invalid profile name " + name);
    return;
  }

  if (HasProfile(ident)) {
    Error::PopulateAndLog(FROM_HERE, error, Error::kInvalidArguments,
                          "Cannot remove profile name " + name +
                          " since it is on stack");
    return;
  }

  ProfileRefPtr profile;
  if (ident.user.empty()) {
    profile = new DefaultProfile(control_interface_,
                                 metrics_,
                                 this,
                                 storage_path_,
                                 ident.identifier,
                                 props_);
  } else {
    profile = new Profile(control_interface_,
                          metrics_,
                          this,
                          ident,
                          user_storage_path_,
                          false);
  }


  // |error| will have been populated if RemoveStorage fails.
  profile->RemoveStorage(error);

  return;
}

bool Manager::DeviceManagementAllowed(const string& device_name) {
  if (std::find(blacklisted_devices_.begin(),
                blacklisted_devices_.end(),
                device_name) != blacklisted_devices_.end()) {
    return false;
  }
  if (!whitelisted_devices_.size()) {
    // If no whitelist is specified, all devices are considered whitelisted.
    return true;
  }
  if (std::find(whitelisted_devices_.begin(),
                whitelisted_devices_.end(),
                device_name) != whitelisted_devices_.end()) {
    return true;
  }
  return false;
}

void Manager::ClaimDevice(const string& claimer_name,
                          const string& device_name,
                          Error* error) {
  SLOG(this, 2) << __func__;

  // Basic check for device name.
  if (device_name.empty()) {
    Error::PopulateAndLog(FROM_HERE, error, Error::kInvalidArguments,
                          "Empty device name");
    return;
  }

  if (!DeviceManagementAllowed(device_name)) {
    Error::PopulateAndLog(FROM_HERE, error, Error::kInvalidArguments,
                          "Not allowed to claim unmanaged device");
    return;
  }

  // Verify default claimer.
  if (claimer_name.empty() &&
      (!device_claimer_ || !device_claimer_->default_claimer())) {
    Error::PopulateAndLog(FROM_HERE, error, Error::kInvalidArguments,
                          "No default claimer");
    return;
  }

  // Create a new device claimer if one doesn't exist yet.
  if (!device_claimer_) {
    // Start a device claimer.  No need to verify the existence of the claimer,
    // since we are using message sender as the claimer name.
    device_claimer_.reset(
        new DeviceClaimer(claimer_name, &device_info_, false));
  }

  // Verify claimer's name, since we only allow one claimer to exist at a time.
  if (device_claimer_->name() != claimer_name) {
    Error::PopulateAndLog(FROM_HERE, error, Error::kInvalidArguments,
                          "Invalid claimer name " + claimer_name +
                          ". Claimer " + device_claimer_->name() +
                          " already exist");
    return;
  }

  // Error will be populated by the claimer if failed to claim the device.
  if (!device_claimer_->Claim(device_name, error)) {
    return;
  }

  // Deregister the device from manager if it is registered.
  DeregisterDeviceByLinkName(device_name);
}

void Manager::ReleaseDevice(const string& claimer_name,
                            const string& device_name,
                            bool* claimer_removed,
                            Error* error) {
  SLOG(this, 2) << __func__;

  *claimer_removed = false;

  if (!DeviceManagementAllowed(device_name)) {
    Error::PopulateAndLog(FROM_HERE, error, Error::kInvalidArguments,
                          "Not allowed to release unmanaged device");
    return;
  }

  if (!device_claimer_) {
    Error::PopulateAndLog(FROM_HERE, error, Error::kInvalidArguments,
                          "Device claimer doesn't exist");
    return;
  }

  // Verify claimer's name, since we only allow one claimer to exist at a time.
  if (device_claimer_->name() != claimer_name) {
    Error::PopulateAndLog(FROM_HERE, error, Error::kInvalidArguments,
                          "Invalid claimer name " + claimer_name +
                          ". Claimer " + device_claimer_->name() +
                          " already exist");
    return;
  }

  // Release the device from the claimer. Error should be populated by the
  // claimer if it failed to release the given device.
  device_claimer_->Release(device_name, error);

  // Reset claimer if this is not the default claimer and no more devices are
  // claimed by this claimer.
  if (!device_claimer_->default_claimer() &&
      !device_claimer_->DevicesClaimed()) {
    device_claimer_.reset();
    *claimer_removed = true;
  }
}

#if !defined(DISABLE_WIFI) && defined(__BRILLO__)
bool Manager::SetupApModeInterface(string* out_interface_name, Error* error) {
  string interface_name = wifi_driver_hal_->SetupApModeInterface();
  if (interface_name.empty()) {
    Error::PopulateAndLog(FROM_HERE, error, Error::kOperationFailed,
                          "Failed to setup AP mode interface");
    return false;
  }
  *out_interface_name = interface_name;
  return true;
}

bool Manager::SetupStationModeInterface(string* out_interface_name,
                                        Error* error) {
  string interface_name = wifi_driver_hal_->SetupStationModeInterface();
  if (interface_name.empty()) {
    Error::PopulateAndLog(FROM_HERE, error, Error::kOperationFailed,
                          "Failed to setup station mode interface");
    return false;
  }
  *out_interface_name = interface_name;
  return true;
}

void Manager::OnApModeSetterVanished() {
  // Restore station mode interface.
  string interface_name = wifi_driver_hal_->SetupStationModeInterface();
  if (interface_name.empty()) {
    LOG(ERROR) << "Failed to restore station mode interface";
  }
}
#endif  // !DISABLE_WIFI && __BRILLO__

void Manager::RemoveService(const ServiceRefPtr& service) {
  LOG(INFO) << __func__ << " for service " << service->unique_name();
  if (!IsServiceEphemeral(service)) {
    service->profile()->AbandonService(service);
    if (MatchProfileWithService(service)) {
      // We found another profile to adopt the service; no need to unload.
      UpdateService(service);
      return;
    }
  }
  auto service_it = std::find(services_.begin(), services_.end(), service);
  CHECK(service_it != services_.end());
  if (!UnloadService(&service_it)) {
    UpdateService(service);
  }
  SortServices();
}

bool Manager::HandleProfileEntryDeletion(const ProfileRefPtr& profile,
                                         const std::string& entry_name) {
  bool moved_services = false;
  for (auto it = services_.begin(); it != services_.end();) {
    if ((*it)->profile().get() == profile.get() &&
        (*it)->GetStorageIdentifier() == entry_name) {
      profile->AbandonService(*it);
      if (MatchProfileWithService(*it) ||
          !UnloadService(&it)) {
        ++it;
      }
      moved_services = true;
    } else {
      ++it;
    }
  }
  if (moved_services) {
    SortServices();
  }
  return moved_services;
}

map<string, string> Manager::GetLoadableProfileEntriesForService(
    const ServiceConstRefPtr& service) {
  map<string, string> profile_entries;
  for (const auto& profile : profiles_) {
    string entry_name = service->GetLoadableStorageIdentifier(
        *profile->GetConstStorage());
    if (!entry_name.empty()) {
      profile_entries[profile->GetRpcIdentifier()] = entry_name;
    }
  }
  return profile_entries;
}

ServiceRefPtr Manager::GetServiceWithStorageIdentifier(
    const ProfileRefPtr& profile, const std::string& entry_name, Error* error) {
  for (const auto& service : services_) {
    if (service->profile().get() == profile.get() &&
        service->GetStorageIdentifier() == entry_name) {
      return service;
    }
  }

  SLOG(this, 2) << "Entry " << entry_name
                << " is not registered in the manager";
  return nullptr;
}

ServiceRefPtr Manager::CreateTemporaryServiceFromProfile(
    const ProfileRefPtr& profile, const std::string& entry_name, Error* error) {
  Technology::Identifier technology =
      Technology::IdentifierFromStorageGroup(entry_name);
  if (technology == Technology::kUnknown) {
    Error::PopulateAndLog(
        FROM_HERE, error, Error::kInternalError,
        "Could not determine technology for entry: " + entry_name);
    return nullptr;
  }

  ServiceRefPtr service = nullptr;
  // Since there is no provider for Ethernet services (Ethernet services are
  // created/provided by the Ethernet device), we will explicitly create
  // temporary Ethernet services for loading Ethernet entries.
  if (technology == Technology::kEthernet) {
    service = new EthernetTemporaryService(control_interface_,
                                           dispatcher_,
                                           metrics_,
                                           this,
                                           entry_name);
  } else if (ContainsKey(providers_, technology)) {
    service =
        providers_[technology]->CreateTemporaryServiceFromProfile(
            profile, entry_name, error);
  }

  if (!service) {
    Error::PopulateAndLog(FROM_HERE, error, Error::kNotSupported,
                          kErrorUnsupportedServiceType);
    return nullptr;
  }

  profile->LoadService(service);
  return service;
}

ServiceRefPtr Manager::GetServiceWithGUID(
    const std::string& guid, Error* error) {
  for (const auto& service : services_) {
    if (service->guid() == guid) {
      return service;
    }
  }

  string error_string(
      StringPrintf("Service wth GUID %s is not registered in the manager",
                   guid.c_str()));
  if (error) {
    error->Populate(Error::kNotFound, error_string);
  }
  SLOG(this, 2) << error_string;
  return nullptr;
}

ServiceRefPtr Manager::GetDefaultService() const {
  SLOG(this, 2) << __func__;
  if (services_.empty() || !services_[0]->connection().get()) {
    SLOG(this, 2) << "In " << __func__ << ": No default connection exists.";
    return nullptr;
  }
  return services_[0];
}

RpcIdentifier Manager::GetDefaultServiceRpcIdentifier(Error* /*error*/) {
  ServiceRefPtr default_service = GetDefaultService();
  return default_service ? default_service->GetRpcIdentifier() :
      control_interface_->NullRPCIdentifier();
}

bool Manager::IsTechnologyInList(const string& technology_list,
                                 Technology::Identifier tech) const {
  if (technology_list.empty())
    return false;

  Error error;
  vector<Technology::Identifier> technologies;
  return Technology::GetTechnologyVectorFromString(technology_list,
                                                   &technologies,
                                                   &error) &&
      std::find(technologies.begin(), technologies.end(), tech) !=
          technologies.end();
}

bool Manager::IsPortalDetectionEnabled(Technology::Identifier tech) {
  return IsTechnologyInList(GetCheckPortalList(nullptr), tech);
}

void Manager::SetStartupPortalList(const string& portal_list) {
  startup_portal_list_ = portal_list;
  use_startup_portal_list_ = true;
}

bool Manager::IsProfileBefore(const ProfileRefPtr& a,
                              const ProfileRefPtr& b) const {
  DCHECK(a != b);
  for (const auto& profile : profiles_) {
    if (profile == a) {
      return true;
    }
    if (profile == b) {
      return false;
    }
  }
  NOTREACHED() << "We should have found both profiles in the profiles_ list!";
  return false;
}

bool Manager::IsServiceEphemeral(const ServiceConstRefPtr& service) const {
  return service->profile() == ephemeral_profile_;
}

bool Manager::IsTechnologyLinkMonitorEnabled(
    Technology::Identifier technology) const {
  return IsTechnologyInList(props_.link_monitor_technologies, technology);
}

bool Manager::IsTechnologyAutoConnectDisabled(
    Technology::Identifier technology) const {
  return IsTechnologyInList(props_.no_auto_connect_technologies, technology);
}

bool Manager::IsTechnologyProhibited(
    Technology::Identifier technology) const {
  return IsTechnologyInList(props_.prohibited_technologies, technology);
}

void Manager::OnProfileStorageInitialized(Profile* profile) {
#if !defined(DISABLE_WIFI)
  wifi_provider_->LoadAndFixupServiceEntries(profile);
#endif  // DISABLE_WIFI
}

DeviceRefPtr Manager::GetEnabledDeviceWithTechnology(
    Technology::Identifier technology) const {
  for (const auto& device : FilterByTechnology(technology)) {
    if (device->enabled()) {
      return device;
    }
  }
  return nullptr;
}

DeviceRefPtr Manager::GetEnabledDeviceByLinkName(
    const string& link_name) const {
  for (const auto& device : devices_) {
    if (device->link_name() == link_name) {
      if (!device->enabled()) {
        return nullptr;
      }
      return device;
    }
  }
  return nullptr;
}

const ProfileRefPtr& Manager::ActiveProfile() const {
  DCHECK_NE(profiles_.size(), 0U);
  return profiles_.back();
}

bool Manager::IsActiveProfile(const ProfileRefPtr& profile) const {
  return (profiles_.size() > 0 &&
          ActiveProfile().get() == profile.get());
}

bool Manager::MoveServiceToProfile(const ServiceRefPtr& to_move,
                                   const ProfileRefPtr& destination) {
  const ProfileRefPtr from = to_move->profile();
  SLOG(this, 2) << "Moving service "
                << to_move->unique_name()
                << " to profile "
                << destination->GetFriendlyName()
                << " from "
                << from->GetFriendlyName();
  return destination->AdoptService(to_move) && from->AbandonService(to_move);
}

ProfileRefPtr Manager::LookupProfileByRpcIdentifier(
    const string& profile_rpcid) {
  for (const auto& profile : profiles_) {
    if (profile_rpcid == profile->GetRpcIdentifier()) {
      return profile;
    }
  }
  return nullptr;
}

void Manager::SetProfileForService(const ServiceRefPtr& to_set,
                                   const string& profile_rpcid,
                                   Error* error) {
  ProfileRefPtr profile = LookupProfileByRpcIdentifier(profile_rpcid);
  if (!profile) {
    Error::PopulateAndLog(FROM_HERE, error, Error::kInvalidArguments,
                          StringPrintf("Unknown Profile %s requested for "
                                       "Service", profile_rpcid.c_str()));
    return;
  }

  if (!to_set->profile()) {
    // We are being asked to set the profile property of a service that
    // has never been registered.  Now is a good time to register it.
    RegisterService(to_set);
  }

  if (to_set->profile().get() == profile.get()) {
    Error::PopulateAndLog(FROM_HERE, error, Error::kInvalidArguments,
                          "Service is already connected to this profile");
  } else if (!MoveServiceToProfile(to_set, profile)) {
    Error::PopulateAndLog(FROM_HERE, error, Error::kInternalError,
                          "Unable to move service to profile");
  }
}

void Manager::SetEnabledStateForTechnology(const std::string& technology_name,
                                           bool enabled_state,
                                           bool persist,
                                           Error* error,
                                           const ResultCallback& callback) {
  CHECK(error);
  DCHECK(error->IsOngoing());
  Technology::Identifier id = Technology::IdentifierFromName(technology_name);
  if (id == Technology::kUnknown) {
    error->Populate(Error::kInvalidArguments, "Unknown technology");
    return;
  }
  if (enabled_state && IsTechnologyProhibited(id)) {
    error->Populate(Error::kPermissionDenied,
                    "The " + technology_name + " technology is prohibited");
    return;
  }
  bool deferred = false;
  auto result_aggregator(make_scoped_refptr(new ResultAggregator(callback)));
  for (auto& device : devices_) {
    if (device->technology() != id)
      continue;

    Error device_error(Error::kOperationInitiated);
    ResultCallback aggregator_callback(
        Bind(&ResultAggregator::ReportResult, result_aggregator));
    if (persist) {
      device->SetEnabledPersistent(
          enabled_state, &device_error, aggregator_callback);
    } else {
      device->SetEnabledNonPersistent(
          enabled_state, &device_error, aggregator_callback);
    }
    if (device_error.IsOngoing()) {
      deferred = true;
    } else if (!error->IsFailure()) {  // Report first failure.
      error->CopyFrom(device_error);
    }
  }
  if (deferred) {
    // Some device is handling this change asynchronously. Clobber any error
    // from another device, so that we can indicate the operation is still in
    // progress.
    error->Populate(Error::kOperationInitiated);
  } else if (error->IsOngoing()) {
    // |error| IsOngoing at entry to this method, but no device
    // |deferred|. Reset |error|, to indicate we're done.
    error->Reset();
  }
}

void Manager::UpdateEnabledTechnologies() {
  Error error;
  adaptor_->EmitStringsChanged(kEnabledTechnologiesProperty,
                               EnabledTechnologies(&error));
}

void Manager::UpdateUninitializedTechnologies() {
  Error error;
  adaptor_->EmitStringsChanged(kUninitializedTechnologiesProperty,
                               UninitializedTechnologies(&error));
}

void Manager::SetPassiveMode() {
  CHECK(!device_claimer_);
  // Create a default device claimer to claim devices from  shill as they're
  // detected.  Devices will be managed by remote application, which will use
  // the default claimer to specify the devices for shill to manage.
  device_claimer_.reset(
      new DeviceClaimer(kDefaultClaimerName, &device_info_, true));
}

void Manager::SetIgnoreUnknownEthernet(bool ignore) {
  LOG(INFO) << __func__ << "(" << ignore << ")";
  ignore_unknown_ethernet_ = ignore;
}

void Manager::SetPrependDNSServers(const std::string& prepend_dns_servers) {
  props_.prepend_dns_servers = prepend_dns_servers;
}

void Manager::SetAcceptHostnameFrom(const string& hostname_from) {
  accept_hostname_from_ = hostname_from;
}

bool Manager::ShouldAcceptHostnameFrom(const string& device_name) const {
  return base::MatchPattern(device_name, accept_hostname_from_);
}

void Manager::SetDHCPv6EnabledDevices(const vector<string>& device_list) {
  dhcpv6_enabled_devices_ = device_list;
}

bool Manager::IsDHCPv6EnabledForDevice(const string& device_name) const {
  return std::find(dhcpv6_enabled_devices_.begin(),
                   dhcpv6_enabled_devices_.end(),
                   device_name) != dhcpv6_enabled_devices_.end();
}

vector<string> Manager::FilterPrependDNSServersByFamily(
    IPAddress::Family family) const {
  vector<string> dns_servers;
  vector<string> split_servers = base::SplitString(
      props_.prepend_dns_servers, ",", base::TRIM_WHITESPACE,
      base::SPLIT_WANT_ALL);
  for (const auto& server : split_servers) {
    const IPAddress address(server);
    if (address.family() == family) {
      dns_servers.push_back(server);
    }
  }
  return dns_servers;
}

bool Manager::IsSuspending() {
  if (power_manager_ && power_manager_->suspending()) {
    return true;
  }
  return false;
}

void Manager::RecordDarkResumeWakeReason(const string& wake_reason) {
  power_manager_->RecordDarkResumeWakeReason(wake_reason);
}

void Manager::RegisterDevice(const DeviceRefPtr& to_manage) {
  LOG(INFO) << "Device " << to_manage->FriendlyName() << " registered.";
  // Manager is running in passive mode when default claimer is created, which
  // means devices are being managed by remote application. Only manage the
  // device if it was explicitly released by remote application through
  // default claimer.
  if (device_claimer_ && device_claimer_->default_claimer()) {
    if (!device_claimer_->IsDeviceReleased(to_manage->link_name())) {
      Error error;
      device_claimer_->Claim(to_manage->link_name(), &error);
      return;
    }
  }

  for (const auto& device : devices_) {
    if (to_manage == device)
      return;
  }
  devices_.push_back(to_manage);

  LoadDeviceFromProfiles(to_manage);

  if (IsTechnologyProhibited(to_manage->technology())) {
    Error unused_error;
    to_manage->SetEnabledNonPersistent(false, &unused_error, ResultCallback());
  }

  // If |to_manage| is new, it needs to be persisted.
  UpdateDevice(to_manage);

  // In normal usage, running_ will always be true when we are here, however
  // unit tests sometimes do things in otherwise invalid states.
  if (running_ && (to_manage->enabled_persistent() ||
                   to_manage->IsUnderlyingDeviceEnabled()))
    to_manage->SetEnabled(true);

  EmitDeviceProperties();
}

void Manager::DeregisterDevice(const DeviceRefPtr& to_forget) {
  SLOG(this, 2) << __func__ << "(" << to_forget->FriendlyName() << ")";
  for (auto it = devices_.begin(); it != devices_.end(); ++it) {
    if (to_forget.get() == it->get()) {
      SLOG(this, 2) << "Deregistered device: " << to_forget->UniqueName();
      UpdateDevice(to_forget);
      to_forget->SetEnabled(false);
      devices_.erase(it);
      EmitDeviceProperties();
      return;
    }
  }
  SLOG(this, 2) << __func__ << " unknown device: "
                << to_forget->UniqueName();
}

void Manager::DeregisterDeviceByLinkName(const string& link_name) {
  for (const auto& device : devices_) {
    if (device->link_name() == link_name) {
      DeregisterDevice(device);
      break;
    }
  }
}

vector<string> Manager::ClaimedDevices(Error* error) {
  vector<string> results;
  if (!device_claimer_) {
    return results;
  }

  const auto& devices = device_claimer_->claimed_device_names();
  results.resize(devices.size());
  std::copy(devices.begin(), devices.end(), results.begin());
  return results;
}

void Manager::LoadDeviceFromProfiles(const DeviceRefPtr& device) {
  // We are applying device properties from the DefaultProfile, and adding the
  // union of hidden services in all loaded profiles to the device.
  for (const auto& profile : profiles_) {
    // Load device configuration, if any exists, as well as hidden services.
    profile->ConfigureDevice(device);
  }
}

void Manager::EmitDeviceProperties() {
  Error error;
  vector<string> device_paths = EnumerateDevices(&error);
  adaptor_->EmitRpcIdentifierArrayChanged(kDevicesProperty,
                                          device_paths);
  adaptor_->EmitStringsChanged(kAvailableTechnologiesProperty,
                               AvailableTechnologies(&error));
  adaptor_->EmitStringsChanged(kEnabledTechnologiesProperty,
                               EnabledTechnologies(&error));
  adaptor_->EmitStringsChanged(kUninitializedTechnologiesProperty,
                               UninitializedTechnologies(&error));
}

void Manager::OnInnerDevicesChanged() {
  EmitDeviceProperties();
}

void Manager::OnDeviceClaimerVanished() {
  // Reset device claimer.
  device_claimer_.reset();
}

#if !defined(DISABLE_WIFI)
bool Manager::SetDisableWiFiVHT(const bool& disable_wifi_vht, Error* error) {
  if (disable_wifi_vht == wifi_provider_->disable_vht()) {
    return false;
  }
  wifi_provider_->set_disable_vht(disable_wifi_vht);
  return true;
}

bool Manager::GetDisableWiFiVHT(Error* error) {
  return wifi_provider_->disable_vht();
}
#endif  // DISABLE_WIFI

bool Manager::SetProhibitedTechnologies(const string& prohibited_technologies,
                                        Error* error) {
  vector<Technology::Identifier> technology_vector;
  if (!Technology::GetTechnologyVectorFromString(prohibited_technologies,
                                                 &technology_vector,
                                                 error)) {
    return false;
  }
  for (const auto& technology : technology_vector) {
    Error unused_error(Error::kOperationInitiated);
    ResultCallback result_callback(Bind(
        &Manager::OnTechnologyProhibited, Unretained(this), technology));
    const bool kPersistentSave = false;
    SetEnabledStateForTechnology(Technology::NameFromIdentifier(technology),
                                 false,
                                 kPersistentSave,
                                 &unused_error,
                                 result_callback);
  }
  props_.prohibited_technologies = prohibited_technologies;

  return true;
}

void Manager::OnTechnologyProhibited(Technology::Identifier technology,
                                     const Error& error) {
  SLOG(this, 2) << __func__ << " for "
                << Technology::NameFromIdentifier(technology);
}

string Manager::GetProhibitedTechnologies(Error* error) {
  return props_.prohibited_technologies;
}

bool Manager::HasService(const ServiceRefPtr& service) {
  for (const auto& manager_service : services_) {
    if (manager_service->unique_name() == service->unique_name())
      return true;
  }
  return false;
}

void Manager::RegisterService(const ServiceRefPtr& to_manage) {
  SLOG(this, 2) << "Registering service " << to_manage->unique_name();

  MatchProfileWithService(to_manage);

  // Now add to OUR list.
  for (const auto& service : services_) {
    CHECK(to_manage->unique_name() != service->unique_name());
  }
  services_.push_back(to_manage);
  SortServices();
}

void Manager::DeregisterService(const ServiceRefPtr& to_forget) {
  for (auto it = services_.begin(); it != services_.end(); ++it) {
    if (to_forget->unique_name() == (*it)->unique_name()) {
      DLOG_IF(FATAL, (*it)->connection())
          << "Service " << (*it)->unique_name()
          << " still has a connection (in call to " << __func__ << ")";
      (*it)->Unload();
      (*it)->SetProfile(nullptr);
      services_.erase(it);
      SortServices();
      return;
    }
  }
}

bool Manager::UnloadService(vector<ServiceRefPtr>::iterator* service_iterator) {
  if (!(**service_iterator)->Unload()) {
    return false;
  }

  DCHECK(!(**service_iterator)->connection());
  (**service_iterator)->SetProfile(nullptr);
  *service_iterator = services_.erase(*service_iterator);

  return true;
}

void Manager::UpdateService(const ServiceRefPtr& to_update) {
  CHECK(to_update);
  bool is_interesting_state_change = false;
  const auto& state_it = watched_service_states_.find(to_update->unique_name());
  if (state_it != watched_service_states_.end()) {
    is_interesting_state_change = (to_update->state() != state_it->second);
  } else {
    is_interesting_state_change = to_update->IsActive(nullptr);
  }

  string log_message = StringPrintf(
      "Service %s updated; state: %s failure %s",
      to_update->unique_name().c_str(),
      Service::ConnectStateToString(to_update->state()),
      Service::ConnectFailureToString(to_update->failure()));
  if (is_interesting_state_change) {
    LOG(INFO) << log_message;
  } else {
    SLOG(this, 2) << log_message;
  }
  SLOG(this, 2) << "IsConnected(): " << to_update->IsConnected();
  SLOG(this, 2) << "IsConnecting(): " << to_update->IsConnecting();
  if (to_update->IsConnected()) {
    to_update->EnableAndRetainAutoConnect();
    // Persists the updated auto_connect setting in the profile.
    SaveServiceToProfile(to_update);
  }
  SortServices();
}

void Manager::UpdateDevice(const DeviceRefPtr& to_update) {
  LOG(INFO) << "Device " << to_update->link_name() << " updated: "
            << (to_update->enabled_persistent() ? "enabled" : "disabled");
  // Saves the device to the topmost profile that accepts it (ordinary
  // profiles don't update but default profiles do). Normally, the topmost
  // updating profile would be the DefaultProfile at the bottom of the stack.
  // Autotests, differ from the normal scenario, however, in that they push a
  // second test-only DefaultProfile.
  for (auto rit = profiles_.rbegin(); rit != profiles_.rend(); ++rit) {
    if ((*rit)->UpdateDevice(to_update)) {
      return;
    }
  }
}

#if !defined(DISABLE_WIFI)
void Manager::UpdateWiFiProvider() {
  // Saves |wifi_provider_| to the topmost profile that accepts it (ordinary
  // profiles don't update but default profiles do). Normally, the topmost
  // updating profile would be the DefaultProfile at the bottom of the stack.
  // Autotests, differ from the normal scenario, however, in that they push a
  // second test-only DefaultProfile.
  for (auto rit = profiles_.rbegin(); rit != profiles_.rend(); ++rit) {
    if ((*rit)->UpdateWiFiProvider(*wifi_provider_)) {
      return;
    }
  }
}
#endif  // DISABLE_WIFI

void Manager::SaveServiceToProfile(const ServiceRefPtr& to_update) {
  if (IsServiceEphemeral(to_update)) {
    if (profiles_.empty()) {
      LOG(ERROR) << "Cannot assign profile to service: no profiles exist!";
    } else {
      MoveServiceToProfile(to_update, profiles_.back());
    }
  } else {
    to_update->profile()->UpdateService(to_update);
  }
}

void Manager::LoadProperties(const scoped_refptr<DefaultProfile>& profile) {
  profile->LoadManagerProperties(&props_, dhcp_properties_.get());
  SetIgnoredDNSSearchPaths(props_.ignored_dns_search_paths, nullptr);
}

void Manager::AddTerminationAction(const string& name,
                                   const base::Closure& start) {
  termination_actions_.Add(name, start);
}

void Manager::TerminationActionComplete(const string& name) {
  SLOG(this, 2) << __func__;
  termination_actions_.ActionComplete(name);
}

void Manager::RemoveTerminationAction(const string& name) {
  SLOG(this, 2) << __func__;
  termination_actions_.Remove(name);
}

void Manager::RunTerminationActions(const ResultCallback& done_callback) {
  LOG(INFO) << "Running termination actions.";
  termination_actions_.Run(kTerminationActionsTimeoutMilliseconds,
                           done_callback);
}

bool Manager::RunTerminationActionsAndNotifyMetrics(
    const ResultCallback& done_callback) {
  if (termination_actions_.IsEmpty())
    return false;

  metrics_->NotifyTerminationActionsStarted();
  RunTerminationActions(done_callback);
  return true;
}

int Manager::RegisterDefaultServiceCallback(const ServiceCallback& callback) {
  default_service_callbacks_[++default_service_callback_tag_] = callback;
  return default_service_callback_tag_;
}

void Manager::DeregisterDefaultServiceCallback(int tag) {
  default_service_callbacks_.erase(tag);
}

#if !defined(DISABLE_WIFI)
void Manager::VerifyDestination(const string& certificate,
                                const string& public_key,
                                const string& nonce,
                                const string& signed_data,
                                const string& destination_udn,
                                const string& hotspot_ssid,
                                const string& hotspot_bssid,
                                const ResultBoolCallback& cb,
                                Error* error) {
  if (hotspot_bssid.length() > 32) {
    error->Populate(Error::kOperationFailed,
                    "Invalid SSID given for verification.");
    return;
  }
  vector<uint8_t> ssid;
  string bssid;
  if (hotspot_ssid.length() || hotspot_bssid.length()) {
    // If Chrome thinks this destination is already configured, service
    // will be an AP that both we and the destination are connected
    // to, and not the thing we should verify against.
    ssid.assign(hotspot_ssid.begin(), hotspot_ssid.end());
    bssid = hotspot_bssid;
  } else {
    // For now, we only support a single connected WiFi service.  If we change
    // that, we'll need to revisit this.
    bool found_one = false;
    for (const auto& service : services_) {
      if (service->technology() == Technology::kWifi &&
          service->IsConnected()) {
        WiFiService* wifi = reinterpret_cast<WiFiService*>(&(*service));
        bssid = wifi->bssid();
        ssid = wifi->ssid();
        found_one = true;
        break;
      }
    }
    if (!found_one) {
      error->Populate(Error::kOperationFailed,
                      "Unable to find connected WiFi service.");
      return;
    }
  }
  crypto_util_proxy_->VerifyDestination(certificate, public_key, nonce,
                                        signed_data, destination_udn,
                                        ssid, bssid, cb, error);
}

void Manager::VerifyToEncryptLink(string public_key,
                                  string data,
                                  ResultStringCallback cb,
                                  const Error& error,
                                  bool success) {
  if (!success || !error.IsSuccess()) {
    CHECK(error.IsFailure()) << "Return code from CryptoUtilProxy "
                             << "inconsistent with error code.";
    cb.Run(error, "");
    return;
  }
  Error encrypt_error;
  if (!crypto_util_proxy_->EncryptData(public_key, data, cb, &encrypt_error)) {
    CHECK(encrypt_error.IsFailure()) << "CryptoUtilProxy::EncryptData returned "
                                     << "inconsistently.";
    cb.Run(encrypt_error, "");
  }
}

void Manager::VerifyAndEncryptData(const string& certificate,
                                   const string& public_key,
                                   const string& nonce,
                                   const string& signed_data,
                                   const string& destination_udn,
                                   const string& hotspot_ssid,
                                   const string& hotspot_bssid,
                                   const string& data,
                                   const ResultStringCallback& cb,
                                   Error* error) {
  ResultBoolCallback on_verification_success = Bind(
      &Manager::VerifyToEncryptLink, AsWeakPtr(), public_key, data, cb);
  VerifyDestination(certificate, public_key, nonce, signed_data,
                    destination_udn, hotspot_ssid, hotspot_bssid,
                    on_verification_success, error);
}

void Manager::VerifyAndEncryptCredentials(const string& certificate,
                                          const string& public_key,
                                          const string& nonce,
                                          const string& signed_data,
                                          const string& destination_udn,
                                          const string& hotspot_ssid,
                                          const string& hotspot_bssid,
                                          const string& network_path,
                                          const ResultStringCallback& cb,
                                          Error* error) {
  // This is intentionally left unimplemented until we have a security review.
  error->Populate(Error::kNotImplemented, "Not implemented");
}
#endif  // DISABLE_WIFI

int Manager::CalcConnectionId(std::string gateway_ip,
                              std::string gateway_mac) {
  return static_cast<int>(std::hash<std::string>()(gateway_ip + gateway_mac +
      std::to_string(props_.connection_id_salt)));
}

void Manager::ReportServicesOnSameNetwork(int connection_id) {
  int num_services = 0;
  for (const auto& service : services_) {
    if (service->connection_id() == connection_id) {
      num_services++;
    }
  }
  metrics_->NotifyServicesOnSameNetwork(num_services);
}

void Manager::NotifyDefaultServiceChanged(const ServiceRefPtr& service) {
  for (const auto& callback : default_service_callbacks_) {
    callback.second.Run(service);
  }
  metrics_->NotifyDefaultServiceChanged(service.get());
  EmitDefaultService();
}

void Manager::EmitDefaultService() {
  RpcIdentifier rpc_identifier = GetDefaultServiceRpcIdentifier(nullptr);
  if (rpc_identifier != default_service_rpc_identifier_) {
    adaptor_->EmitRpcIdentifierChanged(kDefaultServiceProperty, rpc_identifier);
    default_service_rpc_identifier_ = rpc_identifier;
  }
}

void Manager::OnSuspendImminent() {
  metrics_->NotifySuspendActionsStarted();
  if (devices_.empty()) {
    // If there are no devices, then suspend actions succeeded synchronously.
    // Make a call to the Manager::OnSuspendActionsComplete directly, since
    // result_aggregator will not.
    OnSuspendActionsComplete(Error(Error::kSuccess));
    return;
  }
  auto result_aggregator(make_scoped_refptr(new ResultAggregator(
      Bind(&Manager::OnSuspendActionsComplete, AsWeakPtr()), dispatcher_,
      kTerminationActionsTimeoutMilliseconds)));
  for (const auto& device : devices_) {
    ResultCallback aggregator_callback(
        Bind(&ResultAggregator::ReportResult, result_aggregator));
    device->OnBeforeSuspend(aggregator_callback);
  }
}

void Manager::OnSuspendDone() {
  metrics_->NotifySuspendDone();
  // Un-suppress auto-connect in case this flag was left set in dark resume.
  set_suppress_autoconnect(false);
  for (const auto& service : services_) {
    service->OnAfterResume();
  }
  SortServices();
  for (const auto& device : devices_) {
    device->OnAfterResume();
  }
}

void Manager::OnDarkSuspendImminent() {
  metrics_->NotifyDarkResumeActionsStarted();
  if (devices_.empty()) {
    // If there are no devices, then suspend actions succeeded synchronously.
    // Make a call to the Manager::OnDarkResumeActionsComplete directly, since
    // result_aggregator will not.
    OnDarkResumeActionsComplete(Error(Error::kSuccess));
    return;
  }
  auto result_aggregator(make_scoped_refptr(new ResultAggregator(
      Bind(&Manager::OnDarkResumeActionsComplete, AsWeakPtr()), dispatcher_,
      kTerminationActionsTimeoutMilliseconds)));
  for (const auto& device : devices_) {
    ResultCallback aggregator_callback(
        Bind(&ResultAggregator::ReportResult, result_aggregator));
    device->OnDarkResume(aggregator_callback);
  }
}

void Manager::OnSuspendActionsComplete(const Error& error) {
  LOG(INFO) << "Finished suspend actions. Result: " << error;
  metrics_->NotifySuspendActionsCompleted(error.IsSuccess());
  power_manager_->ReportSuspendReadiness();
}

void Manager::OnDarkResumeActionsComplete(const Error& error) {
  LOG(INFO) << "Finished dark resume actions. Result: " << error;
  metrics_->NotifyDarkResumeActionsCompleted(error.IsSuccess());
  power_manager_->ReportDarkSuspendReadiness();
}


vector<DeviceRefPtr>
Manager::FilterByTechnology(Technology::Identifier tech) const {
  vector<DeviceRefPtr> found;
  for (const auto& device : devices_) {
    if (device->technology() == tech)
      found.push_back(device);
  }
  return found;
}

ServiceRefPtr Manager::FindService(const string& name) {
  for (const auto& service : services_) {
    if (name == service->unique_name())
      return service;
  }
  return nullptr;
}

void Manager::HelpRegisterConstDerivedRpcIdentifier(
    const string& name,
    RpcIdentifier(Manager::*get)(Error* error)) {
  store_.RegisterDerivedRpcIdentifier(
      name,
      RpcIdentifierAccessor(
          new CustomAccessor<Manager, RpcIdentifier>(this, get, nullptr)));
}

void Manager::HelpRegisterConstDerivedRpcIdentifiers(
    const string& name,
    RpcIdentifiers(Manager::*get)(Error* error)) {
  store_.RegisterDerivedRpcIdentifiers(
      name,
      RpcIdentifiersAccessor(
          new CustomAccessor<Manager, RpcIdentifiers>(this, get, nullptr)));
}

void Manager::HelpRegisterDerivedString(
    const string& name,
    string(Manager::*get)(Error* error),
    bool(Manager::*set)(const string&, Error*)) {
  store_.RegisterDerivedString(
      name,
      StringAccessor(new CustomAccessor<Manager, string>(this, get, set)));
}

void Manager::HelpRegisterConstDerivedStrings(
    const string& name,
    Strings(Manager::*get)(Error*)) {
  store_.RegisterDerivedStrings(
      name, StringsAccessor(
                new CustomAccessor<Manager, Strings>(this, get, nullptr)));
}

void Manager::HelpRegisterDerivedBool(
    const string& name,
    bool(Manager::*get)(Error* error),
    bool(Manager::*set)(const bool&, Error*)) {
  store_.RegisterDerivedBool(
      name,
      BoolAccessor(new CustomAccessor<Manager, bool>(this, get, set, nullptr)));
}

void Manager::SortServices() {
  // We might be called in the middle of a series of events that
  // may result in multiple calls to Manager::SortServices, or within
  // an outer loop that may also be traversing the services_ list.
  // Defer this work to the event loop.
  if (sort_services_task_.IsCancelled()) {
    sort_services_task_.Reset(Bind(&Manager::SortServicesTask, AsWeakPtr()));
    dispatcher_->PostTask(sort_services_task_.callback());
  }
}

void Manager::SortServicesTask() {
  SLOG(this, 4) << "In " << __func__;
  sort_services_task_.Cancel();
  ServiceRefPtr default_service;

  if (!services_.empty()) {
    // Keep track of the service that is the candidate for the default
    // service.  We have not yet tested to see if this service has a
    // connection.
    default_service = services_[0];
  }
  const bool kCompareConnectivityState = true;
  sort(services_.begin(), services_.end(),
       ServiceSorter(this, kCompareConnectivityState, technology_order_));

  if (!services_.empty()) {
    ConnectionRefPtr default_connection = default_service->connection();
    if (default_connection &&
        services_[0]->connection() != default_connection) {
      default_connection->SetIsDefault(false);
    }
    if (services_[0]->connection()) {
      services_[0]->connection()->SetIsDefault(true);
      if (default_service != services_[0]) {
        default_service = services_[0];
        LOG(INFO) << "Default service is now "
                  << default_service->unique_name();
      }
    } else {
      default_service = nullptr;
    }
  }

  Error error;
  adaptor_->EmitRpcIdentifierArrayChanged(kServiceCompleteListProperty,
                                          EnumerateCompleteServices(nullptr));
  adaptor_->EmitRpcIdentifierArrayChanged(kServicesProperty,
                                          EnumerateAvailableServices(nullptr));
  adaptor_->EmitRpcIdentifierArrayChanged(kServiceWatchListProperty,
                                          EnumerateWatchedServices(nullptr));
  adaptor_->EmitStringsChanged(kConnectedTechnologiesProperty,
                               ConnectedTechnologies(&error));
  adaptor_->EmitStringChanged(kDefaultTechnologyProperty,
                              DefaultTechnology(&error));
  NotifyDefaultServiceChanged(default_service);
  RefreshConnectionState();
  DetectMultiHomedDevices();

  AutoConnect();
}

void Manager::DeviceStatusCheckTask() {
  SLOG(this, 4) << "In " << __func__;

  ConnectionStatusCheck();
  DevicePresenceStatusCheck();

  dispatcher_->PostDelayedTask(device_status_check_task_.callback(),
                               kDeviceStatusCheckIntervalMilliseconds);
}

void Manager::ConnectionStatusCheck() {
  SLOG(this, 4) << "In " << __func__;
  // Report current connection status.
  Metrics::ConnectionStatus status = Metrics::kConnectionStatusOffline;
  if (IsConnected()) {
    status = Metrics::kConnectionStatusConnected;
    // Check if device is online as well.
    if (IsOnline()) {
      metrics_->NotifyDeviceConnectionStatus(Metrics::kConnectionStatusOnline);
    }
  }
  metrics_->NotifyDeviceConnectionStatus(status);
}

void Manager::DevicePresenceStatusCheck() {
  Error error;
  vector<string> available_technologies = AvailableTechnologies(&error);

  for (const auto& technology : kProbeTechnologies) {
    bool presence = std::find(available_technologies.begin(),
                              available_technologies.end(),
                              technology) != available_technologies.end();
    metrics_->NotifyDevicePresenceStatus(
        Technology::IdentifierFromName(technology), presence);
  }
}

bool Manager::MatchProfileWithService(const ServiceRefPtr& service) {
  vector<ProfileRefPtr>::reverse_iterator it;
  for (it = profiles_.rbegin(); it != profiles_.rend(); ++it) {
    if ((*it)->ConfigureService(service)) {
      break;
    }
  }
  if (it == profiles_.rend()) {
    ephemeral_profile_->AdoptService(service);
    return false;
  }
  return true;
}

void Manager::AutoConnect() {
  if (suppress_autoconnect_) {
    LOG(INFO) << "Auto-connect suppressed -- explicitly suppressed.";
    return;
  }
  if (!running_) {
    LOG(INFO) << "Auto-connect suppressed -- not running.";
    return;
  }
  if (power_manager_ && power_manager_->suspending() &&
      !power_manager_->in_dark_resume()) {
    LOG(INFO) << "Auto-connect suppressed -- system is suspending.";
    return;
  }
  if (services_.empty()) {
    LOG(INFO) << "Auto-connect suppressed -- no services.";
    return;
  }

  if (SLOG_IS_ON(Manager, 4)) {
    SLOG(this, 4) << "Sorted service list for AutoConnect: ";
    for (size_t i = 0; i < services_.size(); ++i) {
      ServiceRefPtr service = services_[i];
      const char* compare_reason = nullptr;
      if (i + 1 < services_.size()) {
        const bool kCompareConnectivityState = true;
        Service::Compare(
            this, service, services_[i+1], kCompareConnectivityState,
            technology_order_, &compare_reason);
      } else {
        compare_reason = "last";
      }
      SLOG(this, 4) << "Service " << service->unique_name()
                    << " Profile: " << service->profile()->GetFriendlyName()
                    << " IsConnected: " << service->IsConnected()
                    << " IsConnecting: " << service->IsConnecting()
                    << " HasEverConnected: " << service->has_ever_connected()
                    << " IsFailed: " << service->IsFailed()
                    << " connectable: " << service->connectable()
                    << " auto_connect: " << service->auto_connect()
                    << " retain_auto_connect: "
                    << service->retain_auto_connect()
                    << " priority: " << service->priority()
                    << " crypto_algorithm: " << service->crypto_algorithm()
                    << " key_rotation: " << service->key_rotation()
                    << " endpoint_auth: " << service->endpoint_auth()
                    << " strength: " << service->strength()
                    << " sorted: " << compare_reason;
    }
  }

#if !defined(DISABLE_WIFI)
  // Report the number of auto-connectable wifi services available when wifi is
  // idle (no active or pending connection), which will trigger auto connect
  // for wifi services.
  if (IsWifiIdle()) {
    wifi_provider_->ReportAutoConnectableServices();
  }
#endif  // DISABLE_WIFI

  // Perform auto-connect.
  for (const auto& service : services_) {
    if (service->auto_connect()) {
      service->AutoConnect();
    }
  }
}

void Manager::ConnectToBestServices(Error* /*error*/) {
  dispatcher_->PostTask(Bind(&Manager::ConnectToBestServicesTask, AsWeakPtr()));
}

void Manager::ConnectToBestServicesTask() {
  vector<ServiceRefPtr> services_copy = services_;
  const bool kCompareConnectivityState = false;
  sort(services_copy.begin(), services_copy.end(),
       ServiceSorter(this, kCompareConnectivityState, technology_order_));
  set<Technology::Identifier> connecting_technologies;
  for (const auto& service : services_copy) {
    if (!service->connectable()) {
      // Due to service sort order, it is guaranteed that no services beyond
      // this one will be connectable either.
      break;
    }
    if (!service->auto_connect() || !service->IsVisible()) {
      continue;
    }
    Technology::Identifier technology = service->technology();
    if (!Technology::IsPrimaryConnectivityTechnology(technology) &&
        !IsConnected()) {
      // Non-primary services need some other service connected first.
      continue;
    }
    if (ContainsKey(connecting_technologies, technology)) {
      // We have already started a connection for this technology.
      continue;
    }
    if (service->explicitly_disconnected())
      continue;
    connecting_technologies.insert(technology);
    if (!service->IsConnected() && !service->IsConnecting()) {
      // At first blush, it may seem that using Service::AutoConnect might
      // be the right choice, however Service::IsAutoConnectable and its
      // overridden implementations consider a host of conditions which
      // prevent it from attempting a connection which we'd like to ignore
      // for the purposes of this user-initiated action.
      Error error;
      service->Connect(&error, __func__);
      if (error.IsFailure()) {
        LOG(ERROR) << "Connection failed: " << error.message();
      }
    }
  }

  if (SLOG_IS_ON(Manager, 4)) {
    SLOG(this, 4) << "Sorted service list for ConnectToBestServicesTask: ";
    for (size_t i = 0; i < services_copy.size(); ++i) {
      ServiceRefPtr service = services_copy[i];
      const char* compare_reason = nullptr;
      if (i + 1 < services_copy.size()) {
        if (!service->connectable()) {
          // Due to service sort order, it is guaranteed that no services beyond
          // this one are connectable either.
          break;
        }
        Service::Compare(
            this, service, services_copy[i+1],
            kCompareConnectivityState, technology_order_,
            &compare_reason);
      } else {
        compare_reason = "last";
      }
      SLOG(this, 4) << "Service " << service->unique_name()
                    << " Profile: " << service->profile()->GetFriendlyName()
                    << " IsConnected: " << service->IsConnected()
                    << " IsConnecting: " << service->IsConnecting()
                    << " HasEverConnected: " << service->has_ever_connected()
                    << " IsFailed: " << service->IsFailed()
                    << " connectable: " << service->connectable()
                    << " auto_connect: " << service->auto_connect()
                    << " retain_auto_connect: "
                    << service->retain_auto_connect()
                    << " priority: " << service->priority()
                    << " crypto_algorithm: " << service->crypto_algorithm()
                    << " key_rotation: " << service->key_rotation()
                    << " endpoint_auth: " << service->endpoint_auth()
                    << " strength: " << service->strength()
                    << " sorted: " << compare_reason;
    }
  }
}

void Manager::CreateConnectivityReport(Error* /*error*/) {
  LOG(INFO) << "Creating Connectivity Report";

  // For each of the connected services, perform a single portal detection
  // test to assess connectivity.  The results should be written to the log.
  for (const auto& service : services_) {
    if (!service->IsConnected()) {
      // Service sort order guarantees that no service beyond this one will be
      // connected either.
      break;
    }
    // Get the underlying device for this service and perform connectivity test.
    for (const auto& device : devices_) {
      if (device->IsConnectedToService(service)) {
        if (device->StartConnectivityTest()) {
          SLOG(this, 3) << "Started connectivity test for service "
                        << service->unique_name();
        } else {
          SLOG(this, 3) << "Failed to start connectivity test for service "
                        << service->unique_name()
                           << " device not reporting IsConnected.";
        }
        break;
      }
    }
  }
}

bool Manager::IsConnected() const {
  // |services_| is sorted such that connected services are first.
  return !services_.empty() && services_.front()->IsConnected();
}

bool Manager::IsOnline() const {
  // |services_| is sorted such that online services are first.
  return !services_.empty() && services_.front()->IsOnline();
}

string Manager::CalculateState(Error* /*error*/) {
  return IsConnected() ? kStateOnline : kStateOffline;
}

void Manager::RefreshConnectionState() {
  const ServiceRefPtr& service = GetDefaultService();
  string connection_state = service ? service->GetStateString() : kStateIdle;
  if (connection_state_ == connection_state) {
    return;
  }
  connection_state_ = connection_state;
  adaptor_->EmitStringChanged(kConnectionStateProperty, connection_state_);
  // Send upstart notifications for the initial idle state
  // and when we transition in/out of connected states.
  if ((!is_connected_state_) && (IsConnected())) {
      is_connected_state_ = true;
      upstart_->NotifyConnected();
  } else if ((is_connected_state_) && (!IsConnected())) {
      is_connected_state_ = false;
      upstart_->NotifyDisconnected();
  } else if (connection_state_ == kStateIdle) {
      upstart_->NotifyDisconnected();
  }
}

vector<string> Manager::AvailableTechnologies(Error* /*error*/) {
  set<string> unique_technologies;
  for (const auto& device : devices_) {
    unique_technologies.insert(
        Technology::NameFromIdentifier(device->technology()));
  }
  return vector<string>(unique_technologies.begin(), unique_technologies.end());
}

vector<string> Manager::ConnectedTechnologies(Error* /*error*/) {
  set<string> unique_technologies;
  for (const auto& device : devices_) {
    if (device->IsConnected())
      unique_technologies.insert(
          Technology::NameFromIdentifier(device->technology()));
  }
  return vector<string>(unique_technologies.begin(), unique_technologies.end());
}

bool Manager::IsTechnologyConnected(Technology::Identifier technology) const {
  for (const auto& device : devices_) {
    if (device->technology() == technology && device->IsConnected())
      return true;
  }
  return false;
}

string Manager::DefaultTechnology(Error* /*error*/) {
  return (!services_.empty() && services_[0]->IsConnected()) ?
      services_[0]->GetTechnologyString() : "";
}

vector<string> Manager::EnabledTechnologies(Error* /*error*/) {
  set<string> unique_technologies;
  for (const auto& device : devices_) {
    if (device->enabled())
      unique_technologies.insert(
          Technology::NameFromIdentifier(device->technology()));
  }
  return vector<string>(unique_technologies.begin(), unique_technologies.end());
}

vector<string> Manager::UninitializedTechnologies(Error* /*error*/) {
  return device_info_.GetUninitializedTechnologies();
}

RpcIdentifiers Manager::EnumerateDevices(Error* /*error*/) {
  RpcIdentifiers device_rpc_ids;
  for (const auto& device : devices_) {
    device_rpc_ids.push_back(device->GetRpcIdentifier());
  }
  // Enumerate devices that are internal to the services, such as PPPoE devices.
  for (const auto& service : services_) {
    if (!service->GetInnerDeviceRpcIdentifier().empty()) {
      device_rpc_ids.push_back(service->GetInnerDeviceRpcIdentifier());
    }
  }
  return device_rpc_ids;
}

RpcIdentifiers Manager::EnumerateProfiles(Error* /*error*/) {
  RpcIdentifiers profile_rpc_ids;
  for (const auto& profile : profiles_) {
    profile_rpc_ids.push_back(profile->GetRpcIdentifier());
  }
  return profile_rpc_ids;
}

RpcIdentifiers Manager::EnumerateAvailableServices(Error* /*error*/) {
  RpcIdentifiers service_rpc_ids;
  for (const auto& service : services_) {
    if (service->IsVisible()) {
      service_rpc_ids.push_back(service->GetRpcIdentifier());
    }
  }
  return service_rpc_ids;
}

RpcIdentifiers Manager::EnumerateCompleteServices(Error* /*error*/) {
  RpcIdentifiers service_rpc_ids;
  for (const auto& service : services_) {
    service_rpc_ids.push_back(service->GetRpcIdentifier());
  }
  return service_rpc_ids;
}

RpcIdentifiers Manager::EnumerateWatchedServices(Error* /*error*/) {
  RpcIdentifiers service_rpc_ids;
  watched_service_states_.clear();
  for (const auto& service : services_) {
    if (service->IsVisible() && service->IsActive(nullptr)) {
      service_rpc_ids.push_back(service->GetRpcIdentifier());
      watched_service_states_[service->unique_name()] = service->state();
    }
  }
  return service_rpc_ids;
}

string Manager::GetActiveProfileRpcIdentifier(Error* /*error*/) {
  return ActiveProfile()->GetRpcIdentifier();
}

string Manager::GetCheckPortalList(Error* /*error*/) {
  return use_startup_portal_list_ ? startup_portal_list_ :
      props_.check_portal_list;
}

bool Manager::SetCheckPortalList(const string& portal_list, Error* error) {
  use_startup_portal_list_ = false;
  if (props_.check_portal_list == portal_list) {
    return false;
  }
  props_.check_portal_list = portal_list;
  return true;
}

string Manager::GetIgnoredDNSSearchPaths(Error* /*error*/) {
  return props_.ignored_dns_search_paths;
}

bool Manager::SetIgnoredDNSSearchPaths(const string& ignored_paths,
                                       Error* /*error*/) {
  if (props_.ignored_dns_search_paths == ignored_paths) {
    return false;
  }
  vector<string> ignored_path_list;
  if (!ignored_paths.empty()) {
    ignored_path_list = base::SplitString(
        ignored_paths, ",", base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);
  }
  props_.ignored_dns_search_paths = ignored_paths;
  resolver_->set_ignored_search_list(ignored_path_list);
  return true;
}

// called via RPC (e.g., from ManagerDBusAdaptor)
ServiceRefPtr Manager::GetService(const KeyValueStore& args, Error* error) {
  if (args.ContainsString(kTypeProperty) &&
      args.GetString(kTypeProperty) == kTypeVPN) {
     // GetService on a VPN service should actually perform ConfigureService.
     // TODO(pstew): Remove this hack and change Chrome to use ConfigureService
     // instead, when we no longer need to support flimflam.  crbug.com/213802
     return ConfigureService(args, error);
  }

  ServiceRefPtr service = GetServiceInner(args, error);
  if (service) {
    // Configures the service using the rest of the passed-in arguments.
    service->Configure(args, error);
  }

  return service;
}

ServiceRefPtr Manager::GetServiceInner(const KeyValueStore& args,
                                       Error* error) {
  if (args.ContainsString(kGuidProperty)) {
    SLOG(this, 2) << __func__ << ": searching by GUID";
    ServiceRefPtr service =
        GetServiceWithGUID(args.GetString(kGuidProperty), nullptr);
    if (service) {
      return service;
    }
  }

  if (!args.ContainsString(kTypeProperty)) {
    Error::PopulateAndLog(
        FROM_HERE, error, Error::kInvalidArguments, kErrorTypeRequired);
    return nullptr;
  }

  string type = args.GetString(kTypeProperty);
  Technology::Identifier technology = Technology::IdentifierFromName(type);
  if (!ContainsKey(providers_, technology)) {
    Error::PopulateAndLog(FROM_HERE, error, Error::kNotSupported,
                          kErrorUnsupportedServiceType);
    return nullptr;
  }

  SLOG(this, 2) << __func__ << ": getting " << type << " Service";
  return providers_[technology]->GetService(args, error);
}

// called via RPC (e.g., from ManagerDBusAdaptor)
ServiceRefPtr Manager::ConfigureService(const KeyValueStore& args,
                                        Error* error) {
  ProfileRefPtr profile = ActiveProfile();
  bool profile_specified = args.ContainsString(kProfileProperty);
  if (profile_specified) {
    string profile_rpcid = args.GetString(kProfileProperty);
    profile = LookupProfileByRpcIdentifier(profile_rpcid);
    if (!profile) {
      Error::PopulateAndLog(FROM_HERE, error, Error::kInvalidArguments,
                            "Invalid profile name " + profile_rpcid);
      return nullptr;
    }
  }

  ServiceRefPtr service = GetServiceInner(args, error);
  if (error->IsFailure() || !service) {
    LOG(ERROR) << "GetService failed; returning upstream error.";
    return nullptr;
  }

  // First pull in any stored configuration associated with the service.
  if (service->profile() == profile) {
    SLOG(this, 2) << __func__ << ": service " << service->unique_name()
                  << " is already a member of profile "
                     << profile->GetFriendlyName()
                     << " so a load is not necessary.";
  } else if (profile->LoadService(service)) {
    SLOG(this, 2) << __func__ << ": applied stored information from profile "
                  << profile->GetFriendlyName()
                  << " into service "
                  << service->unique_name();
  } else {
    SLOG(this, 2) << __func__ << ": no previous information in profile "
                  << profile->GetFriendlyName()
                  << " exists for service "
                  << service->unique_name();
  }

  // Overlay this with the passed-in configuration parameters.
  service->Configure(args, error);

  // Overwrite the profile data with the resulting configured service.
  if (!profile->UpdateService(service)) {
    Error::PopulateAndLog(FROM_HERE, error, Error::kInternalError,
                          "Unable to save service to profile");
    return nullptr;
  }

  if (HasService(service)) {
    // If the service has been registered (it may not be -- as is the case
    // with invisible WiFi networks), we can now transfer the service between
    // profiles.
    if (IsServiceEphemeral(service) ||
        (profile_specified && service->profile() != profile)) {
      SLOG(this, 2) << "Moving service to profile "
                    << profile->GetFriendlyName();
      if (!MoveServiceToProfile(service, profile)) {
        Error::PopulateAndLog(FROM_HERE, error, Error::kInternalError,
                              "Unable to move service to profile");
      }
    }
  }

  // Notify the service that a profile has been configured for it.
  service->OnProfileConfigured();

  return service;
}

// called via RPC (e.g., from ManagerDBusAdaptor)
ServiceRefPtr Manager::ConfigureServiceForProfile(
    const string& profile_rpcid, const KeyValueStore& args, Error* error) {
  if (!args.ContainsString(kTypeProperty)) {
    Error::PopulateAndLog(
        FROM_HERE, error, Error::kInvalidArguments, kErrorTypeRequired);
    return nullptr;
  }

  string type = args.GetString(kTypeProperty);
  Technology::Identifier technology = Technology::IdentifierFromName(type);

  if (!ContainsKey(providers_, technology)) {
    Error::PopulateAndLog(FROM_HERE, error, Error::kNotSupported,
                          kErrorUnsupportedServiceType);
    return nullptr;
  }

  ProviderInterface* provider = providers_[technology];

  ProfileRefPtr profile = LookupProfileByRpcIdentifier(profile_rpcid);
  if (!profile) {
    Error::PopulateAndLog(FROM_HERE, error, Error::kNotFound,
                          "Profile specified was not found");
    return nullptr;
  }
  if (args.LookupString(kProfileProperty, profile_rpcid) != profile_rpcid) {
    Error::PopulateAndLog(FROM_HERE, error, Error::kInvalidArguments,
                          "Profile argument does not match that in "
                          "the configuration arguments");
    return nullptr;
  }

  ServiceRefPtr service;
  if (args.ContainsString(kGuidProperty)) {
    SLOG(this, 2) << __func__ << ": searching by GUID";
    service = GetServiceWithGUID(args.GetString(kGuidProperty), nullptr);
    if (service && service->technology() != technology) {
      Error::PopulateAndLog(FROM_HERE, error, Error::kNotSupported,
                            StringPrintf("This GUID matches a non-%s service",
                                         type.c_str()));
      return nullptr;
    }
  }

  if (!service) {
    Error find_error;
    service = provider->FindSimilarService(args, &find_error);
  }

  // If no matching service exists, create a new service in the specified
  // profile using ConfigureService().
  if (!service) {
    KeyValueStore configure_args;
    configure_args.CopyFrom(args);
    configure_args.SetString(kProfileProperty, profile_rpcid);
    return ConfigureService(configure_args, error);
  }

  // The service already exists and is set to the desired profile,
  // the service is in the ephemeral profile, or the current profile
  // for the service appears before the desired profile, we need to
  // reassign the service to the new profile if necessary, leaving
  // the old profile intact (i.e, not calling Profile::AbandonService()).
  // Then, configure the properties on the service as well as its newly
  // associated profile.
  if (service->profile() == profile ||
      IsServiceEphemeral(service) ||
      IsProfileBefore(service->profile(), profile)) {
    SetupServiceInProfile(service, profile, args, error);
    return service;
  }

  // The current profile for the service appears after the desired
  // profile.  We must create a temporary service specifically for
  // the task of creating configuration data.  This service will
  // neither inherit properties from the visible service, nor will
  // it exist after this function returns.
  service = provider->CreateTemporaryService(args, error);
  if (!service || !error->IsSuccess()) {
    // Service::CreateTemporaryService() failed, and has set the error
    // appropriately.
    return nullptr;
  }

  // The profile may already have configuration for this service.
  profile->ConfigureService(service);

  SetupServiceInProfile(service, profile, args, error);

  // Although we have succeeded, this service will not exist, so its
  // path is of no use to the caller.
  DCHECK(service->HasOneRef());
  return nullptr;
}

void Manager::SetupServiceInProfile(ServiceRefPtr service,
                                    ProfileRefPtr profile,
                                    const KeyValueStore& args,
                                    Error* error) {
  service->SetProfile(profile);
  service->Configure(args, error);
  profile->UpdateService(service);
}

ServiceRefPtr Manager::FindMatchingService(const KeyValueStore& args,
                                           Error* error) {
  for (const auto& service : services_) {
    if (service->DoPropertiesMatch(args)) {
      return service;
    }
  }
  error->Populate(Error::kNotFound, "Matching service was not found");
  return nullptr;
}

const map<string, GeolocationInfos>
    &Manager::GetNetworksForGeolocation() const {
  return networks_for_geolocation_;
}

void Manager::OnDeviceGeolocationInfoUpdated(const DeviceRefPtr& device) {
  SLOG(this, 2) << __func__ << " for technology "
                << Technology::NameFromIdentifier(device->technology());
  switch (device->technology()) {
    // TODO(gauravsh): crbug.com/217833 Need a strategy for combining
    // geolocation objects from multiple devices of the same technolgy.
    // Currently, we just override the any previously acquired
    // geolocation objects for the retrieved technology type.
    case Technology::kWifi:
      networks_for_geolocation_[kGeoWifiAccessPointsProperty] =
          device->GetGeolocationObjects();
      break;
    case Technology::kCellular:
      networks_for_geolocation_[kGeoCellTowersProperty] =
          device->GetGeolocationObjects();
      break;
    default:
      // Ignore other technologies.
      break;
  }
}

void Manager::RecheckPortal(Error* /*error*/) {
  for (const auto& device : devices_) {
    if (device->RequestPortalDetection()) {
      // Only start Portal Detection on the device with the default connection.
      // We will get a "true" return value when we've found that device, and
      // can end our loop early as a result.
      break;
    }
  }
}

void Manager::RecheckPortalOnService(const ServiceRefPtr& service) {
  for (const auto& device : devices_) {
    if (device->IsConnectedToService(service)) {
      // As opposed to RecheckPortal() above, we explicitly stop and then
      // restart portal detection, since the service to recheck was explicitly
      // specified.
      device->RestartPortalDetection();
      break;
    }
  }
}

void Manager::RequestScan(Device::ScanType scan_type,
                          const string& technology, Error* error) {
  if (technology == kTypeWifi || technology == "") {
    for (const auto& wifi_device : FilterByTechnology(Technology::kWifi)) {
      metrics_->NotifyUserInitiatedEvent(Metrics::kUserInitiatedEventWifiScan);
      wifi_device->Scan(scan_type, error, __func__);
    }
  } else {
    // TODO(quiche): support scanning for other technologies?
    Error::PopulateAndLog(FROM_HERE, error, Error::kInvalidArguments,
                          "Unrecognized technology " + technology);
  }
}

void Manager::SetSchedScan(bool enable, Error* error) {
  for (const auto& wifi_device : FilterByTechnology(Technology::kWifi)) {
    wifi_device->SetSchedScan(enable, error);
  }
}

string Manager::GetTechnologyOrder() {
  vector<string> technology_names;
  for (const auto& technology : technology_order_) {
    technology_names.push_back(Technology::NameFromIdentifier(technology));
  }

  return base::JoinString(technology_names, ",");
}

void Manager::SetTechnologyOrder(const string& order, Error* error) {
  vector<Technology::Identifier> new_order;
  SLOG(this, 2) << "Setting technology order to " << order;
  if (!Technology::GetTechnologyVectorFromString(order, &new_order, error)) {
    return;
  }

  technology_order_ = new_order;
  if (running_) {
    SortServices();
  }
}

bool Manager::IsWifiIdle() {
  bool ret = false;

  // Since services are sorted by connection state, status of the wifi device
  // can be determine by examing the connection state of the first wifi service.
  for (const auto& service : services_) {
    if (service->technology() == Technology::kWifi) {
      if (!service->IsConnecting() && !service->IsConnected()) {
        ret = true;
      }
      break;
    }
  }
  return ret;
}

void Manager::UpdateProviderMapping() {
#if !defined(DISABLE_WIRED_8021X)
  providers_[Technology::kEthernetEap] = ethernet_eap_provider_.get();
#endif  // DISABLE_WIRED_8021X
  providers_[Technology::kVPN] = vpn_provider_.get();
#if !defined(DISABLE_WIFI)
  providers_[Technology::kWifi] = wifi_provider_.get();
#endif  // DISABLE_WIFI
#if !defined(DISABLE_WIMAX)
  providers_[Technology::kWiMax] = wimax_provider_.get();
#endif  // DISABLE_WIMAX
}

DeviceRefPtr Manager::GetDeviceConnectedToService(ServiceRefPtr service) {
  for (DeviceRefPtr device : devices_) {
    if (device->IsConnectedToService(service)) {
      return device;
    }
  }
  return nullptr;
}

void Manager::DetectMultiHomedDevices() {
  map<string, vector<DeviceRefPtr>> subnet_buckets;
  for (const auto& device : devices_) {
    const auto& connection = device->connection();
    string subnet_name;
    if (connection) {
      subnet_name = connection->GetSubnetName();
    }
    if (subnet_name.empty()) {
      device->SetIsMultiHomed(false);
    } else {
      subnet_buckets[subnet_name].push_back(device);
    }
  }

  for (const auto& subnet_bucket : subnet_buckets) {
    const auto& device_list = subnet_bucket.second;
    if (device_list.size() > 1) {
      for (const auto& device : device_list) {
        device->SetIsMultiHomed(true);
      }
    } else {
      DCHECK_EQ(1U, device_list.size());
      device_list.back()->SetIsMultiHomed(false);
    }
  }
}

}  // namespace shill
