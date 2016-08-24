// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/base_api_handler.h"

#include <base/bind.h>
#include <weave/device.h>

#include "src/commands/schema_constants.h"
#include "src/device_registration_info.h"

namespace weave {

namespace {
const char kBaseComponent[] = "base";
const char kBaseTrait[] = "base";
const char kBaseStateFirmwareVersion[] = "base.firmwareVersion";
const char kBaseStateAnonymousAccessRole[] = "base.localAnonymousAccessMaxRole";
const char kBaseStateDiscoveryEnabled[] = "base.localDiscoveryEnabled";
const char kBaseStatePairingEnabled[] = "base.localPairingEnabled";
}  // namespace

BaseApiHandler::BaseApiHandler(DeviceRegistrationInfo* device_info,
                               Device* device)
    : device_info_{device_info}, device_{device} {
  device_->AddTraitDefinitionsFromJson(R"({
    "base": {
      "commands": {
        "updateBaseConfiguration": {
          "minimalRole": "manager",
          "parameters": {
            "localAnonymousAccessMaxRole": {
              "enum": [ "none", "viewer", "user" ],
              "type": "string"
            },
            "localDiscoveryEnabled": {
              "type": "boolean"
            },
            "localPairingEnabled": {
              "type": "boolean"
            }
          }
        },
        "updateDeviceInfo": {
          "minimalRole": "manager",
          "parameters": {
            "description": {
              "type": "string"
            },
            "location": {
              "type": "string"
            },
            "name": {
              "type": "string"
            }
          }
        },
        "reboot": {
          "minimalRole": "user",
          "parameters": {},
          "errors": ["notEnoughBattery"]
        },
        "identify": {
          "minimalRole": "user",
          "parameters": {}
        }
      },
      "state": {
        "firmwareVersion": {
          "type": "string",
          "isRequired": true
        },
        "localDiscoveryEnabled": {
          "type": "boolean",
          "isRequired": true
        },
        "localAnonymousAccessMaxRole": {
          "type": "string",
          "enum": [ "none", "viewer", "user" ],
        "isRequired": true
        },
        "localPairingEnabled": {
          "type": "boolean",
          "isRequired": true
        },
        "connectionStatus": {
          "type": "string"
        },
        "network": {
          "type": "object",
          "additionalProperties": false,
          "properties": {
            "name": { "type": "string" }
          }
        }
      }
    }
  })");
  CHECK(device_->AddComponent(kBaseComponent, {kBaseTrait}, nullptr));
  OnConfigChanged(device_->GetSettings());

  const auto& settings = device_info_->GetSettings();
  base::DictionaryValue state;
  state.SetString(kBaseStateFirmwareVersion, settings.firmware_version);
  CHECK(device_->SetStateProperty(kBaseComponent, kBaseStateFirmwareVersion,
                                  base::StringValue{settings.firmware_version},
                                  nullptr));

  device_->AddCommandHandler(
      kBaseComponent, "base.updateBaseConfiguration",
      base::Bind(&BaseApiHandler::UpdateBaseConfiguration,
                 weak_ptr_factory_.GetWeakPtr()));

  device_->AddCommandHandler(kBaseComponent, "base.updateDeviceInfo",
                             base::Bind(&BaseApiHandler::UpdateDeviceInfo,
                                        weak_ptr_factory_.GetWeakPtr()));

  device_info_->GetMutableConfig()->AddOnChangedCallback(base::Bind(
      &BaseApiHandler::OnConfigChanged, weak_ptr_factory_.GetWeakPtr()));
}

void BaseApiHandler::UpdateBaseConfiguration(
    const std::weak_ptr<Command>& cmd) {
  auto command = cmd.lock();
  if (!command)
    return;
  CHECK(command->GetState() == Command::State::kQueued)
      << EnumToString(command->GetState());
  command->SetProgress(base::DictionaryValue{}, nullptr);

  const auto& settings = device_info_->GetSettings();
  std::string anonymous_access_role{
      EnumToString(settings.local_anonymous_access_role)};
  bool discovery_enabled{settings.local_discovery_enabled};
  bool pairing_enabled{settings.local_pairing_enabled};

  const auto& parameters = command->GetParameters();
  parameters.GetString("localAnonymousAccessMaxRole", &anonymous_access_role);
  parameters.GetBoolean("localDiscoveryEnabled", &discovery_enabled);
  parameters.GetBoolean("localPairingEnabled", &pairing_enabled);

  AuthScope auth_scope{AuthScope::kNone};
  if (!StringToEnum(anonymous_access_role, &auth_scope)) {
    ErrorPtr error;
    Error::AddToPrintf(&error, FROM_HERE, errors::commands::kInvalidPropValue,
                       "Invalid localAnonymousAccessMaxRole value '%s'",
                       anonymous_access_role.c_str());
    command->Abort(error.get(), nullptr);
    return;
  }

  device_info_->UpdateBaseConfig(auth_scope, discovery_enabled,
                                 pairing_enabled);

  command->Complete({}, nullptr);
}

void BaseApiHandler::OnConfigChanged(const Settings& settings) {
  base::DictionaryValue state;
  state.SetString(kBaseStateAnonymousAccessRole,
                  EnumToString(settings.local_anonymous_access_role));
  state.SetBoolean(kBaseStateDiscoveryEnabled,
                   settings.local_discovery_enabled);
  state.SetBoolean(kBaseStatePairingEnabled, settings.local_pairing_enabled);
  device_->SetStateProperties(kBaseComponent, state, nullptr);
}

void BaseApiHandler::UpdateDeviceInfo(const std::weak_ptr<Command>& cmd) {
  auto command = cmd.lock();
  if (!command)
    return;
  CHECK(command->GetState() == Command::State::kQueued)
      << EnumToString(command->GetState());
  command->SetProgress(base::DictionaryValue{}, nullptr);

  const auto& settings = device_info_->GetSettings();
  std::string name{settings.name};
  std::string description{settings.description};
  std::string location{settings.location};

  const auto& parameters = command->GetParameters();
  parameters.GetString("name", &name);
  parameters.GetString("description", &description);
  parameters.GetString("location", &location);

  device_info_->UpdateDeviceInfo(name, description, location);
  command->Complete({}, nullptr);
}

}  // namespace weave
