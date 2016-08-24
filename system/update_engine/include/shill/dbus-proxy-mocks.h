// Automatic generation of D-Bus interface mock proxies for:
//  - org.chromium.flimflam.Manager
//  - org.chromium.flimflam.Service
#ifndef ____CHROMEOS_DBUS_BINDING___UPDATE_ENGINE_INCLUDE_SHILL_DBUS_PROXY_MOCKS_H
#define ____CHROMEOS_DBUS_BINDING___UPDATE_ENGINE_INCLUDE_SHILL_DBUS_PROXY_MOCKS_H
#include <string>
#include <vector>

#include <base/callback_forward.h>
#include <base/logging.h>
#include <base/macros.h>
#include <brillo/any.h>
#include <brillo/errors/error.h>
#include <brillo/variant_dictionary.h>
#include <gmock/gmock.h>

#include "shill/dbus-proxies.h"

namespace org {
namespace chromium {
namespace flimflam {

// Mock object for ManagerProxyInterface.
class ManagerProxyMock : public ManagerProxyInterface {
 public:
  ManagerProxyMock() = default;

  MOCK_METHOD3(GetProperties,
               bool(brillo::VariantDictionary*,
                    brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD3(GetPropertiesAsync,
               void(const base::Callback<void(const brillo::VariantDictionary&)>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD4(SetProperty,
               bool(const std::string&,
                    const brillo::Any&,
                    brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD5(SetPropertyAsync,
               void(const std::string&,
                    const brillo::Any&,
                    const base::Callback<void()>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD3(GetState,
               bool(std::string*,
                    brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD3(GetStateAsync,
               void(const base::Callback<void(const std::string&)>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD4(CreateProfile,
               bool(const std::string&,
                    dbus::ObjectPath*,
                    brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD4(CreateProfileAsync,
               void(const std::string&,
                    const base::Callback<void(const dbus::ObjectPath&)>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD3(RemoveProfile,
               bool(const std::string&,
                    brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD4(RemoveProfileAsync,
               void(const std::string&,
                    const base::Callback<void()>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD4(PushProfile,
               bool(const std::string&,
                    dbus::ObjectPath*,
                    brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD4(PushProfileAsync,
               void(const std::string&,
                    const base::Callback<void(const dbus::ObjectPath&)>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD5(InsertUserProfile,
               bool(const std::string&,
                    const std::string&,
                    dbus::ObjectPath*,
                    brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD5(InsertUserProfileAsync,
               void(const std::string&,
                    const std::string&,
                    const base::Callback<void(const dbus::ObjectPath&)>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD3(PopProfile,
               bool(const std::string&,
                    brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD4(PopProfileAsync,
               void(const std::string&,
                    const base::Callback<void()>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD2(PopAnyProfile,
               bool(brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD3(PopAnyProfileAsync,
               void(const base::Callback<void()>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD2(PopAllUserProfiles,
               bool(brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD3(PopAllUserProfilesAsync,
               void(const base::Callback<void()>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD2(RecheckPortal,
               bool(brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD3(RecheckPortalAsync,
               void(const base::Callback<void()>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD3(RequestScan,
               bool(const std::string&,
                    brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD4(RequestScanAsync,
               void(const std::string&,
                    const base::Callback<void()>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD3(EnableTechnology,
               bool(const std::string&,
                    brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD4(EnableTechnologyAsync,
               void(const std::string&,
                    const base::Callback<void()>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD3(DisableTechnology,
               bool(const std::string&,
                    brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD4(DisableTechnologyAsync,
               void(const std::string&,
                    const base::Callback<void()>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD4(GetService,
               bool(const brillo::VariantDictionary&,
                    dbus::ObjectPath*,
                    brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD4(GetServiceAsync,
               void(const brillo::VariantDictionary&,
                    const base::Callback<void(const dbus::ObjectPath&)>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD4(GetWifiService,
               bool(const brillo::VariantDictionary&,
                    dbus::ObjectPath*,
                    brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD4(GetWifiServiceAsync,
               void(const brillo::VariantDictionary&,
                    const base::Callback<void(const dbus::ObjectPath&)>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD4(ConfigureService,
               bool(const brillo::VariantDictionary&,
                    dbus::ObjectPath*,
                    brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD4(ConfigureServiceAsync,
               void(const brillo::VariantDictionary&,
                    const base::Callback<void(const dbus::ObjectPath&)>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD5(ConfigureServiceForProfile,
               bool(const dbus::ObjectPath&,
                    const brillo::VariantDictionary&,
                    dbus::ObjectPath*,
                    brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD5(ConfigureServiceForProfileAsync,
               void(const dbus::ObjectPath&,
                    const brillo::VariantDictionary&,
                    const base::Callback<void(const dbus::ObjectPath&)>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD4(FindMatchingService,
               bool(const brillo::VariantDictionary&,
                    dbus::ObjectPath*,
                    brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD4(FindMatchingServiceAsync,
               void(const brillo::VariantDictionary&,
                    const base::Callback<void(const dbus::ObjectPath&)>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD4(GetVPNService,
               bool(const brillo::VariantDictionary&,
                    dbus::ObjectPath*,
                    brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD4(GetVPNServiceAsync,
               void(const brillo::VariantDictionary&,
                    const base::Callback<void(const dbus::ObjectPath&)>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD3(GetDebugLevel,
               bool(int32_t*,
                    brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD3(GetDebugLevelAsync,
               void(const base::Callback<void(int32_t)>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD3(SetDebugLevel,
               bool(int32_t,
                    brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD4(SetDebugLevelAsync,
               void(int32_t,
                    const base::Callback<void()>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD3(GetServiceOrder,
               bool(std::string*,
                    brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD3(GetServiceOrderAsync,
               void(const base::Callback<void(const std::string&)>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD3(SetServiceOrder,
               bool(const std::string&,
                    brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD4(SetServiceOrderAsync,
               void(const std::string&,
                    const base::Callback<void()>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD3(GetDebugTags,
               bool(std::string*,
                    brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD3(GetDebugTagsAsync,
               void(const base::Callback<void(const std::string&)>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD3(SetDebugTags,
               bool(const std::string&,
                    brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD4(SetDebugTagsAsync,
               void(const std::string&,
                    const base::Callback<void()>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD3(ListDebugTags,
               bool(std::string*,
                    brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD3(ListDebugTagsAsync,
               void(const base::Callback<void(const std::string&)>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD3(GetNetworksForGeolocation,
               bool(brillo::VariantDictionary*,
                    brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD3(GetNetworksForGeolocationAsync,
               void(const base::Callback<void(const brillo::VariantDictionary&)>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD10(VerifyDestination,
                bool(const std::string& /*in_certificate*/,
                     const std::string& /*in_public_key*/,
                     const std::string& /*in_nonce*/,
                     const std::string& /*in_signed_data*/,
                     const std::string& /*in_destination_udn*/,
                     const std::string& /*in_hotspot_ssid*/,
                     const std::string& /*in_hotspot_bssid*/,
                     bool*,
                     brillo::ErrorPtr* /*error*/,
                     int /*timeout_ms*/));
  MOCK_METHOD10(VerifyDestinationAsync,
                void(const std::string& /*in_certificate*/,
                     const std::string& /*in_public_key*/,
                     const std::string& /*in_nonce*/,
                     const std::string& /*in_signed_data*/,
                     const std::string& /*in_destination_udn*/,
                     const std::string& /*in_hotspot_ssid*/,
                     const std::string& /*in_hotspot_bssid*/,
                     const base::Callback<void(bool)>& /*success_callback*/,
                     const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                     int /*timeout_ms*/));
  bool VerifyAndEncryptCredentials(const std::string& /*in_certificate*/,
                                   const std::string& /*in_public_key*/,
                                   const std::string& /*in_nonce*/,
                                   const std::string& /*in_signed_data*/,
                                   const std::string& /*in_destination_udn*/,
                                   const std::string& /*in_hotspot_ssid*/,
                                   const std::string& /*in_hotspot_bssid*/,
                                   const dbus::ObjectPath& /*in_network*/,
                                   std::string*,
                                   brillo::ErrorPtr* /*error*/,
                                   int /*timeout_ms*/) override {
    LOG(WARNING) << "VerifyAndEncryptCredentials(): gmock can't handle methods with 11 arguments. You can override this method in a subclass if you need to.";
    return false;
  }
  void VerifyAndEncryptCredentialsAsync(const std::string& /*in_certificate*/,
                                        const std::string& /*in_public_key*/,
                                        const std::string& /*in_nonce*/,
                                        const std::string& /*in_signed_data*/,
                                        const std::string& /*in_destination_udn*/,
                                        const std::string& /*in_hotspot_ssid*/,
                                        const std::string& /*in_hotspot_bssid*/,
                                        const dbus::ObjectPath& /*in_network*/,
                                        const base::Callback<void(const std::string&)>& /*success_callback*/,
                                        const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                                        int /*timeout_ms*/) override {
    LOG(WARNING) << "VerifyAndEncryptCredentialsAsync(): gmock can't handle methods with 11 arguments. You can override this method in a subclass if you need to.";
  }
  bool VerifyAndEncryptData(const std::string& /*in_certificate*/,
                            const std::string& /*in_public_key*/,
                            const std::string& /*in_nonce*/,
                            const std::string& /*in_signed_data*/,
                            const std::string& /*in_destination_udn*/,
                            const std::string& /*in_hotspot_ssid*/,
                            const std::string& /*in_hotspot_bssid*/,
                            const std::string& /*in_data*/,
                            std::string*,
                            brillo::ErrorPtr* /*error*/,
                            int /*timeout_ms*/) override {
    LOG(WARNING) << "VerifyAndEncryptData(): gmock can't handle methods with 11 arguments. You can override this method in a subclass if you need to.";
    return false;
  }
  void VerifyAndEncryptDataAsync(const std::string& /*in_certificate*/,
                                 const std::string& /*in_public_key*/,
                                 const std::string& /*in_nonce*/,
                                 const std::string& /*in_signed_data*/,
                                 const std::string& /*in_destination_udn*/,
                                 const std::string& /*in_hotspot_ssid*/,
                                 const std::string& /*in_hotspot_bssid*/,
                                 const std::string& /*in_data*/,
                                 const base::Callback<void(const std::string&)>& /*success_callback*/,
                                 const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                                 int /*timeout_ms*/) override {
    LOG(WARNING) << "VerifyAndEncryptDataAsync(): gmock can't handle methods with 11 arguments. You can override this method in a subclass if you need to.";
  }
  MOCK_METHOD2(ConnectToBestServices,
               bool(brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD3(ConnectToBestServicesAsync,
               void(const base::Callback<void()>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD2(CreateConnectivityReport,
               bool(brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD3(CreateConnectivityReportAsync,
               void(const base::Callback<void()>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD4(ClaimInterface,
               bool(const std::string& /*in_claimer_name*/,
                    const std::string& /*in_interface_name*/,
                    brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD5(ClaimInterfaceAsync,
               void(const std::string& /*in_claimer_name*/,
                    const std::string& /*in_interface_name*/,
                    const base::Callback<void()>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD4(ReleaseInterface,
               bool(const std::string& /*in_claimer_name*/,
                    const std::string& /*in_interface_name*/,
                    brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD5(ReleaseInterfaceAsync,
               void(const std::string& /*in_claimer_name*/,
                    const std::string& /*in_interface_name*/,
                    const base::Callback<void()>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD3(SetSchedScan,
               bool(bool,
                    brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD4(SetSchedScanAsync,
               void(bool,
                    const base::Callback<void()>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD3(SetupApModeInterface,
               bool(std::string* /*out_interface_name*/,
                    brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD3(SetupApModeInterfaceAsync,
               void(const base::Callback<void(const std::string& /*interface_name*/)>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD3(SetupStationModeInterface,
               bool(std::string* /*out_interface_name*/,
                    brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD3(SetupStationModeInterfaceAsync,
               void(const base::Callback<void(const std::string& /*interface_name*/)>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD2(RegisterPropertyChangedSignalHandler,
               void(const base::Callback<void(const std::string&,
                                              const brillo::Any&)>& /*signal_callback*/,
                    dbus::ObjectProxy::OnConnectedCallback /*on_connected_callback*/));
  MOCK_METHOD2(RegisterStateChangedSignalHandler,
               void(const base::Callback<void(const std::string&)>& /*signal_callback*/,
                    dbus::ObjectProxy::OnConnectedCallback /*on_connected_callback*/));
  MOCK_CONST_METHOD0(GetObjectPath, const dbus::ObjectPath&());

 private:
  DISALLOW_COPY_AND_ASSIGN(ManagerProxyMock);
};
}  // namespace flimflam
}  // namespace chromium
}  // namespace org

namespace org {
namespace chromium {
namespace flimflam {

// Mock object for ServiceProxyInterface.
class ServiceProxyMock : public ServiceProxyInterface {
 public:
  ServiceProxyMock() = default;

  MOCK_METHOD3(GetProperties,
               bool(brillo::VariantDictionary*,
                    brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD3(GetPropertiesAsync,
               void(const base::Callback<void(const brillo::VariantDictionary&)>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD4(SetProperty,
               bool(const std::string&,
                    const brillo::Any&,
                    brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD5(SetPropertyAsync,
               void(const std::string&,
                    const brillo::Any&,
                    const base::Callback<void()>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD3(SetProperties,
               bool(const brillo::VariantDictionary&,
                    brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD4(SetPropertiesAsync,
               void(const brillo::VariantDictionary&,
                    const base::Callback<void()>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD3(ClearProperty,
               bool(const std::string&,
                    brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD4(ClearPropertyAsync,
               void(const std::string&,
                    const base::Callback<void()>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD4(ClearProperties,
               bool(const std::vector<std::string>&,
                    std::vector<bool>*,
                    brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD4(ClearPropertiesAsync,
               void(const std::vector<std::string>&,
                    const base::Callback<void(const std::vector<bool>&)>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD2(Connect,
               bool(brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD3(ConnectAsync,
               void(const base::Callback<void()>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD2(Disconnect,
               bool(brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD3(DisconnectAsync,
               void(const base::Callback<void()>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD2(Remove,
               bool(brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD3(RemoveAsync,
               void(const base::Callback<void()>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD3(ActivateCellularModem,
               bool(const std::string&,
                    brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD4(ActivateCellularModemAsync,
               void(const std::string&,
                    const base::Callback<void()>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD2(CompleteCellularActivation,
               bool(brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD3(CompleteCellularActivationAsync,
               void(const base::Callback<void()>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD3(GetLoadableProfileEntries,
               bool(std::map<dbus::ObjectPath, std::string>*,
                    brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD3(GetLoadableProfileEntriesAsync,
               void(const base::Callback<void(const std::map<dbus::ObjectPath, std::string>&)>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD2(RegisterPropertyChangedSignalHandler,
               void(const base::Callback<void(const std::string&,
                                              const brillo::Any&)>& /*signal_callback*/,
                    dbus::ObjectProxy::OnConnectedCallback /*on_connected_callback*/));
  MOCK_CONST_METHOD0(GetObjectPath, const dbus::ObjectPath&());

 private:
  DISALLOW_COPY_AND_ASSIGN(ServiceProxyMock);
};
}  // namespace flimflam
}  // namespace chromium
}  // namespace org

#endif  // ____CHROMEOS_DBUS_BINDING___UPDATE_ENGINE_INCLUDE_SHILL_DBUS_PROXY_MOCKS_H
