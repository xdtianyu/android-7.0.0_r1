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

#include "shill/cellular/cellular_capability_universal_cdma.h"

#include <base/strings/string_number_conversions.h>
#include <base/strings/string_util.h>
#include <base/strings/stringprintf.h>
#if defined(__ANDROID__)
#include <dbus/service_constants.h>
#else
#include <chromeos/dbus/service_constants.h>
#endif  // __ANDROID__

#include "shill/cellular/cellular_bearer.h"
#include "shill/cellular/cellular_service.h"
#include "shill/control_interface.h"
#include "shill/dbus_properties_proxy_interface.h"
#include "shill/error.h"
#include "shill/logging.h"
#include "shill/pending_activation_store.h"

#ifdef MM_MODEM_CDMA_REGISTRATION_STATE_UNKNOWN
#error "Do not include mm-modem.h"
#endif

using base::UintToString;

using std::string;
using std::vector;

namespace shill {

namespace Logging {
static auto kModuleLogScope = ScopeLogger::kCellular;
static string ObjectID(CellularCapabilityUniversalCDMA* c) {
  return c->cellular()->GetRpcIdentifier();
}
}

namespace {

const char kPhoneNumber[] = "#777";
const char kPropertyConnectNumber[] = "number";

}  // namespace

CellularCapabilityUniversalCDMA::CellularCapabilityUniversalCDMA(
    Cellular* cellular,
    ControlInterface* control_interface,
    ModemInfo* modem_info)
    : CellularCapabilityUniversal(cellular, control_interface, modem_info),
      weak_cdma_ptr_factory_(this),
      activation_state_(MM_MODEM_CDMA_ACTIVATION_STATE_NOT_ACTIVATED),
      cdma_1x_registration_state_(MM_MODEM_CDMA_REGISTRATION_STATE_UNKNOWN),
      cdma_evdo_registration_state_(MM_MODEM_CDMA_REGISTRATION_STATE_UNKNOWN),
      nid_(0),
      sid_(0) {
  SLOG(this, 2) << "Cellular capability constructed: Universal CDMA";
  // TODO(armansito): Update PRL for activation over cellular.
  // See crbug.com/197330.
}

CellularCapabilityUniversalCDMA::~CellularCapabilityUniversalCDMA() {}

void CellularCapabilityUniversalCDMA::InitProxies() {
  SLOG(this, 2) << __func__;
  modem_cdma_proxy_.reset(
      control_interface()->CreateMM1ModemModemCdmaProxy(
          cellular()->dbus_path(), cellular()->dbus_service()));
  modem_cdma_proxy_->set_activation_state_callback(
      Bind(&CellularCapabilityUniversalCDMA::OnActivationStateChangedSignal,
      weak_cdma_ptr_factory_.GetWeakPtr()));
  CellularCapabilityUniversal::InitProxies();
}

void CellularCapabilityUniversalCDMA::ReleaseProxies() {
  SLOG(this, 2) << __func__;
  modem_cdma_proxy_.reset();
  CellularCapabilityUniversal::ReleaseProxies();
}

void CellularCapabilityUniversalCDMA::Activate(const string& carrier,
                                               Error* error,
                                               const ResultCallback& callback) {
  // Currently activation over the cellular network is not supported using
  // ModemManager-next. Service activation is currently carried through over
  // non-cellular networks and only the final step of the OTA activation
  // procedure ("automatic activation") is performed by this class.
  OnUnsupportedOperation(__func__, error);
}

void CellularCapabilityUniversalCDMA::CompleteActivation(Error* error) {
  SLOG(this, 2) << __func__;
  if (cellular()->state() < Cellular::kStateEnabled) {
    Error::PopulateAndLog(FROM_HERE, error, Error::kInvalidArguments,
                          "Unable to activate in state " +
                          Cellular::GetStateString(cellular()->state()));
    return;
  }
  ActivateAutomatic();
}

void CellularCapabilityUniversalCDMA::ActivateAutomatic() {
  if (!cellular()->serving_operator_info()->IsMobileNetworkOperatorKnown() ||
      cellular()->serving_operator_info()->activation_code().empty()) {
    SLOG(this, 2) << "OTA activation cannot be run in the presence of no "
                  << "activation code.";
    return;
  }

  PendingActivationStore::State state =
      modem_info()->pending_activation_store()->GetActivationState(
          PendingActivationStore::kIdentifierMEID, cellular()->meid());
  if (state == PendingActivationStore::kStatePending) {
    SLOG(this, 2) << "There's already a pending activation. Ignoring.";
    return;
  }
  if (state == PendingActivationStore::kStateActivated) {
    SLOG(this, 2) << "A call to OTA activation has already completed "
                  << "successfully. Ignoring.";
    return;
  }

  // Mark as pending activation, so that shill can recover if anything fails
  // during OTA activation.
  modem_info()->pending_activation_store()->SetActivationState(
      PendingActivationStore::kIdentifierMEID,
      cellular()->meid(),
      PendingActivationStore::kStatePending);

  // Initiate OTA activation.
  ResultCallback activation_callback =
    Bind(&CellularCapabilityUniversalCDMA::OnActivateReply,
         weak_cdma_ptr_factory_.GetWeakPtr(),
         ResultCallback());

  Error error;
  modem_cdma_proxy_->Activate(
      cellular()->serving_operator_info()->activation_code(),
      &error,
      activation_callback,
      kTimeoutActivate);
}

void CellularCapabilityUniversalCDMA::UpdatePendingActivationState() {
  SLOG(this, 2) << __func__;
  if (IsActivated()) {
    SLOG(this, 3) << "CDMA service activated. Clear store.";
    modem_info()->pending_activation_store()->RemoveEntry(
        PendingActivationStore::kIdentifierMEID, cellular()->meid());
    return;
  }
  PendingActivationStore::State state =
      modem_info()->pending_activation_store()->GetActivationState(
          PendingActivationStore::kIdentifierMEID, cellular()->meid());
  if (IsActivating() && state != PendingActivationStore::kStateFailureRetry) {
    SLOG(this, 3) << "OTA activation in progress. Nothing to do.";
    return;
  }
  switch (state) {
    case PendingActivationStore::kStateFailureRetry:
      SLOG(this, 3) << "OTA activation failed. Scheduling a retry.";
      cellular()->dispatcher()->PostTask(
          Bind(&CellularCapabilityUniversalCDMA::ActivateAutomatic,
               weak_cdma_ptr_factory_.GetWeakPtr()));
      break;
    case PendingActivationStore::kStateActivated:
      SLOG(this, 3) << "OTA Activation has completed successfully. "
                    << "Waiting for activation state update to finalize.";
      break;
    default:
      break;
  }
}

bool CellularCapabilityUniversalCDMA::IsServiceActivationRequired() const {
  // If there is no online payment portal information, it's safer to assume
  // the service does not require activation.
  if (!cellular()->serving_operator_info()->IsMobileNetworkOperatorKnown() ||
      cellular()->serving_operator_info()->olp_list().empty()) {
    return false;
  }

  // We could also use the MDN to determine whether or not the service is
  // activated, however, the CDMA ActivatonState property is a more absolute
  // and fine-grained indicator of activation status.
  return (activation_state_ == MM_MODEM_CDMA_ACTIVATION_STATE_NOT_ACTIVATED);
}

bool CellularCapabilityUniversalCDMA::IsActivated() const {
  return (activation_state_ == MM_MODEM_CDMA_ACTIVATION_STATE_ACTIVATED);
}

void CellularCapabilityUniversalCDMA::OnServiceCreated() {
  SLOG(this, 2) << __func__;
  cellular()->service()->SetActivationType(
      CellularService::kActivationTypeOTASP);
  UpdateServiceActivationStateProperty();
  HandleNewActivationStatus(MM_CDMA_ACTIVATION_ERROR_NONE);
  UpdatePendingActivationState();
}

void CellularCapabilityUniversalCDMA::UpdateServiceActivationStateProperty() {
  string activation_state;
  if (IsActivating())
      activation_state = kActivationStateActivating;
  else if (IsServiceActivationRequired())
      activation_state = kActivationStateNotActivated;
  else
      activation_state = kActivationStateActivated;
  cellular()->service()->SetActivationState(activation_state);
}

void CellularCapabilityUniversalCDMA::UpdateServiceOLP() {
  SLOG(this, 2) << __func__;

  // In this case, the Home Provider is trivial. All information comes from the
  // Serving Operator.
  if (!cellular()->serving_operator_info()->IsMobileNetworkOperatorKnown()) {
    return;
  }

  const vector<MobileOperatorInfo::OnlinePortal>& olp_list =
      cellular()->serving_operator_info()->olp_list();
  if (olp_list.empty()) {
    return;
  }

  if (olp_list.size() > 1) {
    SLOG(this, 1) << "Found multiple online portals. Choosing the first.";
  }
  string post_data = olp_list[0].post_data;
  base::ReplaceSubstringsAfterOffset(&post_data, 0, "${esn}",
                                     cellular()->esn());
  base::ReplaceSubstringsAfterOffset(
      &post_data, 0, "${mdn}",
      GetMdnForOLP(cellular()->serving_operator_info()));
  base::ReplaceSubstringsAfterOffset(&post_data, 0,
                                     "${meid}", cellular()->meid());
  base::ReplaceSubstringsAfterOffset(&post_data, 0, "${oem}", "GOG2");
  cellular()->service()->SetOLP(olp_list[0].url, olp_list[0].method, post_data);
}

void CellularCapabilityUniversalCDMA::GetProperties() {
  SLOG(this, 2) << __func__;
  CellularCapabilityUniversal::GetProperties();

  std::unique_ptr<DBusPropertiesProxyInterface> properties_proxy(
      control_interface()->CreateDBusPropertiesProxy(
          cellular()->dbus_path(), cellular()->dbus_service()));

  KeyValueStore properties(
      properties_proxy->GetAll(MM_DBUS_INTERFACE_MODEM_MODEMCDMA));
  OnModemCDMAPropertiesChanged(properties, vector<string>());
}

void CellularCapabilityUniversalCDMA::OnActivationStateChangedSignal(
    uint32_t activation_state,
    uint32_t activation_error,
    const KeyValueStore& status_changes) {
  SLOG(this, 2) << __func__;

  activation_state_ =
      static_cast<MMModemCdmaActivationState>(activation_state);

  string value;
  if (status_changes.ContainsString("mdn"))
    cellular()->set_mdn(status_changes.GetString("mdn"));
  if (status_changes.ContainsString("min"))
    cellular()->set_min(status_changes.GetString("min"));
  SLOG(this, 2) << "Activation state: "
                << GetActivationStateString(activation_state_);

  HandleNewActivationStatus(activation_error);
  UpdatePendingActivationState();
}

void CellularCapabilityUniversalCDMA::OnActivateReply(
    const ResultCallback& callback,
    const Error& error) {
  SLOG(this, 2) << __func__;
  if (error.IsSuccess()) {
    LOG(INFO) << "Activation completed successfully.";
    modem_info()->pending_activation_store()->SetActivationState(
        PendingActivationStore::kIdentifierMEID,
        cellular()->meid(),
        PendingActivationStore::kStateActivated);
  } else {
    LOG(ERROR) << "Activation failed with error: " << error;
    modem_info()->pending_activation_store()->SetActivationState(
        PendingActivationStore::kIdentifierMEID,
        cellular()->meid(),
        PendingActivationStore::kStateFailureRetry);
  }
  UpdatePendingActivationState();

  // CellularCapabilityUniversalCDMA::ActivateAutomatic passes a dummy
  // ResultCallback when it calls Activate on the proxy object, in which case
  // |callback.is_null()| will return true.
  if (!callback.is_null())
    callback.Run(error);
}

void CellularCapabilityUniversalCDMA::HandleNewActivationStatus(
    uint32_t error) {
  SLOG(this, 2) << __func__ << "(" << error << ")";
  if (!cellular()->service().get()) {
    LOG(ERROR) << "In " << __func__ << "(): service is null.";
    return;
  }
  SLOG(this, 2) << "Activation State: " << activation_state_;
  cellular()->service()->SetActivationState(
      GetActivationStateString(activation_state_));
  cellular()->service()->set_error(GetActivationErrorString(error));
  UpdateServiceOLP();
}

// static
string CellularCapabilityUniversalCDMA::GetActivationStateString(
    uint32_t state) {
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
string CellularCapabilityUniversalCDMA::GetActivationErrorString(
    uint32_t error) {
  switch (error) {
    case MM_CDMA_ACTIVATION_ERROR_WRONG_RADIO_INTERFACE:
      return kErrorNeedEvdo;
    case MM_CDMA_ACTIVATION_ERROR_ROAMING:
      return kErrorNeedHomeNetwork;
    case MM_CDMA_ACTIVATION_ERROR_COULD_NOT_CONNECT:
    case MM_CDMA_ACTIVATION_ERROR_SECURITY_AUTHENTICATION_FAILED:
    case MM_CDMA_ACTIVATION_ERROR_PROVISIONING_FAILED:
      return kErrorOtaspFailed;
    case MM_CDMA_ACTIVATION_ERROR_NONE:
      return "";
    case MM_CDMA_ACTIVATION_ERROR_NO_SIGNAL:
    default:
      return kErrorActivationFailed;
  }
}

void CellularCapabilityUniversalCDMA::Register(const ResultCallback& callback) {
  // TODO(armansito): Remove once 3GPP is implemented in its own class.
}

void CellularCapabilityUniversalCDMA::RegisterOnNetwork(
    const string& network_id,
    Error* error,
    const ResultCallback& callback) {
  // TODO(armansito): Remove once 3GPP is implemented in its own class.
}

bool CellularCapabilityUniversalCDMA::IsActivating() const {
  PendingActivationStore::State state =
      modem_info()->pending_activation_store()->GetActivationState(
          PendingActivationStore::kIdentifierMEID, cellular()->meid());
  return (state == PendingActivationStore::kStatePending) ||
      (state == PendingActivationStore::kStateFailureRetry) ||
      (activation_state_ == MM_MODEM_CDMA_ACTIVATION_STATE_ACTIVATING);
}

bool CellularCapabilityUniversalCDMA::IsRegistered() const {
  return (cdma_1x_registration_state_ !=
              MM_MODEM_CDMA_REGISTRATION_STATE_UNKNOWN ||
          cdma_evdo_registration_state_ !=
              MM_MODEM_CDMA_REGISTRATION_STATE_UNKNOWN);
}

void CellularCapabilityUniversalCDMA::SetUnregistered(bool /*searching*/) {
  cdma_1x_registration_state_ = MM_MODEM_CDMA_REGISTRATION_STATE_UNKNOWN;
  cdma_evdo_registration_state_ = MM_MODEM_CDMA_REGISTRATION_STATE_UNKNOWN;
}

void CellularCapabilityUniversalCDMA::SetupConnectProperties(
    KeyValueStore* properties) {
  properties->SetString(kPropertyConnectNumber, kPhoneNumber);
}

void CellularCapabilityUniversalCDMA::RequirePIN(
    const string& pin, bool require,
    Error* error, const ResultCallback& callback) {
  // TODO(armansito): Remove once 3GPP is implemented in its own class.
}

void CellularCapabilityUniversalCDMA::EnterPIN(
    const string& pin,
    Error* error,
    const ResultCallback& callback) {
  // TODO(armansito): Remove once 3GPP is implemented in its own class.
}

void CellularCapabilityUniversalCDMA::UnblockPIN(
    const string& unblock_code,
    const string& pin,
    Error* error,
    const ResultCallback& callback) {
  // TODO(armansito): Remove once 3GPP is implemented in its own class.
}

void CellularCapabilityUniversalCDMA::ChangePIN(
    const string& old_pin, const string& new_pin,
    Error* error, const ResultCallback& callback) {
  // TODO(armansito): Remove once 3GPP is implemented in its own class.
}

void CellularCapabilityUniversalCDMA::Scan(
    Error* error,
    const ResultStringmapsCallback& callback) {
  // TODO(armansito): Remove once 3GPP is implemented in its own class.
  OnUnsupportedOperation(__func__, error);
}

void CellularCapabilityUniversalCDMA::OnSimPathChanged(
    const string& sim_path) {
  // TODO(armansito): Remove once 3GPP is implemented in its own class.
}

string CellularCapabilityUniversalCDMA::GetRoamingStateString() const {
  uint32_t state = cdma_evdo_registration_state_;
  if (state == MM_MODEM_CDMA_REGISTRATION_STATE_UNKNOWN) {
    state = cdma_1x_registration_state_;
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

void CellularCapabilityUniversalCDMA::OnPropertiesChanged(
    const string& interface,
    const KeyValueStore& changed_properties,
    const vector<string>& invalidated_properties) {
  SLOG(this, 2) << __func__ << "(" << interface << ")";
  if (interface == MM_DBUS_INTERFACE_MODEM_MODEMCDMA) {
    OnModemCDMAPropertiesChanged(changed_properties, invalidated_properties);
  } else {
    CellularCapabilityUniversal::OnPropertiesChanged(
        interface, changed_properties, invalidated_properties);
  }
}

void CellularCapabilityUniversalCDMA::OnModemCDMAPropertiesChanged(
    const KeyValueStore& properties,
    const std::vector<std::string>& /*invalidated_properties*/) {
  SLOG(this, 2) << __func__;
  string str_value;
  if (properties.ContainsString(MM_MODEM_MODEMCDMA_PROPERTY_MEID)) {
    cellular()->set_meid(
        properties.GetString(MM_MODEM_MODEMCDMA_PROPERTY_MEID));
  }
  if (properties.ContainsString(MM_MODEM_MODEMCDMA_PROPERTY_ESN)) {
    cellular()->set_esn(properties.GetString(MM_MODEM_MODEMCDMA_PROPERTY_ESN));
  }

  uint32_t sid = sid_;
  uint32_t nid = nid_;
  MMModemCdmaRegistrationState state_1x = cdma_1x_registration_state_;
  MMModemCdmaRegistrationState state_evdo = cdma_evdo_registration_state_;
  bool registration_changed = false;
  if (properties.ContainsUint(
      MM_MODEM_MODEMCDMA_PROPERTY_CDMA1XREGISTRATIONSTATE)) {
    state_1x = static_cast<MMModemCdmaRegistrationState>(
        properties.GetUint(
            MM_MODEM_MODEMCDMA_PROPERTY_CDMA1XREGISTRATIONSTATE));
    registration_changed = true;
  }
  if (properties.ContainsUint(
      MM_MODEM_MODEMCDMA_PROPERTY_EVDOREGISTRATIONSTATE)) {
    state_evdo = static_cast<MMModemCdmaRegistrationState>(
        properties.GetUint(MM_MODEM_MODEMCDMA_PROPERTY_EVDOREGISTRATIONSTATE));
    registration_changed = true;
  }
  if (properties.ContainsUint(MM_MODEM_MODEMCDMA_PROPERTY_SID)) {
    sid = properties.GetUint(MM_MODEM_MODEMCDMA_PROPERTY_SID);
    registration_changed = true;
  }
  if (properties.ContainsUint(MM_MODEM_MODEMCDMA_PROPERTY_NID)) {
    nid = properties.GetUint(MM_MODEM_MODEMCDMA_PROPERTY_NID);
    registration_changed = true;
  }
  if (properties.ContainsUint(MM_MODEM_MODEMCDMA_PROPERTY_ACTIVATIONSTATE)) {
    activation_state_ = static_cast<MMModemCdmaActivationState>(
        properties.GetUint(MM_MODEM_MODEMCDMA_PROPERTY_ACTIVATIONSTATE));
    HandleNewActivationStatus(MM_CDMA_ACTIVATION_ERROR_NONE);
  }
  if (registration_changed)
    OnCDMARegistrationChanged(state_1x, state_evdo, sid, nid);
}

void CellularCapabilityUniversalCDMA::OnCDMARegistrationChanged(
      MMModemCdmaRegistrationState state_1x,
      MMModemCdmaRegistrationState state_evdo,
      uint32_t sid, uint32_t nid) {
  SLOG(this, 2) << __func__ << ": state_1x=" << state_1x
                            << ", state_evdo=" << state_evdo;
  cdma_1x_registration_state_ = state_1x;
  cdma_evdo_registration_state_ = state_evdo;
  sid_ = sid;
  nid_ = nid;
  cellular()->serving_operator_info()->UpdateSID(UintToString(sid));
  cellular()->serving_operator_info()->UpdateNID(UintToString(nid));
  cellular()->HandleNewRegistrationState();
}

}  // namespace shill
