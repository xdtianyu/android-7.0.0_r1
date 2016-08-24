//
// Copyright (C) 2013 The Android Open Source Project
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

#include "shill/wifi/wifi_provider.h"

#include <stdlib.h>

#include <algorithm>
#include <limits>
#include <set>
#include <string>
#include <vector>

#include <base/bind.h>
#include <base/format_macros.h>
#include <base/strings/string_number_conversions.h>
#include <base/strings/string_split.h>
#include <base/strings/string_util.h>

#include "shill/error.h"
#include "shill/event_dispatcher.h"
#include "shill/key_value_store.h"
#include "shill/logging.h"
#include "shill/manager.h"
#include "shill/metrics.h"
#include "shill/net/byte_string.h"
#include "shill/net/ieee80211.h"
#include "shill/net/shill_time.h"
#include "shill/profile.h"
#include "shill/store_interface.h"
#include "shill/technology.h"
#include "shill/wifi/wifi_endpoint.h"
#include "shill/wifi/wifi_service.h"

using base::Bind;
using base::SplitString;
using base::StringPrintf;
using std::set;
using std::string;
using std::vector;

namespace shill {

namespace Logging {
static auto kModuleLogScope = ScopeLogger::kWiFi;
static string ObjectID(WiFiProvider* w) { return "(wifi_provider)"; }
}

// Note that WiFiProvider generates some manager-level errors, because it
// implements the WiFi portion of the Manager.GetService flimflam API. The
// API is implemented here, rather than in manager, to keep WiFi-specific
// logic in the right place.
const char WiFiProvider::kManagerErrorSSIDRequired[] = "must specify SSID";
const char WiFiProvider::kManagerErrorSSIDTooLong[]  = "SSID is too long";
const char WiFiProvider::kManagerErrorSSIDTooShort[] = "SSID is too short";
const char WiFiProvider::kManagerErrorUnsupportedSecurityClass[] =
    "security class is unsupported";
const char WiFiProvider::kManagerErrorUnsupportedSecurityMode[] =
    "security mode is unsupported";
const char WiFiProvider::kManagerErrorUnsupportedServiceMode[] =
    "service mode is unsupported";
const char WiFiProvider::kManagerErrorArgumentConflict[] =
    "provided arguments are inconsistent";
const char WiFiProvider::kFrequencyDelimiter = ':';
const char WiFiProvider::kStartWeekHeader[] = "@";
const time_t WiFiProvider::kIllegalStartWeek =
    std::numeric_limits<time_t>::max();
const char WiFiProvider::kStorageId[] = "provider_of_wifi";
const char WiFiProvider::kStorageFrequencies[] = "Frequencies";
const int WiFiProvider::kMaxStorageFrequencies = 20;
const time_t WiFiProvider::kWeeksToKeepFrequencyCounts = 3;
const time_t WiFiProvider::kSecondsPerWeek = 60 * 60 * 24 * 7;

WiFiProvider::WiFiProvider(ControlInterface* control_interface,
                           EventDispatcher* dispatcher,
                           Metrics* metrics,
                           Manager* manager)
    : control_interface_(control_interface),
      dispatcher_(dispatcher),
      metrics_(metrics),
      manager_(manager),
      running_(false),
      total_frequency_connections_(-1L),
      time_(Time::GetInstance()),
      disable_vht_(false) {}

WiFiProvider::~WiFiProvider() {}

void WiFiProvider::Start() {
  running_ = true;
}

void WiFiProvider::Stop() {
  SLOG(this, 2) << __func__;
  while (!services_.empty()) {
    WiFiServiceRefPtr service = services_.back();
    ForgetService(service);
    SLOG(this, 3) << "WiFiProvider deregistering service "
                  << service->unique_name();
    manager_->DeregisterService(service);
  }
  service_by_endpoint_.clear();
  running_ = false;
}

void WiFiProvider::CreateServicesFromProfile(const ProfileRefPtr& profile) {
  const StoreInterface* storage = profile->GetConstStorage();
  KeyValueStore args;
  args.SetString(kTypeProperty, kTypeWifi);
  bool created_hidden_service = false;
  for (const auto& group : storage->GetGroupsWithProperties(args)) {
    vector<uint8_t> ssid_bytes;
    string network_mode;
    string security;
    bool is_hidden = false;
    if (!GetServiceParametersFromStorage(storage,
                                         group,
                                         &ssid_bytes,
                                         &network_mode,
                                         &security,
                                         &is_hidden,
                                         nullptr)) {
      continue;
    }

    if (FindService(ssid_bytes, network_mode, security)) {
      // If service already exists, we have nothing to do, since the
      // service has already loaded its configuration from storage.
      // This is guaranteed to happen in the single case where
      // CreateServicesFromProfile() is called on a WiFiProvider from
      // Manager::PushProfile():
      continue;
    }

    AddService(ssid_bytes, network_mode, security, is_hidden);

    // By registering the service in AddService, the rest of the configuration
    // will be loaded from the profile into the service via ConfigureService().

    if (is_hidden) {
      created_hidden_service = true;
    }
  }

  // If WiFi is unconnected and we created a hidden service as a result
  // of opening the profile, we should initiate a WiFi scan, which will
  // allow us to find any hidden services that we may have created.
  if (created_hidden_service &&
      !manager_->IsTechnologyConnected(Technology::kWifi)) {
    Error unused_error;
    manager_->RequestScan(Device::kProgressiveScan, kTypeWifi, &unused_error);
  }

  ReportRememberedNetworkCount();

  // Only report service source metrics when a user profile is pushed.
  // This ensures that we have an equal number of samples for the
  // default profile and user profiles.
  if (!profile->IsDefault()) {
    ReportServiceSourceMetrics();
  }
}

ServiceRefPtr WiFiProvider::FindSimilarService(
    const KeyValueStore& args, Error* error) const {
  vector<uint8_t> ssid;
  string mode;
  string security;
  bool hidden_ssid;

  if (!GetServiceParametersFromArgs(
          args, &ssid, &mode, &security, &hidden_ssid, error)) {
    return nullptr;
  }

  WiFiServiceRefPtr service(FindService(ssid, mode, security));
  if (!service) {
    error->Populate(Error::kNotFound, "Matching service was not found");
  }

  return service;
}

ServiceRefPtr WiFiProvider::CreateTemporaryService(
    const KeyValueStore& args, Error* error) {
  vector<uint8_t> ssid;
  string mode;
  string security;
  bool hidden_ssid;

  if (!GetServiceParametersFromArgs(
          args, &ssid, &mode, &security, &hidden_ssid, error)) {
    return nullptr;
  }

  return new WiFiService(control_interface_,
                         dispatcher_,
                         metrics_,
                         manager_,
                         this,
                         ssid,
                         mode,
                         security,
                         hidden_ssid);
}

ServiceRefPtr WiFiProvider::CreateTemporaryServiceFromProfile(
    const ProfileRefPtr& profile, const std::string& entry_name, Error* error) {
  vector<uint8_t> ssid;
  string mode;
  string security;
  bool hidden_ssid;
  if (!GetServiceParametersFromStorage(profile->GetConstStorage(),
                                       entry_name,
                                       &ssid,
                                       &mode,
                                       &security,
                                       &hidden_ssid,
                                       error)) {
    return nullptr;
  }
  return new WiFiService(control_interface_,
                         dispatcher_,
                         metrics_,
                         manager_,
                         this,
                         ssid,
                         mode,
                         security,
                         hidden_ssid);
}

ServiceRefPtr WiFiProvider::GetService(
    const KeyValueStore& args, Error* error) {
  return GetWiFiService(args, error);
}

WiFiServiceRefPtr WiFiProvider::GetWiFiService(
    const KeyValueStore& args, Error* error) {
  vector<uint8_t> ssid_bytes;
  string mode;
  string security_method;
  bool hidden_ssid;

  if (!GetServiceParametersFromArgs(
          args, &ssid_bytes, &mode, &security_method, &hidden_ssid, error)) {
    return nullptr;
  }

  WiFiServiceRefPtr service(FindService(ssid_bytes, mode, security_method));
  if (!service) {
    service = AddService(ssid_bytes,
                         mode,
                         security_method,
                         hidden_ssid);
  }

  return service;
}

WiFiServiceRefPtr WiFiProvider::FindServiceForEndpoint(
    const WiFiEndpointConstRefPtr& endpoint) {
  EndpointServiceMap::iterator service_it =
      service_by_endpoint_.find(endpoint.get());
  if (service_it == service_by_endpoint_.end())
    return nullptr;
  return service_it->second;
}

void WiFiProvider::OnEndpointAdded(const WiFiEndpointConstRefPtr& endpoint) {
  if (!running_) {
    return;
  }

  WiFiServiceRefPtr service = FindService(endpoint->ssid(),
                                          endpoint->network_mode(),
                                          endpoint->security_mode());
  if (!service) {
    const bool hidden_ssid = false;
    service = AddService(
        endpoint->ssid(),
        endpoint->network_mode(),
        WiFiService::ComputeSecurityClass(endpoint->security_mode()),
        hidden_ssid);
  }

  service->AddEndpoint(endpoint);
  service_by_endpoint_[endpoint.get()] = service;

  SLOG(this, 1) << "Assigned endpoint " << endpoint->bssid_string()
                << " to service " << service->unique_name() << ".";

  manager_->UpdateService(service);
}

WiFiServiceRefPtr WiFiProvider::OnEndpointRemoved(
    const WiFiEndpointConstRefPtr& endpoint) {
  if (!running_) {
    return nullptr;
  }

  WiFiServiceRefPtr service = FindServiceForEndpoint(endpoint);

  CHECK(service) << "Can't find Service for Endpoint "
                 << "(with BSSID " << endpoint->bssid_string() << ").";
  SLOG(this, 1) << "Removing endpoint " << endpoint->bssid_string()
                << " from Service " << service->unique_name();
  service->RemoveEndpoint(endpoint);
  service_by_endpoint_.erase(endpoint.get());

  if (service->HasEndpoints() || service->IsRemembered()) {
    // Keep services around if they are in a profile or have remaining
    // endpoints.
    manager_->UpdateService(service);
    return nullptr;
  }

  ForgetService(service);
  manager_->DeregisterService(service);

  return service;
}

void WiFiProvider::OnEndpointUpdated(const WiFiEndpointConstRefPtr& endpoint) {
  if (!running_) {
    return;
  }

  WiFiService* service = FindServiceForEndpoint(endpoint).get();
  CHECK(service);

  // If the service still matches the endpoint in its new configuration,
  // we need only to update the service.
  if (service->ssid() == endpoint->ssid() &&
      service->mode() == endpoint->network_mode() &&
      service->IsSecurityMatch(endpoint->security_mode())) {
    service->NotifyEndpointUpdated(endpoint);
    return;
  }

  // The endpoint no longer matches the associated service.  Remove the
  // endpoint, so current references to the endpoint are reset, then add
  // it again so it can be associated with a new service.
  OnEndpointRemoved(endpoint);
  OnEndpointAdded(endpoint);
}

bool WiFiProvider::OnServiceUnloaded(const WiFiServiceRefPtr& service) {
  // If the service still has endpoints, it should remain in the service list.
  if (service->HasEndpoints()) {
    return false;
  }

  // This is the one place where we forget the service but do not also
  // deregister the service with the manager.  However, by returning
  // true below, the manager will do so itself.
  ForgetService(service);
  return true;
}

void WiFiProvider::LoadAndFixupServiceEntries(Profile* profile) {
  CHECK(profile);
  StoreInterface* storage = profile->GetStorage();
  bool is_default_profile = profile->IsDefault();
  if (WiFiService::FixupServiceEntries(storage)) {
    storage->Flush();
    Metrics::ServiceFixupProfileType profile_type =
        is_default_profile ?
            Metrics::kMetricServiceFixupDefaultProfile :
            Metrics::kMetricServiceFixupUserProfile;
    metrics_->SendEnumToUMA(
        metrics_->GetFullMetricName(Metrics::kMetricServiceFixupEntriesSuffix,
                                    Technology::kWifi),
        profile_type,
        Metrics::kMetricServiceFixupMax);
  }
  // TODO(wdg): Determine how this should be structured for, currently
  // non-existant, autotests.  |kStorageFrequencies| should only exist in the
  // default profile except for autotests where a test_profile is pushed.  This
  // may need to be modified for that case.
  if (is_default_profile) {
    static_assert(kMaxStorageFrequencies > kWeeksToKeepFrequencyCounts,
                  "Persistently storing more frequencies than we can hold");
    total_frequency_connections_ = 0L;
    connect_count_by_frequency_.clear();
    time_t this_week = time_->GetSecondsSinceEpoch() / kSecondsPerWeek;
    for (int freq = 0; freq < kMaxStorageFrequencies; ++freq) {
      ConnectFrequencyMap connect_count_by_frequency;
      string freq_string = StringPrintf("%s%d", kStorageFrequencies, freq);
      vector<string> frequencies;
      if (!storage->GetStringList(kStorageId, freq_string, &frequencies)) {
        SLOG(this, 7) << "Frequency list " << freq_string << " not found";
        break;
      }
      time_t start_week = StringListToFrequencyMap(frequencies,
                                                  &connect_count_by_frequency);
      if (start_week == kIllegalStartWeek) {
        continue;  // |StringListToFrequencyMap| will have output an error msg.
      }

      if (start_week > this_week) {
        LOG(WARNING) << "Discarding frequency count info from the future";
        continue;
      }
      connect_count_by_frequency_dated_[start_week] =
          connect_count_by_frequency;

      for (const auto& freq_count :
           connect_count_by_frequency_dated_[start_week]) {
        connect_count_by_frequency_[freq_count.first] += freq_count.second;
        total_frequency_connections_ += freq_count.second;
      }
    }
    SLOG(this, 7) << __func__ << " - total count="
                  << total_frequency_connections_;
  }
}

bool WiFiProvider::Save(StoreInterface* storage) const {
  int freq = 0;
  // Iterating backwards since I want to make sure that I get the newest data.
  ConnectFrequencyMapDated::const_reverse_iterator freq_count;
  for (freq_count = connect_count_by_frequency_dated_.crbegin();
       freq_count != connect_count_by_frequency_dated_.crend();
       ++freq_count) {
    vector<string> frequencies;
    FrequencyMapToStringList(freq_count->first, freq_count->second,
                             &frequencies);
    string freq_string = StringPrintf("%s%d", kStorageFrequencies, freq);
    storage->SetStringList(kStorageId, freq_string, frequencies);
    if (++freq >= kMaxStorageFrequencies) {
      LOG(WARNING) << "Internal frequency count list has more entries than the "
                   << "string list we had allocated for it.";
      break;
    }
  }
  return true;
}

WiFiServiceRefPtr WiFiProvider::AddService(const vector<uint8_t>& ssid,
                                           const string& mode,
                                           const string& security,
                                           bool is_hidden) {
  WiFiServiceRefPtr service = new WiFiService(control_interface_,
                                              dispatcher_,
                                              metrics_,
                                              manager_,
                                              this,
                                              ssid,
                                              mode,
                                              security,
                                              is_hidden);

  services_.push_back(service);
  manager_->RegisterService(service);
  return service;
}

WiFiServiceRefPtr WiFiProvider::FindService(const vector<uint8_t>& ssid,
                                            const string& mode,
                                            const string& security) const {
  for (const auto& service : services_) {
    if (service->ssid() == ssid && service->mode() == mode &&
        service->IsSecurityMatch(security)) {
      return service;
    }
  }
  return nullptr;
}

ByteArrays WiFiProvider::GetHiddenSSIDList() {
  // Create a unique set of hidden SSIDs.
  set<ByteArray> hidden_ssids_set;
  for (const auto& service : services_) {
    if (service->hidden_ssid() && service->IsRemembered()) {
      hidden_ssids_set.insert(service->ssid());
    }
  }
  SLOG(this, 2) << "Found " << hidden_ssids_set.size() << " hidden services";
  return ByteArrays(hidden_ssids_set.begin(), hidden_ssids_set.end());
}

void WiFiProvider::ForgetService(const WiFiServiceRefPtr& service) {
  vector<WiFiServiceRefPtr>::iterator it;
  it = std::find(services_.begin(), services_.end(), service);
  if (it == services_.end()) {
    return;
  }
  (*it)->ResetWiFi();
  services_.erase(it);
}

void WiFiProvider::ReportRememberedNetworkCount() {
  metrics_->SendToUMA(
      Metrics::kMetricRememberedWiFiNetworkCount,
      std::count_if(
          services_.begin(), services_.end(),
          [](ServiceRefPtr s) { return s->IsRemembered(); }),
      Metrics::kMetricRememberedWiFiNetworkCountMin,
      Metrics::kMetricRememberedWiFiNetworkCountMax,
      Metrics::kMetricRememberedWiFiNetworkCountNumBuckets);
}

void WiFiProvider::ReportServiceSourceMetrics() {
  for (const auto& security_mode :
    {kSecurityNone, kSecurityWep, kSecurityPsk, kSecurity8021x}) {
    metrics_->SendToUMA(
        base::StringPrintf(
            Metrics::
            kMetricRememberedSystemWiFiNetworkCountBySecurityModeFormat,
            security_mode),
        std::count_if(
            services_.begin(), services_.end(),
            [security_mode](WiFiServiceRefPtr s) {
              return s->IsRemembered() && s->IsSecurityMatch(security_mode) &&
                  s->profile()->IsDefault();
            }),
        Metrics::kMetricRememberedWiFiNetworkCountMin,
        Metrics::kMetricRememberedWiFiNetworkCountMax,
        Metrics::kMetricRememberedWiFiNetworkCountNumBuckets);
    metrics_->SendToUMA(
        base::StringPrintf(
            Metrics::
            kMetricRememberedUserWiFiNetworkCountBySecurityModeFormat,
            security_mode),
        std::count_if(
            services_.begin(), services_.end(),
            [security_mode](WiFiServiceRefPtr s) {
              return s->IsRemembered() && s->IsSecurityMatch(security_mode) &&
                  !s->profile()->IsDefault();
            }),
        Metrics::kMetricRememberedWiFiNetworkCountMin,
        Metrics::kMetricRememberedWiFiNetworkCountMax,
        Metrics::kMetricRememberedWiFiNetworkCountNumBuckets);
  }
}

// static
bool WiFiProvider::GetServiceParametersFromArgs(const KeyValueStore& args,
                                                vector<uint8_t>* ssid_bytes,
                                                string* mode,
                                                string* security_method,
                                                bool* hidden_ssid,
                                                Error* error) {
  CHECK_EQ(args.LookupString(kTypeProperty, ""), kTypeWifi);

  string mode_test =
      args.LookupString(kModeProperty, kModeManaged);
  if (!WiFiService::IsValidMode(mode_test)) {
    Error::PopulateAndLog(FROM_HERE, error, Error::kNotSupported,
                          kManagerErrorUnsupportedServiceMode);
    return false;
  }

  vector<uint8_t> ssid;
  if (args.ContainsString(kWifiHexSsid)) {
    string ssid_hex_string = args.GetString(kWifiHexSsid);
    if (!base::HexStringToBytes(ssid_hex_string, &ssid)) {
      Error::PopulateAndLog(FROM_HERE, error, Error::kInvalidArguments,
                            "Hex SSID parameter is not valid");
      return false;
    }
  } else if (args.ContainsString(kSSIDProperty)) {
    string ssid_string = args.GetString(kSSIDProperty);
    ssid = vector<uint8_t>(ssid_string.begin(), ssid_string.end());
  } else {
    Error::PopulateAndLog(FROM_HERE, error, Error::kInvalidArguments,
                          kManagerErrorSSIDRequired);
    return false;
  }

  if (ssid.size() < 1) {
    Error::PopulateAndLog(FROM_HERE, error, Error::kInvalidNetworkName,
                          kManagerErrorSSIDTooShort);
    return false;
  }

  if (ssid.size() > IEEE_80211::kMaxSSIDLen) {
    Error::PopulateAndLog(FROM_HERE, error, Error::kInvalidNetworkName,
                          kManagerErrorSSIDTooLong);
    return false;
  }

  const string kDefaultSecurity = kSecurityNone;
  if (args.ContainsString(kSecurityProperty) &&
      args.ContainsString(kSecurityClassProperty) &&
      args.LookupString(kSecurityClassProperty, kDefaultSecurity) !=
      args.LookupString(kSecurityProperty, kDefaultSecurity)) {
    Error::PopulateAndLog(FROM_HERE, error, Error::kInvalidArguments,
                          kManagerErrorArgumentConflict);
    return false;
  }
  if (args.ContainsString(kSecurityClassProperty)) {
    string security_class_test =
        args.LookupString(kSecurityClassProperty, kDefaultSecurity);
    if (!WiFiService::IsValidSecurityClass(security_class_test)) {
      Error::PopulateAndLog(FROM_HERE, error, Error::kNotSupported,
                            kManagerErrorUnsupportedSecurityClass);
      return false;
    }
    *security_method = security_class_test;
  } else if (args.ContainsString(kSecurityProperty)) {
    string security_method_test =
        args.LookupString(kSecurityProperty, kDefaultSecurity);
    if (!WiFiService::IsValidSecurityMethod(security_method_test)) {
      Error::PopulateAndLog(FROM_HERE, error, Error::kNotSupported,
                            kManagerErrorUnsupportedSecurityMode);
      return false;
    }
    *security_method = security_method_test;
  } else {
    *security_method = kDefaultSecurity;
  }

  *ssid_bytes = ssid;
  *mode = mode_test;

  // If the caller hasn't specified otherwise, we assume it is a hidden service.
  *hidden_ssid = args.LookupBool(kWifiHiddenSsid, true);

  return true;
}

// static
bool WiFiProvider::GetServiceParametersFromStorage(
    const StoreInterface* storage,
    const std::string& entry_name,
    std::vector<uint8_t>* ssid_bytes,
    std::string* mode,
    std::string* security,
    bool* hidden_ssid,
    Error* error) {
  // Verify service type.
  string type;
  if (!storage->GetString(entry_name, WiFiService::kStorageType, &type) ||
      type != kTypeWifi) {
    Error::PopulateAndLog(FROM_HERE, error, Error::kInvalidArguments,
                          "Unspecified or invalid network type");
    return false;
  }
  string ssid_hex;
  if (!storage->GetString(entry_name, WiFiService::kStorageSSID, &ssid_hex) ||
      !base::HexStringToBytes(ssid_hex, ssid_bytes)) {
    Error::PopulateAndLog(FROM_HERE, error, Error::kInvalidArguments,
                          "Unspecified or invalid SSID");
    return false;
  }
  if (!storage->GetString(entry_name, WiFiService::kStorageMode, mode) ||
      mode->empty()) {
    Error::PopulateAndLog(FROM_HERE, error, Error::kInvalidArguments,
                          "Network mode not specified");
    return false;
  }
  if (!storage->GetString(entry_name, WiFiService::kStorageSecurity, security)
      || !WiFiService::IsValidSecurityMethod(*security)) {
    Error::PopulateAndLog(FROM_HERE, error, Error::kInvalidArguments,
                          "Unspecified or invalid security mode");
    return false;
  }
  if (!storage->GetBool(
      entry_name, WiFiService::kStorageHiddenSSID, hidden_ssid)) {
    Error::PopulateAndLog(FROM_HERE, error, Error::kInvalidArguments,
                          "Hidden SSID not specified");
    return false;
  }
  return true;
}

// static
time_t WiFiProvider::StringListToFrequencyMap(const vector<string>& strings,
                                            ConnectFrequencyMap* numbers) {
  if (!numbers) {
    LOG(ERROR) << "Null |numbers| parameter";
    return kIllegalStartWeek;
  }

  // Extract the start week from the first string.
  vector<string>::const_iterator strings_it = strings.begin();
  if (strings_it == strings.end()) {
    SLOG(nullptr, 7) << "Empty |strings|.";
    return kIllegalStartWeek;
  }
  time_t start_week = GetStringListStartWeek(*strings_it);
  if (start_week == kIllegalStartWeek) {
    return kIllegalStartWeek;
  }

  // Extract the frequency:count values from the remaining strings.
  for (++strings_it; strings_it != strings.end(); ++strings_it) {
    ParseStringListFreqCount(*strings_it, numbers);
  }
  return start_week;
}

// static
time_t WiFiProvider::GetStringListStartWeek(const string& week_string) {
  if (!base::StartsWith(week_string, kStartWeekHeader,
                        base::CompareCase::INSENSITIVE_ASCII)) {
    LOG(ERROR) << "Found no leading '" << kStartWeekHeader << "' in '"
               << week_string << "'";
    return kIllegalStartWeek;
  }
  return atoll(week_string.c_str() + 1);
}

// static
void WiFiProvider::ParseStringListFreqCount(const string& freq_count_string,
                                            ConnectFrequencyMap* numbers) {
  vector<string> freq_count = SplitString(
      freq_count_string, std::string{kFrequencyDelimiter},
      base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);
  if (freq_count.size() != 2) {
    LOG(WARNING) << "Found " << freq_count.size() - 1 << " '"
                 << kFrequencyDelimiter << "' in '" << freq_count_string
                 << "'.  Expected 1.";
    return;
  }
  uint16_t freq = atoi(freq_count[0].c_str());
  uint64_t connections = atoll(freq_count[1].c_str());
  (*numbers)[freq] = connections;
}

// static
void WiFiProvider::FrequencyMapToStringList(time_t start_week,
                                            const ConnectFrequencyMap& numbers,
                                            vector<string>* strings) {
  if (!strings) {
    LOG(ERROR) << "Null |strings| parameter";
    return;
  }

  strings->push_back(StringPrintf("%s%" PRIu64, kStartWeekHeader,
                                  static_cast<uint64_t>(start_week)));

  for (const auto& freq_conn : numbers) {
    // Use base::Int64ToString() instead of using something like "%llu"
    // (not correct for native 64 bit architectures) or PRId64 (does not
    // work correctly using cros_workon_make due to include intricacies).
    strings->push_back(StringPrintf("%u%c%s",
        freq_conn.first, kFrequencyDelimiter,
        base::Int64ToString(freq_conn.second).c_str()));
  }
}

void WiFiProvider::IncrementConnectCount(uint16_t frequency_mhz) {
  CHECK(total_frequency_connections_ < std::numeric_limits<int64_t>::max());

  ++connect_count_by_frequency_[frequency_mhz];
  ++total_frequency_connections_;

  time_t this_week = time_->GetSecondsSinceEpoch() / kSecondsPerWeek;
  ++connect_count_by_frequency_dated_[this_week][frequency_mhz];

  ConnectFrequencyMapDated::iterator oldest =
      connect_count_by_frequency_dated_.begin();
  time_t oldest_legal_week = this_week - kWeeksToKeepFrequencyCounts;
  while (oldest->first < oldest_legal_week) {
    SLOG(this, 6) << "Discarding frequency count info that's "
                  << this_week - oldest->first << " weeks old";
    for (const auto& freq_count : oldest->second) {
      connect_count_by_frequency_[freq_count.first] -= freq_count.second;
      if (connect_count_by_frequency_[freq_count.first] <= 0) {
        connect_count_by_frequency_.erase(freq_count.first);
      }
      total_frequency_connections_ -= freq_count.second;
    }
    connect_count_by_frequency_dated_.erase(oldest);
    oldest = connect_count_by_frequency_dated_.begin();
  }

  manager_->UpdateWiFiProvider();
  metrics_->SendToUMA(
      Metrics::kMetricFrequenciesConnectedEver,
      connect_count_by_frequency_.size(),
      Metrics::kMetricFrequenciesConnectedMin,
      Metrics::kMetricFrequenciesConnectedMax,
      Metrics::kMetricFrequenciesConnectedNumBuckets);
}

WiFiProvider::FrequencyCountList WiFiProvider::GetScanFrequencies() const {
  FrequencyCountList freq_connects_list;
  for (const auto freq_count : connect_count_by_frequency_) {
    freq_connects_list.push_back(FrequencyCount(freq_count.first,
                                                freq_count.second));
  }
  return freq_connects_list;
}

void WiFiProvider::ReportAutoConnectableServices() {
  int num_services = NumAutoConnectableServices();
  // Only report stats when there are wifi services available.
  if (num_services) {
    metrics_->NotifyWifiAutoConnectableServices(num_services);
  }
}

int WiFiProvider::NumAutoConnectableServices() {
  const char* reason = nullptr;
  int num_services = 0;
  // Determine the number of services available for auto-connect.
  for (const auto& service : services_) {
    // Service is available for auto connect if it is configured for auto
    // connect, and is auto-connectable.
    if (service->auto_connect() && service->IsAutoConnectable(&reason)) {
      num_services++;
    }
  }
  return num_services;
}

vector<ByteString> WiFiProvider::GetSsidsConfiguredForAutoConnect() {
  vector<ByteString> results;
  for (const auto& service : services_) {
    if (service->auto_connect()) {
      // Service configured for auto-connect.
      ByteString ssid_bytes(service->ssid());
      results.push_back(ssid_bytes);
    }
  }
  return results;
}

}  // namespace shill
