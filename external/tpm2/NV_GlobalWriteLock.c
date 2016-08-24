// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 3: Commands
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include "InternalRoutines.h"
#include "NV_GlobalWriteLock_fp.h"
TPM_RC
TPM2_NV_GlobalWriteLock(
   NV_GlobalWriteLock_In       *in             // IN: input parameter list
   )
{
   TPM_RC           result;

   // Input parameter is not reference in command action
   in = NULL; // to silence compiler warnings.

   // The command needs NV update. Check if NV is available.
   // A TPM_RC_NV_UNAVAILABLE or TPM_RC_NV_RATE error may be returned at
   // this point
   result = NvIsAvailable();
   if(result != TPM_RC_SUCCESS)
       return result;

// Internal Data Update

   // Implementation dependent method of setting the global lock
   NvSetGlobalLock();

   return TPM_RC_SUCCESS;
}
