// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 3: Commands
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include "InternalRoutines.h"
#include "Policy_spt_fp.h"
#include "PolicySigned_fp.h"
//
//
//     Error Returns                     Meaning
//
//     TPM_RC_CPHASH                     cpHash was previously set to a different value
//     TPM_RC_EXPIRED                    expiration indicates a time in the past or expiration is non-zero but no
//                                       nonceTPM is present
//     TPM_RC_HANDLE                     authObject need to have sensitive portion loaded
//     TPM_RC_KEY                        authObject is not a signing scheme
//     TPM_RC_NONCE                      nonceTPM is not the nonce associated with the policySession
//     TPM_RC_SCHEME                     the signing scheme of auth is not supported by the TPM
//     TPM_RC_SIGNATURE                  the signature is not genuine
//     TPM_RC_SIZE                       input cpHash has wrong size
//     TPM_RC_VALUE                      input policyID or expiration does not match the internal data in policy
//                                       session
//
TPM_RC
TPM2_PolicySigned(
   PolicySigned_In       *in,                  // IN: input parameter list
   PolicySigned_Out      *out                  // OUT: output parameter list
   )
{
   TPM_RC                     result = TPM_RC_SUCCESS;
   SESSION                   *session;
   TPM2B_NAME                 entityName;
   TPM2B_DIGEST               authHash;
   HASH_STATE                 hashState;
   UINT32                     expiration = (in->expiration < 0)
                                           ? -(in->expiration) : in->expiration;
   UINT64                     authTimeout = 0;

// Input Validation

   // Set up local pointers
   session = SessionGet(in->policySession);               // the session structure

   // Only do input validation if this is not a trial policy session
   if(session->attributes.isTrialPolicy == CLEAR)
   {
       if(expiration != 0)
           authTimeout = expiration * 1000 + session->startTime;

       result = PolicyParameterChecks(session, authTimeout,
                                       &in->cpHashA, &in->nonceTPM,
                                       RC_PolicySigned_nonceTPM,
                                       RC_PolicySigned_cpHashA,
                                       RC_PolicySigned_expiration);
       if(result != TPM_RC_SUCCESS)
           return result;

       // Re-compute the digest being signed
       /*(See part 3 specification)
       // The digest is computed as:
       //     aHash := hash ( nonceTPM | expiration | cpHashA | policyRef)
       // where:
       //      hash()      the hash associated with the signed auth
       //      nonceTPM    the nonceTPM value from the TPM2_StartAuthSession .
       //                  response If the authorization is not limited to this
       //                  session, the size of this value is zero.
       //      expiration time limit on authorization set by authorizing object.
       //                  This 32-bit value is set to zero if the expiration
       //                  time is not being set.
       //      cpHashA     hash of the command parameters for the command being
       //                  approved using the hash algorithm of the PSAP session.
       //                  Set to NULLauth if the authorization is not limited
       //                  to a specific command.
       //      policyRef   hash of an opaque value determined by the authorizing
       //                  object. Set to the NULLdigest if no hash is present.
       */
       // Start hash
       authHash.t.size = CryptStartHash(CryptGetSignHashAlg(&in->auth),
                                        &hashState);

       // add nonceTPM
       CryptUpdateDigest2B(&hashState, &in->nonceTPM.b);

       // add expiration
       CryptUpdateDigestInt(&hashState, sizeof(UINT32), (BYTE*) &in->expiration);

       // add cpHashA
       CryptUpdateDigest2B(&hashState, &in->cpHashA.b);

       // add policyRef
       CryptUpdateDigest2B(&hashState, &in->policyRef.b);

       // Complete digest
       CryptCompleteHash2B(&hashState, &authHash.b);

       // Validate Signature. A TPM_RC_SCHEME, TPM_RC_HANDLE or TPM_RC_SIGNATURE
       // error may be returned at this point
       result = CryptVerifySignature(in->authObject, &authHash, &in->auth);
       if(result != TPM_RC_SUCCESS)
           return RcSafeAddToResult(result, RC_PolicySigned_auth);
   }
// Internal Data Update
   // Need the Name of the signing entity
   entityName.t.size = EntityGetName(in->authObject, &entityName.t.name);

   // Update policy with input policyRef and name of auth key
   // These values are updated even if the session is a trial session
   PolicyContextUpdate(TPM_CC_PolicySigned, &entityName, &in->policyRef,
                       &in->cpHashA, authTimeout, session);

// Command Output

   // Create ticket and timeout buffer if in->expiration < 0 and this is not
   // a trial session.
   // NOTE: PolicyParameterChecks() makes sure that nonceTPM is present
   // when expiration is non-zero.
   if(   in->expiration < 0
      && session->attributes.isTrialPolicy == CLEAR
     )
   {
       // Generate timeout buffer. The format of output timeout buffer is
       // TPM-specific.
       // Note: can't do a direct copy because the output buffer is a byte
       // array and it may not be aligned to accept a 64-bit value. The method
       // used has the side-effect of making the returned value a big-endian,
       // 64-bit value that is byte aligned.
       out->timeout.t.size = sizeof(UINT64);
       UINT64_TO_BYTE_ARRAY(authTimeout, out->timeout.t.buffer);

       // Compute policy ticket
       TicketComputeAuth(TPM_ST_AUTH_SIGNED, EntityGetHierarchy(in->authObject),
                         authTimeout, &in->cpHashA, &in->policyRef, &entityName,
                         &out->policyTicket);
   }
   else
   {
       // Generate a null ticket.
       // timeout buffer is null
       out->timeout.t.size = 0;

       // auth ticket is null
       out->policyTicket.tag = TPM_ST_AUTH_SIGNED;
       out->policyTicket.hierarchy = TPM_RH_NULL;
       out->policyTicket.digest.t.size = 0;
   }

   return TPM_RC_SUCCESS;
}
