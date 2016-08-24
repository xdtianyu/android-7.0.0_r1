// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 3: Commands
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include "InternalRoutines.h"
#include "PolicyCommandCode_fp.h"
//
//
//     Error Returns                     Meaning
//
//     TPM_RC_VALUE                      commandCode of policySession previously set to a different value
//
TPM_RC
TPM2_PolicyCommandCode(
   PolicyCommandCode_In      *in                   // IN: input parameter list
   )
{
   SESSION      *session;
   TPM_CC       commandCode = TPM_CC_PolicyCommandCode;
   HASH_STATE   hashState;

// Input validation

   // Get pointer to the session structure
   session = SessionGet(in->policySession);

   if(session->commandCode != 0 && session->commandCode != in->code)
       return TPM_RC_VALUE + RC_PolicyCommandCode_code;
   if(!CommandIsImplemented(in->code))
       return TPM_RC_POLICY_CC + RC_PolicyCommandCode_code;

// Internal Data Update
   // Update policy hash
   // policyDigestnew = hash(policyDigestold || TPM_CC_PolicyCommandCode || code)
   // Start hash
   CryptStartHash(session->authHashAlg, &hashState);

   // add old digest
   CryptUpdateDigest2B(&hashState, &session->u2.policyDigest.b);

   // add commandCode
   CryptUpdateDigestInt(&hashState, sizeof(TPM_CC), &commandCode);

   // add input commandCode
   CryptUpdateDigestInt(&hashState, sizeof(TPM_CC), &in->code);

   // complete the hash and get the results
   CryptCompleteHash2B(&hashState, &session->u2.policyDigest.b);

   // update commandCode value in session context
   session->commandCode = in->code;

   return TPM_RC_SUCCESS;
}
