// Copyright 2015 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// THIS CODE IS GENERATED - DO NOT MODIFY!

#include "GetCommandCodeString_fp.h"

const char* GetCommandCodeString(TPM_CC command_code) {
  switch (command_code) {
#ifdef TPM_CC_ActivateCredential
    case TPM_CC_ActivateCredential:
      return "ActivateCredential";
#endif
#ifdef TPM_CC_Certify
    case TPM_CC_Certify:
      return "Certify";
#endif
#ifdef TPM_CC_CertifyCreation
    case TPM_CC_CertifyCreation:
      return "CertifyCreation";
#endif
#ifdef TPM_CC_ChangeEPS
    case TPM_CC_ChangeEPS:
      return "ChangeEPS";
#endif
#ifdef TPM_CC_ChangePPS
    case TPM_CC_ChangePPS:
      return "ChangePPS";
#endif
#ifdef TPM_CC_Clear
    case TPM_CC_Clear:
      return "Clear";
#endif
#ifdef TPM_CC_ClearControl
    case TPM_CC_ClearControl:
      return "ClearControl";
#endif
#ifdef TPM_CC_ClockRateAdjust
    case TPM_CC_ClockRateAdjust:
      return "ClockRateAdjust";
#endif
#ifdef TPM_CC_ClockSet
    case TPM_CC_ClockSet:
      return "ClockSet";
#endif
#ifdef TPM_CC_Commit
    case TPM_CC_Commit:
      return "Commit";
#endif
#ifdef TPM_CC_ContextLoad
    case TPM_CC_ContextLoad:
      return "ContextLoad";
#endif
#ifdef TPM_CC_ContextSave
    case TPM_CC_ContextSave:
      return "ContextSave";
#endif
#ifdef TPM_CC_Create
    case TPM_CC_Create:
      return "Create";
#endif
#ifdef TPM_CC_CreatePrimary
    case TPM_CC_CreatePrimary:
      return "CreatePrimary";
#endif
#ifdef TPM_CC_DictionaryAttackLockReset
    case TPM_CC_DictionaryAttackLockReset:
      return "DictionaryAttackLockReset";
#endif
#ifdef TPM_CC_DictionaryAttackParameters
    case TPM_CC_DictionaryAttackParameters:
      return "DictionaryAttackParameters";
#endif
#ifdef TPM_CC_Duplicate
    case TPM_CC_Duplicate:
      return "Duplicate";
#endif
#ifdef TPM_CC_ECC_Parameters
    case TPM_CC_ECC_Parameters:
      return "ECC_Parameters";
#endif
#ifdef TPM_CC_ECDH_KeyGen
    case TPM_CC_ECDH_KeyGen:
      return "ECDH_KeyGen";
#endif
#ifdef TPM_CC_ECDH_ZGen
    case TPM_CC_ECDH_ZGen:
      return "ECDH_ZGen";
#endif
#ifdef TPM_CC_EC_Ephemeral
    case TPM_CC_EC_Ephemeral:
      return "EC_Ephemeral";
#endif
#ifdef TPM_CC_EncryptDecrypt
    case TPM_CC_EncryptDecrypt:
      return "EncryptDecrypt";
#endif
#ifdef TPM_CC_EventSequenceComplete
    case TPM_CC_EventSequenceComplete:
      return "EventSequenceComplete";
#endif
#ifdef TPM_CC_EvictControl
    case TPM_CC_EvictControl:
      return "EvictControl";
#endif
#ifdef TPM_CC_FieldUpgradeData
    case TPM_CC_FieldUpgradeData:
      return "FieldUpgradeData";
#endif
#ifdef TPM_CC_FieldUpgradeStart
    case TPM_CC_FieldUpgradeStart:
      return "FieldUpgradeStart";
#endif
#ifdef TPM_CC_FirmwareRead
    case TPM_CC_FirmwareRead:
      return "FirmwareRead";
#endif
#ifdef TPM_CC_FlushContext
    case TPM_CC_FlushContext:
      return "FlushContext";
#endif
#ifdef TPM_CC_GetCapability
    case TPM_CC_GetCapability:
      return "GetCapability";
#endif
#ifdef TPM_CC_GetCommandAuditDigest
    case TPM_CC_GetCommandAuditDigest:
      return "GetCommandAuditDigest";
#endif
#ifdef TPM_CC_GetRandom
    case TPM_CC_GetRandom:
      return "GetRandom";
#endif
#ifdef TPM_CC_GetSessionAuditDigest
    case TPM_CC_GetSessionAuditDigest:
      return "GetSessionAuditDigest";
#endif
#ifdef TPM_CC_GetTestResult
    case TPM_CC_GetTestResult:
      return "GetTestResult";
#endif
#ifdef TPM_CC_GetTime
    case TPM_CC_GetTime:
      return "GetTime";
#endif
#ifdef TPM_CC_HMAC
    case TPM_CC_HMAC:
      return "HMAC";
#endif
#ifdef TPM_CC_HMAC_Start
    case TPM_CC_HMAC_Start:
      return "HMAC_Start";
#endif
#ifdef TPM_CC_Hash
    case TPM_CC_Hash:
      return "Hash";
#endif
#ifdef TPM_CC_HashSequenceStart
    case TPM_CC_HashSequenceStart:
      return "HashSequenceStart";
#endif
#ifdef TPM_CC_HierarchyChangeAuth
    case TPM_CC_HierarchyChangeAuth:
      return "HierarchyChangeAuth";
#endif
#ifdef TPM_CC_HierarchyControl
    case TPM_CC_HierarchyControl:
      return "HierarchyControl";
#endif
#ifdef TPM_CC_Import
    case TPM_CC_Import:
      return "Import";
#endif
#ifdef TPM_CC_IncrementalSelfTest
    case TPM_CC_IncrementalSelfTest:
      return "IncrementalSelfTest";
#endif
#ifdef TPM_CC_Load
    case TPM_CC_Load:
      return "Load";
#endif
#ifdef TPM_CC_LoadExternal
    case TPM_CC_LoadExternal:
      return "LoadExternal";
#endif
#ifdef TPM_CC_MakeCredential
    case TPM_CC_MakeCredential:
      return "MakeCredential";
#endif
#ifdef TPM_CC_NV_Certify
    case TPM_CC_NV_Certify:
      return "NV_Certify";
#endif
#ifdef TPM_CC_NV_ChangeAuth
    case TPM_CC_NV_ChangeAuth:
      return "NV_ChangeAuth";
#endif
#ifdef TPM_CC_NV_DefineSpace
    case TPM_CC_NV_DefineSpace:
      return "NV_DefineSpace";
#endif
#ifdef TPM_CC_NV_Extend
    case TPM_CC_NV_Extend:
      return "NV_Extend";
#endif
#ifdef TPM_CC_NV_GlobalWriteLock
    case TPM_CC_NV_GlobalWriteLock:
      return "NV_GlobalWriteLock";
#endif
#ifdef TPM_CC_NV_Increment
    case TPM_CC_NV_Increment:
      return "NV_Increment";
#endif
#ifdef TPM_CC_NV_Read
    case TPM_CC_NV_Read:
      return "NV_Read";
#endif
#ifdef TPM_CC_NV_ReadLock
    case TPM_CC_NV_ReadLock:
      return "NV_ReadLock";
#endif
#ifdef TPM_CC_NV_ReadPublic
    case TPM_CC_NV_ReadPublic:
      return "NV_ReadPublic";
#endif
#ifdef TPM_CC_NV_SetBits
    case TPM_CC_NV_SetBits:
      return "NV_SetBits";
#endif
#ifdef TPM_CC_NV_UndefineSpace
    case TPM_CC_NV_UndefineSpace:
      return "NV_UndefineSpace";
#endif
#ifdef TPM_CC_NV_UndefineSpaceSpecial
    case TPM_CC_NV_UndefineSpaceSpecial:
      return "NV_UndefineSpaceSpecial";
#endif
#ifdef TPM_CC_NV_Write
    case TPM_CC_NV_Write:
      return "NV_Write";
#endif
#ifdef TPM_CC_NV_WriteLock
    case TPM_CC_NV_WriteLock:
      return "NV_WriteLock";
#endif
#ifdef TPM_CC_ObjectChangeAuth
    case TPM_CC_ObjectChangeAuth:
      return "ObjectChangeAuth";
#endif
#ifdef TPM_CC_PCR_Allocate
    case TPM_CC_PCR_Allocate:
      return "PCR_Allocate";
#endif
#ifdef TPM_CC_PCR_Event
    case TPM_CC_PCR_Event:
      return "PCR_Event";
#endif
#ifdef TPM_CC_PCR_Extend
    case TPM_CC_PCR_Extend:
      return "PCR_Extend";
#endif
#ifdef TPM_CC_PCR_Read
    case TPM_CC_PCR_Read:
      return "PCR_Read";
#endif
#ifdef TPM_CC_PCR_Reset
    case TPM_CC_PCR_Reset:
      return "PCR_Reset";
#endif
#ifdef TPM_CC_PCR_SetAuthPolicy
    case TPM_CC_PCR_SetAuthPolicy:
      return "PCR_SetAuthPolicy";
#endif
#ifdef TPM_CC_PCR_SetAuthValue
    case TPM_CC_PCR_SetAuthValue:
      return "PCR_SetAuthValue";
#endif
#ifdef TPM_CC_PP_Commands
    case TPM_CC_PP_Commands:
      return "PP_Commands";
#endif
#ifdef TPM_CC_PolicyAuthValue
    case TPM_CC_PolicyAuthValue:
      return "PolicyAuthValue";
#endif
#ifdef TPM_CC_PolicyAuthorize
    case TPM_CC_PolicyAuthorize:
      return "PolicyAuthorize";
#endif
#ifdef TPM_CC_PolicyCommandCode
    case TPM_CC_PolicyCommandCode:
      return "PolicyCommandCode";
#endif
#ifdef TPM_CC_PolicyCounterTimer
    case TPM_CC_PolicyCounterTimer:
      return "PolicyCounterTimer";
#endif
#ifdef TPM_CC_PolicyCpHash
    case TPM_CC_PolicyCpHash:
      return "PolicyCpHash";
#endif
#ifdef TPM_CC_PolicyDuplicationSelect
    case TPM_CC_PolicyDuplicationSelect:
      return "PolicyDuplicationSelect";
#endif
#ifdef TPM_CC_PolicyGetDigest
    case TPM_CC_PolicyGetDigest:
      return "PolicyGetDigest";
#endif
#ifdef TPM_CC_PolicyLocality
    case TPM_CC_PolicyLocality:
      return "PolicyLocality";
#endif
#ifdef TPM_CC_PolicyNV
    case TPM_CC_PolicyNV:
      return "PolicyNV";
#endif
#ifdef TPM_CC_PolicyNameHash
    case TPM_CC_PolicyNameHash:
      return "PolicyNameHash";
#endif
#ifdef TPM_CC_PolicyNvWritten
    case TPM_CC_PolicyNvWritten:
      return "PolicyNvWritten";
#endif
#ifdef TPM_CC_PolicyOR
    case TPM_CC_PolicyOR:
      return "PolicyOR";
#endif
#ifdef TPM_CC_PolicyPCR
    case TPM_CC_PolicyPCR:
      return "PolicyPCR";
#endif
#ifdef TPM_CC_PolicyPassword
    case TPM_CC_PolicyPassword:
      return "PolicyPassword";
#endif
#ifdef TPM_CC_PolicyPhysicalPresence
    case TPM_CC_PolicyPhysicalPresence:
      return "PolicyPhysicalPresence";
#endif
#ifdef TPM_CC_PolicyRestart
    case TPM_CC_PolicyRestart:
      return "PolicyRestart";
#endif
#ifdef TPM_CC_PolicySecret
    case TPM_CC_PolicySecret:
      return "PolicySecret";
#endif
#ifdef TPM_CC_PolicySigned
    case TPM_CC_PolicySigned:
      return "PolicySigned";
#endif
#ifdef TPM_CC_PolicyTicket
    case TPM_CC_PolicyTicket:
      return "PolicyTicket";
#endif
#ifdef TPM_CC_Quote
    case TPM_CC_Quote:
      return "Quote";
#endif
#ifdef TPM_CC_RSA_Decrypt
    case TPM_CC_RSA_Decrypt:
      return "RSA_Decrypt";
#endif
#ifdef TPM_CC_RSA_Encrypt
    case TPM_CC_RSA_Encrypt:
      return "RSA_Encrypt";
#endif
#ifdef TPM_CC_ReadClock
    case TPM_CC_ReadClock:
      return "ReadClock";
#endif
#ifdef TPM_CC_ReadPublic
    case TPM_CC_ReadPublic:
      return "ReadPublic";
#endif
#ifdef TPM_CC_Rewrap
    case TPM_CC_Rewrap:
      return "Rewrap";
#endif
#ifdef TPM_CC_SelfTest
    case TPM_CC_SelfTest:
      return "SelfTest";
#endif
#ifdef TPM_CC_SequenceComplete
    case TPM_CC_SequenceComplete:
      return "SequenceComplete";
#endif
#ifdef TPM_CC_SequenceUpdate
    case TPM_CC_SequenceUpdate:
      return "SequenceUpdate";
#endif
#ifdef TPM_CC_SetAlgorithmSet
    case TPM_CC_SetAlgorithmSet:
      return "SetAlgorithmSet";
#endif
#ifdef TPM_CC_SetCommandCodeAuditStatus
    case TPM_CC_SetCommandCodeAuditStatus:
      return "SetCommandCodeAuditStatus";
#endif
#ifdef TPM_CC_SetPrimaryPolicy
    case TPM_CC_SetPrimaryPolicy:
      return "SetPrimaryPolicy";
#endif
#ifdef TPM_CC_Shutdown
    case TPM_CC_Shutdown:
      return "Shutdown";
#endif
#ifdef TPM_CC_Sign
    case TPM_CC_Sign:
      return "Sign";
#endif
#ifdef TPM_CC_StartAuthSession
    case TPM_CC_StartAuthSession:
      return "StartAuthSession";
#endif
#ifdef TPM_CC_Startup
    case TPM_CC_Startup:
      return "Startup";
#endif
#ifdef TPM_CC_StirRandom
    case TPM_CC_StirRandom:
      return "StirRandom";
#endif
#ifdef TPM_CC_TestParms
    case TPM_CC_TestParms:
      return "TestParms";
#endif
#ifdef TPM_CC_Unseal
    case TPM_CC_Unseal:
      return "Unseal";
#endif
#ifdef TPM_CC_VerifySignature
    case TPM_CC_VerifySignature:
      return "VerifySignature";
#endif
#ifdef TPM_CC_ZGen_2Phase
    case TPM_CC_ZGen_2Phase:
      return "ZGen_2Phase";
#endif
    default:
      return "Unknown command";
  }
}