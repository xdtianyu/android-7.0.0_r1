
# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

obj ?= ./build
CROSS_COMPILE ?=
CC ?= $(CROSS_COMPILE)gcc
AR ?= $(CROSS_COMPILE)ar

HOST_SOURCES =
SOURCES  = ActivateCredential.c
SOURCES += AlgorithmCap.c
SOURCES += Attest_spt.c
SOURCES += Bits.c
SOURCES += Cancel.c
SOURCES += Certify.c
SOURCES += CertifyCreation.c
SOURCES += ChangeEPS.c
SOURCES += ChangePPS.c
SOURCES += Clear.c
SOURCES += ClearControl.c
SOURCES += Clock.c
SOURCES += ClockRateAdjust.c
SOURCES += ClockSet.c
SOURCES += CommandAudit.c
SOURCES += CommandCodeAttributes.c
SOURCES += CommandDispatcher.c
SOURCES += Commit.c
SOURCES += ContextLoad.c
SOURCES += ContextSave.c
SOURCES += Context_spt.c
HOST_SOURCES += CpriCryptPri.c
HOST_SOURCES += CpriECC.c
HOST_SOURCES += CpriHash.c
HOST_SOURCES += CpriMisc.c
HOST_SOURCES += CpriRNG.c
HOST_SOURCES += CpriRSA.c
HOST_SOURCES += CpriSym.c
SOURCES += Create.c
SOURCES += CreatePrimary.c
SOURCES += CryptSelfTest.c
SOURCES += CryptUtil.c
SOURCES += DA.c
SOURCES += DRTM.c
SOURCES += DictionaryAttackLockReset.c
SOURCES += DictionaryAttackParameters.c
SOURCES += Duplicate.c
SOURCES += ECC_Parameters.c
SOURCES += ECDH_KeyGen.c
SOURCES += ECDH_ZGen.c
SOURCES += EC_Ephemeral.c
SOURCES += EncryptDecrypt.c
SOURCES += Entity.c
HOST_SOURCES += Entropy.c
SOURCES += EventSequenceComplete.c
SOURCES += EvictControl.c
SOURCES += ExecCommand.c
SOURCES += FieldUpgradeData.c
SOURCES += FieldUpgradeStart.c
SOURCES += FirmwareRead.c
SOURCES += FlushContext.c
SOURCES += GetCapability.c
SOURCES += GetCommandAuditDigest.c
SOURCES += GetCommandCodeString.c
SOURCES += GetRandom.c
SOURCES += GetSessionAuditDigest.c
SOURCES += GetTestResult.c
SOURCES += GetTime.c
SOURCES += Global.c
SOURCES += HMAC.c
SOURCES += HMAC_Start.c
SOURCES += Handle.c
SOURCES += HandleProcess.c
SOURCES += Hash.c
SOURCES += HashSequenceStart.c
SOURCES += Hierarchy.c
SOURCES += HierarchyChangeAuth.c
SOURCES += HierarchyControl.c
SOURCES += Import.c
SOURCES += IncrementalSelfTest.c
SOURCES += Load.c
SOURCES += LoadExternal.c
SOURCES += Locality.c
SOURCES += LocalityPlat.c
SOURCES += MakeCredential.c
SOURCES += Marshal_ActivateCredential.c
SOURCES += Marshal_Certify.c
SOURCES += Marshal_CertifyCreation.c
SOURCES += Marshal_ChangeEPS.c
SOURCES += Marshal_ChangePPS.c
SOURCES += Marshal_Clear.c
SOURCES += Marshal_ClearControl.c
SOURCES += Marshal_ClockRateAdjust.c
SOURCES += Marshal_ClockSet.c
SOURCES += Marshal_Commit.c
SOURCES += Marshal_ContextLoad.c
SOURCES += Marshal_ContextSave.c
SOURCES += Marshal_Create.c
SOURCES += Marshal_CreatePrimary.c
SOURCES += Marshal_DictionaryAttackLockReset.c
SOURCES += Marshal_DictionaryAttackParameters.c
SOURCES += Marshal_Duplicate.c
SOURCES += Marshal_ECC_Parameters.c
SOURCES += Marshal_ECDH_KeyGen.c
SOURCES += Marshal_ECDH_ZGen.c
SOURCES += Marshal_EC_Ephemeral.c
SOURCES += Marshal_EncryptDecrypt.c
SOURCES += Marshal_EventSequenceComplete.c
SOURCES += Marshal_EvictControl.c
SOURCES += Marshal_FieldUpgradeData.c
SOURCES += Marshal_FieldUpgradeStart.c
SOURCES += Marshal_FirmwareRead.c
SOURCES += Marshal_FlushContext.c
SOURCES += Marshal_GetCapability.c
SOURCES += Marshal_GetCommandAuditDigest.c
SOURCES += Marshal_GetRandom.c
SOURCES += Marshal_GetSessionAuditDigest.c
SOURCES += Marshal_GetTestResult.c
SOURCES += Marshal_GetTime.c
SOURCES += Marshal_HMAC.c
SOURCES += Marshal_HMAC_Start.c
SOURCES += Marshal_Hash.c
SOURCES += Marshal_HashSequenceStart.c
SOURCES += Marshal_HierarchyChangeAuth.c
SOURCES += Marshal_HierarchyControl.c
SOURCES += Marshal_Import.c
SOURCES += Marshal_IncrementalSelfTest.c
SOURCES += Marshal_Load.c
SOURCES += Marshal_LoadExternal.c
SOURCES += Marshal_MakeCredential.c
SOURCES += Marshal_NV_Certify.c
SOURCES += Marshal_NV_ChangeAuth.c
SOURCES += Marshal_NV_DefineSpace.c
SOURCES += Marshal_NV_Extend.c
SOURCES += Marshal_NV_GlobalWriteLock.c
SOURCES += Marshal_NV_Increment.c
SOURCES += Marshal_NV_Read.c
SOURCES += Marshal_NV_ReadLock.c
SOURCES += Marshal_NV_ReadPublic.c
SOURCES += Marshal_NV_SetBits.c
SOURCES += Marshal_NV_UndefineSpace.c
SOURCES += Marshal_NV_UndefineSpaceSpecial.c
SOURCES += Marshal_NV_Write.c
SOURCES += Marshal_NV_WriteLock.c
SOURCES += Marshal_ObjectChangeAuth.c
SOURCES += Marshal_PCR_Allocate.c
SOURCES += Marshal_PCR_Event.c
SOURCES += Marshal_PCR_Extend.c
SOURCES += Marshal_PCR_Read.c
SOURCES += Marshal_PCR_Reset.c
SOURCES += Marshal_PCR_SetAuthPolicy.c
SOURCES += Marshal_PCR_SetAuthValue.c
SOURCES += Marshal_PP_Commands.c
SOURCES += Marshal_PolicyAuthValue.c
SOURCES += Marshal_PolicyAuthorize.c
SOURCES += Marshal_PolicyCommandCode.c
SOURCES += Marshal_PolicyCounterTimer.c
SOURCES += Marshal_PolicyCpHash.c
SOURCES += Marshal_PolicyDuplicationSelect.c
SOURCES += Marshal_PolicyGetDigest.c
SOURCES += Marshal_PolicyLocality.c
SOURCES += Marshal_PolicyNV.c
SOURCES += Marshal_PolicyNameHash.c
SOURCES += Marshal_PolicyNvWritten.c
SOURCES += Marshal_PolicyOR.c
SOURCES += Marshal_PolicyPCR.c
SOURCES += Marshal_PolicyPassword.c
SOURCES += Marshal_PolicyPhysicalPresence.c
SOURCES += Marshal_PolicyRestart.c
SOURCES += Marshal_PolicySecret.c
SOURCES += Marshal_PolicySigned.c
SOURCES += Marshal_PolicyTicket.c
SOURCES += Marshal_Quote.c
SOURCES += Marshal_RSA_Decrypt.c
SOURCES += Marshal_RSA_Encrypt.c
SOURCES += Marshal_ReadClock.c
SOURCES += Marshal_ReadPublic.c
SOURCES += Marshal_Rewrap.c
SOURCES += Marshal_SelfTest.c
SOURCES += Marshal_SequenceComplete.c
SOURCES += Marshal_SequenceUpdate.c
SOURCES += Marshal_SetAlgorithmSet.c
SOURCES += Marshal_SetCommandCodeAuditStatus.c
SOURCES += Marshal_SetPrimaryPolicy.c
SOURCES += Marshal_Shutdown.c
SOURCES += Marshal_Sign.c
SOURCES += Marshal_StartAuthSession.c
SOURCES += Marshal_Startup.c
SOURCES += Marshal_StirRandom.c
SOURCES += Marshal_TestParms.c
SOURCES += Marshal_Unseal.c
SOURCES += Marshal_VerifySignature.c
SOURCES += Marshal_ZGen_2Phase.c
SOURCES += Manufacture.c
HOST_SOURCES += MathFunctions.c
SOURCES += MemoryLib.c
SOURCES += NV.c
HOST_SOURCES += NVMem.c
SOURCES += NV_Certify.c
SOURCES += NV_ChangeAuth.c
SOURCES += NV_DefineSpace.c
SOURCES += NV_Extend.c
SOURCES += NV_GlobalWriteLock.c
SOURCES += NV_Increment.c
SOURCES += NV_Read.c
SOURCES += NV_ReadLock.c
SOURCES += NV_ReadPublic.c
SOURCES += NV_SetBits.c
SOURCES += NV_UndefineSpace.c
SOURCES += NV_UndefineSpaceSpecial.c
SOURCES += NV_Write.c
SOURCES += NV_WriteLock.c
SOURCES += NV_spt.c
SOURCES += Object.c
SOURCES += ObjectChangeAuth.c
SOURCES += Object_spt.c
SOURCES += PCR.c
SOURCES += PCR_Allocate.c
SOURCES += PCR_Event.c
SOURCES += PCR_Extend.c
SOURCES += PCR_Read.c
SOURCES += PCR_Reset.c
SOURCES += PCR_SetAuthPolicy.c
SOURCES += PCR_SetAuthValue.c
SOURCES += PP.c
SOURCES += PPPlat.c
SOURCES += PP_Commands.c
SOURCES += PlatformData.c
SOURCES += PolicyAuthValue.c
SOURCES += PolicyAuthorize.c
SOURCES += PolicyCommandCode.c
SOURCES += PolicyCounterTimer.c
SOURCES += PolicyCpHash.c
SOURCES += PolicyDuplicationSelect.c
SOURCES += PolicyGetDigest.c
SOURCES += PolicyLocality.c
SOURCES += PolicyNV.c
SOURCES += PolicyNameHash.c
SOURCES += PolicyNvWritten.c
SOURCES += PolicyOR.c
SOURCES += PolicyPCR.c
SOURCES += PolicyPassword.c
SOURCES += PolicyPhysicalPresence.c
SOURCES += PolicyRestart.c
SOURCES += PolicySecret.c
SOURCES += PolicySigned.c
SOURCES += PolicyTicket.c
SOURCES += Policy_spt.c
SOURCES += Power.c
SOURCES += PowerPlat.c
SOURCES += PropertyCap.c
SOURCES += Quote.c
HOST_SOURCES += RSAData.c
HOST_SOURCES += RSAKeySieve.c
SOURCES += RSA_Decrypt.c
SOURCES += RSA_Encrypt.c
SOURCES += ReadClock.c
SOURCES += ReadPublic.c
SOURCES += Rewrap.c
SOURCES += SelfTest.c
SOURCES += SequenceComplete.c
SOURCES += SequenceUpdate.c
SOURCES += Session.c
SOURCES += SessionProcess.c
SOURCES += SetAlgorithmSet.c
SOURCES += SetCommandCodeAuditStatus.c
SOURCES += SetPrimaryPolicy.c
SOURCES += Shutdown.c
SOURCES += Sign.c
SOURCES += StartAuthSession.c
SOURCES += Startup.c
SOURCES += StirRandom.c
#SOURCES += TPMCmdp.c
#SOURCES += TPMCmds.c
#SOURCES += TcpServer.c
SOURCES += TestParms.c
SOURCES += Ticket.c
SOURCES += Time.c
SOURCES += TpmFail.c
SOURCES += Unique.c
SOURCES += Unseal.c
SOURCES += VerifySignature.c
SOURCES += ZGen_2Phase.c
SOURCES += _TPM_Hash_Data.c
SOURCES += _TPM_Hash_End.c
SOURCES += _TPM_Hash_Start.c
SOURCES += _TPM_Init.c
SOURCES += tpm_generated.c

