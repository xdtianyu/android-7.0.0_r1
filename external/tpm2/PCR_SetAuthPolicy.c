// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 3: Commands
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include "InternalRoutines.h"
#include "PCR_SetAuthPolicy_fp.h"
//
//
//     Error Returns                     Meaning
//
//     TPM_RC_SIZE                       size of authPolicy is not the size of a digest produced by policyDigest
//     TPM_RC_VALUE                      PCR referenced by pcrNum is not a member of a PCR policy group
//
TPM_RC
TPM2_PCR_SetAuthPolicy(
   PCR_SetAuthPolicy_In       *in                   // IN: input parameter list
   )
{
   UINT32       groupIndex;

   TPM_RC       result;

   // The command needs NV update. Check if NV is available.
   // A TPM_RC_NV_UNAVAILABLE or TPM_RC_NV_RATE error may be returned at
   // this point
   result = NvIsAvailable();
   if(result != TPM_RC_SUCCESS) return result;

// Input Validation:

   // Check the authPolicy consistent with hash algorithm
   if(in->authPolicy.t.size != CryptGetHashDigestSize(in->hashAlg))
       return TPM_RC_SIZE + RC_PCR_SetAuthPolicy_authPolicy;

   // If PCR does not belong to a policy group, return TPM_RC_VALUE
   if(!PCRBelongsPolicyGroup(in->pcrNum, &groupIndex))
       return TPM_RC_VALUE + RC_PCR_SetAuthPolicy_pcrNum;

// Internal Data Update

   // Set PCR policy
   gp.pcrPolicies.hashAlg[groupIndex] = in->hashAlg;
   gp.pcrPolicies.policy[groupIndex] = in->authPolicy;

   // Save new policy to NV
   NvWriteReserved(NV_PCR_POLICIES, &gp.pcrPolicies);

   return TPM_RC_SUCCESS;
}
