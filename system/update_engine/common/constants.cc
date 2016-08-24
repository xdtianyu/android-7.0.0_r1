//
// Copyright (C) 2013 The Android Open Source Project
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

#include "update_engine/common/constants.h"

namespace chromeos_update_engine {

const char kPowerwashMarkerFile[] =
    "/mnt/stateful_partition/factory_install_reset";

const char kPowerwashCommand[] = "safe fast keepimg reason=update_engine\n";

const char kPowerwashSafePrefsSubDirectory[] = "update_engine/prefs";

const char kPrefsSubDirectory[] = "prefs";

const char kStatefulPartition[] = "/mnt/stateful_partition";

const char kPostinstallDefaultScript[] = "postinst";

// Constants defining keys for the persisted state of update engine.
const char kPrefsAttemptInProgress[] = "attempt-in-progress";
const char kPrefsBackoffExpiryTime[] = "backoff-expiry-time";
const char kPrefsBootId[] = "boot-id";
const char kPrefsCurrentBytesDownloaded[] = "current-bytes-downloaded";
const char kPrefsCurrentResponseSignature[] = "current-response-signature";
const char kPrefsCurrentUrlFailureCount[] = "current-url-failure-count";
const char kPrefsCurrentUrlIndex[] = "current-url-index";
const char kPrefsDailyMetricsLastReportedAt[] =
    "daily-metrics-last-reported-at";
const char kPrefsDeltaUpdateFailures[] = "delta-update-failures";
const char kPrefsFullPayloadAttemptNumber[] = "full-payload-attempt-number";
const char kPrefsInstallDateDays[] = "install-date-days";
const char kPrefsLastActivePingDay[] = "last-active-ping-day";
const char kPrefsLastRollCallPingDay[] = "last-roll-call-ping-day";
const char kPrefsManifestMetadataSize[] = "manifest-metadata-size";
const char kPrefsManifestSignatureSize[] = "manifest-signature-size";
const char kPrefsMetricsAttemptLastReportingTime[] =
    "metrics-attempt-last-reporting-time";
const char kPrefsMetricsCheckLastReportingTime[] =
    "metrics-check-last-reporting-time";
const char kPrefsNumReboots[] = "num-reboots";
const char kPrefsNumResponsesSeen[] = "num-responses-seen";
const char kPrefsOmahaCohort[] = "omaha-cohort";
const char kPrefsOmahaCohortHint[] = "omaha-cohort-hint";
const char kPrefsOmahaCohortName[] = "omaha-cohort-name";
const char kPrefsP2PEnabled[] = "p2p-enabled";
const char kPrefsP2PFirstAttemptTimestamp[] = "p2p-first-attempt-timestamp";
const char kPrefsP2PNumAttempts[] = "p2p-num-attempts";
const char kPrefsPayloadAttemptNumber[] = "payload-attempt-number";
const char kPrefsPreviousVersion[] = "previous-version";
const char kPrefsResumedUpdateFailures[] = "resumed-update-failures";
const char kPrefsRollbackVersion[] = "rollback-version";
const char kPrefsChannelOnSlotPrefix[] = "channel-on-slot-";
const char kPrefsSystemUpdatedMarker[] = "system-updated-marker";
const char kPrefsTargetVersionAttempt[] = "target-version-attempt";
const char kPrefsTargetVersionInstalledFrom[] = "target-version-installed-from";
const char kPrefsTargetVersionUniqueId[] = "target-version-unique-id";
const char kPrefsTotalBytesDownloaded[] = "total-bytes-downloaded";
const char kPrefsUpdateCheckCount[] = "update-check-count";
const char kPrefsUpdateCheckResponseHash[] = "update-check-response-hash";
const char kPrefsUpdateCompletedBootTime[] = "update-completed-boot-time";
const char kPrefsUpdateCompletedOnBootId[] = "update-completed-on-boot-id";
const char kPrefsUpdateDurationUptime[] = "update-duration-uptime";
const char kPrefsUpdateFirstSeenAt[] = "update-first-seen-at";
const char kPrefsUpdateOverCellularPermission[] =
    "update-over-cellular-permission";
const char kPrefsUpdateServerCertificate[] = "update-server-cert";
const char kPrefsUpdateStateNextDataLength[] = "update-state-next-data-length";
const char kPrefsUpdateStateNextDataOffset[] = "update-state-next-data-offset";
const char kPrefsUpdateStateNextOperation[] = "update-state-next-operation";
const char kPrefsUpdateStateSHA256Context[] = "update-state-sha-256-context";
const char kPrefsUpdateStateSignatureBlob[] = "update-state-signature-blob";
const char kPrefsUpdateStateSignedSHA256Context[] =
    "update-state-signed-sha-256-context";
const char kPrefsUpdateTimestampStart[] = "update-timestamp-start";
const char kPrefsUrlSwitchCount[] = "url-switch-count";
const char kPrefsWallClockWaitPeriod[] = "wall-clock-wait-period";

const char kPayloadPropertyFileSize[] = "FILE_SIZE";
const char kPayloadPropertyFileHash[] = "FILE_HASH";
const char kPayloadPropertyMetadataSize[] = "METADATA_SIZE";
const char kPayloadPropertyMetadataHash[] = "METADATA_HASH";
const char kPayloadPropertyAuthorization[] = "AUTHORIZATION";
const char kPayloadPropertyUserAgent[] = "USER_AGENT";

}  // namespace chromeos_update_engine
