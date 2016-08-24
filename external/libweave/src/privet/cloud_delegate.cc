// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/privet/cloud_delegate.h"

#include <map>
#include <vector>

#include <base/bind.h>
#include <base/logging.h>
#include <base/memory/weak_ptr.h>
#include <base/values.h>
#include <weave/error.h>
#include <weave/provider/task_runner.h>

#include "src/backoff_entry.h"
#include "src/component_manager.h"
#include "src/config.h"
#include "src/device_registration_info.h"
#include "src/privet/constants.h"

namespace weave {
namespace privet {

namespace {

const BackoffEntry::Policy register_backoff_policy = {0,    1000, 2.0,  0.2,
                                                      5000, -1,   false};

const int kMaxDeviceRegistrationRetries = 100;  // ~ 8 minutes @5s retries.

CommandInstance* ReturnNotFound(const std::string& command_id,
                                ErrorPtr* error) {
  Error::AddToPrintf(error, FROM_HERE, errors::kNotFound,
                     "Command not found, ID='%s'", command_id.c_str());
  return nullptr;
}

class CloudDelegateImpl : public CloudDelegate {
 public:
  CloudDelegateImpl(provider::TaskRunner* task_runner,
                    DeviceRegistrationInfo* device,
                    ComponentManager* component_manager)
      : task_runner_{task_runner},
        device_{device},
        component_manager_{component_manager} {
    device_->GetMutableConfig()->AddOnChangedCallback(base::Bind(
        &CloudDelegateImpl::OnConfigChanged, weak_factory_.GetWeakPtr()));
    device_->AddGcdStateChangedCallback(base::Bind(
        &CloudDelegateImpl::OnRegistrationChanged, weak_factory_.GetWeakPtr()));

    component_manager_->AddTraitDefChangedCallback(
        base::Bind(&CloudDelegateImpl::NotifyOnTraitDefsChanged,
                   weak_factory_.GetWeakPtr()));
    component_manager_->AddCommandAddedCallback(base::Bind(
        &CloudDelegateImpl::OnCommandAdded, weak_factory_.GetWeakPtr()));
    component_manager_->AddCommandRemovedCallback(base::Bind(
        &CloudDelegateImpl::OnCommandRemoved, weak_factory_.GetWeakPtr()));
    component_manager_->AddStateChangedCallback(base::Bind(
        &CloudDelegateImpl::NotifyOnStateChanged, weak_factory_.GetWeakPtr()));
    component_manager_->AddComponentTreeChangedCallback(
        base::Bind(&CloudDelegateImpl::NotifyOnComponentTreeChanged,
                   weak_factory_.GetWeakPtr()));
  }

  ~CloudDelegateImpl() override = default;

  std::string GetDeviceId() const override {
    return device_->GetSettings().device_id;
  }

  std::string GetModelId() const override {
    CHECK_EQ(5u, device_->GetSettings().model_id.size());
    return device_->GetSettings().model_id;
  }

  std::string GetName() const override { return device_->GetSettings().name; }

  std::string GetDescription() const override {
    return device_->GetSettings().description;
  }

  std::string GetLocation() const override {
    return device_->GetSettings().location;
  }

  void UpdateDeviceInfo(const std::string& name,
                        const std::string& description,
                        const std::string& location) override {
    device_->UpdateDeviceInfo(name, description, location);
  }

  std::string GetOemName() const override {
    return device_->GetSettings().oem_name;
  }

  std::string GetModelName() const override {
    return device_->GetSettings().model_name;
  }

  AuthScope GetAnonymousMaxScope() const override {
    return device_->GetSettings().local_anonymous_access_role;
  }

  const ConnectionState& GetConnectionState() const override {
    return connection_state_;
  }

  const SetupState& GetSetupState() const override { return setup_state_; }

  bool Setup(const std::string& ticket_id,
             const std::string& user,
             ErrorPtr* error) override {
    VLOG(1) << "GCD Setup started. ticket_id: " << ticket_id
            << ", user:" << user;
    // Set (or reset) the retry counter, since we are starting a new
    // registration process.
    registation_retry_count_ = kMaxDeviceRegistrationRetries;
    ticket_id_ = ticket_id;
    if (setup_state_.IsStatusEqual(SetupState::kInProgress)) {
      // Another registration is in progress. In case it fails, we will use
      // the new ticket ID when retrying the request.
      return true;
    }
    setup_state_ = SetupState(SetupState::kInProgress);
    setup_weak_factory_.InvalidateWeakPtrs();
    backoff_entry_.Reset();
    task_runner_->PostDelayedTask(
        FROM_HERE, base::Bind(&CloudDelegateImpl::CallManagerRegisterDevice,
                              setup_weak_factory_.GetWeakPtr()),
        {});
    // Return true because we initiated setup.
    return true;
  }

