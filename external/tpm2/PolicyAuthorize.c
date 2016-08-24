// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 3: Commands
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include "InternalRoutines.h"
#include "PolicyAuthorize_fp.h"
#include "Policy_spt_fp.h"
//
//
//     Error Returns                     Meaning
//
//     TPM_RC_HASH                       hash algorithm in keyName is not supported
//     TPM_RC_SIZE                       keyName is not the correct size for its hash algorithm
//     TPM_RC_VALUE                      the current policyDigest of policySession does not match
//                                       approvedPolicy; or checkTicket doesn't match the provided values
//
TPM_RC
TPM2_PolicyAuthorize(
   PolicyAuthorize_In    *in                   // IN: input parameter list
   )
{
   SESSION                     *session;
   TPM2B_DIGEST                 authHash;
   HASH_STATE                   hashState;
   TPMT_TK_VERIFIED             ticket;
   TPM_ALG_ID                   hashAlg;
   UINT16                       digestSize;

// Input Validation

   // Get pointer to the session structure
   session = SessionGet(in->policySession);

   // Extract from the Name of the key, the algorithm used to compute it's Name
   hashAlg = BYTE_ARRAY_TO_UINT16(in->keySign.t.name);

   // 'keySign' parameter needs to use a supported hash algorithm, otherwise
   // can't tell how large the digest should be
   digestSize = CryptGetHashDigestSize(hashAlg);
   if(digestSize == 0)
       return TPM_RC_HASH + RC_PolicyAuthorize_keySign;

   if(digestSize != (in->keySign.t.size - 2))
       return TPM_RC_SIZE + RC_PolicyAuthorize_keySign;

   //If this is a trial policy, skip all validations
   if(session->attributes.isTrialPolicy == CLEAR)
   {
       // Check that "approvedPolicy" matches the current value of the
       // policyDigest in policy session
       if(!Memory2BEqual(&session->u2.policyDigest.b,
                         &in->approvedPolicy.b))
           return TPM_RC_VALUE + RC_PolicyAuthorize_approvedPolicy;

         // Validate ticket TPMT_TK_VERIFIED
         // Compute aHash. The authorizing object sign a digest
         // aHash := hash(approvedPolicy || policyRef).
         // Start hash
         authHash.t.size = CryptStartHash(hashAlg, &hashState);

         // add approvedPolicy
         CryptUpdateDigest2B(&hashState, &in->approvedPolicy.b);

      // add policyRef
      CryptUpdateDigest2B(&hashState, &in->policyRef.b);

      // complete hash
      CryptCompleteHash2B(&hashState, &authHash.b);

      // re-compute TPMT_TK_VERIFIED
      TicketComputeVerified(in->checkTicket.hierarchy, &authHash,
                            &in->keySign, &ticket);

      // Compare ticket digest. If not match, return error
      if(!Memory2BEqual(&in->checkTicket.digest.b, &ticket.digest.b))
          return TPM_RC_VALUE+ RC_PolicyAuthorize_checkTicket;
  }

// Internal Data Update

  // Set policyDigest to zero digest
  MemorySet(session->u2.policyDigest.t.buffer, 0,
            session->u2.policyDigest.t.size);

  // Update policyDigest
  PolicyContextUpdate(TPM_CC_PolicyAuthorize, &in->keySign, &in->policyRef,
                      NULL, 0, session);

  return TPM_RC_SUCCESS;

}
