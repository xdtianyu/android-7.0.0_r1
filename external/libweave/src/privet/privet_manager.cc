// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/privet/privet_manager.h"

#include <memory>
#include <set>
#include <string>

#include <base/bind.h>
#include <base/json/json_reader.h>
#include <base/json/json_writer.h>
#include <base/memory/weak_ptr.h>
#include <base/scoped_observer.h>
#include <base/strings/string_number_conversions.h>
#include <base/values.h>
#include <weave/provider/network.h>

#include "src/bind_lambda.h"
#include "src/component_manager.h"
#include "src/device_registration_info.h"
#include "src/http_constants.h"
#include "src/privet/auth_manager.h"
#include "src/privet/cloud_delegate.h"
#include "src/privet/constants.h"
#include "src/privet/device_delegate.h"
#include "src/privet/privet_handler.h"
#include "src/privet/publisher.h"
#include "src/streams.h"
#include "src/string_utils.h"

namespace weave {
namespace privet {

using provider::TaskRunner;
using provider::Network;
using provider::DnsServiceDiscovery;
using provider::HttpServer;
using provider::Wifi;

Manager::Manager(TaskRunner* task_runner) : task_runner_{task_runner} {}

Manager::~Manager() {}

void Manager::Start(Network* network,
                    DnsServiceDiscovery* dns_sd,
                    HttpServer* http_server,
                    Wifi* wifi,
                    AuthManager* auth_manager,
                    DeviceRegistrationInfo* device,
                    ComponentManager* component_manager) {
  CHECK(auth_manager);
  CHECK(device);

  device_ = DeviceDelegate::CreateDefault(
      task_runner_, http_server->GetHttpPort(), http_server->GetHttpsPort(),
      http_server->GetRequestTimeout());
  cloud_ =
      CloudDelegate::CreateDefault(task_runner_, device, component_manager);
  cloud_observer_.Add(cloud_.get());

  security_.reset(new SecurityManager(device->GetMutableConfig(), auth_manager,
                                      task_runner_));
  network->AddConnectionChangedCallback(
      base::Bind(&Manager::OnConnectivityChanged, base::Unretained(this)));

  if (wifi && device->GetSettings().wifi_auto_setup_enabled) {
    VLOG(1) << "Enabling WiFi bootstrapping.";
    wifi_bootstrap_manager_.reset(new WifiBootstrapManager(
        device->GetMutableConfig(), task_runner_, network, wifi, cloud_.get()));
    wifi_bootstrap_manager_->Init();
  }

  if (dns_sd) {
    publisher_.reset(new Publisher(device_.get(), cloud_.get(),
                                   wifi_bootstrap_manager_.get(), dns_sd));
  }

  privet_handler_.reset(new PrivetHandler(cloud_.get(), device_.get(),
                                          security_.get(),
                                          wifi_bootstrap_manager_.get()));

  for (const auto& path : privet_handler_->GetHttpPaths()) {
    http_server->AddHttpRequestHandler(
        path, base::Bind(&Manager::PrivetRequestHandler,
                         weak_ptr_factory_.GetWeakPtr()));
  }

  for (const auto& path : privet_handler_->GetHttpsPaths()) {
    http_server->AddHttpsRequestHandler(
        path, base::Bind(&Manager::PrivetRequestHandler,
                         weak_ptr_factory_.GetWeakPtr()));
  }
}

std::string Manager::GetCurrentlyConnectedSsid() const {
  return wifi_bootstrap_manager_
             ? wifi_bootstrap_manager_->GetCurrentlyConnectedSsid()
             : "";
}

void Manager::AddOnPairingChangedCallbacks(
    const SecurityManager::PairingStartListener& on_start,
    const SecurityManager::PairingEndListener& on_end) {
  security_->RegisterPairingListeners(on_start, on_end);
}

void Manager::OnDeviceInfoChanged() {
  OnChanged();
}

void Manager::PrivetRequestHandler(
    std::unique_ptr<provider::HttpServer::Request> req) {
  std::shared_ptr<provider::HttpServer::Request> request{std::move(req)};

  std::string content_type =
      SplitAtFirst(request->GetFirstHeader(http::kContentType), ";", true)
          .first;

  return PrivetRequestHandlerWithData(request, content_type == http::kJson
                                                   ? request->GetData()
                                                   : std::string{});
}

void Manager::PrivetRequestHandlerWithData(
    const std::shared_ptr<provider::HttpServer::Request>& request,
    const std::string& data) {
  std::string auth_header = request->GetFirstHeader(http::kAuthorization);
  base::DictionaryValue empty;
  auto value = base::JSONReader::Read(data);
  const base::DictionaryValue* dictionary = &empty;
  if (value)
    value->GetAsDictionary(&dictionary);

  VLOG(3) << "Input: " << *dictionary;

  privet_handler_->HandleRequest(
      request->GetPath(), auth_header, dictionary,
      base::Bind(&Manager::PrivetResponseHandler,
                 weak_ptr_factory_.GetWeakPtr(), request));
}

void Manager::PrivetResponseHandler(
    const std::shared_ptr<provider::HttpServer::Request>& request,
    int status,
    const base::DictionaryValue& output) {
  VLOG(3) << "status: " << status << ", Output: " << output;
  std::string data;
  base::JSONWriter::WriteWithOptions(
      output, base::JSONWriter::OPTIONS_PRETTY_PRINT, &data);
  request->SendReply(status, data, http::kJson);
}

void Manager::OnChanged() {
  VLOG(1) << "Manager::OnChanged";
  if (publisher_)
    publisher_->Update();
}

void Manager::OnConnectivityChanged() {
  OnChanged();
}

}  // namespace privet
}  // namespace weave
