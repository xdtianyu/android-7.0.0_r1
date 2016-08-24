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

#include "trunks/tpm_state_impl.h"

#include <base/logging.h>

#include "trunks/error_codes.h"
#include "trunks/tpm_generated.h"
#include "trunks/trunks_factory.h"

namespace {

// From definition of TPMA_PERMANENT.
const trunks::TPMA_PERMANENT kOwnerAuthSetMask = 1U;
const trunks::TPMA_PERMANENT kEndorsementAuthSetMask = 1U << 1;
const trunks::TPMA_PERMANENT kLockoutAuthSetMask = 1U << 2;
const trunks::TPMA_PERMANENT kInLockoutMask = 1U << 9;

// From definition of TPMA_STARTUP_CLEAR.
const trunks::TPMA_STARTUP_CLEAR kPlatformHierarchyMask = 1U;
const trunks::TPMA_STARTUP_CLEAR kStorageHierarchyMask = 1U << 1;
const trunks::TPMA_STARTUP_CLEAR kEndorsementHierarchyMask = 1U << 2;
const trunks::TPMA_STARTUP_CLEAR kOrderlyShutdownMask = 1U << 31;

// From definition of TPMA_ALGORITHM
const trunks::TPMA_ALGORITHM kAsymmetricAlgMask = 1U;

}  // namespace

