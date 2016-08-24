// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 3: Commands
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include "InternalRoutines.h"
#include "ECC_Parameters_fp.h"
#ifdef TPM_ALG_ECC
//
//
//     Error Returns                     Meaning
//
//     TPM_RC_VALUE                      Unsupported ECC curve ID
//
TPM_RC
TPM2_ECC_Parameters(
   ECC_Parameters_In     *in,                // IN: input parameter list
   ECC_Parameters_Out    *out                // OUT: output parameter list
   )
{
// Command Output

   // Get ECC curve parameters
   if(CryptEccGetParameters(in->curveID, &out->parameters))
       return TPM_RC_SUCCESS;
   else
       return TPM_RC_VALUE + RC_ECC_Parameters_curveID;
}
#endif
