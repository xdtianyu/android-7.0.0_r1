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

#ifndef SHILL_WIFI_WIFI_PROVIDER_H_
#define SHILL_WIFI_WIFI_PROVIDER_H_

#include <time.h>

#include <deque>
#include <map>
#include <string>
#include <vector>

#include <gtest/gtest_prod.h>  // for FRIEND_TEST

#include "shill/accessor_interface.h"  // for ByteArrays
#include "shill/provider_interface.h"
#include "shill/refptr_types.h"

namespace shill {

class ByteString;
class ControlInterface;
class Error;
class EventDispatcher;
class KeyValueStore;
class Manager;
class Metrics;
class StoreInterface;
class Time;
class WiFiEndpoint;
class WiFiService;

// The WiFi Provider is the holder of all WiFi Services.  It holds both
// visible (created due to an Endpoint becoming visible) and invisible
// (created due to user or storage configuration) Services.
class WiFiProvider : public ProviderInterface {
 public:
  static const char kStorageFrequencies[];
  static const int kMaxStorageFrequencies;
  typedef std::map<uint16_t, int64_t> ConnectFrequencyMap;
  // The key to |ConnectFrequencyMapDated| is the number of days since the
  // Epoch.
  typedef std::map<time_t, ConnectFrequencyMap> ConnectFrequencyMapDated;
  struct FrequencyCount {
    FrequencyCount() : frequency(0), connection_count(0) {}
    FrequencyCount(uint16_t freq, size_t conn)
        : frequency(freq), connection_count(conn) {}
    uint16_t frequency;
    size_t connection_count;  // Number of successful connections at this
                              // frequency.
  };
  typedef std::deque<FrequencyCount> FrequencyCountList;

  WiFiProvider(ControlInterface* control_interface,
               EventDispatcher* dispatcher,
               Metrics* metrics,
               Manager* manager);
  ~WiFiProvider() override;

  // Called by Manager as a part of the Provider interface.  The attributes
  // used for matching services for the WiFi provider are the SSID, mode and
  // security parameters.
  void CreateServicesFromProfile(const ProfileRefPtr& profile) override;
  ServiceRefPtr FindSimilarService(
      const KeyValueStore& args, Error* error) const override;
  ServiceRefPtr GetService(const KeyValueStore& args, Error* error) override;
  ServiceRefPtr CreateTemporaryService(
      const KeyValueStore& args, Error* error) override;
  ServiceRefPtr CreateTemporaryServiceFromProfile(
      const ProfileRefPtr& profile,
      const std::string& entry_name,
      Error* error) override;
  void Start() override;
  void Stop() override;

  // Find a Service this Endpoint should be associated with.
  virtual WiFiServiceRefPtr FindServiceForEndpoint(
      const WiFiEndpointConstRefPtr& endpoint);

  // Find or create a Service for |endpoint| to be associated with.  This
  // method first calls FindServiceForEndpoint, and failing this, creates
  // a new Service.  It then associates |endpoint| with this service.
  virtual void OnEndpointAdded(const WiFiEndpointConstRefPtr& endpoint);

  // Called by a Device when it removes an Endpoint.  If the Provider
  // forgets a service as a result, it returns a reference to the
  // forgotten service, otherwise it returns a null reference.
  virtual WiFiServiceRefPtr OnEndpointRemoved(
      const WiFiEndpointConstRefPtr& endpoint);

  // Called by a Device when it receives notification that an Endpoint
  // has changed.  Ensure the updated endpoint still matches its
  // associated service.  If necessary re-assign the endpoint to a new
  // service, otherwise notify the associated service of the update to
  // the endpoint.
  virtual void OnEndpointUpdated(const WiFiEndpointConstRefPtr& endpoint);

  // Called by a WiFiService when it is unloaded and no longer visible.
  virtual bool OnServiceUnloaded(const WiFiServiceRefPtr& service);

  // Get the list of SSIDs for hidden WiFi services we are aware of.
  virtual ByteArrays GetHiddenSSIDList();

  // Calls WiFiService::FixupServiceEntries() and adds a UMA metric if
  // this causes entries to be updated.
  virtual void LoadAndFixupServiceEntries(Profile* profile);

  // Save configuration for wifi_provider to |storage|.
  virtual bool Save(StoreInterface* storage) const;

  virtual void IncrementConnectCount(uint16_t frequency_mhz);

  // Returns a list of all of the frequencies on which this device has
  // connected.  This data is accumulated across multiple shill runs.
  virtual FrequencyCountList GetScanFrequencies() const;

  // Report the number of auto connectable services available to uma
  // metrics.
  void ReportAutoConnectableServices();

  // Returns number of services available for auto-connect.
  virtual int NumAutoConnectableServices();

  // Returns a list of ByteStrings representing the SSIDs of WiFi services
  // configured for auto-connect.
  std::vector<ByteString> GetSsidsConfiguredForAutoConnect();

  bool disable_vht() { return disable_vht_; }
  void set_disable_vht(bool disable_vht) { disable_vht_ = disable_vht; }

