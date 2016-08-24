// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 3: Commands
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include "InternalRoutines.h"
#include "PCR_Reset_fp.h"
//
//
//     Error Returns                     Meaning
//
//     TPM_RC_LOCALITY                   current command locality is not allowed to reset the PCR referenced
//                                       by pcrHandle
//
TPM_RC
TPM2_PCR_Reset(
   PCR_Reset_In      *in                 // IN: input parameter list
   )
{
   TPM_RC        result;

// Input Validation

   // Check if the reset operation is allowed by the current command locality
   if(!PCRIsResetAllowed(in->pcrHandle))
       return TPM_RC_LOCALITY;

   // If PCR is state saved and we need to update orderlyState, check NV
   // availability
   if(PCRIsStateSaved(in->pcrHandle) && gp.orderlyState != SHUTDOWN_NONE)
   {
       result = NvIsAvailable();
       if(result != TPM_RC_SUCCESS)
           return result;
       g_clearOrderly = TRUE;
   }

// Internal Data Update

   // Reset selected PCR in all banks to 0
   PCRSetValue(in->pcrHandle, 0);

   // Indicate that the PCR changed so that pcrCounter will be incremented if
   // necessary.
   PCRChanged(in->pcrHandle);

   return TPM_RC_SUCCESS;
}
