// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBWEAVE_SRC_DEVICE_REGISTRATION_INFO_H_
#define LIBWEAVE_SRC_DEVICE_REGISTRATION_INFO_H_

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <base/callback.h>
#include <base/macros.h>
#include <base/memory/weak_ptr.h>
#include <base/time/time.h>
#include <weave/device.h>
#include <weave/error.h>
#include <weave/provider/http_client.h>

#include "src/backoff_entry.h"
#include "src/commands/cloud_command_update_interface.h"
#include "src/component_manager.h"
#include "src/config.h"
#include "src/data_encoding.h"
#include "src/notification/notification_channel.h"
#include "src/notification/notification_delegate.h"
#include "src/notification/pull_channel.h"

namespace base {
class DictionaryValue;
}  // namespace base

namespace weave {

class StateManager;

namespace provider {
class Network;
class TaskRunner;
}

namespace privet {
class AuthManager;
}

// The DeviceRegistrationInfo class represents device registration information.
class DeviceRegistrationInfo : public NotificationDelegate,
                               public CloudCommandUpdateInterface {
 public:
  using CloudRequestDoneCallback =
      base::Callback<void(const base::DictionaryValue& response,
                          ErrorPtr error)>;

  DeviceRegistrationInfo(Config* config,
                         ComponentManager* component_manager,
                         provider::TaskRunner* task_runner,
                         provider::HttpClient* http_client,
                         provider::Network* network,
                         privet::AuthManager* auth_manager);

  ~DeviceRegistrationInfo() override;

  void AddGcdStateChangedCallback(
      const Device::GcdStateChangedCallback& callback);
  void RegisterDevice(const std::string& ticket_id,
                      const DoneCallback& callback);

  void UpdateDeviceInfo(const std::string& name,
                        const std::string& description,
                        const std::string& location);
  void UpdateBaseConfig(AuthScope anonymous_access_role,
                        bool local_discovery_enabled,
                        bool local_pairing_enabled);
  bool UpdateServiceConfig(const std::string& client_id,
                           const std::string& client_secret,
                           const std::string& api_key,
                           const std::string& oauth_url,
                           const std::string& service_url,
                           const std::string& xmpp_endpoint,
                           ErrorPtr* error);

  void GetDeviceInfo(const CloudRequestDoneCallback& callback);

  // Returns the GCD service request URL. If |subpath| is specified, it is
  // appended to the base URL which is normally
  //    https://www.googleapis.com/weave/v1/".
  // If |params| are specified, each key-value pair is formatted using
  // WebParamsEncode() and appended to URL as a query
  // string.
  // So, calling:
  //    GetServiceURL("ticket", {{"key","apiKey"}})
  // will return something like:
  //    https://www.googleapis.com/weave/v1/ticket?key=apiKey
  std::string GetServiceURL(const std::string& subpath = {},
                            const WebParamList& params = {}) const;

  // Returns a service URL to access the registered device on GCD server.
  // The base URL used to construct the full URL looks like this:
  //    https://www.googleapis.com/weave/v1/devices/<cloud_id>/
  std::string GetDeviceURL(const std::string& subpath = {},
                           const WebParamList& params = {}) const;

  // Similar to GetServiceURL, GetOAuthURL() returns a URL of OAuth 2.0 server.
  // The base URL used is https://accounts.google.com/o/oauth2/.
  std::string GetOAuthURL(const std::string& subpath = {},
                          const WebParamList& params = {}) const;

  // Starts GCD device if credentials available.
  void Start();

  // Updates a command (override from CloudCommandUpdateInterface).
  void UpdateCommand(const std::string& command_id,
                     const base::DictionaryValue& command_patch,
                     const DoneCallback& callback) override;

  // TODO(vitalybuka): remove getters and pass config to dependent code.
  const Config::Settings& GetSettings() const { return config_->GetSettings(); }
  Config* GetMutableConfig() { return config_; }

  GcdState GetGcdState() const { return gcd_state_; }

 private:
  friend class DeviceRegistrationInfoTest;

  base::WeakPtr<DeviceRegistrationInfo> AsWeakPtr() {
    return weak_factory_.GetWeakPtr();
  }

  // Checks whether we have credentials generated during registration.
  bool HaveRegistrationCredentials() const;
  // Calls HaveRegistrationCredentials() and logs an error if no credentials
  // are available.
  bool VerifyRegistrationCredentials(ErrorPtr* error) const;

  // Cause DeviceRegistrationInfo to attempt to connect to cloud server on
  // its own later.
  void ScheduleCloudConnection(const base::TimeDelta& delay);

  // Initiates the connection to the cloud server.
  // Device will do required start up chores and then start to listen
  // to new commands.
  void ConnectToCloud(ErrorPtr error);
  // Notification called when ConnectToCloud() succeeds.
  void OnConnectedToCloud(ErrorPtr error);

  // Forcibly refreshes the access token.
  void RefreshAccessToken(const DoneCallback& callback);

  // Callbacks for RefreshAccessToken().
  void OnRefreshAccessTokenDone(
      const DoneCallback& callback,
      std::unique_ptr<provider::HttpClient::Response> response,
      ErrorPtr error);

  // Parse the OAuth response, and sets registration status to
  // kInvalidCredentials if our registration is no longer valid.
  std::unique_ptr<base::DictionaryValue> ParseOAuthResponse(
      const provider::HttpClient::Response& response,
      ErrorPtr* error);

  // This attempts to open a notification channel. The channel needs to be
  // restarted anytime the access_token is refreshed.
  void StartNotificationChannel();

  // Do a HTTPS request to cloud services.
  // Handles many cases like reauthorization, 5xx HTTP response codes
  // and device removal.  It is a recommended way to do cloud API
  // requests.
  // TODO(antonm): Consider moving into some other class.
  void DoCloudRequest(provider::HttpClient::Method method,
                      const std::string& url,
                      const base::DictionaryValue* body,
                      const CloudRequestDoneCallback& callback);

  // Helper for DoCloudRequest().
  struct CloudRequestData {
    provider::HttpClient::Method method;
    std::string url;
    std::string body;
    CloudRequestDoneCallback callback;
  };
  void SendCloudRequest(const std::shared_ptr<const CloudRequestData>& data);
  void OnCloudRequestDone(
      const std::shared_ptr<const CloudRequestData>& data,
      std::unique_ptr<provider::HttpClient::Response> response,
      ErrorPtr error);
  void RetryCloudRequest(const std::shared_ptr<const CloudRequestData>& data);
  void OnAccessTokenRefreshed(
      const std::shared_ptr<const CloudRequestData>& data,
      ErrorPtr error);
  void CheckAccessTokenError(ErrorPtr error);

  void UpdateDeviceResource(const DoneCallback& callback);
  void StartQueuedUpdateDeviceResource();
  void OnUpdateDeviceResourceDone(const base::DictionaryValue& device_info,
                                  ErrorPtr error);
  void OnUpdateDeviceResourceError(ErrorPtr error);

  void SendAuthInfo();
  void OnSendAuthInfoDone(const std::vector<uint8_t>& token,
                          const base::DictionaryValue& body,
                          ErrorPtr error);

  // Callback from GetDeviceInfo() to retrieve the device resource timestamp
  // and retry UpdateDeviceResource() call.
  void OnDeviceInfoRetrieved(const base::DictionaryValue& device_info,
                             ErrorPtr error);

  // Extracts the timestamp from the device resource and sets it to
  // |last_device_resource_updated_timestamp_|.
  // Returns false if the "lastUpdateTimeMs" field is not found in the device
  // resource or it is invalid.
  bool UpdateDeviceInfoTimestamp(const base::DictionaryValue& device_info);

  void FetchCommands(
      const base::Callback<void(const base::ListValue&, ErrorPtr)>& callback,
      const std::string& reason);
  void OnFetchCommandsDone(
      const base::Callback<void(const base::ListValue&, ErrorPtr)>& callback,
      const base::DictionaryValue& json,
      ErrorPtr);
  // Called when FetchCommands completes (with either success or error).
  // This method reschedules any pending/queued fetch requests.
  void OnFetchCommandsReturned();

  // Processes the command list that is fetched from the server on connection.
  // Aborts commands which are in transitional states and publishes queued
  // commands which are queued.
  void ProcessInitialCommandList(const base::ListValue& commands,
                                 ErrorPtr error);

  void PublishCommands(const base::ListValue& commands, ErrorPtr error);
  void PublishCommand(const base::DictionaryValue& command);

  // Helper function to pull the pending command list from the server using
  // FetchCommands() and make them available on D-Bus with PublishCommands().
  // |backup_fetch| is set to true when performing backup ("just-in-case")
  // command fetch while XMPP channel is up and running.
  void FetchAndPublishCommands(const std::string& reason);

  void PublishStateUpdates();
  void OnPublishStateDone(ComponentManager::UpdateID update_id,
                          const base::DictionaryValue& reply,
                          ErrorPtr error);
  void OnPublishStateError(ErrorPtr error);

  // If unrecoverable error occurred (e.g. error parsing command instance),
  // notify the server that the command is aborted by the device.
  void NotifyCommandAborted(const std::string& command_id, ErrorPtr error);

  // Builds Cloud API devices collection REST resource which matches
  // current state of the device including command definitions
  // for all supported commands and current device state.
  std::unique_ptr<base::DictionaryValue> BuildDeviceResource() const;

  void SetGcdState(GcdState new_state);
  void SetDeviceId(const std::string& cloud_id);

  // Callback called when command definitions are changed to re-publish new CDD.
  void OnTraitDefsChanged();
  void OnComponentTreeChanged();
  void OnStateChanged();

  // Overrides from NotificationDelegate.
  void OnConnected(const std::string& channel_name) override;
  void OnDisconnected() override;
  void OnPermanentFailure() override;
  void OnCommandCreated(const base::DictionaryValue& command,
                        const std::string& channel_name) override;
  void OnDeviceDeleted(const std::string& cloud_id) override;

  // Wipes out the device registration information and stops server connections.
  void RemoveCredentials();

  void RegisterDeviceError(const DoneCallback& callback, ErrorPtr error);
  void RegisterDeviceOnTicketSent(
      const std::string& ticket_id,
      const DoneCallback& callback,
      std::unique_ptr<provider::HttpClient::Response> response,
      ErrorPtr error);
  void RegisterDeviceOnTicketFinalized(
      const DoneCallback& callback,
      std::unique_ptr<provider::HttpClient::Response> response,
      ErrorPtr error);
  void RegisterDeviceOnAuthCodeSent(
      const std::string& cloud_id,
      const std::string& robot_account,
      const DoneCallback& callback,
      std::unique_ptr<provider::HttpClient::Response> response,
      ErrorPtr error);

  // Transient data
  std::string access_token_;
  base::Time access_token_expiration_;
  // The time stamp of last device resource update on the server.
  std::string last_device_resource_updated_timestamp_;
  // Set to true if the device has connected to the cloud server correctly.
  // At this point, normal state and command updates can be dispatched to the
  // server.
  bool connected_to_cloud_{false};

  // HTTP transport used for communications.
  provider::HttpClient* http_client_{nullptr};

  provider::TaskRunner* task_runner_{nullptr};

  Config* config_{nullptr};

  // Global component manager.
  ComponentManager* component_manager_{nullptr};

  // Backoff manager for DoCloudRequest() method.
  std::unique_ptr<BackoffEntry::Policy> cloud_backoff_policy_;
  std::unique_ptr<BackoffEntry> cloud_backoff_entry_;
  std::unique_ptr<BackoffEntry> oauth2_backoff_entry_;

  // Flag set to true while a device state update patch request is in flight
  // to the cloud server.
  bool device_state_update_pending_{false};

  // Set to true when command queue fetch request is in flight to the server.
  bool fetch_commands_request_sent_{false};
  // Set to true when another command queue fetch request is queued while
  // another one was in flight.
  bool fetch_commands_request_queued_{false};
  // Specifies the reason for queued command fetch request.
  std::string queued_fetch_reason_;

  using ResourceUpdateCallbackList = std::vector<DoneCallback>;
  // Callbacks for device resource update request currently in flight to the
  // cloud server.
  ResourceUpdateCallbackList in_progress_resource_update_callbacks_;
  // Callbacks for device resource update requests queued while another request
  // is in flight to the cloud server.
  ResourceUpdateCallbackList queued_resource_update_callbacks_;

  bool auth_info_update_inprogress_{false};

  std::unique_ptr<NotificationChannel> primary_notification_channel_;
  std::unique_ptr<PullChannel> pull_channel_;
  NotificationChannel* current_notification_channel_{nullptr};
  bool notification_channel_starting_{false};

  provider::Network* network_{nullptr};
  privet::AuthManager* auth_manager_{nullptr};

  // Tracks our GCD state.
  GcdState gcd_state_{GcdState::kUnconfigured};

  std::vector<Device::GcdStateChangedCallback> gcd_state_changed_callbacks_;

  base::WeakPtrFactory<DeviceRegistrationInfo> weak_factory_{this};
  DISALLOW_COPY_AND_ASSIGN(DeviceRegistrationInfo);
};

}  // namespace weave

#endif  // LIBWEAVE_SRC_DEVICE_REGISTRATION_INFO_H_
