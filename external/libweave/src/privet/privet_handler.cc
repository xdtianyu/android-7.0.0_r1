// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/privet/privet_handler.h"

#include <algorithm>
#include <memory>
#include <set>
#include <string>
#include <utility>

#include <base/bind.h>
#include <base/location.h>
#include <base/strings/stringprintf.h>
#include <base/values.h>
#include <weave/enum_to_string.h>
#include <weave/provider/task_runner.h>

#include "src/config.h"
#include "src/http_constants.h"
#include "src/privet/cloud_delegate.h"
#include "src/privet/constants.h"
#include "src/privet/device_delegate.h"
#include "src/privet/device_ui_kind.h"
#include "src/privet/security_delegate.h"
#include "src/privet/wifi_delegate.h"
#include "src/string_utils.h"
#include "src/utils.h"

namespace weave {
namespace privet {

namespace {

const char kInfoVersionKey[] = "version";
const char kInfoVersionValue[] = "3.0";

const char kNameKey[] = "name";
const char kDescrptionKey[] = "description";
const char kLocationKey[] = "location";

const char kGcdKey[] = "gcd";
const char kWifiKey[] = "wifi";
const char kStatusKey[] = "status";
const char kErrorKey[] = "error";
const char kCryptoKey[] = "crypto";
const char kStatusErrorValue[] = "error";

const char kInfoIdKey[] = "id";
const char kInfoServicesKey[] = "services";

const char kInfoEndpointsKey[] = "endpoints";
const char kInfoEndpointsHttpPortKey[] = "httpPort";
const char kInfoEndpointsHttpUpdatePortKey[] = "httpUpdatesPort";
const char kInfoEndpointsHttpsPortKey[] = "httpsPort";
const char kInfoEndpointsHttpsUpdatePortKey[] = "httpsUpdatesPort";

const char kInfoModelIdKey[] = "modelManifestId";
const char kInfoModelManifestKey[] = "basicModelManifest";
const char kInfoManifestUiDeviceKind[] = "uiDeviceKind";
const char kInfoManifestOemName[] = "oemName";
const char kInfoManifestModelName[] = "modelName";

const char kInfoAuthenticationKey[] = "authentication";

const char kInfoAuthAnonymousMaxScopeKey[] = "anonymousMaxScope";

const char kInfoWifiCapabilitiesKey[] = "capabilities";
const char kInfoWifiSsidKey[] = "ssid";
const char kInfoWifiHostedSsidKey[] = "hostedSsid";
const char kInfoTimeKey[] = "time";
const char kInfoSessionIdKey[] = "sessionId";

const char kPairingKey[] = "pairing";
const char kPairingSessionIdKey[] = "sessionId";
const char kPairingDeviceCommitmentKey[] = "deviceCommitment";
const char kPairingClientCommitmentKey[] = "clientCommitment";
const char kPairingFingerprintKey[] = "certFingerprint";
const char kPairingSignatureKey[] = "certSignature";

const char kAuthModeKey[] = "mode";
const char kAuthCodeKey[] = "authCode";
const char kAuthRequestedScopeKey[] = "requestedScope";
const char kAuthScopeAutoValue[] = "auto";

const char kAuthAccessTokenKey[] = "accessToken";
const char kAuthTokenTypeKey[] = "tokenType";
const char kAuthExpiresInKey[] = "expiresIn";
const char kAuthScopeKey[] = "scope";
const char kAuthClientTokenKey[] = "clientToken";

const char kAuthorizationHeaderPrefix[] = "Privet";

const char kErrorDebugInfoKey[] = "debugInfo";

const char kSetupStartSsidKey[] = "ssid";
const char kSetupStartPassKey[] = "passphrase";
const char kSetupStartTicketIdKey[] = "ticketId";
const char kSetupStartUserKey[] = "user";

const char kFingerprintKey[] = "fingerprint";
const char kStateKey[] = "state";
const char kCommandsKey[] = "commands";
const char kTraitsKey[] = "traits";
const char kComponentsKey[] = "components";
const char kCommandsIdKey[] = "id";
const char kPathKey[] = "path";
const char kFilterKey[] = "filter";

const char kStateFingerprintKey[] = "stateFingerprint";
const char kCommandsFingerprintKey[] = "commandsFingerprint";
const char kTraitsFingerprintKey[] = "traitsFingerprint";
const char kComponentsFingerprintKey[] = "componentsFingerprint";
const char kWaitTimeoutKey[] = "waitTimeout";

const char kInvalidParamValueFormat[] = "Invalid parameter: '%s'='%s'";

template <class Container>
std::unique_ptr<base::ListValue> ToValue(const Container& list) {
  std::unique_ptr<base::ListValue> value_list(new base::ListValue());
  for (const std::string& val : list)
    value_list->AppendString(val);
  return value_list;
}

struct {
  const char* const reason;
  int code;
} kReasonToCode[] = {
    {errors::kInvalidClientCommitment, http::kForbidden},
    {errors::kInvalidFormat, http::kBadRequest},
    {errors::kMissingAuthorization, http::kDenied},
    {errors::kInvalidAuthorization, http::kDenied},
    {errors::kInvalidAuthorizationScope, http::kForbidden},
    {errors::kAuthorizationExpired, http::kForbidden},
    {errors::kCommitmentMismatch, http::kForbidden},
    {errors::kUnknownSession, http::kNotFound},
    {errors::kInvalidAuthCode, http::kForbidden},
    {errors::kInvalidAuthMode, http::kBadRequest},
    {errors::kInvalidRequestedScope, http::kBadRequest},
    {errors::kAccessDenied, http::kForbidden},
    {errors::kInvalidParams, http::kBadRequest},
    {errors::kSetupUnavailable, http::kBadRequest},
    {errors::kDeviceBusy, http::kServiceUnavailable},
    {errors::kInvalidState, http::kInternalServerError},
    {errors::kNotFound, http::kNotFound},
    {errors::kNotImplemented, http::kNotSupported},
    {errors::kAlreadyClaimed, http::kDenied},
};

std::string GetAuthTokenFromAuthHeader(const std::string& auth_header) {
  return SplitAtFirst(auth_header, " ", true).second;
}

// Creates JSON similar to GCD server error format.
std::unique_ptr<base::DictionaryValue> ErrorToJson(const Error& error) {
  std::unique_ptr<base::DictionaryValue> output{ErrorInfoToJson(error)};

  // Optional debug information.
  std::unique_ptr<base::ListValue> errors{new base::ListValue};
  for (const Error* it = &error; it; it = it->GetInnerError()) {
    std::unique_ptr<base::DictionaryValue> inner{ErrorInfoToJson(*it)};
    tracked_objects::Location location{it->GetLocation().function_name.c_str(),
                                       it->GetLocation().file_name.c_str(),
                                       it->GetLocation().line_number, nullptr};
    inner->SetString(kErrorDebugInfoKey, location.ToString());
    errors->Append(inner.release());
  }
  output->Set(kErrorDebugInfoKey, errors.release());
  return output;
}

template <class T>
void SetStateProperties(const T& state, base::DictionaryValue* parent) {
  if (!state.error()) {
    parent->SetString(kStatusKey, EnumToString(state.status()));
    return;
  }
  parent->SetString(kStatusKey, kStatusErrorValue);
  parent->Set(kErrorKey, ErrorToJson(*state.error()).release());
}

void ReturnError(const Error& error,
                 const PrivetHandler::RequestCallback& callback) {
  int code = http::kInternalServerError;
  for (const auto& it : kReasonToCode) {
    if (error.HasError(it.reason)) {
      code = it.code;
      break;
    }
  }
  std::unique_ptr<base::DictionaryValue> output{new base::DictionaryValue};
  output->Set(kErrorKey, ErrorToJson(error).release());
  callback.Run(code, *output);
}

void OnCommandRequestSucceeded(const PrivetHandler::RequestCallback& callback,
                               const base::DictionaryValue& output,
                               ErrorPtr error) {
  if (!error)
    return callback.Run(http::kOk, output);

  if (error->HasError("unknown_command")) {
    Error::AddTo(&error, FROM_HERE, errors::kNotFound, "Unknown command ID");
    return ReturnError(*error, callback);
  }
  if (error->HasError("access_denied")) {
    Error::AddTo(&error, FROM_HERE, errors::kAccessDenied, error->GetMessage());
    return ReturnError(*error, callback);
  }
  return ReturnError(*error, callback);
}

std::unique_ptr<base::DictionaryValue> CreateManifestSection(
    const CloudDelegate& cloud) {
  std::unique_ptr<base::DictionaryValue> manifest(new base::DictionaryValue());
  manifest->SetString(kInfoManifestUiDeviceKind,
                      GetDeviceUiKind(cloud.GetModelId()));
  manifest->SetString(kInfoManifestOemName, cloud.GetOemName());
  manifest->SetString(kInfoManifestModelName, cloud.GetModelName());
  return manifest;
}

std::unique_ptr<base::DictionaryValue> CreateEndpointsSection(
    const DeviceDelegate& device) {
  std::unique_ptr<base::DictionaryValue> endpoints(new base::DictionaryValue());
  auto http_endpoint = device.GetHttpEnpoint();
  endpoints->SetInteger(kInfoEndpointsHttpPortKey, http_endpoint.first);
  endpoints->SetInteger(kInfoEndpointsHttpUpdatePortKey, http_endpoint.second);

  auto https_endpoint = device.GetHttpsEnpoint();
  endpoints->SetInteger(kInfoEndpointsHttpsPortKey, https_endpoint.first);
  endpoints->SetInteger(kInfoEndpointsHttpsUpdatePortKey,
                        https_endpoint.second);

  return endpoints;
}

std::unique_ptr<base::DictionaryValue> CreateInfoAuthSection(
    const SecurityDelegate& security,
    AuthScope anonymous_max_scope) {
  std::unique_ptr<base::DictionaryValue> auth(new base::DictionaryValue());

  auth->SetString(kInfoAuthAnonymousMaxScopeKey,
                  EnumToString(anonymous_max_scope));

  std::unique_ptr<base::ListValue> pairing_types(new base::ListValue());
  for (PairingType type : security.GetPairingTypes())
    pairing_types->AppendString(EnumToString(type));
  auth->Set(kPairingKey, pairing_types.release());

  std::unique_ptr<base::ListValue> auth_types(new base::ListValue());
  for (AuthType type : security.GetAuthTypes())
    auth_types->AppendString(EnumToString(type));
  auth->Set(kAuthModeKey, auth_types.release());

  std::unique_ptr<base::ListValue> crypto_types(new base::ListValue());
  for (CryptoType type : security.GetCryptoTypes())
    crypto_types->AppendString(EnumToString(type));
  auth->Set(kCryptoKey, crypto_types.release());

  return auth;
}

std::unique_ptr<base::DictionaryValue> CreateWifiSection(
    const WifiDelegate& wifi) {
  std::unique_ptr<base::DictionaryValue> result(new base::DictionaryValue());

  std::unique_ptr<base::ListValue> capabilities(new base::ListValue());
  for (WifiType type : wifi.GetTypes())
    capabilities->AppendString(EnumToString(type));
  result->Set(kInfoWifiCapabilitiesKey, capabilities.release());

  result->SetString(kInfoWifiSsidKey, wifi.GetCurrentlyConnectedSsid());

  std::string hosted_ssid = wifi.GetHostedSsid();
  const ConnectionState& state = wifi.GetConnectionState();
  if (!hosted_ssid.empty()) {
    DCHECK(!state.IsStatusEqual(ConnectionState::kDisabled));
    DCHECK(!state.IsStatusEqual(ConnectionState::kOnline));
    result->SetString(kInfoWifiHostedSsidKey, hosted_ssid);
  }
  SetStateProperties(state, result.get());
  return result;
}

std::unique_ptr<base::DictionaryValue> CreateGcdSection(
    const CloudDelegate& cloud) {
  std::unique_ptr<base::DictionaryValue> gcd(new base::DictionaryValue());
  gcd->SetString(kInfoIdKey, cloud.GetCloudId());
  SetStateProperties(cloud.GetConnectionState(), gcd.get());
  return gcd;
}

AuthScope GetAnonymousMaxScope(const CloudDelegate& cloud,
                               const WifiDelegate* wifi) {
  if (wifi && !wifi->GetHostedSsid().empty())
    return AuthScope::kNone;
  return cloud.GetAnonymousMaxScope();
}

// Forward-declaration.
std::unique_ptr<base::DictionaryValue> CloneComponentTree(
    const base::DictionaryValue& parent,
    const std::set<std::string>& filter);

// Clones a particular component JSON object in a manner similar to that of
// DeepCopy(), except it includes only sub-objects specified in |filter| (if not
// empty) and has special handling for "components" sub-dictionary.
std::unique_ptr<base::DictionaryValue> CloneComponent(
    const base::DictionaryValue& component,
    const std::set<std::string>& filter) {
  std::unique_ptr<base::DictionaryValue> clone{new base::DictionaryValue};
  for (base::DictionaryValue::Iterator it(component); !it.IsAtEnd();
       it.Advance()) {
    if (filter.empty() || filter.find(it.key()) != filter.end()) {
      if (it.key() == kComponentsKey) {
        // Handle "components" separately as we need to recursively clone
        // sub-components.
        const base::DictionaryValue* sub_components = nullptr;
        CHECK(it.value().GetAsDictionary(&sub_components));
        clone->SetWithoutPathExpansion(
            it.key(), CloneComponentTree(*sub_components, filter).release());
      } else {
        clone->SetWithoutPathExpansion(it.key(), it.value().DeepCopy());
      }
    }
  }
  return clone;
}

// Clones a dictionary containing a bunch of component JSON objects in a manner
// similar to that of DeepCopy(). Calls CloneComponent() on each instance of
// the component sub-object.
std::unique_ptr<base::DictionaryValue> CloneComponentTree(
    const base::DictionaryValue& parent,
    const std::set<std::string>& filter) {
  std::unique_ptr<base::DictionaryValue> clone{new base::DictionaryValue};
  for (base::DictionaryValue::Iterator it(parent); !it.IsAtEnd();
       it.Advance()) {
    const base::DictionaryValue* component = nullptr;
    CHECK(it.value().GetAsDictionary(&component));
    clone->SetWithoutPathExpansion(
        it.key(), CloneComponent(*component, filter).release());
  }
  return clone;
}

}  // namespace

std::vector<std::string> PrivetHandler::GetHttpPaths() const {
  std::vector<std::string> result;
  for (const auto& pair : handlers_) {
    if (!pair.second.https_only)
      result.push_back(pair.first);
  }
  return result;
}

std::vector<std::string> PrivetHandler::GetHttpsPaths() const {
  std::vector<std::string> result;
  for (const auto& pair : handlers_)
    result.push_back(pair.first);
  return result;
}

PrivetHandler::PrivetHandler(CloudDelegate* cloud,
                             DeviceDelegate* device,
                             SecurityDelegate* security,
                             WifiDelegate* wifi,
                             base::Clock* clock)
    : cloud_(cloud),
      device_(device),
      security_(security),
      wifi_(wifi),
      clock_(clock ? clock : &default_clock_) {
  CHECK(cloud_);
  CHECK(device_);
  CHECK(security_);
  CHECK(clock_);
  cloud_observer_.Add(cloud_);

  AddHandler("/privet/info", &PrivetHandler::HandleInfo, AuthScope::kNone);
  AddHandler("/privet/v3/pairing/start", &PrivetHandler::HandlePairingStart,
             AuthScope::kNone);
  AddHandler("/privet/v3/pairing/confirm", &PrivetHandler::HandlePairingConfirm,
             AuthScope::kNone);
  AddHandler("/privet/v3/pairing/cancel", &PrivetHandler::HandlePairingCancel,
             AuthScope::kNone);

  AddSecureHandler("/privet/v3/auth", &PrivetHandler::HandleAuth,
                   AuthScope::kNone);
  AddSecureHandler("/privet/v3/accessControl/claim",
                   &PrivetHandler::HandleAccessControlClaim, AuthScope::kOwner);
  AddSecureHandler("/privet/v3/accessControl/confirm",
                   &PrivetHandler::HandleAccessControlConfirm,
                   AuthScope::kOwner);
  AddSecureHandler("/privet/v3/setup/start", &PrivetHandler::HandleSetupStart,
                   AuthScope::kManager);
  AddSecureHandler("/privet/v3/setup/status", &PrivetHandler::HandleSetupStatus,
                   AuthScope::kManager);
  AddSecureHandler("/privet/v3/state", &PrivetHandler::HandleState,
                   AuthScope::kViewer);
  AddSecureHandler("/privet/v3/commandDefs", &PrivetHandler::HandleCommandDefs,
                   AuthScope::kViewer);
  AddSecureHandler("/privet/v3/commands/execute",
                   &PrivetHandler::HandleCommandsExecute, AuthScope::kViewer);
  AddSecureHandler("/privet/v3/commands/status",
                   &PrivetHandler::HandleCommandsStatus, AuthScope::kViewer);
  AddSecureHandler("/privet/v3/commands/cancel",
                   &PrivetHandler::HandleCommandsCancel, AuthScope::kViewer);
  AddSecureHandler("/privet/v3/commands/list",
                   &PrivetHandler::HandleCommandsList, AuthScope::kViewer);
  AddSecureHandler("/privet/v3/checkForUpdates",
                   &PrivetHandler::HandleCheckForUpdates, AuthScope::kViewer);
  AddSecureHandler("/privet/v3/traits", &PrivetHandler::HandleTraits,
                   AuthScope::kViewer);
  AddSecureHandler("/privet/v3/components", &PrivetHandler::HandleComponents,
                   AuthScope::kViewer);
}

PrivetHandler::~PrivetHandler() {
  for (const auto& req : update_requests_)
    ReplyToUpdateRequest(req.callback);
}

void PrivetHandler::OnTraitDefsChanged() {
  ++traits_fingerprint_;
  auto pred = [this](const UpdateRequestParameters& params) {
    return params.traits_fingerprint == 0;
  };
  auto last =
      std::partition(update_requests_.begin(), update_requests_.end(), pred);
  for (auto p = last; p != update_requests_.end(); ++p)
    ReplyToUpdateRequest(p->callback);
  update_requests_.erase(last, update_requests_.end());
}

void PrivetHandler::OnStateChanged() {
  // State updates also change the component tree, so update both fingerprints.
  ++state_fingerprint_;
  ++components_fingerprint_;
  auto pred = [this](const UpdateRequestParameters& params) {
    return params.state_fingerprint == 0 && params.components_fingerprint == 0;
  };
  auto last =
      std::partition(update_requests_.begin(), update_requests_.end(), pred);
  for (auto p = last; p != update_requests_.end(); ++p)
    ReplyToUpdateRequest(p->callback);
  update_requests_.erase(last, update_requests_.end());
}

void PrivetHandler::OnComponentTreeChanged() {
  ++components_fingerprint_;
  auto pred = [this](const UpdateRequestParameters& params) {
    return params.components_fingerprint == 0;
  };
  auto last =
      std::partition(update_requests_.begin(), update_requests_.end(), pred);
  for (auto p = last; p != update_requests_.end(); ++p)
    ReplyToUpdateRequest(p->callback);
  update_requests_.erase(last, update_requests_.end());
}

void PrivetHandler::HandleRequest(const std::string& api,
                                  const std::string& auth_header,
                                  const base::DictionaryValue* input,
                                  const RequestCallback& callback) {
  ErrorPtr error;
  if (!input) {
    Error::AddTo(&error, FROM_HERE, errors::kInvalidFormat, "Malformed JSON");
    return ReturnError(*error, callback);
  }
  auto handler = handlers_.find(api);
  if (handler == handlers_.end()) {
    Error::AddTo(&error, FROM_HERE, errors::kNotFound, "Path not found");
    return ReturnError(*error, callback);
  }
  if (auth_header.empty()) {
    Error::AddTo(&error, FROM_HERE, errors::kMissingAuthorization,
                 "Authorization header must not be empty");
    return ReturnError(*error, callback);
  }
  std::string token = GetAuthTokenFromAuthHeader(auth_header);
  if (token.empty()) {
    Error::AddToPrintf(&error, FROM_HERE, errors::kInvalidAuthorization,
                       "Invalid authorization header: %s", auth_header.c_str());
    return ReturnError(*error, callback);
  }
  UserInfo user_info;
  if (token != EnumToString(AuthType::kAnonymous)) {
    if (!security_->ParseAccessToken(token, &user_info, &error))
      return ReturnError(*error, callback);
  }

  if (handler->second.scope > user_info.scope()) {
    Error::AddToPrintf(&error, FROM_HERE, errors::kInvalidAuthorizationScope,
                       "Scope '%s' does not allow '%s'",
                       EnumToString(user_info.scope()).c_str(), api.c_str());
    return ReturnError(*error, callback);
  }
  (this->*handler->second.handler)(*input, user_info, callback);
}

void PrivetHandler::AddHandler(const std::string& path,
                               ApiHandler handler,
                               AuthScope scope) {
  HandlerParameters params;
  params.handler = handler;
  params.scope = scope;
  params.https_only = false;
  CHECK(handlers_.insert(std::make_pair(path, params)).second);
}

void PrivetHandler::AddSecureHandler(const std::string& path,
                                     ApiHandler handler,
                                     AuthScope scope) {
  HandlerParameters params;
  params.handler = handler;
  params.scope = scope;
  params.https_only = true;
  CHECK(handlers_.insert(std::make_pair(path, params)).second);
}

void PrivetHandler::HandleInfo(const base::DictionaryValue&,
                               const UserInfo& user_info,
                               const RequestCallback& callback) {
  base::DictionaryValue output;

  std::string name = cloud_->GetName();
  std::string model_id = cloud_->GetModelId();

  output.SetString(kInfoVersionKey, kInfoVersionValue);
  output.SetString(kInfoIdKey, cloud_->GetDeviceId());
  output.SetString(kNameKey, name);

  std::string description{cloud_->GetDescription()};
  if (!description.empty())
    output.SetString(kDescrptionKey, description);

  std::string location{cloud_->GetLocation()};
  if (!location.empty())
    output.SetString(kLocationKey, location);

  output.SetString(kInfoModelIdKey, model_id);
  output.Set(kInfoModelManifestKey, CreateManifestSection(*cloud_).release());
  output.Set(
      kInfoServicesKey,
      ToValue(std::vector<std::string>{GetDeviceUiKind(cloud_->GetModelId())})
          .release());

  output.Set(
      kInfoAuthenticationKey,
      CreateInfoAuthSection(*security_, GetAnonymousMaxScope(*cloud_, wifi_))
          .release());

  output.Set(kInfoEndpointsKey, CreateEndpointsSection(*device_).release());

  if (wifi_)
    output.Set(kWifiKey, CreateWifiSection(*wifi_).release());

  output.Set(kGcdKey, CreateGcdSection(*cloud_).release());

  output.SetDouble(kInfoTimeKey, clock_->Now().ToJsTime());
  output.SetString(kInfoSessionIdKey, security_->CreateSessionId());

  callback.Run(http::kOk, output);
}

void PrivetHandler::HandlePairingStart(const base::DictionaryValue& input,
                                       const UserInfo& user_info,
                                       const RequestCallback& callback) {
  ErrorPtr error;

  std::string pairing_str;
  input.GetString(kPairingKey, &pairing_str);

  std::string crypto_str;
  input.GetString(kCryptoKey, &crypto_str);

  PairingType pairing;
  std::set<PairingType> modes = security_->GetPairingTypes();
  if (!StringToEnum(pairing_str, &pairing) ||
      modes.find(pairing) == modes.end()) {
    Error::AddToPrintf(&error, FROM_HERE, errors::kInvalidParams,
                       kInvalidParamValueFormat, kPairingKey,
                       pairing_str.c_str());
    return ReturnError(*error, callback);
  }

  CryptoType crypto;
  std::set<CryptoType> cryptos = security_->GetCryptoTypes();
  if (!StringToEnum(crypto_str, &crypto) ||
      cryptos.find(crypto) == cryptos.end()) {
    Error::AddToPrintf(&error, FROM_HERE, errors::kInvalidParams,
                       kInvalidParamValueFormat, kCryptoKey,
                       crypto_str.c_str());
    return ReturnError(*error, callback);
  }

  std::string id;
  std::string commitment;
  if (!security_->StartPairing(pairing, crypto, &id, &commitment, &error))
    return ReturnError(*error, callback);

  base::DictionaryValue output;
  output.SetString(kPairingSessionIdKey, id);
  output.SetString(kPairingDeviceCommitmentKey, commitment);
  callback.Run(http::kOk, output);
}

void PrivetHandler::HandlePairingConfirm(const base::DictionaryValue& input,
                                         const UserInfo& user_info,
                                         const RequestCallback& callback) {
  std::string id;
  input.GetString(kPairingSessionIdKey, &id);

  std::string commitment;
  input.GetString(kPairingClientCommitmentKey, &commitment);

  std::string fingerprint;
  std::string signature;
  ErrorPtr error;
  if (!security_->ConfirmPairing(id, commitment, &fingerprint, &signature,
                                 &error)) {
    return ReturnError(*error, callback);
  }

  base::DictionaryValue output;
  output.SetString(kPairingFingerprintKey, fingerprint);
  output.SetString(kPairingSignatureKey, signature);
  callback.Run(http::kOk, output);
}

void PrivetHandler::HandlePairingCancel(const base::DictionaryValue& input,
                                        const UserInfo& user_info,
                                        const RequestCallback& callback) {
  std::string id;
  input.GetString(kPairingSessionIdKey, &id);

  ErrorPtr error;
  if (!security_->CancelPairing(id, &error))
    return ReturnError(*error, callback);

  base::DictionaryValue output;
  callback.Run(http::kOk, output);
}

void PrivetHandler::HandleAuth(const base::DictionaryValue& input,
                               const UserInfo& user_info,
                               const RequestCallback& callback) {
  ErrorPtr error;

  std::string auth_code_type;
  AuthType auth_type{};
  if (!input.GetString(kAuthModeKey, &auth_code_type) ||
      !StringToEnum(auth_code_type, &auth_type)) {
    Error::AddToPrintf(&error, FROM_HERE, errors::kInvalidAuthMode,
                       kInvalidParamValueFormat, kAuthModeKey,
                       auth_code_type.c_str());
    return ReturnError(*error, callback);
  }

  AuthScope desired_scope = AuthScope::kOwner;
  AuthScope acceptable_scope = AuthScope::kViewer;

  std::string requested_scope;
  input.GetString(kAuthRequestedScopeKey, &requested_scope);
  if (requested_scope != kAuthScopeAutoValue) {
    if (!StringToEnum(requested_scope, &desired_scope)) {
      Error::AddToPrintf(&error, FROM_HERE, errors::kInvalidRequestedScope,
                         kInvalidParamValueFormat, kAuthRequestedScopeKey,
                         requested_scope.c_str());
      return ReturnError(*error, callback);
    }
    acceptable_scope = std::max(desired_scope, acceptable_scope);
  }

  if (auth_type == AuthType::kAnonymous)
    desired_scope = GetAnonymousMaxScope(*cloud_, wifi_);

  std::string auth_code;
  input.GetString(kAuthCodeKey, &auth_code);

  std::string access_token;
  base::TimeDelta access_token_ttl;
  AuthScope access_token_scope = AuthScope::kNone;
  if (!security_->CreateAccessToken(auth_type, auth_code, desired_scope,
                                    &access_token, &access_token_scope,
                                    &access_token_ttl, &error)) {
    return ReturnError(*error, callback);
  }

  if (access_token_scope < acceptable_scope) {
    Error::AddToPrintf(&error, FROM_HERE, errors::kAccessDenied,
                       "Scope '%s' is not allowed",
                       EnumToString(access_token_scope).c_str());
    return ReturnError(*error, callback);
  }

  base::DictionaryValue output;
  output.SetString(kAuthAccessTokenKey, access_token);
  output.SetString(kAuthTokenTypeKey, kAuthorizationHeaderPrefix);
  output.SetInteger(kAuthExpiresInKey, access_token_ttl.InSeconds());
  output.SetString(kAuthScopeKey, EnumToString(access_token_scope));

  callback.Run(http::kOk, output);
}

void PrivetHandler::HandleAccessControlClaim(const base::DictionaryValue& input,
                                             const UserInfo& user_info,
                                             const RequestCallback& callback) {
  ErrorPtr error;
  auto token = security_->ClaimRootClientAuthToken(&error);
  if (token.empty())
    return ReturnError(*error, callback);

  base::DictionaryValue output;
  output.SetString(kAuthClientTokenKey, token);
  callback.Run(http::kOk, output);
}

void PrivetHandler::HandleAccessControlConfirm(
    const base::DictionaryValue& input,
    const UserInfo& user_info,
    const RequestCallback& callback) {
  ErrorPtr error;

  std::string token;
  if (!input.GetString(kAuthClientTokenKey, &token)) {
    Error::AddToPrintf(&error, FROM_HERE, errors::kInvalidParams,
                       kInvalidParamValueFormat, kAuthClientTokenKey,
                       token.c_str());
    return ReturnError(*error, callback);
  }

  if (!security_->ConfirmClientAuthToken(token, &error))
    return ReturnError(*error, callback);

  base::DictionaryValue output;
  callback.Run(http::kOk, output);
}

void PrivetHandler::HandleSetupStart(const base::DictionaryValue& input,
                                     const UserInfo& user_info,
                                     const RequestCallback& callback) {
  std::string name{cloud_->GetName()};
  input.GetString(kNameKey, &name);

  std::string description{cloud_->GetDescription()};
  input.GetString(kDescrptionKey, &description);

  std::string location{cloud_->GetLocation()};
  input.GetString(kLocationKey, &location);

  std::string ssid;
  std::string passphrase;
  std::string ticket;
  std::string user;

  const base::DictionaryValue* wifi = nullptr;
  if (input.GetDictionary(kWifiKey, &wifi)) {
    if (!wifi_ || wifi_->GetTypes().empty()) {
      ErrorPtr error;
      Error::AddTo(&error, FROM_HERE, errors::kSetupUnavailable,
                   "WiFi setup unavailable");
      return ReturnError(*error, callback);
    }
    wifi->GetString(kSetupStartSsidKey, &ssid);
    if (ssid.empty()) {
      ErrorPtr error;
      Error::AddToPrintf(&error, FROM_HERE, errors::kInvalidParams,
                         kInvalidParamValueFormat, kSetupStartSsidKey, "");
      return ReturnError(*error, callback);
    }
    wifi->GetString(kSetupStartPassKey, &passphrase);
  }

  const base::DictionaryValue* registration = nullptr;
  if (input.GetDictionary(kGcdKey, &registration)) {
    if (user_info.scope() < AuthScope::kOwner) {
      ErrorPtr error;
      Error::AddTo(&error, FROM_HERE, errors::kInvalidAuthorizationScope,
                   "Only owner can register device");
      return ReturnError(*error, callback);
    }
    registration->GetString(kSetupStartTicketIdKey, &ticket);
    if (ticket.empty()) {
      ErrorPtr error;
      Error::AddToPrintf(&error, FROM_HERE, errors::kInvalidParams,
                         kInvalidParamValueFormat, kSetupStartTicketIdKey, "");
      return ReturnError(*error, callback);
    }
    registration->GetString(kSetupStartUserKey, &user);
  }

  cloud_->UpdateDeviceInfo(name, description, location);

  ErrorPtr error;
  if (!ssid.empty() && !wifi_->ConfigureCredentials(ssid, passphrase, &error))
    return ReturnError(*error, callback);

  if (!ticket.empty() && !cloud_->Setup(ticket, user, &error))
    return ReturnError(*error, callback);

  ReplyWithSetupStatus(callback);
}

void PrivetHandler::HandleSetupStatus(const base::DictionaryValue&,
                                      const UserInfo& user_info,
                                      const RequestCallback& callback) {
  ReplyWithSetupStatus(callback);
}

void PrivetHandler::ReplyWithSetupStatus(
    const RequestCallback& callback) const {
  base::DictionaryValue output;

  const SetupState& state = cloud_->GetSetupState();
  if (!state.IsStatusEqual(SetupState::kNone)) {
    base::DictionaryValue* gcd = new base::DictionaryValue;
    output.Set(kGcdKey, gcd);
    SetStateProperties(state, gcd);
    if (state.IsStatusEqual(SetupState::kSuccess))
      gcd->SetString(kInfoIdKey, cloud_->GetCloudId());
  }

  if (wifi_) {
    const SetupState& state = wifi_->GetSetupState();
    if (!state.IsStatusEqual(SetupState::kNone)) {
      base::DictionaryValue* wifi = new base::DictionaryValue;
      output.Set(kWifiKey, wifi);
      SetStateProperties(state, wifi);
      if (state.IsStatusEqual(SetupState::kSuccess))
        wifi->SetString(kInfoWifiSsidKey, wifi_->GetCurrentlyConnectedSsid());
    }
  }

  callback.Run(http::kOk, output);
}

void PrivetHandler::HandleState(const base::DictionaryValue& input,
                                const UserInfo& user_info,
                                const RequestCallback& callback) {
  base::DictionaryValue output;
  output.Set(kStateKey, cloud_->GetLegacyState().DeepCopy());
  output.SetString(kFingerprintKey, std::to_string(state_fingerprint_));

  callback.Run(http::kOk, output);
}

void PrivetHandler::HandleTraits(const base::DictionaryValue& input,
                                 const UserInfo& user_info,
                                 const RequestCallback& callback) {
  base::DictionaryValue output;
  output.Set(kTraitsKey, cloud_->GetTraits().DeepCopy());
  output.SetString(kFingerprintKey, std::to_string(traits_fingerprint_));

  callback.Run(http::kOk, output);
}

void PrivetHandler::HandleComponents(const base::DictionaryValue& input,
                                     const UserInfo& user_info,
                                     const RequestCallback& callback) {
  std::string path;
  std::set<std::string> filter;
  std::unique_ptr<base::DictionaryValue> components;

  input.GetString(kPathKey, &path);
  const base::ListValue* filter_items = nullptr;
  if (input.GetList(kFilterKey, &filter_items)) {
    for (const base::Value* value : *filter_items) {
      std::string filter_item;
      if (value->GetAsString(&filter_item))
        filter.insert(filter_item);
    }
  }
  const base::DictionaryValue* component = nullptr;
  if (!path.empty()) {
    ErrorPtr error;
    component = cloud_->FindComponent(path, &error);
    if (!component)
      return ReturnError(*error, callback);
    components.reset(new base::DictionaryValue);
    // Get the last element of the path and use it as a dictionary key here.
    auto parts = Split(path, ".", true, false);
    components->Set(parts.back(), CloneComponent(*component, filter).release());
  } else {
    components = CloneComponentTree(cloud_->GetComponents(), filter);
  }
  base::DictionaryValue output;
  output.Set(kComponentsKey, components.release());
  output.SetString(kFingerprintKey, std::to_string(components_fingerprint_));

  callback.Run(http::kOk, output);
}

void PrivetHandler::HandleCommandDefs(const base::DictionaryValue& input,
                                      const UserInfo& user_info,
                                      const RequestCallback& callback) {
  base::DictionaryValue output;
  output.Set(kCommandsKey, cloud_->GetLegacyCommandDef().DeepCopy());
  // Use traits fingerprint since right now we treat traits and command defs
  // as being equivalent.
  output.SetString(kFingerprintKey, std::to_string(traits_fingerprint_));

  callback.Run(http::kOk, output);
}

void PrivetHandler::HandleCommandsExecute(const base::DictionaryValue& input,
                                          const UserInfo& user_info,
                                          const RequestCallback& callback) {
  cloud_->AddCommand(input, user_info,
                     base::Bind(&OnCommandRequestSucceeded, callback));
}

void PrivetHandler::HandleCommandsStatus(const base::DictionaryValue& input,
                                         const UserInfo& user_info,
                                         const RequestCallback& callback) {
  std::string id;
  if (!input.GetString(kCommandsIdKey, &id)) {
    ErrorPtr error;
    Error::AddToPrintf(&error, FROM_HERE, errors::kInvalidParams,
                       kInvalidParamValueFormat, kCommandsIdKey, id.c_str());
    return ReturnError(*error, callback);
  }
  cloud_->GetCommand(id, user_info,
                     base::Bind(&OnCommandRequestSucceeded, callback));
}

void PrivetHandler::HandleCommandsList(const base::DictionaryValue& input,
                                       const UserInfo& user_info,
                                       const RequestCallback& callback) {
  cloud_->ListCommands(user_info,
                       base::Bind(&OnCommandRequestSucceeded, callback));
}

void PrivetHandler::HandleCommandsCancel(const base::DictionaryValue& input,
                                         const UserInfo& user_info,
                                         const RequestCallback& callback) {
  std::string id;
  if (!input.GetString(kCommandsIdKey, &id)) {
    ErrorPtr error;
    Error::AddToPrintf(&error, FROM_HERE, errors::kInvalidParams,
                       kInvalidParamValueFormat, kCommandsIdKey, id.c_str());
    return ReturnError(*error, callback);
  }
  cloud_->CancelCommand(id, user_info,
                        base::Bind(&OnCommandRequestSucceeded, callback));
}

void PrivetHandler::HandleCheckForUpdates(const base::DictionaryValue& input,
                                          const UserInfo& user_info,
                                          const RequestCallback& callback) {
  int timeout_seconds = -1;
  input.GetInteger(kWaitTimeoutKey, &timeout_seconds);
  base::TimeDelta timeout = device_->GetHttpRequestTimeout();
  // Allow 10 seconds to cut the timeout short to make sure HTTP server doesn't
  // kill the connection before we have a chance to respond. 10 seconds chosen
  // at random here without any scientific basis for the value.
  const base::TimeDelta safety_gap = base::TimeDelta::FromSeconds(10);
  if (timeout != base::TimeDelta::Max()) {
    if (timeout > safety_gap)
      timeout -= safety_gap;
    else
      timeout = base::TimeDelta::FromSeconds(0);
  }
  if (timeout_seconds >= 0)
    timeout = std::min(timeout, base::TimeDelta::FromSeconds(timeout_seconds));
  if (timeout == base::TimeDelta{})
    return ReplyToUpdateRequest(callback);

  std::string state_fingerprint;
  std::string commands_fingerprint;
  std::string traits_fingerprint;
  std::string components_fingerprint;
  input.GetString(kStateFingerprintKey, &state_fingerprint);
  input.GetString(kCommandsFingerprintKey, &commands_fingerprint);
  input.GetString(kTraitsFingerprintKey, &traits_fingerprint);
  input.GetString(kComponentsFingerprintKey, &components_fingerprint);
  const bool ignore_state = state_fingerprint.empty();
  const bool ignore_commands = commands_fingerprint.empty();
  const bool ignore_traits = traits_fingerprint.empty();
  const bool ignore_components = components_fingerprint.empty();
  // If all fingerprints are missing, nothing to wait for, return immediately.
  if (ignore_state && ignore_commands && ignore_traits && ignore_components)
    return ReplyToUpdateRequest(callback);
  // If the current state fingerprint is different from the requested one,
  // return new fingerprints.
  if (!ignore_state && state_fingerprint != std::to_string(state_fingerprint_))
    return ReplyToUpdateRequest(callback);
  // If the current commands fingerprint is different from the requested one,
  // return new fingerprints.
  // NOTE: We are using traits fingerprint for command fingerprint as well.
  if (!ignore_commands &&
      commands_fingerprint != std::to_string(traits_fingerprint_)) {
    return ReplyToUpdateRequest(callback);
  }
  // If the current traits fingerprint is different from the requested one,
  // return new fingerprints.
  if (!ignore_traits &&
      traits_fingerprint != std::to_string(traits_fingerprint_)) {
    return ReplyToUpdateRequest(callback);
  }
  // If the current components fingerprint is different from the requested one,
  // return new fingerprints.
  if (!ignore_components &&
      components_fingerprint != std::to_string(components_fingerprint_)) {
    return ReplyToUpdateRequest(callback);
  }

  UpdateRequestParameters params;
  params.request_id = ++last_update_request_id_;
  params.callback = callback;
  params.traits_fingerprint =
      (ignore_traits && ignore_commands) ? 0 : traits_fingerprint_;
  params.state_fingerprint = ignore_state ? 0 : state_fingerprint_;
  params.components_fingerprint =
      ignore_components ? 0 : components_fingerprint_;
  update_requests_.push_back(params);
  if (timeout != base::TimeDelta::Max()) {
    device_->PostDelayedTask(
        FROM_HERE,
        base::Bind(&PrivetHandler::OnUpdateRequestTimeout,
                   weak_ptr_factory_.GetWeakPtr(), last_update_request_id_),
        timeout);
  }
}

void PrivetHandler::ReplyToUpdateRequest(
    const RequestCallback& callback) const {
  base::DictionaryValue output;
  output.SetString(kStateFingerprintKey, std::to_string(state_fingerprint_));
  output.SetString(kCommandsFingerprintKey,
                   std::to_string(traits_fingerprint_));
  output.SetString(kTraitsFingerprintKey, std::to_string(traits_fingerprint_));
  output.SetString(kComponentsFingerprintKey,
                   std::to_string(components_fingerprint_));
  callback.Run(http::kOk, output);
}

void PrivetHandler::OnUpdateRequestTimeout(int update_request_id) {
  auto pred = [update_request_id](const UpdateRequestParameters& params) {
    return params.request_id != update_request_id;
  };
  auto last =
      std::partition(update_requests_.begin(), update_requests_.end(), pred);
  for (auto p = last; p != update_requests_.end(); ++p)
    ReplyToUpdateRequest(p->callback);
  update_requests_.erase(last, update_requests_.end());
}

}  // namespace privet
}  // namespace weave