 private:
  friend class WiFiProviderTest;
  FRIEND_TEST(WiFiProviderTest, FrequencyMapAgingIllegalDay);
  FRIEND_TEST(WiFiProviderTest, FrequencyMapBasicAging);
  FRIEND_TEST(WiFiProviderTest, FrequencyMapToStringList);
  FRIEND_TEST(WiFiProviderTest, FrequencyMapToStringListEmpty);
  FRIEND_TEST(WiFiProviderTest, IncrementConnectCount);
  FRIEND_TEST(WiFiProviderTest, IncrementConnectCountCreateNew);
  FRIEND_TEST(WiFiProviderTest, LoadAndFixupServiceEntriesDefaultProfile);
  FRIEND_TEST(WiFiProviderTest, LoadAndFixupServiceEntriesUserProfile);
  FRIEND_TEST(WiFiProviderTest, LoadAndFixupServiceEntriesNothingToDo);
  FRIEND_TEST(WiFiProviderTest, StringListToFrequencyMap);
  FRIEND_TEST(WiFiProviderTest, StringListToFrequencyMapEmpty);

  typedef std::map<const WiFiEndpoint*, WiFiServiceRefPtr> EndpointServiceMap;

  static const char kManagerErrorSSIDTooLong[];
  static const char kManagerErrorSSIDTooShort[];
  static const char kManagerErrorSSIDRequired[];
  static const char kManagerErrorUnsupportedSecurityClass[];
  static const char kManagerErrorUnsupportedSecurityMode[];
  static const char kManagerErrorUnsupportedServiceMode[];
  static const char kManagerErrorArgumentConflict[];
  static const char kFrequencyDelimiter;
  static const char kStartWeekHeader[];
  static const time_t kIllegalStartWeek;
  static const char kStorageId[];
  static const time_t kWeeksToKeepFrequencyCounts;
  static const time_t kSecondsPerWeek;

  // Add a service to the service_ vector and register it with the Manager.
  WiFiServiceRefPtr AddService(const std::vector<uint8_t>& ssid,
                               const std::string& mode,
                               const std::string& security,
                               bool is_hidden);

  // Find a service given its properties.
  WiFiServiceRefPtr FindService(const std::vector<uint8_t>& ssid,
                                const std::string& mode,
                                const std::string& security) const;

  // Returns a WiFiServiceRefPtr for unit tests and for down-casting to a
  // ServiceRefPtr in GetService().
  WiFiServiceRefPtr GetWiFiService(const KeyValueStore& args, Error* error);

  // Disassociate the service from its WiFi device and remove it from the
  // services_ vector.
  void ForgetService(const WiFiServiceRefPtr& service);

  void ReportRememberedNetworkCount();
  void ReportServiceSourceMetrics();

  // Retrieve a WiFi service's identifying properties from passed-in |args|.
  // Returns true if |args| are valid and populates |ssid|, |mode|,
  // |security| and |hidden_ssid|, if successful.  Otherwise, this function
  // returns false and populates |error| with the reason for failure.  It
  // is a fatal error if the "Type" parameter passed in |args| is not kWiFi.
  static bool GetServiceParametersFromArgs(const KeyValueStore& args,
                                           std::vector<uint8_t>* ssid_bytes,
                                           std::string* mode,
                                           std::string* security_method,
                                           bool* hidden_ssid,
                                           Error* error);
  // Retrieve a WiFi service's identifying properties from passed-in |storage|.
  // Return true if storage contain valid parameter values and populates |ssid|,
  // |mode|, |security| and |hidden_ssid|. Otherwise, this function returns
  // false and populates |error| with the reason for failure.
  static bool GetServiceParametersFromStorage(const StoreInterface* storage,
                                              const std::string& entry_name,
                                              std::vector<uint8_t>* ssid_bytes,
                                              std::string* mode,
                                              std::string* security_method,
                                              bool* hidden_ssid,
                                              Error* error);

  // Converts frequency profile information from a list of strings of the form
  // "frequency:connection_count" to a form consistent with
  // |connect_count_by_frequency_|.  The first string must be of the form
  // [nnn] where |nnn| is a positive integer that represents the creation time
  // (number of days since the Epoch) of the data.
  static time_t StringListToFrequencyMap(
      const std::vector<std::string>& strings,
      ConnectFrequencyMap* numbers);

  // Extracts the start week from the first string in the StringList for
  // |StringListToFrequencyMap|.
  static time_t GetStringListStartWeek(const std::string& week_string);

  // Extracts frequency and connection count from a string from the StringList
  // for |StringListToFrequencyMap|.  Places those values in |numbers|.
  static void ParseStringListFreqCount(const std::string& freq_count_string,
                                       ConnectFrequencyMap* numbers);

  // Converts frequency profile information from a form consistent with
  // |connect_count_by_frequency_| to a list of strings of the form
  // "frequency:connection_count".  The |creation_day| is the day that the
  // data was first createed (represented as the number of days since the
  // Epoch).
  static void FrequencyMapToStringList(time_t creation_day,
                                       const ConnectFrequencyMap& numbers,
                                       std::vector<std::string>* strings);

  ControlInterface* control_interface_;
  EventDispatcher* dispatcher_;
  Metrics* metrics_;
  Manager* manager_;

  std::vector<WiFiServiceRefPtr> services_;
  EndpointServiceMap service_by_endpoint_;

  bool running_;

  // Map of frequencies at which we've connected and the number of times a
  // successful connection has been made at that frequency.  Absent frequencies
  // have not had a successful connection.
  ConnectFrequencyMap connect_count_by_frequency_;
  // A number of entries of |ConnectFrequencyMap| stored by date of creation.
  ConnectFrequencyMapDated connect_count_by_frequency_dated_;

  // Count of successful wifi connections we've made.
  int64_t total_frequency_connections_;

  Time* time_;

  // Disable 802.11ac Very High Throughput (VHT) connections.
  bool disable_vht_;

  DISALLOW_COPY_AND_ASSIGN(WiFiProvider);
};

}  // namespace shill

#endif  // SHILL_WIFI_WIFI_PROVIDER_H_
