// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 3: Commands
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include "InternalRoutines.h"
#include "PolicyPassword_fp.h"
#include "Policy_spt_fp.h"
TPM_RC
TPM2_PolicyPassword(
   PolicyPassword_In     *in            // IN: input parameter list
   )
{
   SESSION               *session;
   TPM_CC                 commandCode = TPM_CC_PolicyAuthValue;
   HASH_STATE             hashState;

// Internal Data Update

   // Get pointer to the session structure
   session = SessionGet(in->policySession);

   // Update policy hash
   // policyDigestnew = hash(policyDigestold || TPM_CC_PolicyAuthValue)
   // Start hash
   CryptStartHash(session->authHashAlg, &hashState);

   // add old digest
   CryptUpdateDigest2B(&hashState, &session->u2.policyDigest.b);

   // add commandCode
   CryptUpdateDigestInt(&hashState, sizeof(TPM_CC), &commandCode);

   // complete the digest
   CryptCompleteHash2B(&hashState, &session->u2.policyDigest.b);

   // Update isPasswordNeeded bit
   session->attributes.isPasswordNeeded = SET;
   session->attributes.isAuthValueNeeded = CLEAR;

   return TPM_RC_SUCCESS;
}
