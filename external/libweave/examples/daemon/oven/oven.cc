// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "examples/daemon/common/daemon.h"

#include <weave/device.h>
#include <weave/provider/task_runner.h>

#include <base/bind.h>
#include <base/memory/weak_ptr.h>

namespace {
// Time for sensor temperature to match setting temperature
const double kWarmUpTime = 60.0;
// Oven max temp
const double kMaxTemp = 300.0;
// Oven min temp
const double kMinTemp = 20.0;

const char kTraits[] = R"({
  "temperatureSetting": {
    "commands": {
      "setConfig": {
        "minimalRole": "user",
        "parameters": {
          "units": {
            "type": "string"
          },
          "tempSetting": {
            "type": "number"
          }
        },
        "errors": ["tempOutOfRange", "unsupportedUnits"]
      }
    },
    "state": {
      "supportedUnits": {
        "type": "array",
        "items": {
          "type": "string",
          "enum": [ "celsius", "fahrenheit", "kelvin" ]
        },
        "minItems": 1,
        "uniqueItems": true,
        "isRequired": true
      },
      "units": {
        "type": "string",
        "enum": [ "celsius", "fahrenheit", "kelvin" ],
        "isRequired": true
      },
      "tempSetting": {
        "type": "number",
        "isRequired": true
      },
      "maxTempSetting": {
        "type": "number",
        "isRequired": true
      },
      "minTempSetting": {
        "type": "number",
        "isRequired": true
      }
    }
  },
  "temperatureSensor": {
    "commands": {
      "setConfig": {
        "minimalRole": "user",
        "parameters": {
          "units": {
            "type": "string"
          }
        },
        "errors": ["unsupportedUnits"]
      }
    },
    "state": {
      "supportedUnits": {
        "type": "array",
        "items": {
          "type": "string",
          "enum": [
            "celsius",
            "fahrenheit",
            "kelvin"
          ]
        },
        "minItems": 1,
        "uniqueItems": true,
        "isRequired": true
      },
      "units": {
        "type": "string",
        "enum": [ "celsius", "fahrenheit", "kelvin" ],
        "isRequired": true
      },
      "value": {
        "type": "number",
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
  }
})";

const char kComponent[] = "oven";
}  // anonymous namespace

// OvenHandler is a virtual oven example
// It implements the following commands from traits:
// - temperatureSetting: sets the temperature for the oven
// - brightness: sets the brightness of the oven light
// It exposes the following states from traits:
// - temperatureSetting: temperature setting for the oven
// - temperatureSensor: current oven temperature
// - brightness: current oven brightness
class OvenHandler {
 public:
  OvenHandler(weave::provider::TaskRunner* task_runner)
      : task_runner_{task_runner} {}

  void Register(weave::Device* device) {
    device_ = device;

    device->AddTraitDefinitionsFromJson(kTraits);
    CHECK(device->AddComponent(
        kComponent, {"temperatureSetting", "temperatureSensor", "brightness"},
        nullptr));

    UpdateOvenState();

    device->AddCommandHandler(kComponent, "temperatureSetting.setConfig",
                              base::Bind(&OvenHandler::OnSetTempCommand,
                                         weak_ptr_factory_.GetWeakPtr()));

    device->AddCommandHandler(kComponent, "brightness.setConfig",
                              base::Bind(&OvenHandler::OnSetBrightnessCommand,
                                         weak_ptr_factory_.GetWeakPtr()));
  }

 private:
  void OnSetTempCommand(const std::weak_ptr<weave::Command>& command) {
    auto cmd = command.lock();
    if (!cmd)
      return;
    LOG(INFO) << "received command: " << cmd->GetName();

    const auto& params = cmd->GetParameters();
    std::string units;
    double temp;

    if (params.GetString("units", &units) &&
        params.GetDouble("tempSetting", &temp)) {
      units_ = units;
      target_temperature_ = temp;

      UpdateOvenState();

      cmd->Complete({}, nullptr);
      LOG(INFO) << cmd->GetName() << " updated oven, matching temp";

      if (target_temperature_ != current_temperature_ && !is_match_ticking_) {
        double tickIncrement =
            ((target_temperature_ - current_temperature_) / kWarmUpTime);
        DoTick(tickIncrement);
      }
      return;
    }

    weave::ErrorPtr error;
    weave::Error::AddTo(&error, FROM_HERE, "invalid_parameter_value",
                        "Invalid parameters");
    cmd->Abort(error.get(), nullptr);
  }

  void OnSetBrightnessCommand(const std::weak_ptr<weave::Command>& command) {
    auto cmd = command.lock();
    if (!cmd)
      return;
    LOG(INFO) << "received command: " << cmd->GetName();

    const auto& params = cmd->GetParameters();

    int brightness;
    if (params.GetInteger("brightness", &brightness)) {
      brightness_ = brightness;

      UpdateOvenState();

      cmd->Complete({}, nullptr);
      return;
    }

    weave::ErrorPtr error;
    weave::Error::AddTo(&error, FROM_HERE, "invalid_parameter_value",
                        "Invalid parameters");
    cmd->Abort(error.get(), nullptr);
  }

  void UpdateOvenState() {
    base::DictionaryValue state;
    base::ListValue supportedUnits;
    supportedUnits.AppendStrings({"celsius"});

    state.SetString("temperatureSensor.units", units_);
    state.SetDouble("temperatureSensor.value", current_temperature_);
    state.Set("temperatureSensor.supportedUnits", supportedUnits.DeepCopy());

    state.SetString("temperatureSetting.units", units_);
    state.SetDouble("temperatureSetting.tempSetting", target_temperature_);
    state.Set("temperatureSetting.supportedUnits", supportedUnits.DeepCopy());
    state.SetDouble("temperatureSetting.maxTempSetting", kMaxTemp);
    state.SetDouble("temperatureSetting.minTempSetting", kMinTemp);

    state.SetInteger("brightness.brightness", brightness_);

    device_->SetStateProperties(kComponent, state, nullptr);
  }

  void DoTick(double tickIncrement) {
    LOG(INFO) << "Oven matching temp tick";

    if (std::fabs(target_temperature_ - current_temperature_) >=
        tickIncrement) {
      is_match_ticking_ = true;
      current_temperature_ += tickIncrement;
      UpdateOvenState();
      task_runner_->PostDelayedTask(
          FROM_HERE, base::Bind(&OvenHandler::DoTick,
                                weak_ptr_factory_.GetWeakPtr(), tickIncrement),
          base::TimeDelta::FromSeconds(1));
      return;
    }

    is_match_ticking_ = false;
    current_temperature_ = target_temperature_;
    UpdateOvenState();

    LOG(INFO) << "Oven temp matched";
  }

  weave::Device* device_{nullptr};
  weave::provider::TaskRunner* task_runner_{nullptr};

  std::string units_ = "celsius";
  double target_temperature_ = 0.0;
  double current_temperature_ = 0.0;
  int brightness_ = 0;
  bool is_match_ticking_ = false;

  base::WeakPtrFactory<OvenHandler> weak_ptr_factory_{this};
};

int main(int argc, char** argv) {
  Daemon::Options opts;
  if (!opts.Parse(argc, argv)) {
    Daemon::Options::ShowUsage(argv[0]);
    return 1;
  }
  Daemon daemon{opts};
  OvenHandler handler{daemon.GetTaskRunner()};
  handler.Register(daemon.GetDevice());
  daemon.Run();
  return 0;
}
