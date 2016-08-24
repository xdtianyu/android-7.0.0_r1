//
// Copyright (C) 2015 The Android Open Source Project
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

#include "shill/dbus/chromeos_supplicant_interface_proxy.h"

#include <string>

#include <base/bind.h>

#include "shill/logging.h"
#include "shill/supplicant/supplicant_event_delegate_interface.h"
#include "shill/supplicant/wpa_supplicant.h"

using std::string;

namespace shill {

namespace Logging {
static auto kModuleLogScope = ScopeLogger::kDBus;
static string ObjectID(const dbus::ObjectPath* p) { return p->value(); }
}

const char ChromeosSupplicantInterfaceProxy::kInterfaceName[] =
    "fi.w1.wpa_supplicant1.Interface";
const char ChromeosSupplicantInterfaceProxy::kPropertyDisableHighBitrates[] =
    "DisableHighBitrates";
const char ChromeosSupplicantInterfaceProxy::kPropertyFastReauth[] =
    "FastReauth";
const char ChromeosSupplicantInterfaceProxy::kPropertyRoamThreshold[] =
    "RoamThreshold";
const char ChromeosSupplicantInterfaceProxy::kPropertyScan[] = "Scan";
const char ChromeosSupplicantInterfaceProxy::kPropertyScanInterval[] =
    "ScanInterval";
const char ChromeosSupplicantInterfaceProxy::kPropertySchedScan[] = "SchedScan";

ChromeosSupplicantInterfaceProxy::PropertySet::PropertySet(
    dbus::ObjectProxy* object_proxy,
    const std::string& interface_name,
    const PropertyChangedCallback& callback)
    : dbus::PropertySet(object_proxy, interface_name, callback) {
  RegisterProperty(kPropertyDisableHighBitrates, &disable_high_bitrates);
  RegisterProperty(kPropertyFastReauth, &fast_reauth);
  RegisterProperty(kPropertyRoamThreshold, &roam_threshold);
  RegisterProperty(kPropertyScan, &scan);
  RegisterProperty(kPropertyScanInterval, &scan_interval);
  RegisterProperty(kPropertySchedScan, &sched_scan);
}

ChromeosSupplicantInterfaceProxy::ChromeosSupplicantInterfaceProxy(
    const scoped_refptr<dbus::Bus>& bus,
    const std::string& object_path,
    SupplicantEventDelegateInterface* delegate)
    : interface_proxy_(
          new fi::w1::wpa_supplicant1::InterfaceProxy(
              bus,
              WPASupplicant::kDBusAddr,
              dbus::ObjectPath(object_path))),
      delegate_(delegate) {
  // Register properites.
  properties_.reset(
      new PropertySet(
          interface_proxy_->GetObjectProxy(),
          kInterfaceName,
          base::Bind(&ChromeosSupplicantInterfaceProxy::OnPropertyChanged,
                     weak_factory_.GetWeakPtr())));

  // Register signal handlers.
  dbus::ObjectProxy::OnConnectedCallback on_connected_callback =
      base::Bind(&ChromeosSupplicantInterfaceProxy::OnSignalConnected,
                 weak_factory_.GetWeakPtr());
  interface_proxy_->RegisterScanDoneSignalHandler(
      base::Bind(&ChromeosSupplicantInterfaceProxy::ScanDone,
                 weak_factory_.GetWeakPtr()),
      on_connected_callback);
  interface_proxy_->RegisterBSSAddedSignalHandler(
      base::Bind(&ChromeosSupplicantInterfaceProxy::BSSAdded,
                 weak_factory_.GetWeakPtr()),
      on_connected_callback);
  interface_proxy_->RegisterBSSRemovedSignalHandler(
      base::Bind(&ChromeosSupplicantInterfaceProxy::BSSRemoved,
                 weak_factory_.GetWeakPtr()),
      on_connected_callback);
  interface_proxy_->RegisterBlobAddedSignalHandler(
      base::Bind(&ChromeosSupplicantInterfaceProxy::BlobAdded,
                 weak_factory_.GetWeakPtr()),
      on_connected_callback);
  interface_proxy_->RegisterBlobRemovedSignalHandler(
      base::Bind(&ChromeosSupplicantInterfaceProxy::BlobRemoved,
                 weak_factory_.GetWeakPtr()),
      on_connected_callback);
  interface_proxy_->RegisterCertificationSignalHandler(
      base::Bind(&ChromeosSupplicantInterfaceProxy::Certification,
                 weak_factory_.GetWeakPtr()),
      on_connected_callback);
  interface_proxy_->RegisterEAPSignalHandler(
      base::Bind(&ChromeosSupplicantInterfaceProxy::EAP,
                 weak_factory_.GetWeakPtr()),
      on_connected_callback);
  interface_proxy_->RegisterNetworkAddedSignalHandler(
      base::Bind(&ChromeosSupplicantInterfaceProxy::NetworkAdded,
                 weak_factory_.GetWeakPtr()),
      on_connected_callback);
  interface_proxy_->RegisterNetworkRemovedSignalHandler(
      base::Bind(&ChromeosSupplicantInterfaceProxy::NetworkRemoved,
                 weak_factory_.GetWeakPtr()),
      on_connected_callback);
  interface_proxy_->RegisterNetworkSelectedSignalHandler(
      base::Bind(&ChromeosSupplicantInterfaceProxy::NetworkSelected,
                 weak_factory_.GetWeakPtr()),
      on_connected_callback);
  interface_proxy_->RegisterPropertiesChangedSignalHandler(
      base::Bind(&ChromeosSupplicantInterfaceProxy::PropertiesChanged,
                 weak_factory_.GetWeakPtr()),
      on_connected_callback);
  interface_proxy_->RegisterTDLSDiscoverResponseSignalHandler(
      base::Bind(&ChromeosSupplicantInterfaceProxy::TDLSDiscoverResponse,
                 weak_factory_.GetWeakPtr()),
      on_connected_callback);

  // Connect property signals and initialize cached values. Based on
  // recommendations from src/dbus/property.h.
  properties_->ConnectSignals();
  properties_->GetAll();
}

ChromeosSupplicantInterfaceProxy::~ChromeosSupplicantInterfaceProxy() {
  interface_proxy_->ReleaseObjectProxy(base::Bind(&base::DoNothing));
}

bool ChromeosSupplicantInterfaceProxy::AddNetwork(const KeyValueStore& args,
                                                  string* network) {
  SLOG(&interface_proxy_->GetObjectPath(), 2) << __func__;
  brillo::VariantDictionary dict;
  KeyValueStore::ConvertToVariantDictionary(args, &dict);
  dbus::ObjectPath path;
  brillo::ErrorPtr error;
  if (!interface_proxy_->AddNetwork(dict, &path, &error)) {
    LOG(ERROR) << "Failed to add network: "
               << error->GetCode() << " " << error->GetMessage();
    return false;
  }
  *network = path.value();
  return true;
}

bool ChromeosSupplicantInterfaceProxy::EnableHighBitrates() {
  SLOG(&interface_proxy_->GetObjectPath(), 2) << __func__;
#if !defined(__ANDROID__)
  brillo::ErrorPtr error;
  if (!interface_proxy_->EnableHighBitrates(&error)) {
    LOG(ERROR) << "Failed to enable high bitrates: "
               << error->GetCode() << " " << error->GetMessage();
    return false;
  }
#endif  // __ANDROID__
  return true;
}

bool ChromeosSupplicantInterfaceProxy::EAPLogoff() {
  SLOG(&interface_proxy_->GetObjectPath(), 2) << __func__;
  brillo::ErrorPtr error;
  if (!interface_proxy_->EAPLogoff(&error)) {
    LOG(ERROR) << "Failed to EPA logoff "
               << error->GetCode() << " " << error->GetMessage();
    return false;
  }
  return true;
}

bool ChromeosSupplicantInterfaceProxy::EAPLogon() {
  SLOG(&interface_proxy_->GetObjectPath(), 2) << __func__;
  brillo::ErrorPtr error;
  if (!interface_proxy_->EAPLogon(&error)) {
    LOG(ERROR) << "Failed to EAP logon: "
               << error->GetCode() << " " << error->GetMessage();
    return false;
  }
  return true;
}

bool ChromeosSupplicantInterfaceProxy::Disconnect() {
  SLOG(&interface_proxy_->GetObjectPath(), 2) << __func__;
  brillo::ErrorPtr error;
  if (!interface_proxy_->Disconnect(&error)) {
    LOG(ERROR) << "Failed to disconnect: "
               << error->GetCode() << " " << error->GetMessage();
    return false;
  }
  return true;
}

bool ChromeosSupplicantInterfaceProxy::FlushBSS(const uint32_t& age) {
  SLOG(&interface_proxy_->GetObjectPath(), 2) << __func__;
  brillo::ErrorPtr error;
  if (!interface_proxy_->FlushBSS(age, &error)) {
    LOG(ERROR) << "Failed to flush BSS: "
               << error->GetCode() << " " << error->GetMessage();
    return false;
  }
  return true;
}

bool ChromeosSupplicantInterfaceProxy::NetworkReply(const string& network,
                                                    const string& field,
                                                    const string& value) {
  SLOG(&interface_proxy_->GetObjectPath(), 2) << __func__
      << " network: " << network << " field: " << field << " value: " << value;
  brillo::ErrorPtr error;
  if (!interface_proxy_->NetworkReply(dbus::ObjectPath(network),
                                      field,
                                      value,
                                      &error)) {
    LOG(ERROR) << "Failed to network reply: "
               << error->GetCode() << " " << error->GetMessage();
    return false;
  }
  return true;
}

bool ChromeosSupplicantInterfaceProxy::Roam(const string& addr) {
  SLOG(&interface_proxy_->GetObjectPath(), 2) << __func__;
#if !defined(__ANDROID__)
  brillo::ErrorPtr error;
  if (!interface_proxy_->Roam(addr, &error)) {
    LOG(ERROR) << "Failed to Roam: "
               << error->GetCode() << " " << error->GetMessage();
    return false;
  }
#endif  // __ANDROID__
  return true;
}

bool ChromeosSupplicantInterfaceProxy::Reassociate() {
  SLOG(&interface_proxy_->GetObjectPath(), 2) << __func__;
  brillo::ErrorPtr error;
  if (!interface_proxy_->Reassociate(&error)) {
    LOG(ERROR) << "Failed to reassociate: "
               << error->GetCode() << " " << error->GetMessage();
    return false;
  }
  return true;
}

bool ChromeosSupplicantInterfaceProxy::Reattach() {
  SLOG(&interface_proxy_->GetObjectPath(), 2) << __func__;
  brillo::ErrorPtr error;
  if (!interface_proxy_->Reattach(&error)) {
    LOG(ERROR) << "Failed to reattach: "
               << error->GetCode() << " " << error->GetMessage();
    return false;
  }
  return true;
}

bool ChromeosSupplicantInterfaceProxy::RemoveAllNetworks() {
  SLOG(&interface_proxy_->GetObjectPath(), 2) << __func__;
  brillo::ErrorPtr error;
  if (!interface_proxy_->RemoveAllNetworks(&error)) {
    LOG(ERROR) << "Failed to remove all networks: "
               << error->GetCode() << " " << error->GetMessage();
    return false;
  }
  return true;
}

bool ChromeosSupplicantInterfaceProxy::RemoveNetwork(const string& network) {
  SLOG(&interface_proxy_->GetObjectPath(), 2) << __func__ << ": " << network;
  brillo::ErrorPtr error;
  if (!interface_proxy_->RemoveNetwork(dbus::ObjectPath(network),
                                       &error)) {
    LOG(ERROR) << "Failed to remove network: "
               << error->GetCode() << " " << error->GetMessage();
    // RemoveNetwork can fail with three different errors.
    //
    // If RemoveNetwork fails with a NetworkUnknown error, supplicant has
    // already removed the network object, so return true as if
    // RemoveNetwork removes the network object successfully.
    //
    // As shill always passes a valid network object path, RemoveNetwork
    // should not fail with an InvalidArgs error. Return false in such case
    // as something weird may have happened. Similarly, return false in case
    // of an UnknownError.
    if (error->GetCode() != WPASupplicant::kErrorNetworkUnknown) {
      return false;
    }
  }
  return true;
}

bool ChromeosSupplicantInterfaceProxy::Scan(const KeyValueStore& args) {
  SLOG(&interface_proxy_->GetObjectPath(), 2) << __func__;
  brillo::VariantDictionary dict;
  KeyValueStore::ConvertToVariantDictionary(args, &dict);
  brillo::ErrorPtr error;
  if (!interface_proxy_->Scan(dict, &error)) {
    LOG(ERROR) << "Failed to scan: "
               << error->GetCode() << " " << error->GetMessage();
    return false;
  }
  return true;
}

bool ChromeosSupplicantInterfaceProxy::SelectNetwork(const string& network) {
  SLOG(&interface_proxy_->GetObjectPath(), 2) << __func__ << ": " << network;
  brillo::ErrorPtr error;
  if (!interface_proxy_->SelectNetwork(dbus::ObjectPath(network), &error)) {
    LOG(ERROR) << "Failed to select network: "
               << error->GetCode() << " " << error->GetMessage();
    return false;
  }
  return true;
}

bool ChromeosSupplicantInterfaceProxy::SetHT40Enable(const string& network,
                                                     bool enable) {
  SLOG(&interface_proxy_->GetObjectPath(), 2) << __func__
      << " network: " << network << " enable: " << enable;
#if defined(__ANDROID__)
  brillo::ErrorPtr error;
  if (!interface_proxy_->SetHT40Enable(dbus::ObjectPath(network),
                                       enable,
                                       &error)) {
    LOG(ERROR) << "Failed to set HT40 enable: "
               << error->GetCode() << " " << error->GetMessage();
    return false;
  }
#endif  // __ANDROID__
  return true;
}

bool ChromeosSupplicantInterfaceProxy::TDLSDiscover(const string& peer) {
  SLOG(&interface_proxy_->GetObjectPath(), 2) << __func__ << ": " << peer;
  brillo::ErrorPtr error;
  if (!interface_proxy_->TDLSDiscover(peer, &error)) {
    LOG(ERROR) << "Failed to perform TDLS discover: "
               << error->GetCode() << " " << error->GetMessage();
    return false;
  }
  return true;
}

bool ChromeosSupplicantInterfaceProxy::TDLSSetup(const string& peer) {
  SLOG(&interface_proxy_->GetObjectPath(), 2) << __func__ << ": " << peer;
  brillo::ErrorPtr error;
  if (!interface_proxy_->TDLSSetup(peer, &error)) {
    LOG(ERROR) << "Failed to perform TDLS setup: "
               << error->GetCode() << " " << error->GetMessage();
    return false;
  }
  return true;
}

bool ChromeosSupplicantInterfaceProxy::TDLSStatus(const string& peer,
                                                  string* status) {
  SLOG(&interface_proxy_->GetObjectPath(), 2) << __func__ << ": " << peer;
  brillo::ErrorPtr error;
  if (!interface_proxy_->TDLSStatus(peer, status, &error)) {
    LOG(ERROR) << "Failed to retrieve TDLS status: "
               << error->GetCode() << " " << error->GetMessage();
    return false;
  }
  return true;
}

bool ChromeosSupplicantInterfaceProxy::TDLSTeardown(const string& peer) {
  SLOG(&interface_proxy_->GetObjectPath(), 2) << __func__ << ": " << peer;
  brillo::ErrorPtr error;
  if (!interface_proxy_->TDLSTeardown(peer, &error)) {
    LOG(ERROR) << "Failed to perform TDLS teardown: "
               << error->GetCode() << " " << error->GetMessage();
    return false;
  }
  return true;
}

bool ChromeosSupplicantInterfaceProxy::SetFastReauth(bool enabled) {
  SLOG(&interface_proxy_->GetObjectPath(), 2) << __func__ << ": " << enabled;
#if !defined(__ANDROID__)
  if (!properties_->fast_reauth.SetAndBlock(enabled)) {
    LOG(ERROR) << __func__ << " failed: " << enabled;
    return false;
  }
#endif  // __ANDROID__
  return true;
}

bool ChromeosSupplicantInterfaceProxy::SetRoamThreshold(uint16_t threshold) {
  SLOG(&interface_proxy_->GetObjectPath(), 2) << __func__ << ": " << threshold;
#if !defined(__ANDROID__)
  if (!properties_->roam_threshold.SetAndBlock(threshold)) {
    LOG(ERROR) << __func__ << " failed: " << threshold;
    return false;
  }
#endif  // __ANDROID__
  return true;
}

bool ChromeosSupplicantInterfaceProxy::SetScanInterval(int32_t scan_interval) {
  SLOG(&interface_proxy_->GetObjectPath(), 2) << __func__ << ": "
      << scan_interval;
#if !defined(__ANDROID__)
  if (!properties_->scan_interval.SetAndBlock(scan_interval)) {
    LOG(ERROR) << __func__ << " failed: " << scan_interval;
    return false;
  }
#endif  // __ANDROID__
  return true;
}

bool ChromeosSupplicantInterfaceProxy::SetDisableHighBitrates(
    bool disable_high_bitrates) {
  SLOG(&interface_proxy_->GetObjectPath(), 2) << __func__ << ": "
      << disable_high_bitrates;
#if !defined(__ANDROID__)
  if (!properties_->disable_high_bitrates.SetAndBlock(disable_high_bitrates)) {
    LOG(ERROR) << __func__ << " failed: " << disable_high_bitrates;
    return false;
  }
#endif  // __ANDROID__
  return true;
}

bool ChromeosSupplicantInterfaceProxy::SetSchedScan(bool enable) {
  SLOG(&interface_proxy_->GetObjectPath(), 2) << __func__ << ": " << enable;
#if !defined(__ANDROID__)
  if (!properties_->sched_scan.SetAndBlock(enable)) {
    LOG(ERROR) << __func__ << " failed: " << enable;
    return false;
  }
#endif  // __ANDROID__
  return true;
}

bool ChromeosSupplicantInterfaceProxy::SetScan(bool enable) {
  SLOG(&interface_proxy_->GetObjectPath(), 2) << __func__ << ": " << enable;
#if !defined(__ANDROID__)
  if (!properties_->scan.SetAndBlock(enable)) {
    LOG(ERROR) << __func__ << " failed: " << enable;
    return false;
  }
#endif  // __ANDROID__
  return true;
}

void ChromeosSupplicantInterfaceProxy::BlobAdded(const string& /*blobname*/) {
  SLOG(&interface_proxy_->GetObjectPath(), 2) << __func__;
  // XXX
}

void ChromeosSupplicantInterfaceProxy::BlobRemoved(const string& /*blobname*/) {
  SLOG(&interface_proxy_->GetObjectPath(), 2) << __func__;
  // XXX
}

void ChromeosSupplicantInterfaceProxy::BSSAdded(
    const dbus::ObjectPath& BSS,
    const brillo::VariantDictionary& properties) {
  SLOG(&interface_proxy_->GetObjectPath(), 2) << __func__;
  KeyValueStore store;
  KeyValueStore::ConvertFromVariantDictionary(properties, &store);
  delegate_->BSSAdded(BSS.value(), store);
}

void ChromeosSupplicantInterfaceProxy::Certification(
    const brillo::VariantDictionary& properties) {
  SLOG(&interface_proxy_->GetObjectPath(), 2) << __func__;
  KeyValueStore store;
  KeyValueStore::ConvertFromVariantDictionary(properties, &store);
  delegate_->Certification(store);
}

void ChromeosSupplicantInterfaceProxy::EAP(
    const string& status, const string& parameter) {
  SLOG(&interface_proxy_->GetObjectPath(), 2) << __func__ << ": status "
      << status << ", parameter " << parameter;
  delegate_->EAPEvent(status, parameter);
}

void ChromeosSupplicantInterfaceProxy::BSSRemoved(const dbus::ObjectPath& BSS) {
  SLOG(&interface_proxy_->GetObjectPath(), 2) << __func__;
  delegate_->BSSRemoved(BSS.value());
}

void ChromeosSupplicantInterfaceProxy::NetworkAdded(
    const dbus::ObjectPath& /*network*/,
    const brillo::VariantDictionary& /*properties*/) {
  SLOG(&interface_proxy_->GetObjectPath(), 2) << __func__;
  // XXX
}

void ChromeosSupplicantInterfaceProxy::NetworkRemoved(
    const dbus::ObjectPath& /*network*/) {
  SLOG(&interface_proxy_->GetObjectPath(), 2) << __func__;
  // TODO(quiche): Pass this up to the delegate, so that it can clean its
  // rpcid_by_service_ map. crbug.com/207648
}

void ChromeosSupplicantInterfaceProxy::NetworkSelected(
    const dbus::ObjectPath& /*network*/) {
  SLOG(&interface_proxy_->GetObjectPath(), 2) << __func__;
  // XXX
}

void ChromeosSupplicantInterfaceProxy::PropertiesChanged(
    const brillo::VariantDictionary& properties) {
  SLOG(&interface_proxy_->GetObjectPath(), 2) << __func__;
  KeyValueStore store;
  KeyValueStore::ConvertFromVariantDictionary(properties, &store);
  delegate_->PropertiesChanged(store);
}

void ChromeosSupplicantInterfaceProxy::ScanDone(bool success) {
  SLOG(&interface_proxy_->GetObjectPath(), 2) << __func__ << ": " << success;
  delegate_->ScanDone(success);
}

void ChromeosSupplicantInterfaceProxy::TDLSDiscoverResponse(
    const std::string& peer_address) {
  SLOG(&interface_proxy_->GetObjectPath(), 2) << __func__ << ": "
      << peer_address;
  delegate_->TDLSDiscoverResponse(peer_address);
}

void ChromeosSupplicantInterfaceProxy::OnPropertyChanged(
    const std::string& property_name) {
  SLOG(&interface_proxy_->GetObjectPath(), 2) << __func__ << ": "
      << property_name;
}

void ChromeosSupplicantInterfaceProxy::OnSignalConnected(
    const string& interface_name, const string& signal_name, bool success) {
  SLOG(&interface_proxy_->GetObjectPath(), 2) << __func__
      << "interface: " << interface_name << " signal: " << signal_name
      << "success: " << success;
  if (!success) {
    LOG(ERROR) << "Failed to connect signal " << signal_name
        << " to interface " << interface_name;
  }
}

}  // namespace shill
