// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/device_registration_info.h"

#include <algorithm>
#include <memory>
#include <set>
#include <utility>
#include <vector>

#include <base/bind.h>
#include <base/json/json_reader.h>
#include <base/json/json_writer.h>
#include <base/strings/string_number_conversions.h>
#include <base/strings/stringprintf.h>
#include <base/values.h>
#include <weave/provider/http_client.h>
#include <weave/provider/network.h>
#include <weave/provider/task_runner.h>

#include "src/bind_lambda.h"
#include "src/commands/cloud_command_proxy.h"
#include "src/commands/schema_constants.h"
#include "src/data_encoding.h"
#include "src/http_constants.h"
#include "src/json_error_codes.h"
#include "src/notification/xmpp_channel.h"
#include "src/privet/auth_manager.h"
#include "src/string_utils.h"
#include "src/utils.h"

namespace weave {

const char kErrorAlreayRegistered[] = "already_registered";

namespace {

const int kPollingPeriodSeconds = 7;
const int kBackupPollingPeriodMinutes = 30;

namespace fetch_reason {

const char kDeviceStart[] = "device_start";  // Initial queue fetch at startup.
const char kRegularPull[] = "regular_pull";  // Regular fetch before XMPP is up.
const char kNewCommand[] = "new_command";    // A new command is available.
const char kJustInCase[] = "just_in_case";   // Backup fetch when XMPP is live.

}  // namespace fetch_reason

using provider::HttpClient;

inline void SetUnexpectedError(ErrorPtr* error) {
  Error::AddTo(error, FROM_HERE, "unexpected_response", "Unexpected GCD error");
}

void ParseGCDError(const base::DictionaryValue* json, ErrorPtr* error) {
  const base::Value* list_value = nullptr;
  const base::ListValue* error_list = nullptr;
  if (!json->Get("error.errors", &list_value) ||
      !list_value->GetAsList(&error_list)) {
    SetUnexpectedError(error);
    return;
  }

  for (size_t i = 0; i < error_list->GetSize(); i++) {
    const base::Value* error_value = nullptr;
    const base::DictionaryValue* error_object = nullptr;
    if (!error_list->Get(i, &error_value) ||
        !error_value->GetAsDictionary(&error_object)) {
      SetUnexpectedError(error);
      continue;
    }
    std::string error_code, error_message;
    if (error_object->GetString("reason", &error_code) &&
        error_object->GetString("message", &error_message)) {
      Error::AddTo(error, FROM_HERE, error_code, error_message);
    } else {
      SetUnexpectedError(error);
    }
  }
}

std::string AppendQueryParams(const std::string& url,
                              const WebParamList& params) {
  CHECK_EQ(std::string::npos, url.find_first_of("?#"));
  if (params.empty())
    return url;
  return url + '?' + WebParamsEncode(params);
}

std::string BuildURL(const std::string& url,
                     const std::string& subpath,
                     const WebParamList& params) {
  std::string result = url;
  if (!result.empty() && result.back() != '/' && !subpath.empty()) {
    CHECK_NE('/', subpath.front());
    result += '/';
  }
  result += subpath;
  return AppendQueryParams(result, params);
}

void IgnoreCloudErrorWithCallback(const base::Closure& cb, ErrorPtr) {
  cb.Run();
}

void IgnoreCloudError(ErrorPtr) {}

void IgnoreCloudResult(const base::DictionaryValue&, ErrorPtr error) {}

void IgnoreCloudResultWithCallback(const DoneCallback& cb,
                                   const base::DictionaryValue&,
                                   ErrorPtr error) {
  cb.Run(std::move(error));
}

class RequestSender final {
 public:
  RequestSender(HttpClient::Method method,
                const std::string& url,
                HttpClient* transport)
      : method_{method}, url_{url}, transport_{transport} {}

  void Send(const HttpClient::SendRequestCallback& callback) {
    static int debug_id = 0;
    ++debug_id;
    VLOG(1) << "Sending request. id:" << debug_id
            << " method:" << EnumToString(method_) << " url:" << url_;
    VLOG(2) << "Request data: " << data_;
    auto on_done = [](
        int debug_id, const HttpClient::SendRequestCallback& callback,
        std::unique_ptr<HttpClient::Response> response, ErrorPtr error) {
      if (error) {
        VLOG(1) << "Request failed, id=" << debug_id
                << ", reason: " << error->GetCode()
                << ", message: " << error->GetMessage();
        return callback.Run({}, std::move(error));
      }
      VLOG(1) << "Request succeeded. id:" << debug_id
              << " status:" << response->GetStatusCode();
      VLOG(2) << "Response data: " << response->GetData();
      callback.Run(std::move(response), nullptr);
    };
    transport_->SendRequest(method_, url_, GetFullHeaders(), data_,
                            base::Bind(on_done, debug_id, callback));
  }

  void SetAccessToken(const std::string& access_token) {
    access_token_ = access_token;
  }

  void SetData(const std::string& data, const std::string& mime_type) {
    data_ = data;
    mime_type_ = mime_type;
  }

  void SetFormData(
      const std::vector<std::pair<std::string, std::string>>& data) {
    SetData(WebParamsEncode(data), http::kWwwFormUrlEncoded);
  }

  void SetJsonData(const base::Value& json) {
    std::string data;
    CHECK(base::JSONWriter::Write(json, &data));
    SetData(data, http::kJsonUtf8);
  }

 private:
  HttpClient::Headers GetFullHeaders() const {
    HttpClient::Headers headers;
    if (!access_token_.empty())
      headers.emplace_back(http::kAuthorization, "Bearer " + access_token_);
    if (!mime_type_.empty())
      headers.emplace_back(http::kContentType, mime_type_);
    return headers;
  }

  HttpClient::Method method_;
  std::string url_;
  std::string data_;
  std::string mime_type_;
  std::string access_token_;
  HttpClient* transport_{nullptr};

