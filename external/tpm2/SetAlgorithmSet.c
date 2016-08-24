// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 3: Commands
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include "InternalRoutines.h"
#include "SetAlgorithmSet_fp.h"
TPM_RC
TPM2_SetAlgorithmSet(
   SetAlgorithmSet_In     *in            // IN: input parameter list
   )
{
   TPM_RC       result;

   // The command needs NV update. Check if NV is available.
   // A TPM_RC_NV_UNAVAILABLE or TPM_RC_NV_RATE error may be returned at
   // this point
   result = NvIsAvailable();
   if(result != TPM_RC_SUCCESS) return result;

// Internal Data Update
   gp.algorithmSet = in->algorithmSet;

   // Write the algorithm set changes to NV
   NvWriteReserved(NV_ALGORITHM_SET, &gp.algorithmSet);

   return TPM_RC_SUCCESS;
}
