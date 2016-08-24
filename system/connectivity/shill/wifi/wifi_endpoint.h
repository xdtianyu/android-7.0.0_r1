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

#ifndef SHILL_WIFI_WIFI_ENDPOINT_H_
#define SHILL_WIFI_WIFI_ENDPOINT_H_

#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include <base/memory/ref_counted.h>
#include <gtest/gtest_prod.h>  // for FRIEND_TEST

#include "shill/event_dispatcher.h"
#include "shill/key_value_store.h"
#include "shill/metrics.h"
#include "shill/refptr_types.h"

namespace shill {

class ControlInterface;
class SupplicantBSSProxyInterface;

class WiFiEndpoint : public base::RefCounted<WiFiEndpoint> {
 public:
  struct SecurityFlags {
    SecurityFlags()
        : rsn_8021x(false),
          rsn_psk(false),
          wpa_8021x(false),
          wpa_psk(false),
          privacy(false) {}
    bool rsn_8021x;
    bool rsn_psk;
    bool wpa_8021x;
    bool wpa_psk;
    bool privacy;
  };
  struct VendorInformation {
    std::string wps_manufacturer;
    std::string wps_model_name;
    std::string wps_model_number;
    std::string wps_device_name;
    std::set<uint32_t> oui_set;
  };
  WiFiEndpoint(ControlInterface* control_interface,
               const WiFiRefPtr& device,
               const std::string& rpc_id,
               const KeyValueStore& properties);
  virtual ~WiFiEndpoint();

  // Set up RPC channel. Broken out from the ctor, so that WiFi can
  // look over the Endpoint details before commiting to setting up
  // RPC.
  virtual void Start();

  // Called by SupplicantBSSProxy, in response to events from
  // wpa_supplicant.
  void PropertiesChanged(const KeyValueStore& properties);

  // Called by WiFi when it polls for signal strength from the kernel.
  void UpdateSignalStrength(int16_t strength);

  // Maps mode strings from flimflam's nomenclature, as defined
  // in chromeos/dbus/service_constants.h, to uints used by supplicant
  static uint32_t ModeStringToUint(const std::string& mode_string);

  // Returns a stringmap containing information gleaned about the
  // vendor of this AP.
  std::map<std::string, std::string> GetVendorInformation() const;

  const std::vector<uint8_t>& ssid() const;
  const std::string& ssid_string() const;
  const std::string& ssid_hex() const;
  const std::string& bssid_string() const;
  const std::string& bssid_hex() const;
  const std::string& country_code() const;
  const WiFiRefPtr& device() const;
  int16_t signal_strength() const;
  uint16_t frequency() const;
  uint16_t physical_mode() const;
  const std::string& network_mode() const;
  const std::string& security_mode() const;
  bool ieee80211w_required() const;
  bool has_rsn_property() const;
  bool has_wpa_property() const;
  bool has_tethering_signature() const;

 private:
  friend class WiFiEndpointTest;
  friend class WiFiObjectTest;  // for MakeOpenEndpoint
  friend class WiFiProviderTest;  // for MakeOpenEndpoint
  friend class WiFiServiceTest;  // for MakeOpenEndpoint
  // these test cases need access to the KeyManagement enum
  FRIEND_TEST(WiFiEndpointTest, DeterminePhyModeFromFrequency);
  FRIEND_TEST(WiFiEndpointTest, ParseKeyManagementMethodsEAP);
  FRIEND_TEST(WiFiEndpointTest, ParseKeyManagementMethodsPSK);
  FRIEND_TEST(WiFiEndpointTest, ParseKeyManagementMethodsEAPAndPSK);
  FRIEND_TEST(WiFiEndpointTest, HasTetheringSignature);
  FRIEND_TEST(WiFiProviderTest, OnEndpointAddedWithSecurity);
  FRIEND_TEST(WiFiProviderTest, OnEndpointUpdated);
  FRIEND_TEST(WiFiServiceTest, ConnectTaskWPA80211w);
  FRIEND_TEST(WiFiServiceTest, GetTethering);
  FRIEND_TEST(WiFiServiceUpdateFromEndpointsTest, EndpointModified);
  FRIEND_TEST(WiFiServiceUpdateFromEndpointsTest, Ieee80211w);
  // for physical_mode_
  FRIEND_TEST(WiFiServiceUpdateFromEndpointsTest, PhysicalMode);

