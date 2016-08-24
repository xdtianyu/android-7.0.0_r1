//
// Copyright (C) 2011 The Android Open Source Project
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

#include "update_engine/omaha_response_handler_action.h"

#include <string>

#include <base/logging.h>
#include <base/strings/string_util.h>
#include <policy/device_policy.h>

#include "update_engine/common/constants.h"
#include "update_engine/common/hardware_interface.h"
#include "update_engine/common/prefs_interface.h"
#include "update_engine/common/utils.h"
#include "update_engine/connection_manager_interface.h"
#include "update_engine/omaha_request_params.h"
#include "update_engine/payload_consumer/delta_performer.h"
#include "update_engine/payload_state_interface.h"

using std::string;

namespace chromeos_update_engine {

OmahaResponseHandlerAction::OmahaResponseHandlerAction(
    SystemState* system_state)
    : OmahaResponseHandlerAction(system_state,
                                 constants::kOmahaResponseDeadlineFile) {}

OmahaResponseHandlerAction::OmahaResponseHandlerAction(
    SystemState* system_state, const string& deadline_file)
    : system_state_(system_state),
      got_no_update_response_(false),
      key_path_(constants::kUpdatePayloadPublicKeyPath),
      deadline_file_(deadline_file) {}

void OmahaResponseHandlerAction::PerformAction() {
  CHECK(HasInputObject());
  ScopedActionCompleter completer(processor_, this);
  const OmahaResponse& response = GetInputObject();
  if (!response.update_exists) {
    got_no_update_response_ = true;
    LOG(INFO) << "There are no updates. Aborting.";
    return;
  }

  // All decisions as to which URL should be used have already been done. So,
  // make the current URL as the download URL.
  string current_url = system_state_->payload_state()->GetCurrentUrl();
  if (current_url.empty()) {
    // This shouldn't happen as we should always supply the HTTPS backup URL.
    // Handling this anyway, just in case.
    LOG(ERROR) << "There are no suitable URLs in the response to use.";
    completer.set_code(ErrorCode::kOmahaResponseInvalid);
    return;
  }

  install_plan_.download_url = current_url;
  install_plan_.version = response.version;

  OmahaRequestParams* const params = system_state_->request_params();
  PayloadStateInterface* const payload_state = system_state_->payload_state();

  // If we're using p2p to download and there is a local peer, use it.
  if (payload_state->GetUsingP2PForDownloading() &&
      !payload_state->GetP2PUrl().empty()) {
    LOG(INFO) << "Replacing URL " << install_plan_.download_url
              << " with local URL " << payload_state->GetP2PUrl()
              << " since p2p is enabled.";
    install_plan_.download_url = payload_state->GetP2PUrl();
    payload_state->SetUsingP2PForDownloading(true);
  }

  // Fill up the other properties based on the response.
  install_plan_.payload_size = response.size;
  install_plan_.payload_hash = response.hash;
  install_plan_.metadata_size = response.metadata_size;
  install_plan_.metadata_signature = response.metadata_signature;
  install_plan_.public_key_rsa = response.public_key_rsa;
  install_plan_.hash_checks_mandatory = AreHashChecksMandatory(response);
  install_plan_.is_resume =
      DeltaPerformer::CanResumeUpdate(system_state_->prefs(), response.hash);
  if (install_plan_.is_resume) {
    payload_state->UpdateResumed();
  } else {
    payload_state->UpdateRestarted();
    LOG_IF(WARNING, !DeltaPerformer::ResetUpdateProgress(
        system_state_->prefs(), false))
        << "Unable to reset the update progress.";
    LOG_IF(WARNING, !system_state_->prefs()->SetString(
        kPrefsUpdateCheckResponseHash, response.hash))
        << "Unable to save the update check response hash.";
  }
  install_plan_.payload_type = response.is_delta_payload
                                   ? InstallPayloadType::kDelta
                                   : InstallPayloadType::kFull;

  install_plan_.source_slot = system_state_->boot_control()->GetCurrentSlot();
  install_plan_.target_slot = install_plan_.source_slot == 0 ? 1 : 0;

  // The Omaha response doesn't include the channel name for this image, so we
  // use the download_channel we used during the request to tag the target slot.
  // This will be used in the next boot to know the channel the image was
  // downloaded from.
  string current_channel_key =
      kPrefsChannelOnSlotPrefix + std::to_string(install_plan_.target_slot);
  system_state_->prefs()->SetString(current_channel_key,
                                    params->download_channel());

  if (params->to_more_stable_channel() && params->is_powerwash_allowed())
    install_plan_.powerwash_required = true;

  TEST_AND_RETURN(HasOutputPipe());
  if (HasOutputPipe())
    SetOutputObject(install_plan_);
  LOG(INFO) << "Using this install plan:";
  install_plan_.Dump();

  // Send the deadline data (if any) to Chrome through a file. This is a pretty
  // hacky solution but should be OK for now.
  //
  // TODO(petkov): Re-architect this to avoid communication through a
  // file. Ideally, we would include this information in D-Bus's GetStatus
  // method and UpdateStatus signal. A potential issue is that update_engine may
  // be unresponsive during an update download.
  if (!deadline_file_.empty()) {
    utils::WriteFile(deadline_file_.c_str(),
                     response.deadline.data(),
                     response.deadline.size());
    chmod(deadline_file_.c_str(), S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
  }

  completer.set_code(ErrorCode::kSuccess);
}

bool OmahaResponseHandlerAction::AreHashChecksMandatory(
    const OmahaResponse& response) {
  // We sometimes need to waive the hash checks in order to download from
  // sources that don't provide hashes, such as dev server.
  // At this point UpdateAttempter::IsAnyUpdateSourceAllowed() has already been
  // checked, so an unofficial update URL won't get this far unless it's OK to
  // use without a hash. Additionally, we want to always waive hash checks on
  // unofficial builds (i.e. dev/test images).
  // The end result is this:
  //  * Base image:
  //    - Official URLs require a hash.
  //    - Unofficial URLs only get this far if the IsAnyUpdateSourceAllowed()
  //      devmode/debugd checks pass, in which case the hash is waived.
  //  * Dev/test image:
  //    - Any URL is allowed through with no hash checking.
  if (!system_state_->request_params()->IsUpdateUrlOfficial() ||
      !system_state_->hardware()->IsOfficialBuild()) {
    // Still do a hash check if a public key is included.
    if (!response.public_key_rsa.empty()) {
      // The autoupdate_CatchBadSignatures test checks for this string
      // in log-files. Keep in sync.
      LOG(INFO) << "Mandating payload hash checks since Omaha Response "
                << "for unofficial build includes public RSA key.";
      return true;
    } else {
      LOG(INFO) << "Waiving payload hash checks for unofficial update URL.";
      return false;
    }
  }

  // If we're using p2p, |install_plan_.download_url| may contain a
  // HTTP URL even if |response.payload_urls| contain only HTTPS URLs.
  if (!base::StartsWith(install_plan_.download_url, "https://",
                        base::CompareCase::INSENSITIVE_ASCII)) {
    LOG(INFO) << "Mandating hash checks since download_url is not HTTPS.";
    return true;
  }

  // TODO(jaysri): VALIDATION: For official builds, we currently waive hash
  // checks for HTTPS until we have rolled out at least once and are confident
  // nothing breaks. chromium-os:37082 tracks turning this on for HTTPS
  // eventually.

  // Even if there's a single non-HTTPS URL, make the hash checks as
  // mandatory because we could be downloading the payload from any URL later
  // on. It's really hard to do book-keeping based on each byte being
  // downloaded to see whether we only used HTTPS throughout.
  for (size_t i = 0; i < response.payload_urls.size(); i++) {
    if (!base::StartsWith(response.payload_urls[i], "https://",
                          base::CompareCase::INSENSITIVE_ASCII)) {
      LOG(INFO) << "Mandating payload hash checks since Omaha response "
                << "contains non-HTTPS URL(s)";
      return true;
    }
  }

  LOG(INFO) << "Waiving payload hash checks since Omaha response "
            << "only has HTTPS URL(s)";
  return false;
}

}  // namespace chromeos_update_engine
