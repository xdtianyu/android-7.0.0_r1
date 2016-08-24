// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 3: Commands
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include "InternalRoutines.h"
#include "NV_Extend_fp.h"
#include "NV_spt_fp.h"
//
//
//     Error Returns               Meaning
//
//     TPM_RC_ATTRIBUTES           the TPMA_NV_EXTEND attribute is not SET in the Index referenced
//                                 by nvIndex
//     TPM_RC_NV_AUTHORIZATION     the authorization was valid but the authorizing entity (authHandle) is
//                                 not allowed to write to the Index referenced by nvIndex
//     TPM_RC_NV_LOCKED            the Index referenced by nvIndex is locked for writing
//
TPM_RC
TPM2_NV_Extend(
   NV_Extend_In      *in            // IN: input parameter list
   )
{
   TPM_RC                  result;
   NV_INDEX                nvIndex;

   TPM2B_DIGEST            oldDigest;
   TPM2B_DIGEST            newDigest;
   HASH_STATE              hashState;

// Input Validation

   // Common access checks, NvWriteAccessCheck() may return TPM_RC_NV_AUTHORIZATION
   // or TPM_RC_NV_LOCKED
   result = NvWriteAccessChecks(in->authHandle, in->nvIndex);
   if(result != TPM_RC_SUCCESS)
       return result;

   // Get NV index info
   NvGetIndexInfo(in->nvIndex, &nvIndex);

   // Make sure that this is an extend index
   if(nvIndex.publicArea.attributes.TPMA_NV_EXTEND != SET)
       return TPM_RC_ATTRIBUTES + RC_NV_Extend_nvIndex;

   // If the Index is not-orderly, or if this is the first write, NV will
   // need to be updated.
   if(   nvIndex.publicArea.attributes.TPMA_NV_ORDERLY == CLEAR
      || nvIndex.publicArea.attributes.TPMA_NV_WRITTEN == CLEAR)
   {
       // Check if NV is available. NvIsAvailable may return TPM_RC_NV_UNAVAILABLE
       // TPM_RC_NV_RATE or TPM_RC_SUCCESS.
       result = NvIsAvailable();
       if(result != TPM_RC_SUCCESS)
           return result;
   }

// Internal Data Update

   // Perform the write.
   oldDigest.t.size = CryptGetHashDigestSize(nvIndex.publicArea.nameAlg);
   pAssert(oldDigest.t.size <= sizeof(oldDigest.t.buffer));
   if(nvIndex.publicArea.attributes.TPMA_NV_WRITTEN == SET)
   {
       NvGetIndexData(in->nvIndex, &nvIndex, 0,
                      oldDigest.t.size, oldDigest.t.buffer);
   }
   else
   {
       MemorySet(oldDigest.t.buffer, 0, oldDigest.t.size);
   }
   // Start hash
   newDigest.t.size = CryptStartHash(nvIndex.publicArea.nameAlg, &hashState);

   // Adding old digest
   CryptUpdateDigest2B(&hashState, &oldDigest.b);

   // Adding new data
   CryptUpdateDigest2B(&hashState, &in->data.b);

   // Complete hash
   CryptCompleteHash2B(&hashState, &newDigest.b);

   // Write extended hash back.
   // Note, this routine will SET the TPMA_NV_WRITTEN attribute if necessary
   return NvWriteIndexData(in->nvIndex, &nvIndex, 0,
                           newDigest.t.size, newDigest.t.buffer);
}
