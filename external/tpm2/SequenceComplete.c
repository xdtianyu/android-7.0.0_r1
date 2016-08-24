// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 3: Commands
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include "InternalRoutines.h"
#include "SequenceComplete_fp.h"
#include "Platform.h"
//
//
//     Error Returns               Meaning
//
//     TPM_RC_TYPE                 sequenceHandle does not reference a hash or HMAC sequence
//                                 object
//
TPM_RC
TPM2_SequenceComplete(
   SequenceComplete_In    *in,               // IN: input parameter list
   SequenceComplete_Out   *out               // OUT: output parameter list
   )
{
   OBJECT                     *object;

// Input validation

   // Get hash object pointer
   object = ObjectGet(in->sequenceHandle);

   // input handle must be a hash or HMAC sequence object.
   if(   object->attributes.hashSeq == CLEAR
      && object->attributes.hmacSeq == CLEAR)
       return TPM_RC_MODE + RC_SequenceComplete_sequenceHandle;

// Command Output

   if(object->attributes.hashSeq == SET)           // sequence object for hash
   {
       // Update last piece of data
       HASH_OBJECT     *hashObject = (HASH_OBJECT *)object;

      // Get the hash algorithm before the algorithm is lost in CryptCompleteHash
       TPM_ALG_ID       hashAlg = hashObject->state.hashState[0].state.hashAlg;

       CryptUpdateDigest2B(&hashObject->state.hashState[0], &in->buffer.b);

       // Complete hash
       out->result.t.size
           = CryptGetHashDigestSize(
                 CryptGetContextAlg(&hashObject->state.hashState[0]));

       CryptCompleteHash2B(&hashObject->state.hashState[0], &out->result.b);

       // Check if the first block of the sequence has been received
       if(hashObject->attributes.firstBlock == CLEAR)
       {
           // If not, then this is the first block so see if it is 'safe'
           // to sign.
           if(TicketIsSafe(&in->buffer.b))
               hashObject->attributes.ticketSafe = SET;
       }

       // Output ticket
       out->validation.tag = TPM_ST_HASHCHECK;
       out->validation.hierarchy = in->hierarchy;

       if(in->hierarchy == TPM_RH_NULL)
       {
            // Ticket is not required
            out->validation.digest.t.size = 0;
       }
       else if(object->attributes.ticketSafe == CLEAR)
       {
           // Ticket is not safe to generate
           out->validation.hierarchy = TPM_RH_NULL;
           out->validation.digest.t.size = 0;
       }
       else
       {
           // Compute ticket
           TicketComputeHashCheck(out->validation.hierarchy, hashAlg,
                                  &out->result, &out->validation);
       }
   }
   else
   {
       HASH_OBJECT       *hashObject = (HASH_OBJECT *)object;

       //   Update last piece of data
       CryptUpdateDigest2B(&hashObject->state.hmacState, &in->buffer.b);
       // Complete hash/HMAC
       out->result.t.size =
           CryptGetHashDigestSize(
               CryptGetContextAlg(&hashObject->state.hmacState.hashState));
       CryptCompleteHMAC2B(&(hashObject->state.hmacState), &out->result.b);

       // No ticket is generated for HMAC sequence
       out->validation.tag = TPM_ST_HASHCHECK;
       out->validation.hierarchy = TPM_RH_NULL;
       out->validation.digest.t.size = 0;
   }

// Internal Data Update

   // mark sequence object as evict so it will be flushed on the way out
   object->attributes.evict = SET;

   return TPM_RC_SUCCESS;
}
