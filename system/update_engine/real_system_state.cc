//
// Copyright (C) 2012 The Android Open Source Project
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

#include "update_engine/real_system_state.h"

#include <string>

#include <base/bind.h>
#include <base/files/file_util.h>
#include <base/location.h>
#include <base/time/time.h>
#include <brillo/make_unique_ptr.h>
#include <brillo/message_loops/message_loop.h>

#include "update_engine/common/boot_control.h"
#include "update_engine/common/boot_control_stub.h"
#include "update_engine/common/constants.h"
#include "update_engine/common/hardware.h"
#include "update_engine/common/utils.h"
#include "update_engine/update_manager/state_factory.h"
#include "update_engine/weave_service_factory.h"

using brillo::MessageLoop;

namespace chromeos_update_engine {

RealSystemState::RealSystemState(const scoped_refptr<dbus::Bus>& bus)
    : debugd_proxy_(bus),
      power_manager_proxy_(bus),
      session_manager_proxy_(bus),
      shill_proxy_(bus),
      libcros_proxy_(bus) {
}

RealSystemState::~RealSystemState() {
  // Prevent any DBus communication from UpdateAttempter when shutting down the
  // daemon.
  if (update_attempter_)
    update_attempter_->ClearObservers();
}

bool RealSystemState::Initialize() {
  metrics_lib_.Init();

  boot_control_ = boot_control::CreateBootControl();
  if (!boot_control_) {
    LOG(WARNING) << "Unable to create BootControl instance, using stub "
                 << "instead. All update attempts will fail.";
    boot_control_ = brillo::make_unique_ptr(new BootControlStub());
  }

  hardware_ = hardware::CreateHardware();
  if (!hardware_) {
    LOG(ERROR) << "Error intializing the HardwareInterface.";
    return false;
  }

  LOG_IF(INFO, !hardware_->IsNormalBootMode()) << "Booted in dev mode.";
  LOG_IF(INFO, !hardware_->IsOfficialBuild()) << "Booted non-official build.";

  if (!shill_proxy_.Init()) {
    LOG(ERROR) << "Failed to initialize shill proxy.";
    return false;
  }

  // Initialize standard and powerwash-safe prefs.
  base::FilePath non_volatile_path;
  // TODO(deymo): Fall back to in-memory prefs if there's no physical directory
  // available.
  if (!hardware_->GetNonVolatileDirectory(&non_volatile_path)) {
    LOG(ERROR) << "Failed to get a non-volatile directory.";
    return false;
  }
  Prefs* prefs;
  prefs_.reset(prefs = new Prefs());
  if (!prefs->Init(non_volatile_path.Append(kPrefsSubDirectory))) {
    LOG(ERROR) << "Failed to initialize preferences.";
    return false;
  }

  base::FilePath powerwash_safe_path;
  if (!hardware_->GetPowerwashSafeDirectory(&powerwash_safe_path)) {
    // TODO(deymo): Fall-back to in-memory prefs if there's no powerwash-safe
    // directory, or disable powerwash feature.
    powerwash_safe_path = non_volatile_path.Append("powerwash-safe");
    LOG(WARNING) << "No powerwash-safe directory, using non-volatile one.";
  }
  powerwash_safe_prefs_.reset(prefs = new Prefs());
  if (!prefs->Init(
          powerwash_safe_path.Append(kPowerwashSafePrefsSubDirectory))) {
    LOG(ERROR) << "Failed to initialize powerwash preferences.";
    return false;
  }

  // Check the system rebooted marker file.
  std::string boot_id;
  if (utils::GetBootId(&boot_id)) {
    std::string prev_boot_id;
    system_rebooted_ = (!prefs_->GetString(kPrefsBootId, &prev_boot_id) ||
                        prev_boot_id != boot_id);
    prefs_->SetString(kPrefsBootId, boot_id);
  } else {
    LOG(WARNING) << "Couldn't detect the bootid, assuming system was rebooted.";
    system_rebooted_ = true;
  }

  // Initialize the OmahaRequestParams with the default settings. These settings
  // will be re-initialized before every request using the actual request
  // options. This initialization here pre-loads current channel and version, so
  // the DBus service can access it.
  if (!request_params_.Init("", "", false)) {
    LOG(WARNING) << "Ignoring OmahaRequestParams initialization error. Some "
                    "features might not work properly.";
  }

  certificate_checker_.reset(
      new CertificateChecker(prefs_.get(), &openssl_wrapper_));
  certificate_checker_->Init();

  // Initialize the UpdateAttempter before the UpdateManager.
  update_attempter_.reset(
      new UpdateAttempter(this, certificate_checker_.get(), &libcros_proxy_,
                          &debugd_proxy_));
  update_attempter_->Init();

  weave_service_ = ConstructWeaveService(update_attempter_.get());
  if (weave_service_)
    update_attempter_->AddObserver(weave_service_.get());

  // Initialize the Update Manager using the default state factory.
  chromeos_update_manager::State* um_state =
      chromeos_update_manager::DefaultStateFactory(
          &policy_provider_, &shill_proxy_, &session_manager_proxy_, this);
  if (!um_state) {
    LOG(ERROR) << "Failed to initialize the Update Manager.";
    return false;
  }
  update_manager_.reset(
      new chromeos_update_manager::UpdateManager(
          &clock_, base::TimeDelta::FromSeconds(5),
          base::TimeDelta::FromHours(12), um_state));

  // The P2P Manager depends on the Update Manager for its initialization.
  p2p_manager_.reset(P2PManager::Construct(
          nullptr, &clock_, update_manager_.get(), "cros_au",
          kMaxP2PFilesToKeep, base::TimeDelta::FromDays(kMaxP2PFileAgeDays)));

  if (!payload_state_.Initialize(this)) {
    LOG(ERROR) << "Failed to initialize the payload state object.";
    return false;
  }

  // All is well. Initialization successful.
  return true;
}

bool RealSystemState::StartUpdater() {
  // Initiate update checks.
  update_attempter_->ScheduleUpdates();

  // Update boot flags after 45 seconds.
  MessageLoop::current()->PostDelayedTask(
      FROM_HERE,
      base::Bind(&UpdateAttempter::UpdateBootFlags,
                 base::Unretained(update_attempter_.get())),
      base::TimeDelta::FromSeconds(45));

  // Broadcast the update engine status on startup to ensure consistent system
  // state on crashes.
  MessageLoop::current()->PostTask(FROM_HERE, base::Bind(
      &UpdateAttempter::BroadcastStatus,
      base::Unretained(update_attempter_.get())));

  // Run the UpdateEngineStarted() method on |update_attempter|.
  MessageLoop::current()->PostTask(FROM_HERE, base::Bind(
      &UpdateAttempter::UpdateEngineStarted,
      base::Unretained(update_attempter_.get())));
  return true;
}

void RealSystemState::AddObserver(ServiceObserverInterface* observer) {
  CHECK(update_attempter_.get());
  update_attempter_->AddObserver(observer);
}

void RealSystemState::RemoveObserver(ServiceObserverInterface* observer) {
  CHECK(update_attempter_.get());
  update_attempter_->RemoveObserver(observer);
}

}  // namespace chromeos_update_engine
