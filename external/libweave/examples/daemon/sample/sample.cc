// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "examples/daemon/common/daemon.h"

#include <weave/device.h>
#include <weave/provider/task_runner.h>

#include <base/bind.h>
#include <base/memory/weak_ptr.h>

namespace {

const char kTraits[] = R"({
  "_sample": {
    "commands": {
      "hello": {
        "minimalRole": "user",
        "parameters": {
          "name": { "type": "string" }
        },
        "results": {
          "reply": { "type": "string" }
        }
      },
      "ping": {
        "minimalRole": "user",
        "parameters": {}
      },
      "countdown": {
        "minimalRole": "user",
        "parameters": {
          "seconds": {
            "type": "integer",
            "minimum": 1,
            "maximum": 25
          }
        }
      }
    },
    "state": {
      "pingCount": { "type": "integer" }
    }
  }
})";

const char kComponent[] = "sample";

}  // anonymous namespace

// SampleHandler is a command handler example.
// It implements the following commands:
// - _hello: handle a command with an argument and set its results.
// - _ping: update device state.
// - _countdown: handle long running command and report progress.
class SampleHandler {
 public:
  SampleHandler(weave::provider::TaskRunner* task_runner)
      : task_runner_{task_runner} {}
  void Register(weave::Device* device) {
    device_ = device;

    device->AddTraitDefinitionsFromJson(kTraits);
    CHECK(device->AddComponent(kComponent, {"_sample"}, nullptr));
    CHECK(device->SetStatePropertiesFromJson(
        kComponent, R"({"_sample": {"pingCount": 0}})", nullptr));

    device->AddCommandHandler(kComponent, "_sample.hello",
                              base::Bind(&SampleHandler::OnHelloCommand,
                                         weak_ptr_factory_.GetWeakPtr()));
    device->AddCommandHandler(kComponent, "_sample.ping",
                              base::Bind(&SampleHandler::OnPingCommand,
                                         weak_ptr_factory_.GetWeakPtr()));
    device->AddCommandHandler(kComponent, "_sample.countdown",
                              base::Bind(&SampleHandler::OnCountdownCommand,
                                         weak_ptr_factory_.GetWeakPtr()));
  }

 private:
  void OnHelloCommand(const std::weak_ptr<weave::Command>& command) {
    auto cmd = command.lock();
    if (!cmd)
      return;
    LOG(INFO) << "received command: " << cmd->GetName();

    const auto& params = cmd->GetParameters();
    std::string name;
    if (!params.GetString("name", &name)) {
      weave::ErrorPtr error;
      weave::Error::AddTo(&error, FROM_HERE, "invalid_parameter_value",
                          "Name is missing");
      cmd->Abort(error.get(), nullptr);
      return;
    }

    base::DictionaryValue result;
    result.SetString("reply", "Hello " + name);
    cmd->Complete(result, nullptr);
    LOG(INFO) << cmd->GetName() << " command finished: " << result;
  }

  void OnPingCommand(const std::weak_ptr<weave::Command>& command) {
    auto cmd = command.lock();
    if (!cmd)
      return;
    LOG(INFO) << "received command: " << cmd->GetName();

    device_->SetStateProperty(kComponent, "_sample.pingCount",
                              base::FundamentalValue{++ping_count_}, nullptr);
    LOG(INFO) << "New component state: " << device_->GetComponents();

    cmd->Complete({}, nullptr);

    LOG(INFO) << cmd->GetName() << " command finished";
  }

  void OnCountdownCommand(const std::weak_ptr<weave::Command>& command) {
    auto cmd = command.lock();
    if (!cmd)
      return;
    LOG(INFO) << "received command: " << cmd->GetName();

    const auto& params = cmd->GetParameters();
    int seconds;
    if (!params.GetInteger("seconds", &seconds))
      seconds = 10;

    LOG(INFO) << "starting countdown";
    DoTick(cmd, seconds);
  }

  void DoTick(const std::weak_ptr<weave::Command>& command, int seconds) {
    auto cmd = command.lock();
    if (!cmd)
      return;

    if (seconds > 0) {
      LOG(INFO) << "countdown tick: " << seconds << " seconds left";
      base::DictionaryValue progress;
      progress.SetInteger("seconds_left", seconds);
      cmd->SetProgress(progress, nullptr);
      task_runner_->PostDelayedTask(
          FROM_HERE,
          base::Bind(&SampleHandler::DoTick, weak_ptr_factory_.GetWeakPtr(),
                     command, --seconds),
          base::TimeDelta::FromSeconds(1));
      return;
    }

    cmd->Complete({}, nullptr);
    LOG(INFO) << "countdown finished";
    LOG(INFO) << cmd->GetName() << " command finished";
  }

  weave::Device* device_{nullptr};
  weave::provider::TaskRunner* task_runner_{nullptr};

  int ping_count_{0};
  base::WeakPtrFactory<SampleHandler> weak_ptr_factory_{this};
};

int main(int argc, char** argv) {
  Daemon::Options opts;
  if (!opts.Parse(argc, argv)) {
    Daemon::Options::ShowUsage(argv[0]);
    return 1;
  }
  Daemon daemon{opts};
  SampleHandler handler{daemon.GetTaskRunner()};
  handler.Register(daemon.GetDevice());
  daemon.Run();
  return 0;
}
