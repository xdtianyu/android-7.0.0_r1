// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 3: Commands
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include "InternalRoutines.h"
#include "Commit_fp.h"
#ifdef TPM_ALG_ECC
//
//
//     Error Returns                     Meaning
//
//     TPM_RC_ATTRIBUTES                 keyHandle references a restricted key that is not a signing key
//     TPM_RC_ECC_POINT                  either P1 or the point derived from s2 is not on the curve of
//                                       keyHandle
//     TPM_RC_HASH                       invalid name algorithm in keyHandle
//     TPM_RC_KEY                        keyHandle does not reference an ECC key
//     TPM_RC_SCHEME                     the scheme of keyHandle is not an anonymous scheme
//     TPM_RC_NO_RESULT                  K, L or E was a point at infinity; or failed to generate r value
//     TPM_RC_SIZE                       s2 is empty but y2 is not or s2 provided but y2 is not
//
TPM_RC
TPM2_Commit(
   Commit_In         *in,                 // IN: input parameter list
   Commit_Out        *out                 // OUT: output parameter list
   )
{
   OBJECT                    *eccKey;
   TPMS_ECC_POINT             P2;
   TPMS_ECC_POINT            *pP2 = NULL;
   TPMS_ECC_POINT            *pP1 = NULL;
   TPM2B_ECC_PARAMETER        r;
   TPM2B                     *p;
   TPM_RC                     result;
   TPMS_ECC_PARMS            *parms;

// Input Validation

   eccKey = ObjectGet(in->signHandle);
   parms = & eccKey->publicArea.parameters.eccDetail;

   // Input key must be an ECC key
   if(eccKey->publicArea.type != TPM_ALG_ECC)
       return TPM_RC_KEY + RC_Commit_signHandle;

    // This command may only be used with a sign-only key using an anonymous
    // scheme.
    // NOTE: a sign + decrypt key has no scheme so it will not be an anonymous one
    // and an unrestricted sign key might no have a signing scheme but it can't
    // be use in Commit()
   if(!CryptIsSchemeAnonymous(parms->scheme.scheme))
            return TPM_RC_SCHEME + RC_Commit_signHandle;

   // Make sure that both parts of P2 are present if either is present
   if((in->s2.t.size == 0) != (in->y2.t.size == 0))
       return TPM_RC_SIZE + RC_Commit_y2;

   // Get prime modulus for the curve. This is needed later but getting this now
   // allows confirmation that the curve exists
   p = (TPM2B *)CryptEccGetParameter('p', parms->curveID);

   // if no p, then the curve ID is bad
//
  // NOTE: This should never occur if the input unmarshaling code is working
  // correctly
  pAssert(p != NULL);

  // Get the random value that will be used in the point multiplications
  // Note: this does not commit the count.
  if(!CryptGenerateR(&r, NULL, parms->curveID, &eccKey->name))
      return TPM_RC_NO_RESULT;

  // Set up P2 if s2 and Y2 are provided
  if(in->s2.t.size != 0)
  {
      pP2 = &P2;

      // copy y2 for P2
      MemoryCopy2B(&P2.y.b, &in->y2.b, sizeof(P2.y.t.buffer));
      // Compute x2 HnameAlg(s2) mod p

      //      do the hash operation on s2 with the size of curve 'p'
      P2.x.t.size = CryptHashBlock(eccKey->publicArea.nameAlg,
                                   in->s2.t.size,
                                   in->s2.t.buffer,
                                   p->size,
                                   P2.x.t.buffer);

      // If there were error returns in the hash routine, indicate a problem
      // with the hash in
      if(P2.x.t.size == 0)
          return TPM_RC_HASH + RC_Commit_signHandle;

      // set p2.x = hash(s2) mod p
      if(CryptDivide(&P2.x.b, p, NULL, &P2.x.b) != TPM_RC_SUCCESS)
          return TPM_RC_NO_RESULT;

      if(!CryptEccIsPointOnCurve(parms->curveID, pP2))
          return TPM_RC_ECC_POINT + RC_Commit_s2;

      if(eccKey->attributes.publicOnly == SET)
          return TPM_RC_KEY + RC_Commit_signHandle;

  }
  // If there is a P1, make sure that it is on the curve
  // NOTE: an "empty" point has two UINT16 values which are the size values
  // for each of the coordinates.
  if(in->P1.t.size > 4)
  {
      pP1 = &in->P1.t.point;
      if(!CryptEccIsPointOnCurve(parms->curveID, pP1))
          return TPM_RC_ECC_POINT + RC_Commit_P1;
  }

  // Pass the parameters to CryptCommit.
  // The work is not done in-line because it does several point multiplies
  // with the same curve. There is significant optimization by not
  // having to reload the curve parameters multiple times.
  result = CryptCommitCompute(&out->K.t.point,
                              &out->L.t.point,
                              &out->E.t.point,
                              parms->curveID,
                              pP1,
                              pP2,
                              &eccKey->sensitive.sensitive.ecc,
                              &r);
  if(result != TPM_RC_SUCCESS)
      return result;

   out->K.t.size = TPMS_ECC_POINT_Marshal(&out->K.t.point, NULL, NULL);
   out->L.t.size = TPMS_ECC_POINT_Marshal(&out->L.t.point, NULL, NULL);
   out->E.t.size = TPMS_ECC_POINT_Marshal(&out->E.t.point, NULL, NULL);

   // The commit computation was successful so complete the commit by setting
   // the bit
   out->counter = CryptCommit();

   return TPM_RC_SUCCESS;
}
#endif
