//
// Copyright (C) 2016 The Android Open Source Project
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

#ifndef UPDATE_ENGINE_UPDATE_ATTEMPTER_ANDROID_H_
#define UPDATE_ENGINE_UPDATE_ATTEMPTER_ANDROID_H_

#include <stdint.h>

#include <memory>
#include <string>
#include <vector>

#include <base/time/time.h>

#include "update_engine/client_library/include/update_engine/update_status.h"
#include "update_engine/common/action_processor.h"
#include "update_engine/common/boot_control_interface.h"
#include "update_engine/common/cpu_limiter.h"
#include "update_engine/common/hardware_interface.h"
#include "update_engine/common/prefs_interface.h"
#include "update_engine/payload_consumer/download_action.h"
#include "update_engine/payload_consumer/postinstall_runner_action.h"
#include "update_engine/service_delegate_android_interface.h"
#include "update_engine/service_observer_interface.h"

namespace chromeos_update_engine {

class DaemonStateAndroid;

class UpdateAttempterAndroid
    : public ServiceDelegateAndroidInterface,
      public ActionProcessorDelegate,
      public DownloadActionDelegate,
      public PostinstallRunnerAction::DelegateInterface {
 public:
  using UpdateStatus = update_engine::UpdateStatus;

  UpdateAttempterAndroid(DaemonStateAndroid* daemon_state,
                         PrefsInterface* prefs,
                         BootControlInterface* boot_control_,
                         HardwareInterface* hardware_);
  ~UpdateAttempterAndroid() override;

  // Further initialization to be done post construction.
  void Init();

  // ServiceDelegateAndroidInterface overrides.
  bool ApplyPayload(const std::string& payload_url,
                    int64_t payload_offset,
                    int64_t payload_size,
                    const std::vector<std::string>& key_value_pair_headers,
                    brillo::ErrorPtr* error) override;
  bool SuspendUpdate(brillo::ErrorPtr* error) override;
  bool ResumeUpdate(brillo::ErrorPtr* error) override;
  bool CancelUpdate(brillo::ErrorPtr* error) override;
  bool ResetStatus(brillo::ErrorPtr* error) override;

  // ActionProcessorDelegate methods:
  void ProcessingDone(const ActionProcessor* processor,
                      ErrorCode code) override;
  void ProcessingStopped(const ActionProcessor* processor) override;
  void ActionCompleted(ActionProcessor* processor,
                       AbstractAction* action,
                       ErrorCode code) override;

  // DownloadActionDelegate overrides.
  void BytesReceived(uint64_t bytes_progressed,
                     uint64_t bytes_received,
                     uint64_t total) override;
  bool ShouldCancel(ErrorCode* cancel_reason) override;
  void DownloadComplete() override;

  // PostinstallRunnerAction::DelegateInterface
  void ProgressUpdate(double progress) override;

 private:
  // Asynchronously marks the current slot as successful if needed. If already
  // marked as good, CompleteUpdateBootFlags() is called starting the action
  // processor.
  void UpdateBootFlags();

  // Called when the boot flags have been updated.
  void CompleteUpdateBootFlags(bool success);

  // Schedules an event loop callback to start the action processor. This is
  // scheduled asynchronously to unblock the event loop.
  void ScheduleProcessingStart();

  // Notifies an update request completed with the given error |code| to all
  // observers.
  void TerminateUpdateAndNotify(ErrorCode error_code);

  // Sets the status to the given |status| and notifies a status update to
  // all observers.
  void SetStatusAndNotify(UpdateStatus status);

  // Helper method to construct the sequence of actions to be performed for
  // applying an update.
  void BuildUpdateActions();

  // Sets up the download parameters based on the update requested on the
  // |install_plan_|.
  void SetupDownload();

  // Writes to the processing completed marker. Does nothing if
  // |update_completed_marker_| is empty.
  bool WriteUpdateCompletedMarker();

  // Returns whether an update was completed in the current boot.
  bool UpdateCompletedOnThisBoot();

  DaemonStateAndroid* daemon_state_;

  // DaemonStateAndroid pointers.
  PrefsInterface* prefs_;
  BootControlInterface* boot_control_;
  HardwareInterface* hardware_;

  // Last status notification timestamp used for throttling. Use monotonic
  // TimeTicks to ensure that notifications are sent even if the system clock is
  // set back in the middle of an update.
  base::TimeTicks last_notify_time_;

  // The list of actions and action processor that runs them asynchronously.
  // Only used when |ongoing_update_| is true.
  std::vector<std::shared_ptr<AbstractAction>> actions_;
  std::unique_ptr<ActionProcessor> processor_;

  // Pointer to the DownloadAction in the actions_ vector.
  std::shared_ptr<DownloadAction> download_action_;

  // Whether there is an ongoing update. This implies that an update was started
  // but not finished yet. This value will be true even if the update was
  // suspended.
  bool ongoing_update_{false};

  // The InstallPlan used during the ongoing update.
  InstallPlan install_plan_;

  // For status:
  UpdateStatus status_{UpdateStatus::IDLE};
  double download_progress_{0.0};

  // The offset in the payload file where the CrAU part starts.
  int64_t base_offset_{0};

  // Only direct proxy supported.
  DirectProxyResolver proxy_resolver_;

  // CPU limiter during the update.
  CPULimiter cpu_limiter_;

  // Whether we have marked the current slot as good. This step is required
  // before applying an update to the other slot.
  bool updated_boot_flags_ = false;

  DISALLOW_COPY_AND_ASSIGN(UpdateAttempterAndroid);
};

}  // namespace chromeos_update_engine

#endif  // UPDATE_ENGINE_UPDATE_ATTEMPTER_ANDROID_H_
