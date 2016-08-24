// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 3: Commands
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include "InternalRoutines.h"
#include "NV_SetBits_fp.h"
#include "NV_spt_fp.h"
//
//
//     Error Returns                 Meaning
//
//     TPM_RC_ATTRIBUTES             the TPMA_NV_BITS attribute is not SET in the Index referenced by
//                                   nvIndex
//     TPM_RC_NV_AUTHORIZATION       the authorization was valid but the authorizing entity (authHandle) is
//                                   not allowed to write to the Index referenced by nvIndex
//     TPM_RC_NV_LOCKED              the Index referenced by nvIndex is locked for writing
//
TPM_RC
TPM2_NV_SetBits(
   NV_SetBits_In     *in              // IN: input parameter list
   )
{
   TPM_RC            result;
   NV_INDEX          nvIndex;
   UINT64            oldValue;
   UINT64            newValue;

// Input Validation

   // Common access checks, NvWriteAccessCheck() may return TPM_RC_NV_AUTHORIZATION
   // or TPM_RC_NV_LOCKED
   // error may be returned at this point
   result = NvWriteAccessChecks(in->authHandle, in->nvIndex);
   if(result != TPM_RC_SUCCESS)
       return result;

   // Get NV index info
   NvGetIndexInfo(in->nvIndex, &nvIndex);

   // Make sure that this is a bit field
   if(nvIndex.publicArea.attributes.TPMA_NV_BITS != SET)
       return TPM_RC_ATTRIBUTES + RC_NV_SetBits_nvIndex;

   // If index is not been written, initialize it
   if(nvIndex.publicArea.attributes.TPMA_NV_WRITTEN == CLEAR)
       oldValue = 0;
   else
       // Read index data
       NvGetIntIndexData(in->nvIndex, &nvIndex, &oldValue);

   // Figure out what the new value is going to be
   newValue = oldValue | in->bits;

   // If the Index is not-orderly and it has changed, or if this is the first
   // write, NV will need to be updated.
   if(    (    nvIndex.publicArea.attributes.TPMA_NV_ORDERLY == CLEAR
           && newValue != oldValue)
       || nvIndex.publicArea.attributes.TPMA_NV_WRITTEN == CLEAR)
   {

// Internal Data Update
       // Check if NV is available. NvIsAvailable may return TPM_RC_NV_UNAVAILABLE
       // TPM_RC_NV_RATE or TPM_RC_SUCCESS.
       result = NvIsAvailable();
       if(result != TPM_RC_SUCCESS)
           return result;

       // Write index data back. If necessary, this function will SET
       // TPMA_NV_WRITTEN.
       result = NvWriteIndexData(in->nvIndex, &nvIndex, 0, 8, &newValue);
   }
   return result;

}
