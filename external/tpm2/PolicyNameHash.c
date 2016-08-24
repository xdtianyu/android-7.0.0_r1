// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 3: Commands
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include "InternalRoutines.h"
#include "PolicyNameHash_fp.h"
//
//
//     Error Returns                     Meaning
//
//     TPM_RC_CPHASH                     nameHash has been previously set to a different value
//     TPM_RC_SIZE                       nameHash is not the size of the digest produced by the hash
//                                       algorithm associated with policySession
//
TPM_RC
TPM2_PolicyNameHash(
   PolicyNameHash_In     *in                  // IN: input parameter list
   )
{
   SESSION               *session;
   TPM_CC                 commandCode = TPM_CC_PolicyNameHash;
   HASH_STATE             hashState;

// Input Validation

   // Get pointer to the session structure
   session = SessionGet(in->policySession);

   // A new nameHash is given in input parameter, but cpHash in session context
   // is not empty
   if(in->nameHash.t.size != 0 && session->u1.cpHash.t.size != 0)
       return TPM_RC_CPHASH;

   // A valid nameHash must have the same size as session hash digest
   if(in->nameHash.t.size != CryptGetHashDigestSize(session->authHashAlg))
       return TPM_RC_SIZE + RC_PolicyNameHash_nameHash;

// Internal Data Update

   // Update policy hash
   // policyDigestnew = hash(policyDigestold || TPM_CC_PolicyNameHash || nameHash)
   // Start hash
   CryptStartHash(session->authHashAlg, &hashState);

   // add old digest
   CryptUpdateDigest2B(&hashState, &session->u2.policyDigest.b);

   // add commandCode
   CryptUpdateDigestInt(&hashState, sizeof(TPM_CC), &commandCode);

   // add nameHash
   CryptUpdateDigest2B(&hashState, &in->nameHash.b);

   // complete the digest
   CryptCompleteHash2B(&hashState, &session->u2.policyDigest.b);

   // clear iscpHashDefined bit to indicate now this field contains a nameHash
   session->attributes.iscpHashDefined = CLEAR;

   // update nameHash in session context
   session->u1.cpHash = in->nameHash;

   return TPM_RC_SUCCESS;
}
