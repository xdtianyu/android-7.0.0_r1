// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 3: Commands
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include "InternalRoutines.h"
#include "PolicyLocality_fp.h"
//
//     Limit a policy to a specific locality
//
//     Error Returns                       Meaning
//
//     TPM_RC_RANGE                        all the locality values selected by locality have been disabled by
//                                         previous TPM2_PolicyLocality() calls.
//
TPM_RC
TPM2_PolicyLocality(
    PolicyLocality_In        *in                 // IN: input parameter list
    )
{
    SESSION        *session;
    BYTE            marshalBuffer[sizeof(TPMA_LOCALITY)];
    BYTE            prevSetting[sizeof(TPMA_LOCALITY)];
    UINT32          marshalSize;
    BYTE           *buffer;
    INT32           bufferSize;
    TPM_CC          commandCode = TPM_CC_PolicyLocality;
    HASH_STATE      hashState;

// Input Validation

    // Get pointer to the session structure
    session = SessionGet(in->policySession);

    // Get new locality setting in canonical form
    buffer = marshalBuffer;
    bufferSize = sizeof(TPMA_LOCALITY);
    marshalSize = TPMA_LOCALITY_Marshal(&in->locality, &buffer, &bufferSize);

    // Its an error if the locality parameter is zero
    if(marshalBuffer[0] == 0)
        return TPM_RC_RANGE + RC_PolicyLocality_locality;

    // Get existing locality setting in canonical form
    buffer = prevSetting;
    bufferSize = sizeof(TPMA_LOCALITY);
    TPMA_LOCALITY_Marshal(&session->commandLocality, &buffer, &bufferSize);

    // If the locality has previously been set
    if(    prevSetting[0] != 0
        // then the current locality setting and the requested have to be the same
        // type (that is, either both normal or both extended
        && ((prevSetting[0] < 32) != (marshalBuffer[0] < 32)))
        return TPM_RC_RANGE + RC_PolicyLocality_locality;

    // See if the input is a regular or extended locality
    if(marshalBuffer[0] < 32)
    {
        // if there was no previous setting, start with all normal localities
        // enabled
        if(prevSetting[0] == 0)
            prevSetting[0] = 0x1F;

         // AND the new setting with the previous setting and store it in prevSetting
         prevSetting[0] &= marshalBuffer[0];

         // The result setting can not be 0
         if(prevSetting[0] == 0)
          return TPM_RC_RANGE + RC_PolicyLocality_locality;
  }
  else
  {
      // for extended locality
      // if the locality has already been set, then it must match the
      if(prevSetting[0] != 0 && prevSetting[0] != marshalBuffer[0])
          return TPM_RC_RANGE + RC_PolicyLocality_locality;

      // Setting is OK
      prevSetting[0] = marshalBuffer[0];

  }

// Internal Data Update

  // Update policy hash
  // policyDigestnew = hash(policyDigestold || TPM_CC_PolicyLocality || locality)
  // Start hash
  CryptStartHash(session->authHashAlg, &hashState);

  // add old digest
  CryptUpdateDigest2B(&hashState, &session->u2.policyDigest.b);

  // add commandCode
  CryptUpdateDigestInt(&hashState, sizeof(TPM_CC), &commandCode);

  // add input locality
  CryptUpdateDigest(&hashState, marshalSize, marshalBuffer);

  // complete the digest
  CryptCompleteHash2B(&hashState, &session->u2.policyDigest.b);

  // update session locality by unmarshal function. The function must succeed
  // because both input and existing locality setting have been validated.
  buffer = prevSetting;
  TPMA_LOCALITY_Unmarshal(&session->commandLocality, &buffer,
                          (INT32 *) &marshalSize);

   return TPM_RC_SUCCESS;
}