# Use V=1 for verbose output
ifeq ($(V),)
Q := @
else
Q :=
endif

ifeq ($(EMBEDDED_MODE),)
SOURCES += $(HOST_SOURCES)
CFLAGS += -Wall -Werror -fPIC
else
SOURCES += stubs_ecc.c
SOURCES += stubs_hash.c
SOURCES += stubs_sym.c
CFLAGS += -DEMBEDDED_MODE
ifneq ($(ROOTDIR),)
CFLAGS += -I$(ROOTDIR)
endif
endif

OBJS = $(patsubst %.c,$(obj)/%.o,$(SOURCES))
DEPS = $(patsubst %.c,$(obj)/%.d,$(SOURCES))

# This is the default target
$(obj)/libtpm2.a: $(OBJS)
	@echo "  AR      $(notdir $@)"
	$(Q)$(AR) scr $@ $^

$(obj):
	@echo "  MKDIR   $(obj)"
	$(Q)mkdir -p $(obj)

$(obj)/%.d $(obj)/%.o: %.c | $(obj)
	@echo "  CC      $(notdir $<)"
	$(Q)$(CC) $(CFLAGS) -c -MMD -MF $(basename $@).d -o $(basename $@).o $<

.PHONY: clean
clean:
	@echo "  RM      $(obj)"
	$(Q)rm -rf $(obj)

ifneq ($(MAKECMDGOALS),clean)
-include $(DEPS)
endif