  enum KeyManagement {
    kKeyManagement802_1x,
    kKeyManagementPSK
  };

  // Build a simple WiFiEndpoint, for testing purposes.
  static WiFiEndpoint* MakeEndpoint(ControlInterface* control_interface,
                                    const WiFiRefPtr& wifi,
                                    const std::string& ssid,
                                    const std::string& bssid,
                                    const std::string& network_mode,
                                    uint16_t frequency,
                                    int16_t signal_dbm,
                                    bool has_wpa_property,
                                    bool has_rsn_property);
  // As above, but with the last two parameters false.
  static WiFiEndpoint* MakeOpenEndpoint(ControlInterface* control_interface,
                                        const WiFiRefPtr& wifi,
                                        const std::string& ssid,
                                        const std::string& bssid,
                                        const std::string& network_mode,
                                        uint16_t frequency,
                                        int16_t signal_dbm);
  // Maps mode strings from supplicant into flimflam's nomenclature, as defined
  // in chromeos/dbus/service_constants.h.
  static const char* ParseMode(const std::string& mode_string);
  // Parses an Endpoint's properties to identify an approprirate flimflam
  // security property value, as defined in chromeos/dbus/service_constants.h.
  // The stored data in the |flags| parameter is merged with the provided
  // properties, and the security value returned is the result of the
  // merger.
  static const char* ParseSecurity(const KeyValueStore& properties,
                                   SecurityFlags* flags);
  // Parses an Endpoint's properties' "RSN" or "WPA" sub-dictionary, to
  // identify supported key management methods (802.1x or PSK).
  static void ParseKeyManagementMethods(
      const KeyValueStore& security_method_properties,
      std::set<KeyManagement>* key_management_methods);
  // Determine the negotiated operating mode for the channel by looking at
  // the information elements, frequency and data rates.  The information
  // elements and data rates live in |properties|.
  static Metrics::WiFiNetworkPhyMode DeterminePhyModeFromFrequency(
      const KeyValueStore& properties,
      uint16_t frequency);
  // Parse information elements to determine the physical mode, vendor
  // information and IEEE 802.11w requirement information associated
  // with the AP.  Returns true if a physical mode was determined from
  // the IE elements, false otherwise.
  static bool ParseIEs(const KeyValueStore& properties,
                       Metrics::WiFiNetworkPhyMode* phy_mode,
                       VendorInformation* vendor_information,
                       bool* ieee80211w_required, std::string* country_code);
  // Parse a WPA information element and set *|ieee80211w_required| to true
  // if IEEE 802.11w is required by this AP.
  static void ParseWPACapabilities(std::vector<uint8_t>::const_iterator ie,
                                   std::vector<uint8_t>::const_iterator end,
                                   bool* ieee80211w_required);
  // Parse a single vendor information element.  If this is a WPA vendor
  // element, call ParseWPACapabilites with |ieee80211w_required|.
  static void ParseVendorIE(std::vector<uint8_t>::const_iterator ie,
                            std::vector<uint8_t>::const_iterator end,
                            VendorInformation* vendor_information,
                            bool* ieee80211w_required);

  // Assigns a value to |has_tethering_signature_|.
  void CheckForTetheringSignature();

  // Private setter used in unit tests.
  void set_security_mode(const std::string& mode) { security_mode_ = mode; }

  // TODO(quiche): make const?
  std::vector<uint8_t> ssid_;
  std::vector<uint8_t> bssid_;
  std::string ssid_string_;
  std::string ssid_hex_;
  std::string bssid_string_;
  std::string bssid_hex_;
  std::string country_code_;
  int16_t signal_strength_;
  uint16_t frequency_;
  uint16_t physical_mode_;
  // network_mode_ and security_mode_ are represented as flimflam names
  // (not necessarily the same as wpa_supplicant names)
  std::string network_mode_;
  std::string security_mode_;
  VendorInformation vendor_information_;
  bool ieee80211w_required_;
  bool has_rsn_property_;
  bool has_wpa_property_;
  bool has_tethering_signature_;
  SecurityFlags security_flags_;

  ControlInterface* control_interface_;
  WiFiRefPtr device_;
  std::string rpc_id_;
  std::unique_ptr<SupplicantBSSProxyInterface> supplicant_bss_proxy_;

  DISALLOW_COPY_AND_ASSIGN(WiFiEndpoint);
};

}  // namespace shill

#endif  // SHILL_WIFI_WIFI_ENDPOINT_H_
