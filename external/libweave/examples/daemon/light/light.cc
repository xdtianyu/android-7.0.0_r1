// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "examples/daemon/common/daemon.h"

#include <weave/device.h>

#include <base/bind.h>
#include <base/memory/weak_ptr.h>

namespace {

const char kTraits[] = R"({
  "onOff": {
    "commands": {
      "setConfig": {
        "minimalRole": "user",
        "parameters": {
          "state": {
            "type": "string",
            "enum": [ "on", "standby" ]
          }
        }
      }
    },
    "state": {
      "state": {
        "type": "string",
        "enum": [ "on", "standby" ],
        "isRequired": true
      }
    }
  },
  "brightness": {
    "commands": {
      "setConfig": {
        "minimalRole": "user",
        "parameters": {
          "brightness": {
            "type": "integer",
            "minimum": 0,
            "maximum": 100
          }
        }
      }
    },
    "state": {
      "brightness": {
        "type": "integer",
        "isRequired": true,
        "minimum": 0,
        "maximum": 100
      }
    }
  },
  "colorXY": {
    "commands": {
      "setConfig": {
        "minimalRole": "user",
        "parameters": {
          "colorSetting": {
            "type": "object",
            "required": [
              "colorX",
              "colorY"
            ],
            "properties": {
              "colorX": {
                "type": "number",
                "minimum": 0.0,
                "maximum": 1.0
              },
              "colorY": {
                "type": "number",
                "minimum": 0.0,
                "maximum": 1.0
              }
            },
            "additionalProperties": false
          }
        },
        "errors": ["colorOutOfRange"]
      }
    },
    "state": {
      "colorSetting": {
        "type": "object",
        "isRequired": true,
        "required": [ "colorX", "colorY" ],
        "properties": {
          "colorX": {
            "type": "number",
            "minimum": 0.0,
            "maximum": 1.0
          },
          "colorY": {
            "type": "number",
            "minimum": 0.0,
            "maximum": 1.0
          }
        }
      },
      "colorCapRed": {
        "type": "object",
        "isRequired": true,
        "required": [ "colorX", "colorY" ],
        "properties": {
          "colorX": {
            "type": "number",
            "minimum": 0.0,
            "maximum": 1.0
          },
          "colorY": {
            "type": "number",
            "minimum": 0.0,
            "maximum": 1.0
          }
        }
      },
      "colorCapGreen": {
        "type": "object",
        "isRequired": true,
        "required": [ "colorX", "colorY" ],
        "properties": {
          "colorX": {
            "type": "number",
            "minimum": 0.0,
            "maximum": 1.0
          },
          "colorY": {
            "type": "number",
            "minimum": 0.0,
            "maximum": 1.0
          }
        }
      },
      "colorCapBlue": {
        "type": "object",
        "isRequired": true,
        "required": [ "colorX", "colorY" ],
        "properties": {
          "colorX": {
            "type": "number",
            "minimum": 0.0,
            "maximum": 1.0
          },
          "colorY": {
            "type": "number",
            "minimum": 0.0,
            "maximum": 1.0
          }
        }
      }
    }
  }
})";

const char kDefaultState[] = R"({
  "colorXY": {
    "colorSetting": {"colorX": 0, "colorY": 0},
    "colorCapRed":  {"colorX": 0.674, "colorY": 0.322},
    "colorCapGreen":{"colorX": 0.408, "colorY": 0.517},
    "colorCapBlue": {"colorX": 0.168, "colorY": 0.041}
  }
})";

const char kComponent[] = "light";

}  // anonymous namespace

// LightHandler is a command handler example that shows
// how to handle commands for a Weave light.
class LightHandler {
 public:
  LightHandler() = default;
  void Register(weave::Device* device) {
    device_ = device;

    device->AddTraitDefinitionsFromJson(kTraits);
    CHECK(device->AddComponent(kComponent, {"onOff", "brightness", "colorXY"},
                               nullptr));
    CHECK(
        device->SetStatePropertiesFromJson(kComponent, kDefaultState, nullptr));
    UpdateLightState();

    device->AddCommandHandler(kComponent, "onOff.setConfig",
                              base::Bind(&LightHandler::OnOnOffSetConfig,
                                         weak_ptr_factory_.GetWeakPtr()));
    device->AddCommandHandler(kComponent, "brightness.setConfig",
                              base::Bind(&LightHandler::OnBrightnessSetConfig,
                                         weak_ptr_factory_.GetWeakPtr()));
    device->AddCommandHandler(kComponent, "colorXY.setConfig",
                              base::Bind(&LightHandler::OnColorXYSetConfig,
                                         weak_ptr_factory_.GetWeakPtr()));
  }

