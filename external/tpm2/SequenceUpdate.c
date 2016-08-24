// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 3: Commands
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include "InternalRoutines.h"
#include "SequenceUpdate_fp.h"
//
//
//     Error Returns                   Meaning
//
//     TPM_RC_MODE                     sequenceHandle does not reference a hash or HMAC sequence
//                                     object
//
TPM_RC
TPM2_SequenceUpdate(
   SequenceUpdate_In     *in               // IN: input parameter list
   )
{
   OBJECT                      *object;

// Input Validation

   // Get sequence object pointer
   object = ObjectGet(in->sequenceHandle);

   // Check that referenced object is a sequence object.
   if(!ObjectIsSequence(object))
       return TPM_RC_MODE + RC_SequenceUpdate_sequenceHandle;

// Internal Data Update

   if(object->attributes.eventSeq == SET)
   {
       // Update event sequence object
       UINT32           i;
       HASH_OBJECT     *hashObject = (HASH_OBJECT *)object;
       for(i = 0; i < HASH_COUNT; i++)
       {
           // Update sequence object
           CryptUpdateDigest2B(&hashObject->state.hashState[i], &in->buffer.b);
       }
   }
   else
   {
       HASH_OBJECT     *hashObject = (HASH_OBJECT *)object;

       // Update hash/HMAC sequence object
       if(hashObject->attributes.hashSeq == SET)
       {
           // Is this the first block of the sequence
           if(hashObject->attributes.firstBlock == CLEAR)
           {
               // If so, indicate that first block was received
               hashObject->attributes.firstBlock = SET;

                // Check the first block to see if the first block can contain
                // the TPM_GENERATED_VALUE. If it does, it is not safe for
                // a ticket.
                if(TicketIsSafe(&in->buffer.b))
                    hashObject->attributes.ticketSafe = SET;
            }
            // Update sequence object hash/HMAC stack
            CryptUpdateDigest2B(&hashObject->state.hashState[0], &in->buffer.b);

       }
       else if(object->attributes.hmacSeq == SET)
       {
           HASH_OBJECT     *hashObject = (HASH_OBJECT *)object;

            // Update sequence object hash/HMAC stack
            CryptUpdateDigest2B(&hashObject->state.hmacState, &in->buffer.b);
       }
   }

   return TPM_RC_SUCCESS;
}