namespace trunks {

TpmStateImpl::TpmStateImpl(const TrunksFactory& factory)
    : factory_(factory),
      initialized_(false),
      permanent_flags_(0),
      startup_clear_flags_(0),
      rsa_flags_(0),
      ecc_flags_(0) {
}

TpmStateImpl::~TpmStateImpl() {}

TPM_RC TpmStateImpl::Initialize() {
  TPM_RC result = GetTpmProperty(TPM_PT_PERMANENT, &permanent_flags_);
  if (result != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error getting permanent flags: " << GetErrorString(result);
    return result;
  }
  result = GetTpmProperty(TPM_PT_STARTUP_CLEAR, &startup_clear_flags_);
  if (result != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error getting startup flags: " << GetErrorString(result);
    return result;
  }
  result = GetTpmProperty(TPM_PT_LOCKOUT_COUNTER, &lockout_counter_);
  if (result != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error getting lockout counter: " << GetErrorString(result);
    return result;
  }
  result = GetTpmProperty(TPM_PT_MAX_AUTH_FAIL, &lockout_threshold_);
  if (result != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error getting lockout threshold: " << GetErrorString(result);
    return result;
  }
  result = GetTpmProperty(TPM_PT_LOCKOUT_INTERVAL, &lockout_interval_);
  if (result != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error getting lockout interval: " << GetErrorString(result);
    return result;
  }
  result = GetTpmProperty(TPM_PT_LOCKOUT_RECOVERY, &lockout_recovery_);
  if (result != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error getting lockout recovery: " << GetErrorString(result);
    return result;
  }

  TPMI_YES_NO more_data;
  TPMS_CAPABILITY_DATA capability_data;
  result = factory_.GetTpm()->GetCapabilitySync(TPM_CAP_ALGS,
                                                TPM_ALG_RSA,
                                                1,  // There is only one value.
                                                &more_data,
                                                &capability_data,
                                                nullptr);
  if (result) {
    LOG(ERROR) << __func__ << ": " << GetErrorString(result);
    return result;
  }
  if (capability_data.capability != TPM_CAP_ALGS ||
      capability_data.data.algorithms.count != 1) {
    LOG(ERROR) << __func__ << ": Unexpected capability data.";
    return SAPI_RC_MALFORMED_RESPONSE;
  }
  if (capability_data.data.algorithms.alg_properties[0].alg == TPM_ALG_RSA) {
    rsa_flags_ =
        capability_data.data.algorithms.alg_properties[0].alg_properties;
  }
  result = factory_.GetTpm()->GetCapabilitySync(TPM_CAP_ALGS,
                                                TPM_ALG_ECC,
                                                1,  // There is only one value.
                                                &more_data,
                                                &capability_data,
                                                nullptr);
  if (result) {
    LOG(ERROR) << __func__ << ": " << GetErrorString(result);
    return result;
  }
  if (capability_data.capability != TPM_CAP_ALGS ||
      capability_data.data.algorithms.count != 1) {
    LOG(ERROR) << __func__ << ": Unexpected capability data.";
    return SAPI_RC_MALFORMED_RESPONSE;
  }
  if (capability_data.data.algorithms.alg_properties[0].alg == TPM_ALG_ECC) {
    ecc_flags_ =
        capability_data.data.algorithms.alg_properties[0].alg_properties;
  }
  initialized_ = true;
  return TPM_RC_SUCCESS;
}

bool TpmStateImpl::IsOwnerPasswordSet() {
  CHECK(initialized_);
  return ((permanent_flags_ & kOwnerAuthSetMask) == kOwnerAuthSetMask);
}

bool TpmStateImpl::IsEndorsementPasswordSet() {
  CHECK(initialized_);
  return ((permanent_flags_ & kEndorsementAuthSetMask) ==
          kEndorsementAuthSetMask);
}

bool TpmStateImpl::IsLockoutPasswordSet() {
  CHECK(initialized_);
  return ((permanent_flags_ & kLockoutAuthSetMask) == kLockoutAuthSetMask);
}

bool TpmStateImpl::IsOwned() {
  return (IsOwnerPasswordSet() &&
          IsEndorsementPasswordSet() &&
          IsLockoutPasswordSet());
}

bool TpmStateImpl::IsInLockout() {
  CHECK(initialized_);
  return ((permanent_flags_ & kInLockoutMask) == kInLockoutMask);
}

bool TpmStateImpl::IsPlatformHierarchyEnabled() {
  CHECK(initialized_);
  return ((startup_clear_flags_ & kPlatformHierarchyMask) ==
      kPlatformHierarchyMask);
}

bool TpmStateImpl::IsStorageHierarchyEnabled() {
  CHECK(initialized_);
  return ((startup_clear_flags_ & kStorageHierarchyMask) ==
      kStorageHierarchyMask);
}

bool TpmStateImpl::IsEndorsementHierarchyEnabled() {
  CHECK(initialized_);
  return ((startup_clear_flags_ & kEndorsementHierarchyMask) ==
      kEndorsementHierarchyMask);
}

bool TpmStateImpl::IsEnabled() {
  return (!IsPlatformHierarchyEnabled() &&
          IsStorageHierarchyEnabled() &&
          IsEndorsementHierarchyEnabled());
}

bool TpmStateImpl::WasShutdownOrderly() {
  CHECK(initialized_);
  return ((startup_clear_flags_ & kOrderlyShutdownMask) ==
      kOrderlyShutdownMask);
}

bool TpmStateImpl::IsRSASupported() {
  CHECK(initialized_);
  return ((rsa_flags_ & kAsymmetricAlgMask) == kAsymmetricAlgMask);
}

bool TpmStateImpl::IsECCSupported() {
  CHECK(initialized_);
  return ((ecc_flags_ & kAsymmetricAlgMask) == kAsymmetricAlgMask);
}

uint32_t TpmStateImpl::GetLockoutCounter() {
  CHECK(initialized_);
  return lockout_counter_;
}

uint32_t TpmStateImpl::GetLockoutThreshold() {
  CHECK(initialized_);
  return lockout_threshold_;
}

uint32_t TpmStateImpl::GetLockoutInterval() {
  CHECK(initialized_);
  return lockout_interval_;
}

uint32_t TpmStateImpl::GetLockoutRecovery() {
  CHECK(initialized_);
  return lockout_recovery_;
}

TPM_RC TpmStateImpl::GetTpmProperty(uint32_t property,
                                    uint32_t* value) {
  CHECK(value);
  TPMI_YES_NO more_data;
  TPMS_CAPABILITY_DATA capability_data;
  TPM_RC result = factory_.GetTpm()->GetCapabilitySync(TPM_CAP_TPM_PROPERTIES,
                                                       property,
                                                       1,  // Only one property.
                                                       &more_data,
                                                       &capability_data,
                                                       nullptr);
  if (result != TPM_RC_SUCCESS) {
    LOG(ERROR) << __func__ << ": " << GetErrorString(result);
    return result;
  }
  if (capability_data.capability != TPM_CAP_TPM_PROPERTIES ||
      capability_data.data.tpm_properties.count != 1 ||
      capability_data.data.tpm_properties.tpm_property[0].property !=
      property) {
    LOG(ERROR) << __func__ << ": Unexpected capability data.";
    return SAPI_RC_MALFORMED_RESPONSE;
  }
  *value = capability_data.data.tpm_properties.tpm_property[0].value;
  return TPM_RC_SUCCESS;
}

}  // namespace trunks
