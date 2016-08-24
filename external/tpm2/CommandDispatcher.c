// Copyright 2015 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// THIS CODE IS GENERATED - DO NOT MODIFY!
#include "ActivateCredential_fp.h"
#include "Certify_fp.h"
#include "CertifyCreation_fp.h"
#include "ChangeEPS_fp.h"
#include "ChangePPS_fp.h"
#include "Clear_fp.h"
#include "ClearControl_fp.h"
#include "ClockRateAdjust_fp.h"
#include "ClockSet_fp.h"
#include "Commit_fp.h"
#include "ContextLoad_fp.h"
#include "ContextSave_fp.h"
#include "Create_fp.h"
#include "CreatePrimary_fp.h"
#include "DictionaryAttackLockReset_fp.h"
#include "DictionaryAttackParameters_fp.h"
#include "Duplicate_fp.h"
#include "ECC_Parameters_fp.h"
#include "ECDH_KeyGen_fp.h"
#include "ECDH_ZGen_fp.h"
#include "EC_Ephemeral_fp.h"
#include "EncryptDecrypt_fp.h"
#include "EventSequenceComplete_fp.h"
#include "EvictControl_fp.h"
#include "FieldUpgradeData_fp.h"
#include "FieldUpgradeStart_fp.h"
#include "FirmwareRead_fp.h"
#include "FlushContext_fp.h"
#include "GetCapability_fp.h"
#include "GetCommandAuditDigest_fp.h"
#include "GetRandom_fp.h"
#include "GetSessionAuditDigest_fp.h"
#include "GetTestResult_fp.h"
#include "GetTime_fp.h"
#include "HMAC_fp.h"
#include "HMAC_Start_fp.h"
#include "Hash_fp.h"
#include "HashSequenceStart_fp.h"
#include "HierarchyChangeAuth_fp.h"
#include "HierarchyControl_fp.h"
#include "Import_fp.h"
#include "IncrementalSelfTest_fp.h"
#include "Load_fp.h"
#include "LoadExternal_fp.h"
#include "MakeCredential_fp.h"
#include "NV_Certify_fp.h"
#include "NV_ChangeAuth_fp.h"
#include "NV_DefineSpace_fp.h"
#include "NV_Extend_fp.h"
#include "NV_GlobalWriteLock_fp.h"
#include "NV_Increment_fp.h"
#include "NV_Read_fp.h"
#include "NV_ReadLock_fp.h"
#include "NV_ReadPublic_fp.h"
#include "NV_SetBits_fp.h"
#include "NV_UndefineSpace_fp.h"
#include "NV_UndefineSpaceSpecial_fp.h"
#include "NV_Write_fp.h"
#include "NV_WriteLock_fp.h"
#include "ObjectChangeAuth_fp.h"
#include "PCR_Allocate_fp.h"
#include "PCR_Event_fp.h"
#include "PCR_Extend_fp.h"
#include "PCR_Read_fp.h"
#include "PCR_Reset_fp.h"
#include "PCR_SetAuthPolicy_fp.h"
#include "PCR_SetAuthValue_fp.h"
#include "PP_Commands_fp.h"
#include "PolicyAuthValue_fp.h"
#include "PolicyAuthorize_fp.h"
#include "PolicyCommandCode_fp.h"
#include "PolicyCounterTimer_fp.h"
#include "PolicyCpHash_fp.h"
#include "PolicyDuplicationSelect_fp.h"
#include "PolicyGetDigest_fp.h"
#include "PolicyLocality_fp.h"
#include "PolicyNV_fp.h"
#include "PolicyNameHash_fp.h"
#include "PolicyNvWritten_fp.h"
#include "PolicyOR_fp.h"
#include "PolicyPCR_fp.h"
#include "PolicyPassword_fp.h"
#include "PolicyPhysicalPresence_fp.h"
#include "PolicyRestart_fp.h"
#include "PolicySecret_fp.h"
#include "PolicySigned_fp.h"
#include "PolicyTicket_fp.h"
#include "Quote_fp.h"
#include "RSA_Decrypt_fp.h"
#include "RSA_Encrypt_fp.h"
#include "ReadClock_fp.h"
#include "ReadPublic_fp.h"
#include "Rewrap_fp.h"
#include "SelfTest_fp.h"
#include "SequenceComplete_fp.h"
#include "SequenceUpdate_fp.h"
#include "SetAlgorithmSet_fp.h"
#include "SetCommandCodeAuditStatus_fp.h"
#include "SetPrimaryPolicy_fp.h"
#include "Shutdown_fp.h"
#include "Sign_fp.h"
#include "StartAuthSession_fp.h"
#include "Startup_fp.h"
#include "StirRandom_fp.h"
#include "TestParms_fp.h"
#include "Unseal_fp.h"
#include "VerifySignature_fp.h"
#include "ZGen_2Phase_fp.h"

