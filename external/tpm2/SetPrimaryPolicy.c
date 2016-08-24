// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 3: Commands
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include "InternalRoutines.h"
#include "SetPrimaryPolicy_fp.h"
//
//
//     Error Returns               Meaning
//
//     TPM_RC_SIZE                 size of input authPolicy is not consistent with input hash algorithm
//
TPM_RC
TPM2_SetPrimaryPolicy(
   SetPrimaryPolicy_In    *in                 // IN: input parameter list
   )
{
   TPM_RC                  result;

// Input Validation

   // Check the authPolicy consistent with hash algorithm. If the policy size is
   // zero, then the algorithm is required to be TPM_ALG_NULL
   if(in->authPolicy.t.size != CryptGetHashDigestSize(in->hashAlg))
       return TPM_RC_SIZE + RC_SetPrimaryPolicy_authPolicy;

   // The command need NV update for OWNER and ENDORSEMENT hierarchy, and
   // might need orderlyState update for PLATFROM hierarchy.
   // Check if NV is available. A TPM_RC_NV_UNAVAILABLE or TPM_RC_NV_RATE
   // error may be returned at this point
   result = NvIsAvailable();
   if(result != TPM_RC_SUCCESS)
       return result;

// Internal Data Update

   // Set hierarchy policy
   switch(in->authHandle)
   {
       case TPM_RH_OWNER:
           gp.ownerAlg = in->hashAlg;
           gp.ownerPolicy = in->authPolicy;
           NvWriteReserved(NV_OWNER_ALG, &gp.ownerAlg);
           NvWriteReserved(NV_OWNER_POLICY, &gp.ownerPolicy);
           break;
       case TPM_RH_ENDORSEMENT:
           gp.endorsementAlg = in->hashAlg;
           gp.endorsementPolicy = in->authPolicy;
           NvWriteReserved(NV_ENDORSEMENT_ALG, &gp.endorsementAlg);
           NvWriteReserved(NV_ENDORSEMENT_POLICY, &gp.endorsementPolicy);
           break;
       case TPM_RH_PLATFORM:
           gc.platformAlg = in->hashAlg;
           gc.platformPolicy = in->authPolicy;
           // need to update orderly state
           g_clearOrderly = TRUE;
           break;
       case TPM_RH_LOCKOUT:
           gp.lockoutAlg = in->hashAlg;
           gp.lockoutPolicy = in->authPolicy;
           NvWriteReserved(NV_LOCKOUT_ALG, &gp.lockoutAlg);
           NvWriteReserved(NV_LOCKOUT_POLICY, &gp.lockoutPolicy);
           break;

       default:
            pAssert(FALSE);
            break;
   }

   return TPM_RC_SUCCESS;
}
