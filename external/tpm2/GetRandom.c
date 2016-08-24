// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 3: Commands
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include "InternalRoutines.h"
#include "GetRandom_fp.h"
TPM_RC
TPM2_GetRandom(
   GetRandom_In     *in,            // IN: input parameter list
   GetRandom_Out    *out            // OUT: output parameter list
   )
{
// Command Output

   // if the requested bytes exceed the output buffer size, generates the
   // maximum bytes that the output buffer allows
   if(in->bytesRequested > sizeof(TPMU_HA))
       out->randomBytes.t.size = sizeof(TPMU_HA);
   else
       out->randomBytes.t.size = in->bytesRequested;

   CryptGenerateRandom(out->randomBytes.t.size, out->randomBytes.t.buffer);

   return TPM_RC_SUCCESS;
}
