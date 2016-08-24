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

#ifndef SHILL_ETHERNET_ETHERNET_H_
#define SHILL_ETHERNET_ETHERNET_H_

#include <map>
#include <memory>
#include <string>

#include <base/cancelable_callback.h>
#include <base/memory/weak_ptr.h>

#include "shill/certificate_file.h"
#include "shill/device.h"
#include "shill/event_dispatcher.h"
#include "shill/refptr_types.h"

#if !defined(DISABLE_WIRED_8021X)
#include "shill/key_value_store.h"
#include "shill/supplicant/supplicant_eap_state_handler.h"
#include "shill/supplicant/supplicant_event_delegate_interface.h"
#endif  // DISABLE_WIRED_8021X

namespace shill {

class Sockets;
class StoreInterface;

#if !defined(DISABLE_WIRED_8021X)
class CertificateFile;
class EapListener;
class EthernetEapProvider;
class SupplicantEAPStateHandler;
class SupplicantInterfaceProxyInterface;
class SupplicantProcessProxyInterface;
#endif  // DISABLE_WIRED_8021X

class Ethernet
#if !defined(DISABLE_WIRED_8021X)
    : public Device, public SupplicantEventDelegateInterface {
#else
    : public Device {
#endif  // DISABLE_WIRED_8021X
 public:
  Ethernet(ControlInterface* control_interface,
           EventDispatcher* dispatcher,
           Metrics* metrics,
           Manager* manager,
           const std::string& link_name,
           const std::string& address,
           int interface_index);
  ~Ethernet() override;

  void Start(Error* error,
             const EnabledStateChangedCallback& callback) override;
  void Stop(Error* error, const EnabledStateChangedCallback& callback) override;
  void LinkEvent(unsigned int flags, unsigned int change) override;
  bool Load(StoreInterface* storage) override;
  bool Save(StoreInterface* storage) override;

  virtual void ConnectTo(EthernetService* service);
  virtual void DisconnectFrom(EthernetService* service);

#if !defined(DISABLE_WIRED_8021X)
  // Test to see if conditions are correct for EAP authentication (both
  // credentials and a remote EAP authenticator is present) and initiate
  // an authentication if possible.
  virtual void TryEapAuthentication();

  // Implementation of SupplicantEventDelegateInterface.  These methods
  // are called by SupplicantInterfaceProxy, in response to events from
  // wpa_supplicant.
  void BSSAdded(
      const std::string& BSS,
      const KeyValueStore& properties) override;
  void BSSRemoved(const std::string& BSS) override;
  void Certification(const KeyValueStore& properties) override;
  void EAPEvent(const std::string& status,
                const std::string& parameter) override;
  void PropertiesChanged(const KeyValueStore& properties) override;
  void ScanDone(const bool& /*success*/) override;
  void TDLSDiscoverResponse(const std::string& peer_address) override;
#endif  // DISABLE_WIRED_8021X

  virtual bool link_up() const { return link_up_; }

 private:
  friend class EthernetTest;
  friend class EthernetServiceTest;  // For weak_ptr_factory_.
  friend class PPPoEServiceTest;     // For weak_ptr_factory_.

#if !defined(DISABLE_WIRED_8021X)
  // Return a pointer to the EAP provider for Ethernet devices.
  EthernetEapProvider* GetEapProvider();

  // Return a reference to the shared service that contains EAP credentials
  // for Ethernet.
  ServiceConstRefPtr GetEapService();

  // Invoked by |eap_listener_| when an EAP authenticator is detected.
  void OnEapDetected();

  // Start and stop a supplicant instance on this link.
  bool StartSupplicant();
  void StopSupplicant();

  // Start the EAP authentication process.
  bool StartEapAuthentication();

  // Change our EAP authentication state.
  void SetIsEapAuthenticated(bool is_eap_authenticated);

  // Callback tasks run as a result of event delegate methods.
  void CertificationTask(const std::string& subject, uint32_t depth);
  void EAPEventTask(const std::string& status, const std::string& parameter);
  void SupplicantStateChangedTask(const std::string& state);

  // Callback task run as a result of TryEapAuthentication().
  void TryEapAuthenticationTask();
#endif  // DISABLE_WIRED_8021X

  // Accessors for the PPoE property.
  bool GetPPPoEMode(Error* error);
  bool ConfigurePPPoEMode(const bool& mode, Error* error);
  void ClearPPPoEMode(Error* error);

  // Helpers for creating services with |this| as their device.
  EthernetServiceRefPtr CreateEthernetService();
  EthernetServiceRefPtr CreatePPPoEService();

  void SetupWakeOnLan();

  ControlInterface* control_interface_;

  EthernetServiceRefPtr service_;
  bool link_up_;

#if !defined(DISABLE_WIRED_8021X)
  // Track whether we have completed EAP authentication successfully.
  bool is_eap_authenticated_;

  // Track whether an EAP authenticator has been detected on this link.
  bool is_eap_detected_;
  std::unique_ptr<EapListener> eap_listener_;

  // Track the progress of EAP authentication.
  SupplicantEAPStateHandler eap_state_handler_;

  // Proxy instances used to talk to wpa_supplicant.
  std::unique_ptr<SupplicantProcessProxyInterface> supplicant_process_proxy_;
  std::unique_ptr<SupplicantInterfaceProxyInterface>
      supplicant_interface_proxy_;
  std::string supplicant_interface_path_;
  std::string supplicant_network_path_;

  // Certificate file instance to generate public key data for remote
  // authentication.
  CertificateFile certificate_file_;

  // Make sure TryEapAuthenticationTask is only queued for execution once
  // at a time.
  base::CancelableClosure try_eap_authentication_callback_;
#endif  // DISABLE_WIRED_8021X

  std::unique_ptr<Sockets> sockets_;

  base::WeakPtrFactory<Ethernet> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(Ethernet);
};

}  // namespace shill

#endif  // SHILL_ETHERNET_ETHERNET_H_
