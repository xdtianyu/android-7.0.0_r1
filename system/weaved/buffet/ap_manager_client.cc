// Copyright 2015 The Android Open Source Project
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

#include "buffet/ap_manager_client.h"

namespace buffet {

using org::chromium::apmanager::ConfigProxyInterface;
using org::chromium::apmanager::ManagerProxyInterface;
using org::chromium::apmanager::ServiceProxyInterface;

ApManagerClient::ApManagerClient(const scoped_refptr<dbus::Bus>& bus)
    : bus_(bus) {}

ApManagerClient::~ApManagerClient() {
  Stop();
}

void ApManagerClient::Start(const std::string& ssid) {
  if (service_path_.IsValid()) {
    return;
  }

  ssid_ = ssid;

  object_manager_proxy_.reset(
      new org::chromium::apmanager::ObjectManagerProxy{bus_});
  object_manager_proxy_->SetManagerAddedCallback(base::Bind(
      &ApManagerClient::OnManagerAdded, weak_ptr_factory_.GetWeakPtr()));
  object_manager_proxy_->SetServiceAddedCallback(base::Bind(
      &ApManagerClient::OnServiceAdded, weak_ptr_factory_.GetWeakPtr()));

  object_manager_proxy_->SetServiceRemovedCallback(base::Bind(
      &ApManagerClient::OnServiceRemoved, weak_ptr_factory_.GetWeakPtr()));
  object_manager_proxy_->SetManagerRemovedCallback(base::Bind(
      &ApManagerClient::OnManagerRemoved, weak_ptr_factory_.GetWeakPtr()));
}

void ApManagerClient::Stop() {
  if (manager_proxy_ && service_path_.IsValid()) {
    RemoveService(service_path_);
  }
  service_path_ = dbus::ObjectPath();
  service_proxy_ = nullptr;
  manager_proxy_ = nullptr;
  object_manager_proxy_.reset();
  ssid_.clear();
}

void ApManagerClient::RemoveService(const dbus::ObjectPath& object_path) {
  CHECK(object_path.IsValid());
  brillo::ErrorPtr error;
  if (!manager_proxy_->RemoveService(object_path, &error)) {
    LOG(ERROR) << "RemoveService failed: " << error->GetMessage();
  }
}

void ApManagerClient::OnManagerAdded(ManagerProxyInterface* manager_proxy) {
  VLOG(1) << "manager added: " << manager_proxy->GetObjectPath().value();
  manager_proxy_ = manager_proxy;

  if (service_path_.IsValid())
    return;

  brillo::ErrorPtr error;
  if (!manager_proxy_->CreateService(&service_path_, &error)) {
    LOG(ERROR) << "CreateService failed: " << error->GetMessage();
  }
}

void ApManagerClient::OnServiceAdded(ServiceProxyInterface* service_proxy) {
  VLOG(1) << "service added: " << service_proxy->GetObjectPath().value();
  if (service_proxy->GetObjectPath() != service_path_) {
    RemoveService(service_proxy->GetObjectPath());
    return;
  }
  service_proxy_ = service_proxy;

  ConfigProxyInterface* config_proxy =
      object_manager_proxy_->GetConfigProxy(service_proxy->config());
  config_proxy->set_ssid(ssid_, base::Bind(&ApManagerClient::OnSsidSet,
                                           weak_ptr_factory_.GetWeakPtr()));
}

void ApManagerClient::OnSsidSet(bool success) {
  if (!success || !service_proxy_) {
    LOG(ERROR) << "Failed to set ssid.";
    return;
  }
  VLOG(1) << "SSID is set: " << ssid_;

  brillo::ErrorPtr error;
  if (!service_proxy_->Start(&error)) {
    LOG(ERROR) << "Service start failed: " << error->GetMessage();
  }
}

void ApManagerClient::OnServiceRemoved(const dbus::ObjectPath& object_path) {
  VLOG(1) << "service removed: " << object_path.value();
  if (object_path != service_path_)
    return;
  service_path_ = dbus::ObjectPath();
  service_proxy_ = nullptr;
}

void ApManagerClient::OnManagerRemoved(const dbus::ObjectPath& object_path) {
  VLOG(1) << "manager removed: " << object_path.value();
  manager_proxy_ = nullptr;
  Stop();
}

}  // namespace buffet
