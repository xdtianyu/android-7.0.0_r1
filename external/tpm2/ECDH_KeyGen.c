// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 3: Commands
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include "InternalRoutines.h"
#include "ECDH_KeyGen_fp.h"
#ifdef TPM_ALG_ECC
//
//
//     Error Returns                     Meaning
//
//     TPM_RC_KEY                        keyHandle does not reference a non-restricted decryption ECC key
//
TPM_RC
TPM2_ECDH_KeyGen(
   ECDH_KeyGen_In        *in,                 // IN: input parameter list
   ECDH_KeyGen_Out       *out                 // OUT: output parameter list
   )
{
   OBJECT                    *eccKey;
   TPM2B_ECC_PARAMETER        sensitive;
   TPM_RC                     result;

// Input Validation

   eccKey = ObjectGet(in->keyHandle);

   // Input key must be a non-restricted, decrypt ECC key
   if(   eccKey->publicArea.type != TPM_ALG_ECC)
       return TPM_RC_KEY + RC_ECDH_KeyGen_keyHandle;

   if(     eccKey->publicArea.objectAttributes.restricted == SET
      ||   eccKey->publicArea.objectAttributes.decrypt != SET
     )
       return TPM_RC_KEY + RC_ECDH_KeyGen_keyHandle;

// Command Output
   do
   {
       // Create ephemeral ECC key
       CryptNewEccKey(eccKey->publicArea.parameters.eccDetail.curveID,
                      &out->pubPoint.t.point, &sensitive);

       out->pubPoint.t.size = TPMS_ECC_POINT_Marshal(&out->pubPoint.t.point,
                              NULL, NULL);

       // Compute Z
       result = CryptEccPointMultiply(&out->zPoint.t.point,
                                  eccKey->publicArea.parameters.eccDetail.curveID,
                                  &sensitive, &eccKey->publicArea.unique.ecc);
       // The point in the key is not on the curve. Indicate that the key is bad.
       if(result == TPM_RC_ECC_POINT)
           return TPM_RC_KEY + RC_ECDH_KeyGen_keyHandle;
       // The other possible error is TPM_RC_NO_RESULT indicating that the
       // multiplication resulted in the point at infinity, so get a new
       // random key and start over (hardly ever happens).
   }
   while(result == TPM_RC_NO_RESULT);

   if(result == TPM_RC_SUCCESS)
       // Marshal the values to generate the point.
       out->zPoint.t.size = TPMS_ECC_POINT_Marshal(&out->zPoint.t.point,
                                                   NULL, NULL);

   return result;
}
#endif
