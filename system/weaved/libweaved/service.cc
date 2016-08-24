// Copyright 2016 The Android Open Source Project
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

#include "libweaved/service.h"

#include <algorithm>

#include <base/bind.h>
#include <base/memory/weak_ptr.h>
#include <base/strings/stringprintf.h>
#include <binderwrapper/binder_wrapper.h>
#include <brillo/message_loops/message_loop.h>

#include "android/weave/BnWeaveClient.h"
#include "android/weave/BnWeaveServiceManagerNotificationListener.h"
#include "android/weave/IWeaveCommand.h"
#include "android/weave/IWeaveService.h"
#include "android/weave/IWeaveServiceManager.h"
#include "common/binder_constants.h"
#include "common/binder_utils.h"

using weaved::binder_utils::StatusToError;
using weaved::binder_utils::ToString;
using weaved::binder_utils::ToString16;

// The semantic of weaved connection is a bit complicated and that's why we have
// the numerous classes defined here.
// When the client wants to connect to weaved they would call Service::Connect
// and provide a callback to be invoked when the connection is fully established
// and ready to be used.
//
// Service::Connect() creates an instance of ServiceImpl class and sets the only
// strong pointer into ServiceSubscription class which is returned to the client
// as std::unqiue_ptr<Service::Subscription>. This allows us to hide the actual
// service object from the client until the connection is fully ready to be
// used, and at the same time give the client an exclusive ownership of the
// connection. They are free to destroy the Subscription and abort the
// connection at any point.
//
// At the same time an asynchronous process to establish a connection to weaved
// over binder is initiated. ServiceImpl periodically tries to get hold of
// IWeaveServiceManager binder object from binder service manager. Once this
// succeeds, we know that weaved is running. We create a callback binder object,
// WeaveClient, which implements IWeaveClient binder interface and pass it to
// weaved in IWeaveServiceManager::connect() method. The weaved daemon keeps the
// list of all the clients registered with it for two reasons:
//    1. It watches each client for death notifications and cleans up the
//       resources added by the client (e.g. weave components) when the client
//       dies.
//    2. It notifies the client of weaved being ready to talk to (by calling
//       onServiceConnected callback) and when new weave commands are available
//       for the client (via onCommand callback).
// When weaved is fully initialized (which can take some time after the daemon
// physically starts up), it invokes IWeaveClient::onServiceConnection on each
// client and passes a unique copy of IWeaveService to each of the client.
// The clients will use its own IWeaveService interface to further interact with
// weaved. This allows weaved to distinguish binder calls from each client and
// maintain the track record of which client adds each resource.

// Once IWeaveClient::onServiceConnection is called, we have a fully-established
// service connection to weaved and we invoke the client callback provided in
// the original call to Service::Connect() and pass the weak pointer to the
// service as an argument.

// In case a connection to weaved is lost, the ServiceImpl class will be deleted
// and any weak pointers to it the client may have will be invalidated.
// A new instance of ServiceImpl is created and the strong reference in
// ServiceSubscription is replace to the new instance. A new re-connection cycle
// is started as if the client just invoked Service::Connect() again on the new
// instance of ServiceImpl.

namespace weaved {

namespace {
// An implementation for service subscription. This object keeps a reference to
// the actual instance of weaved service object. This is generally the only hard
// reference to the shared pointer to the service object. The client receives
// a weak pointer only.
class ServiceSubscription : public Service::Subscription {
 public:
  ServiceSubscription() = default;
  ~ServiceSubscription() override = default;

  void SetService(const std::shared_ptr<Service>& service) {
    service_ = service;
  }

 private:
  std::shared_ptr<Service> service_;
  DISALLOW_COPY_AND_ASSIGN(ServiceSubscription);
};

}  // anonymous namespace

class ServiceImpl;

// Each system process wishing to expose functionality via weave establishes a
// connection to weaved via Binder. The communication channel is two-way.
// The client obtains a reference to weaved's android::weave::IWeaveService from
// the system service manager, and registers an instance of
// android::weave::IWeaveClient with weaved via IWeaveService.
// WeaveClient is an implementation of android::weave::IWeaveClient binder
// interface. Apart from providing callback methods (such as onCommand), it is
// used by weaved to track the life-time of this particular client. If the
// client exits, weaved automatically cleans up resources added by this client.
class WeaveClient : public android::weave::BnWeaveClient {
 public:
  explicit WeaveClient(const std::weak_ptr<ServiceImpl>& service);

 private:
  // Implementation for IWeaveClient interface.
  // A notification that the service binder is successfully instantiated and
  // weaved daemon is ready to process incoming request for component creation,
  // device state updates and so on.
  android::binder::Status onServiceConnected(
      const android::sp<android::weave::IWeaveService>& service) override;

