// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 3: Commands
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include "InternalRoutines.h"
#include "Rewrap_fp.h"
#include "Object_spt_fp.h"
//
//
//     Error Returns                 Meaning
//
//     TPM_RC_ATTRIBUTES             newParent is not a decryption key
//     TPM_RC_HANDLE                 oldParent does not consistent with inSymSeed
//     TPM_RC_INTEGRITY              the integrity check of inDuplicate failed
//     TPM_RC_KEY                    for an ECC key, the public key is not on the curve of the curve ID
//     TPM_RC_KEY_SIZE               the decrypted input symmetric key size does not matches the
//                                   symmetric algorithm key size of oldParent
//     TPM_RC_TYPE                   oldParent is not a storage key, or 'newParent is not a storage key
//     TPM_RC_VALUE                  for an 'oldParent; RSA key, the data to be decrypted is greater than
//                                   the public exponent
//     Unmarshal errors              errors during unmarshaling the input encrypted buffer to a ECC public
//                                   key, or unmarshal the private buffer to sensitive
//
TPM_RC
TPM2_Rewrap(
   Rewrap_In         *in,             // IN: input parameter list
   Rewrap_Out        *out             // OUT: output parameter list
   )
{
   TPM_RC                   result = TPM_RC_SUCCESS;
   OBJECT                   *oldParent;
   TPM2B_DATA               data;               // symmetric key
   UINT16                   hashSize = 0;
   TPM2B_PRIVATE            privateBlob;        // A temporary private blob
                                                // to transit between old
                                                // and new wrappers

// Input Validation

   if((in->inSymSeed.t.size == 0 && in->oldParent != TPM_RH_NULL)
           || (in->inSymSeed.t.size != 0 && in->oldParent == TPM_RH_NULL))
       return TPM_RC_HANDLE + RC_Rewrap_oldParent;

   if(in->oldParent != TPM_RH_NULL)
   {
       // Get old parent pointer
       oldParent = ObjectGet(in->oldParent);

         // old parent key must be a storage object
         if(!ObjectIsStorage(in->oldParent))
             return TPM_RC_TYPE + RC_Rewrap_oldParent;

         // Decrypt input secret data via asymmetric decryption. A
         // TPM_RC_VALUE, TPM_RC_KEY or unmarshal errors may be returned at this
         // point
         result = CryptSecretDecrypt(in->oldParent, NULL,
                                     "DUPLICATE", &in->inSymSeed, &data);
         if(result != TPM_RC_SUCCESS)
             return TPM_RC_VALUE + RC_Rewrap_inSymSeed;

       // Unwrap Outer
       result = UnwrapOuter(in->oldParent, &in->name,
                            oldParent->publicArea.nameAlg, (TPM2B_SEED *) &data,
                            FALSE,
                            in->inDuplicate.t.size, in->inDuplicate.t.buffer);
       if(result != TPM_RC_SUCCESS)
           return RcSafeAddToResult(result, RC_Rewrap_inDuplicate);

       // Copy unwrapped data to temporary variable, remove the integrity field
       hashSize = sizeof(UINT16) +
                  CryptGetHashDigestSize(oldParent->publicArea.nameAlg);
       privateBlob.t.size = in->inDuplicate.t.size - hashSize;
       MemoryCopy(privateBlob.t.buffer, in->inDuplicate.t.buffer + hashSize,
                  privateBlob.t.size, sizeof(privateBlob.t.buffer));
   }
   else
   {
       // No outer wrap from input blob.   Direct copy.
       privateBlob = in->inDuplicate;
   }

   if(in->newParent != TPM_RH_NULL)
   {
       OBJECT          *newParent;
       newParent = ObjectGet(in->newParent);

       // New parent must be a storage object
       if(!ObjectIsStorage(in->newParent))
           return TPM_RC_TYPE + RC_Rewrap_newParent;

       // Make new encrypt key and its associated secret structure. A
       // TPM_RC_VALUE error may be returned at this point if RSA algorithm is
       // enabled in TPM
       out->outSymSeed.t.size = sizeof(out->outSymSeed.t.secret);
       result = CryptSecretEncrypt(in->newParent,
                                   "DUPLICATE", &data, &out->outSymSeed);
       if(result != TPM_RC_SUCCESS) return result;

// Command output
       // Copy temporary variable to output, reserve the space for integrity
       hashSize = sizeof(UINT16) +
                  CryptGetHashDigestSize(newParent->publicArea.nameAlg);
       out->outDuplicate.t.size = privateBlob.t.size;
       MemoryCopy(out->outDuplicate.t.buffer + hashSize, privateBlob.t.buffer,
                  privateBlob.t.size, sizeof(out->outDuplicate.t.buffer));

       // Produce outer wrapper for output
       out->outDuplicate.t.size = ProduceOuterWrap(in->newParent, &in->name,
                                  newParent->publicArea.nameAlg,
                                  (TPM2B_SEED *) &data,
                                  FALSE,
                                  out->outDuplicate.t.size,
                                  out->outDuplicate.t.buffer);

   }
   else // New parent is a null key so there is no seed
   {
       out->outSymSeed.t.size = 0;

       // Copy privateBlob directly
       out->outDuplicate = privateBlob;
   }

   return TPM_RC_SUCCESS;
}
