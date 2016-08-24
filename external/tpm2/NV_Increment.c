// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 3: Commands
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include "InternalRoutines.h"
#include "NV_Increment_fp.h"
#include "NV_spt_fp.h"
//
//
//     Error Returns                   Meaning
//
//     TPM_RC_ATTRIBUTES               NV index is not a counter
//     TPM_RC_NV_AUTHORIZATION         authorization failure
//     TPM_RC_NV_LOCKED                Index is write locked
//
TPM_RC
TPM2_NV_Increment(
   NV_Increment_In       *in                  // IN: input parameter list
   )
{
   TPM_RC            result;
   NV_INDEX          nvIndex;
   UINT64            countValue;

// Input Validation

   // Common access checks, a TPM_RC_NV_AUTHORIZATION or TPM_RC_NV_LOCKED
   // error may be returned at this point
   result = NvWriteAccessChecks(in->authHandle, in->nvIndex);
   if(result != TPM_RC_SUCCESS)
       return result;

   // Get NV index info
   NvGetIndexInfo(in->nvIndex, &nvIndex);

   // Make sure that this is a counter
   if(nvIndex.publicArea.attributes.TPMA_NV_COUNTER != SET)
       return TPM_RC_ATTRIBUTES + RC_NV_Increment_nvIndex;

// Internal Data Update

   // If counter index is not been written, initialize it
   if(nvIndex.publicArea.attributes.TPMA_NV_WRITTEN == CLEAR)
       countValue = NvInitialCounter();
   else
       // Read NV data in native format for TPM CPU.
       NvGetIntIndexData(in->nvIndex, &nvIndex, &countValue);

   // Do the increment
   countValue++;

   // If this is an orderly counter that just rolled over, need to be able to
   // write to NV to proceed. This check is done here, because NvWriteIndexData()
   // does not see if the update is for counter rollover.
   if(    nvIndex.publicArea.attributes.TPMA_NV_ORDERLY == SET
       && (countValue & MAX_ORDERLY_COUNT) == 0)
   {
       result = NvIsAvailable();
       if(result != TPM_RC_SUCCESS)
           return result;

       // Need to force an NV update
       g_updateNV = TRUE;
//
   }

   // Write NV data back. A TPM_RC_NV_UNAVAILABLE or TPM_RC_NV_RATE error may
   // be returned at this point. If necessary, this function will set the
   // TPMA_NV_WRITTEN attribute
   return NvWriteIndexData(in->nvIndex, &nvIndex, 0, 8, &countValue);

}
