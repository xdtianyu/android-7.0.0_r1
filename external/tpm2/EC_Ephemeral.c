// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 3: Commands
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include "InternalRoutines.h"
#include "EC_Ephemeral_fp.h"
#ifdef TPM_ALG_ECC
//
//
//     Error Returns                    Meaning
//
//     none                             ...
//
TPM_RC
TPM2_EC_Ephemeral(
   EC_Ephemeral_In       *in,               // IN: input parameter list
   EC_Ephemeral_Out      *out               // OUT: output parameter list
   )
{
   TPM2B_ECC_PARAMETER          r;

   // Get the random value that will be used in the point multiplications
   // Note: this does not commit the count.
   if(!CryptGenerateR(&r,
                      NULL,
                      in->curveID,
                      NULL))
       return TPM_RC_NO_RESULT;

   CryptEccPointMultiply(&out->Q.t.point, in->curveID, &r, NULL);

   // commit the count value
   out->counter = CryptCommit();

   return TPM_RC_SUCCESS;
}
#endif
