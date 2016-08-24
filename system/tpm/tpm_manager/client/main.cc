//
// Copyright (C) 2015 The Android Open Source Project
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
//

#include <stdio.h>
#include <stdlib.h>
#include <sysexits.h>

#include <memory>
#include <string>

#include <base/command_line.h>
#include <base/logging.h>
#include <base/message_loop/message_loop.h>
#include <brillo/bind_lambda.h>
#include <brillo/daemons/daemon.h>
#include <brillo/syslog_logging.h>

#include "tpm_manager/client/tpm_nvram_dbus_proxy.h"
#include "tpm_manager/client/tpm_ownership_dbus_proxy.h"
#include "tpm_manager/common/print_tpm_ownership_interface_proto.h"
#include "tpm_manager/common/print_tpm_nvram_interface_proto.h"
#include "tpm_manager/common/tpm_ownership_interface.pb.h"
#include "tpm_manager/common/tpm_nvram_interface.pb.h"

namespace tpm_manager {

const char kGetTpmStatusCommand[] = "status";
const char kTakeOwnershipCommand[] = "take_ownership";
const char kRemoveOwnerDependencyCommand[] = "remove_dependency";
const char kDefineNvramCommand[] = "define_nvram";
const char kDestroyNvramCommand[] = "destroy_nvram";
const char kWriteNvramCommand[] = "write_nvram";
const char kReadNvramCommand[] = "read_nvram";
const char kIsNvramDefinedCommand[] = "is_nvram_defined";
const char kIsNvramLockedCommand[] = "is_nvram_locked";
const char kGetNvramSizeCommand[] = "get_nvram_size";

const char kNvramIndexArg[] = "nvram_index";
const char kNvramLengthArg[] = "nvram_length";
const char kNvramDataArg[] = "nvram_data";

const char kUsage[] = R"(
Usage: tpm_manager_client <command> [<arguments>]
Commands (used as switches):
  --status
      Prints the current status of the Tpm.
  --take_ownership
      Takes ownership of the Tpm with a random password.
  --remove_dependency=<owner_dependency>
      Removes the provided Tpm owner dependency.
  --define_nvram
      Defines an NV space at |nvram_index| with length |nvram_length|.
  --destroy_nvram
      Destroys the NV space at |nvram_index|.
  --write_nvram
      Writes the NV space at |nvram_index| with |nvram_data|.
  --read_nvram
      Prints the contents of the NV space at |nvram_index|.
  --is_nvram_defined
      Prints whether the NV space at |nvram_index| is defined.
  --is_nvram_locked
      Prints whether the NV space at |nvram_index|  is locked for writing.
  --get_nvram_size
      Prints the size of the NV space at |nvram_index|.
Arguments (used as switches):
  --nvram_index=<index>
      Index of NV space to operate on.
  --nvram_length=<length>
      Size in bytes of the NV space to be created.
  --nvram_data=<data>
      Data to write to NV space.
)";

using ClientLoopBase = brillo::Daemon;
class ClientLoop : public ClientLoopBase {
 public:
  ClientLoop() = default;
  ~ClientLoop() override = default;

 protected:
  int OnInit() override {
    int exit_code = ClientLoopBase::OnInit();
    if (exit_code != EX_OK) {
      LOG(ERROR) << "Error initializing tpm_manager_client.";
      return exit_code;
    }
    TpmNvramDBusProxy* nvram_proxy = new TpmNvramDBusProxy();
    if (!nvram_proxy->Initialize()) {
      LOG(ERROR) << "Error initializing proxy to nvram interface.";
      return EX_UNAVAILABLE;
    }
    TpmOwnershipDBusProxy* ownership_proxy = new TpmOwnershipDBusProxy();
    if (!ownership_proxy->Initialize()) {
      LOG(ERROR) << "Error initializing proxy to ownership interface.";
      return EX_UNAVAILABLE;
    }
    tpm_nvram_.reset(nvram_proxy);
    tpm_ownership_.reset(ownership_proxy);
    exit_code = ScheduleCommand();
    if (exit_code == EX_USAGE) {
      printf("%s", kUsage);
    }
    return exit_code;
  }

  void OnShutdown(int* exit_code) override {
    tpm_nvram_.reset();
    tpm_ownership_.reset();
    ClientLoopBase::OnShutdown(exit_code);
  }