 private:
  void OnBrightnessSetConfig(const std::weak_ptr<weave::Command>& command) {
    auto cmd = command.lock();
    if (!cmd)
      return;
    LOG(INFO) << "received command: " << cmd->GetName();
    const auto& params = cmd->GetParameters();
    int32_t brightness_value = 0;
    if (params.GetInteger("brightness", &brightness_value)) {
      // Display this command in terminal.
      LOG(INFO) << cmd->GetName() << " brightness: " << brightness_value;

      if (brightness_state_ != brightness_value) {
        brightness_state_ = brightness_value;
        UpdateLightState();
      }
      cmd->Complete({}, nullptr);
      return;
    }
    weave::ErrorPtr error;
    weave::Error::AddTo(&error, FROM_HERE, "invalid_parameter_value",
                        "Invalid parameters");
    cmd->Abort(error.get(), nullptr);
  }

  void OnOnOffSetConfig(const std::weak_ptr<weave::Command>& command) {
    auto cmd = command.lock();
    if (!cmd)
      return;
    LOG(INFO) << "received command: " << cmd->GetName();
    const auto& params = cmd->GetParameters();
    std::string requested_state;
    if (params.GetString("state", &requested_state)) {
      LOG(INFO) << cmd->GetName() << " state: " << requested_state;

      bool new_light_status = requested_state == "on";
      if (new_light_status != light_status_) {
        light_status_ = new_light_status;

        LOG(INFO) << "Light is now: " << (light_status_ ? "ON" : "OFF");
        UpdateLightState();
      }
      cmd->Complete({}, nullptr);
      return;
    }
    weave::ErrorPtr error;
    weave::Error::AddTo(&error, FROM_HERE, "invalid_parameter_value",
                        "Invalid parameters");
    cmd->Abort(error.get(), nullptr);
  }

  void OnColorXYSetConfig(const std::weak_ptr<weave::Command>& command) {
    auto cmd = command.lock();
    if (!cmd)
      return;
    LOG(INFO) << "received command: " << cmd->GetName();
    const auto& params = cmd->GetParameters();
    const base::DictionaryValue* colorXY = nullptr;
    if (params.GetDictionary("colorSetting", &colorXY)) {
      bool updateState = false;
      double X = 0.0;
      double Y = 0.0;
      if (colorXY->GetDouble("colorX", &X)) {
        color_X_ = X;
        updateState = true;
      }

      if (colorXY->GetDouble("colorY", &Y)) {
        color_Y_ = Y;
        updateState = true;
      }

      if (updateState)
        UpdateLightState();

      cmd->Complete({}, nullptr);
      return;
    }

    weave::ErrorPtr error;
    weave::Error::AddTo(&error, FROM_HERE, "invalid_parameter_value",
                        "Invalid parameters");
    cmd->Abort(error.get(), nullptr);
  }

  void UpdateLightState() {
    base::DictionaryValue state;
    state.SetString("onOff.state", light_status_ ? "on" : "standby");
    state.SetInteger("brightness.brightness", brightness_state_);

    std::unique_ptr<base::DictionaryValue> colorXY(new base::DictionaryValue());
    colorXY->SetDouble("colorX", color_X_);
    colorXY->SetDouble("colorY", color_Y_);
    state.Set("colorXY.colorSetting", colorXY.release());
    device_->SetStateProperties(kComponent, state, nullptr);
  }

  weave::Device* device_{nullptr};

  // Simulate the state of the light.
  bool light_status_{false};
  int32_t brightness_state_{0};
  double color_X_{0.0};
  double color_Y_{0.0};
  base::WeakPtrFactory<LightHandler> weak_ptr_factory_{this};
};

int main(int argc, char** argv) {
  Daemon::Options opts;
  opts.model_id_ = "AIAAA";
  if (!opts.Parse(argc, argv)) {
    Daemon::Options::ShowUsage(argv[0]);
    return 1;
  }
  Daemon daemon{opts};
  LightHandler handler;
  handler.Register(daemon.GetDevice());
  daemon.Run();
  return 0;
}
