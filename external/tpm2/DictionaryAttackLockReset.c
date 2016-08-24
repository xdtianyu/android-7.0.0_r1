// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 3: Commands
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include "InternalRoutines.h"
#include "DictionaryAttackLockReset_fp.h"
TPM_RC
TPM2_DictionaryAttackLockReset(
   DictionaryAttackLockReset_In      *in             // IN: input parameter list
   )
{
   TPM_RC         result;

   // Input parameter is not reference in command action
   in = NULL;

   // The command needs NV update. Check if NV is available.
   // A TPM_RC_NV_UNAVAILABLE or TPM_RC_NV_RATE error may be returned at
   // this point
   result = NvIsAvailable();
   if(result != TPM_RC_SUCCESS) return result;

// Internal Data Update

   // Set failed tries to 0
   gp.failedTries = 0;

   // Record the changes to NV
   NvWriteReserved(NV_FAILED_TRIES, &gp.failedTries);

   return TPM_RC_SUCCESS;
}
