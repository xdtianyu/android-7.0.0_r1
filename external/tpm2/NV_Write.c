// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 3: Commands
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include "InternalRoutines.h"
#include "NV_Write_fp.h"
#include "NV_spt_fp.h"
//
//
//     Error Returns                    Meaning
//
//     TPM_RC_ATTRIBUTES                Index referenced by nvIndex has either TPMA_NV_BITS,
//                                      TPMA_NV_COUNTER, or TPMA_NV_EVENT attribute SET
//     TPM_RC_NV_AUTHORIZATION          the authorization was valid but the authorizing entity (authHandle) is
//                                      not allowed to write to the Index referenced by nvIndex
//     TPM_RC_NV_LOCKED                 Index referenced by nvIndex is write locked
//     TPM_RC_NV_RANGE                  if TPMA_NV_WRITEALL is SET then the write is not the size of the
//                                      Index referenced by nvIndex; otherwise, the write extends beyond the
//                                      limits of the Index
//
TPM_RC
TPM2_NV_Write(
   NV_Write_In       *in                 // IN: input parameter list
   )
{
   NV_INDEX          nvIndex;
   TPM_RC            result;

// Input Validation

   // Get NV index info
   NvGetIndexInfo(in->nvIndex, &nvIndex);

   // common access checks. NvWrtieAccessChecks() may return
   // TPM_RC_NV_AUTHORIZATION or TPM_RC_NV_LOCKED
   result = NvWriteAccessChecks(in->authHandle, in->nvIndex);
   if(result != TPM_RC_SUCCESS)
       return result;

   // Bits index, extend index or counter index may not be updated by
   // TPM2_NV_Write
   if(   nvIndex.publicArea.attributes.TPMA_NV_COUNTER == SET
      || nvIndex.publicArea.attributes.TPMA_NV_BITS == SET
      || nvIndex.publicArea.attributes.TPMA_NV_EXTEND == SET)
       return TPM_RC_ATTRIBUTES;

   // Too much data
   if((in->data.t.size + in->offset) > nvIndex.publicArea.dataSize)
       return TPM_RC_NV_RANGE;

   // If this index requires a full sized write, make sure that input range is
   // full sized
   if(   nvIndex.publicArea.attributes.TPMA_NV_WRITEALL == SET
      && in->data.t.size < nvIndex.publicArea.dataSize)
       return TPM_RC_NV_RANGE;

// Internal Data Update

   // Perform the write. This called routine will SET the TPMA_NV_WRITTEN
   // attribute if it has not already been SET. If NV isn't available, an error
   // will be returned.
   return NvWriteIndexData(in->nvIndex, &nvIndex, in->offset,
                           in->data.t.size, in->data.t.buffer);

}
