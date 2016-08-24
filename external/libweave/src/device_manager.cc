// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/device_manager.h"

#include <string>

#include <base/bind.h>

#include "src/access_api_handler.h"
#include "src/access_black_list_manager_impl.h"
#include "src/base_api_handler.h"
#include "src/commands/schema_constants.h"
#include "src/component_manager_impl.h"
#include "src/config.h"
#include "src/device_registration_info.h"
#include "src/privet/auth_manager.h"
#include "src/privet/privet_manager.h"
#include "src/string_utils.h"
#include "src/utils.h"

namespace weave {

DeviceManager::DeviceManager(provider::ConfigStore* config_store,
                             provider::TaskRunner* task_runner,
                             provider::HttpClient* http_client,
                             provider::Network* network,
                             provider::DnsServiceDiscovery* dns_sd,
                             provider::HttpServer* http_server,
                             provider::Wifi* wifi,
                             provider::Bluetooth* bluetooth)
    : config_{new Config{config_store}},
      component_manager_{new ComponentManagerImpl{task_runner}} {
  if (http_server) {
    auth_manager_.reset(new privet::AuthManager(
        config_.get(), http_server->GetHttpsCertificateFingerprint()));
  }

  device_info_.reset(new DeviceRegistrationInfo(
      config_.get(), component_manager_.get(), task_runner, http_client,
      network, auth_manager_.get()));
  base_api_handler_.reset(new BaseApiHandler{device_info_.get(), this});

  black_list_manager_.reset(new AccessBlackListManagerImpl{config_store});
  access_api_handler_.reset(
      new AccessApiHandler{this, black_list_manager_.get()});

  device_info_->Start();

  if (http_server) {
    StartPrivet(task_runner, network, dns_sd, http_server, wifi, bluetooth);
  } else {
    CHECK(!dns_sd);
  }
}

DeviceManager::~DeviceManager() {}

const Settings& DeviceManager::GetSettings() const {
  return device_info_->GetSettings();
}

void DeviceManager::AddSettingsChangedCallback(
    const SettingsChangedCallback& callback) {
  device_info_->GetMutableConfig()->AddOnChangedCallback(callback);
}

Config* DeviceManager::GetConfig() {
  return device_info_->GetMutableConfig();
}

void DeviceManager::StartPrivet(provider::TaskRunner* task_runner,
                                provider::Network* network,
                                provider::DnsServiceDiscovery* dns_sd,
                                provider::HttpServer* http_server,
                                provider::Wifi* wifi,
                                provider::Bluetooth* bluetooth) {
  privet_.reset(new privet::Manager{task_runner});
  privet_->Start(network, dns_sd, http_server, wifi, auth_manager_.get(),
                 device_info_.get(), component_manager_.get());
}

GcdState DeviceManager::GetGcdState() const {
  return device_info_->GetGcdState();
}

void DeviceManager::AddGcdStateChangedCallback(
    const GcdStateChangedCallback& callback) {
  device_info_->AddGcdStateChangedCallback(callback);
}

void DeviceManager::AddTraitDefinitionsFromJson(const std::string& json) {
  CHECK(component_manager_->LoadTraits(json, nullptr));
}

void DeviceManager::AddTraitDefinitions(const base::DictionaryValue& dict) {
  CHECK(component_manager_->LoadTraits(dict, nullptr));
}

const base::DictionaryValue& DeviceManager::GetTraits() const {
  return component_manager_->GetTraits();
}

void DeviceManager::AddTraitDefsChangedCallback(const base::Closure& callback) {
  component_manager_->AddTraitDefChangedCallback(callback);
}

bool DeviceManager::AddComponent(const std::string& name,
                                 const std::vector<std::string>& traits,
                                 ErrorPtr* error) {
  return component_manager_->AddComponent("", name, traits, error);
}

bool DeviceManager::RemoveComponent(const std::string& name, ErrorPtr* error) {
  return component_manager_->RemoveComponent("", name, error);
}

void DeviceManager::AddComponentTreeChangedCallback(
    const base::Closure& callback) {
  component_manager_->AddComponentTreeChangedCallback(callback);
}

const base::DictionaryValue& DeviceManager::GetComponents() const {
  return component_manager_->GetComponents();
}

bool DeviceManager::SetStatePropertiesFromJson(const std::string& component,
                                               const std::string& json,
                                               ErrorPtr* error) {
  return component_manager_->SetStatePropertiesFromJson(component, json, error);
}

bool DeviceManager::SetStateProperties(const std::string& component,
                                       const base::DictionaryValue& dict,
                                       ErrorPtr* error) {
  return component_manager_->SetStateProperties(component, dict, error);
}

const base::Value* DeviceManager::GetStateProperty(const std::string& component,
                                                   const std::string& name,
                                                   ErrorPtr* error) const {
  return component_manager_->GetStateProperty(component, name, error);
}

bool DeviceManager::SetStateProperty(const std::string& component,
                                     const std::string& name,
                                     const base::Value& value,
                                     ErrorPtr* error) {
  return component_manager_->SetStateProperty(component, name, value, error);
}

void DeviceManager::AddCommandHandler(const std::string& component,
                                      const std::string& command_name,
                                      const CommandHandlerCallback& callback) {
  component_manager_->AddCommandHandler(component, command_name, callback);
}

void DeviceManager::AddCommandDefinitionsFromJson(const std::string& json) {
  auto dict = LoadJsonDict(json, nullptr);
  CHECK(dict);
  AddCommandDefinitions(*dict);
}

void DeviceManager::AddCommandDefinitions(const base::DictionaryValue& dict) {
  CHECK(component_manager_->AddLegacyCommandDefinitions(dict, nullptr));
}

bool DeviceManager::AddCommand(const base::DictionaryValue& command,
                               std::string* id,
                               ErrorPtr* error) {
  auto command_instance = component_manager_->ParseCommandInstance(
      command, Command::Origin::kLocal, UserRole::kOwner, id, error);
  if (!command_instance)
    return false;
  component_manager_->AddCommand(std::move(command_instance));
  return true;
}

Command* DeviceManager::FindCommand(const std::string& id) {
  return component_manager_->FindCommand(id);
}

void DeviceManager::AddCommandHandler(const std::string& command_name,
                                      const CommandHandlerCallback& callback) {
  if (command_name.empty())
    return component_manager_->AddCommandHandler("", "", callback);

  auto trait = SplitAtFirst(command_name, ".", true).first;
  std::string component = component_manager_->FindComponentWithTrait(trait);
  CHECK(!component.empty());
  component_manager_->AddCommandHandler(component, command_name, callback);
}

void DeviceManager::AddStateChangedCallback(const base::Closure& callback) {
  component_manager_->AddStateChangedCallback(callback);
}

void DeviceManager::AddStateDefinitionsFromJson(const std::string& json) {
  auto dict = LoadJsonDict(json, nullptr);
  CHECK(dict);
  AddStateDefinitions(*dict);
}

void DeviceManager::AddStateDefinitions(const base::DictionaryValue& dict) {
  CHECK(component_manager_->AddLegacyStateDefinitions(dict, nullptr));
}

bool DeviceManager::SetStatePropertiesFromJson(const std::string& json,
                                               ErrorPtr* error) {
  auto dict = LoadJsonDict(json, error);
  return dict && SetStateProperties(*dict, error);
}

bool DeviceManager::SetStateProperties(const base::DictionaryValue& dict,
                                       ErrorPtr* error) {
  for (base::DictionaryValue::Iterator it(dict); !it.IsAtEnd(); it.Advance()) {
    std::string component =
        component_manager_->FindComponentWithTrait(it.key());
    if (component.empty()) {
      Error::AddToPrintf(error, FROM_HERE, "unrouted_state",
                         "Unable to set property value because there is no "
                         "component supporting "
                         "trait '%s'",
                         it.key().c_str());
      return false;
    }
    base::DictionaryValue trait_state;
    trait_state.Set(it.key(), it.value().DeepCopy());
    if (!component_manager_->SetStateProperties(component, trait_state, error))
      return false;
  }
  return true;
}

const base::Value* DeviceManager::GetStateProperty(
    const std::string& name) const {
  auto trait = SplitAtFirst(name, ".", true).first;
  std::string component = component_manager_->FindComponentWithTrait(trait);
  if (component.empty())
    return nullptr;
  return component_manager_->GetStateProperty(component, name, nullptr);
}

bool DeviceManager::SetStateProperty(const std::string& name,
                                     const base::Value& value,
                                     ErrorPtr* error) {
  auto trait = SplitAtFirst(name, ".", true).first;
  std::string component = component_manager_->FindComponentWithTrait(trait);
  if (component.empty()) {
    Error::AddToPrintf(
        error, FROM_HERE, "unrouted_state",
        "Unable set value of state property '%s' because there is no component "
        "supporting trait '%s'",
        name.c_str(), trait.c_str());
    return false;
  }
  return component_manager_->SetStateProperty(component, name, value, error);
}

const base::DictionaryValue& DeviceManager::GetState() const {
  return component_manager_->GetLegacyState();
}

void DeviceManager::Register(const std::string& ticket_id,
                             const DoneCallback& callback) {
  device_info_->RegisterDevice(ticket_id, callback);
}

void DeviceManager::AddPairingChangedCallbacks(
    const PairingBeginCallback& begin_callback,
    const PairingEndCallback& end_callback) {
  if (privet_)
    privet_->AddOnPairingChangedCallbacks(begin_callback, end_callback);
}

std::unique_ptr<Device> Device::Create(provider::ConfigStore* config_store,
                                       provider::TaskRunner* task_runner,
                                       provider::HttpClient* http_client,
                                       provider::Network* network,
                                       provider::DnsServiceDiscovery* dns_sd,
                                       provider::HttpServer* http_server,
                                       provider::Wifi* wifi,
                                       provider::Bluetooth* bluetooth) {
  return std::unique_ptr<Device>{
      new DeviceManager{config_store, task_runner, http_client, network, dns_sd,
                        http_server, wifi, bluetooth}};
}

}  // namespace weave
