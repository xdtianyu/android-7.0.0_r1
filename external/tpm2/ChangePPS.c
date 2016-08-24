// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 3: Commands
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include "InternalRoutines.h"
#include "ChangePPS_fp.h"
TPM_RC
TPM2_ChangePPS(
   ChangePPS_In   *in             // IN: input parameter list
   )
{
   UINT32         i;
   TPM_RC         result;

   // Check if NV is available. A TPM_RC_NV_UNAVAILABLE or TPM_RC_NV_RATE
   // error may be returned at this point
   result = NvIsAvailable();
   if(result != TPM_RC_SUCCESS) return result;

   // Input parameter is not reference in command action
   in = NULL;

// Internal Data Update

   // Reset platform hierarchy seed from RNG
   CryptGenerateRandom(PRIMARY_SEED_SIZE, gp.PPSeed.t.buffer);

   // Create a new phProof value from RNG to prevent the saved platform
   // hierarchy contexts being loaded
   CryptGenerateRandom(PROOF_SIZE, gp.phProof.t.buffer);

   // Set platform authPolicy to null
   gc.platformAlg = TPM_ALG_NULL;
   gc.platformPolicy.t.size = 0;

   // Flush loaded object in platform hierarchy
   ObjectFlushHierarchy(TPM_RH_PLATFORM);

   // Flush platform evict object and index in NV
   NvFlushHierarchy(TPM_RH_PLATFORM);

   // Save hierarchy changes to NV
   NvWriteReserved(NV_PP_SEED, &gp.PPSeed);
   NvWriteReserved(NV_PH_PROOF, &gp.phProof);

   // Re-initialize PCR policies
   for(i = 0; i < NUM_POLICY_PCR_GROUP; i++)
   {
       gp.pcrPolicies.hashAlg[i] = TPM_ALG_NULL;
       gp.pcrPolicies.policy[i].t.size = 0;
   }
   NvWriteReserved(NV_PCR_POLICIES, &gp.pcrPolicies);

   // orderly state should be cleared because of the update to state clear data
   g_clearOrderly = TRUE;

   return TPM_RC_SUCCESS;
}