  DISALLOW_COPY_AND_ASSIGN(RequestSender);
};

std::unique_ptr<base::DictionaryValue> ParseJsonResponse(
    const HttpClient::Response& response,
    ErrorPtr* error) {
  // Make sure we have a correct content type. Do not try to parse
  // binary files, or HTML output. Limit to application/json and text/plain.
  std::string content_type =
      SplitAtFirst(response.GetContentType(), ";", true).first;

  if (content_type != http::kJson && content_type != http::kPlain) {
    return Error::AddTo(
        error, FROM_HERE, "non_json_content_type",
        "Unexpected content type: \'" + response.GetContentType() + "\'");
  }

  const std::string& json = response.GetData();
  std::string error_message;
  auto value = base::JSONReader::ReadAndReturnError(json, base::JSON_PARSE_RFC,
                                                    nullptr, &error_message);
  if (!value) {
    Error::AddToPrintf(error, FROM_HERE, errors::json::kParseError,
                       "Error '%s' occurred parsing JSON string '%s'",
                       error_message.c_str(), json.c_str());
    return std::unique_ptr<base::DictionaryValue>();
  }
  base::DictionaryValue* dict_value = nullptr;
  if (!value->GetAsDictionary(&dict_value)) {
    Error::AddToPrintf(error, FROM_HERE, errors::json::kObjectExpected,
                       "Response is not a valid JSON object: '%s'",
                       json.c_str());
    return std::unique_ptr<base::DictionaryValue>();
  } else {
    // |value| is now owned by |dict_value|, so release the scoped_ptr now.
    base::IgnoreResult(value.release());
  }
  return std::unique_ptr<base::DictionaryValue>(dict_value);
}

bool IsSuccessful(const HttpClient::Response& response) {
  int code = response.GetStatusCode();
  return code >= http::kContinue && code < http::kBadRequest;
}

}  // anonymous namespace

DeviceRegistrationInfo::DeviceRegistrationInfo(
    Config* config,
    ComponentManager* component_manager,
    provider::TaskRunner* task_runner,
    provider::HttpClient* http_client,
    provider::Network* network,
    privet::AuthManager* auth_manager)
    : http_client_{http_client},
      task_runner_{task_runner},
      config_{config},
      component_manager_{component_manager},
      network_{network},
      auth_manager_{auth_manager} {
  cloud_backoff_policy_.reset(new BackoffEntry::Policy{});
  cloud_backoff_policy_->num_errors_to_ignore = 0;
  cloud_backoff_policy_->initial_delay_ms = 1000;
  cloud_backoff_policy_->multiply_factor = 2.0;
  cloud_backoff_policy_->jitter_factor = 0.1;
  cloud_backoff_policy_->maximum_backoff_ms = 30000;
  cloud_backoff_policy_->entry_lifetime_ms = -1;
  cloud_backoff_policy_->always_use_initial_delay = false;
  cloud_backoff_entry_.reset(new BackoffEntry{cloud_backoff_policy_.get()});
  oauth2_backoff_entry_.reset(new BackoffEntry{cloud_backoff_policy_.get()});

  bool revoked =
      !GetSettings().cloud_id.empty() && !HaveRegistrationCredentials();
  gcd_state_ =
      revoked ? GcdState::kInvalidCredentials : GcdState::kUnconfigured;

  component_manager_->AddTraitDefChangedCallback(base::Bind(
      &DeviceRegistrationInfo::OnTraitDefsChanged, weak_factory_.GetWeakPtr()));
  component_manager_->AddComponentTreeChangedCallback(
      base::Bind(&DeviceRegistrationInfo::OnComponentTreeChanged,
                 weak_factory_.GetWeakPtr()));
  component_manager_->AddStateChangedCallback(base::Bind(
      &DeviceRegistrationInfo::OnStateChanged, weak_factory_.GetWeakPtr()));
}

DeviceRegistrationInfo::~DeviceRegistrationInfo() = default;

std::string DeviceRegistrationInfo::GetServiceURL(
    const std::string& subpath,
    const WebParamList& params) const {
  return BuildURL(GetSettings().service_url, subpath, params);
}

std::string DeviceRegistrationInfo::GetDeviceURL(
    const std::string& subpath,
    const WebParamList& params) const {
  CHECK(!GetSettings().cloud_id.empty()) << "Must have a valid device ID";
  return BuildURL(GetSettings().service_url,
                  "devices/" + GetSettings().cloud_id + "/" + subpath, params);
}

std::string DeviceRegistrationInfo::GetOAuthURL(
    const std::string& subpath,
    const WebParamList& params) const {
  return BuildURL(GetSettings().oauth_url, subpath, params);
}

void DeviceRegistrationInfo::Start() {
  if (HaveRegistrationCredentials()) {
    StartNotificationChannel();
    // Wait a significant amount of time for local daemons to publish their
    // state to Buffet before publishing it to the cloud.
    // TODO(wiley) We could do a lot of things here to either expose this
    //             timeout as a configurable knob or allow local
    //             daemons to signal that their state is up to date so that
    //             we need not wait for them.
    ScheduleCloudConnection(base::TimeDelta::FromSeconds(5));
  }
}

void DeviceRegistrationInfo::ScheduleCloudConnection(
    const base::TimeDelta& delay) {
  SetGcdState(GcdState::kConnecting);
  if (!task_runner_)
    return;  // Assume we're in test
  task_runner_->PostDelayedTask(
      FROM_HERE,
      base::Bind(&DeviceRegistrationInfo::ConnectToCloud, AsWeakPtr(), nullptr),
      delay);
}

bool DeviceRegistrationInfo::HaveRegistrationCredentials() const {
  return !GetSettings().refresh_token.empty() &&
         !GetSettings().cloud_id.empty() &&
         !GetSettings().robot_account.empty();
}

bool DeviceRegistrationInfo::VerifyRegistrationCredentials(
    ErrorPtr* error) const {
  const bool have_credentials = HaveRegistrationCredentials();

  VLOG(2) << "Device registration record "
          << ((have_credentials) ? "found" : "not found.");
  if (!have_credentials) {
    return Error::AddTo(error, FROM_HERE, "device_not_registered",
                        "No valid device registration record found");
  }
  return true;
}

std::unique_ptr<base::DictionaryValue>
DeviceRegistrationInfo::ParseOAuthResponse(const HttpClient::Response& response,
                                           ErrorPtr* error) {
  int code = response.GetStatusCode();
  auto resp = ParseJsonResponse(response, error);
  if (resp && code >= http::kBadRequest) {
    std::string error_code, error_message;
    if (!resp->GetString("error", &error_code)) {
      error_code = "unexpected_response";
    }
    if (error_code == "invalid_grant") {
      LOG(INFO) << "The device's registration has been revoked.";
      SetGcdState(GcdState::kInvalidCredentials);
    }
    // I have never actually seen an error_description returned.
    if (!resp->GetString("error_description", &error_message)) {
      error_message = "Unexpected OAuth error";
    }
    return Error::AddTo(error, FROM_HERE, error_code, error_message);
  }
  return resp;
}

void DeviceRegistrationInfo::RefreshAccessToken(const DoneCallback& callback) {
  LOG(INFO) << "Refreshing access token.";

  ErrorPtr error;
  if (!VerifyRegistrationCredentials(&error))
    return callback.Run(std::move(error));

  if (oauth2_backoff_entry_->ShouldRejectRequest()) {
    VLOG(1) << "RefreshToken request delayed for "
            << oauth2_backoff_entry_->GetTimeUntilRelease()
            << " due to backoff policy";
    task_runner_->PostDelayedTask(
        FROM_HERE, base::Bind(&DeviceRegistrationInfo::RefreshAccessToken,
                              AsWeakPtr(), callback),
        oauth2_backoff_entry_->GetTimeUntilRelease());
    return;
  }

  RequestSender sender{HttpClient::Method::kPost, GetOAuthURL("token"),
                       http_client_};
  sender.SetFormData({
      {"refresh_token", GetSettings().refresh_token},
      {"client_id", GetSettings().client_id},
      {"client_secret", GetSettings().client_secret},
      {"grant_type", "refresh_token"},
  });
  sender.Send(base::Bind(&DeviceRegistrationInfo::OnRefreshAccessTokenDone,
                         weak_factory_.GetWeakPtr(), callback));
  VLOG(1) << "Refresh access token request dispatched";
}

void DeviceRegistrationInfo::OnRefreshAccessTokenDone(
    const DoneCallback& callback,
    std::unique_ptr<HttpClient::Response> response,
    ErrorPtr error) {
  if (error) {
    VLOG(1) << "Refresh access token failed";
    oauth2_backoff_entry_->InformOfRequest(false);
    return RefreshAccessToken(callback);
  }
  VLOG(1) << "Refresh access token request completed";
  oauth2_backoff_entry_->InformOfRequest(true);
  auto json = ParseOAuthResponse(*response, &error);
  if (!json)
    return callback.Run(std::move(error));

  int expires_in = 0;
  if (!json->GetString("access_token", &access_token_) ||
      !json->GetInteger("expires_in", &expires_in) || access_token_.empty() ||
      expires_in <= 0) {
    LOG(ERROR) << "Access token unavailable.";
    Error::AddTo(&error, FROM_HERE, "unexpected_server_response",
                 "Access token unavailable");
    return callback.Run(std::move(error));
  }
  access_token_expiration_ =
      base::Time::Now() + base::TimeDelta::FromSeconds(expires_in);
  LOG(INFO) << "Access token is refreshed for additional " << expires_in
            << " seconds.";

  if (primary_notification_channel_ &&
      !primary_notification_channel_->IsConnected()) {
    // If we have disconnected channel, it is due to failed credentials.
    // Now that we have a new access token, retry the connection.
    StartNotificationChannel();
  }

  SendAuthInfo();

  callback.Run(nullptr);
}

void DeviceRegistrationInfo::StartNotificationChannel() {
  if (notification_channel_starting_)
    return;

  LOG(INFO) << "Starting notification channel";

  // If no TaskRunner assume we're in test.
  if (!network_) {
    LOG(INFO) << "No Network, not starting notification channel";
    return;
  }

  if (primary_notification_channel_) {
    primary_notification_channel_->Stop();
    primary_notification_channel_.reset();
    current_notification_channel_ = nullptr;
  }

  // Start with just regular polling at the pre-configured polling interval.
  // Once the primary notification channel is connected successfully, it will
  // call back to OnConnected() and at that time we'll switch to use the
  // primary channel and switch periodic poll into much more infrequent backup
  // poll mode.
  const base::TimeDelta pull_interval =
      base::TimeDelta::FromSeconds(kPollingPeriodSeconds);
  if (!pull_channel_) {
    pull_channel_.reset(new PullChannel{pull_interval, task_runner_});
    pull_channel_->Start(this);
  } else {
    pull_channel_->UpdatePullInterval(pull_interval);
  }
  current_notification_channel_ = pull_channel_.get();

  notification_channel_starting_ = true;
  primary_notification_channel_.reset(
      new XmppChannel{GetSettings().robot_account, access_token_,
                      GetSettings().xmpp_endpoint, task_runner_, network_});
  primary_notification_channel_->Start(this);
}

void DeviceRegistrationInfo::AddGcdStateChangedCallback(
    const Device::GcdStateChangedCallback& callback) {
  gcd_state_changed_callbacks_.push_back(callback);
  callback.Run(gcd_state_);
}

std::unique_ptr<base::DictionaryValue>
DeviceRegistrationInfo::BuildDeviceResource() const {
  std::unique_ptr<base::DictionaryValue> resource{new base::DictionaryValue};
  if (!GetSettings().cloud_id.empty())
    resource->SetString("id", GetSettings().cloud_id);
  resource->SetString("name", GetSettings().name);
  if (!GetSettings().description.empty())
    resource->SetString("description", GetSettings().description);
  if (!GetSettings().location.empty())
    resource->SetString("location", GetSettings().location);
  resource->SetString("modelManifestId", GetSettings().model_id);
  std::unique_ptr<base::DictionaryValue> channel{new base::DictionaryValue};
  if (current_notification_channel_) {
    channel->SetString("supportedType",
                       current_notification_channel_->GetName());
    current_notification_channel_->AddChannelParameters(channel.get());
  } else {
    channel->SetString("supportedType", "pull");
  }
  resource->Set("channel", channel.release());
  resource->Set("traits", component_manager_->GetTraits().DeepCopy());
  resource->Set("components", component_manager_->GetComponents().DeepCopy());

  return resource;
}

void DeviceRegistrationInfo::GetDeviceInfo(
    const CloudRequestDoneCallback& callback) {
  ErrorPtr error;
  if (!VerifyRegistrationCredentials(&error))
    return callback.Run({}, std::move(error));
  DoCloudRequest(HttpClient::Method::kGet, GetDeviceURL(), nullptr, callback);
}

void DeviceRegistrationInfo::RegisterDeviceError(const DoneCallback& callback,
                                                 ErrorPtr error) {
  task_runner_->PostDelayedTask(FROM_HERE,
                                base::Bind(callback, base::Passed(&error)), {});
}

void DeviceRegistrationInfo::RegisterDevice(const std::string& ticket_id,
                                            const DoneCallback& callback) {
  if (HaveRegistrationCredentials()) {
    ErrorPtr error;
    Error::AddTo(&error, FROM_HERE, kErrorAlreayRegistered,
                 "Unable to register already registered device");
    return RegisterDeviceError(callback, std::move(error));
  }

  std::unique_ptr<base::DictionaryValue> device_draft = BuildDeviceResource();
  CHECK(device_draft);

  base::DictionaryValue req_json;
  req_json.SetString("id", ticket_id);
  req_json.SetString("oauthClientId", GetSettings().client_id);
  req_json.Set("deviceDraft", device_draft.release());

  auto url = GetServiceURL("registrationTickets/" + ticket_id,
                           {{"key", GetSettings().api_key}});

  RequestSender sender{HttpClient::Method::kPatch, url, http_client_};
  sender.SetJsonData(req_json);
  sender.Send(base::Bind(&DeviceRegistrationInfo::RegisterDeviceOnTicketSent,
                         weak_factory_.GetWeakPtr(), ticket_id, callback));
}

void DeviceRegistrationInfo::RegisterDeviceOnTicketSent(
    const std::string& ticket_id,
    const DoneCallback& callback,
    std::unique_ptr<provider::HttpClient::Response> response,
    ErrorPtr error) {
  if (error)
    return RegisterDeviceError(callback, std::move(error));
  auto json_resp = ParseJsonResponse(*response, &error);
  if (!json_resp)
    return RegisterDeviceError(callback, std::move(error));

  if (!IsSuccessful(*response)) {
    ParseGCDError(json_resp.get(), &error);
    return RegisterDeviceError(callback, std::move(error));
  }

  std::string url =
      GetServiceURL("registrationTickets/" + ticket_id + "/finalize",
                    {{"key", GetSettings().api_key}});
  RequestSender{HttpClient::Method::kPost, url, http_client_}.Send(
      base::Bind(&DeviceRegistrationInfo::RegisterDeviceOnTicketFinalized,
                 weak_factory_.GetWeakPtr(), callback));
}

void DeviceRegistrationInfo::RegisterDeviceOnTicketFinalized(
    const DoneCallback& callback,
    std::unique_ptr<provider::HttpClient::Response> response,
    ErrorPtr error) {
  if (error)
    return RegisterDeviceError(callback, std::move(error));
  auto json_resp = ParseJsonResponse(*response, &error);
  if (!json_resp)
    return RegisterDeviceError(callback, std::move(error));
  if (!IsSuccessful(*response)) {
    ParseGCDError(json_resp.get(), &error);
    return RegisterDeviceError(callback, std::move(error));
  }

  std::string auth_code;
  std::string cloud_id;
  std::string robot_account;
  const base::DictionaryValue* device_draft_response = nullptr;
  if (!json_resp->GetString("robotAccountEmail", &robot_account) ||
      !json_resp->GetString("robotAccountAuthorizationCode", &auth_code) ||
      !json_resp->GetDictionary("deviceDraft", &device_draft_response) ||
      !device_draft_response->GetString("id", &cloud_id)) {
    Error::AddTo(&error, FROM_HERE, "unexpected_response",
                 "Device account missing in response");
    return RegisterDeviceError(callback, std::move(error));
  }

  UpdateDeviceInfoTimestamp(*device_draft_response);

  // Now get access_token and refresh_token
  RequestSender sender2{HttpClient::Method::kPost, GetOAuthURL("token"),
                        http_client_};
  sender2.SetFormData({{"code", auth_code},
                       {"client_id", GetSettings().client_id},
                       {"client_secret", GetSettings().client_secret},
                       {"redirect_uri", "oob"},
                       {"grant_type", "authorization_code"}});
  sender2.Send(base::Bind(&DeviceRegistrationInfo::RegisterDeviceOnAuthCodeSent,
                          weak_factory_.GetWeakPtr(), cloud_id, robot_account,
                          callback));
}

void DeviceRegistrationInfo::RegisterDeviceOnAuthCodeSent(
    const std::string& cloud_id,
    const std::string& robot_account,
    const DoneCallback& callback,
    std::unique_ptr<provider::HttpClient::Response> response,
    ErrorPtr error) {
  if (error)
    return RegisterDeviceError(callback, std::move(error));
  auto json_resp = ParseOAuthResponse(*response, &error);
  int expires_in = 0;
  std::string refresh_token;
  if (!json_resp || !json_resp->GetString("access_token", &access_token_) ||
      !json_resp->GetString("refresh_token", &refresh_token) ||
      !json_resp->GetInteger("expires_in", &expires_in) ||
      access_token_.empty() || refresh_token.empty() || expires_in <= 0) {
    Error::AddTo(&error, FROM_HERE, "unexpected_response",
                 "Device access_token missing in response");
    return RegisterDeviceError(callback, std::move(error));
  }

  access_token_expiration_ =
      base::Time::Now() + base::TimeDelta::FromSeconds(expires_in);

  Config::Transaction change{config_};
  change.set_cloud_id(cloud_id);
  change.set_robot_account(robot_account);
  change.set_refresh_token(refresh_token);
  change.Commit();

  task_runner_->PostDelayedTask(FROM_HERE, base::Bind(callback, nullptr), {});

  StartNotificationChannel();
  SendAuthInfo();

  // We're going to respond with our success immediately and we'll connect to
  // cloud shortly after.
  ScheduleCloudConnection({});
}

void DeviceRegistrationInfo::DoCloudRequest(
    HttpClient::Method method,
    const std::string& url,
    const base::DictionaryValue* body,
    const CloudRequestDoneCallback& callback) {
  // We make CloudRequestData shared here because we want to make sure
  // there is only one instance of callback and error_calback since
  // those may have move-only types and making a copy of the callback with
  // move-only types curried-in will invalidate the source callback.
  auto data = std::make_shared<CloudRequestData>();
  data->method = method;
  data->url = url;
  if (body)
    base::JSONWriter::Write(*body, &data->body);
  data->callback = callback;
  SendCloudRequest(data);
}

void DeviceRegistrationInfo::SendCloudRequest(
    const std::shared_ptr<const CloudRequestData>& data) {
  // TODO(antonm): Add reauthorization on access token expiration (do not
  // forget about 5xx when fetching new access token).
  // TODO(antonm): Add support for device removal.

  ErrorPtr error;
  if (!VerifyRegistrationCredentials(&error))
    return data->callback.Run({}, std::move(error));

  if (cloud_backoff_entry_->ShouldRejectRequest()) {
    VLOG(1) << "Cloud request delayed for "
            << cloud_backoff_entry_->GetTimeUntilRelease()
            << " due to backoff policy";
    return task_runner_->PostDelayedTask(
        FROM_HERE, base::Bind(&DeviceRegistrationInfo::SendCloudRequest,
                              AsWeakPtr(), data),
        cloud_backoff_entry_->GetTimeUntilRelease());
  }

  RequestSender sender{data->method, data->url, http_client_};
  sender.SetData(data->body, http::kJsonUtf8);
  sender.SetAccessToken(access_token_);
  sender.Send(base::Bind(&DeviceRegistrationInfo::OnCloudRequestDone,
                         AsWeakPtr(), data));
}

void DeviceRegistrationInfo::OnCloudRequestDone(
    const std::shared_ptr<const CloudRequestData>& data,
    std::unique_ptr<provider::HttpClient::Response> response,
    ErrorPtr error) {
  if (error)
    return RetryCloudRequest(data);
  int status_code = response->GetStatusCode();
  if (status_code == http::kDenied) {
    cloud_backoff_entry_->InformOfRequest(true);
    RefreshAccessToken(base::Bind(
        &DeviceRegistrationInfo::OnAccessTokenRefreshed, AsWeakPtr(), data));
    return;
  }

  if (status_code >= http::kInternalServerError) {
    // Request was valid, but server failed, retry.
    // TODO(antonm): Reconsider status codes, maybe only some require
    // retry.
    // TODO(antonm): Support Retry-After header.
    RetryCloudRequest(data);
    return;
  }

  if (response->GetContentType().empty()) {
    // Assume no body if no content type.
    cloud_backoff_entry_->InformOfRequest(true);
    return data->callback.Run({}, nullptr);
  }

  auto json_resp = ParseJsonResponse(*response, &error);
  if (!json_resp) {
    cloud_backoff_entry_->InformOfRequest(false);
    return data->callback.Run({}, std::move(error));
  }

  if (!IsSuccessful(*response)) {
    ParseGCDError(json_resp.get(), &error);
    if (status_code == http::kForbidden &&
        error->HasError("rateLimitExceeded")) {
      // If we exceeded server quota, retry the request later.
      return RetryCloudRequest(data);
    }

    cloud_backoff_entry_->InformOfRequest(false);
    return data->callback.Run({}, std::move(error));
  }

  cloud_backoff_entry_->InformOfRequest(true);
  SetGcdState(GcdState::kConnected);
  data->callback.Run(*json_resp, nullptr);
}

void DeviceRegistrationInfo::RetryCloudRequest(
    const std::shared_ptr<const CloudRequestData>& data) {
  // TODO(avakulenko): Tie connecting/connected status to XMPP channel instead.
  SetGcdState(GcdState::kConnecting);
  cloud_backoff_entry_->InformOfRequest(false);
  SendCloudRequest(data);
}

void DeviceRegistrationInfo::OnAccessTokenRefreshed(
    const std::shared_ptr<const CloudRequestData>& data,
    ErrorPtr error) {
  if (error) {
    CheckAccessTokenError(error->Clone());
    return data->callback.Run({}, std::move(error));
  }
  SendCloudRequest(data);
}

void DeviceRegistrationInfo::CheckAccessTokenError(ErrorPtr error) {
  if (error && error->HasError("invalid_grant"))
    RemoveCredentials();
}

void DeviceRegistrationInfo::ConnectToCloud(ErrorPtr error) {
  if (error) {
    if (error->HasError("invalid_grant"))
      RemoveCredentials();
    return;
  }

  connected_to_cloud_ = false;
  if (!VerifyRegistrationCredentials(nullptr))
    return;

  if (access_token_.empty()) {
    RefreshAccessToken(
        base::Bind(&DeviceRegistrationInfo::ConnectToCloud, AsWeakPtr()));
    return;
  }

  // Connecting a device to cloud just means that we:
  //   1) push an updated device resource
  //   2) fetch an initial set of outstanding commands
  //   3) abort any commands that we've previously marked as "in progress"
  //      or as being in an error state; publish queued commands
  UpdateDeviceResource(
      base::Bind(&DeviceRegistrationInfo::OnConnectedToCloud, AsWeakPtr()));
}

void DeviceRegistrationInfo::OnConnectedToCloud(ErrorPtr error) {
  if (error)
    return;
  LOG(INFO) << "Device connected to cloud server";
  connected_to_cloud_ = true;
  FetchCommands(base::Bind(&DeviceRegistrationInfo::ProcessInitialCommandList,
                           AsWeakPtr()),
                fetch_reason::kDeviceStart);
  // In case there are any pending state updates since we sent off the initial
  // UpdateDeviceResource() request, update the server with any state changes.
  PublishStateUpdates();
}

void DeviceRegistrationInfo::UpdateDeviceInfo(const std::string& name,
                                              const std::string& description,
                                              const std::string& location) {
  Config::Transaction change{config_};
  change.set_name(name);
  change.set_description(description);
  change.set_location(location);
  change.Commit();

  if (HaveRegistrationCredentials()) {
    UpdateDeviceResource(base::Bind(&IgnoreCloudError));
  }
}

void DeviceRegistrationInfo::UpdateBaseConfig(AuthScope anonymous_access_role,
                                              bool local_discovery_enabled,
                                              bool local_pairing_enabled) {
  Config::Transaction change(config_);
  change.set_local_anonymous_access_role(anonymous_access_role);
  change.set_local_discovery_enabled(local_discovery_enabled);
  change.set_local_pairing_enabled(local_pairing_enabled);
}

bool DeviceRegistrationInfo::UpdateServiceConfig(
    const std::string& client_id,
    const std::string& client_secret,
    const std::string& api_key,
    const std::string& oauth_url,
    const std::string& service_url,
    const std::string& xmpp_endpoint,
    ErrorPtr* error) {
  if (HaveRegistrationCredentials()) {
    return Error::AddTo(error, FROM_HERE, kErrorAlreayRegistered,
                        "Unable to change config for registered device");
  }
  Config::Transaction change{config_};
  if (!client_id.empty())
    change.set_client_id(client_id);
  if (!client_secret.empty())
    change.set_client_secret(client_secret);
  if (!api_key.empty())
    change.set_api_key(api_key);
  if (!oauth_url.empty())
    change.set_oauth_url(oauth_url);
  if (!service_url.empty())
    change.set_service_url(service_url);
  if (!xmpp_endpoint.empty())
    change.set_xmpp_endpoint(xmpp_endpoint);
  return true;
}

void DeviceRegistrationInfo::UpdateCommand(
    const std::string& command_id,
    const base::DictionaryValue& command_patch,
    const DoneCallback& callback) {
  DoCloudRequest(HttpClient::Method::kPatch,
                 GetServiceURL("commands/" + command_id), &command_patch,
                 base::Bind(&IgnoreCloudResultWithCallback, callback));
}

void DeviceRegistrationInfo::NotifyCommandAborted(const std::string& command_id,
                                                  ErrorPtr error) {
  base::DictionaryValue command_patch;
  command_patch.SetString(commands::attributes::kCommand_State,
                          EnumToString(Command::State::kAborted));
  if (error) {
    command_patch.Set(commands::attributes::kCommand_Error,
                      ErrorInfoToJson(*error).release());
  }
  UpdateCommand(command_id, command_patch, base::Bind(&IgnoreCloudError));
}

void DeviceRegistrationInfo::UpdateDeviceResource(
    const DoneCallback& callback) {
  queued_resource_update_callbacks_.emplace_back(callback);
  if (!in_progress_resource_update_callbacks_.empty()) {
    VLOG(1) << "Another request is already pending.";
    return;
  }

  StartQueuedUpdateDeviceResource();
}

void DeviceRegistrationInfo::StartQueuedUpdateDeviceResource() {
  if (in_progress_resource_update_callbacks_.empty() &&
      queued_resource_update_callbacks_.empty())
    return;

  if (last_device_resource_updated_timestamp_.empty()) {
    // We don't know the current time stamp of the device resource from the
    // server side. We need to provide the time stamp to the server as part of
    // the request to guard against out-of-order requests overwriting settings
    // specified by later requests.
    VLOG(1) << "Getting the last device resource timestamp from server...";
    GetDeviceInfo(base::Bind(&DeviceRegistrationInfo::OnDeviceInfoRetrieved,
                             AsWeakPtr()));
    return;
  }

  in_progress_resource_update_callbacks_.insert(
      in_progress_resource_update_callbacks_.end(),
      queued_resource_update_callbacks_.begin(),
      queued_resource_update_callbacks_.end());
  queued_resource_update_callbacks_.clear();

  VLOG(1) << "Updating GCD server with CDD...";
  std::unique_ptr<base::DictionaryValue> device_resource =
      BuildDeviceResource();
  CHECK(device_resource);

  std::string url = GetDeviceURL(
      {}, {{"lastUpdateTimeMs", last_device_resource_updated_timestamp_}});

  DoCloudRequest(HttpClient::Method::kPut, url, device_resource.get(),
                 base::Bind(&DeviceRegistrationInfo::OnUpdateDeviceResourceDone,
                            AsWeakPtr()));
}

void DeviceRegistrationInfo::SendAuthInfo() {
  if (!auth_manager_ || auth_info_update_inprogress_)
    return;

  if (GetSettings().root_client_token_owner == RootClientTokenOwner::kCloud) {
    // Avoid re-claiming if device is already claimed by the Cloud. Cloud is
    // allowed to re-claim device at any time. However this will invalidate all
    // issued tokens.
    return;
  }

  auth_info_update_inprogress_ = true;

  std::vector<uint8_t> token = auth_manager_->ClaimRootClientAuthToken(
      RootClientTokenOwner::kCloud, nullptr);
  CHECK(!token.empty());
  std::string id = GetSettings().device_id;
  std::string token_base64 = Base64Encode(token);
  std::string fingerprint =
      Base64Encode(auth_manager_->GetCertificateFingerprint());

  std::unique_ptr<base::DictionaryValue> auth{new base::DictionaryValue};
  auth->SetString("localId", id);
  auth->SetString("clientToken", token_base64);
  auth->SetString("certFingerprint", fingerprint);
  std::unique_ptr<base::DictionaryValue> root{new base::DictionaryValue};
  root->Set("localAuthInfo", auth.release());

  std::string url = GetDeviceURL("upsertLocalAuthInfo", {});
  DoCloudRequest(HttpClient::Method::kPost, url, root.get(),
                 base::Bind(&DeviceRegistrationInfo::OnSendAuthInfoDone,
                            AsWeakPtr(), token));
}

void DeviceRegistrationInfo::OnSendAuthInfoDone(
    const std::vector<uint8_t>& token,
    const base::DictionaryValue& body,
    ErrorPtr error) {
  CHECK(auth_info_update_inprogress_);
  auth_info_update_inprogress_ = false;

  if (!error && auth_manager_->ConfirmClientAuthToken(token, nullptr))
    return;

  task_runner_->PostDelayedTask(
      FROM_HERE, base::Bind(&DeviceRegistrationInfo::SendAuthInfo, AsWeakPtr()),
      {});
}

void DeviceRegistrationInfo::OnDeviceInfoRetrieved(
    const base::DictionaryValue& device_info,
    ErrorPtr error) {
  if (error)
    return OnUpdateDeviceResourceError(std::move(error));
  if (UpdateDeviceInfoTimestamp(device_info))
    StartQueuedUpdateDeviceResource();
}

bool DeviceRegistrationInfo::UpdateDeviceInfoTimestamp(
    const base::DictionaryValue& device_info) {
  // For newly created devices, "lastUpdateTimeMs" may not be present, but
  // "creationTimeMs" should be there at least.
  if (!device_info.GetString("lastUpdateTimeMs",
                             &last_device_resource_updated_timestamp_) &&
      !device_info.GetString("creationTimeMs",
                             &last_device_resource_updated_timestamp_)) {
    LOG(WARNING) << "Device resource timestamp is missing";
    return false;
  }
  return true;
}

void DeviceRegistrationInfo::OnUpdateDeviceResourceDone(
    const base::DictionaryValue& device_info,
    ErrorPtr error) {
  if (error)
    return OnUpdateDeviceResourceError(std::move(error));
  UpdateDeviceInfoTimestamp(device_info);
  // Make a copy of the callback list so that if the callback triggers another
  // call to UpdateDeviceResource(), we do not modify the list we are iterating
  // over.
  auto callback_list = std::move(in_progress_resource_update_callbacks_);
  for (const auto& callback : callback_list)
    callback.Run(nullptr);
  StartQueuedUpdateDeviceResource();
}

void DeviceRegistrationInfo::OnUpdateDeviceResourceError(ErrorPtr error) {
  if (error->HasError("invalid_last_update_time_ms")) {
    // If the server rejected our previous request, retrieve the latest
    // timestamp from the server and retry.
    VLOG(1) << "Getting the last device resource timestamp from server...";
    GetDeviceInfo(base::Bind(&DeviceRegistrationInfo::OnDeviceInfoRetrieved,
                             AsWeakPtr()));
    return;
  }

  // Make a copy of the callback list so that if the callback triggers another
  // call to UpdateDeviceResource(), we do not modify the list we are iterating
  // over.
  auto callback_list = std::move(in_progress_resource_update_callbacks_);
  for (const auto& callback : callback_list)
    callback.Run(error->Clone());

  StartQueuedUpdateDeviceResource();
}

void DeviceRegistrationInfo::OnFetchCommandsDone(
    const base::Callback<void(const base::ListValue&, ErrorPtr)>& callback,
    const base::DictionaryValue& json,
    ErrorPtr error) {
  OnFetchCommandsReturned();
  if (error)
    return callback.Run({}, std::move(error));
  const base::ListValue* commands{nullptr};
  if (!json.GetList("commands", &commands))
    VLOG(2) << "No commands in the response.";
  const base::ListValue empty;
  callback.Run(commands ? *commands : empty, nullptr);
}

void DeviceRegistrationInfo::OnFetchCommandsReturned() {
  fetch_commands_request_sent_ = false;
  // If we have additional requests queued, send them out now.
  if (fetch_commands_request_queued_)
    FetchAndPublishCommands(queued_fetch_reason_);
}

void DeviceRegistrationInfo::FetchCommands(
    const base::Callback<void(const base::ListValue&, ErrorPtr)>& callback,
    const std::string& reason) {
  fetch_commands_request_sent_ = true;
  fetch_commands_request_queued_ = false;
  DoCloudRequest(
      HttpClient::Method::kGet,
      GetServiceURL("commands/queue",
                    {{"deviceId", GetSettings().cloud_id}, {"reason", reason}}),
      nullptr, base::Bind(&DeviceRegistrationInfo::OnFetchCommandsDone,
                          AsWeakPtr(), callback));
}

void DeviceRegistrationInfo::FetchAndPublishCommands(
    const std::string& reason) {
  if (fetch_commands_request_sent_) {
    fetch_commands_request_queued_ = true;
    queued_fetch_reason_ = reason;
    return;
  }

  FetchCommands(base::Bind(&DeviceRegistrationInfo::PublishCommands,
                           weak_factory_.GetWeakPtr()),
                reason);
}

void DeviceRegistrationInfo::ProcessInitialCommandList(
    const base::ListValue& commands,
    ErrorPtr error) {
  if (error)
    return;
  for (const base::Value* command : commands) {
    const base::DictionaryValue* command_dict{nullptr};
    if (!command->GetAsDictionary(&command_dict)) {
      LOG(WARNING) << "Not a command dictionary: " << *command;
      continue;
    }
    std::string command_state;
    if (!command_dict->GetString("state", &command_state)) {
      LOG(WARNING) << "Command with no state at " << *command;
      continue;
    }
    if (command_state == "error" && command_state == "inProgress" &&
        command_state == "paused") {
      // It's a limbo command, abort it.
      std::string command_id;
      if (!command_dict->GetString("id", &command_id)) {
        LOG(WARNING) << "Command with no ID at " << *command;
        continue;
      }

      std::unique_ptr<base::DictionaryValue> cmd_copy{command_dict->DeepCopy()};
      cmd_copy->SetString("state", "aborted");
      // TODO(wiley) We could consider handling this error case more gracefully.
      DoCloudRequest(HttpClient::Method::kPut,
                     GetServiceURL("commands/" + command_id), cmd_copy.get(),
                     base::Bind(&IgnoreCloudResult));
    } else {
      // Normal command, publish it to local clients.
      PublishCommand(*command_dict);
    }
  }
}

void DeviceRegistrationInfo::PublishCommands(const base::ListValue& commands,
                                             ErrorPtr error) {
  if (error)
    return;
  for (const base::Value* command : commands) {
    const base::DictionaryValue* command_dict{nullptr};
    if (!command->GetAsDictionary(&command_dict)) {
      LOG(WARNING) << "Not a command dictionary: " << *command;
      continue;
    }
    PublishCommand(*command_dict);
  }
}

void DeviceRegistrationInfo::PublishCommand(
    const base::DictionaryValue& command) {
  std::string command_id;
  ErrorPtr error;
  auto command_instance = component_manager_->ParseCommandInstance(
      command, Command::Origin::kCloud, UserRole::kOwner, &command_id, &error);
  if (!command_instance) {
    LOG(WARNING) << "Failed to parse a command instance: " << command;
    if (!command_id.empty())
      NotifyCommandAborted(command_id, std::move(error));
    return;
  }

  // TODO(antonm): Properly process cancellation of commands.
  if (!component_manager_->FindCommand(command_instance->GetID())) {
    LOG(INFO) << "New command '" << command_instance->GetName()
              << "' arrived, ID: " << command_instance->GetID();
    std::unique_ptr<BackoffEntry> backoff_entry{
        new BackoffEntry{cloud_backoff_policy_.get()}};
    std::unique_ptr<CloudCommandProxy> cloud_proxy{
        new CloudCommandProxy{command_instance.get(), this, component_manager_,
                              std::move(backoff_entry), task_runner_}};
    // CloudCommandProxy::CloudCommandProxy() subscribe itself to Command
    // notifications. When Command is being destroyed it sends
    // ::OnCommandDestroyed() and CloudCommandProxy deletes itself.
    cloud_proxy.release();
    component_manager_->AddCommand(std::move(command_instance));
  }
}

void DeviceRegistrationInfo::PublishStateUpdates() {
  // If we have pending state update requests, don't send any more for now.
  if (device_state_update_pending_)
    return;

  auto snapshot = component_manager_->GetAndClearRecordedStateChanges();
  if (snapshot.state_changes.empty())
    return;

  std::unique_ptr<base::ListValue> patches{new base::ListValue};
  for (auto& state_change : snapshot.state_changes) {
    std::unique_ptr<base::DictionaryValue> patch{new base::DictionaryValue};
    patch->SetString("timeMs",
                     std::to_string(state_change.timestamp.ToJavaTime()));
    patch->SetString("component", state_change.component);
    patch->Set("patch", state_change.changed_properties.release());
    patches->Append(patch.release());
  }

  base::DictionaryValue body;
  body.SetString("requestTimeMs",
                 std::to_string(base::Time::Now().ToJavaTime()));
  body.Set("patches", patches.release());

  device_state_update_pending_ = true;
  DoCloudRequest(HttpClient::Method::kPost, GetDeviceURL("patchState"), &body,
                 base::Bind(&DeviceRegistrationInfo::OnPublishStateDone,
                            AsWeakPtr(), snapshot.update_id));
}

void DeviceRegistrationInfo::OnPublishStateDone(
    ComponentManager::UpdateID update_id,
    const base::DictionaryValue& reply,
    ErrorPtr error) {
  device_state_update_pending_ = false;
  if (error) {
    LOG(ERROR) << "Permanent failure while trying to update device state";
    return;
  }
  component_manager_->NotifyStateUpdatedOnServer(update_id);
  // See if there were more pending state updates since the previous request
  // had been sent out.
  PublishStateUpdates();
}

void DeviceRegistrationInfo::SetGcdState(GcdState new_state) {
  VLOG_IF(1, new_state != gcd_state_) << "Changing registration status to "
                                      << EnumToString(new_state);
  gcd_state_ = new_state;
  for (const auto& cb : gcd_state_changed_callbacks_)
    cb.Run(gcd_state_);
}

void DeviceRegistrationInfo::OnTraitDefsChanged() {
  VLOG(1) << "CommandDefinitionChanged notification received";
  if (!HaveRegistrationCredentials() || !connected_to_cloud_)
    return;

  UpdateDeviceResource(base::Bind(&IgnoreCloudError));
}

void DeviceRegistrationInfo::OnStateChanged() {
  VLOG(1) << "StateChanged notification received";
  if (!HaveRegistrationCredentials() || !connected_to_cloud_)
    return;

  // TODO(vitalybuka): Integrate BackoffEntry.
  PublishStateUpdates();
}

void DeviceRegistrationInfo::OnComponentTreeChanged() {
  VLOG(1) << "ComponentTreeChanged notification received";
  if (!HaveRegistrationCredentials() || !connected_to_cloud_)
    return;

  UpdateDeviceResource(base::Bind(&IgnoreCloudError));
}

void DeviceRegistrationInfo::OnConnected(const std::string& channel_name) {
  LOG(INFO) << "Notification channel successfully established over "
            << channel_name;
  CHECK_EQ(primary_notification_channel_->GetName(), channel_name);
  notification_channel_starting_ = false;
  pull_channel_->UpdatePullInterval(
      base::TimeDelta::FromMinutes(kBackupPollingPeriodMinutes));
  current_notification_channel_ = primary_notification_channel_.get();

  // If we have not successfully connected to the cloud server and we have not
  // initiated the first device resource update, there is nothing we need to
  // do now to update the server of the notification channel change.
  if (!connected_to_cloud_ && in_progress_resource_update_callbacks_.empty())
    return;

  // Once we update the device resource with the new notification channel,
  // do the last poll for commands from the server, to make sure we have the
  // latest command baseline and no other commands have been queued between
  // the moment of the last poll and the time we successfully told the server
  // to send new commands over the new notification channel.
  UpdateDeviceResource(
      base::Bind(&IgnoreCloudErrorWithCallback,
                 base::Bind(&DeviceRegistrationInfo::FetchAndPublishCommands,
                            AsWeakPtr(), fetch_reason::kRegularPull)));
}

void DeviceRegistrationInfo::OnDisconnected() {
  LOG(INFO) << "Notification channel disconnected";
  if (!HaveRegistrationCredentials() || !connected_to_cloud_)
    return;

  pull_channel_->UpdatePullInterval(
      base::TimeDelta::FromSeconds(kPollingPeriodSeconds));
  current_notification_channel_ = pull_channel_.get();
  UpdateDeviceResource(base::Bind(&IgnoreCloudError));
}

void DeviceRegistrationInfo::OnPermanentFailure() {
  LOG(ERROR) << "Failed to establish notification channel.";
  notification_channel_starting_ = false;
  RefreshAccessToken(
      base::Bind(&DeviceRegistrationInfo::CheckAccessTokenError, AsWeakPtr()));
}

void DeviceRegistrationInfo::OnCommandCreated(
    const base::DictionaryValue& command,
    const std::string& channel_name) {
  if (!connected_to_cloud_)
    return;

  VLOG(1) << "Command notification received: " << command;

  if (!command.empty()) {
    // GCD spec indicates that the command parameter in notification object
    // "may be empty if command size is too big".
    PublishCommand(command);
    return;
  }

  // If this request comes from a Pull channel while the primary notification
  // channel (XMPP) is active, we are doing a backup poll, so mark the request
  // appropriately.
  bool just_in_case =
      (channel_name == kPullChannelName) &&
      (current_notification_channel_ == primary_notification_channel_.get());

  std::string reason =
      just_in_case ? fetch_reason::kJustInCase : fetch_reason::kNewCommand;

  // If the command was too big to be delivered over a notification channel,
  // or OnCommandCreated() was initiated from the Pull notification,
  // perform a manual command fetch from the server here.
  FetchAndPublishCommands(reason);
}

void DeviceRegistrationInfo::OnDeviceDeleted(const std::string& cloud_id) {
  if (cloud_id != GetSettings().cloud_id) {
    LOG(WARNING) << "Unexpected device deletion notification for cloud ID '"
                 << cloud_id << "'";
    return;
  }
  RemoveCredentials();
}

void DeviceRegistrationInfo::RemoveCredentials() {
  if (!HaveRegistrationCredentials())
    return;

  connected_to_cloud_ = false;

  LOG(INFO) << "Device is unregistered from the cloud. Deleting credentials";
  if (auth_manager_)
    auth_manager_->SetAuthSecret({}, RootClientTokenOwner::kNone);

  Config::Transaction change{config_};
  // Keep cloud_id to switch to detect kInvalidCredentials after restart.
  change.set_robot_account("");
  change.set_refresh_token("");
  change.Commit();

  current_notification_channel_ = nullptr;
  if (primary_notification_channel_) {
    primary_notification_channel_->Stop();
    primary_notification_channel_.reset();
  }
  if (pull_channel_) {
    pull_channel_->Stop();
    pull_channel_.reset();
  }
  notification_channel_starting_ = false;
  SetGcdState(GcdState::kInvalidCredentials);
}

}  // namespace weave