  std::string GetCloudId() const override {
    return connection_state_.status() > ConnectionState::kUnconfigured
               ? device_->GetSettings().cloud_id
               : "";
  }

  const base::DictionaryValue& GetLegacyCommandDef() const override {
    return component_manager_->GetLegacyCommandDefinitions();
  }

  const base::DictionaryValue& GetLegacyState() const override {
    return component_manager_->GetLegacyState();
  }

  const base::DictionaryValue& GetComponents() const override {
    return component_manager_->GetComponents();
  }

  const base::DictionaryValue* FindComponent(const std::string& path,
                                             ErrorPtr* error) const override {
    return component_manager_->FindComponent(path, error);
  }

  const base::DictionaryValue& GetTraits() const override {
    return component_manager_->GetTraits();
  }

  void AddCommand(const base::DictionaryValue& command,
                  const UserInfo& user_info,
                  const CommandDoneCallback& callback) override {
    CHECK(user_info.scope() != AuthScope::kNone);
    CHECK(!user_info.id().IsEmpty());

    ErrorPtr error;
    UserRole role;
    std::string str_scope = EnumToString(user_info.scope());
    if (!StringToEnum(str_scope, &role)) {
      Error::AddToPrintf(&error, FROM_HERE, errors::kInvalidParams,
                         "Invalid role: '%s'", str_scope.c_str());
      return callback.Run({}, std::move(error));
    }

    std::string id;
    auto command_instance = component_manager_->ParseCommandInstance(
        command, Command::Origin::kLocal, role, &id, &error);
    if (!command_instance)
      return callback.Run({}, std::move(error));
    component_manager_->AddCommand(std::move(command_instance));
    command_owners_[id] = user_info.id();
    callback.Run(*component_manager_->FindCommand(id)->ToJson(), nullptr);
  }

  void GetCommand(const std::string& id,
                  const UserInfo& user_info,
                  const CommandDoneCallback& callback) override {
    CHECK(user_info.scope() != AuthScope::kNone);
    ErrorPtr error;
    auto command = GetCommandInternal(id, user_info, &error);
    if (!command)
      return callback.Run({}, std::move(error));
    callback.Run(*command->ToJson(), nullptr);
  }

  void CancelCommand(const std::string& id,
                     const UserInfo& user_info,
                     const CommandDoneCallback& callback) override {
    CHECK(user_info.scope() != AuthScope::kNone);
    ErrorPtr error;
    auto command = GetCommandInternal(id, user_info, &error);
    if (!command || !command->Cancel(&error))
      return callback.Run({}, std::move(error));
    callback.Run(*command->ToJson(), nullptr);
  }

  void ListCommands(const UserInfo& user_info,
                    const CommandDoneCallback& callback) override {
    CHECK(user_info.scope() != AuthScope::kNone);

    base::ListValue list_value;

    for (const auto& it : command_owners_) {
      if (CanAccessCommand(it.second, user_info, nullptr)) {
        list_value.Append(
            component_manager_->FindCommand(it.first)->ToJson().release());
      }
    }

    base::DictionaryValue commands_json;
    commands_json.Set("commands", list_value.DeepCopy());

    callback.Run(commands_json, nullptr);
  }

 private:
  void OnCommandAdded(Command* command) {
    // Set to "" for any new unknown command.
    command_owners_.insert(std::make_pair(command->GetID(), UserAppId{}));
  }

  void OnCommandRemoved(Command* command) {
    CHECK(command_owners_.erase(command->GetID()));
  }

  void OnConfigChanged(const Settings&) { NotifyOnDeviceInfoChanged(); }

  void OnRegistrationChanged(GcdState status) {
    if (status == GcdState::kUnconfigured ||
        status == GcdState::kInvalidCredentials) {
      connection_state_ = ConnectionState{ConnectionState::kUnconfigured};
    } else if (status == GcdState::kConnecting) {
      // TODO(vitalybuka): Find conditions for kOffline.
      connection_state_ = ConnectionState{ConnectionState::kConnecting};
    } else if (status == GcdState::kConnected) {
      connection_state_ = ConnectionState{ConnectionState::kOnline};
    } else {
      ErrorPtr error;
      Error::AddToPrintf(&error, FROM_HERE, errors::kInvalidState,
                         "Unexpected registration status: %s",
                         EnumToString(status).c_str());
      connection_state_ = ConnectionState{std::move(error)};
    }
    NotifyOnDeviceInfoChanged();
  }

