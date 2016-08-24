// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 3: Commands
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include "InternalRoutines.h"
#include "Sign_fp.h"
#include "Attest_spt_fp.h"
//
//
//     Error Returns                     Meaning
//
//     TPM_RC_BINDING                    The public and private portions of the key are not properly bound.
//     TPM_RC_KEY                        signHandle does not reference a signing key;
//     TPM_RC_SCHEME                     the scheme is not compatible with sign key type, or input scheme is
//                                       not compatible with default scheme, or the chosen scheme is not a
//                                       valid sign scheme
//     TPM_RC_TICKET                     validation is not a valid ticket
//     TPM_RC_VALUE                      the value to sign is larger than allowed for the type of keyHandle
//
TPM_RC
TPM2_Sign(
   Sign_In          *in,                   // IN: input parameter list
   Sign_Out         *out                   // OUT: output parameter list
   )
{
   TPM_RC                     result;
   TPMT_TK_HASHCHECK          ticket;
   OBJECT                    *signKey;

// Input Validation
   // Get sign key pointer
   signKey = ObjectGet(in->keyHandle);

   // pick a scheme for sign. If the input sign scheme is not compatible with
   // the default scheme, return an error.
   result = CryptSelectSignScheme(in->keyHandle, &in->inScheme);
   if(result != TPM_RC_SUCCESS)
   {
       if(result == TPM_RC_KEY)
           return TPM_RC_KEY + RC_Sign_keyHandle;
       else
           return RcSafeAddToResult(result, RC_Sign_inScheme);
   }

   // If validation is provided, or the key is restricted, check the ticket
   if(   in->validation.digest.t.size != 0
      || signKey->publicArea.objectAttributes.restricted == SET)
   {
       // Compute and compare ticket
       TicketComputeHashCheck(in->validation.hierarchy,
                              in->inScheme.details.any.hashAlg,
                              &in->digest, &ticket);

       if(!Memory2BEqual(&in->validation.digest.b, &ticket.digest.b))
           return TPM_RC_TICKET + RC_Sign_validation;
   }
   else
   // If we don't have a ticket, at least verify that the provided 'digest'
   // is the size of the scheme hashAlg digest.
   // NOTE: this does not guarantee that the 'digest' is actually produced using
   // the indicated hash algorithm, but at least it might be.
   {
       if(       in->digest.t.size
             != CryptGetHashDigestSize(in->inScheme.details.any.hashAlg))
             return TPM_RC_SIZE + RC_Sign_digest;
   }

// Command Output
   // Sign the hash. A TPM_RC_VALUE or TPM_RC_SCHEME
   // error may be returned at this point
   result = CryptSign(in->keyHandle, &in->inScheme, &in->digest, &out->signature);

   return result;
}
