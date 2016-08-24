// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 3: Commands
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include "InternalRoutines.h"
#include "NV_ChangeAuth_fp.h"
//
//
//     Error Returns                 Meaning
//
//     TPM_RC_SIZE                   newAuth size is larger than the digest size of the Name algorithm for
//                                   the Index referenced by 'nvIndex
//
TPM_RC
TPM2_NV_ChangeAuth(
   NV_ChangeAuth_In   *in                  // IN: input parameter list
   )
{
   TPM_RC         result;
   NV_INDEX       nvIndex;

// Input Validation
   // Check if NV is available. NvIsAvailable may return TPM_RC_NV_UNAVAILABLE
   // TPM_RC_NV_RATE or TPM_RC_SUCCESS.
   result = NvIsAvailable();
   if(result != TPM_RC_SUCCESS) return result;

   // Read index info from NV
   NvGetIndexInfo(in->nvIndex, &nvIndex);

   // Remove any trailing zeros that might have been added by the caller
   // to obfuscate the size.
   MemoryRemoveTrailingZeros(&(in->newAuth));

   // Make sure that the authValue is no larger than the nameAlg of the Index
   if(in->newAuth.t.size > CryptGetHashDigestSize(nvIndex.publicArea.nameAlg))
       return TPM_RC_SIZE + RC_NV_ChangeAuth_newAuth;

// Internal Data Update
   // Change auth
   nvIndex.authValue = in->newAuth;
   // Write index info back to NV
   NvWriteIndexInfo(in->nvIndex, &nvIndex);

   return TPM_RC_SUCCESS;
}
