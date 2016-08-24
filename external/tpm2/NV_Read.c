// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 3: Commands
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include "InternalRoutines.h"
#include "NV_Read_fp.h"
#include "NV_spt_fp.h"
//
//
//     Error Returns                     Meaning
//
//     TPM_RC_NV_AUTHORIZATION           the authorization was valid but the authorizing entity (authHandle) is
//                                       not allowed to read from the Index referenced by nvIndex
//     TPM_RC_NV_LOCKED                  the Index referenced by nvIndex is read locked
//     TPM_RC_NV_RANGE                   read range defined by size and offset is outside the range of the
//                                       Index referenced by nvIndex
//     TPM_RC_NV_UNINITIALIZED           the Index referenced by nvIndex has not been initialized (written)
//
TPM_RC
TPM2_NV_Read(
   NV_Read_In        *in,                 // IN: input parameter list
   NV_Read_Out       *out                 // OUT: output parameter list
   )
{
   NV_INDEX          nvIndex;
   TPM_RC            result;

// Input Validation

   // Get NV index info
   NvGetIndexInfo(in->nvIndex, &nvIndex);

   // Common read access checks. NvReadAccessChecks() returns
   // TPM_RC_NV_AUTHORIZATION, TPM_RC_NV_LOCKED, or TPM_RC_NV_UNINITIALIZED
   // error may be returned at this point
   result = NvReadAccessChecks(in->authHandle, in->nvIndex);
   if(result != TPM_RC_SUCCESS)
       return result;

   // Too much data
   if((in->size + in->offset) > nvIndex.publicArea.dataSize)
       return TPM_RC_NV_RANGE;

// Command Output

   // Set the return size
   out->data.t.size = in->size;
   // Perform the read
   NvGetIndexData(in->nvIndex, &nvIndex, in->offset, in->size, out->data.t.buffer);

   return TPM_RC_SUCCESS;
}
