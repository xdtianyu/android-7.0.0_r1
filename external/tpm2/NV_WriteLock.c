// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 3: Commands
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include "InternalRoutines.h"
#include "NV_WriteLock_fp.h"
#include "NV_spt_fp.h"
//
//
//     Error Returns                    Meaning
//
//     TPM_RC_ATTRIBUTES                neither TPMA_NV_WRITEDEFINE nor
//                                      TPMA_NV_WRITE_STCLEAR is SET in Index referenced by
//                                      nvIndex
//     TPM_RC_NV_AUTHORIZATION          the authorization was valid but the authorizing entity (authHandle) is
//                                      not allowed to write to the Index referenced by nvIndex
//
TPM_RC
TPM2_NV_WriteLock(
   NV_WriteLock_In       *in                  // IN: input parameter list
   )
{
   TPM_RC            result;
   NV_INDEX          nvIndex;

// Input Validation:

   // Common write access checks, a TPM_RC_NV_AUTHORIZATION or TPM_RC_NV_LOCKED
   // error may be returned at this point
   result = NvWriteAccessChecks(in->authHandle, in->nvIndex);
   if(result != TPM_RC_SUCCESS)
   {
       if(result == TPM_RC_NV_AUTHORIZATION)
           return TPM_RC_NV_AUTHORIZATION;
       // If write access failed because the index is already locked, then it is
       // no error.
       return TPM_RC_SUCCESS;
   }

   // Get NV index info
   NvGetIndexInfo(in->nvIndex, &nvIndex);

   // if neither TPMA_NV_WRITEDEFINE nor TPMA_NV_WRITE_STCLEAR is set, the index
   // can not be write-locked
   if(   nvIndex.publicArea.attributes.TPMA_NV_WRITEDEFINE == CLEAR
      && nvIndex.publicArea.attributes.TPMA_NV_WRITE_STCLEAR == CLEAR)
       return TPM_RC_ATTRIBUTES + RC_NV_WriteLock_nvIndex;

// Internal Data Update

   // The command needs NV update. Check if NV is available.
   // A TPM_RC_NV_UNAVAILABLE or TPM_RC_NV_RATE error may be returned at
   // this point
   result = NvIsAvailable();
   if(result != TPM_RC_SUCCESS)
       return result;

   // Set the WRITELOCK attribute.
   // Note: if TPMA_NV_WRITELOCKED were already SET, then the write access check
   // above would have failed and this code isn't executed.
   nvIndex.publicArea.attributes.TPMA_NV_WRITELOCKED = SET;

   // Write index info back
   NvWriteIndexInfo(in->nvIndex, &nvIndex);

   return TPM_RC_SUCCESS;
}
