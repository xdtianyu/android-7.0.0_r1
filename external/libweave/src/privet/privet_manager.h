// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBWEAVE_SRC_PRIVET_PRIVET_MANAGER_H_
#define LIBWEAVE_SRC_PRIVET_PRIVET_MANAGER_H_

#include <memory>
#include <set>
#include <string>
#include <vector>

#include <base/memory/weak_ptr.h>
#include <base/scoped_observer.h>
#include <weave/device.h>

#include "src/privet/cloud_delegate.h"
#include "src/privet/security_manager.h"
#include "src/privet/wifi_bootstrap_manager.h"

namespace libwebserv {
class ProtocolHandler;
class Request;
class Response;
class Server;
}

namespace weave {

class ComponentManager;
class DeviceRegistrationInfo;
class DnsServiceDiscovery;
class Network;

namespace privet {

class CloudDelegate;
class DaemonState;
class DeviceDelegate;
class PrivetHandler;
class Publisher;
class SecurityManager;

class Manager : public CloudDelegate::Observer {
 public:
  explicit Manager(provider::TaskRunner* task_runner);
  ~Manager() override;

  void Start(provider::Network* network,
             provider::DnsServiceDiscovery* dns_sd,
             provider::HttpServer* http_server,
             provider::Wifi* wifi,
             AuthManager* auth_manager,
             DeviceRegistrationInfo* device,
             ComponentManager* component_manager);

  std::string GetCurrentlyConnectedSsid() const;

  void AddOnPairingChangedCallbacks(
      const Device::PairingBeginCallback& begin_callback,
      const Device::PairingEndCallback& end_callback);

 private:
  // CloudDelegate::Observer
  void OnDeviceInfoChanged() override;

  void PrivetRequestHandler(
      std::unique_ptr<provider::HttpServer::Request> request);

  void PrivetRequestHandlerWithData(
      const std::shared_ptr<provider::HttpServer::Request>& request,
      const std::string& data);

  void PrivetResponseHandler(
      const std::shared_ptr<provider::HttpServer::Request>& request,
      int status,
      const base::DictionaryValue& output);

  void OnChanged();
  void OnConnectivityChanged();

  provider::TaskRunner* task_runner_{nullptr};
  std::unique_ptr<CloudDelegate> cloud_;
  std::unique_ptr<DeviceDelegate> device_;
  std::unique_ptr<SecurityManager> security_;
  std::unique_ptr<WifiBootstrapManager> wifi_bootstrap_manager_;
  std::unique_ptr<Publisher> publisher_;
  std::unique_ptr<PrivetHandler> privet_handler_;

  ScopedObserver<CloudDelegate, CloudDelegate::Observer> cloud_observer_{this};

  base::WeakPtrFactory<Manager> weak_ptr_factory_{this};
  DISALLOW_COPY_AND_ASSIGN(Manager);
};

}  // namespace privet
}  // namespace weave

#endif  // LIBWEAVE_SRC_PRIVET_PRIVET_MANAGER_H_
