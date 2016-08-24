// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 3: Commands
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include "InternalRoutines.h"
#include "PCR_Event_fp.h"
//
//
//     Error Returns                 Meaning
//
//     TPM_RC_LOCALITY               current command locality is not allowed to extend the PCR
//                                   referenced by pcrHandle
//
TPM_RC
TPM2_PCR_Event(
   PCR_Event_In      *in,             // IN: input parameter list
   PCR_Event_Out     *out             // OUT: output parameter list
   )
{
   TPM_RC                result;
   HASH_STATE            hashState;
   UINT32                i;
   UINT16                size;

// Input Validation

   // If a PCR extend is required
   if(in->pcrHandle != TPM_RH_NULL)
   {
       // If the PCR is not allow to extend, return error
       if(!PCRIsExtendAllowed(in->pcrHandle))
           return TPM_RC_LOCALITY;

       // If PCR is state saved and we need to update orderlyState, check NV
       // availability
       if(PCRIsStateSaved(in->pcrHandle) && gp.orderlyState != SHUTDOWN_NONE)
       {
           result = NvIsAvailable();
           if(result != TPM_RC_SUCCESS) return result;
           g_clearOrderly = TRUE;
       }
   }

// Internal Data Update

   out->digests.count = HASH_COUNT;

   // Iterate supported PCR bank algorithms to extend
   for(i = 0; i < HASH_COUNT; i++)
   {
       TPM_ALG_ID hash = CryptGetHashAlgByIndex(i);
       out->digests.digests[i].hashAlg = hash;
       size = CryptStartHash(hash, &hashState);
       CryptUpdateDigest2B(&hashState, &in->eventData.b);
       CryptCompleteHash(&hashState, size,
                         (BYTE *) &out->digests.digests[i].digest);
       if(in->pcrHandle != TPM_RH_NULL)
           PCRExtend(in->pcrHandle, hash, size,
                     (BYTE *) &out->digests.digests[i].digest);
   }

   return TPM_RC_SUCCESS;
}
