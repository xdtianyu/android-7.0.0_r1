// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 3: Commands
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include "InternalRoutines.h"
#include "PCR_Allocate_fp.h"
//
//
//     Error Returns                    Meaning
//
//     TPM_RC_PCR                       the allocation did not have required PCR
//     TPM_RC_NV_UNAVAILABLE            NV is not accessible
//     TPM_RC_NV_RATE                   NV is in a rate-limiting mode
//
TPM_RC
TPM2_PCR_Allocate(
   PCR_Allocate_In       *in,                 // IN: input parameter list
   PCR_Allocate_Out      *out                 // OUT: output parameter list
   )
{
   TPM_RC      result;

   // The command needs NV update. Check if NV is available.
   // A TPM_RC_NV_UNAVAILABLE or TPM_RC_NV_RATE error may be returned at
   // this point.
   // Note: These codes are not listed in the return values above because it is
   // an implementation choice to check in this routine rather than in a common
   // function that is called before these actions are called. These return values
   // are described in the Response Code section of Part 3.
   result = NvIsAvailable();
   if(result != TPM_RC_SUCCESS)
       return result;

// Command Output

   // Call PCR Allocation function.
   result = PCRAllocate(&in->pcrAllocation, &out->maxPCR,
                                        &out->sizeNeeded, &out->sizeAvailable);
   if(result == TPM_RC_PCR)
       return result;

   //
   out->allocationSuccess = (result == TPM_RC_SUCCESS);

   // if re-configuration succeeds, set the flag to indicate PCR configuration is
   // going to be changed in next boot
   if(out->allocationSuccess == YES)
       g_pcrReConfig = TRUE;

   return TPM_RC_SUCCESS;
}