#include "Implementation.h"
#include "CommandDispatcher_fp.h"

TPM_RC CommandDispatcher(TPMI_ST_COMMAND_TAG tag,
                         TPM_CC command_code,
                         INT32* request_parameter_buffer_size,
                         BYTE* request_parameter_buffer_start,
                         TPM_HANDLE request_handles[],
                         UINT32* response_handle_buffer_size,
                         UINT32* response_parameter_buffer_size) {
  BYTE* request_parameter_buffer = request_parameter_buffer_start;
  switch (command_code) {
#ifdef TPM_CC_ActivateCredential
    case TPM_CC_ActivateCredential:
      return Exec_ActivateCredential(
          tag, &request_parameter_buffer, request_parameter_buffer_size,
          request_handles, response_handle_buffer_size,
          response_parameter_buffer_size);
#endif
#ifdef TPM_CC_Certify
    case TPM_CC_Certify:
      return Exec_Certify(tag, &request_parameter_buffer,
                          request_parameter_buffer_size, request_handles,
                          response_handle_buffer_size,
                          response_parameter_buffer_size);
#endif
#ifdef TPM_CC_CertifyCreation
    case TPM_CC_CertifyCreation:
      return Exec_CertifyCreation(tag, &request_parameter_buffer,
                                  request_parameter_buffer_size,
                                  request_handles, response_handle_buffer_size,
                                  response_parameter_buffer_size);
#endif
#ifdef TPM_CC_ChangeEPS
    case TPM_CC_ChangeEPS:
      return Exec_ChangeEPS(tag, &request_parameter_buffer,
                            request_parameter_buffer_size, request_handles,
                            response_handle_buffer_size,
                            response_parameter_buffer_size);
#endif
#ifdef TPM_CC_ChangePPS
    case TPM_CC_ChangePPS:
      return Exec_ChangePPS(tag, &request_parameter_buffer,
                            request_parameter_buffer_size, request_handles,
                            response_handle_buffer_size,
                            response_parameter_buffer_size);
#endif
#ifdef TPM_CC_Clear
    case TPM_CC_Clear:
      return Exec_Clear(tag, &request_parameter_buffer,
                        request_parameter_buffer_size, request_handles,
                        response_handle_buffer_size,
                        response_parameter_buffer_size);
#endif
#ifdef TPM_CC_ClearControl
    case TPM_CC_ClearControl:
      return Exec_ClearControl(tag, &request_parameter_buffer,
                               request_parameter_buffer_size, request_handles,
                               response_handle_buffer_size,
                               response_parameter_buffer_size);
#endif
#ifdef TPM_CC_ClockRateAdjust
    case TPM_CC_ClockRateAdjust:
      return Exec_ClockRateAdjust(tag, &request_parameter_buffer,
                                  request_parameter_buffer_size,
                                  request_handles, response_handle_buffer_size,
                                  response_parameter_buffer_size);
#endif
#ifdef TPM_CC_ClockSet
    case TPM_CC_ClockSet:
      return Exec_ClockSet(tag, &request_parameter_buffer,
                           request_parameter_buffer_size, request_handles,
                           response_handle_buffer_size,
                           response_parameter_buffer_size);
#endif
#ifdef TPM_CC_Commit
    case TPM_CC_Commit:
      return Exec_Commit(tag, &request_parameter_buffer,
                         request_parameter_buffer_size, request_handles,
                         response_handle_buffer_size,
                         response_parameter_buffer_size);
#endif
#ifdef TPM_CC_ContextLoad
    case TPM_CC_ContextLoad:
      return Exec_ContextLoad(tag, &request_parameter_buffer,
                              request_parameter_buffer_size, request_handles,
                              response_handle_buffer_size,
                              response_parameter_buffer_size);
#endif
#ifdef TPM_CC_ContextSave
    case TPM_CC_ContextSave:
      return Exec_ContextSave(tag, &request_parameter_buffer,
                              request_parameter_buffer_size, request_handles,
                              response_handle_buffer_size,
                              response_parameter_buffer_size);
#endif
#ifdef TPM_CC_Create
    case TPM_CC_Create:
      return Exec_Create(tag, &request_parameter_buffer,
                         request_parameter_buffer_size, request_handles,
                         response_handle_buffer_size,
                         response_parameter_buffer_size);
#endif
#ifdef TPM_CC_CreatePrimary
    case TPM_CC_CreatePrimary:
      return Exec_CreatePrimary(tag, &request_parameter_buffer,
                                request_parameter_buffer_size, request_handles,
                                response_handle_buffer_size,
                                response_parameter_buffer_size);
#endif
#ifdef TPM_CC_DictionaryAttackLockReset
    case TPM_CC_DictionaryAttackLockReset:
      return Exec_DictionaryAttackLockReset(
          tag, &request_parameter_buffer, request_parameter_buffer_size,
          request_handles, response_handle_buffer_size,
          response_parameter_buffer_size);
#endif
#ifdef TPM_CC_DictionaryAttackParameters
    case TPM_CC_DictionaryAttackParameters:
      return Exec_DictionaryAttackParameters(
          tag, &request_parameter_buffer, request_parameter_buffer_size,
          request_handles, response_handle_buffer_size,
          response_parameter_buffer_size);
#endif
#ifdef TPM_CC_Duplicate
    case TPM_CC_Duplicate:
      return Exec_Duplicate(tag, &request_parameter_buffer,
                            request_parameter_buffer_size, request_handles,
                            response_handle_buffer_size,
                            response_parameter_buffer_size);
#endif
#ifdef TPM_CC_ECC_Parameters
    case TPM_CC_ECC_Parameters:
      return Exec_ECC_Parameters(tag, &request_parameter_buffer,
                                 request_parameter_buffer_size, request_handles,
                                 response_handle_buffer_size,
                                 response_parameter_buffer_size);
#endif
#ifdef TPM_CC_ECDH_KeyGen
    case TPM_CC_ECDH_KeyGen:
      return Exec_ECDH_KeyGen(tag, &request_parameter_buffer,
                              request_parameter_buffer_size, request_handles,
                              response_handle_buffer_size,
                              response_parameter_buffer_size);
#endif
#ifdef TPM_CC_ECDH_ZGen
    case TPM_CC_ECDH_ZGen:
      return Exec_ECDH_ZGen(tag, &request_parameter_buffer,
                            request_parameter_buffer_size, request_handles,
                            response_handle_buffer_size,
                            response_parameter_buffer_size);
#endif
#ifdef TPM_CC_EC_Ephemeral
    case TPM_CC_EC_Ephemeral:
      return Exec_EC_Ephemeral(tag, &request_parameter_buffer,
                               request_parameter_buffer_size, request_handles,
                               response_handle_buffer_size,
                               response_parameter_buffer_size);
#endif
#ifdef TPM_CC_EncryptDecrypt
    case TPM_CC_EncryptDecrypt:
      return Exec_EncryptDecrypt(tag, &request_parameter_buffer,
                                 request_parameter_buffer_size, request_handles,
                                 response_handle_buffer_size,
                                 response_parameter_buffer_size);
#endif
#ifdef TPM_CC_EventSequenceComplete
    case TPM_CC_EventSequenceComplete:
      return Exec_EventSequenceComplete(
          tag, &request_parameter_buffer, request_parameter_buffer_size,
          request_handles, response_handle_buffer_size,
          response_parameter_buffer_size);
#endif
#ifdef TPM_CC_EvictControl
    case TPM_CC_EvictControl:
      return Exec_EvictControl(tag, &request_parameter_buffer,
                               request_parameter_buffer_size, request_handles,
                               response_handle_buffer_size,
                               response_parameter_buffer_size);
#endif
#ifdef TPM_CC_FieldUpgradeData
    case TPM_CC_FieldUpgradeData:
      return Exec_FieldUpgradeData(tag, &request_parameter_buffer,
                                   request_parameter_buffer_size,
                                   request_handles, response_handle_buffer_size,
                                   response_parameter_buffer_size);
#endif
#ifdef TPM_CC_FieldUpgradeStart
    case TPM_CC_FieldUpgradeStart:
      return Exec_FieldUpgradeStart(
          tag, &request_parameter_buffer, request_parameter_buffer_size,
          request_handles, response_handle_buffer_size,
          response_parameter_buffer_size);
#endif
#ifdef TPM_CC_FirmwareRead
    case TPM_CC_FirmwareRead:
      return Exec_FirmwareRead(tag, &request_parameter_buffer,
                               request_parameter_buffer_size, request_handles,
                               response_handle_buffer_size,
                               response_parameter_buffer_size);
#endif
#ifdef TPM_CC_FlushContext
    case TPM_CC_FlushContext:
      return Exec_FlushContext(tag, &request_parameter_buffer,
                               request_parameter_buffer_size, request_handles,
                               response_handle_buffer_size,
                               response_parameter_buffer_size);
#endif
#ifdef TPM_CC_GetCapability
    case TPM_CC_GetCapability:
      return Exec_GetCapability(tag, &request_parameter_buffer,
                                request_parameter_buffer_size, request_handles,
                                response_handle_buffer_size,
                                response_parameter_buffer_size);
#endif
#ifdef TPM_CC_GetCommandAuditDigest
    case TPM_CC_GetCommandAuditDigest:
      return Exec_GetCommandAuditDigest(
          tag, &request_parameter_buffer, request_parameter_buffer_size,
          request_handles, response_handle_buffer_size,
          response_parameter_buffer_size);
#endif
#ifdef TPM_CC_GetRandom
    case TPM_CC_GetRandom:
      return Exec_GetRandom(tag, &request_parameter_buffer,
                            request_parameter_buffer_size, request_handles,
                            response_handle_buffer_size,
                            response_parameter_buffer_size);
#endif
#ifdef TPM_CC_GetSessionAuditDigest
    case TPM_CC_GetSessionAuditDigest:
      return Exec_GetSessionAuditDigest(
          tag, &request_parameter_buffer, request_parameter_buffer_size,
          request_handles, response_handle_buffer_size,
          response_parameter_buffer_size);
#endif
#ifdef TPM_CC_GetTestResult
    case TPM_CC_GetTestResult:
      return Exec_GetTestResult(tag, &request_parameter_buffer,
                                request_parameter_buffer_size, request_handles,
                                response_handle_buffer_size,
                                response_parameter_buffer_size);
#endif
#ifdef TPM_CC_GetTime
    case TPM_CC_GetTime:
      return Exec_GetTime(tag, &request_parameter_buffer,
                          request_parameter_buffer_size, request_handles,
                          response_handle_buffer_size,
                          response_parameter_buffer_size);
#endif
#ifdef TPM_CC_HMAC
    case TPM_CC_HMAC:
      return Exec_HMAC(tag, &request_parameter_buffer,
                       request_parameter_buffer_size, request_handles,
                       response_handle_buffer_size,
                       response_parameter_buffer_size);
#endif
#ifdef TPM_CC_HMAC_Start
    case TPM_CC_HMAC_Start:
      return Exec_HMAC_Start(tag, &request_parameter_buffer,
                             request_parameter_buffer_size, request_handles,
                             response_handle_buffer_size,
                             response_parameter_buffer_size);
#endif
#ifdef TPM_CC_Hash
    case TPM_CC_Hash:
      return Exec_Hash(tag, &request_parameter_buffer,
                       request_parameter_buffer_size, request_handles,
                       response_handle_buffer_size,
                       response_parameter_buffer_size);
#endif
#ifdef TPM_CC_HashSequenceStart
    case TPM_CC_HashSequenceStart:
      return Exec_HashSequenceStart(
          tag, &request_parameter_buffer, request_parameter_buffer_size,
          request_handles, response_handle_buffer_size,
          response_parameter_buffer_size);
#endif
#ifdef TPM_CC_HierarchyChangeAuth
    case TPM_CC_HierarchyChangeAuth:
      return Exec_HierarchyChangeAuth(
          tag, &request_parameter_buffer, request_parameter_buffer_size,
          request_handles, response_handle_buffer_size,
          response_parameter_buffer_size);
#endif
#ifdef TPM_CC_HierarchyControl
    case TPM_CC_HierarchyControl:
      return Exec_HierarchyControl(tag, &request_parameter_buffer,
                                   request_parameter_buffer_size,
                                   request_handles, response_handle_buffer_size,
                                   response_parameter_buffer_size);
#endif
#ifdef TPM_CC_Import
    case TPM_CC_Import:
      return Exec_Import(tag, &request_parameter_buffer,
                         request_parameter_buffer_size, request_handles,
                         response_handle_buffer_size,
                         response_parameter_buffer_size);
#endif
#ifdef TPM_CC_IncrementalSelfTest
    case TPM_CC_IncrementalSelfTest:
      return Exec_IncrementalSelfTest(
          tag, &request_parameter_buffer, request_parameter_buffer_size,
          request_handles, response_handle_buffer_size,
          response_parameter_buffer_size);
#endif
#ifdef TPM_CC_Load
    case TPM_CC_Load:
      return Exec_Load(tag, &request_parameter_buffer,
                       request_parameter_buffer_size, request_handles,
                       response_handle_buffer_size,
                       response_parameter_buffer_size);
#endif
#ifdef TPM_CC_LoadExternal
    case TPM_CC_LoadExternal:
      return Exec_LoadExternal(tag, &request_parameter_buffer,
                               request_parameter_buffer_size, request_handles,
                               response_handle_buffer_size,
                               response_parameter_buffer_size);
#endif
#ifdef TPM_CC_MakeCredential
    case TPM_CC_MakeCredential:
      return Exec_MakeCredential(tag, &request_parameter_buffer,
                                 request_parameter_buffer_size, request_handles,
                                 response_handle_buffer_size,
                                 response_parameter_buffer_size);
#endif
#ifdef TPM_CC_NV_Certify
    case TPM_CC_NV_Certify:
      return Exec_NV_Certify(tag, &request_parameter_buffer,
                             request_parameter_buffer_size, request_handles,
                             response_handle_buffer_size,
                             response_parameter_buffer_size);
#endif
#ifdef TPM_CC_NV_ChangeAuth
    case TPM_CC_NV_ChangeAuth:
      return Exec_NV_ChangeAuth(tag, &request_parameter_buffer,
                                request_parameter_buffer_size, request_handles,
                                response_handle_buffer_size,
                                response_parameter_buffer_size);
#endif
#ifdef TPM_CC_NV_DefineSpace
    case TPM_CC_NV_DefineSpace:
      return Exec_NV_DefineSpace(tag, &request_parameter_buffer,
                                 request_parameter_buffer_size, request_handles,
                                 response_handle_buffer_size,
                                 response_parameter_buffer_size);
#endif
#ifdef TPM_CC_NV_Extend
    case TPM_CC_NV_Extend:
      return Exec_NV_Extend(tag, &request_parameter_buffer,
                            request_parameter_buffer_size, request_handles,
                            response_handle_buffer_size,
                            response_parameter_buffer_size);
#endif
#ifdef TPM_CC_NV_GlobalWriteLock
    case TPM_CC_NV_GlobalWriteLock:
      return Exec_NV_GlobalWriteLock(
          tag, &request_parameter_buffer, request_parameter_buffer_size,
          request_handles, response_handle_buffer_size,
          response_parameter_buffer_size);
#endif
#ifdef TPM_CC_NV_Increment
    case TPM_CC_NV_Increment:
      return Exec_NV_Increment(tag, &request_parameter_buffer,
                               request_parameter_buffer_size, request_handles,
                               response_handle_buffer_size,
                               response_parameter_buffer_size);
#endif
#ifdef TPM_CC_NV_Read
    case TPM_CC_NV_Read:
      return Exec_NV_Read(tag, &request_parameter_buffer,
                          request_parameter_buffer_size, request_handles,
                          response_handle_buffer_size,
                          response_parameter_buffer_size);
#endif
#ifdef TPM_CC_NV_ReadLock
    case TPM_CC_NV_ReadLock:
      return Exec_NV_ReadLock(tag, &request_parameter_buffer,
                              request_parameter_buffer_size, request_handles,
                              response_handle_buffer_size,
                              response_parameter_buffer_size);
#endif
#ifdef TPM_CC_NV_ReadPublic
    case TPM_CC_NV_ReadPublic:
      return Exec_NV_ReadPublic(tag, &request_parameter_buffer,
                                request_parameter_buffer_size, request_handles,
                                response_handle_buffer_size,
                                response_parameter_buffer_size);
#endif
#ifdef TPM_CC_NV_SetBits
    case TPM_CC_NV_SetBits:
      return Exec_NV_SetBits(tag, &request_parameter_buffer,
                             request_parameter_buffer_size, request_handles,
                             response_handle_buffer_size,
                             response_parameter_buffer_size);
#endif
#ifdef TPM_CC_NV_UndefineSpace
    case TPM_CC_NV_UndefineSpace:
      return Exec_NV_UndefineSpace(tag, &request_parameter_buffer,
                                   request_parameter_buffer_size,
                                   request_handles, response_handle_buffer_size,
                                   response_parameter_buffer_size);
#endif
#ifdef TPM_CC_NV_UndefineSpaceSpecial
    case TPM_CC_NV_UndefineSpaceSpecial:
      return Exec_NV_UndefineSpaceSpecial(
          tag, &request_parameter_buffer, request_parameter_buffer_size,
          request_handles, response_handle_buffer_size,
          response_parameter_buffer_size);
#endif
#ifdef TPM_CC_NV_Write
    case TPM_CC_NV_Write:
      return Exec_NV_Write(tag, &request_parameter_buffer,
                           request_parameter_buffer_size, request_handles,
                           response_handle_buffer_size,
                           response_parameter_buffer_size);
#endif
#ifdef TPM_CC_NV_WriteLock
    case TPM_CC_NV_WriteLock:
      return Exec_NV_WriteLock(tag, &request_parameter_buffer,
                               request_parameter_buffer_size, request_handles,
                               response_handle_buffer_size,
                               response_parameter_buffer_size);
#endif
#ifdef TPM_CC_ObjectChangeAuth
    case TPM_CC_ObjectChangeAuth:
      return Exec_ObjectChangeAuth(tag, &request_parameter_buffer,
                                   request_parameter_buffer_size,
                                   request_handles, response_handle_buffer_size,
                                   response_parameter_buffer_size);
#endif
#ifdef TPM_CC_PCR_Allocate
    case TPM_CC_PCR_Allocate:
      return Exec_PCR_Allocate(tag, &request_parameter_buffer,
                               request_parameter_buffer_size, request_handles,
                               response_handle_buffer_size,
                               response_parameter_buffer_size);
#endif
#ifdef TPM_CC_PCR_Event
    case TPM_CC_PCR_Event:
      return Exec_PCR_Event(tag, &request_parameter_buffer,
                            request_parameter_buffer_size, request_handles,
                            response_handle_buffer_size,
                            response_parameter_buffer_size);
#endif
#ifdef TPM_CC_PCR_Extend
    case TPM_CC_PCR_Extend:
      return Exec_PCR_Extend(tag, &request_parameter_buffer,
                             request_parameter_buffer_size, request_handles,
                             response_handle_buffer_size,
                             response_parameter_buffer_size);
#endif
#ifdef TPM_CC_PCR_Read
    case TPM_CC_PCR_Read:
      return Exec_PCR_Read(tag, &request_parameter_buffer,
                           request_parameter_buffer_size, request_handles,
                           response_handle_buffer_size,
                           response_parameter_buffer_size);
#endif
#ifdef TPM_CC_PCR_Reset
    case TPM_CC_PCR_Reset:
      return Exec_PCR_Reset(tag, &request_parameter_buffer,
                            request_parameter_buffer_size, request_handles,
                            response_handle_buffer_size,
                            response_parameter_buffer_size);
#endif
#ifdef TPM_CC_PCR_SetAuthPolicy
    case TPM_CC_PCR_SetAuthPolicy:
      return Exec_PCR_SetAuthPolicy(
          tag, &request_parameter_buffer, request_parameter_buffer_size,
          request_handles, response_handle_buffer_size,
          response_parameter_buffer_size);
#endif
#ifdef TPM_CC_PCR_SetAuthValue
    case TPM_CC_PCR_SetAuthValue:
      return Exec_PCR_SetAuthValue(tag, &request_parameter_buffer,
                                   request_parameter_buffer_size,
                                   request_handles, response_handle_buffer_size,
                                   response_parameter_buffer_size);
#endif
#ifdef TPM_CC_PP_Commands
    case TPM_CC_PP_Commands:
      return Exec_PP_Commands(tag, &request_parameter_buffer,
                              request_parameter_buffer_size, request_handles,
                              response_handle_buffer_size,
                              response_parameter_buffer_size);
#endif
#ifdef TPM_CC_PolicyAuthValue
    case TPM_CC_PolicyAuthValue:
      return Exec_PolicyAuthValue(tag, &request_parameter_buffer,
                                  request_parameter_buffer_size,
                                  request_handles, response_handle_buffer_size,
                                  response_parameter_buffer_size);
#endif
#ifdef TPM_CC_PolicyAuthorize
    case TPM_CC_PolicyAuthorize:
      return Exec_PolicyAuthorize(tag, &request_parameter_buffer,
                                  request_parameter_buffer_size,
                                  request_handles, response_handle_buffer_size,
                                  response_parameter_buffer_size);
#endif
#ifdef TPM_CC_PolicyCommandCode
    case TPM_CC_PolicyCommandCode:
      return Exec_PolicyCommandCode(
          tag, &request_parameter_buffer, request_parameter_buffer_size,
          request_handles, response_handle_buffer_size,
          response_parameter_buffer_size);
#endif
#ifdef TPM_CC_PolicyCounterTimer
    case TPM_CC_PolicyCounterTimer:
      return Exec_PolicyCounterTimer(
          tag, &request_parameter_buffer, request_parameter_buffer_size,
          request_handles, response_handle_buffer_size,
          response_parameter_buffer_size);
#endif
#ifdef TPM_CC_PolicyCpHash
    case TPM_CC_PolicyCpHash:
      return Exec_PolicyCpHash(tag, &request_parameter_buffer,
                               request_parameter_buffer_size, request_handles,
                               response_handle_buffer_size,
                               response_parameter_buffer_size);
#endif
#ifdef TPM_CC_PolicyDuplicationSelect
    case TPM_CC_PolicyDuplicationSelect:
      return Exec_PolicyDuplicationSelect(
          tag, &request_parameter_buffer, request_parameter_buffer_size,
          request_handles, response_handle_buffer_size,
          response_parameter_buffer_size);
#endif
#ifdef TPM_CC_PolicyGetDigest
    case TPM_CC_PolicyGetDigest:
      return Exec_PolicyGetDigest(tag, &request_parameter_buffer,
                                  request_parameter_buffer_size,
                                  request_handles, response_handle_buffer_size,
                                  response_parameter_buffer_size);
#endif
#ifdef TPM_CC_PolicyLocality
    case TPM_CC_PolicyLocality:
      return Exec_PolicyLocality(tag, &request_parameter_buffer,
                                 request_parameter_buffer_size, request_handles,
                                 response_handle_buffer_size,
                                 response_parameter_buffer_size);
#endif
#ifdef TPM_CC_PolicyNV
    case TPM_CC_PolicyNV:
      return Exec_PolicyNV(tag, &request_parameter_buffer,
                           request_parameter_buffer_size, request_handles,
                           response_handle_buffer_size,
                           response_parameter_buffer_size);
#endif
#ifdef TPM_CC_PolicyNameHash
    case TPM_CC_PolicyNameHash:
      return Exec_PolicyNameHash(tag, &request_parameter_buffer,
                                 request_parameter_buffer_size, request_handles,
                                 response_handle_buffer_size,
                                 response_parameter_buffer_size);
#endif
#ifdef TPM_CC_PolicyNvWritten
    case TPM_CC_PolicyNvWritten:
      return Exec_PolicyNvWritten(tag, &request_parameter_buffer,
                                  request_parameter_buffer_size,
                                  request_handles, response_handle_buffer_size,
                                  response_parameter_buffer_size);
#endif
#ifdef TPM_CC_PolicyOR
    case TPM_CC_PolicyOR:
      return Exec_PolicyOR(tag, &request_parameter_buffer,
                           request_parameter_buffer_size, request_handles,
                           response_handle_buffer_size,
                           response_parameter_buffer_size);
#endif
#ifdef TPM_CC_PolicyPCR
    case TPM_CC_PolicyPCR:
      return Exec_PolicyPCR(tag, &request_parameter_buffer,
                            request_parameter_buffer_size, request_handles,
                            response_handle_buffer_size,
                            response_parameter_buffer_size);
#endif
#ifdef TPM_CC_PolicyPassword
    case TPM_CC_PolicyPassword:
      return Exec_PolicyPassword(tag, &request_parameter_buffer,
                                 request_parameter_buffer_size, request_handles,
                                 response_handle_buffer_size,
                                 response_parameter_buffer_size);
#endif
#ifdef TPM_CC_PolicyPhysicalPresence
    case TPM_CC_PolicyPhysicalPresence:
      return Exec_PolicyPhysicalPresence(
          tag, &request_parameter_buffer, request_parameter_buffer_size,
          request_handles, response_handle_buffer_size,
          response_parameter_buffer_size);
#endif
#ifdef TPM_CC_PolicyRestart
    case TPM_CC_PolicyRestart:
      return Exec_PolicyRestart(tag, &request_parameter_buffer,
                                request_parameter_buffer_size, request_handles,
                                response_handle_buffer_size,
                                response_parameter_buffer_size);
#endif
#ifdef TPM_CC_PolicySecret
    case TPM_CC_PolicySecret:
      return Exec_PolicySecret(tag, &request_parameter_buffer,
                               request_parameter_buffer_size, request_handles,
                               response_handle_buffer_size,
                               response_parameter_buffer_size);
#endif
#ifdef TPM_CC_PolicySigned
    case TPM_CC_PolicySigned:
      return Exec_PolicySigned(tag, &request_parameter_buffer,
                               request_parameter_buffer_size, request_handles,
                               response_handle_buffer_size,
                               response_parameter_buffer_size);
#endif
#ifdef TPM_CC_PolicyTicket
    case TPM_CC_PolicyTicket:
      return Exec_PolicyTicket(tag, &request_parameter_buffer,
                               request_parameter_buffer_size, request_handles,
                               response_handle_buffer_size,
                               response_parameter_buffer_size);
#endif
#ifdef TPM_CC_Quote
    case TPM_CC_Quote:
      return Exec_Quote(tag, &request_parameter_buffer,
                        request_parameter_buffer_size, request_handles,
                        response_handle_buffer_size,
                        response_parameter_buffer_size);
#endif
#ifdef TPM_CC_RSA_Decrypt
    case TPM_CC_RSA_Decrypt:
      return Exec_RSA_Decrypt(tag, &request_parameter_buffer,
                              request_parameter_buffer_size, request_handles,
                              response_handle_buffer_size,
                              response_parameter_buffer_size);
#endif
#ifdef TPM_CC_RSA_Encrypt
    case TPM_CC_RSA_Encrypt:
      return Exec_RSA_Encrypt(tag, &request_parameter_buffer,
                              request_parameter_buffer_size, request_handles,
                              response_handle_buffer_size,
                              response_parameter_buffer_size);
#endif
#ifdef TPM_CC_ReadClock
    case TPM_CC_ReadClock:
      return Exec_ReadClock(tag, &request_parameter_buffer,
                            request_parameter_buffer_size, request_handles,
                            response_handle_buffer_size,
                            response_parameter_buffer_size);
#endif
#ifdef TPM_CC_ReadPublic
    case TPM_CC_ReadPublic:
      return Exec_ReadPublic(tag, &request_parameter_buffer,
                             request_parameter_buffer_size, request_handles,
                             response_handle_buffer_size,
                             response_parameter_buffer_size);
#endif
#ifdef TPM_CC_Rewrap
    case TPM_CC_Rewrap:
      return Exec_Rewrap(tag, &request_parameter_buffer,
                         request_parameter_buffer_size, request_handles,
                         response_handle_buffer_size,
                         response_parameter_buffer_size);
#endif
#ifdef TPM_CC_SelfTest
    case TPM_CC_SelfTest:
      return Exec_SelfTest(tag, &request_parameter_buffer,
                           request_parameter_buffer_size, request_handles,
                           response_handle_buffer_size,
                           response_parameter_buffer_size);
#endif
#ifdef TPM_CC_SequenceComplete
    case TPM_CC_SequenceComplete:
      return Exec_SequenceComplete(tag, &request_parameter_buffer,
                                   request_parameter_buffer_size,
                                   request_handles, response_handle_buffer_size,
                                   response_parameter_buffer_size);
#endif
#ifdef TPM_CC_SequenceUpdate
    case TPM_CC_SequenceUpdate:
      return Exec_SequenceUpdate(tag, &request_parameter_buffer,
                                 request_parameter_buffer_size, request_handles,
                                 response_handle_buffer_size,
                                 response_parameter_buffer_size);
#endif
#ifdef TPM_CC_SetAlgorithmSet
    case TPM_CC_SetAlgorithmSet:
      return Exec_SetAlgorithmSet(tag, &request_parameter_buffer,
                                  request_parameter_buffer_size,
                                  request_handles, response_handle_buffer_size,
                                  response_parameter_buffer_size);
#endif
#ifdef TPM_CC_SetCommandCodeAuditStatus
    case TPM_CC_SetCommandCodeAuditStatus:
      return Exec_SetCommandCodeAuditStatus(
          tag, &request_parameter_buffer, request_parameter_buffer_size,
          request_handles, response_handle_buffer_size,
          response_parameter_buffer_size);
#endif
#ifdef TPM_CC_SetPrimaryPolicy
    case TPM_CC_SetPrimaryPolicy:
      return Exec_SetPrimaryPolicy(tag, &request_parameter_buffer,
                                   request_parameter_buffer_size,
                                   request_handles, response_handle_buffer_size,
                                   response_parameter_buffer_size);
#endif
#ifdef TPM_CC_Shutdown
    case TPM_CC_Shutdown:
      return Exec_Shutdown(tag, &request_parameter_buffer,
                           request_parameter_buffer_size, request_handles,
                           response_handle_buffer_size,
                           response_parameter_buffer_size);
#endif
#ifdef TPM_CC_Sign
    case TPM_CC_Sign:
      return Exec_Sign(tag, &request_parameter_buffer,
                       request_parameter_buffer_size, request_handles,
                       response_handle_buffer_size,
                       response_parameter_buffer_size);
#endif
#ifdef TPM_CC_StartAuthSession
    case TPM_CC_StartAuthSession:
      return Exec_StartAuthSession(tag, &request_parameter_buffer,
                                   request_parameter_buffer_size,
                                   request_handles, response_handle_buffer_size,
                                   response_parameter_buffer_size);
#endif
#ifdef TPM_CC_Startup
    case TPM_CC_Startup:
      return Exec_Startup(tag, &request_parameter_buffer,
                          request_parameter_buffer_size, request_handles,
                          response_handle_buffer_size,
                          response_parameter_buffer_size);
#endif
#ifdef TPM_CC_StirRandom
    case TPM_CC_StirRandom:
      return Exec_StirRandom(tag, &request_parameter_buffer,
                             request_parameter_buffer_size, request_handles,
                             response_handle_buffer_size,
                             response_parameter_buffer_size);
#endif
#ifdef TPM_CC_TestParms
    case TPM_CC_TestParms:
      return Exec_TestParms(tag, &request_parameter_buffer,
                            request_parameter_buffer_size, request_handles,
                            response_handle_buffer_size,
                            response_parameter_buffer_size);
#endif
#ifdef TPM_CC_Unseal
    case TPM_CC_Unseal:
      return Exec_Unseal(tag, &request_parameter_buffer,
                         request_parameter_buffer_size, request_handles,
                         response_handle_buffer_size,
                         response_parameter_buffer_size);
#endif
#ifdef TPM_CC_VerifySignature
    case TPM_CC_VerifySignature:
      return Exec_VerifySignature(tag, &request_parameter_buffer,
                                  request_parameter_buffer_size,
                                  request_handles, response_handle_buffer_size,
                                  response_parameter_buffer_size);
#endif
#ifdef TPM_CC_ZGen_2Phase
    case TPM_CC_ZGen_2Phase:
      return Exec_ZGen_2Phase(tag, &request_parameter_buffer,
                              request_parameter_buffer_size, request_handles,
                              response_handle_buffer_size,
                              response_parameter_buffer_size);
#endif
    default:
      return TPM_RC_COMMAND_CODE;
  }
}