 private:
  // Posts tasks on to the message loop based on command line flags.
  int ScheduleCommand() {
    base::Closure task;
    base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
    if (command_line->HasSwitch("help") || command_line->HasSwitch("h")) {
      return EX_USAGE;
    } else if (command_line->HasSwitch(kGetTpmStatusCommand)) {
      task = base::Bind(&ClientLoop::HandleGetTpmStatus,
                        weak_factory_.GetWeakPtr());
    } else if (command_line->HasSwitch(kTakeOwnershipCommand)) {
      task = base::Bind(&ClientLoop::HandleTakeOwnership,
                        weak_factory_.GetWeakPtr());
    } else if (command_line->HasSwitch(kRemoveOwnerDependencyCommand)) {
      task = base::Bind(
          &ClientLoop::HandleRemoveOwnerDependency,
          weak_factory_.GetWeakPtr(),
          command_line->GetSwitchValueASCII(kRemoveOwnerDependencyCommand));
    } else if (command_line->HasSwitch(kDefineNvramCommand)) {
      if (!command_line->HasSwitch(kNvramIndexArg) ||
          !command_line->HasSwitch(kNvramLengthArg)) {
        LOG(ERROR) << "Cannot define nvram without a valid index and length.";
        return EX_USAGE;
      }
      task = base::Bind(
          &ClientLoop::HandleDefineNvram,
          weak_factory_.GetWeakPtr(),
          atoi(command_line->GetSwitchValueASCII(kNvramIndexArg).c_str()),
          atoi(command_line->GetSwitchValueASCII(kNvramLengthArg).c_str()));
    } else if (command_line->HasSwitch(kDestroyNvramCommand)) {
      if (!command_line->HasSwitch(kNvramIndexArg)) {
        LOG(ERROR) << "Cannot destroy nvram without a valid index.";
        return EX_USAGE;
      }
      task = base::Bind(
          &ClientLoop::HandleDestroyNvram,
          weak_factory_.GetWeakPtr(),
          atoi(command_line->GetSwitchValueASCII(kNvramIndexArg).c_str()));
    } else if (command_line->HasSwitch(kWriteNvramCommand)) {
      if (!command_line->HasSwitch(kNvramIndexArg) ||
          !command_line->HasSwitch(kNvramDataArg)) {
        LOG(ERROR) << "Cannot write nvram without a valid index and data.";
        return EX_USAGE;
      }
      task = base::Bind(
          &ClientLoop::HandleWriteNvram,
          weak_factory_.GetWeakPtr(),
          atoi(command_line->GetSwitchValueASCII(kNvramIndexArg).c_str()),
          command_line->GetSwitchValueASCII(kNvramDataArg));
    } else if (command_line->HasSwitch(kReadNvramCommand)) {
      if (!command_line->HasSwitch(kNvramIndexArg)) {
        LOG(ERROR) << "Cannot read nvram without a valid index.";
        return EX_USAGE;
      }
      task = base::Bind(
          &ClientLoop::HandleReadNvram,
          weak_factory_.GetWeakPtr(),
          atoi(command_line->GetSwitchValueASCII(kNvramIndexArg).c_str()));
    } else if (command_line->HasSwitch(kIsNvramDefinedCommand)) {
      if (!command_line->HasSwitch(kNvramIndexArg)) {
        LOG(ERROR) << "Cannot query nvram without a valid index.";
        return EX_USAGE;
      }
      task = base::Bind(
          &ClientLoop::HandleIsNvramDefined,
          weak_factory_.GetWeakPtr(),
          atoi(command_line->GetSwitchValueASCII(kNvramIndexArg).c_str()));
    } else if (command_line->HasSwitch(kIsNvramLockedCommand)) {
      if (!command_line->HasSwitch(kNvramIndexArg)) {
        LOG(ERROR) << "Cannot query nvram without a valid index.";
        return EX_USAGE;
      }
      task = base::Bind(
          &ClientLoop::HandleIsNvramLocked,
          weak_factory_.GetWeakPtr(),
          atoi(command_line->GetSwitchValueASCII(kNvramIndexArg).c_str()));
    } else if (command_line->HasSwitch(kGetNvramSizeCommand)) {
      if (!command_line->HasSwitch(kNvramIndexArg)) {
        LOG(ERROR) << "Cannot query nvram without a valid index.";
        return EX_USAGE;
      }
      task = base::Bind(
          &ClientLoop::HandleGetNvramSize,
          weak_factory_.GetWeakPtr(),
          atoi(command_line->GetSwitchValueASCII(kNvramIndexArg).c_str()));
    } else {
      // Command line arguments did not match any valid commands.
      LOG(ERROR) << "No Valid Command selected.";
      return EX_USAGE;
    }
    base::MessageLoop::current()->PostTask(FROM_HERE, task);
    return EX_OK;
  }

