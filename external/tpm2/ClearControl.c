// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 3: Commands
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include "InternalRoutines.h"
#include "ClearControl_fp.h"
//
//
//     Error Returns                   Meaning
//
//     TPM_RC_AUTH_FAIL                authorization is not properly given
//
TPM_RC
TPM2_ClearControl(
   ClearControl_In       *in                 // IN: input parameter list
   )
{
   TPM_RC      result;

   // The command needs NV update. Check if NV is available.
   // A TPM_RC_NV_UNAVAILABLE or TPM_RC_NV_RATE error may be returned at
   // this point
   result = NvIsAvailable();
   if(result != TPM_RC_SUCCESS) return result;

// Input Validation

   // LockoutAuth may be used to set disableLockoutClear to TRUE but not to FALSE
   if(in->auth == TPM_RH_LOCKOUT && in->disable == NO)
       return TPM_RC_AUTH_FAIL;

// Internal Data Update

   if(in->disable == YES)
       gp.disableClear = TRUE;
   else
       gp.disableClear = FALSE;

   // Record the change to NV
   NvWriteReserved(NV_DISABLE_CLEAR, &gp.disableClear);

   return TPM_RC_SUCCESS;
}
