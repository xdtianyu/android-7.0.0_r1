// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 3: Commands
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include "InternalRoutines.h"
#include "ChangeEPS_fp.h"
TPM_RC
TPM2_ChangeEPS(
   ChangeEPS_In     *in              // IN: input parameter list
   )
{
   TPM_RC           result;

   // The command needs NV update. Check if NV is available.
   // A TPM_RC_NV_UNAVAILABLE or TPM_RC_NV_RATE error may be returned at
   // this point
   result = NvIsAvailable();
   if(result != TPM_RC_SUCCESS) return result;

   // Input parameter is not reference in command action
   in = NULL;

// Internal Data Update

   // Reset endorsement hierarchy seed from RNG
   CryptGenerateRandom(PRIMARY_SEED_SIZE, gp.EPSeed.t.buffer);

   // Create new ehProof value from RNG
   CryptGenerateRandom(PROOF_SIZE, gp.ehProof.t.buffer);

   // Enable endorsement hierarchy
   gc.ehEnable = TRUE;

   // set authValue buffer to zeros
   MemorySet(gp.endorsementAuth.t.buffer, 0, gp.endorsementAuth.t.size);
   // Set endorsement authValue to null
   gp.endorsementAuth.t.size = 0;

   // Set endorsement authPolicy to null
   gp.endorsementAlg = TPM_ALG_NULL;
   gp.endorsementPolicy.t.size = 0;

   // Flush loaded object in endorsement hierarchy
   ObjectFlushHierarchy(TPM_RH_ENDORSEMENT);

   // Flush evict object of endorsement hierarchy stored in NV
   NvFlushHierarchy(TPM_RH_ENDORSEMENT);

   // Save hierarchy changes to NV
   NvWriteReserved(NV_EP_SEED, &gp.EPSeed);
   NvWriteReserved(NV_EH_PROOF, &gp.ehProof);
   NvWriteReserved(NV_ENDORSEMENT_AUTH, &gp.endorsementAuth);
   NvWriteReserved(NV_ENDORSEMENT_ALG, &gp.endorsementAlg);
   NvWriteReserved(NV_ENDORSEMENT_POLICY, &gp.endorsementPolicy);

   // orderly state should be cleared because of the update to state clear data
   g_clearOrderly = TRUE;

   return TPM_RC_SUCCESS;
}
