//
// Copyright (C) 2014 The Android Open Source Project
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

#include <base/at_exit.h>
#include <base/bind.h>
#include <base/command_line.h>
#include <base/threading/thread.h>
#include <brillo/minijail/minijail.h>
#include <brillo/syslog_logging.h>
#include <brillo/userdb_utils.h>

#include "trunks/background_command_transceiver.h"
#include "trunks/resource_manager.h"
#include "trunks/tpm_handle.h"
#include "trunks/tpm_simulator_handle.h"
#if defined(USE_BINDER_IPC)
#include "trunks/trunks_binder_service.h"
#else
#include "trunks/trunks_dbus_service.h"
#endif
#include "trunks/trunks_factory_impl.h"
#include "trunks/trunks_ftdi_spi.h"

namespace {

const uid_t kRootUID = 0;
#if defined(__ANDROID__)
const char kTrunksUser[] = "system";
const char kTrunksGroup[] = "system";
const char kTrunksSeccompPath[] =
    "/system/usr/share/policy/trunksd-seccomp.policy";
#else
const char kTrunksUser[] = "trunks";
const char kTrunksGroup[] = "trunks";
const char kTrunksSeccompPath[] = "/usr/share/policy/trunksd-seccomp.policy";
#endif
const char kBackgroundThreadName[] = "trunksd_background_thread";

void InitMinijailSandbox() {
  uid_t trunks_uid;
  gid_t trunks_gid;
  CHECK(brillo::userdb::GetUserInfo(kTrunksUser, &trunks_uid, &trunks_gid))
      << "Error getting trunks uid and gid.";
  CHECK_EQ(getuid(), kRootUID) << "trunksd not initialized as root.";
  brillo::Minijail* minijail = brillo::Minijail::GetInstance();
  struct minijail* jail = minijail->New();
  minijail->DropRoot(jail, kTrunksUser, kTrunksGroup);
  minijail->UseSeccompFilter(jail, kTrunksSeccompPath);
  minijail->Enter(jail);
  minijail->Destroy(jail);
  CHECK_EQ(getuid(), trunks_uid)
      << "trunksd was not able to drop user privilege.";
  CHECK_EQ(getgid(), trunks_gid)
      << "trunksd was not able to drop group privilege.";
}

}  // namespace

int main(int argc, char **argv) {
  base::CommandLine::Init(argc, argv);
  base::CommandLine *cl = base::CommandLine::ForCurrentProcess();
  int flags = brillo::kLogToSyslog;
  if (cl->HasSwitch("log_to_stderr")) {
    flags |= brillo::kLogToStderr;
  }
  brillo::InitLog(flags);

  // Create a service instance before anything else so objects like
  // AtExitManager exist.
#if defined(USE_BINDER_IPC)
  trunks::TrunksBinderService service;
#else
  trunks::TrunksDBusService service;
#endif

  // Chain together command transceivers:
  //   [IPC] --> BackgroundCommandTransceiver
  //         --> ResourceManager
  //         --> TpmHandle
  //         --> [TPM]
  trunks::CommandTransceiver *low_level_transceiver;
  if (cl->HasSwitch("ftdi")) {
    LOG(INFO) << "Sending commands to FTDI SPI.";
    low_level_transceiver = new trunks::TrunksFtdiSpi();
  } else if (cl->HasSwitch("simulator")) {
    LOG(INFO) << "Sending commands to simulator.";
    low_level_transceiver = new trunks::TpmSimulatorHandle();
  } else {
    low_level_transceiver = new trunks::TpmHandle();
  }
  CHECK(low_level_transceiver->Init())
      << "Error initializing TPM communication.";
  // This needs to be *after* opening the TPM handle and *before* starting the
  // background thread.
  InitMinijailSandbox();
  base::Thread background_thread(kBackgroundThreadName);
  CHECK(background_thread.Start()) << "Failed to start background thread.";
  trunks::TrunksFactoryImpl factory(low_level_transceiver);
  trunks::ResourceManager resource_manager(factory, low_level_transceiver);
  background_thread.task_runner()->PostNonNestableTask(
      FROM_HERE, base::Bind(&trunks::ResourceManager::Initialize,
                            base::Unretained(&resource_manager)));
  trunks::BackgroundCommandTransceiver background_transceiver(
      &resource_manager, background_thread.task_runner());
  service.set_transceiver(&background_transceiver);
  LOG(INFO) << "Trunks service started.";
  return service.Run();
}
