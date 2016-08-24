// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 3: Commands
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include "InternalRoutines.h"
#include "EventSequenceComplete_fp.h"
//
//
//     Error Returns                 Meaning
//
//     TPM_RC_LOCALITY               PCR extension is not allowed at the current locality
//     TPM_RC_MODE                   input handle is not a valid event sequence object
//
TPM_RC
TPM2_EventSequenceComplete(
   EventSequenceComplete_In      *in,                // IN: input parameter list
   EventSequenceComplete_Out     *out                // OUT: output parameter list
   )
{
   TPM_RC              result;
   HASH_OBJECT        *hashObject;
   UINT32              i;
   TPM_ALG_ID          hashAlg;

// Input validation

   // get the event sequence object pointer
   hashObject = (HASH_OBJECT *)ObjectGet(in->sequenceHandle);

   // input handle must reference an event sequence object
   if(hashObject->attributes.eventSeq != SET)
       return TPM_RC_MODE + RC_EventSequenceComplete_sequenceHandle;

   // see if a PCR extend is requested in call
   if(in->pcrHandle != TPM_RH_NULL)
   {
       // see if extend of the PCR is allowed at the locality of the command,
       if(!PCRIsExtendAllowed(in->pcrHandle))
           return TPM_RC_LOCALITY;
       // if an extend is going to take place, then check to see if there has
       // been an orderly shutdown. If so, and the selected PCR is one of the
       // state saved PCR, then the orderly state has to change. The orderly state
       // does not change for PCR that are not preserved.
       // NOTE: This doesn't just check for Shutdown(STATE) because the orderly
       // state will have to change if this is a state-saved PCR regardless
       // of the current state. This is because a subsequent Shutdown(STATE) will
       // check to see if there was an orderly shutdown and not do anything if
       // there was. So, this must indicate that a future Shutdown(STATE) has
       // something to do.
       if(gp.orderlyState != SHUTDOWN_NONE && PCRIsStateSaved(in->pcrHandle))
       {
           result = NvIsAvailable();
           if(result != TPM_RC_SUCCESS) return result;
           g_clearOrderly = TRUE;
       }
   }

// Command Output

   out->results.count = 0;

   for(i = 0; i < HASH_COUNT; i++)
   {
       hashAlg = CryptGetHashAlgByIndex(i);
       // Update last piece of data
       CryptUpdateDigest2B(&hashObject->state.hashState[i], &in->buffer.b);
       // Complete hash
       out->results.digests[out->results.count].hashAlg = hashAlg;
       CryptCompleteHash(&hashObject->state.hashState[i],
                       CryptGetHashDigestSize(hashAlg),
                       (BYTE *) &out->results.digests[out->results.count].digest);

       // Extend PCR
       if(in->pcrHandle != TPM_RH_NULL)
           PCRExtend(in->pcrHandle, hashAlg,
                     CryptGetHashDigestSize(hashAlg),
                     (BYTE *) &out->results.digests[out->results.count].digest);
       out->results.count++;
   }

// Internal Data Update

   // mark sequence object as evict so it will be flushed on the way out
   hashObject->attributes.evict = SET;

   return TPM_RC_SUCCESS;
}
