// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 3: Commands
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include "InternalRoutines.h"
#include "ECDH_ZGen_fp.h"
#ifdef TPM_ALG_ECC
//
//
//     Error Returns                     Meaning
//
//     TPM_RC_ATTRIBUTES                 key referenced by keyA is restricted or not a decrypt key
//     TPM_RC_KEY                        key referenced by keyA is not an ECC key
//     TPM_RC_NO_RESULT                  multiplying inPoint resulted in a point at infinity
//     TPM_RC_SCHEME                     the scheme of the key referenced by keyA is not TPM_ALG_NULL,
//                                       TPM_ALG_ECDH,
//
TPM_RC
TPM2_ECDH_ZGen(
   ECDH_ZGen_In      *in,                  // IN: input parameter list
   ECDH_ZGen_Out     *out                  // OUT: output parameter list
   )
{
   TPM_RC                     result;
   OBJECT                    *eccKey;

// Input Validation

   eccKey = ObjectGet(in->keyHandle);

   // Input key must be a non-restricted, decrypt ECC key
   if(   eccKey->publicArea.type != TPM_ALG_ECC)
       return TPM_RC_KEY + RC_ECDH_ZGen_keyHandle;

   if(     eccKey->publicArea.objectAttributes.restricted == SET
      ||   eccKey->publicArea.objectAttributes.decrypt != SET
     )
       return TPM_RC_KEY + RC_ECDH_ZGen_keyHandle;

   // Make sure the scheme allows this use
   if(     eccKey->publicArea.parameters.eccDetail.scheme.scheme != TPM_ALG_ECDH
       && eccKey->publicArea.parameters.eccDetail.scheme.scheme != TPM_ALG_NULL)
       return TPM_RC_SCHEME + RC_ECDH_ZGen_keyHandle;

// Command Output

   // Compute Z. TPM_RC_ECC_POINT or TPM_RC_NO_RESULT may be returned here.
   result = CryptEccPointMultiply(&out->outPoint.t.point,
                                  eccKey->publicArea.parameters.eccDetail.curveID,
                                  &eccKey->sensitive.sensitive.ecc,
                                  &in->inPoint.t.point);
   if(result != TPM_RC_SUCCESS)
       return RcSafeAddToResult(result, RC_ECDH_ZGen_inPoint);

   out->outPoint.t.size = TPMS_ECC_POINT_Marshal(&out->outPoint.t.point,
                                                 NULL, NULL);

   return TPM_RC_SUCCESS;
}
#endif
