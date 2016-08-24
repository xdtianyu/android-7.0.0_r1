// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 3: Commands
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include "InternalRoutines.h"
#include "StirRandom_fp.h"
TPM_RC
TPM2_StirRandom(
   StirRandom_In   *in            // IN: input parameter list
   )
{
// Internal Data Update
   CryptStirRandom(in->inData.t.size, in->inData.t.buffer);

   return TPM_RC_SUCCESS;
}
