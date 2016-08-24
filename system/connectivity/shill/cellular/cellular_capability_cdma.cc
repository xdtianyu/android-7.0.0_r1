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

#include "shill/cellular/cellular_capability_cdma.h"

#include <string>
#include <vector>

#include <base/bind.h>
#include <base/strings/stringprintf.h>
#include <base/strings/string_util.h>
#if defined(__ANDROID__)
#include <dbus/service_constants.h>
#else
#include <chromeos/dbus/service_constants.h>
#endif  // __ANDROID__
#include <mm/mm-modem.h>

#include "shill/cellular/cellular.h"
#include "shill/cellular/cellular_service.h"
#include "shill/control_interface.h"
#include "shill/logging.h"

using base::Bind;
using std::string;
using std::vector;

namespace shill {

namespace Logging {
static auto kModuleLogScope = ScopeLogger::kCellular;
static string ObjectID(CellularCapabilityCDMA* c) {
  return c->cellular()->GetRpcIdentifier();
}
}

// static
const char CellularCapabilityCDMA::kPhoneNumber[] = "#777";

CellularCapabilityCDMA::CellularCapabilityCDMA(
    Cellular* cellular,
    ControlInterface* control_interface,
    ModemInfo* modem_info)
    : CellularCapabilityClassic(cellular, control_interface, modem_info),
      weak_ptr_factory_(this),
      activation_starting_(false),
      activation_state_(MM_MODEM_CDMA_ACTIVATION_STATE_NOT_ACTIVATED),
      registration_state_evdo_(MM_MODEM_CDMA_REGISTRATION_STATE_UNKNOWN),
      registration_state_1x_(MM_MODEM_CDMA_REGISTRATION_STATE_UNKNOWN) {
  SLOG(this, 2) << "Cellular capability constructed: CDMA";
}

CellularCapabilityCDMA::~CellularCapabilityCDMA() {}

void CellularCapabilityCDMA::InitProxies() {
  CellularCapabilityClassic::InitProxies();
  proxy_.reset(control_interface()->CreateModemCDMAProxy(
      cellular()->dbus_path(), cellular()->dbus_service()));
  proxy_->set_signal_quality_callback(
      Bind(&CellularCapabilityCDMA::OnSignalQualitySignal,
           weak_ptr_factory_.GetWeakPtr()));
  proxy_->set_activation_state_callback(
      Bind(&CellularCapabilityCDMA::OnActivationStateChangedSignal,
           weak_ptr_factory_.GetWeakPtr()));
  proxy_->set_registration_state_callback(
      Bind(&CellularCapabilityCDMA::OnRegistrationStateChangedSignal,
           weak_ptr_factory_.GetWeakPtr()));
}

string CellularCapabilityCDMA::GetTypeString() const {
  return kTechnologyFamilyCdma;
}

void CellularCapabilityCDMA::StartModem(Error* error,
                                        const ResultCallback& callback) {
  SLOG(this, 2) << __func__;
  InitProxies();

  CellularTaskList* tasks = new CellularTaskList();
  ResultCallback cb =
      Bind(&CellularCapabilityCDMA::StepCompletedCallback,
           weak_ptr_factory_.GetWeakPtr(), callback, false, tasks);
  if (!cellular()->IsUnderlyingDeviceEnabled())
    tasks->push_back(Bind(&CellularCapabilityCDMA::EnableModem,
                          weak_ptr_factory_.GetWeakPtr(), cb));
  tasks->push_back(Bind(&CellularCapabilityCDMA::GetModemStatus,
                        weak_ptr_factory_.GetWeakPtr(), cb));
  tasks->push_back(Bind(&CellularCapabilityCDMA::GetMEID,
                        weak_ptr_factory_.GetWeakPtr(), cb));
  tasks->push_back(Bind(&CellularCapabilityCDMA::GetModemInfo,
                        weak_ptr_factory_.GetWeakPtr(), cb));
  tasks->push_back(Bind(&CellularCapabilityCDMA::FinishEnable,
                        weak_ptr_factory_.GetWeakPtr(), cb));

  RunNextStep(tasks);
}

void CellularCapabilityCDMA::ReleaseProxies() {
  CellularCapabilityClassic::ReleaseProxies();
  proxy_.reset();
}

bool CellularCapabilityCDMA::AreProxiesInitialized() const {
  return (CellularCapabilityClassic::AreProxiesInitialized() && proxy_.get());
}

bool CellularCapabilityCDMA::AllowRoaming() {
  return allow_roaming_property();
}


void CellularCapabilityCDMA::OnServiceCreated() {
  SLOG(this, 2) << __func__;
  cellular()->service()->SetUsageURL(usage_url_);
  cellular()->service()->SetActivationType(
      CellularService::kActivationTypeOTASP);
  HandleNewActivationState(MM_MODEM_CDMA_ACTIVATION_ERROR_NO_ERROR);
}

void CellularCapabilityCDMA::UpdateStatus(const KeyValueStore& properties) {
  string carrier;
  if (properties.ContainsUint("activation_state")) {
    activation_state_ = properties.GetUint("activation_state");
  }
  // TODO(petkov): For now, get the payment and usage URLs from ModemManager to
  // match flimflam. In the future, get these from an alternative source (e.g.,
  // database, carrier-specific properties, etc.).
  UpdateOnlinePortal(properties);
  if (properties.ContainsUint("prl_version"))
    cellular()->set_prl_version(properties.GetUint("prl_version"));
}

void CellularCapabilityCDMA::UpdateServiceOLP() {
  SLOG(this, 3) << __func__;
  // All OLP changes are routed up to the Home Provider.
  if (!cellular()->home_provider_info()->IsMobileNetworkOperatorKnown()) {
    return;
  }

  const vector<MobileOperatorInfo::OnlinePortal>& olp_list =
      cellular()->home_provider_info()->olp_list();
  if (olp_list.empty()) {
    return;
  }

  if (olp_list.size() > 1) {
    SLOG(this, 1) << "Found multiple online portals. Choosing the first.";
  }
  cellular()->service()->SetOLP(olp_list[0].url,
                                olp_list[0].method,
                                olp_list[0].post_data);
}

void CellularCapabilityCDMA::SetupConnectProperties(
    KeyValueStore* properties) {
  properties->SetString(kConnectPropertyPhoneNumber, kPhoneNumber);
}

void CellularCapabilityCDMA::Activate(const string& carrier,
                                      Error* error,
                                      const ResultCallback& callback) {
  SLOG(this, 2) << __func__ << "(" << carrier << ")";
  // We're going to trigger something which leads to an activation.
  activation_starting_ = true;
  if (cellular()->state() == Cellular::kStateEnabled ||
      cellular()->state() == Cellular::kStateRegistered) {
    ActivationResultCallback activation_callback =
        Bind(&CellularCapabilityCDMA::OnActivateReply,
             weak_ptr_factory_.GetWeakPtr(),
             callback);
    proxy_->Activate(carrier, error, activation_callback, kTimeoutActivate);
  } else if (cellular()->state() == Cellular::kStateConnected ||
             cellular()->state() == Cellular::kStateLinked) {
    pending_activation_callback_ = callback;
    pending_activation_carrier_ = carrier;
    cellular()->Disconnect(error, __func__);
  } else {
    Error::PopulateAndLog(FROM_HERE, error, Error::kInvalidArguments,
                          "Unable to activate in " +
                          Cellular::GetStateString(cellular()->state()));
    activation_starting_ = false;
  }
}

void CellularCapabilityCDMA::HandleNewActivationState(uint32_t error) {
  SLOG(this, 2) << __func__ << "(" << error << ")";
  if (!cellular()->service().get()) {
    LOG(ERROR) << "In " << __func__ << "(): service is null.";
    return;
  }
  cellular()->service()->SetActivationState(
      GetActivationStateString(activation_state_));
  cellular()->service()->set_error(GetActivationErrorString(error));
}

void CellularCapabilityCDMA::DisconnectCleanup() {
  CellularCapabilityClassic::DisconnectCleanup();
  if (pending_activation_callback_.is_null()) {
    return;
  }
  if (cellular()->state() == Cellular::kStateEnabled ||
      cellular()->state() == Cellular::kStateRegistered) {
    Error ignored_error;
    Activate(pending_activation_carrier_,
             &ignored_error,
             pending_activation_callback_);
  } else {
    Error error;
    Error::PopulateAndLog(
        FROM_HERE,
        &error,
        Error::kOperationFailed,
        "Tried to disconnect before activating cellular service and failed");
    HandleNewActivationState(MM_MODEM_CDMA_ACTIVATION_ERROR_UNKNOWN);
    activation_starting_ = false;
    pending_activation_callback_.Run(error);
  }
  pending_activation_callback_.Reset();
  pending_activation_carrier_.clear();
}

// static
string CellularCapabilityCDMA::GetActivationStateString(uint32_t state) {
  switch (state) {
    case MM_MODEM_CDMA_ACTIVATION_STATE_ACTIVATED:
      return kActivationStateActivated;
    case MM_MODEM_CDMA_ACTIVATION_STATE_ACTIVATING:
      return kActivationStateActivating;
    case MM_MODEM_CDMA_ACTIVATION_STATE_NOT_ACTIVATED:
      return kActivationStateNotActivated;
    case MM_MODEM_CDMA_ACTIVATION_STATE_PARTIALLY_ACTIVATED:
      return kActivationStatePartiallyActivated;
    default:
      return kActivationStateUnknown;
  }
}

// static
string CellularCapabilityCDMA::GetActivationErrorString(uint32_t error) {
  switch (error) {
    case MM_MODEM_CDMA_ACTIVATION_ERROR_WRONG_RADIO_INTERFACE:
      return kErrorNeedEvdo;
    case MM_MODEM_CDMA_ACTIVATION_ERROR_ROAMING:
      return kErrorNeedHomeNetwork;
    case MM_MODEM_CDMA_ACTIVATION_ERROR_COULD_NOT_CONNECT:
    case MM_MODEM_CDMA_ACTIVATION_ERROR_SECURITY_AUTHENTICATION_FAILED:
    case MM_MODEM_CDMA_ACTIVATION_ERROR_PROVISIONING_FAILED:
      return kErrorOtaspFailed;
    case MM_MODEM_CDMA_ACTIVATION_ERROR_NO_ERROR:
      return "";
    case MM_MODEM_CDMA_ACTIVATION_ERROR_NO_SIGNAL:
    default:
      return kErrorActivationFailed;
  }
}

void CellularCapabilityCDMA::GetMEID(const ResultCallback& callback) {
  SLOG(this, 2) << __func__;
  if (cellular()->meid().empty()) {
    // TODO(petkov): Switch to asynchronous calls (crbug.com/200687).
    cellular()->set_meid(proxy_->MEID());
    SLOG(this, 2) << "MEID: " << cellular()->meid();
  }
  callback.Run(Error());
}

void CellularCapabilityCDMA::GetProperties(const ResultCallback& callback) {
  SLOG(this, 2) << __func__;
  // No properties.
  callback.Run(Error());
}

bool CellularCapabilityCDMA::IsActivating() const {
  return activation_starting_ ||
      activation_state_ == MM_MODEM_CDMA_ACTIVATION_STATE_ACTIVATING;
}

bool CellularCapabilityCDMA::IsRegistered() const {
  return registration_state_evdo_ != MM_MODEM_CDMA_REGISTRATION_STATE_UNKNOWN ||
      registration_state_1x_ != MM_MODEM_CDMA_REGISTRATION_STATE_UNKNOWN;
}

void CellularCapabilityCDMA::SetUnregistered(bool searching) {
  registration_state_evdo_ = MM_MODEM_CDMA_REGISTRATION_STATE_UNKNOWN;
  registration_state_1x_ = MM_MODEM_CDMA_REGISTRATION_STATE_UNKNOWN;
}

string CellularCapabilityCDMA::GetNetworkTechnologyString() const {
  if (registration_state_evdo_ != MM_MODEM_CDMA_REGISTRATION_STATE_UNKNOWN) {
    return kNetworkTechnologyEvdo;
  }
  if (registration_state_1x_ != MM_MODEM_CDMA_REGISTRATION_STATE_UNKNOWN) {
    return kNetworkTechnology1Xrtt;
  }
  return "";
}

string CellularCapabilityCDMA::GetRoamingStateString() const {
  uint32_t state = registration_state_evdo_;
  if (state == MM_MODEM_CDMA_REGISTRATION_STATE_UNKNOWN) {
    state = registration_state_1x_;
  }
  switch (state) {
    case MM_MODEM_CDMA_REGISTRATION_STATE_UNKNOWN:
    case MM_MODEM_CDMA_REGISTRATION_STATE_REGISTERED:
      break;
    case MM_MODEM_CDMA_REGISTRATION_STATE_HOME:
      return kRoamingStateHome;
    case MM_MODEM_CDMA_REGISTRATION_STATE_ROAMING:
      return kRoamingStateRoaming;
    default:
      NOTREACHED();
  }
  return kRoamingStateUnknown;
}

void CellularCapabilityCDMA::GetSignalQuality() {
  SLOG(this, 2) << __func__;
  SignalQualityCallback callback =
      Bind(&CellularCapabilityCDMA::OnGetSignalQualityReply,
           weak_ptr_factory_.GetWeakPtr());
  proxy_->GetSignalQuality(nullptr, callback, kTimeoutDefault);
}

void CellularCapabilityCDMA::GetRegistrationState() {
  SLOG(this, 2) << __func__;
  RegistrationStateCallback callback =
      Bind(&CellularCapabilityCDMA::OnGetRegistrationStateReply,
           weak_ptr_factory_.GetWeakPtr());
  proxy_->GetRegistrationState(nullptr, callback, kTimeoutDefault);
}

void CellularCapabilityCDMA::OnActivateReply(
    const ResultCallback& callback, uint32_t status, const Error& error) {
  activation_starting_ = false;
  if (error.IsSuccess()) {
    if (status == MM_MODEM_CDMA_ACTIVATION_ERROR_NO_ERROR) {
      activation_state_ = MM_MODEM_CDMA_ACTIVATION_STATE_ACTIVATING;
    } else {
      LOG(WARNING) << "Modem activation failed with status: "
                   << GetActivationErrorString(status) << " (" << status << ")";
    }
    HandleNewActivationState(status);
  } else {
    LOG(ERROR) << "Activate() failed with error: " << error;
  }
  callback.Run(error);
}

void CellularCapabilityCDMA::OnGetRegistrationStateReply(
    uint32_t state_1x, uint32_t state_evdo, const Error& error) {
  SLOG(this, 2) << __func__;
  if (error.IsSuccess())
    OnRegistrationStateChangedSignal(state_1x, state_evdo);
}

void CellularCapabilityCDMA::OnGetSignalQualityReply(uint32_t quality,
                                                     const Error& error) {
  if (error.IsSuccess())
    OnSignalQualitySignal(quality);
}

void CellularCapabilityCDMA::OnActivationStateChangedSignal(
    uint32_t activation_state,
    uint32_t activation_error,
    const KeyValueStore& status_changes) {
  SLOG(this, 2) << __func__;

  if (status_changes.ContainsString("mdn"))
    cellular()->set_mdn(status_changes.GetString("mdn"));
  if (status_changes.ContainsString("min"))
    cellular()->set_min(status_changes.GetString("min"));

  UpdateOnlinePortal(status_changes);
  activation_state_ = activation_state;
  HandleNewActivationState(activation_error);
}

void CellularCapabilityCDMA::OnRegistrationStateChangedSignal(
    uint32_t state_1x, uint32_t state_evdo) {
  SLOG(this, 2) << __func__;
  registration_state_1x_ = state_1x;
  registration_state_evdo_ = state_evdo;
  cellular()->HandleNewRegistrationState();
}

void CellularCapabilityCDMA::OnSignalQualitySignal(uint32_t strength) {
  cellular()->HandleNewSignalQuality(strength);
}

void CellularCapabilityCDMA::UpdateOnlinePortal(
    const KeyValueStore& properties) {
  // Treat the three updates atomically: Only update the serving operator when
  // all three are known:
  if (properties.ContainsString("payment_url") &&
      properties.ContainsString("payment_url_method") &&
      properties.ContainsString("payment_url_postdata")) {
    cellular()->home_provider_info()->UpdateOnlinePortal(
        properties.GetString("payment_url"),
        properties.GetString("payment_url_method"),
        properties.GetString("payment_url_postdata"));
  }
}

}  // namespace shill
