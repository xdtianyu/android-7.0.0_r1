// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 3: Commands
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include "InternalRoutines.h"
#include "HierarchyChangeAuth_fp.h"
#include "Object_spt_fp.h"
//
//
//     Error Returns                     Meaning
//
//     TPM_RC_SIZE                       newAuth size is greater than that of integrity hash digest
//
TPM_RC
TPM2_HierarchyChangeAuth(
   HierarchyChangeAuth_In    *in                    // IN: input parameter list
   )
{
   TPM_RC       result;

   // The command needs NV update. Check if NV is available.
   // A TPM_RC_NV_UNAVAILABLE or TPM_RC_NV_RATE error may be returned at
   // this point
   result = NvIsAvailable();
   if(result != TPM_RC_SUCCESS) return result;

   // Make sure the the auth value is a reasonable size (not larger than
   // the size of the digest produced by the integrity hash. The integrity
   // hash is assumed to produce the longest digest of any hash implemented
   // on the TPM.
   if( MemoryRemoveTrailingZeros(&in->newAuth)
           > CryptGetHashDigestSize(CONTEXT_INTEGRITY_HASH_ALG))
       return TPM_RC_SIZE + RC_HierarchyChangeAuth_newAuth;

   // Set hierarchy authValue
   switch(in->authHandle)
   {
   case TPM_RH_OWNER:
       gp.ownerAuth = in->newAuth;
       NvWriteReserved(NV_OWNER_AUTH, &gp.ownerAuth);
       break;
   case TPM_RH_ENDORSEMENT:
       gp.endorsementAuth = in->newAuth;
       NvWriteReserved(NV_ENDORSEMENT_AUTH, &gp.endorsementAuth);
       break;
   case TPM_RH_PLATFORM:
       gc.platformAuth = in->newAuth;
       // orderly state should be cleared
       g_clearOrderly = TRUE;
       break;
   case TPM_RH_LOCKOUT:
       gp.lockoutAuth = in->newAuth;
       NvWriteReserved(NV_LOCKOUT_AUTH, &gp.lockoutAuth);
       break;
   default:
       pAssert(FALSE);
       break;
   }

   return TPM_RC_SUCCESS;
}