  // A callback invoked when a new command for which a handler was registered
  // is added to the command queue.
  android::binder::Status onCommand(
      const android::String16& componentName,
      const android::String16& commandName,
      const android::sp<android::weave::IWeaveCommand>& command) override;

  std::weak_ptr<ServiceImpl> service_;

  base::WeakPtrFactory<WeaveClient> weak_ptr_factory_{this};
  DISALLOW_COPY_AND_ASSIGN(WeaveClient);
};

class NotificationListener
    : public android::weave::BnWeaveServiceManagerNotificationListener {
 public:
  explicit NotificationListener(const std::weak_ptr<ServiceImpl>& service);

 private:
  // Implementation for IWeaveServiceManagerNotificationListener interface.
  android::binder::Status notifyServiceManagerChange(
      const std::vector<int>& notificationIds) override;

  std::weak_ptr<ServiceImpl> service_;

  base::WeakPtrFactory<NotificationListener> weak_ptr_factory_{this};
  DISALLOW_COPY_AND_ASSIGN(NotificationListener);
};

// ServiceImpl is a concrete implementation of weaved::Service interface.
// This object is a wrapper around android::weave::IWeaveService binder
// interface to weaved daemon.
// This class is created as soon as Service::Connect() is called and it
// initiates connection attempts to IWeaveService binder. Only when the
// connection is successful and we receive callback notification from weaved
// that the service is ready, we invoke the client-provided callback and pass
// a weak pointer to Service fro the client to talk to weaved.
class ServiceImpl : public std::enable_shared_from_this<ServiceImpl>,
                    public Service {
 public:
  // A constructor. Client code never creates this instance directly, but rather
  // uses Service::Connect which is responsible for creating a instance of this
  // class.
  ServiceImpl(android::BinderWrapper* binder_wrapper,
              brillo::MessageLoop* message_loop,
              ServiceSubscription* service_subscription,
              const ConnectionCallback& connection_callback);
  ~ServiceImpl() override;

  // Service interface methods.
  bool AddComponent(const std::string& component,
                    const std::vector<std::string>& traits,
                    brillo::ErrorPtr* error) override;
  void AddCommandHandler(const std::string& component,
                         const std::string& trait_name,
                         const std::string& command_name,
                         const CommandHandlerCallback& callback) override;
  bool SetStateProperties(const std::string& component,
                          const base::DictionaryValue& dict,
                          brillo::ErrorPtr* error) override;
  bool SetStateProperty(const std::string& component,
                        const std::string& trait_name,
                        const std::string& property_name,
                        const base::Value& value,
                        brillo::ErrorPtr* error) override;
  void SetPairingInfoListener(const PairingInfoCallback& callback) override;

  // Helper method called from Service::Connect() to initiate binder connection
  // to weaved. This message just posts a task to the message loop to invoke
  // TryConnecting() method.
  void BeginConnect();

  // A callback method for WeaveClient::onServiceConnected().
  void OnServiceConnected(
      const android::sp<android::weave::IWeaveService>& service);

  // A callback method for WeaveClient::onCommand().
  void OnCommand(const std::string& component_name,
                 const std::string& command_name,
                 const android::sp<android::weave::IWeaveCommand>& command);

  // A callback method for NotificationListener::notifyServiceManagerChange().
  void OnNotification(const std::vector<int>& notification_ids);

 private:
  // Connects to weaved daemon over binder if the service manager is available
  // and weaved daemon itself is ready to accept connections. If not, schedules
  // another retry after a delay (1 second).
  void TryConnecting();

  // A callback for weaved connection termination. When binder service manager
  // notifies client of weaved binder object destruction (e.g. weaved quits),
  // this callback is invoked and initiates re-connection process.
  // Since the callback can happen synchronously from any call into the binder
  // driver, this method just posts a message that just asynchronously invokes
  // "ReconnectOnServiceDisconnection".
  void OnWeaveServiceDisconnected();

  // Asynchronous notification callback of binder service death. Tears down
  // this instance of ServiceImpl class, creates a new one and re-initiates
  // the binder connection to the service.
  void ReconnectOnServiceDisconnection();

  android::BinderWrapper* binder_wrapper_;
  brillo::MessageLoop* message_loop_;
  ServiceSubscription* service_subscription_;
  ConnectionCallback connection_callback_;
  android::sp<android::weave::IWeaveServiceManager> weave_service_manager_;
  android::sp<android::weave::IWeaveService> weave_service_;
  PairingInfoCallback pairing_info_callback_;
  PairingInfo pairing_info_;

  struct CommandHandlerEntry {
    std::string component;
    std::string command_name;
    CommandHandlerCallback callback;
  };
  std::vector<CommandHandlerEntry> command_handlers_;

  base::WeakPtrFactory<ServiceImpl> weak_ptr_factory_{this};
  DISALLOW_COPY_AND_ASSIGN(ServiceImpl);
};

WeaveClient::WeaveClient(const std::weak_ptr<ServiceImpl>& service)
    : service_{service} {}

android::binder::Status WeaveClient::onServiceConnected(
    const android::sp<android::weave::IWeaveService>& service) {
  LOG(INFO) << "Weave service connection established successfully";
  auto service_proxy = service_.lock();
  if (service_proxy)
    service_proxy->OnServiceConnected(service);
  return android::binder::Status::ok();
}

android::binder::Status WeaveClient::onCommand(
    const android::String16& componentName,
    const android::String16& commandName,
    const android::sp<android::weave::IWeaveCommand>& command) {
  auto service_proxy = service_.lock();
  if (service_proxy) {
    service_proxy->OnCommand(ToString(componentName), ToString(commandName),
                             command);
  } else {
    command->abort(android::String16{"service_unavailable"},
                   android::String16{"Command handler is unavailable"});
  }
  return android::binder::Status::ok();
}

NotificationListener::NotificationListener(
    const std::weak_ptr<ServiceImpl>& service)
    : service_{service} {}

android::binder::Status NotificationListener::notifyServiceManagerChange(
    const std::vector<int>& notificationIds) {
  auto service_proxy = service_.lock();
  if (service_proxy)
    service_proxy->OnNotification(notificationIds);
  return android::binder::Status::ok();
}

ServiceImpl::ServiceImpl(android::BinderWrapper* binder_wrapper,
                         brillo::MessageLoop* message_loop,
                         ServiceSubscription* service_subscription,
                         const ConnectionCallback& connection_callback)
    : binder_wrapper_{binder_wrapper},
      message_loop_{message_loop},
      service_subscription_{service_subscription},
      connection_callback_{connection_callback} {
}

ServiceImpl::~ServiceImpl() {
  if (weave_service_.get()) {
    android::sp<android::IBinder> binder =
        android::IInterface::asBinder(weave_service_);
    binder_wrapper_->UnregisterForDeathNotifications(binder);
  }
}

bool ServiceImpl::AddComponent(const std::string& component,
                               const std::vector<std::string>& traits,
                               brillo::ErrorPtr* error) {
  CHECK(weave_service_.get());
  std::vector<android::String16> trait_list;
  auto to_string16 = [](const std::string& name) {
    return android::String16{name.c_str()};
  };
  std::transform(traits.begin(), traits.end(), std::back_inserter(trait_list),
                 to_string16);
  return StatusToError(weave_service_->addComponent(to_string16(component),
                                                    trait_list),
                       error);
}

void ServiceImpl::AddCommandHandler(const std::string& component,
                                    const std::string& trait_name,
                                    const std::string& command_name,
                                    const CommandHandlerCallback& callback) {
  CHECK(!component.empty() && !command_name.empty());
  CHECK(weave_service_.get());

  std::string full_command_name =
      base::StringPrintf("%s.%s", trait_name.c_str(), command_name.c_str());

  CommandHandlerEntry entry;
  entry.component = component;
  entry.command_name = full_command_name;
  entry.callback = callback;
  command_handlers_.push_back(std::move(entry));

  auto status = weave_service_->registerCommandHandler(
      android::String16{component.c_str()},
      android::String16{full_command_name.c_str()});
  CHECK(status.isOk());
}

bool ServiceImpl::SetStateProperties(const std::string& component,
                                     const base::DictionaryValue& dict,
                                     brillo::ErrorPtr* error) {
  CHECK(!component.empty());
  CHECK(weave_service_.get());
  return StatusToError(weave_service_->updateState(ToString16(component),
                                                   ToString16(dict)),
                       error);
}

bool ServiceImpl::SetStateProperty(const std::string& component,
                                   const std::string& trait_name,
                                   const std::string& property_name,
                                   const base::Value& value,
                                   brillo::ErrorPtr* error) {
  std::string name =
      base::StringPrintf("%s.%s", trait_name.c_str(), property_name.c_str());
  base::DictionaryValue dict;
  dict.Set(name, value.DeepCopy());
  return SetStateProperties(component, dict, error);
}

void ServiceImpl::SetPairingInfoListener(const PairingInfoCallback& callback) {
  pairing_info_callback_ = callback;
  if (!pairing_info_callback_.is_null() &&
      !pairing_info_.session_id.empty() &&
      !pairing_info_.pairing_mode.empty() &&
      !pairing_info_.pairing_code.empty()) {
    callback.Run(&pairing_info_);
  }
}

void ServiceImpl::BeginConnect() {
  message_loop_->PostTask(FROM_HERE,
                          base::Bind(&ServiceImpl::TryConnecting,
                                     weak_ptr_factory_.GetWeakPtr()));
}

void ServiceImpl::OnServiceConnected(
    const android::sp<android::weave::IWeaveService>& service) {
  weave_service_ = service;
  connection_callback_.Run(shared_from_this());
}

void ServiceImpl::OnCommand(
    const std::string& component_name,
    const std::string& command_name,
    const android::sp<android::weave::IWeaveCommand>& command) {
  VLOG(2) << "Weave command received for component '" << component_name << "': "
          << command_name;
  for (const auto& entry : command_handlers_) {
    if (entry.component == component_name &&
        entry.command_name == command_name) {
      std::unique_ptr<Command> command_instance{new Command{command}};
      return entry.callback.Run(std::move(command_instance));
    }
  }
  LOG(WARNING) << "Unexpected command notification. Command = " << command_name
               << ", component = " << component_name;
}

void ServiceImpl::TryConnecting() {
  LOG(INFO) << "Connecting to weave service over binder";
  android::sp<android::IBinder> binder =
      binder_wrapper_->GetService(weaved::binder::kWeaveServiceName);
  if (!binder.get()) {
    LOG(WARNING) << "Weave service is not available yet. Will try again later";
    message_loop_->PostDelayedTask(
        FROM_HERE,
        base::Bind(&ServiceImpl::TryConnecting, weak_ptr_factory_.GetWeakPtr()),
        base::TimeDelta::FromSeconds(1));
    return;
  }

  bool register_success = binder_wrapper_->RegisterForDeathNotifications(
      binder, base::Bind(&ServiceImpl::OnWeaveServiceDisconnected,
                         weak_ptr_factory_.GetWeakPtr()));
  if (!register_success) {
    // Something really bad happened here, restart the connection.
    OnWeaveServiceDisconnected();
    return;
  }
  weave_service_manager_ =
      android::interface_cast<android::weave::IWeaveServiceManager>(binder);
  android::sp<WeaveClient> weave_client = new WeaveClient{shared_from_this()};
  weave_service_manager_->connect(weave_client);
  android::sp<NotificationListener> notification_listener =
      new NotificationListener{shared_from_this()};
  weave_service_manager_->registerNotificationListener(notification_listener);
}

void ServiceImpl::OnWeaveServiceDisconnected() {
  message_loop_->PostTask(
      FROM_HERE,
      base::Bind(&ServiceImpl::ReconnectOnServiceDisconnection,
                 weak_ptr_factory_.GetWeakPtr()));
}

void ServiceImpl::ReconnectOnServiceDisconnection() {
  weave_service_.clear();
  // Need to create a new instance of service to invalidate existing weak
  // pointers.
  auto service = std::make_shared<ServiceImpl>(
      binder_wrapper_, message_loop_, service_subscription_,
      connection_callback_);
  service->BeginConnect();
  // The subscription object owns this instance.
  // Calling SetService() will destroy |this|.
  service_subscription_->SetService(service);
  // Do not call any methods or use resources of ServiceImpl after this point
  // because the object is destroyed now.
}

void ServiceImpl::OnNotification(const std::vector<int>& notification_ids) {
  bool pairing_info_changed = false;
  using NotificationListener =
      android::weave::IWeaveServiceManagerNotificationListener;
  android::String16 string_value;
  for (int id : notification_ids) {
    switch (id) {
      case NotificationListener::PAIRING_SESSION_ID:
        if (weave_service_manager_->getPairingSessionId(&string_value).isOk()) {
          pairing_info_changed = true;
          pairing_info_.session_id = ToString(string_value);
        }
        break;
      case NotificationListener::PAIRING_MODE:
        if (weave_service_manager_->getPairingMode(&string_value).isOk()) {
          pairing_info_changed = true;
          pairing_info_.pairing_mode = ToString(string_value);
        }
        break;
      case NotificationListener::PAIRING_CODE:
        if (weave_service_manager_->getPairingCode(&string_value).isOk()) {
          pairing_info_changed = true;
          pairing_info_.pairing_code = ToString(string_value);
        }
        break;
    }
  }

  if (!pairing_info_changed || pairing_info_callback_.is_null())
    return;

  if (pairing_info_.session_id.empty() || pairing_info_.pairing_mode.empty() ||
      pairing_info_.pairing_code.empty()) {
    pairing_info_callback_.Run(nullptr);
  } else {
    pairing_info_callback_.Run(&pairing_info_);
  }
}

std::unique_ptr<Service::Subscription> Service::Connect(
    brillo::MessageLoop* message_loop,
    const ConnectionCallback& callback) {
  std::unique_ptr<ServiceSubscription> subscription{new ServiceSubscription};
  auto service = std::make_shared<ServiceImpl>(
      android::BinderWrapper::GetOrCreateInstance(), message_loop,
      subscription.get(), callback);
  subscription->SetService(service);
  service->BeginConnect();
  return std::move(subscription);
}

}  // namespace weaved
