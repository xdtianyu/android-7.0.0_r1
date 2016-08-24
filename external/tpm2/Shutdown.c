// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 3: Commands
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include "InternalRoutines.h"
#include "Shutdown_fp.h"
//
//
//     Error Returns                   Meaning
//
//     TPM_RC_TYPE                     if PCR bank has been re-configured, a CLEAR StateSave() is
//                                     required
//
TPM_RC
TPM2_Shutdown(
   Shutdown_In       *in               // IN: input parameter list
   )
{
   TPM_RC            result;

   // The command needs NV update. Check if NV is available.
   // A TPM_RC_NV_UNAVAILABLE or TPM_RC_NV_RATE error may be returned at
   // this point
   result = NvIsAvailable();
   if(result != TPM_RC_SUCCESS) return result;

// Input Validation

   // If PCR bank has been reconfigured, a CLEAR state save is required
   if(g_pcrReConfig && in->shutdownType == TPM_SU_STATE)
       return TPM_RC_TYPE + RC_Shutdown_shutdownType;

// Internal Data Update

   // PCR private date state save
   PCRStateSave(in->shutdownType);

   // Get DRBG state
   CryptDrbgGetPutState(GET_STATE);

   // Save all orderly data
   NvWriteReserved(NV_ORDERLY_DATA, &go);

   // Save RAM backed NV index data
   NvStateSave();

   if(in->shutdownType == TPM_SU_STATE)
   {
       // Save STATE_RESET and STATE_CLEAR data
       NvWriteReserved(NV_STATE_CLEAR, &gc);
       NvWriteReserved(NV_STATE_RESET, &gr);
   }
   else if(in->shutdownType == TPM_SU_CLEAR)
   {
       // Save STATE_RESET data
       NvWriteReserved(NV_STATE_RESET, &gr);
   }

   // Write orderly shut down state
   if(in->shutdownType == TPM_SU_CLEAR)
       gp.orderlyState = TPM_SU_CLEAR;
   else if(in->shutdownType == TPM_SU_STATE)
   {
       gp.orderlyState = TPM_SU_STATE;
       // Hack for the H-CRTM and Startup locality settings
         if(g_DrtmPreStartup)
             gp.orderlyState |= PRE_STARTUP_FLAG;
         else if(g_StartupLocality3)
             gp.orderlyState |= STARTUP_LOCALITY_3;
   }
   else
       pAssert(FALSE);

   NvWriteReserved(NV_ORDERLY, &gp.orderlyState);

   //   If PRE_STARTUP_FLAG was SET, then it will stay set in gp.orderlyState even
   //   if the TPM isn't actually shut down. This is OK because all other checks
   //   of gp.orderlyState are to see if it is SHUTDOWN_NONE. So, having
   //   gp.orderlyState set to another value that is also not SHUTDOWN_NONE, is not
   //   an issue. This must be the case, otherwise, it would be impossible to add
   //   an additional shutdown type without major changes to the code.

   return TPM_RC_SUCCESS;
}
