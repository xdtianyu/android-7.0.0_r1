// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 3: Commands
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include "InternalRoutines.h"
#include "NV_ReadLock_fp.h"
#include "NV_spt_fp.h"
//
//
//     Error Returns                    Meaning
//
//     TPM_RC_ATTRIBUTES                TPMA_NV_READ_STCLEAR is not SET so Index referenced by
//                                      nvIndex may not be write locked
//     TPM_RC_NV_AUTHORIZATION          the authorization was valid but the authorizing entity (authHandle) is
//                                      not allowed to read from the Index referenced by nvIndex
//
TPM_RC
TPM2_NV_ReadLock(
   NV_ReadLock_In    *in                 // IN: input parameter list
   )
{
   TPM_RC            result;
   NV_INDEX          nvIndex;

   // The command needs NV update. Check if NV is available.
   // A TPM_RC_NV_UNAVAILABLE or TPM_RC_NV_RATE error may be returned at
   // this point
   result = NvIsAvailable();
   if(result != TPM_RC_SUCCESS) return result;

// Input Validation

   // Common read access checks. NvReadAccessChecks() returns
   // TPM_RC_NV_AUTHORIZATION, TPM_RC_NV_LOCKED, or TPM_RC_NV_UNINITIALIZED
   // error may be returned at this point
   result = NvReadAccessChecks(in->authHandle, in->nvIndex);
   if(result != TPM_RC_SUCCESS)
   {
       if(result == TPM_RC_NV_AUTHORIZATION)
           return TPM_RC_NV_AUTHORIZATION;
       // Index is already locked for write
       else if(result == TPM_RC_NV_LOCKED)
           return TPM_RC_SUCCESS;

         // If NvReadAccessChecks return TPM_RC_NV_UNINITALIZED, then continue.
         // It is not an error to read lock an uninitialized Index.
   }

   // Get NV index info
   NvGetIndexInfo(in->nvIndex, &nvIndex);

   // if TPMA_NV_READ_STCLEAR is not set, the index can not be read-locked
   if(nvIndex.publicArea.attributes.TPMA_NV_READ_STCLEAR == CLEAR)
       return TPM_RC_ATTRIBUTES + RC_NV_ReadLock_nvIndex;

// Internal Data Update

   // Set the READLOCK attribute
   nvIndex.publicArea.attributes.TPMA_NV_READLOCKED = SET;
   // Write NV info back
   NvWriteIndexInfo(in->nvIndex, &nvIndex);

   return TPM_RC_SUCCESS;
}
