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

#include <sysexits.h>
#include <string>

#include <base/command_line.h>
#include <brillo/daemons/dbus_daemon.h>
#include <brillo/dbus/async_event_sequencer.h>
#include <brillo/minijail/minijail.h>
#include <brillo/syslog_logging.h>
#include <brillo/userdb_utils.h>

#include "tpm_manager/common/tpm_manager_constants.h"
#include "tpm_manager/server/dbus_service.h"
#include "tpm_manager/server/local_data_store_impl.h"
#include "tpm_manager/server/tpm_manager_service.h"

#if USE_TPM2
#include "tpm_manager/server/tpm2_initializer_impl.h"
#include "tpm_manager/server/tpm2_nvram_impl.h"
#include "tpm_manager/server/tpm2_status_impl.h"
#else
#include "tpm_manager/server/tpm_initializer_impl.h"
#include "tpm_manager/server/tpm_nvram_impl.h"
#include "tpm_manager/server/tpm_status_impl.h"
#endif

using brillo::dbus_utils::AsyncEventSequencer;

namespace {

const char kWaitForOwnershipTriggerSwitch[] = "wait_for_ownership_trigger";

class TpmManagerDaemon : public brillo::DBusServiceDaemon {
 public:
  TpmManagerDaemon()
      : brillo::DBusServiceDaemon(tpm_manager::kTpmManagerServiceName) {
    base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
    local_data_store_.reset(new tpm_manager::LocalDataStoreImpl());
#if USE_TPM2
    tpm_status_.reset(new tpm_manager::Tpm2StatusImpl);
    tpm_initializer_.reset(new tpm_manager::Tpm2InitializerImpl(
        local_data_store_.get(),
        tpm_status_.get()));
    tpm_nvram_.reset(new tpm_manager::Tpm2NvramImpl(local_data_store_.get()));
#else
    tpm_status_.reset(new tpm_manager::TpmStatusImpl);
    tpm_initializer_.reset(new tpm_manager::TpmInitializerImpl(
        local_data_store_.get(),
        tpm_status_.get()));
    tpm_nvram_.reset(new tpm_manager::TpmNvramImpl(local_data_store_.get()));
#endif
    tpm_manager_service_.reset(new tpm_manager::TpmManagerService(
        command_line->HasSwitch(kWaitForOwnershipTriggerSwitch),
        local_data_store_.get(),
        tpm_status_.get(),
        tpm_initializer_.get(),
        tpm_nvram_.get()));
  }

 protected:
  int OnInit() override {
    int result = brillo::DBusServiceDaemon::OnInit();
    if (result != EX_OK) {
      LOG(ERROR) << "Error starting tpm_manager dbus daemon.";
      return result;
    }
    CHECK(tpm_manager_service_->Initialize());
    return EX_OK;
  }

  void RegisterDBusObjectsAsync(AsyncEventSequencer* sequencer) override {
    dbus_service_.reset(new tpm_manager::DBusService(
        bus_, tpm_manager_service_.get(), tpm_manager_service_.get()));
    dbus_service_->Register(sequencer->GetHandler("Register() failed.", true));
  }

 private:
  std::unique_ptr<tpm_manager::LocalDataStore> local_data_store_;
  std::unique_ptr<tpm_manager::TpmStatus> tpm_status_;
  std::unique_ptr<tpm_manager::TpmInitializer> tpm_initializer_;
  std::unique_ptr<tpm_manager::TpmNvram> tpm_nvram_;
  std::unique_ptr<tpm_manager::TpmManagerService> tpm_manager_service_;
  std::unique_ptr<tpm_manager::DBusService> dbus_service_;

  DISALLOW_COPY_AND_ASSIGN(TpmManagerDaemon);
};

}  // namespace

int main(int argc, char* argv[]) {
  base::CommandLine::Init(argc, argv);
  base::CommandLine *cl = base::CommandLine::ForCurrentProcess();
  int flags = brillo::kLogToSyslog;
  if (cl->HasSwitch("log_to_stderr")) {
    flags |= brillo::kLogToStderr;
  }
  brillo::InitLog(flags);
  TpmManagerDaemon daemon;
  LOG(INFO) << "TpmManager Daemon Started.";
  return daemon.Run();
}
