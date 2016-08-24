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

#include "shill/cellular/cellular_service.h"

#include <string>

#include <base/strings/stringprintf.h>
#if defined(__ANDROID__)
#include <dbus/service_constants.h>
#else
#include <chromeos/dbus/service_constants.h>
#endif  // __ANDROID__

#include "shill/adaptor_interfaces.h"
#include "shill/cellular/cellular.h"
#include "shill/property_accessor.h"
#include "shill/store_interface.h"

using std::string;

namespace shill {

namespace Logging {
static auto kModuleLogScope = ScopeLogger::kCellular;
static string ObjectID(CellularService* c) { return c->GetRpcIdentifier(); }
}

// statics
const char CellularService::kAutoConnActivating[] = "activating";
const char CellularService::kAutoConnBadPPPCredentials[] =
    "bad PPP credentials";
const char CellularService::kAutoConnDeviceDisabled[] = "device disabled";
const char CellularService::kAutoConnOutOfCredits[] = "device out of credits";
const char CellularService::kAutoConnOutOfCreditsDetectionInProgress[] =
    "device detecting out-of-credits";
const char CellularService::kStoragePPPUsername[] = "Cellular.PPP.Username";
const char CellularService::kStoragePPPPassword[] = "Cellular.PPP.Password";

// TODO(petkov): Add these to system_api/dbus/service_constants.h
namespace {
const char kCellularPPPUsernameProperty[] = "Cellular.PPP.Username";
const char kCellularPPPPasswordProperty[] = "Cellular.PPP.Password";
}  // namespace

namespace {
const char kStorageAPN[] = "Cellular.APN";
const char kStorageLastGoodAPN[] = "Cellular.LastGoodAPN";
}  // namespace

static bool GetNonEmptyField(const Stringmap& stringmap,
                             const string& fieldname,
                             string* value) {
  Stringmap::const_iterator it = stringmap.find(fieldname);
  if (it != stringmap.end() && !it->second.empty()) {
    *value = it->second;
    return true;
  }
  return false;
}

CellularService::CellularService(ModemInfo* modem_info,
                                 const CellularRefPtr& device)
    : Service(modem_info->control_interface(), modem_info->dispatcher(),
              modem_info->metrics(), modem_info->manager(),
              Technology::kCellular),
      weak_ptr_factory_(this),
      activation_type_(kActivationTypeUnknown),
      cellular_(device),
      is_auto_connecting_(false) {
  SetConnectable(true);
  PropertyStore* store = this->mutable_store();
  HelpRegisterDerivedString(kActivationTypeProperty,
                            &CellularService::CalculateActivationType,
                            nullptr);
  store->RegisterConstString(kActivationStateProperty, &activation_state_);
  HelpRegisterDerivedStringmap(kCellularApnProperty,
                               &CellularService::GetApn,
                               &CellularService::SetApn);
  store->RegisterConstStringmap(kCellularLastGoodApnProperty,
                                &last_good_apn_info_);
  store->RegisterConstString(kNetworkTechnologyProperty, &network_technology_);
  HelpRegisterDerivedBool(kOutOfCreditsProperty,
                          &CellularService::IsOutOfCredits,
                          nullptr);
  store->RegisterConstStringmap(kPaymentPortalProperty, &olp_);
  store->RegisterConstString(kRoamingStateProperty, &roaming_state_);
  store->RegisterConstStringmap(kServingOperatorProperty, &serving_operator_);
  store->RegisterConstString(kUsageURLProperty, &usage_url_);
  store->RegisterString(kCellularPPPUsernameProperty, &ppp_username_);
  store->RegisterWriteOnlyString(kCellularPPPPasswordProperty, &ppp_password_);

  set_friendly_name(cellular_->CreateDefaultFriendlyServiceName());
  SetStorageIdentifier(string(kTypeCellular) + "_" +
                       cellular_->address() + "_" + friendly_name());
  // Assume we are not performing any out-of-credits detection.
  // The capability can reinitialize with the appropriate type later.
  InitOutOfCreditsDetection(OutOfCreditsDetector::OOCTypeNone);
}

CellularService::~CellularService() { }

bool CellularService::IsAutoConnectable(const char** reason) const {
  if (!cellular_->running()) {
    *reason = kAutoConnDeviceDisabled;
    return false;
  }
  if (cellular_->IsActivating()) {
    *reason = kAutoConnActivating;
    return false;
  }
  if (failure() == kFailurePPPAuth) {
    *reason = kAutoConnBadPPPCredentials;
    return false;
  }
  if (out_of_credits_detector_->IsDetecting()) {
    *reason = kAutoConnOutOfCreditsDetectionInProgress;
    return false;
  }
  if (out_of_credits_detector_->out_of_credits()) {
    *reason = kAutoConnOutOfCredits;
    return false;
  }
  return Service::IsAutoConnectable(reason);
}

void CellularService::HelpRegisterDerivedString(
    const string& name,
    string(CellularService::*get)(Error* error),
    bool(CellularService::*set)(const string& value, Error* error)) {
  mutable_store()->RegisterDerivedString(
      name,
      StringAccessor(
          new CustomAccessor<CellularService, string>(this, get, set)));
}

void CellularService::HelpRegisterDerivedStringmap(
    const string& name,
    Stringmap(CellularService::*get)(Error* error),
    bool(CellularService::*set)(
        const Stringmap& value, Error* error)) {
  mutable_store()->RegisterDerivedStringmap(
      name,
      StringmapAccessor(
          new CustomAccessor<CellularService, Stringmap>(this, get, set)));
}

void CellularService::HelpRegisterDerivedBool(
    const string& name,
    bool(CellularService::*get)(Error* error),
    bool(CellularService::*set)(const bool&, Error*)) {
  mutable_store()->RegisterDerivedBool(
    name,
    BoolAccessor(new CustomAccessor<CellularService, bool>(this, get, set)));
}

Stringmap* CellularService::GetUserSpecifiedApn() {
  Stringmap::iterator it = apn_info_.find(kApnProperty);
  if (it == apn_info_.end() || it->second.empty())
    return nullptr;
  return &apn_info_;
}

Stringmap* CellularService::GetLastGoodApn() {
  Stringmap::iterator it = last_good_apn_info_.find(kApnProperty);
  if (it == last_good_apn_info_.end() || it->second.empty())
    return nullptr;
  return &last_good_apn_info_;
}

string CellularService::CalculateActivationType(Error* error) {
  return GetActivationTypeString();
}

Stringmap CellularService::GetApn(Error* /*error*/) {
  return apn_info_;
}

bool CellularService::SetApn(const Stringmap& value, Error* error) {
  // Only copy in the fields we care about, and validate the contents.
  // If the "apn" field is missing or empty, the APN is cleared.
  string str;
  Stringmap new_apn_info;
  if (GetNonEmptyField(value, kApnProperty, &str)) {
    new_apn_info[kApnProperty] = str;
    if (GetNonEmptyField(value, kApnUsernameProperty, &str))
      new_apn_info[kApnUsernameProperty] = str;
    if (GetNonEmptyField(value, kApnPasswordProperty, &str))
      new_apn_info[kApnPasswordProperty] = str;
  }
  if (apn_info_ == new_apn_info) {
    return false;
  }
  apn_info_ = new_apn_info;
  if (ContainsKey(apn_info_, kApnProperty)) {
    // Clear the last good APN, otherwise the one the user just
    // set won't be used, since LastGoodApn comes first in the
    // search order when trying to connect. Only do this if a
    // non-empty user APN has been supplied. If the user APN is
    // being cleared, leave LastGoodApn alone.
    ClearLastGoodApn();
  }
  adaptor()->EmitStringmapChanged(kCellularApnProperty, apn_info_);
  return true;
}

void CellularService::SetLastGoodApn(const Stringmap& apn_info) {
  last_good_apn_info_ = apn_info;
  adaptor()->EmitStringmapChanged(kCellularLastGoodApnProperty,
                                  last_good_apn_info_);
}

void CellularService::ClearLastGoodApn() {
  last_good_apn_info_.clear();
  adaptor()->EmitStringmapChanged(kCellularLastGoodApnProperty,
                                  last_good_apn_info_);
}

void CellularService::OnAfterResume() {
  Service::OnAfterResume();
  resume_start_time_ = base::Time::Now();
}

void CellularService::InitOutOfCreditsDetection(
    OutOfCreditsDetector::OOCType ooc_type) {
  out_of_credits_detector_.reset(
      OutOfCreditsDetector::CreateDetector(ooc_type,
                                           dispatcher(),
                                           manager(),
                                           metrics(),
                                           this));
}

bool CellularService::Load(StoreInterface* storage) {
  // Load properties common to all Services.
  if (!Service::Load(storage))
    return false;

  const string id = GetStorageIdentifier();
  LoadApn(storage, id, kStorageAPN, &apn_info_);
  LoadApn(storage, id, kStorageLastGoodAPN, &last_good_apn_info_);

  const string old_username = ppp_username_;
  const string old_password = ppp_password_;
  storage->GetString(id, kStoragePPPUsername, &ppp_username_);
  storage->GetString(id, kStoragePPPPassword, &ppp_password_);
  if (IsFailed() && failure() == kFailurePPPAuth &&
      (old_username != ppp_username_ || old_password != ppp_password_)) {
    SetState(kStateIdle);
  }
  return true;
}

void CellularService::LoadApn(StoreInterface* storage,
                              const string& storage_group,
                              const string& keytag,
                              Stringmap* apn_info) {
  if (!LoadApnField(storage, storage_group, keytag, kApnProperty, apn_info))
    return;
  LoadApnField(storage, storage_group, keytag, kApnUsernameProperty, apn_info);
  LoadApnField(storage, storage_group, keytag, kApnPasswordProperty, apn_info);
}

bool CellularService::LoadApnField(StoreInterface* storage,
                                   const string& storage_group,
                                   const string& keytag,
                                   const string& apntag,
                                   Stringmap* apn_info) {
  string value;
  if (storage->GetString(storage_group, keytag + "." + apntag, &value) &&
      !value.empty()) {
    (*apn_info)[apntag] = value;
    return true;
  }
  return false;
}

bool CellularService::Save(StoreInterface* storage) {
  // Save properties common to all Services.
  if (!Service::Save(storage))
    return false;

  const string id = GetStorageIdentifier();
  SaveApn(storage, id, GetUserSpecifiedApn(), kStorageAPN);
  SaveApn(storage, id, GetLastGoodApn(), kStorageLastGoodAPN);
  SaveString(storage, id, kStoragePPPUsername, ppp_username_, false, true);
  SaveString(storage, id, kStoragePPPPassword, ppp_password_, false, true);
  return true;
}

void CellularService::SaveApn(StoreInterface* storage,
                              const string& storage_group,
                              const Stringmap* apn_info,
                              const string& keytag) {
  SaveApnField(storage, storage_group, apn_info, keytag, kApnProperty);
  SaveApnField(storage, storage_group, apn_info, keytag, kApnUsernameProperty);
  SaveApnField(storage, storage_group, apn_info, keytag, kApnPasswordProperty);
}

void CellularService::SaveApnField(StoreInterface* storage,
                                   const string& storage_group,
                                   const Stringmap* apn_info,
                                   const string& keytag,
                                   const string& apntag) {
  const string key = keytag + "." + apntag;
  string str;
  if (apn_info && GetNonEmptyField(*apn_info, apntag, &str))
    storage->SetString(storage_group, key, str);
  else
    storage->DeleteKey(storage_group, key);
}

bool CellularService::IsOutOfCredits(Error* /*error*/) {
  return out_of_credits_detector_->out_of_credits();
}

void CellularService::set_out_of_credits_detector(
    OutOfCreditsDetector* detector) {
  out_of_credits_detector_.reset(detector);
}

void CellularService::SignalOutOfCreditsChanged(bool state) const {
  adaptor()->EmitBoolChanged(kOutOfCreditsProperty, state);
}

void CellularService::AutoConnect() {
  is_auto_connecting_ = true;
  Service::AutoConnect();
  is_auto_connecting_ = false;
}

void CellularService::Connect(Error* error, const char* reason) {
  Service::Connect(error, reason);
  cellular_->Connect(error);
  if (error->IsFailure())
    out_of_credits_detector_->ResetDetector();
}

void CellularService::Disconnect(Error* error, const char* reason) {
  Service::Disconnect(error, reason);
  cellular_->Disconnect(error, reason);
}

void CellularService::ActivateCellularModem(const string& carrier,
                                            Error* error,
                                            const ResultCallback& callback) {
  cellular_->Activate(carrier, error, callback);
}

void CellularService::CompleteCellularActivation(Error* error) {
  cellular_->CompleteActivation(error);
}

void CellularService::SetState(ConnectState new_state) {
  out_of_credits_detector_->NotifyServiceStateChanged(state(), new_state);
  Service::SetState(new_state);
}

void CellularService::SetStorageIdentifier(const string& identifier) {
  SLOG(this, 3) << __func__ << ": " << identifier;
  storage_identifier_ = identifier;
  std::replace_if(storage_identifier_.begin(),
                  storage_identifier_.end(),
                  &Service::IllegalChar, '_');
}

string CellularService::GetStorageIdentifier() const {
  return storage_identifier_;
}

string CellularService::GetDeviceRpcId(Error* /*error*/) const {
  return cellular_->GetRpcIdentifier();
}

void CellularService::SetActivationType(ActivationType type) {
  if (type == activation_type_) {
    return;
  }
  activation_type_ = type;
  adaptor()->EmitStringChanged(kActivationTypeProperty,
                               GetActivationTypeString());
}

string CellularService::GetActivationTypeString() const {
  switch (activation_type_) {
    case kActivationTypeNonCellular:
      return shill::kActivationTypeNonCellular;
    case kActivationTypeOMADM:
      return shill::kActivationTypeOMADM;
    case kActivationTypeOTA:
      return shill::kActivationTypeOTA;
    case kActivationTypeOTASP:
      return shill::kActivationTypeOTASP;
    case kActivationTypeUnknown:
      return "";
    default:
      NOTREACHED();
      return "";  // Make compiler happy.
  }
}

void CellularService::SetActivationState(const string& state) {
  if (state == activation_state_) {
    return;
  }
  activation_state_ = state;
  adaptor()->EmitStringChanged(kActivationStateProperty, state);
  SetConnectableFull(state != kActivationStateNotActivated);
}

void CellularService::SetOLP(const string& url,
                             const string& method,
                             const string& post_data) {
  Stringmap olp;
  olp[kPaymentPortalURL] = url;
  olp[kPaymentPortalMethod] = method;
  olp[kPaymentPortalPostData] = post_data;

  if (olp_ == olp) {
    return;
  }
  olp_ = olp;
  adaptor()->EmitStringmapChanged(kPaymentPortalProperty, olp);
}

void CellularService::SetUsageURL(const string& url) {
  if (url == usage_url_) {
    return;
  }
  usage_url_ = url;
  adaptor()->EmitStringChanged(kUsageURLProperty, url);
}

void CellularService::SetNetworkTechnology(const string& technology) {
  if (technology == network_technology_) {
    return;
  }
  network_technology_ = technology;
  adaptor()->EmitStringChanged(kNetworkTechnologyProperty,
                               technology);
}

void CellularService::SetRoamingState(const string& state) {
  if (state == roaming_state_) {
    return;
  }
  roaming_state_ = state;
  adaptor()->EmitStringChanged(kRoamingStateProperty, state);
}

void CellularService::set_serving_operator(const Stringmap& serving_operator) {
  if (serving_operator_ == serving_operator)
    return;

  serving_operator_ = serving_operator;
  adaptor()->EmitStringmapChanged(kServingOperatorProperty, serving_operator_);
}

}  // namespace shill
