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

#include "update_engine/daemon.h"

#include <sysexits.h>

#include <base/bind.h>
#include <base/location.h>
#include <base/time/time.h>
#if USE_WEAVE || USE_BINDER
#include <binderwrapper/binder_wrapper.h>
#endif  // USE_WEAVE || USE_BINDER

#if defined(__BRILLO__) || defined(__CHROMEOS__)
#include "update_engine/real_system_state.h"
#else  // !(defined(__BRILLO__) || defined(__CHROMEOS__))
#include "update_engine/daemon_state_android.h"
#endif  // defined(__BRILLO__) || defined(__CHROMEOS__)

#if USE_DBUS
namespace {
const int kDBusSystemMaxWaitSeconds = 2 * 60;
}  // namespace
#endif  // USE_DBUS

namespace chromeos_update_engine {

int UpdateEngineDaemon::OnInit() {
  // Register the |subprocess_| singleton with this Daemon as the signal
  // handler.
  subprocess_.Init(this);

  int exit_code = Daemon::OnInit();
  if (exit_code != EX_OK)
    return exit_code;

#if USE_WEAVE || USE_BINDER
  android::BinderWrapper::Create();
  binder_watcher_.Init();
#endif  // USE_WEAVE || USE_BINDER

#if USE_DBUS
  // We wait for the D-Bus connection for up two minutes to avoid re-spawning
  // the daemon too fast causing thrashing if dbus-daemon is not running.
  scoped_refptr<dbus::Bus> bus = dbus_connection_.ConnectWithTimeout(
      base::TimeDelta::FromSeconds(kDBusSystemMaxWaitSeconds));

  if (!bus) {
    // TODO(deymo): Make it possible to run update_engine even if dbus-daemon
    // is not running or constantly crashing.
    LOG(ERROR) << "Failed to initialize DBus, aborting.";
    return 1;
  }

  CHECK(bus->SetUpAsyncOperations());
#endif  // USE_DBUS

#if defined(__BRILLO__) || defined(__CHROMEOS__)
  // Initialize update engine global state but continue if something fails.
  // TODO(deymo): Move the daemon_state_ initialization to a factory method
  // avoiding the explicit re-usage of the |bus| instance, shared between
  // D-Bus service and D-Bus client calls.
  RealSystemState* real_system_state = new RealSystemState(bus);
  daemon_state_.reset(real_system_state);
  LOG_IF(ERROR, !real_system_state->Initialize())
      << "Failed to initialize system state.";
#else  // !(defined(__BRILLO__) || defined(__CHROMEOS__))
  DaemonStateAndroid* daemon_state_android = new DaemonStateAndroid();
  daemon_state_.reset(daemon_state_android);
  LOG_IF(ERROR, !daemon_state_android->Initialize())
      << "Failed to initialize system state.";
#endif  // defined(__BRILLO__) || defined(__CHROMEOS__)

#if USE_BINDER
  // Create the Binder Service.
#if defined(__BRILLO__) || defined(__CHROMEOS__)
  binder_service_ = new BinderUpdateEngineBrilloService{real_system_state};
#else  // !(defined(__BRILLO__) || defined(__CHROMEOS__))
  binder_service_ = new BinderUpdateEngineAndroidService{
      daemon_state_android->service_delegate()};
#endif  // defined(__BRILLO__) || defined(__CHROMEOS__)
  auto binder_wrapper = android::BinderWrapper::Get();
  if (!binder_wrapper->RegisterService(binder_service_->ServiceName(),
                                       binder_service_)) {
    LOG(ERROR) << "Failed to register binder service.";
  }

  daemon_state_->AddObserver(binder_service_.get());
#endif  // USE_BINDER

#if USE_DBUS
  // Create the DBus service.
  dbus_adaptor_.reset(new UpdateEngineAdaptor(real_system_state, bus));
  daemon_state_->AddObserver(dbus_adaptor_.get());

  dbus_adaptor_->RegisterAsync(base::Bind(&UpdateEngineDaemon::OnDBusRegistered,
                                          base::Unretained(this)));
  LOG(INFO) << "Waiting for DBus object to be registered.";
#else  // !USE_DBUS
  daemon_state_->StartUpdater();
#endif  // USE_DBUS
  return EX_OK;
}

#if USE_DBUS
void UpdateEngineDaemon::OnDBusRegistered(bool succeeded) {
  if (!succeeded) {
    LOG(ERROR) << "Registering the UpdateEngineAdaptor";
    QuitWithExitCode(1);
    return;
  }

  // Take ownership of the service now that everything is initialized. We need
  // to this now and not before to avoid exposing a well known DBus service
  // path that doesn't have the service it is supposed to implement.
  if (!dbus_adaptor_->RequestOwnership()) {
    LOG(ERROR) << "Unable to take ownership of the DBus service, is there "
               << "other update_engine daemon running?";
    QuitWithExitCode(1);
    return;
  }
  daemon_state_->StartUpdater();
}
#endif  // USE_DBUS

}  // namespace chromeos_update_engine