  void OnRegisterSuccess(const std::string& cloud_id) {
    VLOG(1) << "Device registered: " << cloud_id;
    setup_state_ = SetupState(SetupState::kSuccess);
  }

  void CallManagerRegisterDevice() {
    ErrorPtr error;
    CHECK_GE(registation_retry_count_, 0);
    if (registation_retry_count_-- == 0) {
      Error::AddTo(&error, FROM_HERE, errors::kInvalidState,
                   "Failed to register device");
      setup_state_ = SetupState{std::move(error)};
      return;
    }

    device_->RegisterDevice(ticket_id_,
                            base::Bind(&CloudDelegateImpl::RegisterDeviceDone,
                                       setup_weak_factory_.GetWeakPtr()));
  }

  void RegisterDeviceDone(ErrorPtr error) {
    if (error) {
      // Registration failed. Retry with backoff.
      backoff_entry_.InformOfRequest(false);
      return task_runner_->PostDelayedTask(
          FROM_HERE, base::Bind(&CloudDelegateImpl::CallManagerRegisterDevice,
                                setup_weak_factory_.GetWeakPtr()),
          backoff_entry_.GetTimeUntilRelease());
    }
    backoff_entry_.InformOfRequest(true);
    setup_state_ = SetupState(SetupState::kSuccess);
  }

  CommandInstance* GetCommandInternal(const std::string& command_id,
                                      const UserInfo& user_info,
                                      ErrorPtr* error) const {
    if (user_info.scope() < AuthScope::kManager) {
      auto it = command_owners_.find(command_id);
      if (it == command_owners_.end())
        return ReturnNotFound(command_id, error);
      if (CanAccessCommand(it->second, user_info, error))
        return nullptr;
    }

    auto command = component_manager_->FindCommand(command_id);
    if (!command)
      return ReturnNotFound(command_id, error);

    return command;
  }

  bool CanAccessCommand(const UserAppId& owner,
                        const UserInfo& user_info,
                        ErrorPtr* error) const {
    CHECK(user_info.scope() != AuthScope::kNone);
    CHECK(!user_info.id().IsEmpty());

    if (user_info.scope() == AuthScope::kManager ||
        (owner.type == user_info.id().type &&
         owner.user == user_info.id().user &&
         (user_info.id().app.empty() ||  // Token is not restricted to the app.
          owner.app == user_info.id().app))) {
      return true;
    }

    return Error::AddTo(error, FROM_HERE, errors::kAccessDenied,
                        "Need to be owner of the command.");
  }

  provider::TaskRunner* task_runner_{nullptr};
  DeviceRegistrationInfo* device_{nullptr};
  ComponentManager* component_manager_{nullptr};

  // Primary state of GCD.
  ConnectionState connection_state_{ConnectionState::kDisabled};

  // State of the current or last setup.
  SetupState setup_state_{SetupState::kNone};

  // Ticket ID for registering the device.
  std::string ticket_id_;

  // Number of remaining retries for device registration process.
  int registation_retry_count_{0};

  // Map of command IDs to user IDs.
  std::map<std::string, UserAppId> command_owners_;

  // Backoff entry for retrying device registration.
  BackoffEntry backoff_entry_{&register_backoff_policy};

  // |setup_weak_factory_| tracks the lifetime of callbacks used in connection
  // with a particular invocation of Setup().
  base::WeakPtrFactory<CloudDelegateImpl> setup_weak_factory_{this};
  // |weak_factory_| tracks the lifetime of |this|.
  base::WeakPtrFactory<CloudDelegateImpl> weak_factory_{this};
};

}  // namespace

CloudDelegate::CloudDelegate() {}

CloudDelegate::~CloudDelegate() {}

// static
std::unique_ptr<CloudDelegate> CloudDelegate::CreateDefault(
    provider::TaskRunner* task_runner,
    DeviceRegistrationInfo* device,
    ComponentManager* component_manager) {
  return std::unique_ptr<CloudDelegateImpl>{
      new CloudDelegateImpl{task_runner, device, component_manager}};
}

void CloudDelegate::NotifyOnDeviceInfoChanged() {
  FOR_EACH_OBSERVER(Observer, observer_list_, OnDeviceInfoChanged());
}

void CloudDelegate::NotifyOnTraitDefsChanged() {
  FOR_EACH_OBSERVER(Observer, observer_list_, OnTraitDefsChanged());
}

void CloudDelegate::NotifyOnComponentTreeChanged() {
  FOR_EACH_OBSERVER(Observer, observer_list_, OnComponentTreeChanged());
}

void CloudDelegate::NotifyOnStateChanged() {
  FOR_EACH_OBSERVER(Observer, observer_list_, OnStateChanged());
}

}  // namespace privet
}  // namespace weave
