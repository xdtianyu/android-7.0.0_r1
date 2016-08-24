// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 3: Commands
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include "InternalRoutines.h"
#include "Clear_fp.h"
//
//
//     Error Returns                   Meaning
//
//     TPM_RC_DISABLED                 Clear command has been disabled
//
TPM_RC
TPM2_Clear(
   Clear_In          *in                // IN: input parameter list
   )
{
   TPM_RC                  result;

   // Input parameter is not reference in command action
   in = NULL;

   // The command needs NV update. Check if NV is available.
   // A TPM_RC_NV_UNAVAILABLE or TPM_RC_NV_RATE error may be returned at
   // this point
   result = NvIsAvailable();
   if(result != TPM_RC_SUCCESS) return result;

// Input Validation

   // If Clear command is disabled, return an error
   if(gp.disableClear)
       return TPM_RC_DISABLED;

// Internal Data Update

   // Reset storage hierarchy seed from RNG
   CryptGenerateRandom(PRIMARY_SEED_SIZE, gp.SPSeed.t.buffer);

   // Create new shProof and ehProof value from RNG
   CryptGenerateRandom(PROOF_SIZE, gp.shProof.t.buffer);
   CryptGenerateRandom(PROOF_SIZE, gp.ehProof.t.buffer);

   // Enable storage and endorsement hierarchy
   gc.shEnable = gc.ehEnable = TRUE;

   // set the authValue buffers to zero
   MemorySet(gp.ownerAuth.t.buffer, 0, gp.ownerAuth.t.size);
   MemorySet(gp.endorsementAuth.t.buffer, 0, gp.endorsementAuth.t.size);
   MemorySet(gp.lockoutAuth.t.buffer, 0, gp.lockoutAuth.t.size);
   // Set storage, endorsement and lockout authValue to null
   gp.ownerAuth.t.size = gp.endorsementAuth.t.size = gp.lockoutAuth.t.size = 0;

   // Set storage, endorsement, and lockout authPolicy to null
   gp.ownerAlg = gp.endorsementAlg = gp.lockoutAlg = TPM_ALG_NULL;
   gp.ownerPolicy.t.size = 0;
   gp.endorsementPolicy.t.size = 0;
   gp.lockoutPolicy.t.size = 0;

   // Flush loaded object in storage and endorsement hierarchy
   ObjectFlushHierarchy(TPM_RH_OWNER);
   ObjectFlushHierarchy(TPM_RH_ENDORSEMENT);

   // Flush owner and endorsement object and owner index in NV
   NvFlushHierarchy(TPM_RH_OWNER);
   NvFlushHierarchy(TPM_RH_ENDORSEMENT);

   // Save hierarchy changes to NV
   NvWriteReserved(NV_SP_SEED, &gp.SPSeed);
   NvWriteReserved(NV_SH_PROOF, &gp.shProof);
   NvWriteReserved(NV_EH_PROOF, &gp.ehProof);
   NvWriteReserved(NV_OWNER_AUTH, &gp.ownerAuth);
   NvWriteReserved(NV_ENDORSEMENT_AUTH, &gp.endorsementAuth);
   NvWriteReserved(NV_LOCKOUT_AUTH, &gp.lockoutAuth);
   NvWriteReserved(NV_OWNER_ALG, &gp.ownerAlg);
   NvWriteReserved(NV_ENDORSEMENT_ALG, &gp.endorsementAlg);
   NvWriteReserved(NV_LOCKOUT_ALG, &gp.lockoutAlg);
   NvWriteReserved(NV_OWNER_POLICY, &gp.ownerPolicy);
   NvWriteReserved(NV_ENDORSEMENT_POLICY, &gp.endorsementPolicy);
   NvWriteReserved(NV_LOCKOUT_POLICY, &gp.lockoutPolicy);

   // Initialize dictionary attack parameters
   DAPreInstall_Init();

   // Reset clock
   go.clock = 0;
   go.clockSafe = YES;
   // Update the DRBG state whenever writing orderly state to NV
   CryptDrbgGetPutState(GET_STATE);
   NvWriteReserved(NV_ORDERLY_DATA, &go);

   // Reset counters
   gp.resetCount = gr.restartCount = gr.clearCount = 0;
   gp.auditCounter = 0;
   NvWriteReserved(NV_RESET_COUNT, &gp.resetCount);
   NvWriteReserved(NV_AUDIT_COUNTER, &gp.auditCounter);

   // orderly state should be cleared because of the update to state clear data
   g_clearOrderly = TRUE;

   return TPM_RC_SUCCESS;
}
