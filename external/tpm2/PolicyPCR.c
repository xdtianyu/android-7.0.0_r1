// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 3: Commands
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include "InternalRoutines.h"
#include "PolicyPCR_fp.h"
//
//
//     Error Returns                     Meaning
//
//     TPM_RC_VALUE                      if provided, pcrDigest does not match the current PCR settings
//     TPM_RC_PCR_CHANGED                a previous TPM2_PolicyPCR() set pcrCounter and it has changed
//
TPM_RC
TPM2_PolicyPCR(
   PolicyPCR_In      *in                 // IN: input parameter list
   )
{
   SESSION           *session;
   TPM2B_DIGEST       pcrDigest;
   BYTE               pcrs[sizeof(TPML_PCR_SELECTION)];
   UINT32             pcrSize;
   BYTE              *buffer;
   INT32              bufferSize;
   TPM_CC             commandCode = TPM_CC_PolicyPCR;
   HASH_STATE         hashState;

// Input Validation

   // Get pointer to the session structure
   session = SessionGet(in->policySession);

   // Do validation for non trial session
   if(session->attributes.isTrialPolicy == CLEAR)
   {
       // Make sure that this is not going to invalidate a previous PCR check
       if(session->pcrCounter != 0 && session->pcrCounter != gr.pcrCounter)
           return TPM_RC_PCR_CHANGED;

       // Compute current PCR digest
       PCRComputeCurrentDigest(session->authHashAlg, &in->pcrs, &pcrDigest);

       // If the caller specified the PCR digest and it does not
       // match the current PCR settings, return an error..
       if(in->pcrDigest.t.size != 0)
       {
           if(!Memory2BEqual(&in->pcrDigest.b, &pcrDigest.b))
               return TPM_RC_VALUE + RC_PolicyPCR_pcrDigest;
       }
   }
   else
   {
       // For trial session, just use the input PCR digest
       pcrDigest = in->pcrDigest;
   }
// Internal Data Update

   // Update policy hash
   // policyDigestnew = hash(   policyDigestold || TPM_CC_PolicyPCR
   //                      || pcrs || pcrDigest)
   // Start hash
   CryptStartHash(session->authHashAlg, &hashState);

   // add old digest
   CryptUpdateDigest2B(&hashState, &session->u2.policyDigest.b);

  // add commandCode
  CryptUpdateDigestInt(&hashState, sizeof(TPM_CC), &commandCode);

  // add PCRS
  buffer = pcrs;
  bufferSize = sizeof(TPML_PCR_SELECTION);
  pcrSize = TPML_PCR_SELECTION_Marshal(&in->pcrs, &buffer, &bufferSize);
  CryptUpdateDigest(&hashState, pcrSize, pcrs);

  // add PCR digest
  CryptUpdateDigest2B(&hashState, &pcrDigest.b);

  // complete the hash and get the results
  CryptCompleteHash2B(&hashState, &session->u2.policyDigest.b);

  // update pcrCounter in session context for non trial session
  if(session->attributes.isTrialPolicy == CLEAR)
  {
      session->pcrCounter = gr.pcrCounter;
  }

   return TPM_RC_SUCCESS;
}
