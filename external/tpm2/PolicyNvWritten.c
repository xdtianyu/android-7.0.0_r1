// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 3: Commands
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include "InternalRoutines.h"
#include "PolicyNvWritten_fp.h"
//
//     Make an NV Index policy dependent on the state of the TPMA_NV_WRITTEN attribute of the index.
//
//     Error Returns                   Meaning
//
//     TPM_RC_VALUE                    a conflicting request for the attribute has already been processed
//
TPM_RC
TPM2_PolicyNvWritten(
   PolicyNvWritten_In    *in                 // IN: input parameter list
   )
{
   SESSION      *session;
   TPM_CC        commandCode = TPM_CC_PolicyNvWritten;
   HASH_STATE    hashState;

// Input Validation

   // Get pointer to the session structure
   session = SessionGet(in->policySession);

   // If already set is this a duplicate (the same setting)? If it
   // is a conflicting setting, it is an error
   if(session->attributes.checkNvWritten == SET)
   {
       if((    (session->attributes.nvWrittenState == SET)
           != (in->writtenSet == YES)))
           return TPM_RC_VALUE + RC_PolicyNvWritten_writtenSet;
   }

// Internal Data Update

   // Set session attributes so that the NV Index needs to be checked
   session->attributes.checkNvWritten = SET;
   session->attributes.nvWrittenState = (in->writtenSet == YES);

   // Update policy hash
   // policyDigestnew = hash(policyDigestold || TPM_CC_PolicyNvWritten
   //                          || writtenSet)
   // Start hash
   CryptStartHash(session->authHashAlg, &hashState);

   // add old digest
   CryptUpdateDigest2B(&hashState, &session->u2.policyDigest.b);

   // add commandCode
   CryptUpdateDigestInt(&hashState, sizeof(TPM_CC), &commandCode);

   // add the byte of writtenState
   CryptUpdateDigestInt(&hashState, sizeof(TPMI_YES_NO), &in->writtenSet);

   // complete the digest
   CryptCompleteHash2B(&hashState, &session->u2.policyDigest.b);

   return TPM_RC_SUCCESS;
}
