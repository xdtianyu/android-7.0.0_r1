// Copyright 2015 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// THIS CODE IS GENERATED - DO NOT MODIFY!

#include "tpm_generated.h"
#include "HandleProcess_fp.h"
#include "Implementation.h"
#include "TPM_Types.h"

TPM_RC ParseHandleBuffer(TPM_CC command_code,
                         BYTE** request_handle_buffer_start,
                         INT32* request_buffer_remaining_size,
                         TPM_HANDLE request_handles[],
                         UINT32* num_request_handles) {
  TPM_RC result = TPM_RC_SUCCESS;
  *num_request_handles = 0;
  switch (command_code) {
#ifdef TPM_CC_ActivateCredential
    case TPM_CC_ActivateCredential:
      result = TPMI_DH_OBJECT_Unmarshal(
          (TPMI_DH_OBJECT*)&request_handles[*num_request_handles],
          request_handle_buffer_start, request_buffer_remaining_size, FALSE);
      if (result != TPM_RC_SUCCESS) {
        return result;
      }
      ++(*num_request_handles);
      result = TPMI_DH_OBJECT_Unmarshal(
          (TPMI_DH_OBJECT*)&request_handles[*num_request_handles],
          request_handle_buffer_start, request_buffer_remaining_size, FALSE);
      if (result != TPM_RC_SUCCESS) {
        return result;
      }
      ++(*num_request_handles);
      return TPM_RC_SUCCESS;
#endif
#ifdef TPM_CC_Certify
    case TPM_CC_Certify:
      result = TPMI_DH_OBJECT_Unmarshal(
          (TPMI_DH_OBJECT*)&request_handles[*num_request_handles],
          request_handle_buffer_start, request_buffer_remaining_size, FALSE);
      if (result != TPM_RC_SUCCESS) {
        return result;
      }
      ++(*num_request_handles);
      result = TPMI_DH_OBJECT_Unmarshal(
          (TPMI_DH_OBJECT*)&request_handles[*num_request_handles],
          request_handle_buffer_start, request_buffer_remaining_size, TRUE);
      if (result != TPM_RC_SUCCESS) {
        return result;
      }
      ++(*num_request_handles);
      return TPM_RC_SUCCESS;
#endif
#ifdef TPM_CC_CertifyCreation
    case TPM_CC_CertifyCreation:
      result = TPMI_DH_OBJECT_Unmarshal(
          (TPMI_DH_OBJECT*)&request_handles[*num_request_handles],
          request_handle_buffer_start, request_buffer_remaining_size, TRUE);
      if (result != TPM_RC_SUCCESS) {
        return result;
      }
      ++(*num_request_handles);
      result = TPMI_DH_OBJECT_Unmarshal(
          (TPMI_DH_OBJECT*)&request_handles[*num_request_handles],
          request_handle_buffer_start, request_buffer_remaining_size, FALSE);
      if (result != TPM_RC_SUCCESS) {
        return result;
      }
      ++(*num_request_handles);
      return TPM_RC_SUCCESS;
#endif
#ifdef TPM_CC_ChangeEPS
    case TPM_CC_ChangeEPS:
      result = TPMI_RH_PLATFORM_Unmarshal(
          (TPMI_RH_PLATFORM*)&request_handles[*num_request_handles],
          request_handle_buffer_start, request_buffer_remaining_size);
      if (result != TPM_RC_SUCCESS) {
        return result;
      }
      ++(*num_request_handles);
      return TPM_RC_SUCCESS;
#endif
#ifdef TPM_CC_ChangePPS
    case TPM_CC_ChangePPS:
      result = TPMI_RH_PLATFORM_Unmarshal(
          (TPMI_RH_PLATFORM*)&request_handles[*num_request_handles],
          request_handle_buffer_start, request_buffer_remaining_size);
      if (result != TPM_RC_SUCCESS) {
        return result;
      }
      ++(*num_request_handles);
      return TPM_RC_SUCCESS;
#endif
#ifdef TPM_CC_Clear
    case TPM_CC_Clear:
      result = TPMI_RH_CLEAR_Unmarshal(
          (TPMI_RH_CLEAR*)&request_handles[*num_request_handles],
          request_handle_buffer_start, request_buffer_remaining_size);
      if (result != TPM_RC_SUCCESS) {
        return result;
      }
      ++(*num_request_handles);
      return TPM_RC_SUCCESS;
#endif
#ifdef TPM_CC_ClearControl
    case TPM_CC_ClearControl:
      result = TPMI_RH_CLEAR_Unmarshal(
          (TPMI_RH_CLEAR*)&request_handles[*num_request_handles],
          request_handle_buffer_start, request_buffer_remaining_size);
      if (result != TPM_RC_SUCCESS) {
        return result;
      }
      ++(*num_request_handles);
      return TPM_RC_SUCCESS;
#endif
#ifdef TPM_CC_ClockRateAdjust
    case TPM_CC_ClockRateAdjust:
      result = TPMI_RH_PROVISION_Unmarshal(
          (TPMI_RH_PROVISION*)&request_handles[*num_request_handles],
          request_handle_buffer_start, request_buffer_remaining_size);
      if (result != TPM_RC_SUCCESS) {
        return result;
      }
      ++(*num_request_handles);
      return TPM_RC_SUCCESS;
#endif
#ifdef TPM_CC_ClockSet
    case TPM_CC_ClockSet:
      result = TPMI_RH_PROVISION_Unmarshal(
          (TPMI_RH_PROVISION*)&request_handles[*num_request_handles],
          request_handle_buffer_start, request_buffer_remaining_size);
      if (result != TPM_RC_SUCCESS) {
        return result;
      }
      ++(*num_request_handles);
      return TPM_RC_SUCCESS;
#endif
#ifdef TPM_CC_Commit
    case TPM_CC_Commit:
      result = TPMI_DH_OBJECT_Unmarshal(
          (TPMI_DH_OBJECT*)&request_handles[*num_request_handles],
          request_handle_buffer_start, request_buffer_remaining_size, FALSE);
      if (result != TPM_RC_SUCCESS) {
        return result;
      }
      ++(*num_request_handles);
      return TPM_RC_SUCCESS;
#endif
#ifdef TPM_CC_ContextLoad
    case TPM_CC_ContextLoad:
      return TPM_RC_SUCCESS;
#endif
#ifdef TPM_CC_ContextSave
    case TPM_CC_ContextSave:
      result = TPMI_DH_CONTEXT_Unmarshal(
          (TPMI_DH_CONTEXT*)&request_handles[*num_request_handles],
          request_handle_buffer_start, request_buffer_remaining_size);
      if (result != TPM_RC_SUCCESS) {
        return result;
      }
      ++(*num_request_handles);
      return TPM_RC_SUCCESS;
#endif
#ifdef TPM_CC_Create
    case TPM_CC_Create:
      result = TPMI_DH_OBJECT_Unmarshal(
          (TPMI_DH_OBJECT*)&request_handles[*num_request_handles],
          request_handle_buffer_start, request_buffer_remaining_size, FALSE);
      if (result != TPM_RC_SUCCESS) {
        return result;
      }
      ++(*num_request_handles);
      return TPM_RC_SUCCESS;
#endif
#ifdef TPM_CC_CreatePrimary
    case TPM_CC_CreatePrimary:
      result = TPMI_RH_HIERARCHY_Unmarshal(
          (TPMI_RH_HIERARCHY*)&request_handles[*num_request_handles],
          request_handle_buffer_start, request_buffer_remaining_size, TRUE);
      if (result != TPM_RC_SUCCESS) {
        return result;
      }
      ++(*num_request_handles);
      return TPM_RC_SUCCESS;
#endif
#ifdef TPM_CC_DictionaryAttackLockReset
    case TPM_CC_DictionaryAttackLockReset:
      result = TPMI_RH_LOCKOUT_Unmarshal(
          (TPMI_RH_LOCKOUT*)&request_handles[*num_request_handles],
          request_handle_buffer_start, request_buffer_remaining_size);
      if (result != TPM_RC_SUCCESS) {
        return result;
      }
      ++(*num_request_handles);
      return TPM_RC_SUCCESS;
#endif
#ifdef TPM_CC_DictionaryAttackParameters
    case TPM_CC_DictionaryAttackParameters:
      result = TPMI_RH_LOCKOUT_Unmarshal(
          (TPMI_RH_LOCKOUT*)&request_handles[*num_request_handles],
          request_handle_buffer_start, request_buffer_remaining_size);
      if (result != TPM_RC_SUCCESS) {
        return result;
      }
      ++(*num_request_handles);
      return TPM_RC_SUCCESS;
#endif
#ifdef TPM_CC_Duplicate
    case TPM_CC_Duplicate:
      result = TPMI_DH_OBJECT_Unmarshal(
          (TPMI_DH_OBJECT*)&request_handles[*num_request_handles],
          request_handle_buffer_start, request_buffer_remaining_size, FALSE);
      if (result != TPM_RC_SUCCESS) {
        return result;
      }
      ++(*num_request_handles);
      result = TPMI_DH_OBJECT_Unmarshal(
          (TPMI_DH_OBJECT*)&request_handles[*num_request_handles],
          request_handle_buffer_start, request_buffer_remaining_size, TRUE);
      if (result != TPM_RC_SUCCESS) {
        return result;
      }
      ++(*num_request_handles);
      return TPM_RC_SUCCESS;
#endif
#ifdef TPM_CC_ECC_Parameters
    case TPM_CC_ECC_Parameters:
      return TPM_RC_SUCCESS;
#endif
#ifdef TPM_CC_ECDH_KeyGen
    case TPM_CC_ECDH_KeyGen:
      result = TPMI_DH_OBJECT_Unmarshal(
          (TPMI_DH_OBJECT*)&request_handles[*num_request_handles],
          request_handle_buffer_start, request_buffer_remaining_size, FALSE);
      if (result != TPM_RC_SUCCESS) {
        return result;
      }
      ++(*num_request_handles);
      return TPM_RC_SUCCESS;
#endif
#ifdef TPM_CC_ECDH_ZGen
    case TPM_CC_ECDH_ZGen:
      result = TPMI_DH_OBJECT_Unmarshal(
          (TPMI_DH_OBJECT*)&request_handles[*num_request_handles],
          request_handle_buffer_start, request_buffer_remaining_size, FALSE);
      if (result != TPM_RC_SUCCESS) {
        return result;
      }
      ++(*num_request_handles);
      return TPM_RC_SUCCESS;
#endif
#ifdef TPM_CC_EC_Ephemeral
    case TPM_CC_EC_Ephemeral:
      return TPM_RC_SUCCESS;
#endif
#ifdef TPM_CC_EncryptDecrypt
    case TPM_CC_EncryptDecrypt:
      result = TPMI_DH_OBJECT_Unmarshal(
          (TPMI_DH_OBJECT*)&request_handles[*num_request_handles],
          request_handle_buffer_start, request_buffer_remaining_size, FALSE);
      if (result != TPM_RC_SUCCESS) {
        return result;
      }
      ++(*num_request_handles);
      return TPM_RC_SUCCESS;
#endif
#ifdef TPM_CC_EventSequenceComplete
    case TPM_CC_EventSequenceComplete:
      result = TPMI_DH_PCR_Unmarshal(
          (TPMI_DH_PCR*)&request_handles[*num_request_handles],
          request_handle_buffer_start, request_buffer_remaining_size, TRUE);
      if (result != TPM_RC_SUCCESS) {
        return result;
      }
      ++(*num_request_handles);
      result = TPMI_DH_OBJECT_Unmarshal(
          (TPMI_DH_OBJECT*)&request_handles[*num_request_handles],
          request_handle_buffer_start, request_buffer_remaining_size, FALSE);
      if (result != TPM_RC_SUCCESS) {
        return result;
      }
      ++(*num_request_handles);
      return TPM_RC_SUCCESS;
#endif
#ifdef TPM_CC_EvictControl
    case TPM_CC_EvictControl:
      result = TPMI_RH_PROVISION_Unmarshal(
          (TPMI_RH_PROVISION*)&request_handles[*num_request_handles],
          request_handle_buffer_start, request_buffer_remaining_size);
      if (result != TPM_RC_SUCCESS) {
        return result;
      }
      ++(*num_request_handles);
      result = TPMI_DH_OBJECT_Unmarshal(
          (TPMI_DH_OBJECT*)&request_handles[*num_request_handles],
          request_handle_buffer_start, request_buffer_remaining_size, FALSE);
      if (result != TPM_RC_SUCCESS) {
        return result;
      }
      ++(*num_request_handles);
      return TPM_RC_SUCCESS;
#endif
#ifdef TPM_CC_FieldUpgradeData
    case TPM_CC_FieldUpgradeData:
      return TPM_RC_SUCCESS;
#endif
#ifdef TPM_CC_FieldUpgradeStart
    case TPM_CC_FieldUpgradeStart:
      result = TPMI_RH_PLATFORM_Unmarshal(
          (TPMI_RH_PLATFORM*)&request_handles[*num_request_handles],
          request_handle_buffer_start, request_buffer_remaining_size);
      if (result != TPM_RC_SUCCESS) {
        return result;
      }
      ++(*num_request_handles);
      result = TPMI_DH_OBJECT_Unmarshal(
          (TPMI_DH_OBJECT*)&request_handles[*num_request_handles],
          request_handle_buffer_start, request_buffer_remaining_size, FALSE);
      if (result != TPM_RC_SUCCESS) {
        return result;
      }
      ++(*num_request_handles);
      return TPM_RC_SUCCESS;
#endif
#ifdef TPM_CC_FirmwareRead
    case TPM_CC_FirmwareRead:
      return TPM_RC_SUCCESS;
#endif
#ifdef TPM_CC_FlushContext
    case TPM_CC_FlushContext:
      return TPM_RC_SUCCESS;
#endif
#ifdef TPM_CC_GetCapability
    case TPM_CC_GetCapability:
      return TPM_RC_SUCCESS;
#endif
#ifdef TPM_CC_GetCommandAuditDigest
    case TPM_CC_GetCommandAuditDigest:
      result = TPMI_RH_ENDORSEMENT_Unmarshal(
          (TPMI_RH_ENDORSEMENT*)&request_handles[*num_request_handles],
          request_handle_buffer_start, request_buffer_remaining_size, FALSE);
      if (result != TPM_RC_SUCCESS) {
        return result;
      }
      ++(*num_request_handles);
      result = TPMI_DH_OBJECT_Unmarshal(
          (TPMI_DH_OBJECT*)&request_handles[*num_request_handles],
          request_handle_buffer_start, request_buffer_remaining_size, TRUE);
      if (result != TPM_RC_SUCCESS) {
        return result;
      }
      ++(*num_request_handles);
      return TPM_RC_SUCCESS;
#endif
#ifdef TPM_CC_GetRandom
    case TPM_CC_GetRandom:
      return TPM_RC_SUCCESS;
#endif
#ifdef TPM_CC_GetSessionAuditDigest
    case TPM_CC_GetSessionAuditDigest:
      result = TPMI_RH_ENDORSEMENT_Unmarshal(
          (TPMI_RH_ENDORSEMENT*)&request_handles[*num_request_handles],
          request_handle_buffer_start, request_buffer_remaining_size, FALSE);
      if (result != TPM_RC_SUCCESS) {
        return result;
      }
      ++(*num_request_handles);
      result = TPMI_DH_OBJECT_Unmarshal(
          (TPMI_DH_OBJECT*)&request_handles[*num_request_handles],
          request_handle_buffer_start, request_buffer_remaining_size, TRUE);
      if (result != TPM_RC_SUCCESS) {
        return result;
      }
      ++(*num_request_handles);
      result = TPMI_SH_HMAC_Unmarshal(
          (TPMI_SH_HMAC*)&request_handles[*num_request_handles],
          request_handle_buffer_start, request_buffer_remaining_size);
      if (result != TPM_RC_SUCCESS) {
        return result;
      }
      ++(*num_request_handles);
      return TPM_RC_SUCCESS;
#endif
#ifdef TPM_CC_GetTestResult
    case TPM_CC_GetTestResult:
      return TPM_RC_SUCCESS;
#endif
#ifdef TPM_CC_GetTime
    case TPM_CC_GetTime:
      result = TPMI_RH_ENDORSEMENT_Unmarshal(
          (TPMI_RH_ENDORSEMENT*)&request_handles[*num_request_handles],
          request_handle_buffer_start, request_buffer_remaining_size, FALSE);
      if (result != TPM_RC_SUCCESS) {
        return result;
      }
      ++(*num_request_handles);
      result = TPMI_DH_OBJECT_Unmarshal(
          (TPMI_DH_OBJECT*)&request_handles[*num_request_handles],
          request_handle_buffer_start, request_buffer_remaining_size, TRUE);
      if (result != TPM_RC_SUCCESS) {
        return result;
      }
      ++(*num_request_handles);
      return TPM_RC_SUCCESS;
#endif
#ifdef TPM_CC_HMAC
    case TPM_CC_HMAC:
      result = TPMI_DH_OBJECT_Unmarshal(
          (TPMI_DH_OBJECT*)&request_handles[*num_request_handles],
          request_handle_buffer_start, request_buffer_remaining_size, FALSE);
      if (result != TPM_RC_SUCCESS) {
        return result;
      }
      ++(*num_request_handles);
      return TPM_RC_SUCCESS;
#endif
#ifdef TPM_CC_HMAC_Start
    case TPM_CC_HMAC_Start:
      result = TPMI_DH_OBJECT_Unmarshal(
          (TPMI_DH_OBJECT*)&request_handles[*num_request_handles],
          request_handle_buffer_start, request_buffer_remaining_size, FALSE);
      if (result != TPM_RC_SUCCESS) {
        return result;
      }
      ++(*num_request_handles);
      return TPM_RC_SUCCESS;
#endif
#ifdef TPM_CC_Hash
    case TPM_CC_Hash:
      return TPM_RC_SUCCESS;
#endif
#ifdef TPM_CC_HashSequenceStart
    case TPM_CC_HashSequenceStart:
      return TPM_RC_SUCCESS;
#endif
#ifdef TPM_CC_HierarchyChangeAuth
    case TPM_CC_HierarchyChangeAuth:
      result = TPMI_RH_HIERARCHY_AUTH_Unmarshal(
          (TPMI_RH_HIERARCHY_AUTH*)&request_handles[*num_request_handles],
          request_handle_buffer_start, request_buffer_remaining_size);
      if (result != TPM_RC_SUCCESS) {
        return result;
      }
      ++(*num_request_handles);
      return TPM_RC_SUCCESS;
#endif
#ifdef TPM_CC_HierarchyControl
    case TPM_CC_HierarchyControl:
      result = TPMI_RH_HIERARCHY_Unmarshal(
          (TPMI_RH_HIERARCHY*)&request_handles[*num_request_handles],
          request_handle_buffer_start, request_buffer_remaining_size, FALSE);
      if (result != TPM_RC_SUCCESS) {
        return result;
      }
      ++(*num_request_handles);
      return TPM_RC_SUCCESS;
#endif
#ifdef TPM_CC_Import
    case TPM_CC_Import:
      result = TPMI_DH_OBJECT_Unmarshal(
          (TPMI_DH_OBJECT*)&request_handles[*num_request_handles],
          request_handle_buffer_start, request_buffer_remaining_size, FALSE);
      if (result != TPM_RC_SUCCESS) {
        return result;
      }
      ++(*num_request_handles);
      return TPM_RC_SUCCESS;
#endif
#ifdef TPM_CC_IncrementalSelfTest
    case TPM_CC_IncrementalSelfTest:
      return TPM_RC_SUCCESS;
#endif
#ifdef TPM_CC_Load
    case TPM_CC_Load:
      result = TPMI_DH_OBJECT_Unmarshal(
          (TPMI_DH_OBJECT*)&request_handles[*num_request_handles],
          request_handle_buffer_start, request_buffer_remaining_size, FALSE);
      if (result != TPM_RC_SUCCESS) {
        return result;
      }
      ++(*num_request_handles);
      return TPM_RC_SUCCESS;
#endif
#ifdef TPM_CC_LoadExternal
    case TPM_CC_LoadExternal:
      return TPM_RC_SUCCESS;
#endif
#ifdef TPM_CC_MakeCredential
    case TPM_CC_MakeCredential:
      result = TPMI_DH_OBJECT_Unmarshal(
          (TPMI_DH_OBJECT*)&request_handles[*num_request_handles],
          request_handle_buffer_start, request_buffer_remaining_size, FALSE);
      if (result != TPM_RC_SUCCESS) {
        return result;
      }
      ++(*num_request_handles);
      return TPM_RC_SUCCESS;
#endif
#ifdef TPM_CC_NV_Certify
    case TPM_CC_NV_Certify:
      result = TPMI_DH_OBJECT_Unmarshal(
          (TPMI_DH_OBJECT*)&request_handles[*num_request_handles],
          request_handle_buffer_start, request_buffer_remaining_size, TRUE);
      if (result != TPM_RC_SUCCESS) {
        return result;
      }
      ++(*num_request_handles);
      result = TPMI_RH_NV_AUTH_Unmarshal(
          (TPMI_RH_NV_AUTH*)&request_handles[*num_request_handles],
          request_handle_buffer_start, request_buffer_remaining_size);
      if (result != TPM_RC_SUCCESS) {
        return result;
      }
      ++(*num_request_handles);
      result = TPMI_RH_NV_INDEX_Unmarshal(
          (TPMI_RH_NV_INDEX*)&request_handles[*num_request_handles],
          request_handle_buffer_start, request_buffer_remaining_size);
      if (result != TPM_RC_SUCCESS) {
        return result;
      }
      ++(*num_request_handles);
      return TPM_RC_SUCCESS;
#endif
#ifdef TPM_CC_NV_ChangeAuth
    case TPM_CC_NV_ChangeAuth:
      result = TPMI_RH_NV_INDEX_Unmarshal(
          (TPMI_RH_NV_INDEX*)&request_handles[*num_request_handles],
          request_handle_buffer_start, request_buffer_remaining_size);
      if (result != TPM_RC_SUCCESS) {
        return result;
      }
      ++(*num_request_handles);
      return TPM_RC_SUCCESS;
#endif
#ifdef TPM_CC_NV_DefineSpace
    case TPM_CC_NV_DefineSpace:
      result = TPMI_RH_PROVISION_Unmarshal(
          (TPMI_RH_PROVISION*)&request_handles[*num_request_handles],
          request_handle_buffer_start, request_buffer_remaining_size);
      if (result != TPM_RC_SUCCESS) {
        return result;
      }
      ++(*num_request_handles);
      return TPM_RC_SUCCESS;
#endif
#ifdef TPM_CC_NV_Extend
    case TPM_CC_NV_Extend:
      result = TPMI_RH_NV_AUTH_Unmarshal(
          (TPMI_RH_NV_AUTH*)&request_handles[*num_request_handles],
          request_handle_buffer_start, request_buffer_remaining_size);
      if (result != TPM_RC_SUCCESS) {
        return result;
      }
      ++(*num_request_handles);
      result = TPMI_RH_NV_INDEX_Unmarshal(
          (TPMI_RH_NV_INDEX*)&request_handles[*num_request_handles],
          request_handle_buffer_start, request_buffer_remaining_size);
      if (result != TPM_RC_SUCCESS) {
        return result;
      }
      ++(*num_request_handles);
      return TPM_RC_SUCCESS;
#endif
#ifdef TPM_CC_NV_GlobalWriteLock
    case TPM_CC_NV_GlobalWriteLock:
      result = TPMI_RH_PROVISION_Unmarshal(
          (TPMI_RH_PROVISION*)&request_handles[*num_request_handles],
          request_handle_buffer_start, request_buffer_remaining_size);
      if (result != TPM_RC_SUCCESS) {
        return result;
      }
      ++(*num_request_handles);
      return TPM_RC_SUCCESS;
#endif
#ifdef TPM_CC_NV_Increment
    case TPM_CC_NV_Increment:
      result = TPMI_RH_NV_AUTH_Unmarshal(
          (TPMI_RH_NV_AUTH*)&request_handles[*num_request_handles],
          request_handle_buffer_start, request_buffer_remaining_size);
      if (result != TPM_RC_SUCCESS) {
        return result;
      }
      ++(*num_request_handles);
      result = TPMI_RH_NV_INDEX_Unmarshal(
          (TPMI_RH_NV_INDEX*)&request_handles[*num_request_handles],
          request_handle_buffer_start, request_buffer_remaining_size);
      if (result != TPM_RC_SUCCESS) {
        return result;
      }
      ++(*num_request_handles);
      return TPM_RC_SUCCESS;
#endif
#ifdef TPM_CC_NV_Read
    case TPM_CC_NV_Read:
      result = TPMI_RH_NV_AUTH_Unmarshal(
          (TPMI_RH_NV_AUTH*)&request_handles[*num_request_handles],
          request_handle_buffer_start, request_buffer_remaining_size);
      if (result != TPM_RC_SUCCESS) {
        return result;
      }
      ++(*num_request_handles);
      result = TPMI_RH_NV_INDEX_Unmarshal(
          (TPMI_RH_NV_INDEX*)&request_handles[*num_request_handles],
          request_handle_buffer_start, request_buffer_remaining_size);
      if (result != TPM_RC_SUCCESS) {
        return result;
      }
      ++(*num_request_handles);
      return TPM_RC_SUCCESS;
#endif
#ifdef TPM_CC_NV_ReadLock
    case TPM_CC_NV_ReadLock:
      result = TPMI_RH_NV_AUTH_Unmarshal(
          (TPMI_RH_NV_AUTH*)&request_handles[*num_request_handles],
          request_handle_buffer_start, request_buffer_remaining_size);
      if (result != TPM_RC_SUCCESS) {
        return result;
      }
      ++(*num_request_handles);
      result = TPMI_RH_NV_INDEX_Unmarshal(
          (TPMI_RH_NV_INDEX*)&request_handles[*num_request_handles],
          request_handle_buffer_start, request_buffer_remaining_size);
      if (result != TPM_RC_SUCCESS) {
        return result;
      }
      ++(*num_request_handles);
      return TPM_RC_SUCCESS;
#endif
#ifdef TPM_CC_NV_ReadPublic
    case TPM_CC_NV_ReadPublic:
      result = TPMI_RH_NV_INDEX_Unmarshal(
          (TPMI_RH_NV_INDEX*)&request_handles[*num_request_handles],
          request_handle_buffer_start, request_buffer_remaining_size);
      if (result != TPM_RC_SUCCESS) {
        return result;
      }
      ++(*num_request_handles);
      return TPM_RC_SUCCESS;
#endif
#ifdef TPM_CC_NV_SetBits
    case TPM_CC_NV_SetBits:
      result = TPMI_RH_NV_AUTH_Unmarshal(
          (TPMI_RH_NV_AUTH*)&request_handles[*num_request_handles],
          request_handle_buffer_start, request_buffer_remaining_size);
      if (result != TPM_RC_SUCCESS) {
        return result;
      }
      ++(*num_request_handles);
      result = TPMI_RH_NV_INDEX_Unmarshal(
          (TPMI_RH_NV_INDEX*)&request_handles[*num_request_handles],
          request_handle_buffer_start, request_buffer_remaining_size);
      if (result != TPM_RC_SUCCESS) {
        return result;
      }
      ++(*num_request_handles);
      return TPM_RC_SUCCESS;
#endif
#ifdef TPM_CC_NV_UndefineSpace
    case TPM_CC_NV_UndefineSpace:
      result = TPMI_RH_PROVISION_Unmarshal(
          (TPMI_RH_PROVISION*)&request_handles[*num_request_handles],
          request_handle_buffer_start, request_buffer_remaining_size);
      if (result != TPM_RC_SUCCESS) {
        return result;
      }
      ++(*num_request_handles);
      result = TPMI_RH_NV_INDEX_Unmarshal(
          (TPMI_RH_NV_INDEX*)&request_handles[*num_request_handles],
          request_handle_buffer_start, request_buffer_remaining_size);
      if (result != TPM_RC_SUCCESS) {
        return result;
      }
      ++(*num_request_handles);
      return TPM_RC_SUCCESS;
#endif
#ifdef TPM_CC_NV_UndefineSpaceSpecial
    case TPM_CC_NV_UndefineSpaceSpecial:
      result = TPMI_RH_NV_INDEX_Unmarshal(
          (TPMI_RH_NV_INDEX*)&request_handles[*num_request_handles],
          request_handle_buffer_start, request_buffer_remaining_size);
      if (result != TPM_RC_SUCCESS) {
        return result;
      }
      ++(*num_request_handles);
      result = TPMI_RH_PLATFORM_Unmarshal(
          (TPMI_RH_PLATFORM*)&request_handles[*num_request_handles],
          request_handle_buffer_start, request_buffer_remaining_size);
      if (result != TPM_RC_SUCCESS) {
        return result;
      }
      ++(*num_request_handles);
      return TPM_RC_SUCCESS;
#endif
#ifdef TPM_CC_NV_Write
    case TPM_CC_NV_Write:
      result = TPMI_RH_NV_AUTH_Unmarshal(
          (TPMI_RH_NV_AUTH*)&request_handles[*num_request_handles],
          request_handle_buffer_start, request_buffer_remaining_size);
      if (result != TPM_RC_SUCCESS) {
        return result;
      }
      ++(*num_request_handles);
      result = TPMI_RH_NV_INDEX_Unmarshal(
          (TPMI_RH_NV_INDEX*)&request_handles[*num_request_handles],
          request_handle_buffer_start, request_buffer_remaining_size);
      if (result != TPM_RC_SUCCESS) {
        return result;
      }
      ++(*num_request_handles);
      return TPM_RC_SUCCESS;
#endif
#ifdef TPM_CC_NV_WriteLock
    case TPM_CC_NV_WriteLock:
      result = TPMI_RH_NV_AUTH_Unmarshal(
          (TPMI_RH_NV_AUTH*)&request_handles[*num_request_handles],
          request_handle_buffer_start, request_buffer_remaining_size);
      if (result != TPM_RC_SUCCESS) {
        return result;
      }
      ++(*num_request_handles);
      result = TPMI_RH_NV_INDEX_Unmarshal(
          (TPMI_RH_NV_INDEX*)&request_handles[*num_request_handles],
          request_handle_buffer_start, request_buffer_remaining_size);
      if (result != TPM_RC_SUCCESS) {
        return result;
      }
      ++(*num_request_handles);
      return TPM_RC_SUCCESS;
#endif
#ifdef TPM_CC_ObjectChangeAuth
    case TPM_CC_ObjectChangeAuth:
      result = TPMI_DH_OBJECT_Unmarshal(
          (TPMI_DH_OBJECT*)&request_handles[*num_request_handles],
          request_handle_buffer_start, request_buffer_remaining_size, FALSE);
      if (result != TPM_RC_SUCCESS) {
        return result;
      }
      ++(*num_request_handles);
      result = TPMI_DH_OBJECT_Unmarshal(
          (TPMI_DH_OBJECT*)&request_handles[*num_request_handles],
          request_handle_buffer_start, request_buffer_remaining_size, FALSE);
      if (result != TPM_RC_SUCCESS) {
        return result;
      }
      ++(*num_request_handles);
      return TPM_RC_SUCCESS;
#endif
#ifdef TPM_CC_PCR_Allocate
    case TPM_CC_PCR_Allocate:
      result = TPMI_RH_PLATFORM_Unmarshal(
          (TPMI_RH_PLATFORM*)&request_handles[*num_request_handles],
          request_handle_buffer_start, request_buffer_remaining_size);
      if (result != TPM_RC_SUCCESS) {
        return result;
      }
      ++(*num_request_handles);
      return TPM_RC_SUCCESS;
#endif
#ifdef TPM_CC_PCR_Event
    case TPM_CC_PCR_Event:
      result = TPMI_DH_PCR_Unmarshal(
          (TPMI_DH_PCR*)&request_handles[*num_request_handles],
          request_handle_buffer_start, request_buffer_remaining_size, TRUE);
      if (result != TPM_RC_SUCCESS) {
        return result;
      }
      ++(*num_request_handles);
      return TPM_RC_SUCCESS;
#endif
#ifdef TPM_CC_PCR_Extend
    case TPM_CC_PCR_Extend:
      result = TPMI_DH_PCR_Unmarshal(
          (TPMI_DH_PCR*)&request_handles[*num_request_handles],
          request_handle_buffer_start, request_buffer_remaining_size, TRUE);
      if (result != TPM_RC_SUCCESS) {
        return result;
      }
      ++(*num_request_handles);
      return TPM_RC_SUCCESS;
#endif
#ifdef TPM_CC_PCR_Read
    case TPM_CC_PCR_Read:
      return TPM_RC_SUCCESS;
#endif
#ifdef TPM_CC_PCR_Reset
    case TPM_CC_PCR_Reset:
      result = TPMI_DH_PCR_Unmarshal(
          (TPMI_DH_PCR*)&request_handles[*num_request_handles],
          request_handle_buffer_start, request_buffer_remaining_size, FALSE);
      if (result != TPM_RC_SUCCESS) {
        return result;
      }
      ++(*num_request_handles);
      return TPM_RC_SUCCESS;
#endif
#ifdef TPM_CC_PCR_SetAuthPolicy
    case TPM_CC_PCR_SetAuthPolicy:
      result = TPMI_RH_PLATFORM_Unmarshal(
          (TPMI_RH_PLATFORM*)&request_handles[*num_request_handles],
          request_handle_buffer_start, request_buffer_remaining_size);
      if (result != TPM_RC_SUCCESS) {
        return result;
      }
      ++(*num_request_handles);
      result = TPMI_DH_PCR_Unmarshal(
          (TPMI_DH_PCR*)&request_handles[*num_request_handles],
          request_handle_buffer_start, request_buffer_remaining_size, FALSE);
      if (result != TPM_RC_SUCCESS) {
        return result;
      }
      ++(*num_request_handles);
      return TPM_RC_SUCCESS;
#endif
#ifdef TPM_CC_PCR_SetAuthValue
    case TPM_CC_PCR_SetAuthValue:
      result = TPMI_DH_PCR_Unmarshal(
          (TPMI_DH_PCR*)&request_handles[*num_request_handles],
          request_handle_buffer_start, request_buffer_remaining_size, FALSE);
      if (result != TPM_RC_SUCCESS) {
        return result;
      }
      ++(*num_request_handles);
      return TPM_RC_SUCCESS;
#endif
#ifdef TPM_CC_PP_Commands
    case TPM_CC_PP_Commands:
      result = TPMI_RH_PLATFORM_Unmarshal(
          (TPMI_RH_PLATFORM*)&request_handles[*num_request_handles],
          request_handle_buffer_start, request_buffer_remaining_size);
      if (result != TPM_RC_SUCCESS) {
        return result;
      }
      ++(*num_request_handles);
      return TPM_RC_SUCCESS;
#endif
#ifdef TPM_CC_PolicyAuthValue
    case TPM_CC_PolicyAuthValue:
      result = TPMI_SH_POLICY_Unmarshal(
          (TPMI_SH_POLICY*)&request_handles[*num_request_handles],
          request_handle_buffer_start, request_buffer_remaining_size);
      if (result != TPM_RC_SUCCESS) {
        return result;
      }
      ++(*num_request_handles);
      return TPM_RC_SUCCESS;
#endif
#ifdef TPM_CC_PolicyAuthorize
    case TPM_CC_PolicyAuthorize:
      result = TPMI_SH_POLICY_Unmarshal(
          (TPMI_SH_POLICY*)&request_handles[*num_request_handles],
          request_handle_buffer_start, request_buffer_remaining_size);
      if (result != TPM_RC_SUCCESS) {
        return result;
      }
      ++(*num_request_handles);
      return TPM_RC_SUCCESS;
#endif
#ifdef TPM_CC_PolicyCommandCode
    case TPM_CC_PolicyCommandCode:
      result = TPMI_SH_POLICY_Unmarshal(
          (TPMI_SH_POLICY*)&request_handles[*num_request_handles],
          request_handle_buffer_start, request_buffer_remaining_size);
      if (result != TPM_RC_SUCCESS) {
        return result;
      }
      ++(*num_request_handles);
      return TPM_RC_SUCCESS;
#endif
#ifdef TPM_CC_PolicyCounterTimer
    case TPM_CC_PolicyCounterTimer:
      result = TPMI_SH_POLICY_Unmarshal(
          (TPMI_SH_POLICY*)&request_handles[*num_request_handles],
          request_handle_buffer_start, request_buffer_remaining_size);
      if (result != TPM_RC_SUCCESS) {
        return result;
      }
      ++(*num_request_handles);
      return TPM_RC_SUCCESS;
#endif
#ifdef TPM_CC_PolicyCpHash
    case TPM_CC_PolicyCpHash:
      result = TPMI_SH_POLICY_Unmarshal(
          (TPMI_SH_POLICY*)&request_handles[*num_request_handles],
          request_handle_buffer_start, request_buffer_remaining_size);
      if (result != TPM_RC_SUCCESS) {
        return result;
      }
      ++(*num_request_handles);
      return TPM_RC_SUCCESS;
#endif
#ifdef TPM_CC_PolicyDuplicationSelect
    case TPM_CC_PolicyDuplicationSelect:
      result = TPMI_SH_POLICY_Unmarshal(
          (TPMI_SH_POLICY*)&request_handles[*num_request_handles],
          request_handle_buffer_start, request_buffer_remaining_size);
      if (result != TPM_RC_SUCCESS) {
        return result;
      }
      ++(*num_request_handles);
      return TPM_RC_SUCCESS;
#endif
#ifdef TPM_CC_PolicyGetDigest
    case TPM_CC_PolicyGetDigest:
      result = TPMI_SH_POLICY_Unmarshal(
          (TPMI_SH_POLICY*)&request_handles[*num_request_handles],
          request_handle_buffer_start, request_buffer_remaining_size);
      if (result != TPM_RC_SUCCESS) {
        return result;
      }
      ++(*num_request_handles);
      return TPM_RC_SUCCESS;
#endif
#ifdef TPM_CC_PolicyLocality
    case TPM_CC_PolicyLocality:
      result = TPMI_SH_POLICY_Unmarshal(
          (TPMI_SH_POLICY*)&request_handles[*num_request_handles],
          request_handle_buffer_start, request_buffer_remaining_size);
      if (result != TPM_RC_SUCCESS) {
        return result;
      }
      ++(*num_request_handles);
      return TPM_RC_SUCCESS;
#endif
#ifdef TPM_CC_PolicyNV
    case TPM_CC_PolicyNV:
      result = TPMI_RH_NV_AUTH_Unmarshal(
          (TPMI_RH_NV_AUTH*)&request_handles[*num_request_handles],
          request_handle_buffer_start, request_buffer_remaining_size);
      if (result != TPM_RC_SUCCESS) {
        return result;
      }
      ++(*num_request_handles);
      result = TPMI_RH_NV_INDEX_Unmarshal(
          (TPMI_RH_NV_INDEX*)&request_handles[*num_request_handles],
          request_handle_buffer_start, request_buffer_remaining_size);
      if (result != TPM_RC_SUCCESS) {
        return result;
      }
      ++(*num_request_handles);
      result = TPMI_SH_POLICY_Unmarshal(
          (TPMI_SH_POLICY*)&request_handles[*num_request_handles],
          request_handle_buffer_start, request_buffer_remaining_size);
      if (result != TPM_RC_SUCCESS) {
        return result;
      }
      ++(*num_request_handles);
      return TPM_RC_SUCCESS;
#endif
#ifdef TPM_CC_PolicyNameHash
    case TPM_CC_PolicyNameHash:
      result = TPMI_SH_POLICY_Unmarshal(
          (TPMI_SH_POLICY*)&request_handles[*num_request_handles],
          request_handle_buffer_start, request_buffer_remaining_size);
      if (result != TPM_RC_SUCCESS) {
        return result;
      }
      ++(*num_request_handles);
      return TPM_RC_SUCCESS;
#endif
#ifdef TPM_CC_PolicyNvWritten
    case TPM_CC_PolicyNvWritten:
      result = TPMI_SH_POLICY_Unmarshal(
          (TPMI_SH_POLICY*)&request_handles[*num_request_handles],
          request_handle_buffer_start, request_buffer_remaining_size);
      if (result != TPM_RC_SUCCESS) {
        return result;
      }
      ++(*num_request_handles);
      return TPM_RC_SUCCESS;
#endif
#ifdef TPM_CC_PolicyOR
    case TPM_CC_PolicyOR:
      result = TPMI_SH_POLICY_Unmarshal(
          (TPMI_SH_POLICY*)&request_handles[*num_request_handles],
          request_handle_buffer_start, request_buffer_remaining_size);
      if (result != TPM_RC_SUCCESS) {
        return result;
      }
      ++(*num_request_handles);
      return TPM_RC_SUCCESS;
#endif
#ifdef TPM_CC_PolicyPCR
    case TPM_CC_PolicyPCR:
      result = TPMI_SH_POLICY_Unmarshal(
          (TPMI_SH_POLICY*)&request_handles[*num_request_handles],
          request_handle_buffer_start, request_buffer_remaining_size);
      if (result != TPM_RC_SUCCESS) {
        return result;
      }
      ++(*num_request_handles);
      return TPM_RC_SUCCESS;
#endif
#ifdef TPM_CC_PolicyPassword
    case TPM_CC_PolicyPassword:
      result = TPMI_SH_POLICY_Unmarshal(
          (TPMI_SH_POLICY*)&request_handles[*num_request_handles],
          request_handle_buffer_start, request_buffer_remaining_size);
      if (result != TPM_RC_SUCCESS) {
        return result;
      }
      ++(*num_request_handles);
      return TPM_RC_SUCCESS;
#endif
#ifdef TPM_CC_PolicyPhysicalPresence
    case TPM_CC_PolicyPhysicalPresence:
      result = TPMI_SH_POLICY_Unmarshal(
          (TPMI_SH_POLICY*)&request_handles[*num_request_handles],
          request_handle_buffer_start, request_buffer_remaining_size);
      if (result != TPM_RC_SUCCESS) {
        return result;
      }
      ++(*num_request_handles);
      return TPM_RC_SUCCESS;
#endif
#ifdef TPM_CC_PolicyRestart
    case TPM_CC_PolicyRestart:
      result = TPMI_SH_POLICY_Unmarshal(
          (TPMI_SH_POLICY*)&request_handles[*num_request_handles],
          request_handle_buffer_start, request_buffer_remaining_size);
      if (result != TPM_RC_SUCCESS) {
        return result;
      }
      ++(*num_request_handles);
      return TPM_RC_SUCCESS;
#endif
#ifdef TPM_CC_PolicySecret
    case TPM_CC_PolicySecret:
      result = TPMI_DH_ENTITY_Unmarshal(
          (TPMI_DH_ENTITY*)&request_handles[*num_request_handles],
          request_handle_buffer_start, request_buffer_remaining_size, FALSE);
      if (result != TPM_RC_SUCCESS) {
        return result;
      }
      ++(*num_request_handles);
      result = TPMI_SH_POLICY_Unmarshal(
          (TPMI_SH_POLICY*)&request_handles[*num_request_handles],
          request_handle_buffer_start, request_buffer_remaining_size);
      if (result != TPM_RC_SUCCESS) {
        return result;
      }
      ++(*num_request_handles);
      return TPM_RC_SUCCESS;
#endif
#ifdef TPM_CC_PolicySigned
    case TPM_CC_PolicySigned:
      result = TPMI_DH_OBJECT_Unmarshal(
          (TPMI_DH_OBJECT*)&request_handles[*num_request_handles],
          request_handle_buffer_start, request_buffer_remaining_size, FALSE);
      if (result != TPM_RC_SUCCESS) {
        return result;
      }
      ++(*num_request_handles);
      result = TPMI_SH_POLICY_Unmarshal(
          (TPMI_SH_POLICY*)&request_handles[*num_request_handles],
          request_handle_buffer_start, request_buffer_remaining_size);
      if (result != TPM_RC_SUCCESS) {
        return result;
      }
      ++(*num_request_handles);
      return TPM_RC_SUCCESS;
#endif
#ifdef TPM_CC_PolicyTicket
    case TPM_CC_PolicyTicket:
      result = TPMI_SH_POLICY_Unmarshal(
          (TPMI_SH_POLICY*)&request_handles[*num_request_handles],
          request_handle_buffer_start, request_buffer_remaining_size);
      if (result != TPM_RC_SUCCESS) {
        return result;
      }
      ++(*num_request_handles);
      return TPM_RC_SUCCESS;
#endif
#ifdef TPM_CC_Quote
    case TPM_CC_Quote:
      result = TPMI_DH_OBJECT_Unmarshal(
          (TPMI_DH_OBJECT*)&request_handles[*num_request_handles],
          request_handle_buffer_start, request_buffer_remaining_size, TRUE);
      if (result != TPM_RC_SUCCESS) {
        return result;
      }
      ++(*num_request_handles);
      return TPM_RC_SUCCESS;
#endif
#ifdef TPM_CC_RSA_Decrypt
    case TPM_CC_RSA_Decrypt:
      result = TPMI_DH_OBJECT_Unmarshal(
          (TPMI_DH_OBJECT*)&request_handles[*num_request_handles],
          request_handle_buffer_start, request_buffer_remaining_size, FALSE);
      if (result != TPM_RC_SUCCESS) {
        return result;
      }
      ++(*num_request_handles);
      return TPM_RC_SUCCESS;
#endif
#ifdef TPM_CC_RSA_Encrypt
    case TPM_CC_RSA_Encrypt:
      result = TPMI_DH_OBJECT_Unmarshal(
          (TPMI_DH_OBJECT*)&request_handles[*num_request_handles],
          request_handle_buffer_start, request_buffer_remaining_size, FALSE);
      if (result != TPM_RC_SUCCESS) {
        return result;
      }
      ++(*num_request_handles);
      return TPM_RC_SUCCESS;
#endif
#ifdef TPM_CC_ReadClock
    case TPM_CC_ReadClock:
      return TPM_RC_SUCCESS;
#endif
#ifdef TPM_CC_ReadPublic
    case TPM_CC_ReadPublic:
      result = TPMI_DH_OBJECT_Unmarshal(
          (TPMI_DH_OBJECT*)&request_handles[*num_request_handles],
          request_handle_buffer_start, request_buffer_remaining_size, FALSE);
      if (result != TPM_RC_SUCCESS) {
        return result;
      }
      ++(*num_request_handles);
      return TPM_RC_SUCCESS;
#endif
#ifdef TPM_CC_Rewrap
    case TPM_CC_Rewrap:
      result = TPMI_DH_OBJECT_Unmarshal(
          (TPMI_DH_OBJECT*)&request_handles[*num_request_handles],
          request_handle_buffer_start, request_buffer_remaining_size, TRUE);
      if (result != TPM_RC_SUCCESS) {
        return result;
      }
      ++(*num_request_handles);
      result = TPMI_DH_OBJECT_Unmarshal(
          (TPMI_DH_OBJECT*)&request_handles[*num_request_handles],
          request_handle_buffer_start, request_buffer_remaining_size, TRUE);
      if (result != TPM_RC_SUCCESS) {
        return result;
      }
      ++(*num_request_handles);
      return TPM_RC_SUCCESS;
#endif
#ifdef TPM_CC_SelfTest
    case TPM_CC_SelfTest:
      return TPM_RC_SUCCESS;
#endif
#ifdef TPM_CC_SequenceComplete
    case TPM_CC_SequenceComplete:
      result = TPMI_DH_OBJECT_Unmarshal(
          (TPMI_DH_OBJECT*)&request_handles[*num_request_handles],
          request_handle_buffer_start, request_buffer_remaining_size, FALSE);
      if (result != TPM_RC_SUCCESS) {
        return result;
      }
      ++(*num_request_handles);
      return TPM_RC_SUCCESS;
#endif
#ifdef TPM_CC_SequenceUpdate
    case TPM_CC_SequenceUpdate:
      result = TPMI_DH_OBJECT_Unmarshal(
          (TPMI_DH_OBJECT*)&request_handles[*num_request_handles],
          request_handle_buffer_start, request_buffer_remaining_size, FALSE);
      if (result != TPM_RC_SUCCESS) {
        return result;
      }
      ++(*num_request_handles);
      return TPM_RC_SUCCESS;
#endif
#ifdef TPM_CC_SetAlgorithmSet
    case TPM_CC_SetAlgorithmSet:
      result = TPMI_RH_PLATFORM_Unmarshal(
          (TPMI_RH_PLATFORM*)&request_handles[*num_request_handles],
          request_handle_buffer_start, request_buffer_remaining_size);
      if (result != TPM_RC_SUCCESS) {
        return result;
      }
      ++(*num_request_handles);
      return TPM_RC_SUCCESS;
#endif
#ifdef TPM_CC_SetCommandCodeAuditStatus
    case TPM_CC_SetCommandCodeAuditStatus:
      result = TPMI_RH_PROVISION_Unmarshal(
          (TPMI_RH_PROVISION*)&request_handles[*num_request_handles],
          request_handle_buffer_start, request_buffer_remaining_size);
      if (result != TPM_RC_SUCCESS) {
        return result;
      }
      ++(*num_request_handles);
      return TPM_RC_SUCCESS;
#endif
#ifdef TPM_CC_SetPrimaryPolicy
    case TPM_CC_SetPrimaryPolicy:
      result = TPMI_RH_HIERARCHY_AUTH_Unmarshal(
          (TPMI_RH_HIERARCHY_AUTH*)&request_handles[*num_request_handles],
          request_handle_buffer_start, request_buffer_remaining_size);
      if (result != TPM_RC_SUCCESS) {
        return result;
      }
      ++(*num_request_handles);
      return TPM_RC_SUCCESS;
#endif
#ifdef TPM_CC_Shutdown
    case TPM_CC_Shutdown:
      return TPM_RC_SUCCESS;
#endif
#ifdef TPM_CC_Sign
    case TPM_CC_Sign:
      result = TPMI_DH_OBJECT_Unmarshal(
          (TPMI_DH_OBJECT*)&request_handles[*num_request_handles],
          request_handle_buffer_start, request_buffer_remaining_size, FALSE);
      if (result != TPM_RC_SUCCESS) {
        return result;
      }
      ++(*num_request_handles);
      return TPM_RC_SUCCESS;
#endif
#ifdef TPM_CC_StartAuthSession
    case TPM_CC_StartAuthSession:
      result = TPMI_DH_OBJECT_Unmarshal(
          (TPMI_DH_OBJECT*)&request_handles[*num_request_handles],
          request_handle_buffer_start, request_buffer_remaining_size, TRUE);
      if (result != TPM_RC_SUCCESS) {
        return result;
      }
      ++(*num_request_handles);
      result = TPMI_DH_ENTITY_Unmarshal(
          (TPMI_DH_ENTITY*)&request_handles[*num_request_handles],
          request_handle_buffer_start, request_buffer_remaining_size, TRUE);
      if (result != TPM_RC_SUCCESS) {
        return result;
      }
      ++(*num_request_handles);
      return TPM_RC_SUCCESS;
#endif
#ifdef TPM_CC_Startup
    case TPM_CC_Startup:
      return TPM_RC_SUCCESS;
#endif
#ifdef TPM_CC_StirRandom
    case TPM_CC_StirRandom:
      return TPM_RC_SUCCESS;
#endif
#ifdef TPM_CC_TestParms
    case TPM_CC_TestParms:
      return TPM_RC_SUCCESS;
#endif
#ifdef TPM_CC_Unseal
    case TPM_CC_Unseal:
      result = TPMI_DH_OBJECT_Unmarshal(
          (TPMI_DH_OBJECT*)&request_handles[*num_request_handles],
          request_handle_buffer_start, request_buffer_remaining_size, FALSE);
      if (result != TPM_RC_SUCCESS) {
        return result;
      }
      ++(*num_request_handles);
      return TPM_RC_SUCCESS;
#endif
#ifdef TPM_CC_VerifySignature
    case TPM_CC_VerifySignature:
      result = TPMI_DH_OBJECT_Unmarshal(
          (TPMI_DH_OBJECT*)&request_handles[*num_request_handles],
          request_handle_buffer_start, request_buffer_remaining_size, FALSE);
      if (result != TPM_RC_SUCCESS) {
        return result;
      }
      ++(*num_request_handles);
      return TPM_RC_SUCCESS;
#endif
#ifdef TPM_CC_ZGen_2Phase
    case TPM_CC_ZGen_2Phase:
      result = TPMI_DH_OBJECT_Unmarshal(
          (TPMI_DH_OBJECT*)&request_handles[*num_request_handles],
          request_handle_buffer_start, request_buffer_remaining_size, FALSE);
      if (result != TPM_RC_SUCCESS) {
        return result;
      }
      ++(*num_request_handles);
      return TPM_RC_SUCCESS;
#endif
    default:
      return TPM_RC_COMMAND_CODE;
  }
}