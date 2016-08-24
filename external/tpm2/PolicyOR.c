// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 3: Commands
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include "InternalRoutines.h"
#include "PolicyOR_fp.h"
#include "Policy_spt_fp.h"
//
//
//     Error Returns                 Meaning
//
//     TPM_RC_VALUE                  no digest in pHashList matched the current value of policyDigest for
//                                   policySession
//
TPM_RC
TPM2_PolicyOR(
   PolicyOR_In      *in               // IN: input parameter list
   )
{
   SESSION       *session;
   UINT32         i;

// Input Validation and Update

   // Get pointer to the session structure
   session = SessionGet(in->policySession);

   // Compare and Update Internal Session policy if match
   for(i = 0; i < in->pHashList.count; i++)
   {
       if(   session->attributes.isTrialPolicy == SET
          || (Memory2BEqual(&session->u2.policyDigest.b,
                            &in->pHashList.digests[i].b))
         )
       {
           // Found a match
           HASH_STATE      hashState;
           TPM_CC          commandCode = TPM_CC_PolicyOR;

             // Start hash
             session->u2.policyDigest.t.size = CryptStartHash(session->authHashAlg,
                                                            &hashState);
             // Set policyDigest to 0 string and add it to hash
             MemorySet(session->u2.policyDigest.t.buffer, 0,
                       session->u2.policyDigest.t.size);
             CryptUpdateDigest2B(&hashState, &session->u2.policyDigest.b);

             // add command code
             CryptUpdateDigestInt(&hashState, sizeof(TPM_CC), &commandCode);

             // Add each of the hashes in the list
             for(i = 0; i < in->pHashList.count; i++)
             {
                 // Extend policyDigest
                 CryptUpdateDigest2B(&hashState, &in->pHashList.digests[i].b);
             }
             // Complete digest
             CryptCompleteHash2B(&hashState, &session->u2.policyDigest.b);

             return TPM_RC_SUCCESS;
       }
   }
   // None of the values in the list matched the current policyDigest
   return TPM_RC_VALUE + RC_PolicyOR_pHashList;
}
