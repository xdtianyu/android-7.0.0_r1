// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 3: Commands
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include "InternalRoutines.h"
#include "PolicyCpHash_fp.h"
//
//
//     Error Returns                     Meaning
//
//     TPM_RC_CPHASH                     cpHash of policySession has previously been set to a different value
//     TPM_RC_SIZE                       cpHashA is not the size of a digest produced by the hash algorithm
//                                       associated with policySession
//
TPM_RC
TPM2_PolicyCpHash(
   PolicyCpHash_In       *in                   // IN: input parameter list
   )
{
   SESSION      *session;
   TPM_CC       commandCode = TPM_CC_PolicyCpHash;
   HASH_STATE   hashState;

// Input Validation

   // Get pointer to the session structure
   session = SessionGet(in->policySession);

   // A new cpHash is given in input parameter, but cpHash in session context
   // is not empty, or is not the same as the new cpHash
   if(    in->cpHashA.t.size != 0
       && session->u1.cpHash.t.size != 0
       && !Memory2BEqual(&in->cpHashA.b, &session->u1.cpHash.b)
      )
       return TPM_RC_CPHASH;

   // A valid cpHash must have the same size as session hash digest
   if(in->cpHashA.t.size != CryptGetHashDigestSize(session->authHashAlg))
       return TPM_RC_SIZE + RC_PolicyCpHash_cpHashA;

// Internal Data Update

   // Update policy hash
   // policyDigestnew = hash(policyDigestold || TPM_CC_PolicyCpHash || cpHashA)
   // Start hash
   CryptStartHash(session->authHashAlg, &hashState);

   // add old digest
   CryptUpdateDigest2B(&hashState, &session->u2.policyDigest.b);

   // add commandCode
   CryptUpdateDigestInt(&hashState, sizeof(TPM_CC), &commandCode);

   // add cpHashA
   CryptUpdateDigest2B(&hashState, &in->cpHashA.b);

   // complete the digest and get the results
   CryptCompleteHash2B(&hashState, &session->u2.policyDigest.b);

   // update cpHash in session context
   session->u1.cpHash = in->cpHashA;
   session->attributes.iscpHashDefined = SET;

   return TPM_RC_SUCCESS;
}
