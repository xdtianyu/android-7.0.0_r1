// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 4: Supporting Routines
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include   "InternalRoutines.h"
#include   "Policy_spt_fp.h"
#include   "PolicySigned_fp.h"
#include   "PolicySecret_fp.h"
#include   "PolicyTicket_fp.h"
//
//
//           PolicyParameterChecks()
//
//      This function validates the common parameters of TPM2_PolicySiged() and TPM2_PolicySecret(). The
//      common parameters are nonceTPM, expiration, and cpHashA.
//
TPM_RC
PolicyParameterChecks(
     SESSION          *session,
     UINT64            authTimeout,
     TPM2B_DIGEST     *cpHashA,
     TPM2B_NONCE      *nonce,
     TPM_RC            nonceParameterNumber,
     TPM_RC            cpHashParameterNumber,
     TPM_RC            expirationParameterNumber
     )
{
     TPM_RC            result;
     // Validate that input nonceTPM is correct if present
     if(nonce != NULL && nonce->t.size != 0)
//
   {
         if(!Memory2BEqual(&nonce->b, &session->nonceTPM.b))
             return TPM_RC_NONCE + RC_PolicySigned_nonceTPM;
   }
   // If authTimeout is set (expiration != 0...
   if(authTimeout != 0)
   {
       // ...then nonce must be present
       // nonce present isn't checked in PolicyTicket
       if(nonce != NULL && nonce->t.size == 0)
           // This error says that the time has expired but it is pointing
           // at the nonceTPM value.
           return TPM_RC_EXPIRED + nonceParameterNumber;
         // Validate input expiration.
         // Cannot compare time if clock stop advancing. A TPM_RC_NV_UNAVAILABLE
         // or TPM_RC_NV_RATE error may be returned here.
         result = NvIsAvailable();
         if(result != TPM_RC_SUCCESS)
             return result;
         if(authTimeout < go.clock)
             return TPM_RC_EXPIRED + expirationParameterNumber;
   }
   // If the cpHash is present, then check it
   if(cpHashA != NULL && cpHashA->t.size != 0)
   {
       // The cpHash input has to have the correct size
       if(cpHashA->t.size != session->u2.policyDigest.t.size)
           return TPM_RC_SIZE + cpHashParameterNumber;
         // If the cpHash has already been set, then this input value
         // must match the current value.
         if(     session->u1.cpHash.b.size != 0
             && !Memory2BEqual(&cpHashA->b, &session->u1.cpHash.b))
                 return TPM_RC_CPHASH;
   }
   return TPM_RC_SUCCESS;
}
//
//
//          PolicyContextUpdate()
//
//     Update policy hash Update the policyDigest in policy session by extending policyRef and objectName to
//     it. This will also update the cpHash if it is present.
//
void
PolicyContextUpdate(
   TPM_CC              commandCode,     //   IN:   command code
   TPM2B_NAME         *name,            //   IN:   name of entity
   TPM2B_NONCE        *ref,             //   IN:   the reference data
   TPM2B_DIGEST       *cpHash,          //   IN:   the cpHash (optional)
   UINT64              policyTimeout,
   SESSION            *session          // IN/OUT: policy session to be updated
   )
{
   HASH_STATE               hashState;
   UINT16                   policyDigestSize;
   // Start hash
   policyDigestSize = CryptStartHash(session->authHashAlg, &hashState);
   // policyDigest size should always be the digest size of session hash algorithm.
   pAssert(session->u2.policyDigest.t.size == policyDigestSize);
     // add old digest
     CryptUpdateDigest2B(&hashState, &session->u2.policyDigest.b);
     // add commandCode
     CryptUpdateDigestInt(&hashState, sizeof(commandCode), &commandCode);
     // add name if applicable
     if(name != NULL)
         CryptUpdateDigest2B(&hashState, &name->b);
     // Complete the digest and get the results
     CryptCompleteHash2B(&hashState, &session->u2.policyDigest.b);
     // Start second hash computation
     CryptStartHash(session->authHashAlg, &hashState);
     // add policyDigest
     CryptUpdateDigest2B(&hashState, &session->u2.policyDigest.b);
     // add policyRef
     if(ref != NULL)
         CryptUpdateDigest2B(&hashState, &ref->b);
     // Complete second digest
     CryptCompleteHash2B(&hashState, &session->u2.policyDigest.b);
     // Deal with the cpHash. If the cpHash value is present
     // then it would have already been checked to make sure that
     // it is compatible with the current value so all we need
     // to do here is copy it and set the iscoHashDefined attribute
     if(cpHash != NULL && cpHash->t.size != 0)
     {
         session->u1.cpHash = *cpHash;
         session->attributes.iscpHashDefined = SET;
     }
     // update the timeout if it is specified
     if(policyTimeout!= 0)
     {
     // If the timeout has not been set, then set it to the new value
         if(session->timeOut == 0)
             session->timeOut = policyTimeout;
         else if(session->timeOut > policyTimeout)
             session->timeOut = policyTimeout;
     }
     return;
}
