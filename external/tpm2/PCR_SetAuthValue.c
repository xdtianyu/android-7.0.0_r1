// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 3: Commands
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include "InternalRoutines.h"
#include "PCR_SetAuthValue_fp.h"
//
//
//     Error Returns                 Meaning
//
//     TPM_RC_VALUE                  PCR referenced by pcrHandle is not a member of a PCR
//                                   authorization group
//
TPM_RC
TPM2_PCR_SetAuthValue(
   PCR_SetAuthValue_In       *in              // IN: input parameter list
   )
{
   UINT32      groupIndex;
   TPM_RC      result;

// Input Validation:

   // If PCR does not belong to an auth group, return TPM_RC_VALUE
   if(!PCRBelongsAuthGroup(in->pcrHandle, &groupIndex))
       return TPM_RC_VALUE;

   // The command may cause the orderlyState to be cleared due to the update of
   // state clear data. If this is the case, Check if NV is available.
   // A TPM_RC_NV_UNAVAILABLE or TPM_RC_NV_RATE error may be returned at
   // this point
   if(gp.orderlyState != SHUTDOWN_NONE)
   {
       result = NvIsAvailable();
       if(result != TPM_RC_SUCCESS) return result;
       g_clearOrderly = TRUE;
   }

// Internal Data Update

   // Set PCR authValue
   gc.pcrAuthValues.auth[groupIndex] = in->auth;

   return TPM_RC_SUCCESS;
}