  // Template to print reply protobuf.
  template <typename ProtobufType>
  void PrintReplyAndQuit(const ProtobufType& reply) {
    LOG(INFO) << "Message Reply: " << GetProtoDebugString(reply);
    Quit();
  }

  void HandleGetTpmStatus() {
    GetTpmStatusRequest request;
    tpm_ownership_->GetTpmStatus(
        request,
        base::Bind(&ClientLoop::PrintReplyAndQuit<GetTpmStatusReply>,
                   weak_factory_.GetWeakPtr()));
  }

  void HandleTakeOwnership() {
    TakeOwnershipRequest request;
    tpm_ownership_->TakeOwnership(
        request,
        base::Bind(&ClientLoop::PrintReplyAndQuit<TakeOwnershipReply>,
                   weak_factory_.GetWeakPtr()));
  }

  void HandleRemoveOwnerDependency(const std::string& owner_dependency) {
    RemoveOwnerDependencyRequest request;
    request.set_owner_dependency(owner_dependency);
    tpm_ownership_->RemoveOwnerDependency(
        request,
        base::Bind(&ClientLoop::PrintReplyAndQuit<RemoveOwnerDependencyReply>,
                   weak_factory_.GetWeakPtr()));
  }

  void HandleDefineNvram(uint32_t index, size_t length) {
    DefineNvramRequest request;
    request.set_index(index);
    request.set_length(length);
    tpm_nvram_->DefineNvram(
        request,
        base::Bind(&ClientLoop::PrintReplyAndQuit<DefineNvramReply>,
                   weak_factory_.GetWeakPtr()));
  }

  void HandleDestroyNvram(uint32_t index) {
    DestroyNvramRequest request;
    request.set_index(index);
    tpm_nvram_->DestroyNvram(
        request,
        base::Bind(&ClientLoop::PrintReplyAndQuit<DestroyNvramReply>,
                   weak_factory_.GetWeakPtr()));
  }

  void HandleWriteNvram(uint32_t index, const std::string& data) {
    WriteNvramRequest request;
    request.set_index(index);
    request.set_data(data);
    tpm_nvram_->WriteNvram(
        request,
        base::Bind(&ClientLoop::PrintReplyAndQuit<WriteNvramReply>,
                   weak_factory_.GetWeakPtr()));
  }

  void HandleReadNvram(uint32_t index) {
    ReadNvramRequest request;
    request.set_index(index);
    tpm_nvram_->ReadNvram(
        request,
        base::Bind(&ClientLoop::PrintReplyAndQuit<ReadNvramReply>,
                   weak_factory_.GetWeakPtr()));
  }

  void HandleIsNvramDefined(uint32_t index) {
    IsNvramDefinedRequest request;
    request.set_index(index);
    tpm_nvram_->IsNvramDefined(
        request,
        base::Bind(&ClientLoop::PrintReplyAndQuit<IsNvramDefinedReply>,
                   weak_factory_.GetWeakPtr()));
  }

  void HandleIsNvramLocked(uint32_t index) {
    IsNvramLockedRequest request;
    request.set_index(index);
    tpm_nvram_->IsNvramLocked(
        request,
        base::Bind(&ClientLoop::PrintReplyAndQuit<IsNvramLockedReply>,
                   weak_factory_.GetWeakPtr()));
  }

  void HandleGetNvramSize(uint32_t index) {
    GetNvramSizeRequest request;
    request.set_index(index);
    tpm_nvram_->GetNvramSize(
        request,
        base::Bind(&ClientLoop::PrintReplyAndQuit<GetNvramSizeReply>,
                   weak_factory_.GetWeakPtr()));
  }

  // Pointer to a DBus proxy to tpm_managerd.
  std::unique_ptr<tpm_manager::TpmNvramInterface> tpm_nvram_;
  std::unique_ptr<tpm_manager::TpmOwnershipInterface> tpm_ownership_;

  // Declared last so that weak pointers will be destroyed first.
  base::WeakPtrFactory<ClientLoop> weak_factory_{this};

  DISALLOW_COPY_AND_ASSIGN(ClientLoop);
};

}  // namespace tpm_manager

int main(int argc, char* argv[]) {
  base::CommandLine::Init(argc, argv);
  brillo::InitLog(brillo::kLogToStderr);
  tpm_manager::ClientLoop loop;
  return loop.Run();
}
